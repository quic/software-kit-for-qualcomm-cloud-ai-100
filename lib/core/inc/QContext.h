// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
#ifndef QCONTEXT_H
#define QCONTEXT_H

#include <map>
#include <memory>
#include <list>
#include <queue>

#include "QAicRuntimeTypes.h"
#include "QAic.h"
#include "QLogInterface.h"
#include "QLog.h"
#include "QUtil.h"
#include "QKmdRuntime.h"
#include "QRuntimeManager.h"
#include "QComponent.h"
#include "QMonitorDeviceObserver.h"
#include "QWorkqueueElement.h"
#include "QContext.h"

namespace qaic {

// Every AicApi Component should inherit from QIAicAPiContext
class QIAicApiContext {
public:
  QIAicApiContext(shQContext &context) : context_(context) {}
  const shQContext &context() const noexcept { return context_; }

protected:
  const shQContext context_;
};

class QContextInterface {
public:
  QContextInterface(std::string name) : name_(name){};
  virtual const std::string &getContextName() const noexcept { return name_; };
  virtual ~QContextInterface() = default;

protected:
  std::string name_;
};

class QContext : public QContextInterface,
                 public QComponent,
                 public QWorkqueueElement,
                 private QMonitorDeviceObserver {

public:
  static std::shared_ptr<QContext>
  createContext(const QAicContextProperties *properties, uint32_t numDevices,
                const QID *devList, QStatus &status);

  QContext(const QAicContextProperties *properties, uint32_t numDevices,
           const QID *devList);
  virtual ~QContext();
  QStatus init();
  QStatus registerProgram(shQProgram &program);
  QStatus registerQueue(shQIQueue &queue);
  QStatus registerEvent(shQIEvent &event);
  QStatus registerExecObj(shQExecObj &execobj);
  QStatus unRegisterProgram(QProgram *program);
  QStatus unRegisterProgram(const shQProgram &shProgram);
  QStatus unRegisterQueue(QIQueue *queue);
  QStatus unRegisterQueue(const shQIQueue &shQueue);
  QStatus unRegisterEvent(const shQIEvent &shEvent);
  QStatus unRegisterEvent(QIEvent *event);
  QStatus unRegisterExecObj(const shQExecObj &shExecObj);
  QStatus unRegisterExecObj(QExecObj *execobj);
  /* Objects with highest priority is notified first */
  QStatus registerNotifyDeviceStateInfo(NotifyDeviceStateInfoPriority prio,
                                        void *clientObj,
                                        NotifyDeviceStateInfoFuncCb cbFunc);
  QStatus unRegisterNotifyDeviceStateInfo(void *clientObj);
  QStatus setLogLevel(QLogLevel logLevel);
  QStatus getLogLevel(QLogLevel &logLevel);
  void logMessage(QLogLevel logLevel, const char *message);
  QStatus getContextId(QAicContextID &id);
  QRuntimeInterface *rt() { return rt_; }
  static QStatus
  validateContextProperty(const QAicContextProperties *properties);
  const std::vector<shQProgram> &getPrograms() const { return programs_; }

  // from QMonitorDeviceObserver interface
  QStatus notifyDeviceEvent(std::shared_ptr<QDeviceStateInfo> event) override;
  // from QWorkqueueElementInterface
  virtual void run() override;

private:
  QRuntimeInterface *rt_;
  std::vector<QID> qids_;
  std::vector<shQIEvent> events_;
  std::vector<shQProgram> programs_;
  std::vector<shQIQueue> queues_;
  std::vector<shQExecObj> execobjs_;
  static std::mutex programs_lock_;
  static std::mutex constants_lock_;
  static std::mutex queues_lock_;
  static std::mutex events_lock_;
  static std::mutex execobjs_lock_;
  QAicContextID contextId_;
  std::mutex notifyDeviceStateInfoPriorityDbMutex_;
  std::list<NotifyDeviceStateInfoPriorityDbEntry>
      notifyDeviceStateInfoPriorityDb_;
  std::mutex deviceStateInfoQueueMutex_;
  std::queue<std::shared_ptr<QDeviceStateInfo>> deviceStateInfoQueue_;
};

} // namespace qaic

#endif // QCONTEXT_HQ
