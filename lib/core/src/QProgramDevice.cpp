// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include <iostream>
#include <fstream>

#include "QUtil.h"
#include "QAicRuntimeTypes.h"
#include "QContext.h"
#include "QProgramDevice.h"
#include "QProgram.h"
#include "QBindingsParser.h"
#include "QLogger.h"
#include "elfio/elfio.hpp"
#include "metadataflatbufDecode.hpp"
#include "QExecObj.h"

namespace qaic {

static std::atomic<QAicObjId> NextUniqueObjId{0};
constexpr NotifyDeviceStateInfoPriority programDevicePriority = 4;

QProgramDevice::QProgramDevice(shQContext context, QProgram *program, QID dev)
    : QHsm(Q_STATE_CAST(initial)),
      QComponent("ProgDev", NextUniqueObjId++, context.get()),
      QIAicApiContext(context), dev_(dev), qnaid_(UINT32_MAX),
      program_(program), nnImage_(nullptr), nnConstants_(nullptr),
      qnn_(nullptr), rt_(nullptr), devInfoValidated_(false) {
  if ((program_ != nullptr) && (program_->context_ != nullptr)) {
    rt_ = program->context_->rt();
  }
  context_->registerNotifyDeviceStateInfo(programDevicePriority, this,
                                          deviceStateInfoFuncCb_);
}

uQProgramDevice QProgramDevice::Factory(shQContext context, QProgram *program,
                                        QID dev) {
  uQProgramDevice obj = std::unique_ptr<QProgramDevice>(
      new (std::nothrow) QProgramDevice(context, program, dev));

  if ((!obj) || !obj->initialize()) {
    return nullptr;
  }
  return obj;
}

QProgramDevice::~QProgramDevice() {
  if (isActive()) {
    deactivate();
  }
  context_->unRegisterNotifyDeviceStateInfo(this);
}

QNeuralNetworkInterface *QProgramDevice::nn() { return qnn_; }

ProgramState QProgramDevice::getProgramState() {
  // Based on the current HSM State, determine the program state
  // QStateHandler curState = state();

  // Use if/else here because State CAST returns a function pointer, which we
  // can't use in a switch

  // Start from the top of the hiarchy and with the most likely state
  // for optimal performance
  if (isIn(Q_STATE_CAST(this->ac_ready))) {
    return STATE_PROGRAM_READY;
  } else if (isIn(Q_STATE_CAST(this->loaded_super))) {
    return STATE_PROGRAM_LOADED;
  } else if (isIn(Q_STATE_CAST(this->initial)) ||
             isIn(Q_STATE_CAST(this->created)) ||
             isIn(Q_STATE_CAST(this->created_super))) {
    return STATE_PROGRAM_CREATED;
  } else if (isIn(Q_STATE_CAST(this->error_super))) {
    if (isIn(Q_STATE_CAST(this->activate_failed))) {
      return STATE_PROGRAM_ACTIVATE_ERROR;
    } else if (isIn(Q_STATE_CAST(this->load_failed))) {
      return STATE_PROGRAM_LOAD_ERROR;
    } else if (isIn(Q_STATE_CAST(this->device_error))) {
      return STATE_PROGRAM_DEVICE_ERROR;
    }
  }
  return STATE_PROGRAM_TRANSITION;
}

bool QProgramDevice::isInError() {
  std::unique_lock<std::mutex> lk(programMutex_);
  return isIn(Q_STATE_CAST(this->error_super));
}

void QProgramDevice::getProgramInfo(QAicProgramInfo &info) {
  AicMetadataFlat::MetadataT metadata;
  info.numNsp = 0;
  info.numMcid = 0;
  std::unique_lock<std::mutex> lk(programMutex_);
  switch (getProgramState()) {
  case STATE_PROGRAM_CREATED:
    info.status = QAicProgramStatus::QAIC_PROGRAM_INIT;
    break;
  case STATE_PROGRAM_LOADED:
    info.status = QAicProgramStatus::QAIC_PROGRAM_LOADED;
    break;
  case STATE_PROGRAM_READY:
    info.status = QAicProgramStatus::QAIC_PROGRAM_FULLY_ACTIVATED;
    break;
  case STATE_PROGRAM_LOAD_ERROR:
    info.status = QAicProgramStatus::QIAC_PROGRAM_LOAD_ERROR;
    break;
  case STATE_PROGRAM_ACTIVATE_ERROR:
    info.status = QAicProgramStatus::QIAC_PROGRAM_ACTIVATE_ERROR;
    break;
  default:
    info.status = QAicProgramStatus::QAIC_PROGRAM_ERROR;
    break;
  }
  lk.unlock();
  info.programID = Id_;
  if (nnImage_) {
    info.loadedID = nnImage_->getImageID();
  }
  metadata = program_->getMetadata();
  if (!metadata.networkName.empty()) {
    info.numNsp = metadata.numNSPs;
    info.numMcid =
        metadata.nspMulticastTables[0].get()->multicastEntries.size();
  }
  const aicnwdesc::networkDescriptor *networkDescriptor =
      program_->getNetworkDesc();
  info.batchSize = networkDescriptor->batch_size();

  if (networkDescriptor->has_stats() == true) {
    info.isAicStatsAvailable = 1;
  } else {
    info.isAicStatsAvailable = 0;
  }
}

bool QProgramDevice::isActive() {
  std::unique_lock<std::mutex> lk(programMutex_);
  LogDebugApi("Current Program state:{}", str(getProgramState()));
  return (isIn(Q_STATE_CAST(this->active_super)));
}

bool QProgramDevice::isReady() {
  std::unique_lock<std::mutex> lk(programMutex_);
  LogDebugApi("Current Program state {}", str(getProgramState()));
  return (isIn(Q_STATE_CAST(this->ac_ready)));
}

bool QProgramDevice::isDeviceReady() {
  std::unique_lock<std::mutex> lk(programMutex_);
  LogDebugApi("Current Program state {}", str(getProgramState()));
  return !isIn(Q_STATE_CAST(device_error));
}

bool QProgramDevice::isLoaded() {
  std::unique_lock<std::mutex> lk(programMutex_);
  LogDebugApi("Current Program state {}", str(getProgramState()));
  return (isIn(Q_STATE_CAST(this->ac_ready)) ||
          isIn(Q_STATE_CAST(this->loaded)));
}

void QProgramDevice::setQNAID() { qnaid_ = qnn_->getId(); }

QStatus QProgramDevice::getInferenceCompletedCount(uint64_t &count) {
  if (qnn_ == nullptr) {
    count = 0;
    return QS_ERROR;
  }
  count = qnn_->getInfCount();
  return QS_SUCCESS;
}

QStatus QProgramDevice::getDeviceQueueLevel(uint32_t &fillLevel,
                                            uint32_t &queueSize) {
  if (qnn_ == nullptr) {
    return QS_ERROR;
  }
  fillLevel = qnn_->getVcQueueLevel();
  queueSize = qnn_->getVcQueueSize();
  return QS_SUCCESS;
}

QStatus QProgramDevice::load() {
  QStatus status = QS_SUCCESS;
  std::unique_lock<std::mutex> lk(programMutex_);

  if (isIn(Q_STATE_CAST(this->loaded_super))) {
    return QS_SUCCESS;
  }
  setHsmSignalWithLock(LOAD_SIG);
  run();
  ProgramState programState = getProgramState();
  if (programState != STATE_PROGRAM_LOADED) {
    LogErrorApi("Unexpected state {}", str(programState));
    // context_->sendErrorReport("Failed to Load program",
    //                           QAIC_ERROR_PROGRAM_RUNTIME_LOAD_FAILED,
    //                           nullptr,
    //                           0);
    status = QS_ERROR;
  }
  return status;
}

QStatus QProgramDevice::processActivateCmd(QProgramActivationCmd cmd) {
  QStatus status = QS_INVAL;
  LogDebugApi("QProgramDevice 0x{} ActivationCmd {}", (uint64_t) this, cmd);
  switch (cmd) {
  case QProgram_CMD_ACTIVATE_FULL:
    status = activate();
    break;
  case QProgram_CMD_DEACTIVATE_FULL:
    // Deactivating while ExecObj are registered with program device
    // will cause the ExecObj to become invalid, unless they are tied
    // to a VC Reservation
    if (execObjs_.size() > 0) {
      LogErrorApi("Cannot deactivate program, it has active ExecObj");
      return QS_INVAL;
    }
    status = deactivate();
    break;
  default:
    LogErrorApi("Received unsupported Activation command {}", cmd);
    break;
  }
  return status;
}

QStatus QProgramDevice::unload() {
  QStatus status = QS_SUCCESS;
  ProgramState programState;
  std::unique_lock<std::mutex> lk(programMutex_);
  if (isIn(Q_STATE_CAST(this->active_super))) {
    if (execObjs_.size() > 0) {
      LogError("Cannot deactivate program with active execObj");
      return QS_ERROR;
    }
    setHsmSignalWithLock(DEACTIVATE_SIG);
    run();
  }
  setHsmSignalWithLock(UNLOAD_SIG);
  run();
  programState = getProgramState();
  if ((programState != STATE_PROGRAM_CREATED) &&
      (programState != STATE_PROGRAM_LOADED)) {
    LogErrorApi("Failed to Unload, invalid state detected in program{}",
                str(getProgramState()));
    status = QS_ERROR;
  }
  return status;
}

QStatus
QProgramDevice::notifyDeviceStateInfo(std::shared_ptr<QDeviceStateInfo> event) {
  QID qid = event->qid();
  if (qid != dev_) {
    return QS_SUCCESS; // Ignore
  }

  uint32_t vcid = event->vcId();
  std::string deviceSBDF = event->deviceSBDF();

  std::unique_lock<std::mutex> lk(programMutex_);

  uint32_t qnnVcid = qutil::INVALID_VCID;
  if (qnn_ != nullptr) {
    qnnVcid = qnn_->getVcId();
  }

  if (event->deviceEvent() == DEVICE_DOWN) {
    // Set the program state
    setHsmSignalWithLock(DEVICE_DOWN_SIG);
    run();
  } else if (event->deviceEvent() == DEVICE_UP) {
    // Set the program state
    setHsmSignalWithLock(DEVICE_UP_SIG);
    run();
  } else if (event->deviceEvent() == VC_DOWN &&
             qnnVcid != qutil::INVALID_VCID && vcid == qnnVcid) {
    // Set the program state
    setHsmSignalWithLock(VC_DOWN_SIG);
    run();
  } else if (event->deviceEvent() == VC_UP && qnnVcid != qutil::INVALID_VCID &&
             vcid == qnnVcid) {
    // Set the program state
    setHsmSignalWithLock(VC_UP_SIG);
    run();
  } else if (event->deviceEvent() == VC_UP_WAIT_TIMER_EXPIRY &&
             qnnVcid != qutil::INVALID_VCID && vcid == qnnVcid) {
    // Set the program state
    setHsmSignalWithLock(DEVICE_DOWN_SIG);
    run();
  } else {
    LogInfo("Ignored event: {} for QID: {} VcId: {} device: {} ",
            event->deviceEventName(), qid, vcid, deviceSBDF);
    return QS_SUCCESS;
  }

  LogWarn("Received event: {} for QID: {} VcId: {} device: {} ",
          event->deviceEventName(), qid, vcid, deviceSBDF);
  return QS_SUCCESS;
}

QStatus QProgramDevice::registerExecObj(const shQExecObj &execObj) {
  std::unique_lock<std::mutex> lk(execObjsLock_);
  execObjs_.push_back(execObj);
  return QS_SUCCESS;
}

QStatus QProgramDevice::unregisterExecObj(const shQExecObj &execObj) {
  std::unique_lock<std::mutex> lk(execObjsLock_);
  std::vector<shQExecObj>::iterator shExecObjToDelete =
      std::find(execObjs_.begin(), execObjs_.end(), execObj);
  if (shExecObjToDelete != execObjs_.end()) {
    execObjs_.erase(shExecObjToDelete);
    return QS_SUCCESS;
  }
  return QS_INVAL;
}

QStatus QProgramDevice::unregisterExecObj(const QExecObj *execObj) {
  std::unique_lock<std::mutex> lk(execObjsLock_);
  std::vector<shQExecObj>::iterator shExecObjToDelete =
      std::find_if(execObjs_.begin(), execObjs_.end(),
                   [&](shQExecObj &obj) { return obj.get() == execObj; });
  if (shExecObjToDelete != execObjs_.end()) {
    execObjs_.erase(shExecObjToDelete);
    return QS_SUCCESS;
  }
  return QS_INVAL;
}

//-------------------------------------------------------------------------------------------------
// Protected Methods
//-------------------------------------------------------------------------------------------------

QStatus QProgramDevice::readyProgram() {
  std::unique_lock<std::mutex> lk(programMutex_);
  if (!isIn(Q_STATE_CAST(this->loaded_super))) {
    lk.unlock();
    load();
  } else {
    lk.unlock();
  }

  lk.lock();
  if (isIn(Q_STATE_CAST(this->loaded_super))) {
    lk.unlock();
    activate();
  }

  lk.lock();
  if ((isIn(Q_STATE_CAST(this->ac_ready)))) {
    return QS_SUCCESS;
  } else {
    LogErrorApi("failed to ready program, result in {} state",
                str(getProgramState()))
  }
  return QS_ERROR;
}

//-------------------------------------------------------------------------------------------------
// Private Methods
//-------------------------------------------------------------------------------------------------

std::string QProgramDevice::str(ProgramState state) {
  switch (state) {
  case STATE_PROGRAM_CREATED:
    return "STATE_PROGRAM_CREATED";
  case STATE_PROGRAM_LOADED:
    return "STATE_PROGRAM_LOADED";
  case STATE_PROGRAM_READY:
    return "STATE_PROGRAM_READY";
  case STATE_PROGRAM_LOAD_ERROR:
    return "STATE_PROGRAM_LOAD_ERROR";
  case STATE_PROGRAM_ACTIVATE_ERROR:
    return "STATE_PROGRAM_ACTIVATE_ERROR";
  case STATE_PROGRAM_DEVICE_ERROR:
    return "STATE_PROGRAM_DEVICE_ERROR";
  case STATE_PROGRAM_TRANSITION:
    return "STATE_PROGRAM_TRANSITION";
  default:
    return "STATE_UNKNOWN";
  }
}

QStatus QProgramDevice::activate() {
  QStatus status = QS_SUCCESS;
  std::unique_lock<std::mutex> lk(programMutex_);
  if (isIn(Q_STATE_CAST(this->ac_ready))) {
    return QS_SUCCESS;
  }
  setHsmSignalWithLock(ACTIVATE_SIG);
  run();
  if (!isIn(Q_STATE_CAST(this->ac_ready))) {

    LogErrorApi("Failed to Activate program");

    status = QS_ERROR;
  }
  return status;
}

QStatus QProgramDevice::ready_request() {
  QStatus status = QS_SUCCESS;
  std::unique_lock<std::mutex> lk(programMutex_);
  if (getProgramState() == STATE_PROGRAM_READY) {
    return QS_SUCCESS;
  }
  setHsmSignalWithLock(ACTIVATE_SIG);
  run();
  if (getProgramState() != STATE_PROGRAM_READY) {
    // context_->sendErrorReport("Failed to Activate program in standby",
    //                           QAIC_ERROR_PROGRAM_RUNTIME_ACTIVATION_FAILED,
    //                           nullptr, 0);
    status = QS_ERROR;
  }
  return status;
}

QStatus QProgramDevice::deactivate() {
  QStatus status = QS_SUCCESS;
  ProgramState programState;
  std::unique_lock<std::mutex> lk(programMutex_);
  setHsmSignalWithLock(DEACTIVATE_SIG);
  run();
  programState = getProgramState();
  if ((programState != STATE_PROGRAM_LOADED) &&
      (programState != STATE_PROGRAM_CREATED)) {
    LogErrorApi(
        "Failed to Deactivate, invalid state detected in program state:{}",
        str(programState));
    status = QS_ERROR;
  }
  return status;
}

//-------------------------------------------------------------------------------------------------
// HSM Actions
//-------------------------------------------------------------------------------------------------
bool QProgramDevice::validate_action() {
  QStatus status;
  AicMetadataFlat::MetadataT metadata;

  // Validate only once
  if (!devInfoValidated_) {
    if ((program_ == nullptr) || (rt_ == nullptr)) {
      LogErrorApi("null program");
      return false;
    }
    metadata = program_->getMetadata();
    status = rt_->queryStatus(dev_, devInfo_);
    if (status != QS_SUCCESS) {
      LogErrorApi("Invalid response from queryStatus");
      return false;
    }

    if (devInfo_.devStatus != QDevStatus::QDS_READY) {
      LogErrorApi(
          "Failed to initialize program, device no in Ready state, state:{}",
          (uint64_t)devInfo_.devStatus);
      return false;
    }
    uint32_t hwMajor = (devInfo_.devData.hwVersion & 0xffff0000) >> 16;
    if (hwMajor != metadata.hwVersionMajor) {
      LogErrorApi("Failed to initialize program, HW Version incompatible, "
                  "program expects:Major Version{}, HW Reports:{}",
                  metadata.hwVersionMajor, hwMajor);
      return false;
    }
  }
  devInfoValidated_ = true;
  return true;
}

bool QProgramDevice::load_action() {

  if (program_ == nullptr) {
    LogErrorApi("Invalid null pointer");
    return false;
  }
  if (!deviceImage_) {
    QProgramContainer *programContainer = program_->getContainer();
    if (programContainer == nullptr) {
      LogErrorApi("Failed to get program container");
      return false;
    }

    if ((programContainer->getDeviceImage(dev_, deviceImage_) != QS_SUCCESS) ||
        (!deviceImage_)) {
      LogErrorApi("Failed to get deviceImage");
      return false;
    }
  }

  nnConstants_ = deviceImage_->getConstants();
  if (nnConstants_ != nullptr) {
    LogInfoApi("Found built-in constants");
  }

  nnImage_ = deviceImage_->getNNImage();
  if (nnImage_ == nullptr) {
    return false;
  }
  return true;
}

bool QProgramDevice::activate_action() {
  QStatus status = QS_ERROR;

  // Defaults
  QActivationStateType activationState = ACTIVATION_STATE_CMD_READY;

  qnn_ = rt_->activateNetwork(nnImage_, nnConstants_, status, activationState,
                              program_->programProperties_.SubmitRetryTimeoutMs,
                              program_->programProperties_.SubmitNumRetries);
  if ((status != QS_SUCCESS) || (qnn_ == nullptr)) {
    return false;
  }
  return true;
}

bool QProgramDevice::unload_action() {
  QStatus status = QS_SUCCESS;
  // Release This instance of Device Image
  deviceImage_ = nullptr;
  nnImage_ = nullptr;
  if (status != QS_SUCCESS) {
    LogErrorApi("Failed to unload image");
    return false;
  }
  return true;
}

bool QProgramDevice::deactivate_action() {
  if (qnn_ != nullptr) {
    if (qnn_->deactivate() != QS_SUCCESS) {
      return false;
    }
    qnn_ = nullptr;
  }
  return true;
}

bool QProgramDevice::initialize() {
  bool rc = true;
  auto metadata = program_->getMetadata();
  if (metadata.networkName.empty()) {
    LogErrorApi("Error: Network name empty");
    return false;
  }
  numNsp_ = metadata.numNSPs;

  QHsm::init(1);
  QProgramDevice::Event e = QProgramDevice::Event(INIT_SIG);
  dispatch(&e, 1);

  return rc;
}

//=================================================================================================
// State Machine Processing Section
//=================================================================================================

void QProgramDevice::logEventStateName(int32_t sig, const char *state) {
  std::string eventName;
  switch (sig) {
  case Q_ENTRY_SIG:
    eventName = "Q_ENTRY_SIG";
    break;
  case Q_EXIT_SIG:
    eventName = "Q_EXIT_SIG";
    break;
  case Q_INIT_SIG:
    eventName = "Q_INIT_SIG";
    break;
  case INIT_SIG:
    eventName = "INIT_SIG";
    break;
  case INIT_ERROR_SIG:
    eventName = "INIT_ERROR_SIG";
    break;
  case LOAD_SIG:
    eventName = "LOAD_SIG";
    break;
  case UNLOAD_SIG:
    eventName = "UNLOAD_SIG";
    break;
  case ACTIVATE_SIG:
    eventName = "ACTIVATE_SIG";
    break;
  case DEACTIVATE_SIG:
    eventName = "DEACTIVATE_SIG";
    break;
  case LOAD_COMPLETE_SIG:
    eventName = "LOAD_COMPLETE_SIG";
    break;
  case LOAD_FAILED_SIG:
    eventName = "LOAD_FAILED_SIG";
    break;
  case UNLOAD_COMPLETE_SIG:
    eventName = "UNLOAD_COMPLETE_SIG";
    break;
  case ACTIVATE_COMPLETE_SIG:
    eventName = "ACTIVATE_COMPLETE_SIG";
    break;
  case ACTIVATE_FAILED_SIG:
    eventName = "ACTIVATE_FAILED_SIG";
    break;
  case DEACTIVATE_COMPLETE_SIG:
    eventName = "DEACTIVATE_COMPLETE_SIG";
    break;
  case DEVICE_UP_SIG:
    eventName = "DEVICE_UP_SIG";
    break;
  case DEVICE_DOWN_SIG:
    eventName = "DEVICE_DOWN_SIG";
    break;
  case VC_UP_SIG:
    eventName = "VC_UP_SIG";
    break;
  case VC_DOWN_SIG:
    eventName = "VC_DOWN_SIG";
    break;
  default:
    return;
  }
  LogDebugApi("HSM: {} -> {}", eventName, state);
}

//-------------------------------------------------------------------------------------------------
Q_STATE_DEF(QProgramDevice, initial) {

  (void)e; // unused parameter

  // no signal on initial state
  // only entry this stats once when init called on hsm
  return tran(Q_STATE_CAST(&created));
}

//-------------------------------------------------------------------------------------------------
// Created Super State
//-------------------------------------------------------------------------------------------------
Q_STATE_DEF(QProgramDevice, created_super) {

  logEventStateName(e->sig, __func__);
  switch (e->sig) {
  case Q_ENTRY_SIG:
    if (validate_action()) {
    } else {
      setHsmSignal(INIT_ERROR_SIG);
    }
    break;

  case INIT_ERROR_SIG:
    setHsmSignal(DEVICE_DOWN_SIG);
    break;

  case ACTIVATE_SIG:
    setHsmSignal(LOAD_SIG);
    setHsmSignal(ACTIVATE_SIG);
    break;

  case DEVICE_DOWN_SIG:
    // Move to error state
    return tran(Q_STATE_CAST(&device_error));

  case Q_EXIT_SIG:
    break;

  default:
    break;
  }
  return super(&top);
}
//-------------------------------------------------------------------------------------------------
Q_STATE_DEF(QProgramDevice, created) {
  logEventStateName(e->sig, __func__);
  switch (e->sig) {
  case Q_ENTRY_SIG:
    break;

  case LOAD_SIG:
    return tran(Q_STATE_CAST(&loading));
    break;

  case Q_EXIT_SIG:
    break;

  default:
    break;
  }
  return super(&created_super);
}

//-------------------------------------------------------------------------------------------------
Q_STATE_DEF(QProgramDevice, loading) {
  logEventStateName(e->sig, __func__);
  switch (e->sig) {
  case Q_ENTRY_SIG:
    if (load_action() == true) {
      setHsmSignal(LOAD_COMPLETE_SIG);
    } else {
      setHsmSignal(LOAD_FAILED_SIG);
    }
    break;
  case LOAD_FAILED_SIG:
    return tran(Q_STATE_CAST(&load_failed));
    break;

  case LOAD_COMPLETE_SIG:
    return tran(Q_STATE_CAST(&loaded));
    break;

  case Q_EXIT_SIG:
    break;

  default:
    break;
  }
  return super(&created_super);
}

//-------------------------------------------------------------------------------------------------
// Loaded Super State
//-------------------------------------------------------------------------------------------------
Q_STATE_DEF(QProgramDevice, loaded_super) {
  logEventStateName(e->sig, __func__);
  switch (e->sig) {
  case Q_ENTRY_SIG:
    break;

  case LOAD_SIG:
    // Send Error Report
    break;

  case VC_DOWN_SIG:
    break;

  case Q_EXIT_SIG:
    break;

  default:
    break;
  }
  return super(&created_super);
}

//-------------------------------------------------------------------------------------------------
Q_STATE_DEF(QProgramDevice, unloading) {
  logEventStateName(e->sig, __func__);
  switch (e->sig) {
  case Q_ENTRY_SIG:
    unload_action();
    setHsmSignal(UNLOAD_COMPLETE_SIG);
    break;

  case UNLOAD_COMPLETE_SIG:
    return tran(Q_STATE_CAST(&created));
    break;

  case Q_EXIT_SIG:
    break;

  default:
    break;
  }
  return super(&created_super);
}

//-------------------------------------------------------------------------------------------------
Q_STATE_DEF(QProgramDevice, loaded) {
  logEventStateName(e->sig, __func__);
  switch (e->sig) {
  case Q_ENTRY_SIG:
    break;
  case ACTIVATE_SIG:
    return tran(Q_STATE_CAST(&activating));

  case UNLOAD_SIG:
    return tran(Q_STATE_CAST(&unloading));

  case Q_EXIT_SIG:
    break;

  default:
    break;
  }
  return super(&loaded_super);
}

//-------------------------------------------------------------------------------------------------
Q_STATE_DEF(QProgramDevice, activating) {
  logEventStateName(e->sig, __func__);
  switch (e->sig) {
  case Q_ENTRY_SIG:
    if (activate_action()) {
      setHsmSignal(ACTIVATE_COMPLETE_SIG);
    } else {
      setHsmSignal(ACTIVATE_FAILED_SIG);
    }
    break;
  case ACTIVATE_FAILED_SIG:
    return tran(Q_STATE_CAST(&activate_failed));
    break;

  case ACTIVATE_COMPLETE_SIG:
    return tran(Q_STATE_CAST(&ac_ready));
    break;

  default:
    break;
  }
  return super(&loaded_super);
}

Q_STATE_DEF(QProgramDevice, activate_failed) {
  logEventStateName(e->sig, __func__);
  switch (e->sig) {
  case Q_ENTRY_SIG:
    break;

  case ACTIVATE_SIG:
    return tran(Q_STATE_CAST(&activating));

  case Q_EXIT_SIG:
    break;

  default:
    break;
  }
  return super(&loaded_super);
}

//-------------------------------------------------------------------------------------------------
Q_STATE_DEF(QProgramDevice, disabled) {
  logEventStateName(e->sig, __func__);
  switch (e->sig) {
  case Q_ENTRY_SIG:
    break;

  default:
    break;
  }
  return super(&loaded_super);
}

//-------------------------------------------------------------------------------------------------
// Active Super State
//-------------------------------------------------------------------------------------------------
Q_STATE_DEF(QProgramDevice, active_super) {
  logEventStateName(e->sig, __func__);
  switch (e->sig) {
  case Q_ENTRY_SIG:
    break;

  case DEACTIVATE_SIG:
    return tran(Q_STATE_CAST(&ac_deactivating));
    break;

  case Q_EXIT_SIG:
    break;

  default:
    break;
  }
  return super(&loaded_super);
}

//-------------------------------------------------------------------------------------------------
Q_STATE_DEF(QProgramDevice, ac_ready) {
  logEventStateName(e->sig, __func__);
  switch (e->sig) {
  case Q_ENTRY_SIG:
    break;

  case Q_EXIT_SIG:
    break;

  default:
    break;
  }
  return super(&active_super);
}

//-------------------------------------------------------------------------------------------------
Q_STATE_DEF(QProgramDevice, ac_deactivating) {
  logEventStateName(e->sig, __func__);
  switch (e->sig) {
  case Q_ENTRY_SIG:
    if (deactivate_action()) {
      setHsmSignal(DEACTIVATE_COMPLETE_SIG);
    } else {
      setHsmSignal(DEVICE_DOWN_SIG);
    }
    break;

  case DEACTIVATE_COMPLETE_SIG:
    return tran(Q_STATE_CAST(&loaded));
    break;

  case Q_EXIT_SIG:
    break;

  default:
    break;
  }
  return super(&active_super);
}

//-------------------------------------------------------------------------------------------------
Q_STATE_DEF(QProgramDevice, error_super) {
  logEventStateName(e->sig, __func__);
  switch (e->sig) {
  case Q_ENTRY_SIG:
    break;

  case Q_EXIT_SIG:
    break;

  default:
    break;
  }
  return super(&top);
}

//-------------------------------------------------------------------------------------------------
Q_STATE_DEF(QProgramDevice, device_error) {
  logEventStateName(e->sig, __func__);
  switch (e->sig) {
  case Q_ENTRY_SIG:
    deactivate_action();
    devInfoValidated_ = false;
    break;

  case DEVICE_UP_SIG:
    if (validate_action()) {
      return tran(Q_STATE_CAST(&created));
    }
    break;

  case Q_EXIT_SIG:
    break;

  default:
    LogInfoApiS(this, "Device:{} is in removed state", dev_);
    break;
  }

  return super(&error_super);
}

//-------------------------------------------------------------------------------------------------
// In this error state, the objects are still loaded
// Exit when we receive a LOAD sig directly to load
// Or process unload when we receive UNLOAD_SIG

//-------------------------------------------------------------------------------------------------
Q_STATE_DEF(QProgramDevice, load_failed) {
  logEventStateName(e->sig, __func__);
  switch (e->sig) {
  case Q_ENTRY_SIG:
    break;

  case LOAD_SIG:
    return tran(Q_STATE_CAST(&loading));
    break;

  case Q_EXIT_SIG:
    break;

  default:
    break;
  }
  return super(&error_super);
}

void QProgramDevice::run() {
  //---------------------------------------------------------------------
  // Process Signals
  //---------------------------------------------------------------------
  while (!signals_.empty()) {
    signalQueueuMutex_.lock();
    Signals sig = signals_.front();
    Event ev(sig);
    signals_.pop();
    signalQueueuMutex_.unlock();
    QHsm::dispatch(&ev, 1);
  }
}

// For internal Actions only
void QProgramDevice::setHsmSignal(Signals sig) { signals_.push(sig); }

// For External Actions Only
void QProgramDevice::setHsmSignalWithLock(const Signals sig) {
  // Set the signal in HSM
  std::unique_lock<std::mutex> lock(signalQueueuMutex_);
  signals_.push(sig);
  // Mark this workqueue element for processing
  // This will result in the run() method being called which will
  // process the HSM
  signalQueueuMutex_.unlock();
}

} // namespace qaic
