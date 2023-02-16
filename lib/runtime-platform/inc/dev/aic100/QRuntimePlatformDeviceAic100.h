// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
#ifndef QRUNTIME_PLATFORM_DEVICE_AIC100_H
#define QRUNTIME_PLATFORM_DEVICE_AIC100_H

#include "dev/common/QRuntimePlatformDeviceInterface.h"
#include <optional>

namespace qaic {

class QRuntimePlatformDeviceAic100
    : virtual public QRuntimePlatformDeviceInterface {

public:
  static shQRuntimePlatformDeviceInterface Factory(QID qid,
                                                   const std::string &devName);

  QRuntimePlatformDeviceAic100(QID qid, const std::string &devName);

  QID getQid() const override { return qid_; }

  void setQPciInfo(const QPciInfo QPciInfo) override { QPciInfo_ = QPciInfo; }

  bool isDeviceValid() const override;
  QStatus openDevice() override;
  QStatus runDevCmd(QDevInterfaceCmdEnum cmd, const void *data) const override;
  QStatus closeDevice() override;
  QStatus reloadDevHandle() override;
  int getDevFd() const override;
  QStatus getTelemetryInfo(QTelemetryInfo &telemetryInfo,
                           const std::string &pciInfoString) const override;

private:
  static constexpr uint32_t invalidBdcId = -1U;
  static constexpr const char *invalidDevName_ = "_invalid";
  QDevInterfaceEnum qDevInterfaceEnum_;
  QID qid_;
  std::string devName_;
  QDevHandle devHandle_;
  std::optional<QPciInfo> QPciInfo_;
};
} // namespace qaic
#endif // QRUNTIME_PLATFORM_DEVICE_AIC100_H
