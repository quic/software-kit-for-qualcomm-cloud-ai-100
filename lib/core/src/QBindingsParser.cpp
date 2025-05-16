// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QBindingsParser.h"
#include "QLogger.h"

using namespace qaic;

const char *defaultProgramName = "noname";
const char *defaultBufferName = "noname";
QBindingsParser::QBindingsParser()
    : ioBufferInfo_(nullptr), ioDescObject_(nullptr), initialized_(false){};

template <typename T>
uint32_t QBindingsParser::writeBindingElement(const T *src, int elts) {
  int offset = ioDescBuffer_.size();
  offset = alignTo(offset, alignof(T));
  size_t bytes = elts * sizeof(T);
  int newSize = offset + bytes;
  ioDescBuffer_.resize(newSize);
  memcpy(&ioDescBuffer_[offset], src, bytes);
  return ioDescBuffer_.size();
}

struct IoSetSearch {
  const char *name;
  const aicnwdesc::transformKind inputTransformKind;
  const aicnwdesc::transformKind outputTransformKind;
};

// Define all the transformations that will result in IoSets if present
const IoSetSearch IoSetSearchSpace[] = {
    {"quantize", aicnwdesc::QuantizeTransform, aicnwdesc::DequantizeTransform},
    {"transpose", aicnwdesc::TransposeTransform, aicnwdesc::TransposeTransform},
    {"d32convert", aicnwdesc::ConvertToD32Transform,
     aicnwdesc::ConvertFromD32Transform},
    {"convert", aicnwdesc::ConvertTransform, aicnwdesc::ConvertTransform},
    {"dma", aicnwdesc::CopyDMABufferTransform,
     aicnwdesc::CopyDMABufferTransform}};

QStatus QBindingsParser::getTransformIoTypes(
    std::string key, aicnwdesc::transformKind &inputTransformKind,
    aicnwdesc::transformKind &outputTransformKind) {
  for (uint32_t i = 0; i < sizeof(IoSetSearchSpace) / sizeof(IoSetSearch);
       ++i) {
    if (strcasecmp(key.c_str(), IoSetSearchSpace[i].name) == 0) {

      inputTransformKind = IoSetSearchSpace[i].inputTransformKind;
      outputTransformKind = IoSetSearchSpace[i].outputTransformKind;
      return QS_SUCCESS;
    }
  }
  return QS_INVAL;
}

QStatus QBindingsParser::getTransformType(std::string key,
                                          aicnwdesc::transformKind &kind,
                                          bool isInput) {
  for (uint32_t i = 0; i < sizeof(IoSetSearchSpace) / sizeof(IoSetSearch);
       ++i) {
    if (strcmp(key.c_str(), IoSetSearchSpace[i].name) == 0) {
      if (isInput) {
        kind = IoSetSearchSpace[i].inputTransformKind;
      } else {
        kind = IoSetSearchSpace[i].outputTransformKind;
      }
      return QS_SUCCESS;
    }
  }
  return QS_INVAL;
}

uint32_t
QBindingsParser::getBufferSize(uint32_t bufferNumber, QAicBufferIoTypeEnum type,
                               BufferTransformType transformed,
                               const aicnwdesc::networkDescriptor &protoDesc) {

  const aicnwdesc::IODescriptor *io_desc = nullptr;
  uint32_t bufSize = 1; // Multiply-Accumulate
  switch (type) {
  case QAicBufferIoTypeEnum::BUFFER_IO_TYPE_INPUT:
    if (bufferNumber > (uint32_t)protoDesc.inputs().size()) {
      break;
    }
    switch (transformed) {
    case TRANSFORMED:
      io_desc = &protoDesc.inputs(bufferNumber).io_transformed();
      break;
    case INITIAL:
      io_desc = &protoDesc.inputs(bufferNumber).io_initial();
      break;
    default:
      break;
    }
    break;
  case QAicBufferIoTypeEnum::BUFFER_IO_TYPE_OUTPUT:
    if (bufferNumber > (uint32_t)protoDesc.outputs().size()) {
      return 0;
    }
    switch (transformed) {
    case TRANSFORMED:
      io_desc = &protoDesc.outputs(bufferNumber).io_transformed();
      break;
    case INITIAL:
      io_desc = &protoDesc.outputs(bufferNumber).io_initial();
      break;
    default:
      break;
    }
    break;
  default:
    break;
  }
  if (io_desc == nullptr) {
    return 0;
  }

  // Set where dims start
  for (int j = 0; j < io_desc->dims_size(); j++) {
    int32_t dims = io_desc->dims(j);
    if (dims != 0) {
      bufSize *= dims;
    }
  }
  bufSize *= getTypeSize(io_desc->type());

  return bufSize;
}

