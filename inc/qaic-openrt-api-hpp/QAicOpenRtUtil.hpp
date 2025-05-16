// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
#ifndef QAICAPI_UTIL_HPP
#define QAICAPI_UTIL_HPP

#include "QAicOpenRtApiFwdDecl.hpp"
#include "QAicOpenRtLogger.hpp"
#include "QAicOpenRtExceptions.hpp"
#include "QRuntimeManager.h"
#include "QRuntimeInterface.h"

#include <vector>
#include <functional>
#include <string>
#include <libgen.h>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <iomanip>

namespace qaic {
namespace openrt {

/// \defgroup  qaicapihpp-util
/// \{
/// Utility Classes
/// \}

/// \addtogroup qaicapihpp-util Runtime C++ Utility API
/// \ingroup qaicapihpp
/// \{

template <class T> class DataBuffer {
public:
  DataBuffer() : allocationSize_(0) { init(); }
  ~DataBuffer() { release(); }
  const T &getBuffer() { return buffer; }

  size_t size() { return buffer.size; }
  uint8_t *dataPtr();
  void allocate(size_t size);
  QStatus shrinkBuffer(size_t size);
  void release();

private:
  T buffer;
  std::vector<uint8_t> data;
  size_t allocationSize_;
  void init();
};

template <> inline void DataBuffer<QBuffer>::init() {
  buffer.buf = nullptr;
  buffer.size = 0;
  buffer.handle = 0;
  buffer.type = QBufferType::QBUFFER_TYPE_HEAP;
  buffer.offset = 0;
}

template <> inline void DataBuffer<QData>::init() {
  buffer.data = nullptr;
  buffer.size = 0;
}

template <> inline uint8_t *DataBuffer<QBuffer>::dataPtr() {
  return buffer.buf;
}

template <> inline uint8_t *DataBuffer<QData>::dataPtr() { return buffer.data; }

template <> inline void DataBuffer<QBuffer>::allocate(size_t size) {
  data.resize(size);
  buffer.buf = data.data();
  buffer.handle = 0;
  buffer.type = QBufferType::QBUFFER_TYPE_HEAP;
  buffer.offset = 0;
  buffer.size = size;
  allocationSize_ = size;
}

template <> inline void DataBuffer<QData>::allocate(size_t size) {
  data.resize(size);
  buffer.data = data.data();
  buffer.size = size;
  allocationSize_ = size;
}

template <> inline void DataBuffer<QBuffer>::release() {
  if (buffer.buf != nullptr) {
    data.resize(0);
    buffer.buf = nullptr;
    buffer.size = 0;
    buffer.handle = 0;
    buffer.type = QBufferType::QBUFFER_TYPE_HEAP;
    buffer.offset = 0;
    allocationSize_ = 0;
  }
}

template <> inline void DataBuffer<QData>::release() {
  if (buffer.data != nullptr) {
    data.resize(0);
  }
  buffer.data = nullptr;
  buffer.size = 0;
  allocationSize_ = 0;
}

template <> inline QStatus DataBuffer<QBuffer>::shrinkBuffer(size_t size) {
  if (size > allocationSize_) {
    return QS_ERROR;
  }
  data.resize(size);
  buffer.buf = data.data();
  return QS_SUCCESS;
}

template <> inline QStatus DataBuffer<QData>::shrinkBuffer(size_t size) {
  if (size > allocationSize_) {
    return QS_ERROR;
  }
  data.resize(size);
  buffer.data = data.data();
  buffer.size = size;
  return QS_SUCCESS;
}

inline size_t getFileSize(const std::string &filePath) {
  uint64_t fileSize;
  uint64_t loadSize;
  std::ifstream infile;
  infile.open(filePath, std::ios::binary | std::ios::in);
  if (!infile.is_open()) {
    throw CoreExceptionRuntime("Failed to open file" + filePath);
    return 0;
  }
  infile.seekg(0, infile.end);
  loadSize = fileSize = infile.tellg();
  infile.seekg(0, infile.beg);
  return loadSize;
}

inline bool loadFile(const std::string &filePath, uint8_t *dest,
                     size_t &fileSize, size_t destBufferSize,
                     size_t offset = 0) {
  uint64_t loadSize;
  std::ifstream infile;
  if (destBufferSize == 0) {
    return false;
  }
  infile.open(filePath, std::ios::binary | std::ios::in);
  if (!infile.is_open()) {
    throw CoreExceptionRuntime("Failed to open file" + filePath);
    return false;
  }
  infile.seekg(0, infile.end);
  loadSize = fileSize = infile.tellg();
  infile.seekg(0, infile.beg);

  if (destBufferSize < fileSize + offset) {
    throw CoreExceptionRuntime("Invalid input file size, expected " +
                               std::to_string(destBufferSize - offset) +
                               " but " + std::to_string(fileSize) +
                               " is provided.");
    return false;
  }
  infile.read((char *)dest + offset, loadSize);
  if (!infile) {
    throw CoreExceptionRuntime("Failed to read file: " + filePath);
    return false;
  }
  return true;
}

/// \brief File IO to read a QPC from file
class QpcFile {
public:
  QpcFile(const std::string &basePath,
          const std::string &qpcFilename = "programqpc.bin")
      : basePath_(basePath), qpcFilename_(qpcFilename) {}

