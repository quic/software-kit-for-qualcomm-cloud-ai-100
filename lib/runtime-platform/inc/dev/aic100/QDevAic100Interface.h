// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QDEV_AIC100_INTERFACE_H
#define QDEV_AIC100_INTERFACE_H

#include "QDevInterface.h"
#include "dev/aic100/qaic_accel.h"
#include "dev/aic100/QRuntimePlatformDeviceAic100.h"

namespace qaic {

class QDevInterfaceAic100 : public QDevInterface,
                            public QRuntimePlatformDeviceAic100 {
  friend class QDevInterface;

public:
  virtual ~QDevInterfaceAic100() = default;

  virtual QStatus freeMemReq(uint8_t *memReq, uint32_t numReqs) override;

protected:
  QDevInterfaceAic100(QID qid, const std::string &devName);
  virtual bool init() override;

private:
  static constexpr uint32_t invalidBdcId = -1U;
};
} // namespace qaic
#endif // QDEV_AIC100_INTERFACE_H