uint32_t
QBindingsParser::getIoDescBufferSize(const aicnwdesc::IODescriptor &ioDesc) {
  uint32_t bufSize = 1; // Multiply-Accumulate
  for (int j = 0; j < ioDesc.dims_size(); j++) {
    int32_t dims = ioDesc.dims(j);
    if (dims != 0) {
      bufSize *= dims;
    }
  }
  bufSize *= QBindingsParser::getTypeSize(ioDesc.type());
  return bufSize;
}

QStatus
QBindingsParser::getIoBufferInfo(QAicIoBufferInfo *&ioBufferInfo) const {
  ioBufferInfo = ioBufferInfo_;
  return QS_SUCCESS;
}

QStatus
QBindingsParser::getIoBufferInfoDma(QAicIoBufferInfo *&ioBufferInfoDma) const {
  ioBufferInfoDma = ioBufferInfoDmaPtr_;
  return QS_SUCCESS;
}

QStatus QBindingsParser::getIoDescriptorPb(QData &ioDesc) const {
  if (ioDescBuffer_.size() == 0) {
    return QS_ERROR;
  }
  ioDesc.data = (uint8_t *)(ioDescBuffer_.data());
  ioDesc.size = ioDescBuffer_.size();
  return QS_SUCCESS;
}

QStatus QBindingsParser::getIoDescriptorPb(aicapi::IoDesc *&ioDesc) const {
  ioDesc = ioDescObject_.get();
  if (ioDescObject_ == nullptr) {
    return QS_ERROR;
  }
  return QS_SUCCESS;
}

uint32_t QBindingsParser::getTypeSize(aicnwdesc::dataType type) {
  uint32_t typeSize = 0;
  switch (type) {
  case aicnwdesc::FloatTy:
    typeSize = 4; // 32 bit Float type
    break;
  case aicnwdesc::Float16Ty:
    typeSize = 2; // 16 bit float type;
    break;
  case aicnwdesc::Int8QTy:
    typeSize = 1;
    break;
  case aicnwdesc::UInt8QTy:
    typeSize = 1;
    break;
  case aicnwdesc::Int16QTy:
    typeSize = 2;
    break;
  case aicnwdesc::Int32QTy:
    typeSize = 4;
    break;
  case aicnwdesc::Int32ITy:
    typeSize = 4;
    break;
  case aicnwdesc::Int64ITy:
    typeSize = 8;
    break;
  case aicnwdesc::Int8Ty:
    typeSize = 1;
    break;
  default:
    LogErrorG("Unsupported Type");
    break;
  }
  return typeSize;
}

QAicIoDataTypeEnum QBindingsParser::getIoDataType(aicnwdesc::dataType type) {
  QAicIoDataTypeEnum ioType = TYPE_INVALID;
  switch (type) {
  case aicnwdesc::FloatTy:
    ioType = FLOAT_TYPE;
    break;
  case aicnwdesc::Float16Ty:
    ioType = FLOAT_16_TYPE;
    break;
  case aicnwdesc::Int8QTy:
    ioType = INT8_Q_TYPE;
    break;
  case aicnwdesc::UInt8QTy:
    ioType = UINT8_Q_TYPE;
    break;
  case aicnwdesc::Int16QTy:
    ioType = INT16_Q_TYPE;
    break;
  case aicnwdesc::Int32QTy:
    ioType = INT32_Q_TYPE;
    break;
  case aicnwdesc::Int32ITy:
    ioType = INT32_I_TYPE;
    break;
  case aicnwdesc::Int64ITy:
    ioType = INT64_I_TYPE;
    break;
  case aicnwdesc::Int8Ty:
    ioType = INT8_TYPE;
    break;
  default:
    LogErrorG("Unsupported Type");
    break;
  }
  return ioType;
}