  bool load() {
    std::string filePath = basePath_ + "/" + qpcFilename_;
    if (!loadFile(filePath)) {
      return false;
    }
    return true;
  }

  const QData &getBuffer() { return dataBuffer_.getBuffer(); }

  const uint8_t *data() { return dataBuffer_.getBuffer().data; }

  size_t size() { return dataBuffer_.getBuffer().size; }

  ~QpcFile() {}

  QpcFile(const QpcFile &) = delete;            // Disable Copy Constructor
  QpcFile &operator=(const QpcFile &) = delete; // Disable Assignment Operator
private:
  bool loadFile(const std::string &filePath) {
    uint64_t fileSize;
    std::ifstream infile;
    infile.open(filePath, std::ios::binary | std::ios::in);
    if (!infile.is_open()) {
      throw CoreExceptionInit(objType_, "Failed to open file" + filePath);
      return false;
    }
    infile.seekg(0, infile.end);
    fileSize = infile.tellg();
    infile.seekg(0, infile.beg);
    dataBuffer_.allocate(fileSize);
    infile.read((char *)dataBuffer_.dataPtr(), fileSize);
    if (!infile) {
      throw CoreExceptionInit(objType_, "Failed to read file" + filePath);
      return false;
    }
    return true;
  }
  const std::string basePath_;
  const std::string qpcFilename_;
  DataBuffer<QData> dataBuffer_;
  static constexpr const char *objType_ = "QpcFile";
};

/// \defgroup  qaicapihpp-util
/// \{
/// Utility classes to get information on the AIC device
/// \}

/// \addtogroup qaicapihpp-util Runtime C++ Utility API
/// \ingroup qaicapihpp
/// \{
/// \brief Utility class for basic operations such as getting device ID and
/// device information
class Util : public Logger {
public:
  Util(){};
  /// \brief Get list of available devices from the system, pass a
  /// pre-allocated
  /// array of QIDs.
  /// \param[out] devices Allocated array of maxDevices size to store devices.
  /// \retval QS_SUCCESS Successful completion
  /// \retval QS_INVAL Internal error in getting device list
  /// \retval QS_NODEV No valid device
  /// \retval QS_ERROR Driver init failed or Bad device AddrInfo
  QStatus getDeviceIds(std::vector<QID> &devList) {

    QStatus status;
    QRuntimeInterface *rt;

    rt = QRuntimeManager::getRuntime();
    if (rt == nullptr) {
      LogErrorG("Failed to initialize driver");
      return QS_ERROR;
    }

    std::vector<qutil::DeviceAddrInfo> deviceInfoList;
    status = rt->getDeviceAddrInfo(deviceInfoList);
    if (status != QS_SUCCESS) {
      return status;
    }

    for (const auto &deviceInfo : deviceInfoList) {
      devList.emplace_back(deviceInfo.first);
    }

    return QS_SUCCESS;
  }

