// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
#ifndef QBINDING_PARSER_H
#define QBINDING_PARSER_H

#include "QAic.h"
#include "QUtil.h"
#include "QLogger.h"
#include "QAicRuntimeTypes.h"
#include "AICNetworkDesc.pb.h"
#include "QAicApi.pb.h"

namespace qaic {
class QBindingsParser : virtual private QLogger {
public:
  enum BufferTransformType { INITIAL = 0, TRANSFORMED = 1 };

  QBindingsParser();
  bool init(const aicnwdesc::networkDescriptor &protoDesc);

  QStatus getIoBufferInfo(QAicIoBufferInfo *&ioBufferInfo) const;
  QStatus getIoBufferInfoDma(QAicIoBufferInfo *&ioBufferInfoDma) const;
  QStatus getIoDescriptorPb(QData &data) const;
  QStatus getIoDescriptorPb(aicapi::IoDesc *&ioDesc) const;
  static uint32_t getTypeSize(aicnwdesc::dataType type);
  static uint32_t getBufferSize(uint32_t bufferNumber,
                                QAicBufferIoTypeEnum type,
                                BufferTransformType transformed,
                                const aicnwdesc::networkDescriptor &protoDesc);
  static uint32_t getIoDescBufferSize(const aicnwdesc::IODescriptor &ioDesc);
  static QAicIoDataTypeEnum getIoDataType(aicnwdesc::dataType type);
  static QStatus getTransformType(std::string key,
                                  aicnwdesc::transformKind &kind, bool isInput);
  static QStatus
  getTransformIoTypes(std::string key,
                      aicnwdesc::transformKind &inputTransformKind,
                      aicnwdesc::transformKind &outputTransformKind);
  static QStatus
  ProcessPrePostSkipStages(aicapi::IoDesc &ioDescPb,
                           std::vector<std::string> &preProcessSkipStages,
                           std::vector<std::string> &postProcessSkipStages);

  QAicBufferDataTypeEnum getIoFormat(aicnwdesc::dataType);

private:
  bool validate();
  // Support for parsing and generating protocol buffer based IO Descriptor
  bool generateIoDescPb(const aicnwdesc::networkDescriptor &protoDesc);

  // Support for parsing and generating legacy IO Descriptor
  bool generateIoDesc(const aicnwdesc::networkDescriptor &protoDesc);
  bool updateIoBufferInfoDma(const aicnwdesc::networkDescriptor &protoDesc);

  void createBinding(const aicnwdesc::IODescriptor &nwIoDescriptor,
                     aicapi::IoBinding *ioBinding, uint32_t index,
                     aicnwdesc::direction bufferIoDirection,
                     bool is_partial_allowed);

  bool createBindingFromTransform(const aicnwdesc::transform *transform,
                                  aicapi::IoBinding *ioBinding, uint32_t index,
                                  aicnwdesc::direction bufferIoDirection,
                                  bool is_partial_allowed);

  template <typename T> uint32_t writeBindingElement(const T *src, int elts);
  static uint64_t alignTo(uint64_t x, uint64_t m) {
    return (x + m - 1) / m * m;
  }
  static uint8_t *computeAddr(uint8_t *bufferBase, uint64_t bufferSize,
                              uint64_t offset) {
    if (offset > bufferSize) {
      LogErrorG("Invalid offset {}, for bufferSize {}", offset, bufferSize);
      return nullptr;
    }
    return bufferBase + offset;
  }
  static uint32_t alignToWord(uint32_t x) { return ((x + 4 - 1) / 4) * 4; }
  QAicIoBufferInfo *ioBufferInfo_;
  std::vector<uint8_t> ioDescBuffer_; // For ioBufferInfo

  QAicIoBufferInfo ioBufferInfoDma_;
  QAicIoBufferInfo *ioBufferInfoDmaPtr_;
  std::vector<QAicBufferMappings> bufferMappingsVectorDma_;

  std::unique_ptr<aicapi::IoDesc> ioDescObject_; // For ProtoBuf
  bool initialized_;
};

} // namespace qaic

#endif // QBINDING_PARSER_H
