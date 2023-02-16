// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QPROGRAM_DEVICE_H
#define QPROGRAM_DEVICE_H

#include <queue>

#include "QAicRuntimeTypes.h"
#include "QAic.h"
#include "QProgramTypes.h"
#include "QLog.h"
#include "QContext.h"
#include "QKmdRuntime.h"
#include "QRuntimeManager.h"
#include "QBindingsParser.h"
#include "AICNetworkDesc.pb.h"
#include "QComponent.h"
#include "QNeuralNetworkInterface.h"
#include "QProgramInfo.h"
#include "QMonitorDeviceObserver.h"
#include "QProgramContainer.h"

#include <qpcpp.h>

namespace qaic {

class QExecObj;
class QProgram;
class QProgramDevice;

using shQProgram = std::shared_ptr<QProgram>;
using uQProgramDevice = std::unique_ptr<QProgramDevice>;
using shQProgram = std::shared_ptr<QProgram>;
using shQExecObj = std::shared_ptr<QExecObj>;

class QProgramDevice : private QP::QHsm,
                       public QComponent,
                       public QIAicApiContext {
  friend class QExecObj;
  friend class QProgramGroup;
  friend class QCoordinator;

public:
  static uQProgramDevice Factory(shQContext context, QProgram *program,
                                 QID dev);
  QProgramDevice(shQContext context, QProgram *program, QID dev);
  virtual ~QProgramDevice();

  // From API
  QStatus load();
  QStatus unload();
  QStatus processActivateCmd(QProgramActivationCmd cmd);

  // State Info Functions
  bool isActive();      // Program has an Activation ID, either standby or ready
  bool isReady();       // Program has an Activation ID, and is fully activated
  bool isDeviceReady(); // Device is ready for operation, not in error
  bool isInError();
  bool isLoaded();

  QStatus getInferenceCompletedCount(uint64_t &count);
  QStatus getDeviceQueueLevel(uint32_t &fillLevel, uint32_t &queueSize);
  QNeuralNetworkInterface *nn();
  void getProgramInfo(QAicProgramInfo &info);
  QProgramDevice *getProgramDevice(QID qid);
  QStatus registerExecObj(const shQExecObj &execObj);
  QStatus unregisterExecObj(const shQExecObj &execObj);
  QStatus unregisterExecObj(const QExecObj *execObj);

  uint32_t getNumNsp() const { return numNsp_; }

protected:
  QStatus readyProgram();

private:
  enum Signals {
    NULL_SIG = QP::Q_USER_SIG,
    INIT_SIG,                // When created
    INIT_ERROR_SIG,          // From internal api
    LOAD_SIG,                // From external API event LOAD
    UNLOAD_SIG,              // From external API event UNLOAD
    ACTIVATE_SIG,            // From external API event ACTIVATE
    DEACTIVATE_SIG,          // From external API event DEACTIVATE
    LOAD_COMPLETE_SIG,       // From return status of load operation
    LOAD_FAILED_SIG,         // From return status of load operation
    UNLOAD_COMPLETE_SIG,     // From return status of unload operation
    ACTIVATE_COMPLETE_SIG,   // From return status of activate operation
    ACTIVATE_FAILED_SIG,     // From return status of activate operation
    DEACTIVATE_COMPLETE_SIG, // From return status of deactivate operation
    DEVICE_UP_SIG,           // From registered device observer
    DEVICE_DOWN_SIG,         // From registered device observer
    VC_UP_SIG,               // From registered device observer
    VC_DOWN_SIG,             // From registered device observer
  };

  using State = QP::QState;
  struct Event : QP::QEvt {
    Event(Signals s) : QP::QEvt({(QP::QSignal)s, 0, 0}) {}
  };

  // Private support functions
  std::string str(ProgramState);
  ProgramState getProgramState();
  QStatus notifyDeviceStateInfo(std::shared_ptr<QDeviceStateInfo> event);

  bool initialize();
  QStatus activate();
  QStatus deactivate();
  QStatus enable();
  QStatus disable();
  QStatus standby_request();
  QStatus ready_request();
  QStatus parseIoDescriptor();
  uint32_t calcIoSize(const aicnwdesc::IODescriptor &desc);

  // Support for binding elements
  template <typename T> uint32_t writeBindingElement(const T *src, int elts);
  uint32_t writeBinding(const aicnwdesc::IODescriptor &protoDesc, uint8_t *to,
                        uint32_t maxSize);
  const std::string strBufferMappings(const QAicBufferMappings *mapping) const;

  // HSM Actions
  void setQNAID();
  bool validate_action();
  bool load_action();
  bool unload_action();
  bool activate_action();
  bool deactivate_action();
  void handleLoadError();
  void handleActivateError();
  void run();
  void setHsmSignal(Signals sig);
  void setHsmSignalWithLock(const Signals sig);

  void logEventStateName(int32_t sig, const char *state);

  // HSM States
  Q_STATE_DECL(initial);
  // Created Super
  Q_STATE_DECL(created_super);
  Q_STATE_DECL(created);
  Q_STATE_DECL(loading);
  Q_STATE_DECL(load_failed);
  Q_STATE_DECL(unloading);

  // Loaded Super
  Q_STATE_DECL(loaded_super);
  Q_STATE_DECL(loaded);
  Q_STATE_DECL(disabled);
  Q_STATE_DECL(activating);
  Q_STATE_DECL(activate_failed);

  // Active Super
  Q_STATE_DECL(active_super);
  Q_STATE_DECL(ac_ready);
  Q_STATE_DECL(ac_deactivating);

  // Error Super
  Q_STATE_DECL(error_super);
  Q_STATE_DECL(device_error);

  QID dev_;
  QNAID qnaid_;
  QVCID vcid_;
  shQDeviceImage deviceImage_; // Loaded images on device
  QProgram *program_;
  QNNImageInterface *nnImage_;         // Loaded Image
  QNNConstantsInterface *nnConstants_; // Loaded Constants
  QNeuralNetworkInterface *qnn_;       // Activated Network
  QRuntimeInterface *rt_;
  std::mutex programMutex_;
  std::queue<Signals> signals_;
  std::mutex signalQueueuMutex_;
  QAicProgramID programId_;
  QDevInfo devInfo_;
  bool devInfoValidated_;
  std::vector<shQExecObj> execObjs_;
  std::mutex execObjsLock_;
  bool autoLoadActivate_;
  NotifyDeviceStateInfoFuncCb deviceStateInfoFuncCb_ =
      [this](std::shared_ptr<QDeviceStateInfo> event) -> void {
    notifyDeviceStateInfo(event);
  };
  uint32_t numNsp_;
};

} // namespace qaic

#endif // QProgramDEVICEH
