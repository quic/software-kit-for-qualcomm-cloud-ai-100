// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QMonitorDeviceObserver.h"
#include "QDeviceStateMonitor.h"

namespace qaic {

QMonitorDeviceObserver::QMonitorDeviceObserver(bool priority) {
  devMon_ = QDeviceStateMonitorManager::getDeviceStateMonitor();
  if (devMon_ != nullptr) {
    if (priority) {
      devMon_->registerPriorityDeviceObserver(this);
    } else {
      devMon_->registerDeviceObserver(this);
    }
  }
}

QMonitorDeviceObserver::~QMonitorDeviceObserver() {
  unregisterDeviceObserver();
}

void QMonitorDeviceObserver::unregisterDeviceObserver() {
  if (devMon_ != nullptr) {
    devMon_->unregisterDeviceObserver(this);
    devMon_ = nullptr;
  }
}

bool QMonitorDeviceObserver::isDerivedDevice(QID qid) {
  if (qid < qutil::qidDerivedBase)
    return false;
  else
    return true;
}

} // namespace qaic
