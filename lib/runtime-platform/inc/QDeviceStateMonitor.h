// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QDEVICESTATEMONITOR_H
#define QDEVICESTATEMONITOR_H

#include <future>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <thread>
#include <atomic>

#include "QAicRuntimeTypes.h"
#include "QMonitorDeviceObserver.h"
#include "QWorkqueueElement.h"
#include "QDeviceStateInfo.h"
#include "QLogger.h"

namespace qaic {

class QDeviceStateMonitor;
enum subSystemRestartWaitThreadStatus {
  THREAD_RUNNING,
  THREAD_FINISHED,
};
using QDeviceStateMonitorShared = std::shared_ptr<QDeviceStateMonitor>;

class QDeviceStateMonitor : public QLogger, public QWorkqueueElement {
public:
  QDeviceStateMonitor();
  using deviceStateInfoTimePair =
      std::pair<QDeviceStateInfoEvent,
                std::chrono::time_point<std::chrono::system_clock>>;
  QStatus init();
  QStatus registerDeviceObserver(QMonitorDeviceObserver *observer);
  void unregisterDeviceObserver(QMonitorDeviceObserver *observer);
  // The following function is for registry only. Eventually this class should
  // be a part of the device registry
  QStatus registerPriorityDeviceObserver(QMonitorDeviceObserver *observer);
  // from QWorkqueueElementInterface
  virtual void run() override;
  virtual ~QDeviceStateMonitor();
  static bool isDerivedDevice(const std::string &udevSysPath);

private:
  void monitorQaicDevices(std::promise<QStatus> readyPromise);
  QStatus refreshQidMap();
  void waitForSubSystemRestart(std::pair<QID, uint32_t>, std::string);
  void processDeviceStateInfo(std::shared_ptr<QDeviceStateInfo>);
  void processEvents(std::shared_ptr<QDeviceStateInfo>);

  std::atomic_bool stopDevicePolling_;
  std::atomic_bool stopWaitForSubSystemRestart_;
  std::thread devMonThread_;
  std::list<QMonitorDeviceObserver *> deviceObservers_;
  std::queue<std::shared_ptr<QDeviceStateInfo>> stateEventsQueue_;
  std::map<std::string, QID> qidMap_;
  std::mutex deviceObserversMtx_;
  std::mutex stateEventsQueueMtx_;
  static const std::set<std::string> udevPropertyWhitelist_;
  std::map<std::thread::id,
           std::pair<std::thread, subSystemRestartWaitThreadStatus>>
      threadMap_;
  std::mutex threadMapMutex_;
  std::condition_variable threadMapCv_;
  std::map<std::pair<QID, uint32_t>, deviceStateInfoTimePair> vcStatusMap_;
  std::mutex vcStatusMapMutex_;

  friend void sendDeviceStateMonitorEvent(std::shared_ptr<QDeviceStateInfo>);
};

class QDeviceStateMonitorManager {
public:
  static QDeviceStateMonitorShared &getDeviceStateMonitor();
  static QDeviceStateMonitorShared deviceStateMonitor_;

private:
  static std::mutex m_;
};
} // namespace qaic

#endif // QDEVICESTATEMONITOR_H