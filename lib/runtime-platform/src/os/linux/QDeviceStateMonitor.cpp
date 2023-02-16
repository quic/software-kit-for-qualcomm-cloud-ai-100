// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QDeviceStateMonitor.h"
#include "QRuntimePlatformApi.h"
#include "QUtil.h"
#include "QOsal.h"

#include <libudev.h>

namespace qaic {
static const std::string UDEV_NAMESPACE = "kernel";
static const std::string UDEV_DEVICE_FILTER = "Qualcomm Cloud AI 100";
static const std::string UDEV_DEVICE_ADD_STRING = "add";
static const std::string UDEV_DEVICE_REMOVE_STRING = "remove";
static const std::string UDEV_DEVICE_CHANGE_STRING = "change";
static const int32_t UDEV_DBC_STATE_BEFORE_SHUTDOWN = 2; /* VC Down */
static const int32_t UDEV_DBC_STATE_AFTER_POWER_UP = 5;  /* VC Up */

QDeviceStateMonitorShared QDeviceStateMonitorManager::deviceStateMonitor_;
std::mutex QDeviceStateMonitorManager::m_;
//======================================================================
// DeviceMonitor
//======================================================================

QDeviceStateMonitorShared &QDeviceStateMonitorManager::getDeviceStateMonitor() {
  std::unique_lock<std::mutex> lk(m_);
  if (QDeviceStateMonitorManager::deviceStateMonitor_ == nullptr) {
    QDeviceStateMonitorShared deviceStateMonitorTmp =
        std::make_unique<QDeviceStateMonitor>();

    if (deviceStateMonitorTmp->init() == QS_SUCCESS) {
      QDeviceStateMonitorManager::deviceStateMonitor_ =
          std::move(deviceStateMonitorTmp);
    }
  }
  return QDeviceStateMonitorManager::deviceStateMonitor_;
}

const std::set<std::string> QDeviceStateMonitor::udevPropertyWhitelist_ = {
    "DBC_ID", "DBC_STATE"};

QDeviceStateMonitor::QDeviceStateMonitor()
    : QLogger("QDeviceStateMonitor"),
      QWorkqueueElement(DEVICE_STATE_WORKQUEUE) {}

QStatus QDeviceStateMonitor::init() {
  QStatus retStatus = QS_ERROR;

  // Initialize map
  if (QS_SUCCESS != refreshQidMap()) {
    LogError("Failed to build devicemap \n");
    return retStatus;
  }

  // Start the monitor thread
  std::promise<QStatus> readyPromise;
  std::future<QStatus> readyFuture = readyPromise.get_future();

  stopDevicePolling_.store(false, std::memory_order_relaxed);

  stopWaitForSubSystemRestart_.store(false, std::memory_order_relaxed);

  devMonThread_ = std::thread(&QDeviceStateMonitor::monitorQaicDevices, this,
                              std::move(readyPromise));

  retStatus = readyFuture.get();

  if (retStatus != QS_SUCCESS) {
    devMonThread_.join();
  }
  return retStatus;
}

QStatus QDeviceStateMonitor::registerPriorityDeviceObserver(
    QMonitorDeviceObserver *observer) {
  std::unique_lock<std::mutex> lk(deviceObserversMtx_);
  deviceObservers_.push_front(observer);
  LogInfo("Registered priority device observer \n");
  return QS_SUCCESS;
}

QStatus
QDeviceStateMonitor::registerDeviceObserver(QMonitorDeviceObserver *observer) {
  std::unique_lock<std::mutex> lk(deviceObserversMtx_);
  deviceObservers_.push_back(observer);
  LogInfo("Registered device observer \n");
  return QS_SUCCESS;
}

void QDeviceStateMonitor::unregisterDeviceObserver(
    QMonitorDeviceObserver *observer) {
  auto obsCompare = [&](QMonitorDeviceObserver *o) { return o == observer; };
  std::unique_lock<std::mutex> lk(deviceObserversMtx_);
  deviceObservers_.remove_if(obsCompare);
}

void QDeviceStateMonitor::monitorQaicDevices(
    std::promise<QStatus> readyPromise) {
  struct udev *uDev = nullptr;
  struct udev_monitor *uDevMonitor = nullptr;
  struct udev_device *uDevDevice = nullptr;

  int monitorFd = -1;

  uDev = udev_new();
  if (!uDev) {
    LogError("Failed to create udev");
    readyPromise.set_value(QS_ERROR);
    return;
  }

  uDevMonitor = udev_monitor_new_from_netlink(uDev, UDEV_NAMESPACE.c_str());
  if (uDevMonitor == NULL) {
    LogError("Failed to create udev monitor");
    readyPromise.set_value(QS_ERROR);
    goto cleanup;
  }

  // Add filter for qaic devices
  if (udev_monitor_filter_add_match_subsystem_devtype(
          uDevMonitor, UDEV_DEVICE_FILTER.c_str(), NULL) < 0) {
    LogError("Failed to set filter");
    readyPromise.set_value(QS_ERROR);
    goto cleanup;
  }
  if (udev_monitor_enable_receiving(uDevMonitor) < 0) {
    LogError("Failed to enable monitor receiving");
    readyPromise.set_value(QS_ERROR);
    goto cleanup;
  }
  monitorFd = udev_monitor_get_fd(uDevMonitor);

  if (monitorFd < 0) {
    LogError("Failed to get fd");
    readyPromise.set_value(QS_ERROR);
    goto cleanup;
  }

  // From this point on the thread will be running.
  readyPromise.set_value(QS_SUCCESS);
  while (1) {
    fd_set fdSet;
    struct timeval timeVal;
    int retVal;

    FD_ZERO(&fdSet);
    FD_SET(monitorFd, &fdSet);
    timeVal.tv_sec = 1;
    timeVal.tv_usec = 0;

    retVal = select(monitorFd + 1, &fdSet, NULL, NULL, &timeVal);
    if (stopDevicePolling_.load(std::memory_order_relaxed)) {
      // Need to exit the thread
      break;
    }

    if (retVal > 0 && FD_ISSET(monitorFd, &fdSet)) {
      LogDebug("Select return {}", retVal);
      uDevDevice = udev_monitor_receive_device(uDevMonitor);
      if (uDevDevice) {
        std::string action = udev_device_get_action(uDevDevice);

        std::string udevSysPath = udev_device_get_syspath(uDevDevice);
        std::size_t udevSysNameOffset = udevSysPath.find(UDEV_DEVICE_FILTER);
        std::string pcieParentSysPath(udevSysPath, 0, udevSysNameOffset - 1);
        auto found = pcieParentSysPath.find_last_of("/");
        std::string sbdf(pcieParentSysPath, found + 1,
                         (pcieParentSysPath.length() - found - 1));

        if (isDerivedDevice(udevSysPath)) {
          LogInfo("Skip notification for derived: {}", udevSysPath);
          continue;
        }

        struct udev_list_entry *properties =
            udev_device_get_properties_list_entry(uDevDevice);
        struct udev_list_entry *property = nullptr;

        // if the entry exists queue the notification, if the entry is not found
        // print an error message for the user to restart the monitor

        if (qidMap_.find(sbdf) != qidMap_.end()) {
          std::shared_ptr<QDeviceStateInfo> stateInfo =
              std::make_shared<QDeviceStateInfo>(qidMap_[sbdf], sbdf);

          if (stateInfo == nullptr) {
            LogError("stateInfo is empty");
          } else if (action.compare(UDEV_DEVICE_ADD_STRING) == 0) {
            stateInfo->setDeviceEvent(DEVICE_UP);

          } else if (action.compare(UDEV_DEVICE_REMOVE_STRING) == 0) {
            stateInfo->setDeviceEvent(DEVICE_DOWN);
          } else if (action.compare(UDEV_DEVICE_CHANGE_STRING) == 0) {
            udev_list_entry_foreach(property, properties) {
              std::string name = udev_list_entry_get_name(property);
              const char *val = udev_list_entry_get_value(property);

              if (udevPropertyWhitelist_.find(name) !=
                  udevPropertyWhitelist_.end()) {
                /* Extract VCID and VC State */
                if (name.compare("DBC_ID") == 0) {
                  stateInfo->setVcId(std::atoi(val));
                } else if (name.compare("DBC_STATE") == 0) {
                  if (UDEV_DBC_STATE_BEFORE_SHUTDOWN == std::atoi(val)) {
                    stateInfo->setDeviceEvent(VC_DOWN);
                  } else if (UDEV_DBC_STATE_AFTER_POWER_UP == std::atoi(val)) {
                    stateInfo->setDeviceEvent(VC_UP);
                  }
                }
              }
            }
          }
          udev_device_unref(uDevDevice);

          if ((stateInfo != nullptr) &&
              (stateInfo->deviceEvent() != INVALID_EVENT)) {
            processDeviceStateInfo(stateInfo);
          }
          LogDebug("Action: {}, SBDF: {}", action, sbdf);
        } else {
          LogWarn("Unknown SBDF: {}, possible derived device", sbdf);
        }
      }
    }
  }

cleanup:
  /* free udev */
  if (uDevMonitor != nullptr) {
    udev_monitor_unref(uDevMonitor);
  }

  if (uDev != nullptr) {
    udev_unref(uDev);
  }
}

bool QDeviceStateMonitor::isDerivedDevice(const std::string &udevSysPath) {

  // For a derrived device "accel" string is found twice in syspath.
  constexpr char devSoftLinkPattern[] = "accel";

  std::string sysPath = std::string(udevSysPath);
  std::size_t found = sysPath.find(devSoftLinkPattern);
  if (found != std::string::npos) { /* found once */
    std::string str = sysPath.substr(found + sizeof(devSoftLinkPattern) - 1);
    if (str.find(devSoftLinkPattern) != std::string::npos) { /* Found twice */
      return true;
    }
  }
  return false;
}

void QDeviceStateMonitor::processDeviceStateInfo(
    std::shared_ptr<QDeviceStateInfo> stateInfo) {

  QDeviceStateInfoEvent event = stateInfo->deviceEvent();
  QID qid = stateInfo->qid();
  uint32_t vcid = stateInfo->vcId();
  std::string sbdf = stateInfo->deviceSBDF();

  LogWarn("stateInfo: {}, qid: {}, vcId: {}", stateInfo->deviceEventName(), qid,
          vcid);

  // On each VC_DOWN event a new timer is started
  if (event == VC_DOWN) {
    std::unique_lock<std::mutex> lk(vcStatusMapMutex_);
    vcStatusMap_[std::make_pair(qid, vcid)] =
        make_pair(VC_DOWN, std::chrono::system_clock::now());
    std::thread timeoutThread =
        std::thread(&QDeviceStateMonitor::waitForSubSystemRestart, this,
                    std::make_pair(qid, vcid), sbdf);
    std::thread::id threadId = timeoutThread.get_id();
    std::unique_lock<std::mutex> threadMapLock(threadMapMutex_);
    threadMap_[threadId] = make_pair(std::move(timeoutThread), THREAD_RUNNING);
    // Remove all the Finished threads
    for (auto it = threadMap_.begin(); it != threadMap_.end();) {
      if (it->second.second == THREAD_FINISHED) {
        if (it->second.first.joinable()) {
          it->second.first.join();
        }
        it = threadMap_.erase(it);
      } else {
        ++it;
      }
    }
  } else if (event == VC_UP) {
    std::unique_lock<std::mutex> lk(vcStatusMapMutex_);
    if (vcStatusMap_.find(std::make_pair(qid, vcid)) != vcStatusMap_.end()) {
      vcStatusMap_[std::make_pair(qid, vcid)].first = VC_UP;
    }
  }

  stateEventsQueueMtx_.lock();
  stateEventsQueue_.push(std::move(stateInfo));
  stateEventsQueueMtx_.unlock();
  notifyReady();
}

void QDeviceStateMonitor::waitForSubSystemRestart(
    std::pair<QID, uint32_t> qIdVcIdPair, std::string sbdf) {
  // This thread will go to sleep for duration specified by
  // subSystemRestartWaitDurations
  constexpr uint8_t subSystemRestartWaitDuration = 15;
  uint8_t timerCounter = 0;
  std::thread::id thisThreadId = std::this_thread::get_id();
  while (timerCounter != subSystemRestartWaitDuration) {
    if (stopWaitForSubSystemRestart_.load(std::memory_order_relaxed)) {
      std::unique_lock<std::mutex> threadMapLock(threadMapMutex_);
      threadMap_[thisThreadId].second = THREAD_FINISHED;
      threadMapCv_.notify_all();
      return;
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
    timerCounter++;
  }
  LogInfo("Out of Sleep while waiting for SubSystemRestart");
  // After coming out of sleep it calculates time difference,
  // this helps in knowing if second VC_DOWN event was received
  // or not before timer expiry
  std::unique_lock<std::mutex> lk(vcStatusMapMutex_);
  auto time_diff =
      std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::system_clock::now() - vcStatusMap_[qIdVcIdPair].second)
          .count();
  if (vcStatusMap_.find(qIdVcIdPair) != vcStatusMap_.end()) {
    if (vcStatusMap_[qIdVcIdPair].first == VC_UP &&
        time_diff >= subSystemRestartWaitDuration) {
      // If VC_UP is received before this timer expired,
      // erase this entry and return
      vcStatusMap_.erase(qIdVcIdPair);
    } else if (time_diff < subSystemRestartWaitDuration) {
      // Here it indicates that there was another VC_DOWN on same VC
      // before this timer expired, hence new timer would be ongoing
      // and no action is needed by this thread
      return;
    } else {
      // If VC_UP is not received within specified duration
      // DEVICE_DOWN event is raised
      std::shared_ptr<QDeviceStateInfo> stateInfo =
          std::make_shared<QDeviceStateInfo>(qIdVcIdPair.first, sbdf);
      stateInfo->setDeviceEvent(VC_UP_WAIT_TIMER_EXPIRY);
      stateInfo->setVcId(qIdVcIdPair.second);
      LogError("SubSystem Restart wait expiry, Initiate {}, qid: {}, vcId: {}",
               stateInfo->deviceEventName(), stateInfo->qid(),
               stateInfo->vcId());
      stateEventsQueueMtx_.lock();
      stateEventsQueue_.push(std::move(stateInfo));
      stateEventsQueueMtx_.unlock();
      notifyReady();
    }
  }
  std::unique_lock<std::mutex> threadMapLock(threadMapMutex_);
  threadMap_[thisThreadId].second = THREAD_FINISHED;
  threadMapCv_.notify_all();
}

void QDeviceStateMonitor::processEvents(
    std::shared_ptr<QDeviceStateInfo> event) {
  std::unique_lock<std::mutex> lk(deviceObserversMtx_);
  for (const auto &observer : deviceObservers_) {
    observer->notifyDeviceEvent(event);
  }
}

void QDeviceStateMonitor::run() {

  stateEventsQueueMtx_.lock();
  while (!stateEventsQueue_.empty()) {
    auto element = stateEventsQueue_.front();
    stateEventsQueueMtx_.unlock();
    processEvents(element);
    stateEventsQueueMtx_.lock();
    stateEventsQueue_.pop();
  }
  stateEventsQueueMtx_.unlock();
}

QStatus QDeviceStateMonitor::refreshQidMap() {
  QStatus status = QS_SUCCESS;
  qidMap_.clear();

  DevList devList;
  QOsal::enumAicDevices(devList);

  LogInfo("Device count {}", devList.size());
  for (const auto &devInfo : devList) {
    std::string sbdf = qutil::qPciInfoToPCIeStr(devInfo.second);
    LogInfo("Device ID {} SBDF {}", devInfo.first, sbdf);
    qidMap_[sbdf] = devInfo.first;
  }
  return status;
}

QDeviceStateMonitor::~QDeviceStateMonitor() {

  stopDevicePolling_.store(true, std::memory_order_relaxed);
  // Send a signal to terminate the thread
  stopWaitForSubSystemRestart_.store(true, std::memory_order_relaxed);
  devMonThread_.join();
  std::unique_lock<std::mutex> threadMapLock(threadMapMutex_);
  for (auto it = threadMap_.begin(); it != threadMap_.end(); it++) {
    threadMapCv_.wait(threadMapLock,
                      [it] { return (it->second.second == THREAD_FINISHED); });
    if (it->second.first.joinable()) {
      it->second.first.join();
    }
  }

  // clear the queue
  while (!stateEventsQueue_.empty()) {
    stateEventsQueue_.pop();
  }
  // remove all observers
  deviceObservers_.clear();
  // Clear map
  qidMap_.clear();
}

} // namespace qaic
