// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QRuntime.h"
#include "QDeviceInterface.h"
#include "QDeviceFactoryInterface.h"
#include "QImageParserInterface.h"
#include "QMetaDataInterface.h"
#include "QVirtualChannelInterface.h"
#include "QNNConstants.h"
#include "QNNImage.h"
#include "QNeuralNetwork.h"
#include "QUtil.h"
#include "QAicQpc.h"
#include "QImageParser.h"

#include "QDevAic100Interface.h"

#include "unistd.h"

namespace qaic {

constexpr uint16_t activationRetryCount = 5;

QRuntime::QRuntime(std::unique_ptr<QDeviceFactoryInterface> devFactory,
                   std::unique_ptr<QImageParserInterface> imgParser)
    : QLogger("QRuntime"), QRuntimePlatform(devFactory.get()),
      devFactory_(std::move(devFactory)), imgParser_(std::move(imgParser)) {}

QNNImageInterface *QRuntime::loadImage(QID deviceID, const QBuffer &buf,
                                       const char *name, QStatus &status,
                                       uint32_t metadataCRC) {
  QBuffer networkElfBuf = {};
  QDeviceInterface *dev = devFactory_->getDevice(deviceID);
  if (dev == nullptr) {
    status = QS_NODEV;
    return nullptr;
  }

  if (getQPCSegment(buf.buf, "network.elf", &(networkElfBuf.buf),
                    &(networkElfBuf.size), 0)) {
    if (networkElfBuf.buf == nullptr) {
      status = QS_INVAL;
      return nullptr;
    }
  } else {
    networkElfBuf.buf = buf.buf;
    networkElfBuf.size = buf.size;
  }

  // Check if the buf is a DLC. If so extract the network elf
  std::unique_ptr<QMetaDataInterface> meta =
      imgParser_->parseImage(networkElfBuf, status);
  if (status != QS_SUCCESS) {
    LogError("Failed to parse network image , error {}",
             qutil::statusStr(status));
    return nullptr;
  }
  assert(meta != nullptr);
  QNNImageID imageID;
  status = dev->loadImage(networkElfBuf, name, imageID, metadataCRC);
  if (status != QS_SUCCESS) {
    LogError("Failed to load network image with name ({}), error {}", name,
             qutil::statusStr(status));
    return nullptr;
  }
  QNNImage *img =
      new QNNImage(dev, std::move(meta), networkElfBuf, deviceID, imageID);
  if (!img) {
    LogError("Failed to create QNNImage with image ID {}", imageID);
    status = QS_NOMEM;
    return nullptr;
  }
  LogInfo("Network image ({}) loaded with ID {}", name, imageID);
  status = QS_SUCCESS;
  return img;
}

QNNConstantsInterface *QRuntime::loadConstants(QID deviceID, const QBuffer &buf,
                                               const char *name,
                                               QStatus &status,
                                               uint32_t constantsCRC) {
  QBuffer constantsBuf = {};
  QDeviceInterface *dev = devFactory_->getDevice(deviceID);
  if (dev == nullptr) {
    status = QS_NODEV;
    return nullptr;
  }

  QNNConstantsID constantsID;
  if (getQPCSegment(buf.buf, "constants.bin", &(constantsBuf.buf),
                    &(constantsBuf.size), 0)) {
    if (constantsBuf.buf == nullptr) {
      status = QS_INVAL;
      return nullptr;
    }
  } else {
    constantsBuf.buf = buf.buf;
    constantsBuf.size = buf.size;
  }

  status = dev->loadConstants(constantsBuf, name, constantsID, constantsCRC);
  if (status != QS_SUCCESS) {
    LogError("Failed to load network constants with name ({}), error {}", name,
             qutil::statusStr(status));
    return nullptr;
  }
  QNNConstants *constants = new QNNConstants(dev, deviceID, constantsID);
  if (!constants) {
    LogError("Failed to create QNNConstants with ID {}", constantsID);
    status = QS_NOMEM;
    dev->unloadConstants(constantsID);
    return nullptr;
  }
  LogInfo("Network constants ({}) loaded with ID {}", name, constantsID);
  status = QS_SUCCESS;

  return constants;
}

QNNConstantsInterface *QRuntime::loadConstantsEx(QID deviceID,
                                                 const QBuffer &buf,
                                                 const char *name,
                                                 QStatus &status) {
  QBuffer constDescBuf = {};
  QDeviceInterface *dev = devFactory_->getDevice(deviceID);
  if (dev == nullptr) {
    status = QS_NODEV;
    return nullptr;
  }

  QNNConstantsID constantsID = 0;
  if (getQPCSegment(buf.buf, "constantsdesc.bin", &(constDescBuf.buf),
                    &(constDescBuf.size), 0)) {
    if (constDescBuf.buf == nullptr) {
      status = QS_INVAL;
      return nullptr;
    }
  } else {
    constDescBuf.buf = buf.buf;
    constDescBuf.size = buf.size;
  }

  status = dev->loadConstantsEx(constDescBuf, name, constantsID);
  if (status != QS_SUCCESS) {
    LogError("Failed to setup network constants with name ({}), error {}", name,
             qutil::statusStr(status));
    return nullptr;
  }
  QNNConstants *constants = new QNNConstants(dev, deviceID, constantsID);
  if (!constants) {
    LogError("Failed to create QNNConstants with ID {}", constantsID);
    status = QS_NOMEM;
    dev->unloadConstants(constantsID);
    return nullptr;
  }
  LogInfo("Network constants ({}) loaded with ID {}", name, constantsID);
  status = QS_SUCCESS;

  return constants;
}

QNeuralNetworkInterface *
QRuntime::activateNetwork(QNNImageInterface *image,
                          QNNConstantsInterface *constants, QStatus &status,
                          QActivationStateType initialState,
                          uint32_t waitTimeoutMs, uint32_t numMaxWaitRetries) {
  if (image == nullptr) {
    status = QS_INVAL;
    return nullptr;
  }
  QID deviceID = image->getQID();
  // Default value of 0 indicates no constants
  QNNConstantsID constantsId = 0;
  if (constants != nullptr) {
    constantsId = constants->getConstantsID();
  }

  if ((constants != nullptr) && (deviceID != constants->getQID())) {
    LogWarn("activate failed: image/constants on different devices");
    status = QS_INVAL;
    return nullptr;
  }

  QDeviceInterface *dev = devFactory_->getDevice(deviceID);
  if (dev == nullptr) {
    status = QS_NODEV;
    return nullptr;
  }
  LogInfo("Activating image {} constants {} on device {}", image->getImageID(),
          constantsId, uint32_t(deviceID));

  QNAID naID = 0;
  QVirtualChannelInterface *vc = nullptr;
  uint64_t ddrBase = 0, mcIDBase = 0;

  uint16_t retryCount = activationRetryCount;
  while (retryCount--) {
    status = dev->activate(image->getImageID(), constantsId, naID, ddrBase,
                           mcIDBase, &vc, initialState);

    if (status == QS_AGAIN) {
      /* Wait for device to recover */
      std::this_thread::sleep_for(std::chrono::seconds(1));
      LogWarn("Dev {} Activate retryCount {}", deviceID,
              (activationRetryCount - retryCount));
      continue;
    }
    break;
  }

  if (status != QS_SUCCESS) {
    LogError("Dev {} activate failed status {}", deviceID, status);
    return nullptr;
  }

  if (vc == nullptr) {
    LogError("Invalid VC Null pointer");
    return nullptr;
  }

  auto meta = static_cast<QNNImage *>(image)->getMetaData();
  assert(meta != nullptr);
  auto updatedMeta = meta->getUpdatedMetadata(ddrBase, mcIDBase);
  assert(updatedMeta != nullptr);

  QNeuralNetworkInterface *nn = nullptr;
  switch (dev->getDeviceInterfaceType()) {
  case QAIC_DEV_INTERFACE_AIC100:
    nn =
        QNeuralnetwork::Factory(dev, image, constants, std::move( updatedMeta), vc, naID, waitTimeoutMs, numMaxWaitRetries);
    break;
  default:
    LogError("Invalid device type {}", dev->getDeviceInterfaceType());
    break;
  }

  if (nn == nullptr) {
    status = QS_NOMEM;
  }

  static_cast<QNNImage *>(image)->incRef();
  if (constants != nullptr) {
    static_cast<QNNConstants *>(constants)->incRef();
  }

  LogInfo("Activated image  {} constants {} on device {} with QNAI {}",
          image->getImageID(), constantsId, uint32_t(deviceID), naID);

  return nn;
}

QStatus QRuntime::sendActivationStateChangeCommand(
    QID deviceID,
    std::vector<std::pair<QNAID, QActivationStateType>> &stateCmdSet) {

  QDeviceInterface *dev = devFactory_->getDevice(deviceID);
  if (dev == nullptr) {
    return QS_NODEV;
  }

  return dev->sendActivationStateChangeCommand(stateCmdSet);
}

void QRuntime::getRuntimeVersion(QRTVerInfo &version) {
  version.major = 0;
  version.minor = 1;
  version.patch = 0;
  version.variant = "LRT.AIC.REL";
}

bool QRuntime::isValidDevice(QID deviceID) {
  QDeviceInterface *dev = devFactory_->getDevice(deviceID);
  if (dev == nullptr) {
    return false;
  }
  return true;
}

QDeviceInterface *QRuntime::getDevice(QID deviceID) {
  return devFactory_->getDevice(deviceID);
}

QStatus QRuntime::getResourceInfo(QID deviceID, QResourceInfo &resourceInfo) {

  QDeviceInterface *dev = devFactory_->getDevice(deviceID);
  if (dev == nullptr) {
    return QS_NODEV;
  }
  return dev->getResourceInfo(resourceInfo);
}

QStatus QRuntime::getPerformanceInfo(QID deviceID,
                                     QPerformanceInfo &performanceInfo) {
  QDeviceInterface *dev = devFactory_->getDevice(deviceID);
  if (dev == nullptr) {
    return QS_NODEV;
  }
  return dev->getPerformanceInfo(performanceInfo);
}

QStatus QRuntime::createResourceReservation(
    QID deviceID, uint16_t numNetworks, uint32_t numNsps, uint32_t numMcid,
    uint32_t numSem, uint32_t numVc, uint64_t shddrSize, uint64_t l2RegionSize,
    uint64_t ddrSize, QResourceReservationID &resResId,
    QResourceReservationID sourceReservationId = RESOURCE_GROUP_ID_DEFAULT) {

  QDeviceInterface *dev = devFactory_->getDevice(deviceID);
  if (dev == nullptr) {
    return QS_NODEV;
  }

  return dev->createResourceReservation(numNetworks, numNsps, numMcid, numSem,
                                        numVc, shddrSize, l2RegionSize, ddrSize,
                                        resResId, sourceReservationId);
}

QStatus QRuntime::releaseResourceReservation(QID deviceID,
                                             QResourceReservationID acResId) {
  QDeviceInterface *dev = devFactory_->getDevice(deviceID);
  if (dev == nullptr) {
    return QS_NODEV;
  }

  return dev->releaseResourceReservation(acResId);
}

QStatus QRuntime::createResourceReservedDevice(QID deviceID,
                                               QResourceReservationID resResId) {
  QDeviceInterface *dev = devFactory_->getDevice(deviceID);
  if (dev == nullptr) {
    return QS_NODEV;
  }
  return dev->createResourceReservedDevice(resResId);
}

QStatus
QRuntime::releaseResourceReservedDevice(QID deviceID,
                                        QResourceReservationID resResId) {
  QDeviceInterface *dev = devFactory_->getDevice(deviceID);
  if (dev == nullptr) {
    return QS_NODEV;
  }
  return dev->releaseResourceReservedDevice(resResId);
}
//---------------------------------------------------------------------------
// Private Methods
//---------------------------------------------------------------------------
QStatus QRuntime::checkQueryStatusFormat(QDevInfo *devInfo,
                                         QDeviceInterface *dev) {
  if (!devInfo || !dev) {
    return QS_INVAL;
  }

  if (devInfo->devData.formatVersion < deviceInfoFormatVersion) {
    LogError("StatusQuery format mismatch: Device {}: Host {}",
             devInfo->devData.formatVersion, deviceInfoFormatVersion);
    devInfo->devStatus = QDevStatus::QDS_ERROR;
    dev->setDeviceStatus(devInfo->devStatus);
    return QS_PROTOCOL;
  }

  return QS_SUCCESS;
}

} // namespace qaic
