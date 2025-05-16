// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QKMD_DEVICE_H
#define QKMD_DEVICE_H

#include "QDeviceInterface.h"
#include "QVCFactoryInterface.h"
#include "QLogger.h"
#include "QUtil.h"
#include "QMonitorNamedMutex.h"
#include "QDevInterface.h"
#include "QRuntimePlatformKmdDevice.h"

#include <atomic>
#include <unordered_map>
#include <bitset>

namespace qaic {

class QVirtualChannelInterface;

/// Implements the KMD device
class QKmdDevice : public QDeviceInterface,
                   virtual public QRuntimePlatformKmdDevice {
public:
  QKmdDevice(QID qid, std::shared_ptr<QVCFactoryInterface> vcFactory,
             const QPciInfo pciInfo, shQDevInterface deviceInterface,
             QDevInterfaceEnum devInterfaceType, std::string &path);

  // From QDeviceInterface
  [[nodiscard]] virtual QStatus loadImage(const QBuffer &buf, const char *name,
                                          QNNImageID &imageID,
                                          uint32_t metadataCRC) override;
  [[nodiscard]] virtual QStatus
  loadConstantsEx(const QBuffer &constDescBuf, const char *name,
                  QNNConstantsID &constantsID) override;
  [[nodiscard]] virtual QStatus
  loadConstantsAtOffset(QNNConstantsID constantsID, const QBuffer &buf,
                        uint64_t offset) override;
  [[nodiscard]] virtual QStatus unloadImage(QNNImageID imageID) override;
  [[nodiscard]] virtual QStatus loadConstants(const QBuffer &buf,
                                              const char *name,
                                              QNNConstantsID &constantsID,
                                              uint32_t constantsCRC) override;
  [[nodiscard]] virtual QStatus
  unloadConstants(QNNConstantsID constantsID) override;

  [[nodiscard]] virtual QStatus activate(
      QNNImageID imageID, QNNConstantsID constantsID, QNAID &naID,
      uint64_t &ddrBase, uint64_t &mcIDBase, QVirtualChannelInterface **vc,
      QActivationStateType initialState = ACTIVATION_STATE_CMD_READY) override;

  [[nodiscard]] virtual QStatus sendActivationStateChangeCommand(
      std::vector<std::pair<QNAID, QActivationStateType>> &stateCmdSet)
      override;

  [[nodiscard]] virtual QStatus createVC(QVCID vcID, int efd,
                                         QVirtualChannelInterface **vc,
                                         uint32_t qSizePow2);

  [[nodiscard]] virtual QStatus deactivate(uint8_t vc, QNAID naID,
                                           bool vc_deactivate = true) override;

  [[nodiscard]] virtual shQDevInterface &getDeviceInterface() override;

  [[nodiscard]] virtual QStatus
  getResourceInfo(QResourceInfo &resourceInfo) override;

  [[nodiscard]] virtual QStatus
  getPerformanceInfo(QPerformanceInfo &performanceInfo) override;

  [[nodiscard]] virtual QStatus createResourceReservation(
      uint16_t num_networks, uint32_t num_nsp, uint32_t num_mcid,
      uint32_t num_sem, uint32_t numVc, uint64_t shddr_size,
      uint64_t l2region_size, uint64_t ddrSize,
      QResourceReservationID &resResId,
      QResourceReservationID sourceReservationId) override;
  [[nodiscard]] virtual QStatus
  releaseResourceReservation(QResourceReservationID acResId) override;

  [[nodiscard]] virtual QStatus
  createResourceReservedDevice(QResourceReservationID resResId) override;
  [[nodiscard]] virtual QStatus
  releaseResourceReservedDevice(QResourceReservationID resResId) override;

  virtual ~QKmdDevice() = default;

protected:
  /// The mutex is used to protect the access to vcMap_
  std::mutex vcMapMutex_;

  /// Maps QVCIDs to virtual channels. Note that the virtual channels are
  /// owned by vcMap_.
  std::unordered_map<QVCID,
                     std::unique_ptr<QVirtualChannelInterface, VCDeleter>>
      vcMap_;
  // Protect access to device in case QAIC_SERIALIZE_RUNTIME_DEVICE is set
  std::unique_ptr<QMonitorNamedMutex> deviceAccessMtx_;
  bool deviceAccessMtxEnabled_;

private:
  QHostApiInfoNspVersion naNspData_; // NSP Data returned when not queried
  shQDevInterface deviceInterface_;
};

} // namespace qaic

#endif // QKMDDEVICE
