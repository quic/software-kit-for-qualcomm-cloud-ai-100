// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QRuntimeManager.h"
#include "metadataflatbufDecode.hpp"
#include "QLogger.h"
#include "QDeviceInterface.h"
#include "QNncProtocol.h"
#include "QKmdRuntime.h"

namespace qaic {

std::unique_ptr<QRuntimeInterface> QRuntimeManager::runtime_ = nullptr;
QLogInterface *QRuntimeManager::logger_ = nullptr;
std::mutex QRuntimeManager::m_;

void QRuntimeManager::setLogger(QLogInterface *logger) {
  std::unique_lock<std::mutex> lk(m_);
  logger_ = logger;
}

QRuntimeInterface *QRuntimeManager::getRuntime() {
  std::unique_lock<std::mutex> lk(m_);

  std::unique_ptr<QRuntimeInterface> rt = nullptr;
  if (!runtime_) {
    rt = getKmdRuntime();
    if ((!rt) || !checkFWVersions(rt.get())) {
      return nullptr;
    }
    runtime_.swap(rt);
  }
  return runtime_.get();
}

void QRuntimeManager::setRuntime(std::unique_ptr<QRuntimeInterface> rt) {
  std::unique_lock<std::mutex> lk(m_);
  runtime_.swap(rt);
}

QLogInterface *QRuntimeManager::getLogger() {
  std::unique_lock<std::mutex> lk(m_);
  return logger_;
}

bool QRuntimeManager::checkFWVersions(QRuntimeInterface *rt) {
  QStatus status;
  uint32_t liveDevCnt = 0, incompDevCnt = 0;
  QDevInfo devInfo;

  if (rt == nullptr) {
    return false;
  }

  // QID | PcieInfo
  std::vector<qutil::DeviceAddrInfo> deviceAddrInfo;
  status = rt->getDeviceAddrInfo(deviceAddrInfo);
  if (status != QS_SUCCESS) {
    return status;
  }

  LogInfoG("Device count {}", deviceAddrInfo.size());
  for (uint32_t i = 0; i < deviceAddrInfo.size(); i++) {
    QID qid = deviceAddrInfo.at(i).first;
    if (qid < qutil::qidDerivedBase) {
      status = rt->queryStatus(qid, devInfo);
      if (status == QS_SUCCESS) {

        /// If this device is not in a good state then skip it
        if (devInfo.devStatus != QDevStatus::QDS_READY)
          continue;

        /// Check metaversion.  Both the major and minor must match.
        if ((devInfo.devData.metaVerMaj != AIC_METADATA_MAJOR_VERSION) ||
            (devInfo.devData.metaVerMin != AIC_METADATA_MINOR_VERSION)) {

          LogErrorG(
              "Metadata Version mismatch: QID {}:  Device {}.{}: Host {}.{}",
              (uint32_t)devInfo.qid, (uint32_t)devInfo.devData.metaVerMaj,
              (uint32_t)devInfo.devData.metaVerMin, AIC_METADATA_MAJOR_VERSION,
              AIC_METADATA_MINOR_VERSION);
          incompDevCnt++;
          continue;
        }

        /// Check QSM API major version.  Must match.
        if (devInfo.devData.nncCommandProtocolMajorVersion !=
            NNC_COMMAND_PROTOCOL_MAJOR_VERSION) {
          LogErrorG(
              "NNC Command Protocol Major Version mismatch: QID {}: Device "
              "Major Version {}: must be = {}",
              (uint32_t)devInfo.qid,
              (uint32_t)devInfo.devData.nncCommandProtocolMajorVersion,
              NNC_COMMAND_PROTOCOL_MAJOR_VERSION);
          incompDevCnt++;
          continue;
        }

        /// Fail if the device has an older minor version that the host
        uint16_t nncCommandProtocolMinorVersion =
            NNC_COMMAND_PROTOCOL_MINOR_VERSION;
        if (nncCommandProtocolMinorVersion >
            devInfo.devData.nncCommandProtocolMinorVersion) {
          LogWarnG(
              "NNC Command Protocol Minor Version smaller than required: QID "
              "{}: Device Minor Version: {}, must be >= {}",
              (uint32_t)devInfo.qid,
              (uint32_t)devInfo.devData.nncCommandProtocolMinorVersion,
              NNC_COMMAND_PROTOCOL_MINOR_VERSION);
          continue;
        }
      }
      liveDevCnt++;
    } else if (qid >= qidPlatformNspDeviceBase) {
      // Embedded NSP device
      status = rt->queryStatus(qid, devInfo);
      if (status == QS_SUCCESS) {

        /// If this device is not in a good state then skip it
        if (devInfo.devStatus != QDevStatus::QDS_READY)
          continue;

        /// Check metaversion.  Both the major and minor must match.
        if ((devInfo.devData.metaVerMaj != AIC_METADATA_MAJOR_VERSION) ||
            (devInfo.devData.metaVerMin != AIC_METADATA_MINOR_VERSION)) {

          LogErrorG(
              "Metadata Version mismatch: QID {}:  Device {}.{}: Host {}.{}",
              (uint32_t)devInfo.qid, (uint32_t)devInfo.devData.metaVerMaj,
              (uint32_t)devInfo.devData.metaVerMin, AIC_METADATA_MAJOR_VERSION,
              AIC_METADATA_MINOR_VERSION);
          incompDevCnt++;
          continue;
        }

        /// Check NNC major version.  Must match.
        if (devInfo.devData.nncCommandProtocolMajorVersion !=
            NNC_COMMAND_PROTOCOL_MAJOR_VERSION) {
          LogErrorG(
              "NNC Command Protocol Major Version mismatch: QID {}: Device "
              "Major Version {}: must be = {}",
              (uint32_t)devInfo.qid,
              (uint32_t)devInfo.devData.nncCommandProtocolMajorVersion,
              NNC_COMMAND_PROTOCOL_MAJOR_VERSION);
          incompDevCnt++;
          continue;
        }

        /// Fail if the device has an older minor version that the host
        uint16_t nncCommandProtocolMinorVersion =
            NNC_COMMAND_PROTOCOL_MINOR_VERSION;
        if (nncCommandProtocolMinorVersion >
            devInfo.devData.nncCommandProtocolMinorVersion) {
          LogWarnG(
              "NNC Command Protocol Minor Version smaller than required: QID "
              "{}: Device Minor Version: {}, must be >= {}",
              (uint32_t)devInfo.qid,
              (uint32_t)devInfo.devData.nncCommandProtocolMinorVersion,
              NNC_COMMAND_PROTOCOL_MINOR_VERSION);
          continue;
        }
      }
      liveDevCnt++;
    }
  }
  return (liveDevCnt != 0 && incompDevCnt == 0) ? (true) : (false);
}

} // namespace qaic
