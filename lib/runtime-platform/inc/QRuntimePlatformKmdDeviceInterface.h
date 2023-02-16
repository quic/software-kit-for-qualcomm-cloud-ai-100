// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QRUNTIME_PLATFORM_KMD_DEVICE_INTERFACE_H
#define QRUNTIME_PLATFORM_KMD_DEVICE_INTERFACE_H

#include "QAicRuntimeTypes.h"
#include "QLogger.h"
#include "QUtil.h"
#include "QNncProtocol.h"
#include "dev/common/QRuntimePlatformDeviceInterface.h"

#include <atomic>
#include <memory>
#include <bitset>

namespace qaic {

const QID qidPlatformNspDeviceBase = 1000;

/// \mainpage AIC Runtime Library
/// The AIC Runtime library is mainly composed of three layers. The top layer is
/// the API
/// layer which directly interacts with user applications. The main classes in
/// this layer are QRuntime for control path, and QNeuralNetwork for data path.
///
/// The bottom layer is the Driver layer which interfaces with
/// different driver backends. Four driver backends are planned at this time:
/// KMD (Kernel Mode Driver), UMD (User Mode Driver), Loopback and QSIM.
///
/// The middle layer is the Common layer which keeps the common functionality
/// for different driver implementations. For example, the parsing of
/// metadata and generation of request queue elements are both in this layer.
///
/// For the ease of description the API and Common layers are referred to
/// as the "frontend", while the Driver layer is referred to as the "backend".
///
/// The control path data exchange between frontend and backend is through
/// function calls. The data path data exchange between frontend and backend
/// is through request/response queues identical to DMA bridge hardware,
/// with the queues managed through QVirtualChannelInterface interface. The
/// queues
/// are provisioned by the drver backend at network activation.
///
/// The QRuntimePlatformKmdDeviceInterface class is the interface to be
/// implemented by the driver
/// backend. Most of the virtual functions in this class are for control
/// path. Data path related functions are in the implementation of
/// QVirtualChannelInterface class.
///

class QRuntimePlatformKmdDeviceInterface {
public:
  /// \p vcFactory produces virtual channels. One VC factory can be shared
  /// among multiple devices.
  QRuntimePlatformKmdDeviceInterface(QID qid, const std::string &devName)
      : devID_(qid), devStatus_(QDevStatus::QDS_INITIALIZING),
        devName_(devName) {}

  virtual QStatus queryStatus(QDevInfo &devInfo) = 0;

  /// \p fd is the file descriptor for the control channel.
  virtual QStatus openCtrlChannel(QCtrlChannelID ccID, int &fd) = 0;
  virtual QStatus closeCtrlChannel(int fd) = 0;

  /// Returns the ID of the device, and a string identifying it's address
  /// In the case of A PICe device, this is the Pcie Address
  virtual QStatus getDeviceAddrInfo(qutil::DeviceAddrInfo &devInfo) = 0;

  virtual QStatus getBoardInfo(QBoardInfo &boardInfo) = 0;
  virtual QStatus getTelemetryInfo(QTelemetryInfo &telemetryInfo) = 0;

  virtual const QPciInfo *getDevicePciInfo() = 0;

  virtual QStatus isDeviceFeatureEnabled(qutil::DeviceFeatures feature,
                                         bool &isEnabled) const = 0;

  virtual QStatus getDeviceFeatures(QDeviceFeatureBitset &features) const = 0;

  /// Return the interface type
  virtual QDevInterfaceEnum getDeviceInterfaceType() const = 0;
  virtual shQRuntimePlatformDeviceInterface &
  getQRuntimePlatformDeviceInterface() = 0;

  QID getID() const { return devID_; }

  void setDeviceStatus(QDevStatus status) { devStatus_ = status; }
  QDevStatus getDeviceStatus() { return devStatus_; }

  const std::string &getDeviceName() { return devName_; }

  virtual QStatus getDeviceTimeOffset(uint64_t &deviceTimeOffset) = 0;

  virtual ~QRuntimePlatformKmdDeviceInterface() = default;

protected:
  QID devID_;
  std::atomic<QDevStatus> devStatus_;
  std::string devName_;
};

} // namespace qaic

#endif // QRUNTIME_PLATFORM_KMD_DEVICE_INTERFACE_H
