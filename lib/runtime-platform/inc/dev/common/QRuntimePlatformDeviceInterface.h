// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QRUNTIME_PLATFORM_DEVICE_INTERFACE_H
#define QRUNTIME_PLATFORM_DEVICE_INTERFACE_H

#include "QAicRuntimeTypes.h"
#include "QDevCmd.h"

#include <memory>
#include <string>

namespace qaic {

enum QDevInterfaceEnum {
  QAIC_DEV_INTERFACE_AIC100 = 1,
};

class QRuntimePlatformDeviceInterface;
using shQRuntimePlatformDeviceInterface =
    std::shared_ptr<QRuntimePlatformDeviceInterface>;

class QRuntimePlatformDeviceInterface {
public:
  virtual ~QRuntimePlatformDeviceInterface() = default;

  virtual QID getQid() const = 0;
  virtual void setQPciInfo(const QPciInfo QPciInfo) = 0;

  virtual QStatus openDevice() = 0;
  virtual QStatus runDevCmd(QDevInterfaceCmdEnum cmd,
                            const void *data) const = 0;
  virtual QStatus closeDevice() = 0;
  virtual QStatus reloadDevHandle() = 0;
  virtual int getDevFd() const = 0;
  // Only valid devices should be opened.
  virtual bool isDeviceValid() const = 0;
  virtual QStatus getTelemetryInfo(QTelemetryInfo &telemetryInfo,
                                   const std::string &pciInfoString) const = 0;
};
} // namespace qaic
#endif
