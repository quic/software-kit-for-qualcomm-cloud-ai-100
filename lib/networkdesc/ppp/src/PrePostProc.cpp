// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "PrePostProc.h"

#include "AICNetworkDesc.pb.h"

#include <inttypes.h>
#include <list>

using namespace aicnwdesc;
using google::protobuf::RepeatedField;
using std::vector;

#ifndef PPP_CORE
#define PPP_CORE Default
#define PPP_CORE_DEFAULT
#endif

#define CONCAT2(A, B) A##B
#define CONCAT(A, B) CONCAT2(A, B)
#define PPP_CLASS CONCAT(PrePostProcessor, CONCAT(PPP_CORE, Impl))
#define PPP_CREATEFN CONCAT(create, PPP_CLASS)

//#define AICPPP_DEBUG_ENABLED

#ifdef AICPPP_DEBUG_ENABLED
#define AICPPP_DEBUG(X)                                                        \
  do {                                                                         \
    X;                                                                         \
  } while (0)
#else
#define AICPPP_DEBUG(X)
#endif

namespace aicppp {

namespace PPP_CORE {

class PPP_CLASS;

static bool TimingMode = false;

struct ScopedTimer {
  ScopedTimer(const char *name) : name_(name) {
    if (TimingMode) {
      startUS_ = getTimestampUS();
    }
  }
  ~ScopedTimer() {
    if (TimingMode) {
      endUS_ = getTimestampUS();
      printf("%s time: %" PRIu64 " us\n", name_, endUS_ - startUS_);
    }
  }

  static uint64_t getTimestampUS() {
    uint64_t ts = std::chrono::duration_cast<std::chrono::microseconds>(
                      std::chrono::steady_clock::now().time_since_epoch())
                      .count();
    return ts;
  }

  const char *name_{nullptr};
  uint64_t startUS_{0};
  uint64_t endUS_{0};
};

static size_t getTypeSize(dataType type) {
  switch (type) {
  case FloatTy:
    return 4;
  case Float16Ty:
    return 2;
  case Int8QTy:
    return 1;
  case UInt8QTy:
    return 1;
  case Int16QTy:
    return 2;
  case Int32QTy:
    return 4;
  case Int32ITy:
    return 4;
  case Int64ITy:
    return 8;
  case Int8Ty:
    return 1;
  default:
    assert(!"unexpected type");
    return 1;
  }
}

struct Binding {
  enum Kind { User, DMA, Temp };
  Binding() : buf(nullptr), kind(Temp), num(-1), offset(0) {}
  Binding(Kind kind, int num, int off)
      : buf(nullptr), kind(kind), num(num), offset(off) {}
  char *buf;
  Kind kind;
  // index into input/output array or DMA buffer array depending on kind
  int num;
  // For packed partial inputs, byte offset into DMA buffer header containing
  // dynamic offset/size.  Otherwise, static byte offset into DMA buffer.
  int offset;
};

class Tensor {
public:
  Tensor() : type_(FloatTy), scale_(0.0), offset_(0), layout_(FlatNXYD) {}
  Tensor(dataType type, float scale, int32_t offset, networkDescLayout layout,
         const RepeatedField<int> *dims, int tempNum)
      : type_(type), scale_(scale), offset_(offset), layout_(layout),
        dims_(dims->size()), buf_(Binding::Temp, tempNum, 0) {
    for (int i = 0, e = dims->size(); i < e; ++i)
      dims_[i] = dims->Get(i);
  }

  Tensor(const ::IODescriptor &iodesc, Binding::Kind kind, int bufNum,
         int bufOffset)
      : type_(iodesc.type()), scale_(iodesc.qscale()),
        offset_(iodesc.qoffset()), layout_(iodesc.layout()),
        dims_(iodesc.dims().size()), buf_(kind, bufNum, bufOffset) {
    for (int i = 0, e = iodesc.dims().size(); i < e; ++i)
      dims_[i] = iodesc.dims()[i];
  }

