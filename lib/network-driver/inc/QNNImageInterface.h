// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QINN_IMAGE_H
#define QINN_IMAGE_H

#include "QAicRuntimeTypes.h"

namespace qaic {
/// This class contains information of a successfully loaded network image.
class QNNImageInterface {
public:
  /// \returns the QNNImageID of the loaded image.
  virtual QNNImageID getImageID() const = 0;

  /// \returns the QID of the device the network image is loaded into.
  virtual QID getQID() const = 0;

  /// Caller MUST NOT destroy the object through delete, but rather
  /// use the unload() method. After calling unload the object or the
  /// pointer to the object shall not be used again.
  virtual QStatus unload() = 0;

  virtual ~QNNImageInterface() = default;
};

} // namespace qaic
#endif // QINN_IMAGE_H