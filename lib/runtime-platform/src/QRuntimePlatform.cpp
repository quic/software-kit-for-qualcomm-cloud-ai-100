// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QRuntimePlatform.h"
#include "QRuntimePlatformKmdDeviceInterface.h"
#include "QRuntimePlatformKmdDeviceFactoryInterface.h"
#include "QUtil.h"

#include "unistd.h"

namespace qaic {

QRuntimePlatform::QRuntimePlatform(
    QRuntimePlatformKmdDeviceFactoryInterface *devFactory)
    : QLogger("QRuntimePlatform"), devFactoryUnique_(nullptr),
      devFactory_(devFactory) {}

QRuntimePlatform::QRuntimePlatform(
    std::unique_ptr<QRuntimePlatformKmdDeviceFactoryInterface> devFactory)
    : QLogger("QRuntimePlatform"), devFactoryUnique_(std::move(devFactory)) {
  devFactory_ = devFactoryUnique_.get();
}

QStatus QRuntimePlatform::getDeviceIds(std::vector<QID> &devices) {
  QStatus status = QS_ERROR;
  devices.clear();

  std::vector<qutil::DeviceAddrInfo> deviceInfoList;
  status = getDeviceAddrInfo(deviceInfoList);
  if (status != QS_SUCCESS) {
    return status;
  }

  for (const auto &deviceInfo : deviceInfoList) {
    devices.push_back(deviceInfo.first);
  }

  if (devices.size() > 0) {
    status = QS_SUCCESS;
  } else {
    LogErrorG("No valid device found");
    status = QS_NODEV;
  }
  return QS_SUCCESS;
}

QStatus QRuntimePlatform::queryStatus(QID deviceID, QDevInfo &devInfo) {
  QRuntimePlatformKmdDeviceInterface *dev =
      devFactory_->getRtPlatformDevice(deviceID);
  if (dev == nullptr) {
    return QS_NODEV;
  }
  return dev->queryStatus(devInfo);
}

int QRuntimePlatform::openCtrlChannel(QID deviceID,
                                      QCtrlChannelID ctrlChannelID,
                                      QStatus &status) {
  QRuntimePlatformKmdDeviceInterface *dev =
      devFactory_->getRtPlatformDevice(deviceID);
  if (dev == nullptr) {
    status = QS_NODEV;
    return -1;
  }

  int fd = -1;
  status = dev->openCtrlChannel(ctrlChannelID, fd);
  return fd;
}

QStatus QRuntimePlatform::closeCtrlChannel(int handle) {
  close(handle);
  return QS_SUCCESS;
}

QStatus QRuntimePlatform::getDeviceAddrInfo(
    std::vector<qutil::DeviceAddrInfo> &deviceInfo) const {
  return devFactory_->getDeviceAddrInfo(deviceInfo);
}

QStatus QRuntimePlatform::isDeviceFeatureEnabled(QID deviceID,
                                                 qutil::DeviceFeatures feature,
                                                 bool &isEnabled) const {
  QRuntimePlatformKmdDeviceInterface *dev =
      devFactory_->getRtPlatformDevice(deviceID);
  if (dev == nullptr) {
    return QS_NODEV;
  }
  return dev->isDeviceFeatureEnabled(feature, isEnabled);
}

QStatus QRuntimePlatform::getBoardInfo(QID deviceID,
                                       QBoardInfo &boardInfo) const {
  QRuntimePlatformKmdDeviceInterface *dev =
      devFactory_->getRtPlatformDevice(deviceID);
  if (dev == nullptr) {
    return QS_NODEV;
  }
  return dev->getBoardInfo(boardInfo);
}

QStatus
QRuntimePlatform::getTelemetryInfo(QID deviceID,
                                   QTelemetryInfo &telemetryInfo) const {
  QRuntimePlatformKmdDeviceInterface *dev =
      devFactory_->getRtPlatformDevice(deviceID);
  if (dev == nullptr) {
    return QS_NODEV;
  }
  return dev->getTelemetryInfo(telemetryInfo);
}

QStatus QRuntimePlatform::getDeviceName(QID deviceID,
                                        std::string &devName) const {
  QRuntimePlatformKmdDeviceInterface *dev =
      devFactory_->getRtPlatformDevice(deviceID);
  if (dev == nullptr) {
    return QS_NODEV;
  }
  devName = dev->getDeviceName();
  return QS_SUCCESS;
}

const QPciInfo *QRuntimePlatform::getDevicePciInfo(QID deviceID) const {
  QRuntimePlatformKmdDeviceInterface *dev =
      devFactory_->getRtPlatformDevice(deviceID);
  if (dev == nullptr) {
    return nullptr;
  }
  return dev->getDevicePciInfo();
}

} // namespace qaic
