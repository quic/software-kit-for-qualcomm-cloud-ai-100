// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QOS_BUFFER_H
#define QOS_BUFFER_H

#include "QAicRuntimeTypes.h"
#include "QIBuffer.h"
#include <memory>
namespace qaic {

class QOsBuffer : public QIBuffer {
public:
  QOsBuffer(const QBuffer &buf);
  virtual ~QOsBuffer() = default;
  static QIBuffer *create(size_t size);

private:
  QOsBuffer(std::unique_ptr<uint8_t[]> buf, size_t size);
  std::unique_ptr<uint8_t[]> buf_;
};

} // namespace qaic

#endif // QOS_BUFFER_H
