// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QAIC_OPENRT_INFERENCE_VECTOR_HPP
#define QAIC_OPENRT_INFERENCE_VECTOR_HPP

#include "QLogger.h"
#include "QAicOpenRtExceptions.hpp"
#include "QAicOpenRtApiFwdDecl.hpp"
#include "QAicRuntimeTypes.h"
#include <random>

namespace qaic {
namespace openrt {

class InferenceVector : public Logger {
public:
  enum DataSourceType { ZERO_FILL, RANDOM, FILESET };
  /// \brief Generate an inference Vector compatible with the given program
  /// As default, will populate the input buffers with random data
  /// \param bufferMappings Buffer Mappings created from QPC
  /// \return Shared pointer of InferenceVector type
  /// \exception CoreExceptionNullPtr
  /// - When memory allocation for object fails
  /// \exception CoreExceptionInit
  /// - When input Parameters are invalid
  static shInferenceVector Factory(const BufferMappings &bufferMappings,
                                   DataSourceType source = RANDOM) {
    shInferenceVector obj = shInferenceVector(
        new (std::nothrow) InferenceVector(bufferMappings, source));
    if (!obj) {
      throw CoreExceptionNullPtr(objType_);
    }
    obj->init();
    return obj;
  }

  /// \brief Generate an inference Vector compatible with the given program
  /// As default, will populate the input buffers with random data
  /// \param bufferMappings Buffer Mappings created from QPC
  /// \return Shared pointer of InferenceVector type
  /// \exception CoreExceptionNullPtr
  /// - When memory allocation for object fails
  /// \exception CoreExceptionInit
  /// - When input Parameters are invalid
  static shInferenceVector Factory(const shQpc &qpc,
                                   DataSourceType source = RANDOM) {
    if (!qpc) {
      return nullptr;
    }
    return Factory(qpc->getBufferMappings(), source);
  }

  /// \brief Generate an inference Vector compatible with the given program
  /// from a list of files.
  /// \param qpc A previously created QPC shared pointer
  /// \param fileSet A set of files for each buffer. User may provide only
  /// input buffers. Other buffers will be initialized to zero.
  /// \return Shared pointer of InferenceVector type
  /// \exception CoreExceptionNullPtr
  /// - When memory allocation for object fails
  /// \exception CoreExceptionInit
  /// - When input Parameters are invalid
  /// \exception CoreExceptionRuntime
  /// - When fail to retrive buffermapping from QPC
  static shInferenceVector Factory(const BufferMappings &bufferMappings,
                                   const std::vector<std::string> &fileSet) {
    if (fileSet.empty()) {
      return Factory(bufferMappings);
    } else {
      shInferenceVector obj = shInferenceVector(
          new (std::nothrow) InferenceVector(bufferMappings, fileSet));
      if (!obj) {
        throw CoreExceptionNullPtr(objType_);
        return nullptr;
      }
      obj->init();
      return obj;
    }
  }

  /// \brief Generate an inference Vector compatible with the given program
  /// from a list of files.
  /// \param qpc A previously created QPC shared pointer
  /// \param fileSet A set of files for each buffer. User may provide only
  /// input buffers. Other buffers will be initialized to zero.
  /// \return Shared pointer of InferenceVector type
  /// \exception CoreExceptionNullPtr
  /// - When memory allocation for object fails
  /// \exception CoreExceptionInit
  /// - When input Parameters are invalid
  /// \exception CoreExceptionRuntime
  /// - When fail to retrive buffermapping from QPC
  static shInferenceVector Factory(const shQpc &qpc,
                                   const std::vector<std::string> &fileSet) {
    if (!qpc) {
      return nullptr;
    }
    return Factory(qpc->getBufferMappings(), fileSet);
  }

  /// \brief Get the QBuffer vector from this inferenceVector
  /// return QBufferVector
  const std::vector<QBuffer> &getVector() const { return qBufferVector_; }

  /// \brief Loads the input files associated with this inference vector.
  /// If network supports partialBuffer, then the size of input file can be
  /// smaller than that of expected input buffer size.
  /// \retval QS_SUCCESS Successful completion
  /// \retval QS_INVAL Due to either of following
  /// - Empty fileset
  /// - Number of input files do not meet network requirement
  /// - In network that doesnt support partialBuffer, size of input files
  /// do not meet network requirements
  QStatus loadFromFileSet(const std::vector<std::string> &fileSet) {
    fileSet_ = fileSet;
    return loadFileSet();
  }

