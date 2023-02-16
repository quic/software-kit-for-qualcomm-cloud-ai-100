// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QKMD_DEVICE_FACTORY_H
#define QKMD_DEVICE_FACTORY_H

#include "QDeviceFactoryInterface.h"
#include "QVCFactoryInterface.h"
#include "QLogger.h"
#include "QKmdDevice.h"
#include "QKmdVCFactory.h"
#include "QRuntimePlatformKmdDeviceFactory.h"

#include <memory>
#include <vector>
#include <list>

namespace qaic {

class QKmdDeviceFactory : public QDeviceFactoryInterface,
                          public QRuntimePlatformKmdDeviceFactory {
public:
  QKmdDeviceFactory(std::shared_ptr<QKmdVCFactory> vcFactory, DevList devlist,
                    UdevMap udevMap)
      : QRuntimePlatformKmdDeviceFactoryInterface(),
        QLogger("QKmdDeviceFactory"),
        QRuntimePlatformKmdDeviceFactory(devlist, udevMap),
        vcFactory_(vcFactory) {}

  QStatus initDevices() override;

  QRuntimePlatformKmdDeviceInterface *getRtPlatformDevice(QID qid) override;
  QStatus
  getDeviceAddrInfo(std::vector<qutil::DeviceAddrInfo> &deviceInfo) override;

  QDeviceInterface *getDevice(QID qid) override;

protected:
  std::shared_ptr<QKmdVCFactory> vcFactory_;

private:
  QStatus getDerivedDevices(std::list<std::string> &baseDevs);
  QStatus getPlatformNspDevices();

  std::map<QID, std::shared_ptr<QDeviceInterface>> devMap_;
};

} // namespace qaic

#endif // QKMD_DEVICE_FACTORY_H
