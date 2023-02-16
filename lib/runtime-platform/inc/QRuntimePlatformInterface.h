// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QRUNTIME_PLATFORM_INTERFACE_H
#define QRUNTIME_PLATFORM_INTERFACE_H

#include "QAicRuntimeTypes.h"
#include "QLogger.h"
#include "QUtil.h"

#include <utility>
#include <string>

namespace qaic {

class QRuntimePlatformInterface;
using shQRuntimePlatformInterface = std::shared_ptr<QRuntimePlatformInterface>;

class QRuntimePlatformInterface : virtual public QLogger {
public:
  /// @brief  Retrieve a vector of valid Device Ids for all
  /// available AIC devices discovered on PCIe bus
  /// @param devices
  /// @return QS_SUCCESS if valid devices are found
  /// QS_ERROR if no devices are found
  virtual QStatus getDeviceIds(std::vector<QID> &devices) = 0;

  virtual QStatus queryStatus(QID deviceID, QDevInfo &devInfo) = 0;

  /// Open a control channel of a device with QID \p deviceID.
  /// \p ctrlChannelID ID of the channel to be opened
  /// \p status [output] result of the operation, QS_SUCCESS for
  //   success
  /// \returns file descriptor number of the channel if success.
  virtual int openCtrlChannel(QID deviceID, QCtrlChannelID ctrlChannelID,
                              QStatus &status) = 0;
  /// Close the file descriptor opened in openCtrlChannel.
  /// \p handle returned in openCtrlChannel.
  /// \returns QS_SUCCESS for success.
  virtual QStatus closeCtrlChannel(int handle) = 0;

  // Get list of devices and their identifying string
  // In the case of a PCIe device, this will be the address of the PCIe device
  // as a string
  virtual QStatus getDeviceAddrInfo(
      std::vector<qutil::DeviceAddrInfo> &deviceAddrInfoList) const = 0;

  virtual QStatus isDeviceFeatureEnabled(QID deviceID,
                                         qutil::DeviceFeatures feature,
                                         bool &isEnabled) const = 0;

  /// Query static board info of one device as designated by
  /// \p deviceID. Upon success data is returned in the QBoardInfo
  /// structure.
  virtual QStatus getBoardInfo(QID deviceID, QBoardInfo &boardInfo) const = 0;
  virtual QStatus getTelemetryInfo(QID deviceID,
                                   QTelemetryInfo &telemetryInfo) const = 0;

  // Get the device link address for a device
  virtual QStatus getDeviceName(QID deviceID, std::string &devName) const = 0;

  /// Query the Hardware Pci Info of one device as designated by deviceID
  virtual const QPciInfo *getDevicePciInfo(QID deviceID) const = 0;

  virtual ~QRuntimePlatformInterface() = default;
};

} // namespace qaic
#endif // QRUNTIME_PLATFORM_INTERFACE_H
