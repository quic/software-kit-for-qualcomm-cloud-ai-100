// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QIBUFFER_H
#define QIBUFFER_H

#include "QAicRuntimeTypes.h"

namespace qaic {

class QIBuffer {
public:
  QIBuffer(const QBuffer &buf) : qBuffer_(buf){};
  virtual ~QIBuffer() = default;
  const QBuffer &get() { return qBuffer_; };
  size_t size() const { return qBuffer_.size; }
  uint8_t *data() const { return qBuffer_.buf; }

protected:
  QBuffer qBuffer_;
};

} // namespace qaic

#endif // QIBUFFER_H
