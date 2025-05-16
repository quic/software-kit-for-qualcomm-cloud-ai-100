// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QIDEVICE_FACTORY_H
#define QIDEVICE_FACTORY_H

#include "QAicRuntimeTypes.h"
#include "QRuntimePlatformKmdDeviceFactoryInterface.h"

#include <map>
#include <memory>

namespace qaic {

class QDeviceInterface;

class QDeviceFactoryInterface
    : virtual public QRuntimePlatformKmdDeviceFactoryInterface {
public:
  virtual QDeviceInterface *getDevice(QID qid) = 0;

  virtual ~QDeviceFactoryInterface() = default;
};

} // namespace qaic

#endif // QIDEVICE_FACTORY_H