// Separately keep track of the offsets, so we don't need
// scaffolding in the QAicApi.h customer facing file
// This needs to be kept up-to-date with the AicApi QAicIoDec structure
struct IoDescOffsets {
  uint64_t bufferMappingsOffset;
  std::vector<uint64_t> bufferMappingsNameOffset;
  IoDescOffsets() : bufferMappingsOffset(0) {}
};

bool QBindingsParser::validate() {
  // Validate that Protocol buffers are compatible
  if ((aicnwdesc::FloatTy !=
       static_cast<aicnwdesc::dataType>(aicapi::FLOAT_TYPE)) ||
      (aicnwdesc::Float16Ty !=
       static_cast<aicnwdesc::dataType>(aicapi::FLOAT_16_TYPE)) ||
      (aicnwdesc::Int8QTy !=
       static_cast<aicnwdesc::dataType>(aicapi::INT8_Q_TYPE)) ||
      (aicnwdesc::UInt8QTy !=
       static_cast<aicnwdesc::dataType>(aicapi::UINT8_Q_TYPE)) ||
      (aicnwdesc::Int16QTy !=
       static_cast<aicnwdesc::dataType>(aicapi::INT16_Q_TYPE)) ||
      (aicnwdesc::Int32QTy !=
       static_cast<aicnwdesc::dataType>(aicapi::INT32_Q_TYPE)) ||
      (aicnwdesc::Int32ITy !=
       static_cast<aicnwdesc::dataType>(aicapi::INT32_I_TYPE)) ||
      (aicnwdesc::Int64ITy !=
       static_cast<aicnwdesc::dataType>(aicapi::INT64_I_TYPE)) ||
      (aicnwdesc::Int8Ty !=
       static_cast<aicnwdesc::dataType>(aicapi::INT8_TYPE)) ||
      (aicnwdesc::In !=
       static_cast<aicnwdesc::direction>(aicapi::BUFFER_IO_TYPE_INPUT)) ||
      (aicnwdesc::Out !=
       static_cast<aicnwdesc::direction>(aicapi::BUFFER_IO_TYPE_OUTPUT))) {
    return false;
  }
  return true;
}

bool QBindingsParser::init(const aicnwdesc::networkDescriptor &networkDesc) {
  ioDescBuffer_.clear();
  bool rc = validate();
  rc &= generateIoDesc(networkDesc);
  rc &= generateIoDescPb(networkDesc);
  rc &= updateIoBufferInfoDma(networkDesc);
  return rc;
}

void QBindingsParser::createBinding(
    const aicnwdesc::IODescriptor &nwIoDescriptor, aicapi::IoBinding *ioBinding,
    uint32_t index, aicnwdesc::direction bufferIoDirection,
    bool is_partial_allowed) {
  uint32_t bindingSize = 0;
  ioBinding->set_index(index);
  ioBinding->set_type(
      static_cast<aicapi::bufferIoDataTypeEnum>(nwIoDescriptor.type()));
  ioBinding->set_dir(
      static_cast<aicapi::bufferIoDirectionEnum>(bufferIoDirection));
  ioBinding->set_quant_scale(nwIoDescriptor.qscale());
  ioBinding->set_quant_offset(nwIoDescriptor.qoffset());
  ioBinding->mutable_dims()->CopyFrom(nwIoDescriptor.dims());
  ioBinding->set_is_partial_buf_allowed(is_partial_allowed);
  bindingSize = QBindingsParser::getTypeSize(nwIoDescriptor.type());
  for (auto &dim : nwIoDescriptor.dims()) {
    if (dim != 0) {
      bindingSize *= dim;
    }
  }
  ioBinding->set_size(bindingSize);
}

