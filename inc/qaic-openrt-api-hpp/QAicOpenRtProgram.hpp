// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QAIC_OPENRT_PROGRAM_HPP
#define QAIC_OPENRT_PROGRAM_HPP

#include "QAicOpenRtLogger.hpp"
#include "QAicOpenRtExceptions.hpp"
#include "QAicOpenRtQpc.hpp"
#include "QAicRuntimeTypes.h"
#include "QProgram.h"

namespace qaic {
namespace openrt {

/// \brief Represents a instance of a program on a AIC100 device.
/// Once created the program resides in host memory, it will not consume
/// device resources until ExecObj created from this program are
/// enqueued to the device.
/// ExecObj should be created with reference to this program, once
/// enqueued, the program will automatically be loaded and made ready
/// for execution (activated).
/// Alternatively, the user may call the load and activate preemptively,
/// which will cause the program to be made ready for execution ahead
/// of the enqueue time.
/// Create program by passing in the QpcObj to factory.
/// The QpcObj method requires that the user opens the QpcObj, in which
/// case the driver will maintain a reference count and a state of the
/// loaded program on device.  Use this method if multiple programs
/// instances are to be created on the same device, which improves resource
/// usage (memory), as only one instance of the program will be loaded
/// on device
class Program : public Logger {

public:
  /// Instantiate a shared pointer Program object from QPC Object
  /// \param[in] context A previously created context
  /// \param[in] dev Device associated with this program
  /// properties to NULL will result in default properties
  /// \param[in] name Given name for this object, used for logging
  /// \param[in] qpc  Program Container Object containing the program data
  /// \param[in] properties Program properties, omit or set to null for
  /// defaults
  /// \return  Program shared pointer on Success
  /// \exception CoreExceptionNullPtr
  /// - When memory allocation for object fails
  /// \exception CoreExceptionInit
  /// - When input Parameters are invalid
  /// - When out of memory
  /// - When failed to create program from QPC
  /// - When failed to get info from program created
  static shProgram Factory(shContext context, QID dev, const char *name,
                           shQpc qpc,
                           const QAicProgramProperties *properties = nullptr) {
    shProgram obj = shProgram(new (std::nothrow)
                                  Program(context, dev, name, qpc, properties));
    if (!obj) {
      throw CoreExceptionNullPtr("Failed to create program");
    }
    obj->init();
    return obj;
  };

  /// \brief Destructor ensures that object is released
  /// and that all associated resources are released
  ~Program() {
    QStatus status = QS_SUCCESS;
    if (program_ != nullptr) {
      if (program_->releaseProgram() != QS_SUCCESS) {
        std::cerr << "Failed to release Program object" << std::endl;
      }
      program_ = nullptr;
    }
  }

  /// \brief Load a program. This is an optional step, the program will
  /// automatically
  /// be loaded when an ExecObj is enqueued for execution and the program
  /// is required on device.  Alternatively, the user may call this API
  /// to pre-emptively load the program on device
  /// \retval QS_SUCCESS Successful completion
  /// \retval QS_INVAL Internal error in loading program
  /// \retval QS_ERROR Load program failed
  QStatus load() { return program_->load(); }

  /// \brief Unload a program. This is an optional step, the program will
  /// automatically
  /// be unloaded when all references to the program are destroyed, e.g,
  /// when no execObj are pointing to the program and the program itself
  /// is destroyed.
  /// \retval QS_SUCCESS Successful Completion
  /// \retval QS_INVAL Internal error in unloading program
  /// \retval QS_ERROR Unload program failed
  /// - Invalid state or
  /// - Cannot deactivate program
  QStatus unload() { return program_->unload(); }

  /// \brief Returns the loaded state of a program
  /// \retval true If program is currently loaded
  /// \retval false If program is not currently loaded
  /// \exception CoreExceptionRuntime
  /// - When failed to get program status
  bool isLoaded() {
    QAicProgramInfo info;
    if (program_->getProgramInfo(info) != QS_SUCCESS) {
      throw CoreExceptionRuntime("Failed to get Program Status");
    }
    return ((info.status == QAicProgramStatus::QAIC_PROGRAM_FULLY_ACTIVATED) ||
            (info.status == QAicProgramStatus::QAIC_PROGRAM_LOADED));
  }

  /// \brief Activate a program. This is an optional step, the program will
  /// automatically be activated when an ExecObj is enqueued for
  /// execution and the program is required on device.
  ///  Alternatively, the user may call this API
  /// to pre-emptively load the program on device
  /// \retval QS_SUCCESS Successful completion
  /// \retval QS_INVAL Internal error in activating program
  /// \retval QS_ERROR Failed to run Activation command
  QStatus activate() {
    return program_->processActivateCmd(
        QAicProgramActivationCmd::QAIC_PROGRAM_CMD_ACTIVATE_FULL);
  }

  /// \brief Deactivate a program. This is an optional step, the program will
  /// automatically be deactivated when all references to the program
  /// are destroyed, e.g, when no execObj are pointing to the program
  /// and the program itself is destroyed.
  /// \retval QS_SUCCESS Successful completion
  /// \retval QS_INVAL Internal error in deactivating program
  /// \retval QS_ERROR Failed to run Activation command
  QStatus deactivate() {
    return program_->processActivateCmd(
        QAicProgramActivationCmd::QAIC_PROGRAM_CMD_DEACTIVATE_FULL);
  }

