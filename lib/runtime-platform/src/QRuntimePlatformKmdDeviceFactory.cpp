// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QUtil.h"
#include "QRuntimePlatformKmdDeviceFactory.h"
#include "QRuntimePlatformKmdDevice.h"
#include "dev/aic100/QRuntimePlatformDeviceAic100.h"

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
QStatus QRuntimePlatformKmdDeviceFactory::initDevices() {
  std::string path("");
  std::list<std::string> pciDevStrings;
  QStatus status = QS_SUCCESS;

  for (auto &dev : pciDevList_) {
    QID qid = dev.first;
    QOsal::getDevicePath(path, dev.second);
    auto qRuntimePlatformDeviceInterface =
        QRuntimePlatformDeviceAic100::Factory(qid, path);

    if (!qRuntimePlatformDeviceInterface) {
      LogError("Failed to create Platform Device Interface");
      return QS_NOMEM;
    }

    qRuntimePlatformDeviceInterface->setQPciInfo(dev.second);
    status = qRuntimePlatformDeviceInterface->openDevice();

    auto device = std::shared_ptr<QRuntimePlatformKmdDeviceInterface>(
        new QRuntimePlatformKmdDevice(qid, dev.second,
                                      qRuntimePlatformDeviceInterface,
                                      QAIC_DEV_INTERFACE_AIC100, path));

    if (status != QS_SUCCESS) {
      LogWarn("failed to open dev {}: {}", path, QOsal::strerror_safe(errno));

      device->setDeviceStatus(QDevStatus::QDS_ERROR);
    } else {
      LogInfo("Pci dev {} opened with qid {}", path, qid);
    }

    pciDevStrings.push_back(qutil::qPciInfoToPCIeStr(dev.second));
    rtPlatformKmdDevMap_[qid] = std::move(device);

    path.clear(); // Set an empty string
  }

  // Add derived devices to the devMap. For now we do not want to pass along any
  // parent pci info to the derived device. So, just pass in a dummy QPciInfo
  // argument to QRuntimePlatformKmdDevice (..).
  // We will get the device strings from the mutimap, we will add them to a list
  // and sort it.
  pciDevStrings.sort();
  if (getDerivedDevices(pciDevStrings) != QS_SUCCESS) {
    LogInfo("Error getting derived devices");
  }

  return QS_SUCCESS;
}

QRuntimePlatformKmdDeviceInterface *
QRuntimePlatformKmdDeviceFactory::getRtPlatformDevice(QID qid) {
  if (rtPlatformKmdDevMap_.find(qid) == rtPlatformKmdDevMap_.end()) {
    LogError("Failed to find requested device ID: {}", qid);
    return nullptr;
  }
  return rtPlatformKmdDevMap_[qid].get();
}

QStatus QRuntimePlatformKmdDeviceFactory::getDeviceAddrInfo(
    std::vector<qutil::DeviceAddrInfo> &deviceInfo) {
  QStatus status = QS_SUCCESS;
  deviceInfo.clear();
  for (const auto &kv : rtPlatformKmdDevMap_) {
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

QStatus QRuntimePlatformKmdDeviceFactory::getDerivedDevices(
    std::list<std::string> &baseDevs) {
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
    QPciInfo dummyDev;
    QOsal::getDevicePath(devName, dummyDev);

    auto qRuntimePlatformDeviceInterface =
        QRuntimePlatformDeviceAic100::Factory(qidDerived, devName);

    if (!qRuntimePlatformDeviceInterface) {
      LogError("Failed to create Platform Device Interface");
      return QS_NOMEM;
    }

    auto status = qRuntimePlatformDeviceInterface->openDevice();

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

    auto device = std::shared_ptr<QRuntimePlatformKmdDeviceInterface>(
        new (std::nothrow) QRuntimePlatformKmdDevice(
            qidDerived, dummyDev, qRuntimePlatformDeviceInterface,
            QAIC_DEV_INTERFACE_AIC100, devName));

    if (device == nullptr) {
      LogError("Memory allocation failure");
      return QS_NOMEM;
    }
    rtPlatformKmdDevMap_[qidDerived] = std::move(device);
    qidDerived++;
  }

  derivedDevList.clear();

  return QS_SUCCESS;
}

QStatus QRuntimePlatformKmdDeviceFactory::notifyDeviceEvent(
    std::shared_ptr<QDeviceStateInfo> event) {
  QID qid = event->qid();
  std::string deviceSBDF = event->deviceSBDF();
  QRuntimePlatformKmdDeviceInterface *kmdDev = getRtPlatformDevice(qid);
  if (kmdDev) {
    shQRuntimePlatformDeviceInterface devInterface =
        kmdDev->getQRuntimePlatformDeviceInterface();
    if (devInterface) {
      if (event->deviceEvent() == DEVICE_DOWN) {
        LogInfo("Close Device handle: {} {}", qid, deviceSBDF);
        devInterface->closeDevice();
      } else if (event->deviceEvent() == DEVICE_UP) {
        LogInfo("Open Device handle: {} {}", qid, deviceSBDF);
        devInterface->openDevice();
      }
    }
  }

  return QS_SUCCESS;
}

} // namespace qaic