bool QBindingsParser::createBindingFromTransform(
    const aicnwdesc::transform *transform, aicapi::IoBinding *ioBinding,
    uint32_t index, aicnwdesc::direction bufferIoDirection,
    bool is_partial_allowed) {
  uint32_t bindingSize = 0;
  ioBinding->set_index(index);
  ioBinding->set_type(
      static_cast<aicapi::bufferIoDataTypeEnum>(transform->type()));
  ioBinding->set_dir(
      static_cast<aicapi::bufferIoDirectionEnum>(bufferIoDirection));
  ioBinding->set_quant_scale(transform->scale());
  ioBinding->set_quant_offset(transform->offset());
  ioBinding->mutable_dims()->CopyFrom(transform->dims());
  ioBinding->set_is_partial_buf_allowed(is_partial_allowed);
  bindingSize = QBindingsParser::getTypeSize(transform->type());
  for (auto &dim : transform->dims()) {
    if (dim != 0) {
      bindingSize *= dim;
    }
  }
  ioBinding->set_size(bindingSize);
  if (transform->kind() == aicnwdesc::CopyDMABufferTransform) {
    aicapi::DmaBufInfo *dmaBufInfo = ioBinding->add_dma_buf_info();
    if (dmaBufInfo == nullptr) {
      return false;
    }
    dmaBufInfo->set_dma_size(bindingSize);
    dmaBufInfo->set_dma_offset(transform->copy_dma_buffer().offset());
    dmaBufInfo->set_io_binding_offset(
        0); // this should be transform->offset() but it's not supported
    dmaBufInfo->set_dma_buf_index(transform->copy_dma_buffer().buffer_num());
  }
  return true;
}

