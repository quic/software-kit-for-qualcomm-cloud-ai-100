// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef PREPOSTPROC_H
#define PREPOSTPROC_H

#include <assert.h>
#include <memory>
#include <vector>

namespace aicnwdesc {
class networkDescriptor;
}

constexpr uint8_t BINDINGS_MAX_DIMENSIONS = 8;

namespace aicppp {
struct BufferBinding {
  char *ptr;
  size_t size;
};

struct BufferBindings {
  std::vector<BufferBinding> userBindings;
  std::vector<BufferBinding> dmaBindings;
  size_t getDMABufferSize(int bindingNum) const {
    assert(bindingNum < (int)dmaBindings.size());
    return dmaBindings[bindingNum].size;
  }
  char *getDMABufferRaw(int bindingNum) const {
    assert(bindingNum < (int)dmaBindings.size());
    return dmaBindings[bindingNum].ptr;
  }
};

// This is one of the binary interfaces between compiler and LRT.
// Please coordinate with relevant parties before changing it.
struct DynamicSliceInfo {
  uint32_t dynamicOffset;
  uint32_t size;
};

struct DynamicBufferBinding {
  size_t size;    // Size of input DMA buffer.
  bool isPartial; // When set input is a partial DMA input.
};

struct DynamicBufferBindings {
  std::vector<DynamicBufferBinding> dmaBindings;
};

// Pre- and post- process initial/final computations that were hoisted out of
// the graph.
class PrePostProcessor {
public:
  static std::unique_ptr<PrePostProcessor>
  create(const aicnwdesc::networkDescriptor *nwDesc);
  virtual ~PrePostProcessor() {}

  // Compute the size of the DMA buffers based on the actual size of the inputs.
  virtual bool
  computeDynamicBindings(const BufferBindings &bindings,
                         DynamicBufferBindings &dynamicBindings) = 0;

  virtual void preProcessInputs(const BufferBindings &bindings) = 0;
  virtual void postProcessOutputs(const BufferBindings &bindings) = 0;
};
} // namespace aicppp

#endif // PREPOSTPROC_H
