// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QIRUNTIME_H
#define QIRUNTIME_H

#include "QAicRuntimeTypes.h"
#include "QActivationStateCmd.h"
#include "QRuntimePlatformInterface.h"
#include "QLogger.h"
#include "QNeuralNetworkInterface.h"
#include "QNNConstantsInterface.h"
#include "QNNImageInterface.h"

namespace qaic {

class QRuntimeInterface;
class QVirtualChannelInterface;
class QDeviceInterface;

// Providing a null pointer for logger is valid, it simply means
// that there is no callback to the application for logging
QRuntimeInterface *getRuntime(QLogInterface *logger = nullptr);

class QRuntimeInterface : virtual public QRuntimePlatformInterface,
                          virtual public QLogger {
public:
  /// Load a network image into a device. \p buf is the buffer that contains
  /// the network image. \p deviceID has the ID of the device. \p name is an
  /// optional name for the image. \p status is output parameter which has
  /// the result of the load. The return value is a pointer to the
  /// QNNImageInterface
  /// object. The pointer is valid only when \p status is QS_SUCCESS.
  [[nodiscard]] virtual QNNImageInterface *
  loadImage(QID deviceID, const QBuffer &buf, const char *name, QStatus &status,
            uint32_t metadataCRC = 0) = 0;

  /// Load a set of network constants into a device. \p buf is the buffer that
  /// contains the network constants. \p deviceID has the ID of the device.
  /// \p name is an optional name for the image. \p status is output parameter
  /// which has the result of the load. The return value is a pointer to the
  /// QNNConstantsInterface object. The pointer is valid only when \p status is
  /// QS_SUCCESS.
  [[nodiscard]] virtual QNNConstantsInterface *
  loadConstants(QID deviceID, const QBuffer &buf, const char *name,
                QStatus &status, uint32_t constantsCRC = 0) = 0;
  /// Ask device to set up memory
  [[nodiscard]] virtual QNNConstantsInterface *
  loadConstantsEx(QID deviceID, const QBuffer &constDescBuf, const char *name,
                  QStatus &status) = 0;

  /// Activate a network with preloaded \p image and \p constants.
  /// The \p status is an output parameter. The return value is a pointer
  /// to QNeuralNetworkInterface object. The pointer is valid only when status
  /// is
  /// QS_SUCCESS. Caller MUST NOT delete the pointer at the end of
  /// inference, but rather call QNeuralNetworkInterface::deactivate() instead.
  /// Optional parameters:
  [[nodiscard]] virtual QNeuralNetworkInterface *activateNetwork(
      QNNImageInterface *image, QNNConstantsInterface *constants,
      QStatus &status,
      QActivationStateType initialState = ACTIVATION_STATE_CMD_READY,
      uint32_t waitTimeoutMs =
          QAIC_PROGRAM_PROPERTIES_SUBMIT_TIMEOUT_MS_DEFAULT,
      uint32_t numMaxWaitRetries =
          QAIC_PROGRAM_PROPERTIES_SUBMIT_NUM_RETRIES_DEFAULT) = 0;

  [[nodiscard]] virtual QStatus sendActivationStateChangeCommand(
      QID deviceID,
      std::vector<std::pair<QNAID, QActivationStateType>> &stateCmdSet) = 0;

  [[nodiscard]] virtual bool isValidDevice(QID deviceID) = 0;
  [[nodiscard]] virtual QDeviceInterface *getDevice(QID deviceId) = 0;

  /// Query the status of one device as designated by \p deviceID. On a
  /// successful query the data  is in the form of QDevInfo.
  /// If \buf is too small QS_NOSPC will be returned, and buf.size
  /// contains the required buffer size.
  [[nodiscard]] virtual QStatus
  getResourceInfo(QID deviceID, QResourceInfo &resourceInfo) = 0;

  /// Query performance related info of one device as designated by
  /// \p deviceID. Upon success data is returned in the QPerformanceInfo
  /// structure.
  [[nodiscard]] virtual QStatus
  getPerformanceInfo(QID deviceID, QPerformanceInfo &performanceInfo) = 0;

  // Create a resource group reservation which can later be used to create an
  // activation reservation from these resources
  // Memory required can be specified as shDdrSize, l2RegionSize for networks
  // Plus ddrSize for loading network and constants
  // Caller should call releaseResourceReservation
  [[nodiscard]] virtual QStatus createResourceReservation(
      QID deviceID, uint16_t numNetworks, uint32_t numNsps, uint32_t numMcid,
      uint32_t numSem, uint32_t numVc, uint64_t shddrSize,
      uint64_t l2RegionSize, uint64_t ddrSize, QResourceReservationID &resResId,
      QResourceReservationID sourceReservationId) = 0;

  [[nodiscard]] virtual QStatus
  releaseResourceReservation(QID deviceID, QResourceReservationID resResId) = 0;

  /// Create a char device that corresponds to a valid reservation resResId on
  /// deviceID
  /// Creates a new device with the reserved resources if the operation is
  /// allowed.
  [[nodiscard]] virtual QStatus
  createResourceReservedDevice(QID deviceID, QResourceReservationID resResId) = 0;

  /// Create a char device that corresponds to a valid reservation resResId on
  /// deviceID
  /// Creates a new device with the reserved resources if the operation is
  /// allowed.
  [[nodiscard]] virtual QStatus
  releaseResourceReservedDevice(QID deviceID,
                                QResourceReservationID resResId) = 0;

  virtual ~QRuntimeInterface() = default;
};

/// Query the runtime version and place it in \p version.
void getRuntimeVersion(QRTVerInfo &version);

} // namespace qaic
#endif // QIRUNTIME_H
