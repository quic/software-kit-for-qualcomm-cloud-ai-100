// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QAIC_OPENRT_FILE_WRITER_HPP
#define QAIC_OPENRT_FILE_WRITER_HPP

#include "QLogger.h"
#include "QAicOpenRtExceptions.hpp"
#include "QAicOpenRtApiFwdDecl.hpp"
#include "QAicRuntimeTypes.h"

namespace qaic {
namespace openrt {

/// \brief File Writing utility provides capabilites to dump contents
/// of ExecObj buffer to file
class FileWriter : public Logger {
public:
  FileWriter(const std::string outputBasePath = ".")
      : outputBasePath_(outputBasePath) {}
  ~FileWriter() = default;

  /// \brief Set the base path for files being written
  /// \param[in] path Output path
  /// \retval QS_SUCCESS Successful completion
  /// \retval QS_ERROR Inaccessible outputBasePath
  QStatus setOutputPath(const std::string &outputBasePath) {
    std::unique_lock<std::mutex> lock(writeExecObjDataMutex_);
    if (access(outputBasePath.c_str(), W_OK) != 0) {
      logError("Unable to set path to " + outputBasePath + " Not accessible");
      return QS_ERROR;
    }
    outputBasePath_ = outputBasePath;
    return QS_SUCCESS;
  }

  /// \brief Write contents of Qbuffers with inference output data to file
  /// ./<bufferName>-activation-<activationIndex>-inf-<infNumber>.bin
  /// \param[in] execObj ExecObj that completed
  /// \param[in] bufferMappings Mappings can be obtained from either the
  /// qpc object or the program object, as a constant reference
  /// \param[in] infNumber  This is the numeral sequence of the execObj
  /// This value is used to uniquely identify the filename
  /// complete
  /// \param[in] verbose Output to std::out the filename that is being written
  /// \param[in] fileNameAppendString String to append in the output filename
  /// \retval QS_SUCCESS Successful completion
  /// \retval QS_ERROR Due to either of the below
  /// - QBuffers vector provided does not meet network requirements
  /// - Failed to open file to write output data
  QStatus writeExecObjData(const qaic::openrt::shExecObj &execObj,
                           const BufferMappings &bufferMappings,
                           const uint32_t &infNumber,
                           const bool &verbose = true,
                           const std::string &fileNameAppendString = "") {
    std::unique_lock<std::mutex> lock(writeExecObjDataMutex_);
    std::vector<QBuffer> buffers;
    execObj->getData(buffers);
    for (uint32_t idx = 0; idx < bufferMappings.size(); idx++) {
      if (bufferMappings.at(idx).ioType ==
          QAicBufferIoTypeEnum::BUFFER_IO_TYPE_OUTPUT) {
        std::string outputName = bufferMappings.at(idx).bufferName;
        // Some buffer names may include path separators, thus replace them.

        std::replace(outputName.begin(), outputName.end(), '/', '_');
        uint32_t buffIndex = bufferMappings.at(idx).index;
        if (buffIndex >= buffers.size()) {
          logError("Wrong BuffIndex : " + std::to_string(buffIndex) +
                   "numBuffers" + std::to_string(buffers.size()));
          return QS_ERROR;
        }
        if (buffers.size() == 0) {
          logError("User Buffer is not updated");
          return QS_ERROR;
        }
        std::string filename = outputName + fileNameAppendString + "-inf-" +
                               std::to_string(infNumber) + ".bin";
        std::string filepath = outputBasePath_ + "/" + filename;
        std::ofstream outfile;
        outfile.open(filepath, std::ios::binary | std::ios::out);
        if (verbose) {
          std::cout << "Writing file:" << filepath << std::endl;
        }
        if (!outfile.is_open()) {
          logError("Failed to open file: " + filepath);
          return QS_ERROR;
        }
        outfile.write((char *)buffers.at(buffIndex).buf,
                      buffers.at(buffIndex).size);
        outfile.flush();
        outfile.close();
      }
    }
    return QS_SUCCESS;
  }
  FileWriter(const FileWriter &) = delete; // Disable Copy Constructor
  FileWriter &
  operator=(const FileWriter &) = delete; // Disable Assignment Operator
private:
  std::string outputBasePath_;
  std::mutex writeExecObjDataMutex_;
};

} // namespace openrt
} // namespace qaic

#endif // QAIC_OPENRT_FILE_WRITER_HPP
