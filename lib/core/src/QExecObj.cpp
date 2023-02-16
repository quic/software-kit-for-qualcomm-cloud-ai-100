// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include <fstream>
#include <iostream>
#include <limits>

#include "QExecObj.h"
#include "QAicRuntimeTypes.h"
#include "QComponent.h"
#include "QBindingsParser.h"
#include "QContext.h"
#include "QLogger.h"
#include "QUtil.h"
#include <google/protobuf/util/json_util.h>

namespace qaic {

static std::atomic<QAicObjId> NextUniqueObjId{0};

// Inference retry count during Sub System Restart
constexpr uint16_t inferenceRetryCount = 5;

//==================================================================================
// QExecObj Static Api
// createExecObj and releaseExecObj are constructors and destructors for
// QExecObj
//==================================================================================
shQExecObj QExecObj::createExecObj(shQContext context,
                                   const QAicExecObjProperties *properties,
                                   QID qid, const shQProgram &program,
                                   uint32_t *numBuffers, QBuffer **buffers,
                                   QStatus &status) {
  if ((context == nullptr) || (program == nullptr)) {
    status = QS_INVAL;
    return nullptr;
  }

  shQExecObj shExecObj = std::make_shared<QExecObj>(context, properties, qid,
                                                    program, 0 /*numBuffers*/);

  if ((shExecObj == nullptr) || (shExecObj.get() == nullptr)) {
    status = QS_NOMEM;
    return nullptr;
  }

  QProgramDevice *programDevice = program->getProgramDevice();
  if (programDevice == nullptr) {
    status = QS_INVAL;
    return nullptr;
  }
  status = programDevice->registerExecObj(shExecObj);
  if (status != QS_SUCCESS) {
    return nullptr;
  }

  status = shExecObj->init();
  if (status != QS_SUCCESS) {
    programDevice->unregisterExecObj(shExecObj);
    return nullptr;
  }

  if (buffers != nullptr) {
    *buffers = shExecObj->getBufferArrayPtr();
  }

  if (numBuffers != nullptr) {
    *numBuffers = shExecObj->getNumBuffers();
  }

  context->registerExecObj(shExecObj);
  status = QS_SUCCESS;
  return shExecObj;
}

QStatus QExecObj::releaseExecObj() {
  context_->unRegisterExecObj(this);
  programDevice_->unregisterExecObj(this);
  return QS_SUCCESS;
}

// Check if any input has partial tensor
static bool checkPartialTensor(const aicnwdesc::networkDescriptor *netdesc) {
  if (netdesc == nullptr) {
    return false;
  }

  for (const auto &input : netdesc->inputs()) {
    if (input.is_partial_allowed())
      return true;
  }
  return false;
}

QExecObj::QExecObj(shQContext &context, const QAicExecObjProperties *properties,
                   QID qid, const shQProgram &program, uint32_t numBuffers)
    : QComponent("ExecObj", NextUniqueObjId++, context.get()),
      QIAicApiContext(context), sessionID_(0),
      properties_(defaultExecObjProperties_), dev_(qid), program_(program),
      ioDescPbData_{0, nullptr}, numBuffers_(numBuffers),
      metadata_(program->getMetadata()), rt_(context_->rt()), qnn_(nullptr),
      netdesc_(program->getNetworkDesc()), programDevice_(nullptr),
      initialized_(false), hasPartialTensor_(checkPartialTensor(netdesc_)) {
  if (properties != nullptr) {
    properties_ = *properties;
  }
  LogDebugApi("Created ExecObj ID:{}", Id_);
}

QExecObj::~QExecObj() {
  if (((properties_ & static_cast<uint32_t>(
                          QAicExecObjPropertiesBitField::
                              QAIC_EXECOBJ_PROPERTIES_AUTO_LOAD_ACTIVATE)) !=
       0) &&
      (!program_->isManuallyActivated())) {
    infHandle_.reset();
    program_->putActivationHandle(dev_);
  }
}

QStatus QExecObj::prepareToSubmit() {
  if (!programDevice_) {
    return QS_ERROR;
  }
  if (programDevice_->isReady()) {
    return QS_SUCCESS;
  } else {
    if (programDevice_->readyProgram() != QS_SUCCESS) {
      return QS_ERROR;
    } else {
      return QS_SUCCESS;
    }
  }
}

bool QExecObj::isReady() {
  if (!programDevice_) {
    return false;
  }
  return programDevice_->isReady();
}

QStatus QExecObj::setData(const uint32_t numBuffers, const QBuffer *buffers) {

  if ((programDevice_ == nullptr) || (!programDevice_->isActive())) {
    LogErrorApi("{}: Invalid program state, not activated", __FUNCTION__);
    return QS_INVAL;
  }
  if (bufferBindings_.userBindings.size() != numBuffers) {
    bufferBindings_.userBindings.resize(numBuffers);
  }

  // -------------------------------------------------------------------------
  // Set the buffer bindings for Pre/Post transformation
  // -------------------------------------------------------------------------
  QAicIoBufferInfo *bufInfo = nullptr;
  QStatus status = program_->getIoBufferInfo(&bufInfo);
  if (status != QS_SUCCESS) {
    LogErrorApi("Failed to get buffer info from {}",
                program_->getNetworkName());
    return QS_ERROR;
  }

  for (uint32_t i = 0; i < numBuffers; i++) {
    if (hasPartialTensor_ &&
        (buffers[i].size > bufInfo->bufferMappings[i].size)) {
      LogErrorApi("Unexpected buffer size at index {}, buffer size {}, "
                  "expected size {}",
                  i, buffers[i].size, bufInfo->bufferMappings[i].size);
      return QS_ERROR;
    }
  }

  for (uint32_t i = 0; i < numBuffers; i++) {
    bufferBindings_.userBindings[i].ptr = (char *)buffers[i].buf;
    bufferBindings_.userBindings[i].size = buffers[i].size;
  }

  return QS_SUCCESS;
}

// Submit to Hardware
QStatus QExecObj::submit() {
  QStatus status = QS_SUCCESS;
  if ((infHandle_ == nullptr) || (programDevice_ == nullptr)) {
    return QS_ERROR;
  }
  if (!programDevice_->isDeviceReady()) {
    return QS_DEV_ERROR;
  }

  QNeuralNetworkInterface *qnn = program_->nn();
  if (qnn == nullptr) {
    LogError("unexpected null pointer qnn");
    return QS_ERROR;
  }
  std::vector<QBuffer> outBufs;

  std::uint16_t retryCount = inferenceRetryCount;
  while (retryCount--) {
    status = qnn->enqueueData(infHandle_.get());
    if (status != QS_SUCCESS) {
      /* Wait for device to recover */
      std::this_thread::sleep_for(std::chrono::seconds(1));
      LogWarnApi("Enqueue data retryCount {}",
                 (inferenceRetryCount - retryCount));
      if (programDevice_->isDeviceReady()) {
        continue;
      }
    }
    break;
  }

  if (status != QS_SUCCESS) {
    LogErrorApi("Failed to enqueue data");
    return status;
  }

  return status;
}

QStatus QExecObj::finish() {
  QStatus status = QS_SUCCESS;

  if ((infHandle_ == nullptr) || (programDevice_ == nullptr)) {
    return QS_ERROR;
  }
  if (!programDevice_->isDeviceReady()) {
    return QS_DEV_ERROR;
  }

  QNeuralNetworkInterface *qnn = program_->nn();
  if (qnn == nullptr) {
    LogErrorApi("Unexpected null pointer qnn");
    return QS_ERROR;
  }

  status = qnn->wait(infHandle_.get());
  if (status != QS_SUCCESS) {
    LogErrorApi("wait in kernel failed");
    return status;
  }
  postTransform();
  LogDebugApi("Wait completed for inference");

  return status;
}

QStatus
QExecObj::validateExecObjProperties(const QAicExecObjProperties *properties) {
  uint32_t invalidProperties =
      ~(static_cast<uint32_t>(QAicExecObjPropertiesBitField::
                                  QAIC_EXECOBJ_PROPERTIES_AUTO_LOAD_ACTIVATE));
  if ((properties != nullptr) && ((*properties & invalidProperties) != 0)) {
    return QS_INVAL;
  }
  return QS_SUCCESS;
}

QBuffer *QExecObj::getBufferArrayPtr() { return userQBuffersVec_.data(); }

QStatus QExecObj::preTransform() {

  QStatus status = QS_ERROR;

  // Perform Pre-Processing
  std::vector<uint64_t> dmaBufferSizes;
  if (ppHandle_->processInputBuffers(bufferBindings_, dmaBufferSizes) ==
      QS_SUCCESS) {
    status = qnn_->prepareData(infHandle_.get(), dmaBufferSizes);
  }
  return status;
}

QStatus QExecObj::postTransform() {
  // Perform Post-Processing
  return ppHandle_->processOutputBuffers(bufferBindings_);
}

QStatus QExecObj::run() {
  QStatus status = QS_SUCCESS;
  // Run requires that the program be activated
  if ((programDevice_ == nullptr) || (!programDevice_->isActive())) {
    LogErrorApi("{}: Invalid program state, not activated", __FUNCTION__);
    return QS_INVAL;
  }
  status = ppHandle_->validateTransformKind();
  if (status != QS_SUCCESS) {
    LogError("Transform Sequence validation failed");
    return status;
  }

  status = preTransform();
  if (status != QS_SUCCESS) {
    LogError("Failed to perform perTransformation");
    return status;
  }

  status = submit();
  if (status != QS_SUCCESS) {
    LogErrorApi("Failed to run program at submit stage");
    return status;
  }

  status = finish();
  if (status != QS_SUCCESS) {
    LogErrorApi("Failed to run program at finish stage");
    return status;
  }

  return status;
}

//----------------------------------------------------------------------
// Private Methods
//----------------------------------------------------------------------

bool QExecObj::initPrePostTransforms() {
  // This function will initialize ppHandle_ or return false
  // netdesc_ is the default network descriptor created by the program
  ppHandle_ = std::make_unique<QPrePostProc>(netdesc_, context_);
  if (ppHandle_ == nullptr) {
    LogErrorApi("Error in creating PrePost Handle");
    return false;
  } else {
    return true;
  }
}

QStatus QExecObj::init() {
  QStatus status;

  if ((program_ == nullptr) || (netdesc_ == nullptr)) {
    LogErrorApi("Invalid program or network descriptor");
    return QS_ERROR;
  }

  if (programDevice_ == nullptr) {
    programDevice_ = program_->getProgramDevice();
    if (programDevice_ == nullptr) {
      LogErrorApi("Unable to get program Device Error");
      return QS_ERROR;
    }
  }

  uint32_t bufferDirSize = 0;
  const QDirection *bufferDirs = nullptr;

  numBuffers_ = netdesc_->inputs().size() + netdesc_->outputs().size();

  userQBuffersVec_.resize(numBuffers_);
  for (auto &b : userQBuffersVec_) {
    qutil::initQBuffer(b);
  }
  dmaQBuffersVec_.resize(netdesc_->dma_buffers().size());
  for (auto &b : dmaQBuffersVec_) {
    qutil::initQBuffer(b);
  }
  program_->getUserBufferDirections(bufferDirs, bufferDirSize);
  if (bufferDirSize != numBuffers_) {
    LogErrorApi("inconsistent number of buffers in ExecObj, specified{}, "
                "networkDesc has:{} in DmaBuffers field",
                numBuffers_, bufferDirSize);
    return QS_ERROR;
  }
  program_->getDmaBufferSizes(dmaQBuffersVec_.data(), dmaQBuffersVec_.size());

  if (!initPrePostTransforms()) {
    return QS_ERROR;
  }

  // The requirement here is that the program is loaded and activated
  // for the ExecObj to be created
  // Here we call program load and activate, however
  // the program will be loaded and activated only once for the device
  // This operation is very fast normally, it simply checks the state
  // and if already activated it returns.
  // Also this is done at object creation, not in the inference timeline
  // Program must be loaded to get nn,
  if (((properties_ & static_cast<uint32_t>(
                          QAicExecObjPropertiesBitField::
                              QAIC_EXECOBJ_PROPERTIES_AUTO_LOAD_ACTIVATE)) !=
       0) &&
      (!program_->isManuallyActivated())) {

    status = program_->load();
    if (status != QS_SUCCESS) {
      LogErrorApiReport(QAicErrorType::QIAC_ERROR_EXECOBJ_RUNTIME, nullptr, 0,
                        " Error loading program");
      return QS_ERROR;
    }
    status = program_->getActivationHandle(
        dev_, programDevice_, QProgram::ACTIVATION_TYPE_FULL_ACTIVATION);
    if (status != QS_SUCCESS) {
      LogErrorApiReport(QAicErrorType::QIAC_ERROR_EXECOBJ_RUNTIME, nullptr, 0,
                        " Error activating program");
      return QS_ERROR;
    }
  } else {
    programDevice_ = program_->getProgramDevice();
  }
  if (programDevice_ == nullptr) {
    LogError("Invalid Null Program Device pointer");
    return QS_ERROR;
  }

  if (!programDevice_->isActive()) {
    LogError("Cannot Init ExecObj, program is not active (not activated)");
    return QS_INVAL;
  }

  qnn_ = program_->nn();
  if (qnn_ == nullptr) {
    LogErrorApiReport(QAicErrorType::QIAC_ERROR_EXECOBJ_RUNTIME, nullptr, 0,
                      " Error creating exec obj");
    return QS_ERROR;
  }

  infHandle_ =
      qnn_->getInfHandle(dmaQBuffersVec_.data(), dmaQBuffersVec_.size(),
                         bufferDirs, program_->hasPartialTensor());

  if (infHandle_ == nullptr) {
    LogErrorApiReport(QAicErrorType::QIAC_ERROR_EXECOBJ_RUNTIME, nullptr, 0,
                      " Error creating exec obj, failed to get infHandle");
    return QS_ERROR;
  }

  if (qnn_->getInfBuffers(infHandle_.get(), dmaBuffers_) != QS_SUCCESS) {
    return QS_ERROR;
  }

  // DMA Buffer bindings won't change, we keep the same DMA buffer for the
  // life of this object
  for (auto &buf : dmaBuffers_) {
    aicppp::BufferBinding binding = {(char *)buf.buf, buf.size};
    binding.ptr = (char *)buf.buf;
    binding.size = buf.size;
    bufferBindings_.dmaBindings.emplace_back(binding);
  }

  initialized_ = true;
  return QS_SUCCESS;
}

} // namespace qaic
