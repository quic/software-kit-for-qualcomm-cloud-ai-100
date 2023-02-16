// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QAIC_OPENRT_QPC_HPP
#define QAIC_OPENRT_QPC_HPP

#include "QLogger.h"
#include "QAicOpenRtExceptions.hpp"
#include "QAicRuntimeTypes.h"
#include "QAicOpenRtUtil.hpp"
#include "QProgramContainer.h"
#include <vector>

namespace qaic {
namespace openrt {

class Qpc;
using shQpc = std::shared_ptr<Qpc>;

/// \brief Represents a Qualcomm Program Container object that
/// can be used to create programs.
/// It is recommended to create programs from a Qpc object, rather
/// than from a buffer and size pointing to the Qpc, which is the legacy
/// method.
/// Creating a Qpc object from the qpc buffer and size allows the
/// driver to track the use of the Qpc by reference and optimize sharing
/// of resources the same QPC.
/// The program will remain loaded on-target as long as at least
/// one program holds a copy of the QPC shared pointer object.
/// The Qpc class provides multiple factory methods for construction
/// including loading a QPC from file, using QAicApiUtil.
class Qpc : private Logger {
public:
  /// \brief Intantiate a shared pointer Qpc object using QPC buffer and size.
  /// \param[in] qpcBuf A Qpc Buffer.
  /// \param[in] qpcSize The size of the Qpc buffer.
  /// \param[in] makeCopy Option to make a local copy of the Qpc Buffer.
  /// If the user will retain the Qpc Buffer data for the life of this object
  /// this option can be set to false, otherwise should be set to true, and a
  /// local copy of the Qpc Buffer will be made.
  /// \return Shared pointer of Qpc type.
  /// \exception CoreExceptionNullPtr
  /// - When memory allocation for object fails
  /// \exception CoreExceptionInit
  /// - When input Parameters are invalid
  /// - When out of memory
  /// - When error in opening the Program Container
  static shQpc Factory(const uint8_t *qpcBuf, size_t qpcSize,
                       bool makeCopy = false) {
    shQpc obj = shQpc(new (std::nothrow) Qpc(qpcBuf, qpcSize, makeCopy));
    if (!obj) {
      throw CoreExceptionNullPtr("QpcObj");
    }
    // Init will throw an exception on failures
    obj->init();
    return obj;
  };

  /// \brief Instantiate a shared pointer Qpc object using QPC from File
  /// \param[in] basePath Base directory where to find the QPC file
  /// \param[in] qpcFilename Optional qpc filename override
  /// \return Shared pointer of Qpc type
  /// \exception CoreExceptionNullPtr
  /// - When memory allocation for object fails
  /// \exception CoreExceptionInit
  /// - When input Parameters are invalid
  /// - When out of memory
  /// - When error in opening the Program Container
  static shQpc Factory(const std::string basePath,
                       const std::string qpcFilename = "programqpc.bin") {
    QpcFile qpcFile(basePath.c_str(), qpcFilename.c_str());
    if (!qpcFile.load()) {
      return nullptr;
    }
    const QData qpcData = qpcFile.getBuffer();
    // Now create a QPC object with the data, and make a copy.
    shQpc obj = shQpc(new (std::nothrow) Qpc(qpcData.data, qpcData.size, true));
    if (!obj) {
      throw CoreExceptionNullPtr("QpcObj");
    }
    obj->init();
    return obj;
  };

  /// \brief Destructor ensures that qpc object is closed
  /// and that all associated resources are released
  ~Qpc() {
    QStatus status = QS_SUCCESS;
    if (qpcObj_ != nullptr) {
      status = QProgramContainer::close(qpcObj_);
      if (status != QS_SUCCESS) {
        std::cerr << "Failed to close Qpc object" << std::endl;
      }
      qpcObj_ = nullptr;
    }
  }

  /// \brief Get information about the Qpc
  /// \return Qpc Info Pointer
  shQpcInfo getInfo() const { return qpcInfo_; }

  /// \brief Get Buffer Mappings
  /// Buffer mappings provide details on the name, size, direction
  /// and index of each buffer in the inference vector
  /// \exception CoreExceptionRuntime
  /// - When programIndex is invalid
  const BufferMappings &getBufferMappings(uint32_t programIndex = 0) const {
    if (programIndex >= qpcInfo_->program.size()) {
      throw CoreExceptionRuntime("getBufferMappings: invalid index:" +
                                 std::to_string(programIndex));
    }
    return qpcInfo_->program.at(programIndex).bufferMappings;
  }

  /// \brief Get Io Descriptor
  QStatus getIoDescriptor(QData *ioDesc) {
    return qpcObj_->getIoDescriptor(ioDesc);
  }

  /// \brief Get Buffer Mappings for DMA
  /// Buffer mappings provide details on the name, size, direction
  /// and index of each buffer in the inference vector
  /// \exception CoreExceptionRuntime
  /// - When programIndex is invalid
  const BufferMappings &getBufferMappingsDma(uint32_t programIndex = 0) const {
    if (programIndex >= qpcInfo_->program.size()) {
      throw CoreExceptionRuntime("getBufferMappingsDma: invalid index:" +
                                 std::to_string(programIndex));
    }
    return qpcInfo_->program.at(programIndex).bufferMappingsDma;
  }

  const uint8_t *buffer() const { return qpcBuf_; }

  size_t size() const { return qpcSize_; }

  shQProgramContainer getQpc() const { return qpcObj_; }

  Qpc(const Qpc &) = delete;            // Disable Copy Constructor
  Qpc &operator=(const Qpc &) = delete; // Disable Assignment Operator
private:
  Qpc(const uint8_t *qpcBuf, size_t qpcSize, bool makeCopy)
      : qpcBuf_(qpcBuf), qpcSize_(qpcSize), makeCopy_(makeCopy),
        qpcObj_(nullptr), qpcInfo_(nullptr) {}

  void init() {

    QStatus status = QS_SUCCESS;

    qpcObj_ = QProgramContainer::open(qpcBuf_, qpcSize_, status);
    if ((qpcObj_ == nullptr) || (status != QS_SUCCESS)) {
      throw CoreExceptionInit("QpcObj: Failed to open QPC");
    }

    qpcInfo_ = qpcObj_->getInfo();

    if (qpcInfo_ == nullptr) {
      throw CoreExceptionInit("QpcObj: Failed to get QPC info");
    }
  }

  // Constructor Initialized
  const uint8_t *qpcBuf_;
  size_t qpcSize_;
  bool makeCopy_;
  shQProgramContainer qpcObj_;
  shQpcInfo qpcInfo_;
};

} // namespace openrt
} // namespace qaic

#endif // QAIC_OPENRT_QPC_HPP