  Tensor(const ::IODescriptor &iodesc, int tempNum)
      : Tensor(iodesc.type(), iodesc.qscale(), iodesc.qoffset(),
               iodesc.layout(), &iodesc.dims(), tempNum) {}

  int getNumElts() const {
    int elts = 1;
    for (auto d : dims_)
      elts *= d;
    return elts;
  }

  size_t getSizeInBytes() const { return getNumElts() * getTypeSize(type_); }

  char *getBufferRaw(PPP_CLASS *ppp);
  const char *getBufferRaw(PPP_CLASS *ppp) const {
    return const_cast<Tensor *>(this)->getBufferRaw(ppp);
  }

  size_t getBufferRawSize(PPP_CLASS *ppp) const;

  template <class T> T *getBuffer(PPP_CLASS *ppp) {
    assert(sizeof(T) == getTypeSize(type_));
    return (T *)getBufferRaw(ppp);
  }
  template <class T> const T *getBuffer(PPP_CLASS *ppp) const {
    assert(sizeof(T) == getTypeSize(type_));
    return (T *)getBufferRaw(ppp);
  }

  void allocIfNeeded(PPP_CLASS *ppp);

  dataType type_;
  float scale_;
  int32_t offset_;
  networkDescLayout layout_;
  std::vector<int> dims_;
  Binding buf_;
};

class Transform {
public:
  virtual ~Transform() {}
  virtual void run(PPP_CLASS *ppp) = 0;

  Transform(Tensor dst, Tensor src, bool constantNumDims)
      : dstT_(dst), srcT_(src), constantNumDims_(constantNumDims) {}
  Tensor dstT_;
  Tensor srcT_;
  // Does this transform change the tensor dims?
  bool constantNumDims_;
};

class PPP_CLASS : public PrePostProcessor {
public:
  PPP_CLASS(const aicnwdesc::networkDescriptor *nwDesc);

  void preProcessInputs(const BufferBindings &bindings) override;
  void postProcessOutputs(const BufferBindings &bindings) override;

  size_t getDMABufferSize(int bindingNum) const {
    assert(bindings_);
    return bindings_->getDMABufferSize(bindingNum);
  }
  char *getDMABufferRaw(int bindingNum) const {
    assert(bindings_);
    return bindings_->getDMABufferRaw(bindingNum);
  }

private:
  const aicnwdesc::networkDescriptor *nwDesc_{nullptr};

  friend class Tensor;
  void allocateTemps(
      const std::vector<std::vector<std::unique_ptr<Transform>>> &xfms);
  char *getTempBuffer(int size);
  void freeTempBuffer(Tensor &t);
  struct Buffer {
    Buffer() : buf(nullptr), size(0) {}
    Buffer(char *buf, int size) : buf(buf), size(size) {}
    char *buf;
    int size;
  };
  std::list<Buffer> freeBuffers_;

  // Number of partial inputs per DMA buffer.
  std::vector<int> numPartialInputs_;

  void prepareInputTransforms(int numDMABuffers);
  void prepareOutputTransforms();
  std::vector<std::unique_ptr<char[]>> allocedBuffers_;
  std::vector<std::vector<std::unique_ptr<Transform>>> inputTransforms_;
  std::vector<std::vector<std::unique_ptr<Transform>>> outputTransforms_;

  const BufferBindings *bindings_{nullptr};

