// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QKmdDevice.h"
#include "AICMetadataReader.h"
#include "QDmaElement.h"
#include "QVCFactoryInterface.h"
#include "QVirtualChannelInterface.h"
#include "QNncProtocol.h"
#include "QOsBuffer.h"
#include "QOsal.h"
#include "QPlatformCommon.h"
#include "QUtil.h"
#include "QDevAic100Interface.h"
#include "elfio/elfio.hpp"

#include <fcntl.h>
#include <sstream>
#include <unistd.h>
#include <memory>
#include <cstdlib>

uint32_t globalImgFormat = 0; // 0=ELF, 1=BIN

namespace qaic {

const uint32_t NncImageTag = 100;
const uint32_t NncMetaTag = 200;
const uint32_t NncConstantsTag = 300;
const uint32_t MaxVCID = 15;
const uint32_t DefaultVCQueueSizePow2 = 9; // 512
// The size of the gap between request and response queues
const uint32_t QueueBufDMZSize = 0xFF;
// The alignment boundary for the buffer of request+response queues
const uint32_t QueueBufAlignSize = 0x1000;
// Psuedo registers mapped to user space. 4 registers with 4 bytes each.
const uint32_t VCRegRegionSize = 16;

static const std::string QAicDeviceMutexNamePrefix = "QAicSerializeDevice";
static const std::string QAicSerializeDeviceEnv = "QAIC_SERIALIZE_DEVICE";

//
// Helper function to mmap one buffer for both request and response queues.
// KMD already allocate a buffer to hold the queues during activation. The size
// of the buffer is determined by the following formula:
//
//    Align4K((reqElemSize + respElemSize) * (1 << qSizePow2) + QeueuBufDMZSize)
//
// Request queue is at the lower end of the buffer, response queue the higher
// end, with a DMZ zone of at least QueueBufDMSize between the two queues.
//
// The offset parameter in mmap indicate the buffer to be requested. The coding
// schemem is:
//   bit 24-27: VC #
//   bit 28: 0 for queue buffer, 1 for VC queue register base
//
[[maybe_unused]] static uint8_t *mmapQueueBuffer(int fd, uint32_t vc,
                                                 uint32_t qSizePow2,
                                                 uint8_t **rspQAddr,
                                                 uint32_t &bufSize) {
  uint32_t rspQSize = (1 << qSizePow2) * QDmaResponseElement::getSize();
  bufSize = (1 << qSizePow2) * QDmaRequestElement::getSize() + rspQSize;
  bufSize = (bufSize + QueueBufDMZSize + QueueBufAlignSize - 1) &
            ~(QueueBufAlignSize - 1);
  uint32_t offset = (vc << 24);

  void *base = mmap(0, bufSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, offset);

  if (base != (void *)-1) {
    *rspQAddr = (uint8_t *)base + bufSize - rspQSize;
  }
  return (uint8_t *)base;
}

//
// Helper function to mmap queue HP/TP registers. The offset parameter in mmap
// indicate the buffer to be requested. The coding schemem is:
//   bit 24-27: VC #
//   bit 28: 0 for queue buffer, 1 for VC queue register base
//
[[maybe_unused]] static uint8_t *mmapVCRegisters(int fd, uint32_t vc) {
  uint32_t offset = 0x10000000 | (vc << 24);
  return (uint8_t *)mmap(0, VCRegRegionSize, PROT_READ | PROT_WRITE, MAP_SHARED,
                         fd, offset);
}

//
// Helper function to populate an Address Length Pair in the DMA transfer
// message
// with Platform-specific buffer handle, a buffer pointer on Linux
//
static QStatus populateNNCAlp(nnc_transaction_dma_xfer_t *dmaTrans,
                              QOsBuffer *osbuf) {
  if (!dmaTrans)
    return QS_ERROR;

  dmaTrans->addr = reinterpret_cast<uint64_t>(osbuf->data());
  dmaTrans->size = osbuf->size();

  return QS_SUCCESS;
}

QKmdDevice::QKmdDevice(QID qid, std::shared_ptr<QVCFactoryInterface> vcFactory,
                       const QPciInfo pciInfo, shQDevInterface deviceInterface,
                       const QDevInterfaceEnum deviceInterfaceType,
                       std::string &devName)
    : QRuntimePlatformKmdDeviceInterface(qid, devName), QLogger("QKmdDevice"),
      QRuntimePlatformKmdDevice(qid, pciInfo, deviceInterface,
                                deviceInterfaceType, devName),
      QDeviceInterface(qid, vcFactory, 0, 0, devName),
      deviceAccessMtxEnabled_(false), deviceInterface_(deviceInterface) {
  naNspData_.major = 0;
  naNspData_.minor = 0;
  naNspData_.patch = 0;
  QOsal::strlcpy(naNspData_.qc, "undefined", sizeof(naNspData_.qc));
  QOsal::strlcpy(naNspData_.oem, "undefined", sizeof(naNspData_.oem));
  QOsal::strlcpy(naNspData_.variant, "undefined", sizeof(naNspData_.variant));
  // Following code is internal use only so not setting up an init function
  if (std::getenv(QAicSerializeDeviceEnv.c_str())) {
    deviceAccessMtxEnabled_ = true;
    deviceAccessMtx_ = QMonitorNamedMutexBuilder::buildNamedMutex(
        QAicDeviceMutexNamePrefix + qutil::qPciInfoToPCIeStr(pciInfo));
  }
}

// Only ELF is supported. If \p buf contains a "qbin" section, the data in this
// section will be extracted, along with the data in metadata section. The two
// trunks of data will be sent to device in separate DMA transfers. Without the
// qbin section the whole file will be transferred over. The "qbin" feature is
// introduced to overcome RUMI's clumsiness in memory copy, and this feature
// will be removed after SOD.
//
QStatus QKmdDevice::loadImage(const QBuffer &buf, const char *name,
                              QNNImageID &imageID, uint32_t metadataCRC) {

  if (!isValid()) {
    return QS_DEV_ERROR;
  }

  manage_msg_t nncMsg{};
  uint8_t data[QAIC_MANAGE_MAX_MSG_LENGTH] = {};
  auto ptTrans = reinterpret_cast<nnc_transaction_passthrough_t *>(data);
  ptTrans->header.type = NNC_TRANSACTION_TYPE_PASSTHROUGH_UK;
  ptTrans->header.len = sizeof(*ptTrans) + sizeof(nnc_cmd_load_elf_request_t);
  auto dmaTrans = reinterpret_cast<nnc_transaction_dma_xfer_t *>(&data[ptTrans->header.len]);
  dmaTrans->hdr.len = sizeof(*dmaTrans);

  QOsBuffer osbuf(buf);
  if (populateNNCAlp(dmaTrans, &osbuf) != QS_SUCCESS) {
    LogError("Device {} failed to populate NNC ALP", devID_);
    return QS_INVAL;
  }

  constexpr uint32_t LoadImgTransCount = 2;
  nncMsg.count = LoadImgTransCount;
  nncMsg.len += ptTrans->header.len + dmaTrans->hdr.len;
  nncMsg.data = reinterpret_cast<uint64_t>(data);

  auto loadReqCmd = reinterpret_cast<nnc_cmd_load_elf_request_t *>(&ptTrans->cmd);
  loadReqCmd->type = NNC_COMMAND_TYPE_LOAD_ELF_REQ;
  loadReqCmd->tag = NncImageTag;
  loadReqCmd->process_id = getpid();
  loadReqCmd->metadata_crc = metadataCRC;
  loadReqCmd->data_length = buf.size;
  QOsal::strlcpy(loadReqCmd->network_name, name, NNC_MAX_STRING_LENGTH);

  dmaTrans->hdr.type = NNC_TRANSACTION_TYPE_DMA_XFER_UK;
  dmaTrans->count = 1;
  dmaTrans->tag = NncImageTag;

  QStatus status = deviceInterface_->runDevCmd(QAIC_DEV_CMD_MANAGE, &nncMsg);
  if (status != QS_SUCCESS) {
    return status;
  }

  auto loadRspCmd =
      reinterpret_cast<nnc_cmd_load_elf_response_t *>(&ptTrans->cmd);

  if (loadRspCmd->type != NNC_COMMAND_TYPE_LOAD_ELF_RESP) {
    LogError("Device {} invalid load ELF response ({})", devID_,
             (uint32_t)loadRspCmd->type);
    return QS_INVAL;
  }

  if (loadRspCmd->status_code != NNC_STATUS_SUCCESS) {
    LogError("Device {} failed to load image: {}", devID_,
             nncErrorStr(loadRspCmd->status_code));
    return QS_DEV_ERROR;
  }
  imageID = loadRspCmd->elf_id;

  LogInfo("Device {} loaded image with ID {}", devID_, imageID);
  return QS_SUCCESS;
}

//
// The load_constants command is to be consumed by QSM. The dma_xfer transaction
// is to be updated by KMD after DMA map user space virtual address to
// IOVA/physical addresses.
//
QStatus QKmdDevice::loadConstants(const QBuffer &buf, const char *name,
                                  QNNConstantsID &constantsID,
                                  uint32_t constantsCRC) {
  manage_msg_t nncMsg;
  uint8_t data[QAIC_MANAGE_MAX_MSG_LENGTH] = {};
  nnc_transaction_passthrough_t *ptTrans;
  nnc_cmd_load_constants_request_t *loadReqCmd;
  nnc_transaction_dma_xfer_t *dmaTrans;
  QStatus status = QS_SUCCESS;

  if (!isValid()) {
    return QS_DEV_ERROR;
  }

  ptTrans = (nnc_transaction_passthrough_t *)data;
  ptTrans->header.type = NNC_TRANSACTION_TYPE_PASSTHROUGH_UK;
  ptTrans->header.len = sizeof(*ptTrans) + sizeof(*loadReqCmd);
  dmaTrans = (nnc_transaction_dma_xfer_t *)&data[ptTrans->header.len];
  dmaTrans->hdr.len = sizeof(*dmaTrans);

  loadReqCmd = (nnc_cmd_load_constants_request_t *)&ptTrans->cmd;
  loadReqCmd->type = NNC_COMMAND_TYPE_LOAD_CONSTANTS_REQ;
  loadReqCmd->tag = NncConstantsTag;
  loadReqCmd->process_id = getpid();
  loadReqCmd->data_length = buf.size;
  loadReqCmd->constants_crc = constantsCRC;
  QOsal::strlcpy(loadReqCmd->constants_name, name, NNC_MAX_STRING_LENGTH);

  dmaTrans->hdr.type = NNC_TRANSACTION_TYPE_DMA_XFER_UK;
  dmaTrans->hdr.len = sizeof(*dmaTrans);
  dmaTrans->count = 1;
  dmaTrans->tag = NncConstantsTag;
  dmaTrans = (nnc_transaction_dma_xfer_t *)&data[ptTrans->header.len];

  QOsBuffer osbuf(buf);
  if (populateNNCAlp(dmaTrans, &osbuf) != QS_SUCCESS) {
    LogError("Device {} failed to populate NNC ALP", devID_);
    return QS_INVAL;
  }

  nncMsg.count = 2;
  nncMsg.len = ptTrans->header.len + dmaTrans->hdr.len;
  nncMsg.data = reinterpret_cast<uint64_t>(data);

  status = deviceInterface_->runDevCmd(QAIC_DEV_CMD_MANAGE, &nncMsg);
  if (status != QS_SUCCESS) {
    return status;
  }

  if (nncMsg.count != 1) {
    LogError("Device {} expecting one transaction in load constants response, "
             "got {}",
             devID_, nncMsg.count);
    return QS_INVAL;
  }

  nnc_cmd_load_constants_response_t *loadRspCmd =
      (nnc_cmd_load_constants_response_t *)&ptTrans->cmd;

  if (loadRspCmd->type != NNC_COMMAND_TYPE_LOAD_CONSTANTS_RESP) {
    LogError("Device {} invalid load constants response ({})", devID_,
             (uint32_t)loadRspCmd->type);
    return QS_INVAL;
  }

  if (loadRspCmd->constants_id == 0 ||
      loadRspCmd->status_code != NNC_STATUS_SUCCESS) {
    LogError("Device {} failed to load constants: {}", devID_,
             nncErrorStr(loadRspCmd->status_code));
    return QS_DEV_ERROR;
  }

  constantsID = loadRspCmd->constants_id;

  LogInfo("Device {} loaded constants with ID {} ", devID_, constantsID);

  return QS_SUCCESS;
}

//
// loadConstantsEx sets up device memory for constants load. The loading
// part is done by loadConstantsAtOffset().
//
QStatus QKmdDevice::loadConstantsEx(const QBuffer &constDescBuf,
                                    const char *name,
                                    QNNConstantsID &constantsID) {
  manage_msg_t nncMsg;
  uint8_t data[QAIC_MANAGE_MAX_MSG_LENGTH] = {};
  nnc_transaction_passthrough_t *ptTrans;
  nnc_cmd_load_constants_ex_request_t *loadReqCmd;
  QStatus status = QS_SUCCESS;

  if (!isValid()) {
    return QS_DEV_ERROR;
  }

  if (constDescBuf.size >
      sizeof(data) - sizeof(*ptTrans) - sizeof(loadReqCmd)) {
    LogError("Device {} constDescBuf too large {}", devID_, constDescBuf.size);
    return QS_INVAL;
  }

  ptTrans = (nnc_transaction_passthrough_t *)data;
  ptTrans->header.type = NNC_TRANSACTION_TYPE_PASSTHROUGH_UK;

  loadReqCmd = (nnc_cmd_load_constants_ex_request_t *)&ptTrans->cmd;
  loadReqCmd->type = NNC_COMMAND_TYPE_LOAD_CONSTANTS_EX_REQ;
  loadReqCmd->process_id = getpid();

  QOsal::strlcpy(loadReqCmd->constants_name, name, NNC_MAX_STRING_LENGTH);

  loadReqCmd->descriptor_len = constDescBuf.size;
  memcpy((char *)loadReqCmd->descriptor, (char *)constDescBuf.buf,
         constDescBuf.size);

  ptTrans->header.len =
      sizeof(*ptTrans) + sizeof(*loadReqCmd) + constDescBuf.size;

  nncMsg.count = 1;
  nncMsg.len = ptTrans->header.len;
  nncMsg.data = reinterpret_cast<uint64_t>(data);

  status = deviceInterface_->runDevCmd(QAIC_DEV_CMD_MANAGE, &nncMsg);
  if (status != QS_SUCCESS) {
    return status;
  }
  if (nncMsg.count != 1) {
    LogError("Device {} expecting one transaction in load constants response, "
             "got {}",
             devID_, nncMsg.count);
    return QS_INVAL;
  }

  nnc_cmd_load_constants_response_t *loadRspCmd =
      (nnc_cmd_load_constants_response_t *)&ptTrans->cmd;

  if (loadRspCmd->type != NNC_COMMAND_TYPE_LOAD_CONSTANTS_RESP) {
    LogError("Device {} invalid load constants response ({})", devID_,
             (uint32_t)loadRspCmd->type);
    return QS_INVAL;
  }

  if (loadRspCmd->constants_id == 0 ||
      loadRspCmd->status_code != NNC_STATUS_SUCCESS) {
    LogError("Device {} failed to load constants: {}", devID_,
             nncErrorStr(loadRspCmd->status_code));
    return QS_DEV_ERROR;
  }

  constantsID = loadRspCmd->constants_id;

  LogInfo("Device {} ready to load constants with ID {} ", devID_, constantsID);

  return QS_SUCCESS;
}

//
// Load a segement of constnats at specified \p offset.
// The \p constantsID is obtained through loadConstantsEx().
//
QStatus QKmdDevice::loadConstantsAtOffset(QNNConstantsID constantsID,
                                          const QBuffer &buf, uint64_t offset) {
  manage_msg_t nncMsg;
  uint8_t data[QAIC_MANAGE_MAX_MSG_LENGTH] = {};
  nnc_transaction_passthrough_t *ptTrans;
  nnc_cmd_load_constants_payload_request_t *loadReqCmd;
  nnc_transaction_dma_xfer_t *dmaTrans;
  QStatus status = QS_SUCCESS;
  if (!isValid()) {
    return QS_DEV_ERROR;
  }

  ptTrans = (nnc_transaction_passthrough_t *)data;
  ptTrans->header.type = NNC_TRANSACTION_TYPE_PASSTHROUGH_UK;
  ptTrans->header.len = sizeof(*ptTrans) + sizeof(*loadReqCmd);
  dmaTrans = (nnc_transaction_dma_xfer_t *)&data[ptTrans->header.len];
  dmaTrans->hdr.len = sizeof(*dmaTrans);

  loadReqCmd = (nnc_cmd_load_constants_payload_request_t *)&ptTrans->cmd;
  loadReqCmd->type = NNC_COMMAND_TYPE_LOAD_CONSTANTS_PAYLOAD_REQ;
  loadReqCmd->tag = NncConstantsTag;
  loadReqCmd->process_id = getpid();
  loadReqCmd->data_length = buf.size;
  loadReqCmd->constants_id = constantsID;
  loadReqCmd->offset = offset;

  dmaTrans->hdr.type = NNC_TRANSACTION_TYPE_DMA_XFER_UK;
  dmaTrans->hdr.len = sizeof(*dmaTrans);
  dmaTrans->count = 1;
  dmaTrans->tag = NncConstantsTag;
  dmaTrans = (nnc_transaction_dma_xfer_t *)&data[ptTrans->header.len];
  dmaTrans->addr = reinterpret_cast<uint64_t>(buf.buf);
  dmaTrans->size = buf.size;

  nncMsg.count = 2;
  nncMsg.len = ptTrans->header.len + dmaTrans->hdr.len;
  nncMsg.data = reinterpret_cast<uint64_t>(data);

  status = deviceInterface_->runDevCmd(QAIC_DEV_CMD_MANAGE, &nncMsg);
  if (status != QS_SUCCESS) {
    return status;
  }

  if (nncMsg.count != 1) {
    LogError("Device {} expecting one transaction in load constants response, "
             "got {}",
             devID_, nncMsg.count);
    return QS_INVAL;
  }

  nnc_cmd_load_constants_response_t *loadRspCmd =
      (nnc_cmd_load_constants_response_t *)&ptTrans->cmd;

  if (loadRspCmd->type != NNC_COMMAND_TYPE_LOAD_CONSTANTS_RESP) {
    LogError("Device {} expecting load constants response, got type {}", devID_,
             (uint32_t)loadRspCmd->type);
    return QS_INVAL;
  }

  if (loadRspCmd->status_code != NNC_STATUS_SUCCESS) {
    LogError("Device {} failed to load constants segment at offset {}: {}",
             devID_, offset, nncErrorStr(loadRspCmd->status_code));
    return QS_DEV_ERROR;
  }

  LogInfo(
      "Device {} loaded segment of size {} at offset {} for constants ID {} ",
      devID_, buf.size, offset, constantsID);

  return QS_SUCCESS;
}

//
//  The flow of network activation:
//  1. Lib -> KMD
//    Activate Cmd Req: ImageID, ConstantsID
//    Activate Transaction: Q size, eventfd
//  2. KMD -> Lib
//    Activate Cmd Rsp: NAID, VC #, DDR Base, MCID Base
//  3. Lib mmap request/response queues created by KMD for VC
//
QStatus QKmdDevice::activate(QNNImageID imageID, QNNConstantsID constantsID,
                             QNAID &naID, uint64_t &ddrBase, uint64_t &mcIDBase,
                             QVirtualChannelInterface **vc,
                             QActivationStateType initialState) {
  QStatus status = QS_SUCCESS;
  uint32_t qSizePow2 = DefaultVCQueueSizePow2;

  int efd = QOsal::eventfd(0, 0);
  if (efd == -1) {
    LogError("Device {} failed to create eventfd: {}", devID_,
             QOsal::strerror_safe(errno));
    return QS_INVAL;
  }

  //----------------------------------------------------------------
  // If vcResId is provided, we do not need to create a VC
  // Use existing VC
  //----------------------------------------------------------------
  manage_msg_t nncMsg;
  uint8_t data[QAIC_MANAGE_MAX_MSG_LENGTH] = {};
  nnc_transaction_passthrough_t *ptTrans;
  nnc_cmd_activate_request_t *actvReqCmd;
  nnc_cmd_activate_response_t *actvRspCmd = nullptr;
  nnc_transaction_vc_setup_uk_t *actvTrans;

  if (!isValid()) {
    return QS_DEV_ERROR;
  }

  ptTrans = (nnc_transaction_passthrough_t *)data;
  ptTrans->header.type = NNC_TRANSACTION_TYPE_PASSTHROUGH_UK;
  ptTrans->header.len = sizeof(*ptTrans) + sizeof(*actvReqCmd);

  actvReqCmd = (nnc_cmd_activate_request_t *)&ptTrans->cmd;
  actvReqCmd->type = NNC_COMMAND_TYPE_ACTIVATE_REQ;
  actvReqCmd->process_id = getpid();
  actvReqCmd->constants_id = constantsID;
  actvReqCmd->elf_id = imageID;
  actvReqCmd->reservation_type = 0;
  actvReqCmd->resource_res_id = -1;
  actvReqCmd->vc_res_id = -1;
  actvReqCmd->dma_type = 0;
  actvReqCmd->opt_support_datapath_switching = 0;

  switch (initialState) {
  case ACTIVATION_STATE_CMD_STANDBY:
  case ACTIVATION_STATE_CMD_INIT:
    actvReqCmd->initial_activation_state = NNC_ACTIVATION_STATE_CMD_STANDBY;
    break;

  case ACTIVATION_STATE_CMD_DEACTIVATE:
    actvReqCmd->initial_activation_state = NNC_ACTIVATION_STATE_CMD_DEACTIVATE;
    break;
  case ACTIVATION_STATE_CMD_READY:
    actvReqCmd->initial_activation_state = NNC_ACTIVATION_STATE_CMD_READY;
    break;
  default:
    LogError("Device {} invalid activation initial state", devID_);
    return QS_INVAL;
    break;
  }

  actvTrans =
      (nnc_transaction_vc_setup_uk_t *)&data[ptTrans->header.len];
  actvTrans->hdr.len = sizeof(*actvTrans);
  actvReqCmd->opt_vc_setup = 1;
  actvTrans->hdr.type = NNC_TRANSACTION_TYPE_SETUP_VC_UK;
  actvTrans->queue_size = 1 << qSizePow2;
  actvTrans->eventfd = efd;
  actvTrans->options = 0;
  actvTrans->pad = 0;
  nncMsg.count = 2;
  nncMsg.len = ptTrans->header.len + actvTrans->hdr.len;
  nncMsg.data = reinterpret_cast<uint64_t>(data);

  status = deviceInterface_->runDevCmd(QAIC_DEV_CMD_MANAGE, &nncMsg);
  if (status != QS_SUCCESS) {
    return status;
  }

  actvRspCmd = (nnc_cmd_activate_response_t *)&ptTrans->cmd;

  // Validate the output
  if (actvRspCmd->type != NNC_COMMAND_TYPE_ACTIVATE_RESP) {
    LogError("Device {} received invalid activate response ({}) ", devID_,
             (uint32_t)actvRspCmd->type);
    return QS_INVAL;
  }

  if (actvRspCmd->status_code != NNC_STATUS_SUCCESS) {
    LogError("Device {} activate failed: {}", devID_,
             nncErrorStr(actvRspCmd->status_code));
    /* If device is under Sub system restart, application can retry */
    if (actvRspCmd->status_code == NNC_STATUS_SS_UNDER_SSR_ERROR) {
      return QS_AGAIN;
    } else {
      return QS_DEV_ERROR;
    }
  }

  if (nncMsg.count != 2) {
    LogError("Device {} expecting 2 transactions in activate response, got {}",
             devID_, (uint32_t)nncMsg.count);
    return QS_INVAL;
  }

  mcIDBase = actvRspCmd->mcid_base;
  ddrBase = actvRspCmd->ddr_base;
  naID = actvRspCmd->activation_id;
  uint32_t vcid = actvRspCmd->virtual_channel_id;

  if (vcid > MaxVCID) {
    LogError("Device {} received invalid VCID {}", devID_, vcid);
    return QS_INVAL;
  }

  uint32_t dmaChannelType = actvRspCmd->dma_type;
  uint32_t pcMask = actvRspCmd->pc_mask;
  LogInfo("Device {} activated NAID {} for VC {} with MCID base {:x} DDR base "
          "{:x} PC Reservation Type {} PC Mask {} ",
          devID_, naID, vcid, mcIDBase, ddrBase, dmaChannelType, pcMask);
  status = createVC(vcid, efd, vc, qSizePow2);
  if (status != QS_SUCCESS) {
    return status;
  }
  if (vc == nullptr) {
    return QS_INVAL;
  }
  (*vc)->setPcMask(actvRspCmd->pc_mask);
  (*vc)->incRef();
  return QS_SUCCESS;
}

QStatus QKmdDevice::sendActivationStateChangeCommand(
    std::vector<std::pair<QNAID, QActivationStateType>> &stateCmdSet) {
  manage_msg_t nncMsg;
  uint8_t data[QAIC_MANAGE_MAX_MSG_LENGTH] = {};
  nnc_transaction_passthrough_t *ptTrans;
  nnc_activation_state_cmd_request_t *reqCmd;
  nnc_activation_state_cmd_response_t *rspCmd = nullptr;
  QStatus status = QS_SUCCESS;

  if (!isValid()) {
    return QS_DEV_ERROR;
  }

  if (stateCmdSet.size() > NNC_ACTIVATION_CMD_TYPE_MAX_COMMANDS) {
    LogError("Device {}, cannot send Activation Command, number of commands:{} "
             "exceeds message capacity:{}",
             devID_, stateCmdSet.size(), NNC_ACTIVATION_CMD_TYPE_MAX_COMMANDS);
    return QS_INVAL;
  }

  ptTrans = (nnc_transaction_passthrough_t *)data;
  ptTrans->header.type = NNC_TRANSACTION_TYPE_PASSTHROUGH_UK;
  ptTrans->header.len =
      static_cast<uint32_t>(sizeof(*ptTrans) + sizeof(*reqCmd));

  reqCmd = (nnc_activation_state_cmd_request_t *)&ptTrans->cmd;
  reqCmd->type = NNC_COMMAND_TYPE_ACTIVATE_STATE_CMD_REQ;
  reqCmd->num_cmds = stateCmdSet.size();
  LogDebug("Device {} reqCmd->num_cmds {}", devID_, (uint32_t)reqCmd->num_cmds);
  for (uint32_t i = 0; i < stateCmdSet.size(); i++) {
    reqCmd->ac_cmd[i].acid = stateCmdSet.at(i).first;

    switch (stateCmdSet.at(i).second) {
    case ACTIVATION_STATE_CMD_STANDBY:
      reqCmd->ac_cmd[i].cmd = NNC_ACTIVATION_STATE_CMD_STANDBY;
      break;
    case ACTIVATION_STATE_CMD_READY:
      reqCmd->ac_cmd[i].cmd = NNC_ACTIVATION_STATE_CMD_READY;
      break;
    case ACTIVATION_STATE_CMD_DEACTIVATE:
      reqCmd->ac_cmd[i].cmd = NNC_ACTIVATION_STATE_CMD_DEACTIVATE;
      break;
    default:
      reqCmd->ac_cmd[i].cmd = NNC_ACTIVATION_STATE_INVAL;
      break;
    }
    LogDebug("reqCmd->ac_cmd[{}] acid {} cmd {}", i,
             (activation_id_type_t)reqCmd->ac_cmd[i].acid,
             (nnc_activation_state_type_t)reqCmd->ac_cmd[i].cmd);
  }
  nncMsg.count = 1;
  nncMsg.len = ptTrans->header.len;
  nncMsg.data = reinterpret_cast<uint64_t>(data);

  status = deviceInterface_->runDevCmd(QAIC_DEV_CMD_MANAGE, &nncMsg);
  if (status != QS_SUCCESS) {
    return status;
  }

  rspCmd = (nnc_activation_state_cmd_response_t *)&ptTrans->cmd;

  if (rspCmd->type != NNC_COMMAND_TYPE_ACTIVATE_STATE_CMD_RESP) {
    LogError("Device {} invalid activation state command response ({})", devID_,
             (uint32_t)(rspCmd->type));
    return QS_INVAL;
  }

  if (rspCmd->status_code != NNC_STATUS_SUCCESS) {
    LogError("Device {} activation state command response error: {}", devID_,
             nncErrorStr(rspCmd->status_code));
    return QS_DEV_ERROR;
  }

  LogInfo("Device {} activation state command complete", devID_);
  return QS_SUCCESS;
}

QStatus QKmdDevice::createVC(QVCID vcid, int efd, QVirtualChannelInterface **vc,
                             uint32_t qSizePow2) {
  if (efd >= 0)
    close(efd);
  VCDeleter deleter = [=](QVirtualChannelInterface *vc) { delete vc; };
  QStatus status = QS_SUCCESS;

  std::unique_lock<std::mutex> lock(vcMapMutex_);
  if (vcMap_.find(vcid) == vcMap_.end()) {
    std::unique_ptr<QVirtualChannelInterface, VCDeleter> vcBackend =
        vcFactory_->makeVC(devID_, vcid, nullptr, nullptr, qSizePow2, deleter,
                           nullptr);
    *vc = vcBackend.get();
    vcMap_[vcid] = std::move(vcBackend);
  } else if (vcid == NNC_VC_RESERVATION_ID_SKIP) {
    *vc = vcMap_[vcid].get();
  } else {
    LogError("Duplicate vcid");
    status = QS_ERROR;
  }

  return status;
}

//
// User may deactivate anytime during the run. LRT lib and KMD needs to arrange
// for graceful shutdown if NSP or network image does not handle sudden death.
//
QStatus QKmdDevice::deactivate(uint8_t vc, QNAID naID, bool vc_deactivate) {
  manage_msg_t nncMsg;
  uint8_t data[QAIC_MANAGE_MAX_MSG_LENGTH] = {};
  nnc_transaction_passthrough_t *ptTrans = nullptr;
  nnc_transaction_vc_teardown_uk_t *deactvTrans = nullptr;
  nnc_cmd_deactivate_request_t *deactvReqCmd = nullptr;
  std::unique_ptr<QVirtualChannelInterface, VCDeleter> vcToDeactivate = nullptr;
  QStatus status = QS_SUCCESS;

  auto restoreVcMapEntry = [&]() {
    std::unique_lock<std::mutex> lock(vcMapMutex_);
    vcMap_[vc] = std::move(vcToDeactivate);
    return;
  };

  if (!isValid()) {
    return QS_DEV_ERROR;
  }

  ptTrans = (nnc_transaction_passthrough_t *)data;
  ptTrans->header.type = NNC_TRANSACTION_TYPE_PASSTHROUGH_UK;
  ptTrans->header.len = sizeof(*ptTrans) + sizeof(*deactvReqCmd);

  deactvReqCmd = (nnc_cmd_deactivate_request_t *)&ptTrans->cmd;
  deactvReqCmd->type = NNC_COMMAND_TYPE_DEACTIVATE_REQ;
  deactvReqCmd->activation_id = naID;
  (vc_deactivate == true) ? deactvReqCmd->opt_vc_teardown = 1
                          : deactvReqCmd->opt_vc_teardown = 0;

  deactvTrans =
      (nnc_transaction_vc_teardown_uk_t *)&data[ptTrans->header.len];
  deactvTrans->hdr.len = sizeof(*deactvTrans);
  if (vc_deactivate) {
    deactvTrans->hdr.type = NNC_TRANSACTION_TYPE_TEARDOWN_VC_UK;
    deactvTrans->dbc_id = vc;
    nncMsg.count = 2;
    nncMsg.len = ptTrans->header.len + deactvTrans->hdr.len;
  } else {
    nncMsg.count = 1;
    nncMsg.len = ptTrans->header.len;
  }
  nncMsg.data = reinterpret_cast<uint64_t>(data);

  if (vc_deactivate) {
    std::unique_lock<std::mutex> lock(vcMapMutex_);
    auto it = vcMap_.find(vc);
    if (it == vcMap_.end()) {
      LogError("Device {} vcid {} not available", devID_, vc);
      return QS_ERROR;
    }
    vcToDeactivate = std::move(it->second);
    vcMap_.erase(it);
  }
  status = deviceInterface_->runDevCmd(QAIC_DEV_CMD_MANAGE, &nncMsg);

  nnc_cmd_deactivate_response_t *deactvRspCmd =
      (nnc_cmd_deactivate_response_t *)&ptTrans->cmd;

  if (status != QS_SUCCESS) {
    if (vc_deactivate) {
      restoreVcMapEntry();
    }
    /* If device is under Sub system restart, application can retry */
    if ((deactvRspCmd->type == NNC_COMMAND_TYPE_DEACTIVATE_RESP) &&
        (deactvRspCmd->status_code == NNC_STATUS_SS_UNDER_SSR_ERROR)) {
      LogError("Device {} deactivate failed: {}", devID_,
               nncErrorStr(deactvRspCmd->status_code));
      return QS_AGAIN;
    }
    return status;
  }

  if (deactvRspCmd->type != NNC_COMMAND_TYPE_DEACTIVATE_RESP) {
    LogError("Device {} invalid deactivate response ({})", devID_,
             (uint32_t)deactvRspCmd->type);
    if (vc_deactivate) {
      restoreVcMapEntry();
    }
    return QS_INVAL;
  }

  if (deactvRspCmd->status_code != NNC_STATUS_SUCCESS) {
    LogError("Device {} deactivate failed: {}", devID_,
             nncErrorStr(deactvRspCmd->status_code));
    if (vc_deactivate) {
      restoreVcMapEntry();
    }
    return QS_DEV_ERROR;
  }

  LogInfo("Device {} deactivated {} on VC {}", devID_, naID, (uint32_t)vc);

  return QS_SUCCESS;
}

//
// Unload Image
//
QStatus QKmdDevice::unloadImage(QNNImageID imageID) {
  manage_msg_t nncMsg;
  uint8_t data[QAIC_MANAGE_MAX_MSG_LENGTH] = {};
  nnc_transaction_passthrough_t *ptTrans;
  nnc_cmd_unload_elf_request_t *statReqCmd;
  QStatus status = QS_SUCCESS;

  if (!isValid()) {
    return QS_DEV_ERROR;
  }

  ptTrans = (nnc_transaction_passthrough_t *)data;
  ptTrans->header.type = NNC_TRANSACTION_TYPE_PASSTHROUGH_UK;
  ptTrans->header.len = sizeof(*ptTrans) + sizeof(*statReqCmd);

  statReqCmd = (nnc_cmd_unload_elf_request_t *)&ptTrans->cmd;
  statReqCmd->type = NNC_COMMAND_TYPE_UNLOAD_ELF_REQ;
  statReqCmd->elf_id = imageID;

  nncMsg.count = 1;
  nncMsg.len = ptTrans->header.len;
  nncMsg.data = reinterpret_cast<uint64_t>(data);

  status = deviceInterface_->runDevCmd(QAIC_DEV_CMD_MANAGE, &nncMsg);
  if (status != QS_SUCCESS) {
    return status;
  }

  nnc_cmd_unload_elf_response_t *unloadRspCmd =
      (nnc_cmd_unload_elf_response_t *)&ptTrans->cmd;

  if (unloadRspCmd->type != NNC_COMMAND_TYPE_UNLOAD_ELF_RESP) {
    LogError("Device {} invalid unload image response ({})", devID_,
             (uint32_t)unloadRspCmd->type);
    return QS_INVAL;
  }

  if (unloadRspCmd->status_code != NNC_STATUS_SUCCESS) {
    LogError("Device {} unload image failed: {}", devID_,
             nncErrorStr(unloadRspCmd->status_code));
    return QS_DEV_ERROR;
  }

  LogInfo("Device {} unloaded image with ID {}", devID_, imageID);

  return QS_SUCCESS;
}

//
// Unload constants
//
QStatus QKmdDevice::unloadConstants(QNNConstantsID constID) {
  manage_msg_t nncMsg;
  uint8_t data[QAIC_MANAGE_MAX_MSG_LENGTH] = {};
  nnc_transaction_passthrough_t *ptTrans;
  nnc_cmd_unload_constants_request_t *statReqCmd;
  QStatus status = QS_SUCCESS;

  if (!isValid()) {
    return QS_DEV_ERROR;
  }

  ptTrans = (nnc_transaction_passthrough_t *)data;
  ptTrans->header.type = NNC_TRANSACTION_TYPE_PASSTHROUGH_UK;
  ptTrans->header.len = sizeof(*ptTrans) + sizeof(*statReqCmd);

  statReqCmd = (nnc_cmd_unload_constants_request_t *)&ptTrans->cmd;
  statReqCmd->type = NNC_COMMAND_TYPE_UNLOAD_CONSTANTS_REQ;
  statReqCmd->constants_id = constID;

  nncMsg.count = 1;
  nncMsg.len = ptTrans->header.len;
  nncMsg.data = reinterpret_cast<uint64_t>(data);

  status = deviceInterface_->runDevCmd(QAIC_DEV_CMD_MANAGE, &nncMsg);
  if (status != QS_SUCCESS) {
    return status;
  }

  nnc_cmd_unload_constants_response_t *unloadRspCmd =
      (nnc_cmd_unload_constants_response_t *)&ptTrans->cmd;

  if (unloadRspCmd->type != NNC_COMMAND_TYPE_UNLOAD_CONSTANTS_RESP) {
    LogError("Device {} invalid unload constants response ({})", devID_,
             (uint32_t)unloadRspCmd->type);
    return QS_INVAL;
  }

  if (unloadRspCmd->status_code != NNC_STATUS_SUCCESS) {
    LogError("Device {} unload constants failed: {}", devID_,
             nncErrorStr(unloadRspCmd->status_code));
    return QS_DEV_ERROR;
  }

  LogInfo("Device {} unloaded constants with ID {}", devID_, constID);

  return QS_SUCCESS;
}

QStatus QKmdDevice::getResourceInfo(QResourceInfo &resourceInfo) {

  manage_msg_t nncMsg;
  uint8_t data[QAIC_MANAGE_MAX_MSG_LENGTH] = {};
  nnc_transaction_passthrough_t *ptTrans;
  nnc_cmd_resource_info_request_t *infoReqCmd;
  nnc_cmd_resource_info_response_t *infoRspCmd;
  QStatus status = QS_SUCCESS;

  if (!isValid()) {
    return QS_DEV_ERROR;
  }

  ptTrans = (nnc_transaction_passthrough_t *)data;
  ptTrans->header.type = NNC_TRANSACTION_TYPE_PASSTHROUGH_UK;
  ptTrans->header.len =
      static_cast<uint32_t>(sizeof(*ptTrans) + sizeof(*infoReqCmd));

  infoReqCmd = (nnc_cmd_resource_info_request_t *)&ptTrans->cmd;
  infoReqCmd->type = NNC_COMMAND_TYPE_RESOURCE_INFO_REQ;
  infoReqCmd->res_grp_id = -1;

  nncMsg.count = 1;
  nncMsg.len = ptTrans->header.len;
  nncMsg.data = reinterpret_cast<uint64_t>(data);

  status = deviceInterface_->runDevCmd(QAIC_DEV_CMD_MANAGE, &nncMsg);
  if (status != QS_SUCCESS) {
    return status;
  }

  infoRspCmd = (nnc_cmd_resource_info_response_t *)&ptTrans->cmd;

  if (infoRspCmd->type != NNC_COMMAND_TYPE_RESOURCE_INFO_RESP) {
    LogError("Device {} invalid release resource group response ({})", devID_,
             (uint32_t)(infoRspCmd->type));
    return QS_INVAL;
  }

  if (infoRspCmd->status_code != NNC_STATUS_SUCCESS) {
    LogError("Device {} resource group release response : {}", devID_,
             nncErrorStr(infoRspCmd->status_code));
    return QS_DEV_ERROR;
  }

  resourceInfo.dramTotal = infoRspCmd->resource_info.dramTotal;
  resourceInfo.dramFree = infoRspCmd->resource_info.dramFree;
  resourceInfo.nspTotal = infoRspCmd->resource_info.nspTotal;
  resourceInfo.nspFree = infoRspCmd->resource_info.nspFree;
  resourceInfo.semTotal = infoRspCmd->resource_info.semTotal;
  resourceInfo.semFree = infoRspCmd->resource_info.semFree;
  resourceInfo.mcidTotal = infoRspCmd->resource_info.mcidTotal;
  resourceInfo.mcidFree = infoRspCmd->resource_info.mcidFree;
  resourceInfo.vcTotal = infoRspCmd->resource_info.vcTotal;
  resourceInfo.vcFree = infoRspCmd->resource_info.vcFree;
  resourceInfo.pcTotal = infoRspCmd->resource_info.pcTotal;
  resourceInfo.pcReserved = infoRspCmd->resource_info.pcReserved;
  resourceInfo.QResourceReservationID = infoRspCmd->res_grp_id;
  return QS_SUCCESS;
}

QStatus QKmdDevice::getPerformanceInfo(QPerformanceInfo &performanceInfo) {

  manage_msg_t nncMsg;
  uint8_t data[QAIC_MANAGE_MAX_MSG_LENGTH] = {};
  nnc_transaction_passthrough_t *ptTrans;
  nnc_cmd_performance_info_request_t *infoReqCmd;
  nnc_cmd_performance_info_response_t *infoRspCmd;
  QStatus status = QS_SUCCESS;

  if (!isValid()) {
    return QS_DEV_ERROR;
  }

  ptTrans = (nnc_transaction_passthrough_t *)data;
  ptTrans->header.type = NNC_TRANSACTION_TYPE_PASSTHROUGH_UK;
  ptTrans->header.len = sizeof(*ptTrans) + sizeof(*infoReqCmd);

  infoReqCmd = (nnc_cmd_performance_info_request_t *)&ptTrans->cmd;
  infoReqCmd->type = NNC_COMMAND_TYPE_PERFORMANCE_INFO_REQ;

  nncMsg.count = 1;
  nncMsg.len = ptTrans->header.len;
  nncMsg.data = reinterpret_cast<uint64_t>(data);

  status = deviceInterface_->runDevCmd(QAIC_DEV_CMD_MANAGE, &nncMsg);
  if (status != QS_SUCCESS) {
    return status;
  }

  infoRspCmd = (nnc_cmd_performance_info_response_t *)&ptTrans->cmd;

  if (infoRspCmd->type != NNC_COMMAND_TYPE_PERFORMANCE_INFO_RESP) {
    LogError("Device {} invalid performance info response ({})", devID_,
             (uint32_t)(infoRspCmd->type));
    return QS_INVAL;
  }

  if (infoRspCmd->status_code != NNC_STATUS_SUCCESS) {
    LogError("Device {} performance info response : {}", devID_,
             nncErrorStr(infoRspCmd->status_code));
    return QS_DEV_ERROR;
  }

  performanceInfo.compFrequencyHz =
      infoRspCmd->performance_info.compFrequencyHz;
  performanceInfo.ddrFrequencyHz = infoRspCmd->performance_info.ddrFrequencyHz;
  performanceInfo.memFrequencyHz = infoRspCmd->performance_info.memFrequencyHz;
  performanceInfo.nspFrequencyHz = infoRspCmd->performance_info.nspFrequencyHz;
  performanceInfo.sysFrequencyHz = infoRspCmd->performance_info.sysFrequencyHz;
  performanceInfo.peakCompute = infoRspCmd->performance_info.peakCompute;
  performanceInfo.dramBw = infoRspCmd->performance_info.dramBw;
  return QS_SUCCESS;
}

shQDevInterface &QKmdDevice::getDeviceInterface() { return deviceInterface_; }

QStatus QKmdDevice::createResourceReservation(
    uint16_t numNetworks, uint32_t numNsp, uint32_t numMcid, uint32_t numSem,
    uint32_t numVc, uint64_t shDdrSize, uint64_t ddrSize,
    uint64_t l2region_size, QResourceReservationID &resResId,
    QResourceReservationID sourceGroupId = RESOURCE_GROUP_ID_DEFAULT) {

  manage_msg_t nncMsg;
  uint8_t data[QAIC_MANAGE_MAX_MSG_LENGTH] = {};
  nnc_transaction_passthrough_t *ptTrans;
  nnc_cmd_create_resource_group_request_t *resReqCmd;
  nnc_cmd_create_resource_group_response_t *resRspCmd = nullptr;
  QStatus status = QS_SUCCESS;

  if (!isValid()) {
    return QS_DEV_ERROR;
  }

  ptTrans = (nnc_transaction_passthrough_t *)data;
  ptTrans->header.type = NNC_TRANSACTION_TYPE_PASSTHROUGH_UK;
  ptTrans->header.len =
      static_cast<uint32_t>(sizeof(*ptTrans) + sizeof(*resReqCmd));

  resReqCmd = (nnc_cmd_create_resource_group_request_t *)&ptTrans->cmd;
  resReqCmd->type = NNC_COMMAND_TYPE_CREATE_RESOURCE_GROUP_REQ;
  resReqCmd->source_grp_id = sourceGroupId;
  resReqCmd->num_networks = numNetworks;
  resReqCmd->num_nsp = numNsp;
  resReqCmd->num_mcid = numMcid;
  resReqCmd->num_sem = numSem;
  resReqCmd->num_vc = numVc;
  resReqCmd->shddr_size = shDdrSize;
  resReqCmd->l2region_size = l2region_size;
  resReqCmd->mem_size = ddrSize;
  resReqCmd->process_id = getpid();

  nncMsg.count = 1;
  nncMsg.len = ptTrans->header.len;
  nncMsg.data = reinterpret_cast<uint64_t>(data);

  status = deviceInterface_->runDevCmd(QAIC_DEV_CMD_MANAGE, &nncMsg);
  if (status != QS_SUCCESS) {
    return status;
  }

  resRspCmd = (nnc_cmd_create_resource_group_response_t *)&ptTrans->cmd;

  if (resRspCmd->type != NNC_COMMAND_TYPE_CREATE_RESOURCE_GROUP_RESP) {
    LogError("Device {} invalid resource group reservation response ({})",
             devID_, (uint32_t)(resRspCmd->type));
    return QS_INVAL;
  }

  if (resRspCmd->status_code != NNC_STATUS_SUCCESS) {
    LogError("Device {} create resource group response : {}", devID_,
             nncErrorStr(resRspCmd->status_code));
    return QS_DEV_ERROR;
  }
  resResId = resRspCmd->res_grp_id;
  LogInfo("Device {} activation Reservation successed resResId {}", devID_,
          resResId);

  return QS_SUCCESS;
}

QStatus
QKmdDevice::releaseResourceReservation(QResourceReservationID resResId) {

  manage_msg_t nncMsg;
  uint8_t data[QAIC_MANAGE_MAX_MSG_LENGTH] = {};
  nnc_transaction_passthrough_t *ptTrans;
  nnc_cmd_release_resource_group_request_t *resReqCmd;
  nnc_cmd_release_resource_group_response_t *resRspCmd = nullptr;
  QStatus status = QS_SUCCESS;

  if (!isValid()) {
    return QS_DEV_ERROR;
  }

  ptTrans = (nnc_transaction_passthrough_t *)data;
  ptTrans->header.type = NNC_TRANSACTION_TYPE_PASSTHROUGH_UK;
  ptTrans->header.len =
      static_cast<uint32_t>(sizeof(*ptTrans) + sizeof(*resReqCmd));

  resReqCmd = (nnc_cmd_release_resource_group_request_t *)&ptTrans->cmd;
  resReqCmd->type = NNC_COMMAND_TYPE_RELEASE_RESOURCE_GROUP_REQ;
  resReqCmd->res_grp_id = resResId;

  nncMsg.count = 1;
  nncMsg.len = ptTrans->header.len;
  nncMsg.data = reinterpret_cast<uint64_t>(data);

  status = deviceInterface_->runDevCmd(QAIC_DEV_CMD_MANAGE, &nncMsg);
  if (status != QS_SUCCESS) {
    return status;
  }

  resRspCmd = (nnc_cmd_release_resource_group_response_t *)&ptTrans->cmd;

  if (resRspCmd->type != NNC_COMMAND_TYPE_RELEASE_RESOURCE_GROUP_RESP) {
    LogError("Device {} invalid release resource group response ({})", devID_,
             (uint32_t)(resRspCmd->type));
    return QS_INVAL;
  }

  if (resRspCmd->status_code != NNC_STATUS_SUCCESS) {
    LogError("Device {} resource group release response : {}", devID_,
             nncErrorStr(resRspCmd->status_code));
    return QS_DEV_ERROR;
  }

  LogInfo("Device {} activation Reservation Release successed resResId {}",
          devID_, resResId);

  return QS_SUCCESS;
}

QStatus
QKmdDevice::createResourceReservedDevice(QResourceReservationID resResId) {
  QStatus status = QS_SUCCESS;

  if (!isValid()) {
    return QS_DEV_ERROR;
  }

  qaic_part_dev partDevCreate = {};
  partDevCreate.partition_id = resResId;
  partDevCreate.remove = 0;

  status = deviceInterface_->runDevCmd(QAIC_DEV_CMD_PART_DEV, &partDevCreate);
  if (status != QS_SUCCESS) {
    LogError("Failed to create a partial device resid:{}", resResId);
    return status;
  }

  return QS_SUCCESS;
}

QStatus
QKmdDevice::releaseResourceReservedDevice(QResourceReservationID resResId) {
  QStatus status = QS_SUCCESS;
  if (!isValid()) {
    return QS_DEV_ERROR;
  }
  qaic_part_dev partDevCreate = {resResId ,1 ,0};

  status = deviceInterface_->runDevCmd(QAIC_DEV_CMD_PART_DEV, &partDevCreate);
  if (status != QS_SUCCESS) {
    LogError("Failed to release a partial device resid:{}", resResId);
    return status;
  }

  return QS_SUCCESS;
}

} // namespace qaic
