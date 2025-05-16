// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include <typeinfo>
#include <atomic>

#include "QAicRuntimeTypes.h"
#include "QAic.h"
#include "QUtil.h"
#include "QAicRuntimeTypes.h"
#include "QContext.h"
#include "QProgram.h"
#include "QWorkthread.h"

namespace qaic {

static std::atomic<QAicObjId> NextUniqueObjId{0};

std::mutex QContext::programs_lock_;
std::mutex QContext::constants_lock_;
std::mutex QContext::queues_lock_;
std::mutex QContext::events_lock_;
std::mutex QContext::execobjs_lock_;

static std::atomic<QAicContextID> globalContextID{0};
std::mutex globalContextID_lock;

shQContext QContext::createContext(const QAicContextProperties *properties,
                                   uint32_t numDevices, const QID *devList,
                                   QStatus &status) {
  // globalContextID is used in constructor. Access needs to be protected
  std::unique_lock<std::mutex> lk(globalContextID_lock);
  std::shared_ptr<QContext> context =
      std::make_shared<QContext>(properties, numDevices, devList);
  if (context) {
    status = context->init();
  } else {
    status = QS_NOMEM;
  }

  if (status != QS_SUCCESS) {
    return nullptr;
  }
  return context;
}

QContext::QContext(const QAicContextProperties *properties, uint32_t numDevices,
                   const QID *devList)
    : QContextInterface("Aic-" + std::to_string(globalContextID)),
      QComponent("Context", NextUniqueObjId++, this),
      QWorkqueueElement(QContext_WORKQUEUE), rt_(nullptr),
      contextId_(globalContextID++) {

  QLogLevel level = QL_DEBUG;
  QLogControl::getLogControl()->getLogLevel(level);
  rt_ = getRuntime();

  for (uint32_t i = 0; i < numDevices; i++) {
    qids_.push_back(devList[i]);
  }
  if (properties != nullptr) {
    QStatus status = QS_ERROR;
    status = validateContextProperty(properties);
    if (status != QS_SUCCESS) {
      LogError("Invalid Context Property");
    }
  }
}

QContext::~QContext() { unregisterDeviceObserver(); }

QStatus QContext::init() {
  for (auto &dev : qids_) {
    if (!rt_->isValidDevice(dev)) {
      LogError("Invalid Device {}", dev);
      return QS_INVAL;
    }
  }
  return QS_SUCCESS;
}

QStatus QContext::getContextId(QAicContextID &id) {
  id = contextId_;
  return QS_SUCCESS;
}

QStatus QContext::setLogLevel(QLogLevel logLevel) {
  QLogControl::getLogControl()->setLogLevel(
      getContextName(), QLogControl::getSpdLogLevel(logLevel));
  return QS_SUCCESS;
}

QStatus QContext::getLogLevel(QLogLevel &logLevel) {
  QLogControl::getLogControl()->getLogLevel(getContextName(), logLevel);
  return QS_SUCCESS;
}

void QContext::logMessage(QLogLevel logLevel, const char *message) {

  if (message == nullptr) {
    return;
  }

  switch (logLevel) {
  case QL_INFO:
    LogInfo(message);
    break;
  case QL_WARN:
    LogWarn(message);
    break;
  case QL_ERROR:
    LogError(message);
    break;
  case QL_DEBUG:
  default:
    LogDebug(message);
    break;
  }
}

QStatus QContext::registerProgram(shQProgram &shProgram) {
  std::unique_lock<std::mutex> lk(programs_lock_);
  programs_.push_back(shProgram);
  return QS_SUCCESS;
}

QStatus QContext::registerQueue(shQIQueue &shQueue) {
  std::unique_lock<std::mutex> lk(queues_lock_);
  queues_.push_back(shQueue);
  return QS_SUCCESS;
}

QStatus QContext::registerEvent(shQIEvent &shEvent) {
  std::unique_lock<std::mutex> lk(events_lock_);
  events_.push_back(shEvent);
  return QS_SUCCESS;
}

QStatus QContext::registerExecObj(shQExecObj &shExecObj) {
  std::unique_lock<std::mutex> lk(execobjs_lock_);
  execobjs_.push_back(shExecObj);
  return QS_SUCCESS;
}

QStatus QContext::unRegisterProgram(QProgram *program) {
  std::unique_lock<std::mutex> lk(programs_lock_);
  auto shProgramToDelete = std::find_if(programs_.begin(), programs_.end(),
                                        [&program](const shQProgram &element) {
                                          return program == element.get();
                                        });
  if (shProgramToDelete != programs_.end()) {
    programs_.erase(shProgramToDelete);
    return QS_SUCCESS;
  }
  return QS_INVAL;
}

QStatus QContext::unRegisterProgram(const shQProgram &shProgram) {
  std::unique_lock<std::mutex> lk(programs_lock_);
  auto shProgramToDelete =
      std::find(programs_.begin(), programs_.end(), shProgram);
  if (shProgramToDelete != programs_.end()) {
    programs_.erase(shProgramToDelete);
    return QS_SUCCESS;
  }
  return QS_INVAL;
}

QStatus QContext::unRegisterQueue(QIQueue *queue) {
  std::unique_lock<std::mutex> lk(queues_lock_);
  auto shQueueToDelete = std::find_if(
      queues_.begin(), queues_.end(),
      [&queue](const shQIQueue &element) { return queue == element.get(); });
  if (shQueueToDelete != queues_.end()) {
    queues_.erase(shQueueToDelete);
    return QS_SUCCESS;
  }
  return QS_INVAL;
}

QStatus QContext::unRegisterQueue(const shQIQueue &shQueue) {
  std::unique_lock<std::mutex> lk(queues_lock_);
  auto shQueueToDelete = std::find(queues_.begin(), queues_.end(), shQueue);
  if (shQueueToDelete != queues_.end()) {
    queues_.erase(shQueueToDelete);
    return QS_SUCCESS;
  }
  return QS_INVAL;
}

QStatus QContext::unRegisterEvent(QIEvent *event) {
  std::unique_lock<std::mutex> lk(events_lock_);
  auto shEventToDelete = std::find_if(
      events_.begin(), events_.end(),
      [&event](const shQIEvent &element) { return event == element.get(); });
  if (shEventToDelete != events_.end()) {
    events_.erase(shEventToDelete);
    return QS_SUCCESS;
  }
  return QS_INVAL;
}

QStatus QContext::unRegisterEvent(const shQIEvent &shEvent) {
  std::unique_lock<std::mutex> lk(events_lock_);
  auto shEventToDelete = std::find(events_.begin(), events_.end(), shEvent);
  if (shEventToDelete != events_.end()) {
    events_.erase(shEventToDelete);
    return QS_SUCCESS;
  }
  return QS_INVAL;
}

QStatus QContext::unRegisterExecObj(QExecObj *execobj) {
  std::unique_lock<std::mutex> lk(execobjs_lock_);
  auto shExecObjToDelete = std::find_if(execobjs_.begin(), execobjs_.end(),
                                        [&execobj](const shQExecObj &element) {
                                          return execobj == element.get();
                                        });
  if (shExecObjToDelete != execobjs_.end()) {
    execobjs_.erase(shExecObjToDelete);
    return QS_SUCCESS;
  }
  return QS_INVAL;
}

QStatus QContext::unRegisterExecObj(const shQExecObj &shExecObj) {
  std::unique_lock<std::mutex> lk(execobjs_lock_);
  auto shExecObjToDelete =
      std::find(execobjs_.begin(), execobjs_.end(), shExecObj);
  if (shExecObjToDelete != execobjs_.end()) {
    execobjs_.erase(shExecObjToDelete);
    return QS_SUCCESS;
  }
  return QS_INVAL;
}

QStatus QContext::notifyDeviceEvent(std::shared_ptr<QDeviceStateInfo> event) {
  deviceStateInfoQueueMutex_.lock();
  deviceStateInfoQueue_.push(std::move(event));
  deviceStateInfoQueueMutex_.unlock();
  notifyReady();
  return QS_SUCCESS;
}

void QContext::run() {

  deviceStateInfoQueueMutex_.lock();
  while (!deviceStateInfoQueue_.empty()) {
    auto event = deviceStateInfoQueue_.front();
    deviceStateInfoQueueMutex_.unlock();

    /* Priority based Sorted list. Highest priority client is notified first */
    notifyDeviceStateInfoPriorityDbMutex_.lock();
    auto it = notifyDeviceStateInfoPriorityDb_.cbegin();
    for (; it != notifyDeviceStateInfoPriorityDb_.cend(); ++it) {
      it->second.second(event);
    }
    notifyDeviceStateInfoPriorityDbMutex_.unlock();

    deviceStateInfoQueueMutex_.lock();
    deviceStateInfoQueue_.pop();
  }
  deviceStateInfoQueueMutex_.unlock();
}

QStatus
QContext::registerNotifyDeviceStateInfo(NotifyDeviceStateInfoPriority prio,
                                        void *clientObj,
                                        NotifyDeviceStateInfoFuncCb cbFunc) {
  std::unique_lock<std::mutex> lk(notifyDeviceStateInfoPriorityDbMutex_);
  auto it = notifyDeviceStateInfoPriorityDb_.begin();
  for (; it != notifyDeviceStateInfoPriorityDb_.end(); ++it) {
    if (prio >= it->first) {
      break;
    }
  }
  /* Highest priority clientObj is inserted at the beginning of the list */
  notifyDeviceStateInfoPriorityDb_.insert(
      it, std::make_pair(prio, std::make_pair(clientObj, cbFunc)));
  return QS_SUCCESS;
}

QStatus QContext::unRegisterNotifyDeviceStateInfo(void *clientObj) {
  std::unique_lock<std::mutex> lk(notifyDeviceStateInfoPriorityDbMutex_);
  auto it = notifyDeviceStateInfoPriorityDb_.begin();
  for (; it != notifyDeviceStateInfoPriorityDb_.end(); ++it) {
    if (it->second.first == clientObj) {
      it = notifyDeviceStateInfoPriorityDb_.erase(it);
    }
  }
  return QS_SUCCESS;
}

QStatus
QContext::validateContextProperty(const QAicContextProperties *properties) {
  uint32_t invalidContextBitfields = ~(static_cast<uint32_t>(
      QAicContextPropertiesBitfields::QAIC_CONTEXT_DEFAULT));
  if (((*properties) & invalidContextBitfields) != 0) {
    return QS_INVAL;
  }
  return QS_SUCCESS;
}

} // namespace qaic
