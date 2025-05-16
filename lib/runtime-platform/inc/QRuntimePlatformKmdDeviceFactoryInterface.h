// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QRUNTIME_PLATFORM_KMD_DEVICE_FACTORY_INTERFACE_H
#define QRUNTIME_PLATFORM_KMD_DEVICE_FACTORY_INTERFACE_H

#include "QAicRuntimeTypes.h"
#include "QUtil.h"

#include <map>
#include <memory>

namespace qaic {

class QRuntimePlatformKmdDeviceInterface;

class QRuntimePlatformKmdDeviceFactoryInterface {
public:
  virtual QStatus initDevices() = 0;
  virtual QRuntimePlatformKmdDeviceInterface *getRtPlatformDevice(QID qid) = 0;
  virtual QStatus
  getDeviceAddrInfo(std::vector<qutil::DeviceAddrInfo> &deviceInfo) = 0;
  virtual ~QRuntimePlatformKmdDeviceFactoryInterface() = default;
};

} // namespace qaic

#endif // QRUNTIME_PLATFORM_KMD_DEVICE_FACTORY_INTERFACE_H