bool QBindingsParser::generateIoDescPb(
    const aicnwdesc::networkDescriptor &networkDesc) {
  ioDescObject_ = std::make_unique<aicapi::IoDesc>();
  aicapi::IoDesc *ioDescObjPtr = ioDescObject_.get();
  aicapi::IoSet *ioSet = nullptr;
  aicapi::IoBinding *ioBinding = nullptr;
  uint32_t index = 0;
  if (ioDescObjPtr == nullptr) {
    return false;
  }

  // Does Any Output Have the transform we're looking for?
  auto hasInputTransform = [&](const aicnwdesc::networkDescriptor &nwDesc,
                               const IoSetSearch &s) {
    for (auto &b : nwDesc.inputs()) {
      for (auto &t : b.transformseq()) {
        if (t.kind() == s.inputTransformKind) {
          return true;
        }
      }
    }
    // These are not the drones you are looking for
    return false;
  };

  // Does Any Output Have the transform we're looking for?
  auto hasOutputTransform = [&](const aicnwdesc::networkDescriptor &nwDesc,
                                const IoSetSearch &s) {
    for (auto &b : nwDesc.outputs()) {
      for (auto &t : b.transformseq()) {
        if (t.kind() == s.outputTransformKind) {
          return true;
        }
      }
    }
    // These are not the drones you are looking for either
    return false;
  };

  auto bindingFindTransform = [&](
      const aicnwdesc::IOBinding &ioBinding, const aicnwdesc::transformKind &k,
      const aicnwdesc::transform *&transformPtr,
      const aicnwdesc::direction bufferIoDirection) {
    for (int i = 0; i < ioBinding.transformseq_size(); i++) {
      auto &t = ioBinding.transformseq(i);
      if (t.kind() == k) {
        // for Output if it is not CopyDMABufferTransform, take previous
        // transform
        if (bufferIoDirection == aicnwdesc::Out &&
            t.kind() != aicnwdesc::CopyDMABufferTransform) {
          transformPtr = &ioBinding.transformseq(i - 1);
        } else {
          transformPtr = &t;
        }
        return true;
      }
    }
    // These are not the drones you are looking for either
    return false;
  };

  //--------------------------------------------------
  // Add Default set, all transformations included
  //--------------------------------------------------
  ioSet = ioDescObjPtr->add_io_sets();
  if (ioSet == nullptr) {
    LogError("Failed to add set");
    return false;
  }
  ioSet->set_name("default");
  for (auto &binding : networkDesc.inputs()) {
    ioBinding = ioSet->add_bindings();
    if (ioBinding == nullptr) {
      LogError("Failed to add bindings");
      return false;
    }
    ioBinding->set_name(binding.name());
    createBinding(binding.io_initial(), ioBinding, index++, aicnwdesc::In,
                  binding.is_partial_allowed());
    ioBinding->set_is_valid(true);
  }

  for (auto &binding : networkDesc.outputs()) {
    ioBinding = ioSet->add_bindings();
    if (ioBinding == nullptr) {
      LogError("Failed to add bindings");
      return false;
    }
    ioBinding->set_name(binding.name());
    createBinding(binding.io_transformed(), ioBinding, index++, aicnwdesc::Out,
                  binding.is_partial_allowed());
    ioBinding->set_is_valid(true);
  }
  // Set the selected set as the default
  ioDescObjPtr->mutable_selected_set()->CopyFrom(*ioSet);

  for (uint32_t ss = 0;
       ss < sizeof(IoSetSearchSpace) / sizeof(IoSetSearchSpace[0]); ss++) {
    uint32_t bindingIndex = 0;
    const aicnwdesc::transform *transformPtr = nullptr;
    // Determine if any of the inputs have this transform
    bool inTransFound = hasInputTransform(networkDesc, IoSetSearchSpace[ss]);
    bool outTransFound = hasOutputTransform(networkDesc, IoSetSearchSpace[ss]);
    if (inTransFound || outTransFound) {
      // Add a new Set for this transform type
      ioSet = ioDescObjPtr->add_io_sets();
      if (ioSet == nullptr) {
        LogError("Failed to add bindings");
        return false;
      }
      ioSet->set_name(IoSetSearchSpace[ss].name);
      // One of the inputs (possibly many) has an input transform type
      for (auto &b : networkDesc.inputs()) {
        {
          ioBinding = ioSet->add_bindings();
          if (ioBinding == nullptr) {
            LogError("Failed to add bindings");
            return false;
          }
          ioBinding->set_name(b.name());

          // If we find the transform, set that input point for this
          if (bindingFindTransform(b, IoSetSearchSpace[ss].inputTransformKind,
                                   transformPtr, aicnwdesc::In)) {
            // This input has the transform
            createBindingFromTransform(transformPtr, ioBinding, bindingIndex++,
                                       aicnwdesc::In, b.is_partial_allowed());
            ioBinding->set_is_valid(true);
          } else {
            // One of the inputs has the transform, but not this one
            // Use the default transformation for this input
            createBinding(b.io_initial(), ioBinding, bindingIndex++,
                          aicnwdesc::In, b.is_partial_allowed());
            ioBinding->set_is_valid(false);
          }
        }
      }
      for (auto &b : networkDesc.outputs()) {
        {
          ioBinding = ioSet->add_bindings();
          if (ioBinding == nullptr) {
            LogError("Failed to add bindings");
            return false;
          }
          ioBinding->set_name(b.name());
          // If we find the transform, set that output point for this
          if (bindingFindTransform(b, IoSetSearchSpace[ss].outputTransformKind,
                                   transformPtr, aicnwdesc::Out)) {
            // This output has the transform
            createBindingFromTransform(transformPtr, ioBinding, bindingIndex++,
                                       aicnwdesc::Out, b.is_partial_allowed());
            ioBinding->set_is_valid(true);
          } else {
            // One of the outputs has the transform, but not this one
            // Use the default transformation for this input
            createBinding(b.io_transformed(), ioBinding, bindingIndex++,
                          aicnwdesc::Out, b.is_partial_allowed());
            ioBinding->set_is_valid(false);
          }
        }
      }

      LogInfo("Found and Adding Io Set for {}", IoSetSearchSpace[ss].name);
    }
  }
  for (int32_t idx = 0; idx < networkDesc.dma_buffers_size(); idx++) {
    aicapi::DmaBuf *dmaBuf = nullptr;
    dmaBuf = ioDescObjPtr->add_dma_buf();
    if (dmaBuf == nullptr) {
      LogError("Failed to create DMA Buf") return false;
    }
    dmaBuf->set_index(idx);
    dmaBuf->set_size(networkDesc.dma_buffers(idx).size());
    (networkDesc.dma_buffers(idx).dir() == aicnwdesc::In)
        ? dmaBuf->set_dir(aicapi::BUFFER_IO_TYPE_INPUT)
        : dmaBuf->set_dir(aicapi::BUFFER_IO_TYPE_OUTPUT);
  }

  // Add tensor specialization
  for (const auto &nwdShapes : networkDesc.allowed_shapes()) {
    auto *ioShapes = ioDescObjPtr->add_allowed_shapes();
    if (ioShapes == nullptr) {
      LogError("Failed to create IOShapes") return false;
    }
    for (const auto &nwdTensorShape : nwdShapes.shapes()) {
      auto *tensorShape = ioShapes->add_shapes();
      if (tensorShape == nullptr) {
        LogError("Failed to create tensorShape") return false;
      }
      tensorShape->set_type(
          static_cast<aicapi::bufferIoDataTypeEnum>(nwdTensorShape.type()));
      for (const auto &dim : nwdTensorShape.dims()) {
        tensorShape->add_dims(dim);
      }
    }
  }

  return true;
}

