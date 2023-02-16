// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QPRD_KMD_VC_FACTORY_H
#define QPRD_KMD_VC_FACTORY_H

#include "QVCFactoryInterface.h"
#include "QKmdVC.h"

namespace qaic {

class QKmdVCFactory : public QVCFactoryInterface {
public:
  std::unique_ptr<QVirtualChannelInterface, VCDeleter>
  makeVC(QID devID, QVCID vc, uint8_t *, uint8_t *, uint32_t qSizePow2,
         VCDeleter deleter, void *) override {
    return std::unique_ptr<QVirtualChannelInterface, VCDeleter>(
        new QKmdVC(devID, vc, qSizePow2), deleter);
  }
};

} // namespace qaic

#endif // QPRD_KMD_VC_FACTORY_H
