// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QIDEVICE_H
#define QIDEVICE_H

#include "QAicRuntimeTypes.h"
#include "QNncProtocol.h"
#include "QRuntimePlatformKmdDeviceInterface.h"
#include "QDevInterface.h"
#include "QActivationStateCmd.h"

#include <atomic>
#include <memory>
#include <bitset>

namespace qaic {

class QVirtualChannelInterface;
class QVCFactoryInterface;

class QDeviceInterface : virtual public QRuntimePlatformKmdDeviceInterface {
public:
  /// \p vcFactory produces virtual channels. One VC factory can be shared
  /// among multiple devices.
  QDeviceInterface(QID qid, std::shared_ptr<QVCFactoryInterface> vcFactory,
                   uint8_t nsp, uint32_t ddrs, const std::string &devName)
      : QRuntimePlatformKmdDeviceInterface(qid, devName), vcFactory_(vcFactory),
        nspCount_(nsp), ddrSizeMB_(ddrs) {}

  [[nodiscard]] virtual QStatus loadImage(const QBuffer &buf, const char *name,
                                          QNNImageID &imageID,
                                          uint32_t metadataCRC) = 0;
  [[nodiscard]] virtual QStatus unloadImage(QNNImageID imageID) = 0;
  [[nodiscard]] virtual QStatus loadConstants(const QBuffer &buf,
                                              const char *name,
                                              QNNConstantsID &constantsID,
                                              uint32_t constantsCRC) = 0;
  [[nodiscard]] virtual QStatus
  loadConstantsEx(const QBuffer &nwElfbuf, const char *name,
                  QNNConstantsID &constantsID) = 0;
  [[nodiscard]] virtual QStatus
  loadConstantsAtOffset(QNNConstantsID constantsID, const QBuffer &buf,
                        uint64_t offset) = 0;
  [[nodiscard]] virtual QStatus unloadConstants(QNNConstantsID constantsID) = 0;
  ///\p naID and \p vc are output parameters. After successful activation,
  /// a QVirtualChannelInterface object will be returned to caller through \p
  /// vc.
  [[nodiscard]] virtual QStatus
  activate(QNNImageID imageID, QNNConstantsID constantsID, QNAID &naID,
           uint64_t &ddrBase, uint64_t &mcIDBase, QVirtualChannelInterface **vc,
           QActivationStateType initialState = ACTIVATION_STATE_CMD_READY) = 0;
  [[nodiscard]] virtual QStatus sendActivationStateChangeCommand(
      std::vector<std::pair<QNAID, QActivationStateType>> &stateCmdSet) = 0;

  [[nodiscard]] virtual QStatus deactivate(QVCID vc, QNAID naID,
                                           bool vc_deactivate = true) = 0;

  [[nodiscard]] virtual shQDevInterface &getDeviceInterface() = 0;

  [[nodiscard]] QID getID() const { return devID_; }

  void setDeviceStatus(QDevStatus status) { devStatus_ = status; }
  [[nodiscard]] QDevStatus getDeviceStatus() { return devStatus_; }

  [[nodiscard]] virtual QStatus
  getResourceInfo(QResourceInfo &resourceInfo) = 0;

  [[nodiscard]] virtual QStatus
  getPerformanceInfo(QPerformanceInfo &performanceInfo) = 0;

  [[nodiscard]] const std::string &getDeviceName() { return devName_; }

  [[nodiscard]] virtual QStatus
  createResourceReservation(uint16_t num_networks, uint32_t num_nsp,
                            uint32_t num_mcid, uint32_t num_sem, uint32_t numVc,
                            uint64_t shddr_size, uint64_t l2region_size,
                            uint64_t ddrSize, QResourceReservationID &resResId,
                            QResourceReservationID sourceReservationId) = 0;
  [[nodiscard]] virtual QStatus
  releaseResourceReservation(QResourceReservationID resResId) = 0;

  [[nodiscard]] virtual QStatus
  createResourceReservedDevice(QResourceReservationID resResId) = 0;

  [[nodiscard]] virtual QStatus
  releaseResourceReservedDevice(QResourceReservationID resResId) = 0;

  virtual ~QDeviceInterface() = default;

protected:
  std::shared_ptr<QVCFactoryInterface> vcFactory_;
  uint8_t nspCount_;
  uint32_t ddrSizeMB_;
};

} // namespace qaic

#endif // QIDEVICE_H
