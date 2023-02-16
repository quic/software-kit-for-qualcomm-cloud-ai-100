// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QEXECOBJ_H
#define QEXECOBJ_H
#include <queue>

#include "QAicRuntimeTypes.h"
#include "QPrePostProc.h"
#include "QAic.h"
#include "QLog.h"
#include "QContext.h"
#include "QNeuralNetworkInterface.h"
#include "QComponent.h"
#include "QAicApi.pb.h"
#include "metadataflatbufDecode.hpp"
#include "QProgram.h"

namespace qaic {

// #define BINDINGS_MAX_DIMENSIONS 8
constexpr uint8_t BINDINGS_MAX_DIMENSIONS = 8;

using google::protobuf::RepeatedField;
using google::protobuf::RepeatedPtrField;

class QExecObj;
class QProgram;
class QProgramDevice;
class QComponent;
class ExecObjProfilingHandle;
class ExecObjProfiling;
using QSessionID = std::atomic_uint64_t;

class QExecObj : public virtual QComponent, private QIAicApiContext {
public:
  static shQExecObj createExecObj(shQContext context,
                                  const QAicExecObjProperties *properties,
                                  QID qid, const shQProgram &program,
                                  uint32_t *numBuffers, QBuffer **buffers,
                                  QStatus &status);

  QExecObj(shQContext &context, const QAicExecObjProperties *properties,
           QID qid, const shQProgram &program, uint32_t numBuffers);

  // Start virtual interfaces
  virtual QStatus releaseExecObj();
  virtual ~QExecObj();
  virtual QStatus setData(const uint32_t numBuffers, const QBuffer *buffers);

  virtual QStatus submit();
  virtual QStatus finish();
  virtual QStatus run();

  virtual QStatus prepareToSubmit();
  virtual bool isReady(); // Program is loaded and activated
  // End virtual interfaces

  shQContext context() const { return context_; }
  uint32_t getId() { return Id_; }
  shQIEvent &getDefaultEvent();
  uint32_t getNumBuffers() const { return numBuffers_; };
  const QSessionID getSessionID() { return sessionID_.load(); }
  static QStatus
  validateExecObjProperties(const QAicExecObjProperties *properties);
  shQProgram getProgram() const { return program_; }

  QExecObj(const QExecObj &) = delete;            // Disable Copy Constructor
  QExecObj &operator=(const QExecObj &) = delete; // Disable Assignment Operator

protected:
  QSessionID sessionID_;

private:
  QStatus init();

  QBuffer *getBufferArrayPtr();
  bool initPrePostTransforms();
  QStatus preTransform();
  QStatus postTransform();
  static constexpr QAicExecObjProperties defaultExecObjProperties_ =
      static_cast<uint32_t>(
          QAicExecObjPropertiesBitField::QAIC_EXECOBJ_PROPERTIES_DEFAULT);
  QAicExecObjProperties properties_;
  QID dev_;
  shQProgram program_;
  QData ioDescPbData_;
  uint32_t numBuffers_;
  std::unique_ptr<QPrePostProc> ppHandle_;
  AicMetadataFlat::MetadataT metadata_;
  const QRuntimeInterface *rt_;
  const QNeuralNetworkInterface *qnn_;
  const aicnwdesc::networkDescriptor *netdesc_; // Created from Program
  std::vector<QBuffer> dmaQBuffersVec_;         // dma Buffers
  std::vector<QBuffer> userQBuffersVec_;        // user Buffers
  std::shared_ptr<QInfHandle> infHandle_;
  std::vector<QBuffer> dmaBuffers_;
  aicppp::BufferBindings bufferBindings_; // For Pre/Post Processing
  QProgramDevice *programDevice_;
  bool initialized_;
  bool hasPartialTensor_;
};

} // namespace qaic

#endif // QExecObjT_H
