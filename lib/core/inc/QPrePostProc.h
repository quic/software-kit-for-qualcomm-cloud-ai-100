// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QPREPOSTPROCESS_H
#define QPREPOSTPROCESS_H

#include "AICNetworkDesc.pb.h"
#include "PrePostProc.h"

namespace qaic {
class QContext;
using shQContext = std::shared_ptr<QContext>;

class QPrePostProc {
public:
  virtual ~QPrePostProc() = default;
  QPrePostProc(const aicnwdesc::networkDescriptor *nwDesc, shQContext context);

  // DMA buffer size is 0 if the tensor is not partial
  QStatus processInputBuffers(const aicppp::BufferBindings &bufferBindings,
                              std::vector<uint64_t> &dmaBufferSizes) const;
  QStatus
  processOutputBuffers(const aicppp::BufferBindings &bufferBindings) const;
  QStatus validateTransformKind();

private:
  std::unique_ptr<aicppp::PrePostProcessor> ppp_;
  const aicnwdesc::networkDescriptor *nwDesc_;
  shQContext context_;
};

} // namespace qaic

#endif // QPREPOSTPROCESS_H
