// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QAIC_OPENRT_EXECOBJ_HPP
#define QAIC_OPENRT_EXECOBJ_HPP

#include "QAicOpenRtLogger.hpp"
#include "QAicOpenRtExceptions.hpp"
#include "QAicOpenRtQpc.hpp"
#include "QAicOpenRtInferenceVector.hpp"
#include "QAicRuntimeTypes.h"
#include "QAicOpenRtProgram.hpp"
#include "QExecObj.h"

namespace qaic {
namespace openrt {

/// \brief An ExecObj represents an object that can be executed
/// on a supported device.  Typically, multiple ExecObj are created
/// from the same program and are submitted for execution, creating
/// an inference flow and maintaining the device queues busy.
/// Each ExecObj retains a Vector of QBuffers. The size of the QBuffer
/// vector is determined from the program the ExecObj is built for.
/// Each ExecObj is created with a unique ID, this ID can be retrived with
/// the getId method once the object is created.
/// Prior to inference, the user must populate the data into the vector
/// by calling the setData method.
class ExecObj : public Logger {
public:
  /// \brief Create a shared_ptr ExecObj
  /// The number of buffers is determined from the program
  /// \param[in] context A previously created context
  /// \param[in] program A previously created program shared object
  /// \param[in] properties ExecObj properties, omit or set to null for
  /// defaults
  /// \return Shared pointer ExecObj
  /// \exception CoreExceptionNullPtr
  /// - When memory allocation for object fails
  /// \exception CoreExceptionInit
  /// - When input Parameters are invalid
  /// - When execObj properties are invalid
  /// - When out of memory
  /// - When execObj creation fails due to internal error
  static shExecObj Factory(shContext context, shProgram program,
                           const QAicExecObjProperties *properties = nullptr) {
    shExecObj obj =
        shExecObj(new (std::nothrow) ExecObj(context, program, properties));
    if (!obj) {
      throw CoreExceptionNullPtr("Failed to create execObj Object");
    }
    obj->init();
    return obj;
  };

  /// \brief Destructor ensures that object is released
  /// and that all associated resources are released
  ~ExecObj() {
    if (execobj_ != nullptr) {
      execobj_->releaseExecObj();
      execobj_ = nullptr;
    }
  }

  /// \brief Get the AicProgram (C-99 comaptible) object
  /// \return ExecObj pointer
  const shQExecObj &getExecObj() const { return execobj_; }

  /// \brief Get the unique ID of this object
  /// \return Object ID
  QAicExecObjID getId() const { return id_; }

  void setId(QAicExecObjID id) { id_ = id; }

  /// \brief Get QBuffer vector on which ExecObj is working
  /// \param[out] qBufferVect QBuffer Vector for input and output
  /// buffers
  /// \retVal QS_SUCCESS Successful completion
  /// \retVal QS_INVAL Internal buffer is configured for unsupported
  /// buffer type
  QStatus getData(std::vector<QBuffer> &qBufferVect) {
    switch (bufferType_) {
    case BufferType::BUFFER_TYPE_USER:
      qBufferVect = userBuffers_;
      break;
    default:
      return QS_INVAL;
    }
    return QS_SUCCESS;
  }

  /// \brief Set a new vector for execution, this should be called
  /// prior to enqueue or run.
  /// \param[in] qBufferVect QBuffer Vector for input and output buffers
  /// \retval QS_SUCCESS Successful completion
  /// \retval QS_INVAL Due to either of the following
  /// - Non-empty bufDims
  /// - Invalid qBufferVect size
  /// - Invalid qBufferVect buffers
  /// - Invalid program state
  /// - Invalid associated internal execObj
  /// \retval QS_ERROR Invalid Exec Obj Mode
  QStatus setData(const std::vector<QBuffer> &qBufferVect) {
    QStatus status = QS_ERROR;

    status = execobj_->setData(qBufferVect.size(), qBufferVect.data());
    if ((status == QS_SUCCESS) &&
        (bufferType_ == BufferType::BUFFER_TYPE_USER)) {
      userBuffers_.clear();
      userBuffers_ = qBufferVect;
    }

    return status;
  }

  /// \brief Set a new vector for execution, this should be called
  /// prior to enqueue or run
  /// \param[in] InferenceVector A previously created Inference Vector
  /// \retval QS_SUCCESS Successful completion
  /// \retval QS_INVAL Due to either of the following
  /// - Invalid argument \a InferenceVector
  /// - Invalid qBufferVect size
  /// - Invalid qBufferVect buffers
  /// - Invalid program state
  /// - Invalid associated internal execObj
  /// \retval QS_ERROR Invalid Exec Obj Mode
  QStatus setData(shInferenceVector inferenceVector) {
    if (!inferenceVector) {
      logError("Invalid Inference Vector");
      return QS_INVAL;
    }
    const std::vector<QBuffer> &qBufferVector = inferenceVector->getVector();
    QStatus status = QS_ERROR;
    status = execobj_->setData(qBufferVector.size(), qBufferVector.data());

    if ((status == QS_SUCCESS) &&
        (bufferType_ == BufferType::BUFFER_TYPE_USER)) {
      userBuffers_.clear();
      userBuffers_ = qBufferVector;
    }
    return status;
  }

  /// \brief Run an inference with the provided DataVector
  /// \retval QS_SUCCESS Successful completion
  /// \retval QS_INVAL Due to either of the following
  /// - Invalid qBufferVect size
  /// - Invalid qBufferVect buffers
  /// - Invalid associated internal execObj
  /// \retval QS_ERROR Failed to run the ExecObj
  QStatus run() const { return execobj_->run(); }

  ExecObj(const ExecObj &) = delete;            // Disable Copy Constructor
  ExecObj &operator=(const ExecObj &) = delete; // Disable Assignment Operator
private:
  ExecObj(shContext context, shProgram program,
          const QAicExecObjProperties *properties)
      : Logger(context), context_(context), program_(program),
        properties_(properties), execobj_(nullptr), numBuffers_(0),
        bufferType_(BufferType::BUFFER_TYPE_USER), id_(0) {}

  void init() {
    QStatus status = QS_SUCCESS;
    if (!context_) {
      throw CoreExceptionInit("Invalid context");
    }
    if (!program_) {
      throw CoreExceptionInit("Invalid program");
    }
    switch (bufferType_) {
    case BufferType::BUFFER_TYPE_USER: {
      // Get Num Buffers based on program
      const BufferMappings bufferMappings = program_->getBufferMappings();
      numBuffers_ = bufferMappings.size();
      execobj_ = QExecObj::createExecObj(
          context_->getContext(), properties_, program_->getQid(),
          program_->getProgram(), nullptr, nullptr, status);
      if (status != QS_SUCCESS || execobj_ == nullptr) {
        throw CoreExceptionInit("Failed to create execObj");
      }
    } break;
    default:
      throw CoreExceptionInit("Invalid Buffer type");
    }

    id_ = execobj_->getId();
  }

  // Constructor Initialized
  shContext context_;
  shProgram program_;
  const QAicExecObjProperties *properties_;
  shQExecObj execobj_;
  uint32_t numBuffers_;
  const std::vector<QBuffer> dmaBuffers_;
  std::vector<QBuffer> userBuffers_;
  BufferType bufferType_;
  uint32_t id_;
};
///\}
} // namespace rt
} // namespace qaic

#endif // QAIC_OPENRT_EXECOBJ_HPP
