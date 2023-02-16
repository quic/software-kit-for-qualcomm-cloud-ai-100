// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QRuntimePlatformKmdDevice.h"
#include "QOsal.h"
#include "QUtil.h"
#include "QNncProtocol.h"

#include <fcntl.h>
#include <sstream>
#include <fstream>
#include <sys/ioctl.h>
#include <unistd.h>
#include <memory>
#include <cstdlib>

namespace qaic {

QRuntimePlatformKmdDevice::QRuntimePlatformKmdDevice(
    QID qid, const QPciInfo pciInfo,
    shQRuntimePlatformDeviceInterface deviceInterface,
    const QDevInterfaceEnum deviceInterfaceType, std::string &devName)
    : QRuntimePlatformKmdDeviceInterface(qid, devName),
      QLogger("QRuntimePlatformKmdDevice"), pciInfo_(pciInfo),
      pciInfoString_(qutil::qPciInfoToPCIeStr(pciInfo)),
      qRuntimePlatformDeviceInterface_(deviceInterface),
      deviceInterfaceType_(deviceInterfaceType) {}

QStatus QRuntimePlatformKmdDevice::statusQuery(
    manage_msg_t &nncMsg,
    nnc_cmd_status_query_response_t *&statusQueryResponse) {
  nnc_cmd_status_query_request_t *statusQueryRequest;
  nnc_transaction_passthrough_t *ptTrans;
  uint32_t msgLen;
  QStatus status = QS_SUCCESS;

  if (!qRuntimePlatformDeviceInterface_) {
    return QS_DEV_ERROR;
  }

  msgLen = sizeof(*ptTrans) + sizeof(*statusQueryRequest);

  nncMsg.count = 1;
  nncMsg.len = msgLen;

  ptTrans = reinterpret_cast<nnc_transaction_passthrough_t *>(nncMsg.data);
  ptTrans->header.type = NNC_TRANSACTION_TYPE_PASSTHROUGH_UK;
  ptTrans->header.len = sizeof(*ptTrans) + sizeof(*statusQueryRequest);

  statusQueryRequest = reinterpret_cast<nnc_cmd_status_query_request_t *>(&ptTrans->cmd);
  statusQueryRequest->type = NNC_COMMAND_TYPE_STATUS_REQ;

  std::atomic<std::uint16_t> tryIoctl(2);

  while (tryIoctl--) {
    status = qRuntimePlatformDeviceInterface_->runDevCmd(QAIC_DEV_CMD_MANAGE,
                                                         &nncMsg);
    if (status != QS_SUCCESS) {
      if (errno == ENODEV) {
        // Try to reopen the Device again and try the command again
        if (QS_SUCCESS == qRuntimePlatformDeviceInterface_->reloadDevHandle()) {
          continue;
        }
      }
      if (errno == EINTR) {
        return QS_AGAIN;
      }
      return QS_INVAL;
    } else {
      // If the call worked break;
      break;
    }
  }

  statusQueryResponse = (nnc_cmd_status_query_response_t *)&ptTrans->cmd;

  if (statusQueryResponse->type != NNC_COMMAND_TYPE_STATUS_RESP) {
    LogError("Device {} invalid status response ({}) ", devID_,
             (uint32_t)statusQueryResponse->type);
    return QS_INVAL;
  }

  if (statusQueryResponse->status_code != NNC_STATUS_SUCCESS) {
    LogError("Device {} status response failed: {} ", devID_,
             nncErrorStr(statusQueryResponse->status_code));
    return QS_DEV_ERROR;
  }

  firmwarePropertiesQuery();
  return QS_SUCCESS;
}

QStatus QRuntimePlatformKmdDevice::firmwarePropertiesQuery() {
  manage_msg_t nncMsg;
  uint8_t data[QAIC_MANAGE_MAX_MSG_LENGTH] = {};
  nnc_transaction_status_request_t *statusRequestTrans = nullptr;

  uint32_t msgLen;
  QStatus status = QS_SUCCESS;

  if (!qRuntimePlatformDeviceInterface_) {
    return QS_DEV_ERROR;
  }

  msgLen = sizeof(*statusRequestTrans);

  statusRequestTrans = reinterpret_cast<nnc_transaction_status_request_t *>(data);
  statusRequestTrans->hdr.type = NNC_TRANSACTION_TYPE_STATUS_UK;
  statusRequestTrans->hdr.len = sizeof(*statusRequestTrans);
  nncMsg.count = 1;
  nncMsg.len = msgLen;
  nncMsg.data = reinterpret_cast<uint64_t>(data);

  status =
      qRuntimePlatformDeviceInterface_->runDevCmd(QAIC_DEV_CMD_MANAGE, &nncMsg);
  if (status != QS_SUCCESS) {
    LogError("IOCTL Manage error, for transaction type:{},  errno:{}, {}",
             reinterpret_cast<uint32_t>(statusRequestTrans->hdr.type), errno,
             QOsal::strerror_safe(errno));
    return status;
  }
  nnc_transaction_status_response_t *statusResponseTrans =
      reinterpret_cast<nnc_transaction_status_response_t *>(&nncMsg.data);

  LogDebug("Firmware Transaction Protocol Version {}.{} ",
           reinterpret_cast<uint16_t>(statusResponseTrans->major),
           reinterpret_cast<uint16_t>(statusResponseTrans->minor));
  LogDebug("Firmware Features {:0x}",
           reinterpret_cast<uint64_t>(statusResponseTrans->status_flags));
  deviceFeatures_ = statusResponseTrans->status_flags;
  return QS_SUCCESS;
}

QStatus QRuntimePlatformKmdDevice::getStatus(QDevInfo &info) {

  manage_msg_t nncMsg;
  uint8_t data[QAIC_MANAGE_MAX_MSG_LENGTH] = {};
  nnc_cmd_status_query_response_t *statusQueryCmdRsp;

  nncMsg.data = (uint64_t)data;

  info.devStatus = QDevStatus::QDS_ERROR;
  info.pciInfo = pciInfo_;
  info.qid = devID_;

  QStatus status = QS_ERROR;

  status = statusQuery(nncMsg, statusQueryCmdRsp);
  if (status != QS_SUCCESS) {
    goto exit;
  }

  /// NULL terminate any strings.  Device should do this, but we
  /// cannot make that assumption. This is incorrect. We are maintaining
  /// this for backwards compatibility
  statusQueryCmdRsp->info.dev_data
      .sbl_image_name[SHARED_IMEM_SBL_IMAGE_NAME_LEN - 1] = '\0';

  qutil::convertToHostFormat(statusQueryCmdRsp->info.header, info.devData);

  qutil::convertToHostFormat(statusQueryCmdRsp->info.dev_data, deviceFeatures_,
                             info.devData);

  // Copy the NSP Data to the host structure
  qutil::convertToHostFormat(statusQueryCmdRsp->info.nsp_data, info.nspData);

  deviceTimeOffset_ = statusQueryCmdRsp->info.dev_data.qts_time_offset_usecs;

  // Device status is good
  std::memcpy(info.name, getDeviceName().c_str(), sizeof(info.name));
  info.devStatus = QDevStatus::QDS_READY;
  devStatus_ = QDevStatus::QDS_READY;

  return QS_SUCCESS;

exit:
  devStatus_ = QDevStatus::QDS_ERROR;
  return status;
}

QStatus QRuntimePlatformKmdDevice::queryStatus(QDevInfo &devInfo) {
  return getStatus(devInfo);
}

QStatus QRuntimePlatformKmdDevice::openCtrlChannel(QCtrlChannelID ccID,
                                                   int &fd) {

  std::string path;
  QOsal::getCtrlChannelPath(path, pciInfo_, ccID);

  fd = open(path.c_str(), O_RDWR | O_CLOEXEC);
  if (-1 == fd) {
    LogWarn("failed to open ctrl channel {} : {}", ccID,
            QOsal::strerror_safe(errno));
    return QS_ERROR;
  }

  return QS_SUCCESS;
}

QStatus
QRuntimePlatformKmdDevice::getDeviceAddrInfo(qutil::DeviceAddrInfo &devInfo) {
  devInfo = std::make_pair(devID_, pciInfoString_);
  return QS_SUCCESS;
}

QStatus QRuntimePlatformKmdDevice::closeCtrlChannel(int fd) {
  close(fd);
  return QS_SUCCESS;
}

QStatus
QRuntimePlatformKmdDevice::isDeviceFeatureEnabled(qutil::DeviceFeatures feature,
                                                  bool &isEnabled) const {
  QStatus status = QS_SUCCESS;

  switch (feature) {
  case qutil::FW_FUSA_CRC_ENABLED:
    isEnabled = deviceFeatures_[NNC_DEVICE_FEATURE_FLAG_AUTO_SKU_BITPOS];
    break;
  default:
    status = QS_INVAL;
    break;
  }
  return status;
}

QStatus QRuntimePlatformKmdDevice::getDeviceFeatures(
    QDeviceFeatureBitset &features) const {
  features = deviceFeatures_;
  return QS_SUCCESS;
}

QStatus QRuntimePlatformKmdDevice::getBoardInfo(QBoardInfo &boardInfo) {
  manage_msg_t nncMsg;
  uint8_t data[QAIC_MANAGE_MAX_MSG_LENGTH] = {};
  nnc_cmd_status_query_response_t *statusQueryCmdRsp;
  QDeviceFeatureBitset deviceFeatures;

  nncMsg.data = (uint64_t)data;

  QStatus status = QS_ERROR;

  status = statusQuery(nncMsg, statusQueryCmdRsp);
  if (status != QS_SUCCESS) {
    goto exit;
  }

  /// NULL terminate any strings.  Device should do this, but we
  /// cannot make that assumption. This is incorrect. We are maintaining
  /// this for backwards compatibility
  statusQueryCmdRsp->info.dev_data
      .sbl_image_name[SHARED_IMEM_SBL_IMAGE_NAME_LEN - 1] = '\0';

  qutil::convertToHostFormat(statusQueryCmdRsp->info.dev_data, deviceFeatures_,
                             boardInfo);

exit:
  return status;
}

QStatus
QRuntimePlatformKmdDevice::getTelemetryInfo(QTelemetryInfo &telemetryInfo) {
  return qRuntimePlatformDeviceInterface_->getTelemetryInfo(telemetryInfo,
                                                            pciInfoString_);
}

const QPciInfo *QRuntimePlatformKmdDevice::getDevicePciInfo() {
  return &pciInfo_;
}

QDevInterfaceEnum QRuntimePlatformKmdDevice::getDeviceInterfaceType() const {
  return deviceInterfaceType_;
};

shQRuntimePlatformDeviceInterface &
QRuntimePlatformKmdDevice::getQRuntimePlatformDeviceInterface() {
  return qRuntimePlatformDeviceInterface_;
}

const char *QRuntimePlatformKmdDevice::nncErrorStr(int status) {
  const char *retVal;

  switch (status) {
  case NNC_STATUS_PA_L2_INIT_ERROR:
    retVal = "L2Init mapping failed";
    break;
  case NNC_STATUS_PA_GSM_CONF_MMAP_ERROR:
    retVal = "GSM mapping failed";
    break;
  case NNC_STATUS_ERROR_NO_NW_TO_DEACTIVATE:
    retVal = "Nw already deactivated";
    break;
  case NNC_STATUS_PD_MMC_MAP_ERROR:
    retVal = "MMC mapping failed in deactivate";
    break;
  case NNC_STATUS_DE_CLN_ACK_FAILED_ERROR:
    retVal = "Wait from User PD failed in Deactivate";
    break;
  case NNC_STATUS_DE_MQ_SEND_FAILED_ERROR:
    retVal = "Failed to send msg to PD in Deactivate";
    break;
  case NNC_STATUS_M_WAIT_ON_S_SYNC_CMD_TIMEOUT_ERROR:
    retVal = "Master NSP Timeout while waiting for CMD/State";
    break;
  case NNC_STATUS_M_WAIT_ON_S_SYNC_CMD_FAILED_ERROR:
    retVal = "Master NSP Timeout while waiting for CMD/State";
    break;
  case NNC_STATUS_PA_MCAST_RANGE_CHANGE_ERROR:
    retVal = "Failed to change mcast range";
    break;
  case NNC_STATUS_PA_SET_MCAST_ERROR:
    retVal = "Failed to set Mcast range in Pre-activate";
    break;
  case NNC_STATUS_BW_MON_MMAP_ERROR:
    retVal = "Failed to map bw monitor region";
    break;
  case NNC_STATUS_PA_MN_RSP_W_ERROR:
    retVal = "Failed in Pre-activate RSP";
    break;
  case NNC_STATUS_NW_MAP_ERROR:
    retVal = "Failed to create nnc mapping";
    break;
  case NNC_STATUS_MMCADDR_MAP_ERROR:
    retVal = "Failed to create mmcaddr mapping";
    break;
  case NNC_STATUS_MQ_SEND_ERROR:
    retVal = "Failed to send MQ";
    break;
  case NNC_STATUS_PD_SIG_WAIT_ERROR:
    retVal = "Wait from PD failed in activate";
    break;
  case NNC_STATUS_VER_MISMATCH_ERROR:
    retVal = "NNC exec context version mismatch";
    break;
  case NNC_STATUS_MMAP_ERROR:
    retVal = "Failed to map region";
    break;
  case NNC_STATUS_AC_SYNC_ERROR:
    retVal = "Sync error";
    break;
  case NNC_STATUS_DPS_NOT_ENABLED:
    retVal = "DPS not enabled on device";
    break;
  case NNC_STATUS_XPU_ERROR:
    retVal = "XPU program error";
    break;
  case NNC_STATUS_NSP_MCID_COUNT_EXCEED_MAX_LIMIT:
    retVal = "MCID count exceeds more than max supported limit";
    break;
  case NNC_STATUS_PROTO_NOT_SUPPORTED:
    retVal = "Invalid protocol type";
    break;
  case NNC_STATUS_GET_ACT_RECORD_ERROR:
    retVal = "Failed to get activation record from lut";
    break;
  case NNC_STATUS_RETRIEVAL_NSP_4M_METADATA_ERROR:
    retVal = "Parse NSP from metadata failed";
    break;
  case NNC_STATUS_RETRIEVAL_METADATA_ERROR:
    retVal = "Failed to parse metadata";
    break;
  case NNC_SWITCH_CMD_VCID_INVALID:
    retVal = "Invalid vcid";
    break;
  case NNC_STATUS_SWITCH_AC_STATE_ERROR:
    retVal = "Switch cmd sanity check failed";
    break;
  case NNC_STATUS_SWITCH_CMD_SEQ_MISMATCH_ERROR:
    retVal = "Switch cmd sequence failed";
    break;
  case NNC_STATUS_SWITCH_PARAM_ERROR:
    retVal = "Switch cmd param error(Invalid number of switch cmd)";
    break;
  case NNC_STATUS_NUMNSP_RETRIEVAL_ERROR:
    retVal = "Failed to retrieve NSP count from metadata";
    break;
  case NNC_STATUS_VCID_RETRIEVAL_ERROR:
    retVal = "Failed to retrieve vcid from activation record";
    break;
  case NNC_STATUS_META_RETRIEVAL_ERROR:
    retVal = "Failed to parse metadata";
    break;
  case NNC_STATUS_RETRIEVAL_PNSP_ID_ERROR:
    retVal = "Failed to retrive physical nsp id from ac record";
    break;
  case NNC_STATUS_VC_SETUP_ERROR:
    retVal = "Failed to setup dma bridge";
    break;
  case NNC_STATUS_GSM_CONF_ERROR:
    retVal = "Failed to configure GSM";
    break;
  case NNC_STATUS_DYN_CONST_RESOURCE_NOT_FOUND:
    retVal = "Failed to parse dynamic const from metadata";
    break;
  case NNC_STATUS_L2TCM_INIT_STATE_INVALID_ERROR:
    retVal = "Failed to get L2TCM Meminfo data from metadata";
    break;
  case NNC_STATUS_RETRIEVAL_L2TCM_INIT_STATE_ERROR:
    retVal = "Failed to get L2TCM Meminfo from metadata";
    break;
  case NNC_STATUS_MCID_CONFIG_ERROR:
    retVal = "Failed to configure mcid";
    break;
  case NNC_STATUS_RETRIEVAL_PRO_IMG_ERROR:
    retVal = "Failed to parse Program ELF info from metadata";
    break;
  case NNC_STATUS_LUT_UPDATE_FAILED_ERROR:
    retVal = "Failed to update activation lut";
    break;
  case NNC_STATUS_SAFETY_SWIV_ELF_ERROR:
    retVal = "Safety SWIV check error on network elf image verification";
    break;
  case NNC_STATUS_SAFETY_SWIV_CONST_ERROR:
    retVal = "Safety SWIV check error on constant image verification";
    break;
  case NNC_STATUS_SAFETY_SWIV_METADATA_ERROR:
    retVal = "Safety SWIV check error on metadata image verification";
    break;
  case NNC_STATUS_SAFETY_BCS_MHI_ERROR:
    retVal = "Safety BCS check error on MHI path";
    break;
  case NNC_STATUS_SAFETY_BCS_GLINK_ERROR:
    retVal = "Safety BCS check error on Glink path";
    break;
  case NNC_STATUS_SAFETY_BCS_METADATA_ERROR:
    retVal = "Safety BCS check error on Metadata corruption";
    break;
  case NNC_STATUS_SAFETY_BCS_AC_RECORD_ERROR:
    retVal = "Safety BCS check error on Activation record corruption";
    break;
  case NNC_STATUS_SS_UNDER_SSR_ERROR:
    retVal = "Operation failed due to NSP under SSR";
    break;
  case NNC_STATUS_VC_CONFIGURE_ERROR:
    retVal = "VC configuration/enable error";
    break;
  case NNC_STATUS_QUEUE_TOO_SMALL:
    retVal = "P2P VC queue size too small";
    break;
  case NNC_STATUS_IATU_ALLOC_ERROR:
    retVal = "Failed to allocate IATU resource";
    break;
  case NNC_STATUS_OATU_ALLOC_ERROR:
    retVal = "Failed to allocate OATU resource";
    break;
  case NNC_STATUS_DATABASE_IDS_EXHAUSTED:
    retVal = "Database ID's got exhausted";
    break;
  case NNC_STATUS_DATABASE_ID_NOT_FOUND:
    retVal = "Database ID not found in Queue";
    break;
  case NNC_STATUS_RESERVATION_IDS_EXHAUSTED:
    retVal = "Reservation ID's got exhausted";
    break;
  case NNC_STATUS_CMD_LENGTH_ERROR:
    retVal = "Unexpected command length";
    break;
  case NNC_STATUS_NSP_RESPONSE_ERROR:
    retVal = "NSP response timeout /error";
    break;
  case NNC_STATUS_SEM_ALLOC_ERROR:
    retVal = "Failed to allocate GSM semaphore resource";
    break;
  case NNC_STATUS_VC_ALLOC_ERROR:
    retVal = "Failed to allocate virtual channel";
    break;
  case NNC_STATUS_NSP_ALLOC_ERROR:
    retVal = "Failed to allocate NSP resources";
    break;
  case NNC_STATUS_MCID_ALLOC_ERROR:
    retVal = "Failed to allocate multicast ID resources";
    break;
  case NNC_STATUS_RESOURCE_NOT_FOUND:
    retVal = "Requested Resource Not found";
    break;
  case NNC_STATUS_BUFFER_TOO_SMALL:
    retVal = "Buffer too small";
    break;
  case NNC_STATUS_OPERATION_NOT_PERMITTED:
    retVal = "NNC permission denied for invalid handle or attempt to release a "
             "resource in use";
    break;
  case NNC_STATUS_NOT_INITIALIZED:
    retVal = "NNC not yet initialized";
    break;
  case NNC_STATUS_TIMED_OUT_WAITING:
    retVal = "Timed out waiting for a state change to occur";
    break;
  case NNC_STATUS_PARAMETER_ERROR:
    retVal =
        "Error caused by an invalid parameter passed to an internal function";
    break;
  case NNC_STATUS_METADATA_ERROR:
    retVal = "Version mismatch / error processing the ELF metadata";
    break;
  case NNC_STATUS_MEM_ALLOC_ERROR:
    retVal = "Memory resource exhausted";
    break;
  case NNC_STATUS_UNDEFINED_REQUEST:
    retVal = "The request specified is not defined";
    break;
  case NNC_STATUS_DMA_ERROR:
    retVal = "Configuration of the DMA virtual channel failed";
    break;
  case NNC_STATUS_ERROR:
    retVal = "General error occurred";
    break;
  case NNC_STATUS_SUCCESS:
    retVal = "Successful";
    break;
  default:
    retVal = "Unknown Error";
    break;
  }

  return retVal;
}

bool QRuntimePlatformKmdDevice::isValid() const {
  if (!qRuntimePlatformDeviceInterface_) {
    return false;
  }

  if (devStatus_ != QDevStatus::QDS_READY) {
    return false;
  }

  return true;
}

QStatus
QRuntimePlatformKmdDevice::getDeviceTimeOffset(uint64_t &deviceTimeOffset) {
  if (devStatus_ != QDevStatus::QDS_READY) {
    deviceTimeOffset = 0;
    return QS_INVAL;
  }
  deviceTimeOffset = deviceTimeOffset_;
  return QS_SUCCESS;
}

} // namespace qaic
