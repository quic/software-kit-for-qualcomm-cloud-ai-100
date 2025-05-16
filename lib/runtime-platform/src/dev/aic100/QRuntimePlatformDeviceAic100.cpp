// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
#include "dev/common/QRuntimePlatformDeviceInterface.h"
#include "dev/aic100/QRuntimePlatformDeviceAic100.h"
#include "QLogger.h"
#include "QNncProtocol.h"
#include "QOsal.h"

#include <sys/ioctl.h>
#include <iostream>

namespace qaic {

QRuntimePlatformDeviceAic100::QRuntimePlatformDeviceAic100(
    QID qid, const std::string &devName)
    : qid_(qid), devName_(devName) {}

shQRuntimePlatformDeviceInterface
QRuntimePlatformDeviceAic100::Factory(QID qid, const std::string &devName) {
  std::shared_ptr<QRuntimePlatformDeviceAic100> obj =
      std::make_shared<QRuntimePlatformDeviceAic100>(qid, devName);
  return obj;
}

bool QRuntimePlatformDeviceAic100::isDeviceValid() const {

  // If device is valid, its name should not contain invalidDevName_ string
  return (devName_.rfind(invalidDevName_) == std::string::npos);
}

QStatus QRuntimePlatformDeviceAic100::openDevice() {
  QStatus status = QS_SUCCESS;

  if (devName_.empty()) {
    if (QPciInfo_.has_value()) {
      QOsal::getDevicePath(devName_, QPciInfo_.value());
    } else {
      LogErrorG("Failed to open Qid {}, Invalid Dev link and PCI info", qid_);
      return QS_INVAL;
    }
  }

  if (!isDeviceValid()) {
    return QS_INVAL;
  }

  status = QOsal::openDevice(QAIC_DEV_INTERFACE_AIC100, devName_, devHandle_);

  if (status == QS_ERROR) {
    LogErrorG("Failed to open device {}", devName_.c_str());
  }

  return status;
}

QStatus QRuntimePlatformDeviceAic100::runDevCmd(QDevInterfaceCmdEnum cmd,
                                                const void *data) const {

  QDevCmd devCmd;
  QStatus status = QS_SUCCESS;

  /* KMD based on DRM interface structures */
  qaic_create_bo *createBO = nullptr;
  qaic_attach_slice *attachBO = nullptr;

  if (devHandle_.fileDev.fd == INVALID_FILE_DEV_HANDLE) {
    LogErrorG("Invalid file descriptor");
    return QS_BADFD;
  }

  switch (cmd) {
  case QAIC_DEV_CMD_MANAGE:
    devCmd.cmdReq = DRM_IOCTL_QAIC_MANAGE;
    devCmd.cmdSize = sizeof(manage_msg_t);
    devCmd.cmdType = QDEV_CMD_TYPE_WRITE_AND_READ;
    devCmd.cmdRspBuf = data;
    status = QOsal::runDeviceCmd(QAIC_DEV_INTERFACE_AIC100, devHandle_, devCmd);
    if (status != QS_SUCCESS) {
      LogErrorG("Failed to send manage command.");
      break;
    }
    break;
  case QAIC_DEV_CMD_MEM:
    /* Creat buffer object (BO) */
    createBO = (qaic_create_bo *)data;

    devCmd.cmdReq = DRM_IOCTL_QAIC_CREATE_BO;
    devCmd.cmdSize = sizeof(*createBO);
    devCmd.cmdType = QDEV_CMD_TYPE_WRITE_AND_READ;
    devCmd.cmdRspBuf = (void *)createBO;
    status = QOsal::runDeviceCmd(QAIC_DEV_INTERFACE_AIC100, devHandle_, devCmd);

    if (status != QS_SUCCESS) {
      LogErrorG("BO creation failed. Size {} pad {} Error {}", createBO->size, createBO->pad, status);
      break;
    }

    /* attach slicing information to BO */
    attachBO = reinterpret_cast<qaic_attach_slice *>(createBO + 1);
    attachBO->hdr.handle = createBO->handle;

    devCmd.cmdReq = DRM_IOCTL_QAIC_ATTACH_SLICE_BO;
    devCmd.cmdSize = sizeof(*attachBO);
    devCmd.cmdType = QDEV_CMD_TYPE_WRITE_ONLY;
    devCmd.cmdRspBuf = (void *)attachBO;
    status = QOsal::runDeviceCmd(QAIC_DEV_INTERFACE_AIC100, devHandle_, devCmd);
    break;
  case QAIC_DEV_CMD_MMAP_DEV:
    devCmd.cmdReq = DRM_IOCTL_QAIC_MMAP_BO;
    devCmd.cmdSize = sizeof(qaic_mmap_bo);
    devCmd.cmdType = QDEV_CMD_TYPE_WRITE_AND_READ;
    devCmd.cmdRspBuf = data;
    status = QOsal::runDeviceCmd(QAIC_DEV_INTERFACE_AIC100, devHandle_, devCmd);
    break;
  case QAIC_DEV_CMD_FREE_MEM:
    devCmd.cmdReq = DRM_IOCTL_GEM_CLOSE;
    devCmd.cmdSize = sizeof(drm_gem_close);
    devCmd.cmdType = QDEV_CMD_TYPE_WRITE_ONLY;
    devCmd.cmdRspBuf = data;
    status = QOsal::runDeviceCmd(QAIC_DEV_INTERFACE_AIC100, devHandle_, devCmd);
    break;
  case QAIC_DEV_CMD_EXECUTE:
    devCmd.cmdReq = DRM_IOCTL_QAIC_EXECUTE_BO;
    devCmd.cmdSize = sizeof(qaic_execute);
    devCmd.cmdType = QDEV_CMD_TYPE_WRITE_ONLY;
    devCmd.cmdRspBuf = data;
    status = QOsal::runDeviceCmd(QAIC_DEV_INTERFACE_AIC100, devHandle_, devCmd);
    break;
  case QAIC_DEV_CMD_WAIT_EXEC:
    devCmd.cmdReq = DRM_IOCTL_QAIC_WAIT_BO;
    devCmd.cmdSize = sizeof(qaic_wait);
    devCmd.cmdType = QDEV_CMD_TYPE_WRITE_ONLY;
    devCmd.cmdRspBuf = data;
    status = QOsal::runDeviceCmd(QAIC_DEV_INTERFACE_AIC100, devHandle_, devCmd);
    break;
  case QAIC_DEV_CMD_QUERY:
    devCmd.cmdReq = DRM_IOCTL_QAIC_PERF_STATS_BO;
    devCmd.cmdSize = sizeof(qaic_perf_stats);
    devCmd.cmdType = QDEV_CMD_TYPE_WRITE_AND_READ;
    devCmd.cmdRspBuf = data;
    status = QOsal::runDeviceCmd(QAIC_DEV_INTERFACE_AIC100, devHandle_, devCmd);
    break;
  case QAIC_DEV_CMD_PARTIAL_EXECUTE:
    devCmd.cmdReq = DRM_IOCTL_QAIC_PARTIAL_EXECUTE_BO;
    devCmd.cmdSize = sizeof(qaic_execute);
    devCmd.cmdType = QDEV_CMD_TYPE_WRITE_ONLY;
    devCmd.cmdRspBuf = data;
    status = QOsal::runDeviceCmd(QAIC_DEV_INTERFACE_AIC100, devHandle_, devCmd);
    break;
  case QAIC_DEV_CMD_PART_DEV:
    devCmd.cmdReq = DRM_IOCTL_QAIC_PART_DEV;
    devCmd.cmdSize = sizeof(qaic_part_dev);
    devCmd.cmdType = QDEV_CMD_TYPE_WRITE_ONLY;
    devCmd.cmdRspBuf = data;
    status = QOsal::runDeviceCmd(QAIC_DEV_INTERFACE_AIC100, devHandle_, devCmd);
    break;
  default:
    LogErrorG("Unhandled Ioctl requested cmd {}", cmd);
    status = QS_ERROR;
    break;
  }

  return status;
}

QStatus QRuntimePlatformDeviceAic100::closeDevice() {

  if (QOsal::closeDevice(QAIC_DEV_INTERFACE_AIC100, devHandle_) < 0) {
    return QS_ERROR;
  }
  /* Derived devices does not have valid PCI Info.
     We cannot Get dev link, if we clear it */
  if (QPciInfo_.has_value()) {
    devName_.clear();
    devHandle_.fileDev.fd = INVALID_FILE_DEV_HANDLE;
  }
  return QS_SUCCESS;
}

QStatus QRuntimePlatformDeviceAic100::reloadDevHandle() {
  QStatus status = QS_SUCCESS;

  status = closeDevice();
  if (status == QS_SUCCESS) {
    status = openDevice();
  }

  return status;
}

int QRuntimePlatformDeviceAic100::getDevFd() const {
  return devHandle_.fileDev.fd;
}

QStatus QRuntimePlatformDeviceAic100::getTelemetryInfo(
    QTelemetryInfo &telemetryInfo, const std::string &pciInfoString) const {
  return QOsal::getTelemetryInfo(telemetryInfo, pciInfoString);
}

} // namespace qaic