  /// \brief Returns the activated state of a program
  /// \retval true If program is currently activated
  /// \retval false If program is not currently activated
  /// \exception CoreExceptionRuntime
  /// - When failed to get program info
  bool isActivated() {
    QAicProgramInfo info;
    if (program_->getProgramInfo(info) != QS_SUCCESS) {
      throw CoreExceptionRuntime("Failed to get Program Status");
    }
    return info.status == QAicProgramStatus::QAIC_PROGRAM_FULLY_ACTIVATED;
  }

  /// \brief Get Program Information
  /// \return The program info
  const ProgramInfo &getProgramInfo() const { return programInfo_; }

  /// \brief Get Aic Program Information
  /// \retval QS_SUCCESS Successful completion
  /// \retval QS_INVAL Internal error in getting program info
  /// \retval QS_ERROR Internal error on getting program info
  QStatus getAicProgramInfo(QAicProgramInfo &programInfo) {
    return program_->getProgramInfo(programInfo);
  }

  /// \brief Allows the user to define the program identifier
  /// In this ID will be returned in ProgramInfo and returned
  /// from getProgramInfo
  void setId(QAicProgramID id) { programInfo_.userDefinedProgramID = id; }

  /// \brief Retrieve the user defined id
  /// \return user defined id
  QAicProgramID getId() { return programInfo_.userDefinedProgramID; }

  /// \brief Get number of inferences completed for this program
  /// \return Number of inferences completed
  /// \exception CoreExceptionRuntime
  /// - When failed to get inference completed count
  uint64_t getInferenceCompleteCount() {
    uint64_t count;
    if (program_->getInferenceCompletedCount(count) != QS_SUCCESS) {
      throw CoreExceptionRuntime("Failed to get inference completed count");
    }
    return count;
  }

  /// \brief Get Buffer Mappings
  /// Buffer mappings provide details on the name, size, direction
  /// and index of each buffer in the inference vector
  const BufferMappings &getBufferMappings() const { return bufferMappings_; }

  /// Get Io Descriptor
  /// IO Descriptor is a protocol buffer that defines the pre-processing
  /// and post-processing sequence
  /// Refer to QAicApi.pb.h for protocol buffer bindings
  QStatus getIoDescriptor(QData *ioDesc) {
    return program_->getIoDescriptor(ioDesc);
  }

  /// \brief Initialize properties to default for this object
  /// \exception CoreExceptionInit
  /// - When access to QAic API library fails
  static void initProperties(QAicProgramProperties &programProperties) {
    QProgram::initProperties(&programProperties);
  }

  shQProgram &getProgram() { return program_; }

  QID getQid() const noexcept { return program_->getQid(); }

  Program(const Program &) = delete;            // Disable Copy Constructor
  Program &operator=(const Program &) = delete; // Disable Assignment Operator

private:
  Program(shContext context, const std::string &name)
      : Logger(context), context_(context), programName_(name) {}

  Program(shContext context, QID dev, const char *name, shQpc qpcObj,
          const QAicProgramProperties *properties)
      : Logger(context), context_(context), dev_(dev), qpcObj_(qpcObj),
        properties_(properties), programName_(name) {}

  void init() {

    QStatus status;
    QAicIoBufferInfo *bufferInfo = nullptr;
    if (!context_) {
      throw CoreExceptionInit("Invalid Context");
    }

    if ((program_ == nullptr) && (!qpcObj_)) {
      throw CoreExceptionInit("Cannot Use QPC, not a valid object");
    }

    if (program_ == nullptr) {
      program_ = QProgram::createProgram(context_->getContext(), *properties_,
                                         dev_, programName_.c_str(),
                                         qpcObj_->getQpc(), status);
      if (program_ == nullptr || status != QS_SUCCESS) {
        throw CoreExceptionInit("Failed to create program with Qpc Buffer");
      }
    }

    status = program_->getIoBufferInfo(&bufferInfo);
    if (status != QS_SUCCESS || bufferInfo == nullptr) {
      throw CoreExceptionInit("Failed to get program Buffer Info");
    }
    for (uint32_t j = 0; j < bufferInfo->numBufferMappings; j++) {
      bufferMappings_.emplace_back(
          bufferInfo->bufferMappings[j].bufferName,
          bufferInfo->bufferMappings[j].index,
          bufferInfo->bufferMappings[j].ioType,
          bufferInfo->bufferMappings[j].size,
          bufferInfo->bufferMappings[j].isPartialBufferAllowed,
          bufferInfo->bufferMappings[j].dataType);
    }

    {
      QAicProgramInfo info;
      status = program_->getProgramInfo(info);
      if (status != QS_SUCCESS) {
        throw CoreExceptionInit("Failed to get Program info");
      }
      programInfo_.init(info, info.programID);
      Id_ = info.programID;
    }
  }
  // Constructor Initialized
  shContext context_;
  QID dev_;
  shQpc qpcObj_;
  shQProgram program_;
  const QAicProgramProperties *properties_;

  // Initialized by Factory
  BufferMappings bufferMappings_;
  ProgramInfo programInfo_;
  QAicProgramID Id_;
  std::string programName_ = "";
};
///\}
} // namespace openrt
} // namespace qaic

#endif // QAIC_OPENRT_PROGRAM_HPP
