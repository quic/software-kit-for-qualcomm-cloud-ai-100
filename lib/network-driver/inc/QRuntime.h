// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QRUNTIME_H
#define QRUNTIME_H

#include "QLogger.h"
#include "QRuntimePlatform.h"
#include "QNNConstantsInterface.h"
#include "QNNImageInterface.h"
#include "QNeuralNetworkInterface.h"
#include "QRuntimeInterface.h"

#include <map>
#include <memory>

namespace qaic {

class QDeviceInterface;
class QDeviceFactoryInterface;
class QImageParserInterface;
class QVirtualChannelInterface;

/// This class implements the QRuntimeInterface interface. All API functions
/// are implemented in this class.
class QRuntime : public QRuntimeInterface, public QRuntimePlatform {
public:
  /// The runtime is agnostic to driver backend implementation which is
  /// managed by \p devFactory. Network image parsing can also be
  /// customized through \p imgParser.
  QRuntime(std::unique_ptr<QDeviceFactoryInterface> devFactory,
           std::unique_ptr<QImageParserInterface> imgParser);

  QRuntime() = delete;
  QRuntime(const QRuntime &other) = delete;
  QRuntime &operator=(const QRuntime &other) = delete;
  ~QRuntime() = default;

  //   QStatus queryStatus(QID deviceID, QDevInfo *devInfo) override;
  [[nodiscard]] QNNImageInterface *loadImage(QID deviceID, const QBuffer &buf,
                                             const char *name, QStatus &status,
                                             uint32_t metadataCRC) override;
  [[nodiscard]] QNNConstantsInterface *
  loadConstants(QID deviceID, const QBuffer &buf, const char *name,
                QStatus &status, uint32_t constantsCRC) override;
  [[nodiscard]] QNNConstantsInterface *
  loadConstantsEx(QID deviceID, const QBuffer &constDescBuf, const char *name,
                  QStatus &status) override;
  [[nodiscard]] QNeuralNetworkInterface *activateNetwork(
      QNNImageInterface *image, QNNConstantsInterface *constants,
      QStatus &status,
      QActivationStateType initialState = ACTIVATION_STATE_CMD_READY,
      uint32_t waitTimeoutMs =
          QAIC_PROGRAM_PROPERTIES_SUBMIT_TIMEOUT_MS_DEFAULT,
      uint32_t numMaxWaitRetries =
          QAIC_PROGRAM_PROPERTIES_SUBMIT_NUM_RETRIES_DEFAULT) override;

  [[nodiscard]] QStatus sendActivationStateChangeCommand(
      QID deviceID,
      std::vector<std::pair<QNAID, QActivationStateType>> &stateCmdSet)
      override;

  static void getRuntimeVersion(QRTVerInfo &version);

  [[nodiscard]] bool isValidDevice(QID deviceID) override;
  [[nodiscard]] QDeviceInterface *getDevice(QID deviceId) override;

  [[nodiscard]] QStatus getResourceInfo(QID deviceID,
                                        QResourceInfo &resourceInfo) override;

  [[nodiscard]] QStatus
  getPerformanceInfo(QID deviceID, QPerformanceInfo &performanceInfo) override;

  [[nodiscard]] QStatus createResourceReservation(
      QID deviceID, uint16_t numNetworks, uint32_t numNsps, uint32_t numMcid,
      uint32_t numSem, uint32_t numVc, uint64_t l2RegionSize,
      uint64_t shddrSize, uint64_t ddrSize, QResourceReservationID &acResId,
      QResourceReservationID sourceReservationId) override;

  [[nodiscard]] QStatus
  releaseResourceReservation(QID deviceID,
                             QResourceReservationID acResId) override;

  [[nodiscard]] QStatus
  createResourceReservedDevice(QID deviceID, QResourceReservationID resResId) override;
  [[nodiscard]] QStatus
  releaseResourceReservedDevice(QID deviceID,
                                QResourceReservationID resResId) override;

private:
  std::unique_ptr<QDeviceFactoryInterface> devFactory_;
  std::unique_ptr<QImageParserInterface> imgParser_;
  [[nodiscard]] QStatus checkQueryStatusFormat(QDevInfo *devInfo,
                                               QDeviceInterface *dev);
};

} // namespace qaic

#endif // QRUNTIME_H