  /// \brief Get Device information for one device
  /// \param device A device ID that was returned as valid from
  /// qaicGetDeviceIds
  /// \param devInfo A local structure to store the device info
  /// \return QS_SUCCESS Successful completion
  /// \retval QS_INVAL Invalid arguments: \a devInfo
  /// \retval QS_NODEV No valid device
  /// \retval QS_ERROR Driver init failed
  /// \retval QS_AGAIN Try Again
  QStatus getDeviceInfo(QID device, QDevInfo &devInfo) {
    QStatus status;
    QRuntimeInterface *rt = nullptr;
    std::string devName("");

    memset(&devInfo, 0, sizeof(QDevInfo));
    rt = QRuntimeManager::getRuntime();
    if (rt == nullptr) {
      LogErrorG("Failed to initialize driver");
      return QS_ERROR;
    }
    status = rt->queryStatus(device, devInfo);
    if (status == QS_SUCCESS) {
      status = rt->getDeviceName(device, devName);
      if (status == QS_SUCCESS) {
        uint32_t count = 0;
        devInfo.name[sizeof(devInfo.name) - 1] = '\0';
        count = ((sizeof(devInfo.name) - 1) < devName.length())
                    ? (sizeof(devInfo.name) - 1)
                    : devName.length();
        std::memcpy(devInfo.name, devName.c_str(), count);
      }
    }
    return status;
  }

  /// \brief Get Device resource information, this is a simple query that
  /// returns status of dynamic resources such as free memory, etc.
  /// Note the same information is available through GetDeviceInfo, however
  /// this is a more lightweight api allowing to retrieve only the resource
  /// info.
  /// \param device A device ID that was returned as valid from
  /// qaicGetDeviceIds
  /// \param resourceInfo A local structure to store the resource info
  /// \retval QS_SUCCESS Successful completion
  /// \retval QS_INVAL Invalid arguments: \a resourceInfo
  /// \retval QS_ERROR Driver init failed
  /// \retval QS_NODEV Device not found, the device ID provided is not
  /// available
  /// \retval QS_DEV_ERROR Device validity error
  QStatus getResourceInfo(QID device, QResourceInfo &resourceInfo) {
    QStatus status = QS_ERROR;
    QRuntimeInterface *rt;

    rt = QRuntimeManager::getRuntime();
    if (rt == nullptr) {
      LogErrorG("Failed to initialize driver");
      return QS_ERROR;
    }
    status = rt->getResourceInfo(device, resourceInfo);
    return status;
  }

  /// \brief Get Device performance information, this is a simple query that
  /// returns performance info.
  /// Note the same information is available through GetDeviceInfo, however
  /// this is a more lightweight api allowing to retrieve only the performance
  /// info.
  /// \param device A device ID that was returned as valid from
  /// qaicGetDeviceIds
  /// \param performanceInfo A local structure to store the performance info
  /// \retval QS_SUCCESS Successful completion
  /// \retval QS_INVAL Invalid arguments: \a performanceInfo
  /// \retval QS_NODEV Device not found, the device ID provided is not
  /// available
  /// \retval QS_ERROR Driver init failed
  /// \retval QS_DEV_ERROR Device validity error
  QStatus getPerformanceInfo(QID device, QPerformanceInfo &performanceInfo) {
    QStatus status = QS_ERROR;
    QRuntimeInterface *rt;

    rt = QRuntimeManager::getRuntime();
    if (rt == nullptr) {
      LogErrorG("Failed to initialize driver");
      return QS_ERROR;
    }
    status = rt->getPerformanceInfo(device, performanceInfo);
    return status;
  }
};

/// \addtogroup qaicapihpp-util Runtime C++ Utility Filesystem API
/// \ingroup qaicapihpp
/// \{
/// \brief Utility class for basic filesystem operations
class UtilFs {
public:
  static QStatus createDirectory(const std::string &dir) {
    std::string mkdirString = "mkdir -p \"" + dir + "\"";
    const int rc = system(mkdirString.c_str());
    if (rc < 0) {
      std::cerr << "Failed to create directory: " << dir << std::endl;
      return QS_ERROR;
    }
    return QS_SUCCESS;
  }

  static bool fileExists(const std::string &filepath) {
    std::ifstream f(filepath.c_str());
    return f.good();
  }
};
///\}
} // namespace rt
} // namespace qaic

#endif