  bool computeDynamicBindings(const BufferBindings &bindings,
                              DynamicBufferBindings &dynamicBindings) override;
  bool validateBufferDimensions(const BufferBindings &bindings,
                                const std::vector<dataType> &type);
};

PPP_CLASS::PPP_CLASS(const networkDescriptor *nwDesc) : nwDesc_(nwDesc) {
  if (const char *env = getenv("AICPPP_TIMING_MODE")) {
    if (std::string(env) != "0")
      TimingMode = true;
  }

  AICPPP_DEBUG(printf("creating PPP %p\n", this));
}

void Tensor::allocIfNeeded(PPP_CLASS *ppp) {
  if (buf_.kind == Binding::Kind::Temp && buf_.buf == nullptr)
    buf_.buf = ppp->getTempBuffer(getSizeInBytes());
}

size_t Tensor::getBufferRawSize(PPP_CLASS *ppp) const {
  switch (buf_.kind) {
  case Binding::User:
    assert(buf_.num != -1);
    return ppp->bindings_->userBindings[buf_.num].size;
  case Binding::DMA:
    assert(buf_.num != -1);
    return ppp->bindings_->dmaBindings[buf_.num].size;
  default:
    return -1U;
  }
}

char *Tensor::getBufferRaw(PPP_CLASS *ppp) {
  char *bufp = nullptr;
  switch (buf_.kind) {
  case Binding::User:
    assert(buf_.num != -1);
    bufp = ppp->bindings_->userBindings[buf_.num].ptr;
    break;
  case Binding::DMA:
    assert(buf_.num != -1);
    bufp = ppp->bindings_->dmaBindings[buf_.num].ptr;
    break;
  case Binding::Temp:
    bufp = buf_.buf;
    break;
  }

  return bufp + buf_.offset;
}

template <class DestTy, class SrcTy, class ElementXfm>
class ElementwiseXfm : public Transform {
public:
  ElementwiseXfm(const char *name, Tensor dst, Tensor src,
                 typename ElementXfm::ExtraArgs eltXfmArgs)
      : Transform(dst, src, /*constantNumDims=*/true), name_(name),
        eltXfmArgs_(eltXfmArgs) {}
  void run(PPP_CLASS *ppp) override {
    ScopedTimer timer(name_);
    int numElts = dstT_.getNumElts();
    auto *__restrict out = dstT_.getBuffer<DestTy>(ppp);
    auto *__restrict in = srcT_.getBuffer<SrcTy>(ppp);
    assert((char *)in != (char *)out);
    typename ElementXfm::ExtraArgs extraArgs = eltXfmArgs_;
    for (int i = 0; i < numElts; ++i)
      out[i] = ElementXfm::transform(in[i], extraArgs);
  }
  const char *name_;
  typename ElementXfm::ExtraArgs eltXfmArgs_;
};

template <networkDescLayout Layout, int EltSize> struct D32TileTraits {};
template <int EltSize> struct D32TileTraits<D32Untiled, EltSize> {
  static constexpr int dTileSize = 32;
  static constexpr int xTileSize = 1;
  static constexpr int yTileSize = 4;
  static constexpr int xSubTileSize = 1;
  static constexpr int ySubTileSize = 1;
  static constexpr int packedTileSize = 1;
};
template <int EltSize> struct D32TileTraits<D32ChannelMajor8x8, EltSize> {
  static constexpr int dTileSize = 32;
  static constexpr int xTileSize = 8;
  static constexpr int yTileSize = 8;
  static constexpr int xSubTileSize = 1;
  static constexpr int ySubTileSize = 1;
  static constexpr int packedTileSize = 1;
};
template <int EltSize> struct D32TileTraits<D32ChannelMajor1x64, EltSize> {
  static constexpr int dTileSize = 32;
  static constexpr int xTileSize = 1;
  static constexpr int yTileSize = 64;
  static constexpr int xSubTileSize = 1;
  static constexpr int ySubTileSize = 1;
  static constexpr int packedTileSize = 1;
};
template <int EltSize> struct D32TileTraits<D32SpatialMajor8x8, EltSize> {
  static constexpr int dTileSize = 32;
  static constexpr int xTileSize = 8;
  static constexpr int yTileSize = 8 / EltSize;
  static constexpr int xSubTileSize = 2 / EltSize;
  static constexpr int ySubTileSize = 2;
  static constexpr int packedTileSize = 1;
};
template <int EltSize> struct D32TileTraits<D32SpatialMajor1x64, EltSize> {
  static constexpr int dTileSize = 32;
  static constexpr int xTileSize = 1;
  static constexpr int xSubTileSize = 1;
  static constexpr int yTileSize = 64 / EltSize;
  static constexpr int ySubTileSize = 4 / EltSize;
  static constexpr int packedTileSize = 1;
};
template <int EltSize> struct D32TileTraits<D4ChannelMajor8x8, EltSize> {
  static constexpr int dTileSize = 4;
  static constexpr int xTileSize = 8;
  static constexpr int yTileSize = 8;
  static constexpr int xSubTileSize = 1;
  static constexpr int ySubTileSize = 1;
  static constexpr int packedTileSize = 1;
};
template <int EltSize> struct D32TileTraits<D4SpatialMajor8x8, EltSize> {
  static constexpr int dTileSize = 4;
  static constexpr int xTileSize = 8;
  static constexpr int yTileSize = 8 / EltSize;
  static constexpr int xSubTileSize = 2 / EltSize;
  static constexpr int ySubTileSize = 2;
  static constexpr int packedTileSize = 1;
};
template <int EltSize> struct D32TileTraits<PackedD4ChannelMajor8x8, EltSize> {
  static constexpr int dTileSize = 4;
  static constexpr int xTileSize = 8;
  static constexpr int yTileSize = 64;
  static constexpr int xSubTileSize = 1;
  static constexpr int ySubTileSize = 1;
  static constexpr int packedTileSize = 8;
};
template <int EltSize> struct D32TileTraits<PackedD4SpatialMajor8x8, EltSize> {
  static constexpr int dTileSize = 4;
  static constexpr int xTileSize = 8;
  static constexpr int yTileSize = 64 / EltSize;
  static constexpr int xSubTileSize = 2 / EltSize;
  static constexpr int ySubTileSize = 2;
  static constexpr int packedTileSize = 8;
};

static bool getD32Dims(networkDescLayout layout, int eltSize,
                       const std::vector<int> &dims, int &xDim, int &yDim,
                       int &dDim) {
  assert(dims.size() == 5);
  switch (layout) {
  case D32Untiled:
    xDim = dims[1];
    dDim = dims[2] * 32;
    yDim = dims[3] * 4;
    assert(dims[4] == 128);
    break;
  case D32ChannelMajor8x8:
    xDim = dims[1] * 8;
    yDim = dims[2] * 8;
    dDim = dims[3] * 32;
    assert(dims[4] == 2048);
    break;
  case D32ChannelMajor1x64:
    xDim = dims[1] * 1;
    yDim = dims[2] * 64;
    dDim = dims[3] * 32;
    assert(dims[4] == 2048);
    break;
  case D32SpatialMajor8x8:
    xDim = dims[1] * 8;
    yDim = dims[2] * 8 / eltSize;
    dDim = dims[3] * 32;
    assert(dims[4] == 2048 / eltSize);
    break;
  case D32SpatialMajor1x64:
    xDim = dims[1] * 1;
    yDim = dims[2] * 64 / eltSize;
    dDim = dims[3] * 32;
    assert(dims[4] == 2048 / eltSize);
    break;
  case D4ChannelMajor8x8:
    xDim = dims[1] * 8;
    yDim = dims[2] * 8;
    dDim = dims[3] * 4;
    assert(dims[4] == 256);
    break;
  case D4SpatialMajor8x8:
    xDim = dims[1] * 8;
    yDim = dims[2] * 8 / eltSize;
    dDim = dims[3] * 4;
    assert(dims[4] == 256 / eltSize);
    break;
  case PackedD4ChannelMajor8x8:
    xDim = dims[1] * 8;
    yDim = dims[2] * 64;
    dDim = dims[3] * 4;
    assert(dims[4] == 2048);
    break;
  case PackedD4SpatialMajor8x8:
    xDim = dims[1] * 8;
    yDim = dims[2] * 64 / eltSize;
    dDim = dims[3] * 4;
    assert(dims[4] == 2048 / eltSize);
    break;
  default:
    assert(!"unexpected layout");
    return false;
  }

  return true;
}


class CopyXfm : public Transform {
public:
  // When set indicates copy of packed partial input.  The dynamic offset into
  // the DMA buffer is computed at runtime.
  bool isPartial_;
  // For partial inputs the byte offset into DMA buffer.
  int dynamicOffset_;

