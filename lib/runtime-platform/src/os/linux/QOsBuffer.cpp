// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QOsBuffer.h"
#include "QUtil.h"

namespace qaic {

QOsBuffer::QOsBuffer(std::unique_ptr<uint8_t[]> buf, size_t size)
    : QIBuffer({size, buf.get()}), buf_(std::move(buf)){};

QOsBuffer::QOsBuffer(const QBuffer &buf) : QIBuffer(buf) {}

QIBuffer *QOsBuffer::create(size_t size) {
  auto buf = std::unique_ptr<uint8_t[]>(new uint8_t[size]);
  if (buf == nullptr) {
    return nullptr;
  }
  return new QOsBuffer(std::move(buf), size);
}

} // namespace qaic
