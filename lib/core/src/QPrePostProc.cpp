// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include <typeinfo>
#include <iostream>
#include <fstream>

#include "QAic.h"
#include "QAicRuntimeTypes.h"
#include "QPrePostProc.h"
#include "QBindingsParser.h"
#include "QContext.h"
#include "PrePostProc.h"
#include "QLogger.h"

#include "spdlog/spdlog.h"

namespace qaic {

QPrePostProc::QPrePostProc(const aicnwdesc::networkDescriptor *nwDesc,
                           shQContext context)
    : context_(context) {
  nwDesc_ = nwDesc;
  ppp_ = aicppp::PrePostProcessor::create(nwDesc_);
}

QStatus
QPrePostProc::processInputBuffers(const aicppp::BufferBindings &bufferBindings,
                                  std::vector<uint64_t> &dmaBufferSizes) const {
  if ((bufferBindings.dmaBindings.size() == 0) ||
      (bufferBindings.userBindings.size() == 0)) {
    LogErrorG("Invalid bindings User Binding Size:{}, Dma Binding Size:{}",
              bufferBindings.userBindings.size(),
              bufferBindings.dmaBindings.size());
    return QS_ERROR;
  }

  aicppp::DynamicBufferBindings dynamicBufBindings;
  if (!ppp_->computeDynamicBindings(bufferBindings, dynamicBufBindings)) {
    LogErrorG("Failed to compute dynamic binding, likely due to invalid buffer "
              "dimensions");
    return QS_INVAL;
  }

  for (const auto &x : dynamicBufBindings.dmaBindings) {
    dmaBufferSizes.emplace_back(x.isPartial ? x.size : 0);
  }

  ppp_->preProcessInputs(bufferBindings);

  return QS_SUCCESS;
}

QStatus QPrePostProc::processOutputBuffers(
    const aicppp::BufferBindings &bufferBindings) const {
  ppp_->postProcessOutputs(bufferBindings);
  return QS_SUCCESS;
}

QStatus QPrePostProc::validateTransformKind() {
  QStatus status = QS_INVAL;

  auto getEnumString = [&] (const google::protobuf::EnumDescriptor *descriptor,
                          int enumValue) {
    return descriptor->FindValueByNumber(enumValue)->name();
  };

  auto &inputs = nwDesc_->inputs();
  for (int ii = 0, ie = nwDesc_->inputs_size(); ii < ie; ++ii) {
    auto &input = inputs[ii];
    // Ignore inputs that have been optimized away.
    if (input.transformseq_size() == 0)
      continue;

    if (input.transformseq_size() > 1) {
      LogErrorG("Input Buffer: {} UNSUPPORTED pre-processing Transform_sequence size:[{}] Expected:[1]",
                 input.name(), input.transformseq_size());
      return status;
    }

    // only copyDMATransform is suported for now
    auto &copyDMA = input.transformseq()[0];
    if(copyDMA.kind() != aicnwdesc::CopyDMABufferTransform) {
      LogErrorG("Input Buffer: {} UNSUPPORTED pre-processing Transform_kind:[{}] Expected:[CopyDMABufferTransform]",
                 input.name(), getEnumString(aicnwdesc::transformKind_descriptor(),
                                copyDMA.kind()));
      return status;
    }
  }

  auto &outputs = nwDesc_->outputs();
  for (int ii = 0, ie = nwDesc_->outputs_size(); ii < ie; ++ii) {
    auto &output = outputs[ii];
    // Ignore outputs that have been optimized away.
    if (output.transformseq_size() == 0)
      continue;

    if (output.transformseq_size() > 1) {
      LogErrorG("Output Buffer: {} UNSUPPORTED post-processing Transform_sequence size:[{}] Expected:[1]",
                 output.name(), output.transformseq_size());
      return status;
    }

    // only copyDMATransform is suported for now
    auto &copyDMA = output.transformseq()[0];
    if(copyDMA.kind() != aicnwdesc::CopyDMABufferTransform) {
      LogErrorG("Output Buffer: {} UNSUPPORTED post-processing Transform_kind:[{}] Expected:[CopyDMABufferTransform]",
                 output.name(), getEnumString(aicnwdesc::transformKind_descriptor(),
                                copyDMA.kind()));
      return status;
    }
  }
  return QS_SUCCESS;
}

} // namespace qaic