  QStatus setBuffers(const std::vector<QBuffer> &buffers) {
    if (buffers.size() > qBufferVector_.size()) {
      std::stringstream ssMsg;
      ssMsg << "Number of Buffers provided in setBuffer is invalid, expected "
               "max: "
            << qBufferVector_.size() << " got " << buffers.size();
      LogError(ssMsg.str());
      return QS_INVAL;
    }

    for (uint32_t i = 0; i < buffers.size(); i++) {
      qBufferVector_.at(i) = buffers.at(i);
    }
    return QS_SUCCESS;
  }

  /// \brief Set the input buffers for this inferenceVector
  /// \retval QS_SUCCESS Successful completion
  /// \retval QS_INVAL Invalid Due to either of the following
  /// - Number of input buffers provided do not meet network requirements
  QStatus setInputBuffers(const std::vector<QBuffer> &inputBuffers) {
    uint32_t numBufferMappingsInputs = std::count_if(
        bufferMappings_.begin(), bufferMappings_.end(), [](BufferMapping m) {
          return m.ioType == QAicBufferIoTypeEnum::BUFFER_IO_TYPE_INPUT;
        });

    if (inputBuffers.size() != numBufferMappingsInputs) {
      std::stringstream ssMsg;
      ssMsg << "Number of expected input buffers " << numBufferMappingsInputs
            << "incompatible"
            << " got " << inputBuffers.size() << " expected "
            << numBufferMappingsInputs;
      LogError(ssMsg.str());
      return QS_INVAL;
    }

    // Note by convention user buffers are inputs first
    for (uint32_t i = 0; i < inputBuffers.size(); i++) {
      if (inputBuffers.at(i).size != bufferMappings_.at(i).size) {
        std::stringstream ssMsg;
        ssMsg << "Unexpected size for input buffer, expected "
              << bufferMappings_.at(i).size << " got "
              << inputBuffers.at(i).size << " for buffer Index: " << i;
        LogWarn(ssMsg.str());
      }
      qBufferVector_.at(i) = inputBuffers.at(i);
    }
    return QS_SUCCESS;
  }

  /// \brief Set the output buffers for this inferenceVector
  /// \retval QS_SUCCESS Successful completion
  /// \retval QS_INVAL Invalid Due to either of the following
  /// - Number of input buffers provided do not meet network requirements
  QStatus setOutputBuffers(const std::vector<QBuffer> &outputBuffers) {
    uint32_t numBufferMappingsOutput = std::count_if(
        bufferMappings_.begin(), bufferMappings_.end(), [](BufferMapping m) {
          return m.ioType == QAicBufferIoTypeEnum::BUFFER_IO_TYPE_OUTPUT;
        });

    uint32_t indexOfFirstOutputBuffer = 0;
    for (auto const &b : bufferMappings_) {
      if (b.ioType == QAicBufferIoTypeEnum::BUFFER_IO_TYPE_OUTPUT) {
        break;
      }
      indexOfFirstOutputBuffer++;
    }

    if (outputBuffers.size() != numBufferMappingsOutput) {
      std::stringstream ssMsg;
      ssMsg << "Number of expected output buffers " << numBufferMappingsOutput
            << "incompatible"
            << " got " << numBufferMappingsOutput << " expected "
            << outputBuffers.size();
      LogError(ssMsg.str());
      return QS_INVAL;
    }

    for (uint32_t i = indexOfFirstOutputBuffer, j = 0;
         (i < qBufferVector_.size()) && (j < outputBuffers.size()); i++, j++) {

      qBufferVector_.at(i) = outputBuffers.at(j);
    }
    return QS_SUCCESS;
  }

  ~InferenceVector() {}

  InferenceVector(const Qpc &) = delete; // Disable Copy Constructor
  InferenceVector &
  operator=(const Qpc &) = delete; // Disable Assignment Operator
private:
  InferenceVector(const BufferMappings &bufferMappings,
                  DataSourceType dataSourceType)
      : bufferMappings_(bufferMappings), sourceType_(dataSourceType) {}
  InferenceVector(const BufferMappings &bufferMappings,
                  const std::vector<std::string> &fileSet)
      : bufferMappings_(bufferMappings), sourceType_(FILESET),
        fileSet_(fileSet) {}

