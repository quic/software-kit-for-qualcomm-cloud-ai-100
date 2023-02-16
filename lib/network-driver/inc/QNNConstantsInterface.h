// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QINN_CONSTANTS_H
#define QINN_CONSTANTS_H

#include "QAicRuntimeTypes.h"

namespace qaic {
/// This class contains information of a successfully loaded network constants.
class QNNConstantsInterface {
public:
  /// \returns the QNNImageID of the loaded network constants.
  virtual QNNConstantsID getConstantsID() const = 0;

  /// \returns the QID of the device the network constants are loaded into.
  virtual QID getQID() const = 0;

  /// Caller MUST NOT destroy the object through delete, but rather
  /// use the unload() method. After calling unload the object or the
  /// pointer to the object shall not be used again.
  virtual QStatus unload() = 0;

  /// Load a segment of constants at specified offset.
  virtual QStatus loadConstantsAtOffset(const QBuffer &buf,
                                        uint64_t offset) = 0;

  virtual ~QNNConstantsInterface() = default;
};

} // namespace qaic
#endif // QINN_CONSTANTS_H