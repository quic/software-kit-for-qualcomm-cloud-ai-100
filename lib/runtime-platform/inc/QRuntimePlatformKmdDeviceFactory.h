// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
#ifndef QRUNTIME_PLATFORM_KMD_DEVICE_FACTORY_H
#define QRUNTIME_PLATFORM_KMD_DEVICE_FACTORY_H

#include "QRuntimePlatformKmdDeviceFactoryInterface.h"
#include "QMonitorDeviceObserver.h"
#include "QLogger.h"
#include "QOsal.h"

#include <memory>
#include <vector>
#include <list>

namespace qaic {

class QRuntimePlatformKmdDeviceFactory
    : virtual public QRuntimePlatformKmdDeviceFactoryInterface,
      virtual public QLogger,
      public QMonitorDeviceObserver {
public:
  QRuntimePlatformKmdDeviceFactory(DevList devlist, UdevMap udevMap)
      : QLogger("QRuntimePlatformKmdDeviceFactory"),
        QMonitorDeviceObserver(true), pciDevList_(devlist), udevMap_(udevMap) {}
  QRuntimePlatformKmdDeviceFactory() = delete;

  QStatus initDevices() override;
  QRuntimePlatformKmdDeviceInterface *getRtPlatformDevice(QID qid) override;

  QStatus
  getDeviceAddrInfo(std::vector<qutil::DeviceAddrInfo> &deviceInfo) override;

  // from QMonitorDeviceObserver interface
  QStatus notifyDeviceEvent(std::shared_ptr<QDeviceStateInfo> event) override;

protected:
  DevList pciDevList_;
  UdevMap udevMap_;

private:
  QStatus getDerivedDevices(std::list<std::string> &baseDevs);

  std::map<QID, std::shared_ptr<QRuntimePlatformKmdDeviceInterface>>
      rtPlatformKmdDevMap_; /// Derived class does not use this map
};

} // namespace qaic

#endif // QRUNTIME_PLATFORM_KMD_DEVICE_FACTORY_H
