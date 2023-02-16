// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QIVC_FACTORY_H
#define QIVC_FACTORY_H

#include "QAicRuntimeTypes.h"
#include "QTypes.h"

#include <functional>
#include <memory>

namespace qaic {

class QVirtualChannelInterface;

using VCDeleter = std::function<void(QVirtualChannelInterface *)>;

/// Produces QVirtualChannelInterface instances.
class QVCFactoryInterface {
public:
  /// Create a virtual channel object.
  /// \p devID  QID of the device
  /// \p vc  ID of the virtual channel
  /// \p reqQBase base address of the request queue
  /// \p respQBase base address of the response queue
  /// \p qSizePow2 size of queues (both request and response) in the power of 2
  /// \p vcData data specfic to the VC
  ///
  virtual std::unique_ptr<QVirtualChannelInterface, VCDeleter>
  makeVC(QID devID, QVCID vc, uint8_t *reqQBase, uint8_t *respQBase,
         uint32_t qSizePow2, VCDeleter deleter, void *vcData) = 0;

  virtual ~QVCFactoryInterface() = default;
};

} // namespace qaic

#endif // QIVC_FACTORY_H