  CopyXfm(Tensor dst, Tensor src)
      : Transform(dst, src, /*constantNumDims=*/true), isPartial_(false),
        dynamicOffset_(0) {}

  CopyXfm(Tensor dst, Tensor src, bool isPartial)
      : Transform(dst, src, /*constantNumDims=*/true), isPartial_(isPartial),
        dynamicOffset_(0) {
    // dynamicOffset_ is computed at runtime
  }

  void run(PPP_CLASS *ppp) override {
    ScopedTimer timer("  copy");
    assert(dstT_.getSizeInBytes() == srcT_.getSizeInBytes());
    // Nothing to do for partial input of size zero.
    if (srcT_.getSizeInBytes() == 0)
      return;
    char *dst = dstT_.getBufferRaw(ppp);
    if (isPartial_) {
      assert(dstT_.buf_.kind == Binding::DMA && dstT_.buf_.num != -1);
      // Write the dynamic offset and size into the DMA buffer header.
      aicppp::DynamicSliceInfo *sliceInfo = (aicppp::DynamicSliceInfo *)dst;
      sliceInfo->dynamicOffset = dynamicOffset_;
      sliceInfo->size = dstT_.getSizeInBytes();
      // Verify write to dst is inbounds.
      int dmaBuffNum = dstT_.buf_.num;
      assert(dynamicOffset_ + dstT_.getSizeInBytes() <=
             ppp->getDMABufferSize(dmaBuffNum));
      // Compute the real destination.
      dst = ppp->getDMABufferRaw(dmaBuffNum) + dynamicOffset_;
    }
    const char *src = srcT_.getBufferRaw(ppp);
    if (dst != src) {
      size_t writeSize = std::min(dstT_.getBufferRawSize(ppp),
                                  dstT_.getSizeInBytes());
      memcpy(dst, src, writeSize);
    }
  }
};

bool PPP_CLASS::computeDynamicBindings(const BufferBindings &bindings,
                                       DynamicBufferBindings &dynamicBindings) {
  if (inputTransforms_.empty())
    prepareInputTransforms(bindings.dmaBindings.size());

  assert(dynamicBindings.dmaBindings.empty());
  dynamicBindings.dmaBindings.resize(bindings.dmaBindings.size(),
                                     {0, false});

  auto getDMABuffNum =
      [&](const std::vector<std::unique_ptr<Transform>> &inputXfms) {
        assert(inputXfms.size() > 0);
        const auto &dstT = inputXfms[inputXfms.size() - 1]->dstT_;
        assert(dstT.buf_.kind == Binding::DMA);
        return dstT.buf_.num;
      };

  bool anyPartial = false;
  auto &inputs = nwDesc_->inputs();
  std::vector<dataType> ioTypes;
  for(const auto& input: nwDesc_->inputs()){
    ioTypes.emplace_back(input.io_initial().type());
  }
  for(const auto& output: nwDesc_->outputs()){
    ioTypes.emplace_back(output.io_initial().type());
  }

  assert(static_cast<size_t>(inputs.size()) == inputTransforms_.size());
  for (size_t ii = 0, ie = nwDesc_->inputs_size(); ii < ie; ++ii) {
    auto &input = inputs[ii];
    assert(ii < inputTransforms_.size());
    auto &inputXfms = inputTransforms_[ii];

    // Ignore inputs that have been optimized away.
    if (input.transformseq_size() == 0)
      continue;

    // For partial inputs update the tensors dimensions based on the size of the
    // partial input.
    if (input.is_partial_allowed()) {
      auto &init = input.io_initial();
      // Compute the number of elements based on the partial input size.
      size_t partialNumElts =
          bindings.userBindings[ii].size / getTypeSize(init.type());
      for (auto &inputXfm : inputXfms) {
        assert(inputXfm->constantNumDims_);
        inputXfm->srcT_.dims_[0] = partialNumElts;
        inputXfm->dstT_.dims_[0] = partialNumElts;
      }
      anyPartial = true;
      continue;
    }

    // For non-partial inputs, compute the size of the DMA buffer.
    // Get the size from the dst tensor of the last transform.
    assert(inputXfms.size() > 0);
    const auto &dstT = inputXfms[inputXfms.size() - 1]->dstT_;
    assert(dstT.buf_.kind == Binding::DMA);

    int dmaBuffNum = getDMABuffNum(inputXfms);
    auto &dmaBinding = dynamicBindings.dmaBindings[dmaBuffNum];
    uint64_t dmaSize = std::max<uint64_t>(
        dmaBinding.size, dstT.buf_.offset + dstT.getSizeInBytes());
    dmaBinding = {dmaSize, /*isPartial=*/false};
  }

  // The common case is that the user input buffers are combined into a single
  // DMA buffer.  If any of the user input buffers are partial, there's an
  // opportunity to further reduce the size of the DMA buffer by packing the
  // partial inputs. In order to pack partial inputs, we need to dynamically
  // compute the DMA buffer offsets, since the DMA offsets described in the
  // network description assume the maximum size, not the partial size.
  if (!anyPartial)
    return true;

  auto alignTo = [](uint64_t x, uint64_t m) -> uint64_t {
    return (x + m - 1) / m * m;
  };

  // We've computed the size of the DMA buffers for non-partial inputs.  Now
  // compute the dynamic offsets for the packed partial inputs.
  for (int ii = 0, ie = nwDesc_->inputs_size(); ii < ie; ++ii) {
    auto &input = inputs[ii];
    if (!input.is_partial_allowed())
      continue;

    auto &inputXfms = inputTransforms_[ii];
    int dmaBuffNum = getDMABuffNum(inputXfms);

    auto &dmaBinding = dynamicBindings.dmaBindings[dmaBuffNum];
    uint64_t headerSize =
        numPartialInputs_[dmaBuffNum] * sizeof(DynamicSliceInfo);

    uint64_t dynamicOffset =
        alignTo(std::max(dmaBinding.size, headerSize), input.align());
    // Set the dynamicOffset in the destination tensor bindings.
    assert(inputXfms.size() > 0);
    CopyXfm *copy =
        static_cast<CopyXfm *>(inputXfms[inputXfms.size() - 1].get());
    assert(copy);
    copy->dynamicOffset_ = dynamicOffset;

    // Update the size of the DMA buffer.
    int partialSize = copy->dstT_.getSizeInBytes();
    uint64_t dmaSize =
        std::max<uint64_t>(dmaBinding.size, dynamicOffset + partialSize);
    dmaBinding = {dmaSize, /*isPartial=*/true};
  }

  return true;
}

void PPP_CLASS::prepareInputTransforms(int numDMABuffers) {
  auto &inputs = nwDesc_->inputs();
  int tempNum = 0;

  // Compute the number of partial inputs for each DMA buffer.
  numPartialInputs_.resize(numDMABuffers);
  for (int i = 0; i < numDMABuffers; ++i)
    numPartialInputs_[i] = 0;

  for (int ii = 0, ie = nwDesc_->inputs_size(); ii < ie; ++ii) {
    auto &input = inputs[ii];
    if (!input.is_partial_allowed())
      continue;

    // Ignore inputs that have been optimized away.
    if (input.transformseq_size() == 0)
      continue;

    auto &copyDMA = input.transformseq()[input.transformseq_size() - 1];
    assert(copyDMA.kind() == CopyDMABufferTransform);
    auto &dma = copyDMA.copy_dma_buffer();
    size_t dmaBuffNum = dma.buffer_num();
    assert(dmaBuffNum < numPartialInputs_.size());
    numPartialInputs_[dmaBuffNum]++;
  }

  for (int ii = 0, ie = nwDesc_->inputs_size(); ii < ie; ++ii) {
    auto &input = inputs[ii];

    inputTransforms_.emplace_back();
    auto &inputXfms = inputTransforms_.back();

    // Ignore inputs that have been optimized away.
    if (input.transformseq_size() == 0)
      continue;

    // curT is updated as we go along processing each operation
    Tensor curT(input.io_initial(), Binding::Kind::User, ii, 0);
    Tensor tempT;

    for (int ti = 0, te = input.transformseq_size(); ti < te; ++ti) {
      auto &t = input.transformseq(ti);

      tempT = Tensor(t.type(), t.scale(), t.offset(), t.layout(), &t.dims(),
                     tempNum++);

      switch (t.kind()) {
      case QuantizeTransform:
      case TransposeTransform:
      case ConvertTransform:
      case ConvertToD32Transform:
      default:
        assert(0 && "Unexpected pre-processing transform!");
        return;

      case CopyDMABufferTransform: {
        assert((ti == te - 1) && "Copy dma buffer should be last transform.");
        auto &dma = t.copy_dma_buffer();
        auto binding =
            Binding(Binding::Kind::DMA, dma.buffer_num(), dma.offset());
        // If a previous transfrom has been performed and the input is not
        // partial, update the destination bindings to write directly to the DMA
        // buffer.  Otherwise, have the copy transform write the DMA buffer.
        bool isPartialInput = input.is_partial_allowed();
        if (!inputXfms.empty() && !isPartialInput) {
          auto &prevXfrm = inputXfms.back();
          prevXfrm->dstT_.buf_ = binding;
        } else {
          tempT.buf_ = binding;
          inputXfms.emplace_back(new CopyXfm(tempT, curT, isPartialInput));
        }
        break;
      }
      }
    }
  }

  allocateTemps(inputTransforms_);
}

void PPP_CLASS::preProcessInputs(const BufferBindings &bindings) {
  ScopedTimer timer("total preprocess");

  if (inputTransforms_.empty())
    prepareInputTransforms(bindings.dmaBindings.size());

  bindings_ = &bindings;

  for (auto &xfms : inputTransforms_)
    for (auto &xfm : xfms)
      xfm->run(this);

  bindings_ = nullptr;
}

void PPP_CLASS::prepareOutputTransforms() {
  auto &outputs = nwDesc_->outputs();
  int tempNum = 0;

  for (int oi = 0, oe = nwDesc_->outputs_size(); oi < oe; ++oi) {
    auto &output = outputs[oi];
    if (output.transformseq_size() == 0)
      continue;

    outputTransforms_.emplace_back();
    auto &outputXfms = outputTransforms_.back();

    int userBufNum = nwDesc_->inputs_size() + oi;

    // curT is updated as we go along processing each operation
    Tensor curT;
    Tensor tempT;

    for (int ti = 0, te = output.transformseq_size(); ti < te; ++ti) {
      auto &t = output.transformseq(ti);

      tempT = Tensor(t.type(), t.scale(), t.offset(), t.layout(), &t.dims(),
                     tempNum++);
      // The last output transform should write directly into the User buffer.
      if (ti + 1 == te)
        tempT.buf_ = Binding(Binding::Kind::User, userBufNum, 0);

      switch (t.kind()) {
      case DequantizeTransform:
      case TransposeTransform:
      case ConvertFromD32Transform:
      case ConvertTransform:
      default:
        assert(0 && "Unexpected post-processing transform!");
        break;

      case CopyDMABufferTransform: {
        assert(ti == 0 && "Copy dma buffer should be first transform.");
        auto &dma = t.copy_dma_buffer();
        Tensor copyT(output.io_initial(), Binding::Kind::DMA, dma.buffer_num(),
                     dma.offset());
        curT = copyT;
        break;
      }
      }
    }

    if (outputXfms.empty())
      outputXfms.emplace_back(new CopyXfm(tempT, curT));
  }

  allocateTemps(outputTransforms_);
}

// Post-process final placeholder computations that were hoisted out of the
// graph.
void PPP_CLASS::postProcessOutputs(const BufferBindings &bindings) {
  ScopedTimer timer("total postprocess");

  if (outputTransforms_.empty())
    prepareOutputTransforms();

  bindings_ = &bindings;

  for (auto &xfms : outputTransforms_)
    for (auto &xfm : xfms)
      xfm->run(this);

  bindings_ = nullptr;
}

void PPP_CLASS::allocateTemps(
    const std::vector<std::vector<std::unique_ptr<Transform>>> &xfms) {

  std::vector<char *> tempBuffers;

  for (auto &ioxfms : xfms) {
    for (auto &xfm : ioxfms) {
      Tensor &src = xfm->srcT_;
      if (src.buf_.kind == Binding::Temp) {
        assert(src.buf_.buf == nullptr);
        int tempNum = src.buf_.num;
        assert(tempNum != -1);
        src.buf_.buf = tempBuffers[tempNum];
      }

      Tensor &dst = xfm->dstT_;
      if (dst.buf_.kind == Binding::Temp) {
        assert(dst.buf_.buf == nullptr);
        int tempNum = dst.buf_.num;
        assert(tempNum != -1);
        if (tempBuffers.size() <= static_cast<size_t>(tempNum))
          tempBuffers.resize(tempNum + 1, nullptr);
        assert(tempBuffers[tempNum] == nullptr);
        tempBuffers[tempNum] = getTempBuffer(dst.getSizeInBytes());
        dst.buf_.buf = tempBuffers[tempNum];
      }

      if (src.buf_.kind == Binding::Temp) {
        freeTempBuffer(src);
        tempBuffers[src.buf_.num] = nullptr;
      }
    }
  }
}

char *PPP_CLASS::getTempBuffer(int size) {

  AICPPP_DEBUG(printf("%p getTempBuffer %d\n", this, size));

  for (auto it = freeBuffers_.begin(), ie = freeBuffers_.end(); it != ie;
       ++it) {

    AICPPP_DEBUG(printf("%p checking free buffer size %d\n", this, it->size));

    if (it->size >= size) {

      AICPPP_DEBUG(printf("%p reusing free buffer\n", this));

      char *buf = it->buf;
      freeBuffers_.erase(it);
      return buf;
    }
  }

  AICPPP_DEBUG(printf("%p allocate buffer %d\n", this, size));

  allocedBuffers_.emplace_back(new char[size]);
  return allocedBuffers_.back().get();
}

void PPP_CLASS::freeTempBuffer(Tensor &t) {
  if (t.buf_.kind != Binding::Kind::Temp)
    return;

  AICPPP_DEBUG(printf("%p adding free buffer\n", this));

  assert(t.buf_.buf != nullptr);
  freeBuffers_.emplace_front(t.buf_.buf, t.getSizeInBytes());
}

} // namespace PPP_CORE

std::unique_ptr<PrePostProcessor>
PPP_CREATEFN(const aicnwdesc::networkDescriptor *nwDesc) {
  return std::unique_ptr<PrePostProcessor>(new PPP_CORE::PPP_CLASS(nwDesc));
}

#ifdef PPP_CORE_DEFAULT

#ifdef __x86_64__
std::unique_ptr<PrePostProcessor>
createPrePostProcessorSKLImpl(const aicnwdesc::networkDescriptor *nwDesc);
std::unique_ptr<PrePostProcessor>
createPrePostProcessorAVX2Impl(const aicnwdesc::networkDescriptor *nwDesc);
#endif

std::unique_ptr<PrePostProcessor>
PrePostProcessor::create(const aicnwdesc::networkDescriptor *nwDesc) {
  return createPrePostProcessorDefaultImpl(nwDesc);
}
#endif

} // namespace aicppp
