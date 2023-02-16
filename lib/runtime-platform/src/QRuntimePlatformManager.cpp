// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QRuntimePlatformManager.h"
#include "QRuntimePlatform.h"
#include "QRuntimePlatformKmdDeviceFactoryInterface.h"
#include "QRuntimePlatformKmdDeviceFactory.h"
#include "QOsal.h"

#include <vector>
#include <mutex>
#include <map>

namespace qaic {

shQRuntimePlatformInterface QRuntimePlatformManager::runtimePlatform_ = nullptr;
std::mutex QRuntimePlatformManager::m_;

shQRuntimePlatformInterface QRuntimePlatformManager::getKmdRuntimePlatform() {

  QOsal::initPlatform();

  DevList devList;
  QOsal::enumAicDevices(devList);

  UdevMap udevMap;
  QOsal::createUdevMap(udevMap);

  auto devFactory = std::unique_ptr<QRuntimePlatformKmdDeviceFactoryInterface>(
      new (std::nothrow) QRuntimePlatformKmdDeviceFactory(devList, udevMap));
  if (devFactory == nullptr) {
    return nullptr;
  }
  if (devFactory->initDevices() != QS_SUCCESS) {
    return nullptr;
  }

  auto runtimePlatform = shQRuntimePlatformInterface(
      new (std::nothrow) QRuntimePlatform(std::move(devFactory)));
  return runtimePlatform;
}

shQRuntimePlatformInterface QRuntimePlatformManager::getRuntimePlatform() {
  std::unique_lock<std::mutex> lk(m_);

  if (!runtimePlatform_) {
    runtimePlatform_ = getKmdRuntimePlatform();
  }
  return runtimePlatform_;
}

} // namespace qaic