bool QBindingsParser::updateIoBufferInfoDma(
    const aicnwdesc::networkDescriptor &networkDesc) {

  for (int32_t idx = 0; idx < networkDesc.dma_buffers_size(); idx++) {
    QAicBufferIoTypeEnum ioType =
        (networkDesc.dma_buffers(idx).dir() == aicnwdesc::In)
            ? QAicBufferIoTypeEnum::BUFFER_IO_TYPE_INPUT
            : QAicBufferIoTypeEnum::BUFFER_IO_TYPE_OUTPUT;
    uint32_t size = networkDesc.dma_buffers(idx).size();

    QAicBufferMappings dmaBuffMapping = {
        (char *)defaultBufferName,
        (uint32_t)idx,
        ioType,
        size,
        false,
        QAicBufferDataTypeEnum::BUFFER_DATA_TYPE_INVAL};
    bufferMappingsVectorDma_.emplace_back(std::move(dmaBuffMapping));
  }

  ioBufferInfoDma_.numBufferMappings = bufferMappingsVectorDma_.size();
  ioBufferInfoDma_.bufferMappings = bufferMappingsVectorDma_.data();
  ioBufferInfoDmaPtr_ = &ioBufferInfoDma_;
  return true;
}

QAicBufferDataTypeEnum QBindingsParser::getIoFormat(aicnwdesc::dataType type) {
  switch (type) {
  case aicnwdesc::FloatTy:
    return QAicBufferDataTypeEnum::BUFFER_DATA_TYPE_FLOAT;
    break;
  case aicnwdesc::Float16Ty:
    return QAicBufferDataTypeEnum::BUFFER_DATA_TYPE_FLOAT16;
    break;
  case aicnwdesc::Int8QTy:
    return QAicBufferDataTypeEnum::BUFFER_DATA_TYPE_INT8Q;
    break;
  case aicnwdesc::UInt8QTy:
    return QAicBufferDataTypeEnum::BUFFER_DATA_TYPE_UINT8Q;
    break;
  case aicnwdesc::Int16QTy:
    return QAicBufferDataTypeEnum::BUFFER_DATA_TYPE_INT16Q;
    break;
  case aicnwdesc::Int32QTy:
    return QAicBufferDataTypeEnum::BUFFER_DATA_TYPE_INT32Q;
    break;
  case aicnwdesc::Int32ITy:
    return QAicBufferDataTypeEnum::BUFFER_DATA_TYPE_INT32I;
    break;
  case aicnwdesc::Int64ITy:
    return QAicBufferDataTypeEnum::BUFFER_DATA_TYPE_INT64I;
    break;
  case aicnwdesc::Int8Ty:
    return QAicBufferDataTypeEnum::BUFFER_DATA_TYPE_INT8;
    break;
  default:
    throw std::runtime_error("Invalid data type for buffer");
  }
}

