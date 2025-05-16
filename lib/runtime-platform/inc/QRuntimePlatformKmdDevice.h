// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QRUNTIME_PLATFORM_KMD_DEVICE_H
#define QRUNTIME_PLATFORM_KMD_DEVICE_H

#include "QRuntimePlatformKmdDeviceInterface.h"
#include "QLogger.h"
#include "QUtil.h"
#include "dev/common/QRuntimePlatformDeviceInterface.h"

#include <atomic>
#include <unordered_map>
#include <bitset>

namespace qaic {

/// Implements the KMD device
class QRuntimePlatformKmdDevice
    : virtual public QRuntimePlatformKmdDeviceInterface,
      virtual public QLogger {
public:
  QRuntimePlatformKmdDevice(QID qid, const QPciInfo pciInfo,
                            shQRuntimePlatformDeviceInterface deviceInterface,
                            QDevInterfaceEnum devInterfaceType,
                            std::string &devName);

  // From QDeviceInterface
  virtual QStatus queryStatus(QDevInfo &devInfo) override;

  virtual QStatus openCtrlChannel(QCtrlChannelID ccID, int &fd) override;
  virtual QStatus closeCtrlChannel(int fd) override;
  virtual QStatus getDeviceAddrInfo(qutil::DeviceAddrInfo &devInfo) override;
  virtual QDevInterfaceEnum getDeviceInterfaceType() const override;
  virtual shQRuntimePlatformDeviceInterface &
  getQRuntimePlatformDeviceInterface() override;

  virtual QStatus isDeviceFeatureEnabled(qutil::DeviceFeatures feature,
                                         bool &isEnabled) const override;
  virtual QStatus
  getDeviceFeatures(QDeviceFeatureBitset &features) const override;
  virtual QStatus getBoardInfo(QBoardInfo &boardInfo) override;
  virtual QStatus getTelemetryInfo(QTelemetryInfo &telemetryInfo) override;
  virtual QStatus getDeviceTimeOffset(uint64_t &deviceTimeOffset) override;
  virtual const QPciInfo *getDevicePciInfo() override;
  virtual ~QRuntimePlatformKmdDevice() = default;
  static const char *nncErrorStr(int status);

protected:
  const QPciInfo pciInfo_;
  const std::string pciInfoString_;

  QStatus getStatus(QDevInfo &info);
  QStatus statusQuery(manage_msg_t &nncMsg,
                      nnc_cmd_status_query_response_t *&statusQueryResponse);
  QStatus firmwarePropertiesQuery();
  bool isValid() const;

private:
  QRuntimePlatformKmdDevice() = delete;

  shQRuntimePlatformDeviceInterface qRuntimePlatformDeviceInterface_;
  QDevInterfaceEnum deviceInterfaceType_;
  QDeviceFeatureBitset deviceFeatures_;
  uint64_t deviceTimeOffset_;
};

} // namespace qaic

#endif // QKMDDEVICE
