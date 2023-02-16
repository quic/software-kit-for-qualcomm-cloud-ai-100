// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QDEV_INTERFACE_H
#define QDEV_INTERFACE_H

#include "QAicRuntimeTypes.h"
#include "dev/common/QRuntimePlatformDeviceInterface.h"

#include <memory>

namespace qaic {

class QDevInterface;
using shQDevInterface = std::shared_ptr<QDevInterface>;

class QDevInterface : virtual public QRuntimePlatformDeviceInterface {
public:
  static shQDevInterface Factory(QDevInterfaceEnum qDevInterfaceEnum, QID qid,
                                 const std::string &devName);

  // All memory allocations created by MEM_REQ IOCTL must be released
  // through FreeMemReq call
  virtual QStatus freeMemReq(uint8_t *memReq, uint32_t numReqs) = 0;

protected:
  virtual bool init() = 0;
};
} // namespace qaic
#endif
