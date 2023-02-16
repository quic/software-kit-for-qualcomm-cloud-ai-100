// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QIMETADATA_H
#define QIMETADATA_H

#include "QDmaElement.h"
#include "QAicRuntimeTypes.h"

#include <memory>
#include <vector>

namespace qaic {

class QMetaDataInterface {
public:
  /// \returns a clone of this metadata but with updated DDR and MCID base
  /// addresses
  virtual std::unique_ptr<QMetaDataInterface>
  getUpdatedMetadata(uint64_t ddrBase, uint64_t mcIdBase) const = 0;

  /// Update DMA request elements with request ID starting at \p reqID.
  /// \p bufs points to a list of \p bufCount input buffers.
  /// The \p elems vecotr has the addresses of the request elements.
  /// \p lastReqID tracks the request ID of the last output buffer.
  virtual QStatus updateDmaReqElements(QReqID &reqID, const QBuffer *bufs,
                                       uint32_t bufCount,
                                       std::vector<uint8_t *> &elems,
                                       QReqID &lastReqID) const = 0;

  /// Get the count of request elements per inference
  virtual uint32_t getReqElementsCount() const = 0;

  virtual ~QMetaDataInterface() = default;
};

} // namespace qaic

#endif // QIMETADATA_H
