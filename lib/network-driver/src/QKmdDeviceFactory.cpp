// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QKmdDeviceFactory.h"
#include "QUtil.h"
#include "QDevAic100Interface.h"
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <typeinfo>
#include <typeindex>
#include "QOsal.h"

namespace qaic {

//
// Open the char device for each device. QAIC could expose several char devices.
// The one we we interested in here is for inference only.
//
QStatus QKmdDeviceFactory::initDevices() {
  std::string path("");
  std::list<std::string> pciDevStrings;
  QStatus status = QS_SUCCESS;

  for (auto &dev : pciDevList_) {
    QID qid = dev.first;
    QOsal::getDevicePath(path, dev.second);
    auto deviceInterface =
        QDevInterface::Factory(QAIC_DEV_INTERFACE_AIC100, qid, path);

    if (!deviceInterface) {
      LogError("Failed to create device interface");
      return QS_ERROR;
    }

    deviceInterface->setQPciInfo(dev.second);
    status = deviceInterface->openDevice();

    auto device = std::shared_ptr<QDeviceInterface>(
        new QKmdDevice(qid, vcFactory_, dev.second, deviceInterface,
                       QAIC_DEV_INTERFACE_AIC100, path));

    switch (status) {
    case QS_INVAL:
      // Don't log error for invalid devices
      device->setDeviceStatus(QDevStatus::QDS_ERROR);
      break;
    case QS_SUCCESS:
      LogInfo("Pci dev {} opened with qid {}", path, qid);
      break;
    default:
      // All other errors
      LogWarn("failed to open qid {} dev {}: {}", qid, path,
              QOsal::strerror_safe(errno));
      device->setDeviceStatus(QDevStatus::QDS_ERROR);
      break;
    }

    pciDevStrings.push_back(qutil::qPciInfoToPCIeStr(dev.second));
    devMap_[qid] = std::move(device);

    path.clear(); // Set an empty string
  }

  // Add derived devices to the devMap. For now we do not want to pass along any
  // parent pci info to the derived device. So, just pass in a dummy QPciInfo
  // argument to QKmdDevice (..).
  // We will get the device strings from the mutimap, we will add them to a list
  // and sort it.
  pciDevStrings.sort();
  if (getDerivedDevices(pciDevStrings) != QS_SUCCESS) {
    LogInfo("Error getting derived devices");
  }

  return QS_SUCCESS;
}

QRuntimePlatformKmdDeviceInterface *
QKmdDeviceFactory::getRtPlatformDevice(QID qid) {
  if (devMap_.find(qid) == devMap_.end()) {
    LogError("Failed to find requested device ID: {}", qid);
    return nullptr;
  }
  return devMap_[qid].get();
}

QStatus QKmdDeviceFactory::getDeviceAddrInfo(
    std::vector<qutil::DeviceAddrInfo> &deviceInfo) {
  QStatus status = QS_SUCCESS;
  deviceInfo.clear();
  for (const auto &kv : devMap_) {
    std::pair<QID, std::string> pcieInfo;
    if (kv.second) // Unique ptr is valid
    {
      kv.second->getDeviceAddrInfo(pcieInfo);
      deviceInfo.emplace_back(pcieInfo);
    } else {
      status = QS_ERROR;
    }
  }
  return status;
}

QDeviceInterface *QKmdDeviceFactory::getDevice(QID qid) {
  if (devMap_.find(qid) == devMap_.end()) {
    LogError("Failed to find requested device ID: {}", qid);
    return nullptr;
  }
  return devMap_[qid].get();
}

QStatus QKmdDeviceFactory::getDerivedDevices(std::list<std::string> &baseDevs) {
  QID qidDerived = qutil::qidDerivedBase;
  std::list<std::string> derivedDevList;

  for (auto devPair : udevMap_) {
    if (std::find(baseDevs.begin(), baseDevs.end(), devPair.first) !=
        baseDevs.end()) {
      continue;
    }
    derivedDevList.push_back(devPair.second);
  }

  derivedDevList.sort();

  for (auto devName : derivedDevList) {
    QPciInfo dummyDev = {0};
    QOsal::getDevicePath(devName, dummyDev);

    auto deviceInterface =
        QDevInterface::Factory(QAIC_DEV_INTERFACE_AIC100, qidDerived, devName);

    if (!deviceInterface) {
      LogError("{}: Failed to create device interface", __func__);
      return QS_ERROR;
    }

    auto status = deviceInterface->openDevice();
    // Ignore the derived device if we cannot open it.
    // This device will never recover for the user but burn the QID
    if (status != QS_SUCCESS) {
      LogWarn("failed to open dev {}: {}", devName,
              QOsal::strerror_safe(errno));
      qidDerived++;
      continue;
    } else {
      LogInfo("Pci dev {} opened with qid {}", devName, (uint32_t)qidDerived);
    }

    auto device = std::shared_ptr<QDeviceInterface>(
        new (std::nothrow)
            QKmdDevice(qidDerived, vcFactory_, dummyDev, deviceInterface,
                       QAIC_DEV_INTERFACE_AIC100, devName));

    if (device == nullptr) {
      LogError("Memory allocation failure");
      return QS_NOMEM;
    }
    devMap_[qidDerived] = std::move(device);
    qidDerived++;
  }

  derivedDevList.clear();

  return QS_SUCCESS;
}
} // namespace qaic
