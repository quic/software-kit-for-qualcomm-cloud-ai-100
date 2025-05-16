// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QMONITORSDEVICEOBSERVER_H
#define QMONITORSDEVICEOBSERVER_H

#include "QAicRuntimeTypes.h"
#include "QDeviceStateInfo.h"

namespace qaic {
class QDeviceStateMonitor;

using QDeviceStateMonitorShared = std::shared_ptr<QDeviceStateMonitor>;

/// This abstract class defines the interface to QMonitor device event observers
class QMonitorDeviceObserver {
public:
  /// Implement handle request for every given service
  QMonitorDeviceObserver(bool priority = false);
  virtual QStatus
  notifyDeviceEvent(std::shared_ptr<QDeviceStateInfo> event) = 0;

  virtual ~QMonitorDeviceObserver();

protected:
  void unregisterDeviceObserver();
  bool isDerivedDevice(QID qid);

private:
  QDeviceStateMonitorShared devMon_;
};

} // namespace qaic

#endif // QMONITORSDEVICEOBSERVER_H