bool QBindingsParser::generateIoDesc(
    const aicnwdesc::networkDescriptor &protoDesc) {
  QAicIoBufferInfo ioBufferInfoLocal;
  uint64_t offset;
  uint32_t ioBufferIndex = 0;

  IoDescOffsets ioDescOffsets;

  memset(&ioBufferInfoLocal, 0, sizeof(QAicIoBufferInfo));
  size_t bufferSizeBytes = ioDescBuffer_.size();
  ioBufferInfoLocal.numBufferMappings =
      protoDesc.inputs_size() + protoDesc.outputs_size();
  // Write Base IO Descriptor to ioDescBuffer_, argument 1 is only 1 instance
  offset = writeBindingElement(&ioBufferInfoLocal,
                               1); // Write the top part of the structure

  //------------------------------------------------------------------------
  // Desceribe the Buffer Inputs and Outputs
  // The Input Buffers are pre-transformation
  // The Output Buffers are post-transformation
  // This defines the IO Buffers the application must provide to run
  // the program
  //------------------------------------------------------------------------
  offset = alignTo(offset, 8);
  ioDescOffsets.bufferMappingsOffset = offset;
  for (int i = 0, e = protoDesc.inputs_size(); i < e; i++) {
    uint32_t bufSize = 1; // Multiply-Accumulate
    QAicBufferMappings mappingInfo;
    auto &inputs = protoDesc.inputs();
    auto &initial_input = inputs[i].io_initial();
    // Calculate total buffer size
    for (int j = 0, f = initial_input.dims_size(); j < f; j++) {
      int32_t dims = initial_input.dims(j);
      if (dims != 0) {
        bufSize *= dims;
      }
    }
    bufSize *= getTypeSize(initial_input.type());
    mappingInfo.bufferName = nullptr;
    mappingInfo.size = bufSize;
    mappingInfo.ioType = QAicBufferIoTypeEnum::BUFFER_IO_TYPE_INPUT;
    mappingInfo.index = ioBufferIndex++;
    mappingInfo.isPartialBufferAllowed = inputs[i].is_partial_allowed();
    mappingInfo.dataType = getIoFormat(inputs[i].io_initial().type());
    offset = writeBindingElement(&mappingInfo, 1);
  }
  offset = alignTo(offset, 8);
  for (int i = 0, e = protoDesc.outputs_size(); i < e; i++) {
    uint32_t bufSize = 1; // Multiply-Accumulate
    QAicBufferMappings mappingInfo;
    auto &outputs = protoDesc.outputs();
    auto &output_transformed = outputs[i].io_transformed();
    // Calculate total buffer size
    for (int j = 0, f = output_transformed.dims_size(); j < f; j++) {
      int32_t dims = output_transformed.dims(j);
      if (dims != 0) {
        bufSize *= dims;
      }
    }
    bufSize *= getTypeSize(output_transformed.type());
    mappingInfo.bufferName = nullptr;
    mappingInfo.size = bufSize;
    mappingInfo.ioType = QAicBufferIoTypeEnum::BUFFER_IO_TYPE_OUTPUT;
    mappingInfo.index = ioBufferIndex++;
    mappingInfo.isPartialBufferAllowed = outputs[i].is_partial_allowed();
    mappingInfo.dataType = getIoFormat(outputs[i].io_initial().type());
    offset = writeBindingElement(&mappingInfo, 1);
  }

  // Now write all the names
  for (int i = 0, e = protoDesc.inputs_size(); i < e; i++) {
    ioDescOffsets.bufferMappingsNameOffset.push_back(offset);
    if (protoDesc.inputs(i).name().size() != 0) {
      std::string name = protoDesc.inputs(i).name();
      name.resize(alignToWord(name.size() + 1));
      offset = writeBindingElement(name.c_str(), name.size());
    } else {
      offset = writeBindingElement(&defaultBufferName,
                                   alignToWord(strlen(defaultBufferName) + 1));
    }
  }

  for (int i = 0, e = protoDesc.outputs_size(); i < e; i++) {
    ioDescOffsets.bufferMappingsNameOffset.push_back(offset);
    if (protoDesc.outputs(i).name().size() != 0) {
      std::string name = protoDesc.outputs(i).name();
      name.resize(alignToWord(name.size() + 1));
      offset = writeBindingElement(name.c_str(), name.size());
    } else {
      offset = writeBindingElement(&defaultBufferName,
                                   alignToWord(strlen(defaultBufferName) + 1));
    }
  }

  //------------------------------------------------------------------------
  // Finish
  //------------------------------------------------------------------------
  // Since the Data Buffer is a vector, it's data location can change during
  // writing process, as we extend the vector size
  // Thus we must only save the offsets while writing and at the end update the
  // pointers
  //------------------------------------------------------------------------
  uint8_t *base = ioDescBuffer_.data();
  bufferSizeBytes = ioDescBuffer_.size();
  QAicIoBufferInfo *ioBufferInfoPtr =
      (QAicIoBufferInfo *)(ioDescBuffer_.data());

  ioBufferInfoPtr->bufferMappings = (QAicBufferMappings *)computeAddr(
      base, bufferSizeBytes, ioDescOffsets.bufferMappingsOffset);

  if (ioBufferInfoPtr->bufferMappings == nullptr) {
    LogErrorG("Buffer Mapping failed.");
    return false;
  }

  for (uint32_t i = 0; i < ioBufferInfoPtr->numBufferMappings; i++) {
    QAicBufferMappings *mapping = &ioBufferInfoPtr->bufferMappings[i];
    mapping->bufferName = (char *)computeAddr(
        base, bufferSizeBytes, ioDescOffsets.bufferMappingsNameOffset[i]);
  }
  ioBufferInfo_ = ioBufferInfoPtr;
  initialized_ = true;
  return true;
}
