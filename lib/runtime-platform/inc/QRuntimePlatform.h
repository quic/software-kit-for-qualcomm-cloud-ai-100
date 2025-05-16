// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QRUNTIME_PLATFORM_H
#define QRUNTIME_PLATFORM_H

#include "QRuntimePlatformInterface.h"
#include "QLogger.h"
#include "QRuntimePlatformKmdDeviceFactoryInterface.h"
#include "QRuntimePlatformKmdDeviceInterface.h"

#include <map>
#include <memory>

namespace qaic {

/// This class implements the QRuntimePlatformInterface interface. All API
/// functions
/// are implemented in this class.
class QRuntimePlatform : virtual public QRuntimePlatformInterface,
                         virtual public QLogger {
public:
  /// The runtime is agnostic to driver backend implementation which is
  /// managed by \p devFactory.
  /// Call from derrived class object. Use devFactory from derrived class
  QRuntimePlatform(QRuntimePlatformKmdDeviceFactoryInterface *devFactory);
  /// Call when object of QRuntimePlatform is created. Use own devFactory
  QRuntimePlatform(
      std::unique_ptr<QRuntimePlatformKmdDeviceFactoryInterface> devFactory);

  ~QRuntimePlatform() = default;

  QStatus getDeviceIds(std::vector<QID> &devices) override;

  QStatus queryStatus(QID deviceID, QDevInfo &devInfo) override;

  int openCtrlChannel(QID deviceID, QCtrlChannelID ctrlChannelId,
                      QStatus &status) override;
  QStatus closeCtrlChannel(int handle) override;

  QStatus getDeviceAddrInfo(
      std::vector<qutil::DeviceAddrInfo> &deviceInfo) const override;

  virtual QStatus isDeviceFeatureEnabled(QID deviceID,
                                         qutil::DeviceFeatures feature,
                                         bool &isEnabled) const override;

  QStatus getBoardInfo(QID deviceID, QBoardInfo &boardInfo) const override;
  QStatus getTelemetryInfo(QID deviceID,
                           QTelemetryInfo &telemetryInfo) const override;

  QStatus getDeviceName(QID deviceID, std::string &devName) const override;

  const QPciInfo *getDevicePciInfo(QID deviceID) const override;

private:
  QRuntimePlatform() = delete;
  QRuntimePlatform(const QRuntimePlatform &other) = delete;
  QRuntimePlatform &operator=(const QRuntimePlatform &other) = delete;

  std::unique_ptr<QRuntimePlatformKmdDeviceFactoryInterface> devFactoryUnique_;
  QRuntimePlatformKmdDeviceFactoryInterface *devFactory_;
};

} // namespace qaic

#endif // QRUNTIME_PLATFORM_H