  void init() {
    // Allocate from Heap
    dataBufferVector_.resize(bufferMappings_.size());
    for (uint32_t i = 0; i < bufferMappings_.size(); i++) {
      dataBufferVector_.at(i).allocate(bufferMappings_.at(i).size);
    }

    qBufferVector_.resize(bufferMappings_.size());
    for (uint32_t i = 0;
         (i < dataBufferVector_.size()) && (i < qBufferVector_.size()); i++) {
      qBufferVector_.at(i) = dataBufferVector_.at(i).getBuffer();
    }

    switch (sourceType_) {
    case RANDOM: {
      fillRandomUnBounded();
    } break;
    case ZERO_FILL:
      for (auto &b : dataBufferVector_) {
        std::memset(b.dataPtr(), 0, b.size());
      }
      break;
    case FILESET:
      if (loadFileSet() != QS_SUCCESS) {
        throw CoreExceptionInit("Inferencevector: Failed to load file");
      }
      break;
    default:
      break;
    }
  }

  void fillRandomUnBounded() {
    std::default_random_engine engine;
    std::uniform_int_distribution<int> uintDistribution(0, 255);
    for (uint32_t i = 0; i < bufferMappings_.size(); i++) {
      if (bufferMappings_.at(i).ioType ==
          QAicBufferIoTypeEnum::BUFFER_IO_TYPE_OUTPUT) {
        continue;
      }
      uint8_t *dataPtr;
      dataPtr = reinterpret_cast<uint8_t *>(dataBufferVector_.at(i).dataPtr());
      for (size_t j = 0; j < dataBufferVector_.at(i).size(); j++) {
        dataPtr[j] = uintDistribution(engine);
      }
    }
  }

  QStatus loadFileSet() {
    QStatus status = QS_ERROR;
    uint32_t numBufferMappingsInputs = std::count_if(
        bufferMappings_.begin(), bufferMappings_.end(), [](BufferMapping m) {
          return m.ioType == QAicBufferIoTypeEnum::BUFFER_IO_TYPE_INPUT;
        });
    uint32_t numBufferMappingsOutputs = std::count_if(
        bufferMappings_.begin(), bufferMappings_.end(), [](BufferMapping m) {
          return m.ioType == QAicBufferIoTypeEnum::BUFFER_IO_TYPE_OUTPUT;
        });

    if (fileSet_.size() == 0) {
      if (numBufferMappingsInputs == 0) {
        return QS_SUCCESS;
      }
      std::cerr << "Empty fileSet" << std::endl;
      return QS_INVAL;
    }

    if (dataBufferVector_.size() == 0) {
      std::cerr << "Invalid Data Buffer Size 0" << std::endl;
      return QS_ERROR;
    }

    if (fileSet_.size() != 0) {
      if (numBufferMappingsInputs != fileSet_.size() &&
          ((numBufferMappingsInputs + numBufferMappingsOutputs) !=
           fileSet_.size())) {
        std::cerr
            << "Num files provided should match either number of input buffers "
            << "or sum of input and output buffers"
            << "\nExpected: numBufferMappingsInputs: "
            << numBufferMappingsInputs
            << " numBufferMappingsOutputs: " << numBufferMappingsOutputs
            << "\nAvailable: " << fileSet_.size() << std::endl;
        return QS_ERROR;
      }
      for (uint32_t i = 0, fileIndx = 0; i < bufferMappings_.size(); i++) {
        if (bufferMappings_.at(i).ioType ==
            QAicBufferIoTypeEnum::BUFFER_IO_TYPE_OUTPUT) {
          continue;
        }
        size_t fileSize = 0;
        if (!loadFile(fileSet_.at(fileIndx), dataBufferVector_.at(i).dataPtr(),
                      fileSize, dataBufferVector_.at(i).size())) {
          return QS_ERROR;
        }
        fileIndx++;
        // Partial loading. Resize the vector
        if (dataBufferVector_.at(i).size() > fileSize) {
          if (bufferMappings_.at(i).isPartialBufferAllowed == true) {
            status = dataBufferVector_.at(i).shrinkBuffer(fileSize);
            if (status != QS_SUCCESS) {
              return status;
            }
            qBufferVector_.at(i).size = fileSize;
          } else { // not partial buffer then file size should match
            std::cerr << "File size smaller than buffer size" << std::endl;
            return QS_ERROR;
          }
        }
      }
    }
    return QS_SUCCESS;
  }

  // Constructor Initialized
  const BufferMappings bufferMappings_;
  DataSourceType sourceType_;
  std::vector<std::string> fileSet_;

  // Uninitialized Locals
  std::vector<QBuffer> qBufferVector_;
  std::vector<DataBuffer<QBuffer>> dataBufferVector_;
  static constexpr const char *objType_ = "InferenceVector";
};

} // namespace openrt
} // namespace qaic

#endif // QAIC_OPENRT_INFERENCE_VECTOR_HPP
