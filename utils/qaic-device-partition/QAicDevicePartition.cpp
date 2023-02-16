// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QRuntimePlatformApi.h"
#include "QAicOpenRtVersion.hpp"
#include "QAicOpenRtUtil.hpp"
#include "metadataflatbufDecode.hpp"
#include "metadataflatbufEncode.hpp"
#include "QLogger.h"
#include "QAicRuntimeTypes.h"
#include "QUtil.h"
#include "QOsal.h"
#include "QLog.h"
#include "QAicQpc.h"
#include "nlohmann/json.hpp"
#include "elfio/elfio.hpp"

#include <getopt.h>
#include <iostream>
#include <unistd.h>

namespace qaicpart {

constexpr char gKeyDdrSize[] = "ddrSize";
constexpr char gKeyNumNsp[] = "numNsp";
constexpr char gKeyNumMcid[] = "numMcid";
constexpr char gKeyNumSem[] = "numSem";
constexpr char gKeyNumVc[] = "numVc";
constexpr char gKeyL2Region[] = "l2Region";
constexpr char gKeyNumNetworks[] = "numNetworks";
constexpr size_t offset = 0;

class QAicDevicePartition {
public:
  QAicDevicePartition(const std::string &partConfigFilePath);
  QAicDevicePartition(const std::vector<std::string> &programQpcList);
  [[nodiscard]] QStatus init(int32_t qaicDeviceId); // -1 - All devices

private:
  uint8_t addVals_;
  qaic::QRuntimeInterface *rt_;
  std::vector<QID> devList_;
  uint32_t partitionsPerDev_ = 0;
  nlohmann::json partConfigJson_;
  std::string partConfigFilePath_;
  std::vector<std::string> programQpcList_;
  std::vector<std::pair<QID, QResourceReservationID>> resIdList_;

  [[nodiscard]] QStatus populateDevList();
  [[nodiscard]] QStatus createReservationsFromConfig();
  [[nodiscard]] QStatus createReservationsFromQpcList();
  [[nodiscard]] QStatus createResourceReservedDevices();
  [[nodiscard]] QStatus initFromQpcList(int32_t qaicDeviceId);
  [[nodiscard]] QStatus initFromPartConfig(int32_t qaicDeviceId);
  [[nodiscard]] QStatus readBinaryFile(const std::string &path,
                                       std::unique_ptr<uint8_t[]> &qpcBuf,
                                       size_t &qpcSize);
};

QAicDevicePartition::QAicDevicePartition(const std::string &partConfigFilePath)
    : partConfigFilePath_(partConfigFilePath) {}

QAicDevicePartition::QAicDevicePartition(
    const std::vector<std::string> &programQpcList)
    : programQpcList_(programQpcList) {}

QStatus QAicDevicePartition::populateDevList() {
  qaic::openrt::Util util;
  QStatus status = QS_SUCCESS;
  status = util.getDeviceIds(devList_);
  return status;
}

QStatus QAicDevicePartition::readBinaryFile(const std::string &path,
                                            std::unique_ptr<uint8_t[]> &qpcBuf,
                                            size_t &qpcSize) {
  std::ifstream ifs(path, std::ifstream::binary | std::ifstream::ate);
  if (!ifs) {
    LogErrorG("Failed to open qpc binary file");
    return QS_ERROR;
  }

  qpcSize = static_cast<size_t>(ifs.tellg());
  qpcBuf = std::make_unique<uint8_t[]>(qpcSize);
  if (qpcBuf == nullptr) {
    return QS_NOMEM;
  }

  ifs.seekg(0, std::ifstream::beg);
  ifs.read(reinterpret_cast<char *>(qpcBuf.get()), qpcSize);
  ifs.close();
  return QS_SUCCESS;
}

QStatus QAicDevicePartition::createReservationsFromQpcList() {
  QStatus status = QS_SUCCESS;
  uint8_t *networkData;
  size_t qpcSize;

  uint64_t reserveL2regionSize = 0;
  uint64_t reserveShddrSize = 0;
  uint32_t reserveNumMcid = 0;
  uint32_t reserveNumNsp = 0;
  uint32_t reserveNumSem = 0;
  size_t networkDataSize = 0;
  uint32_t reserveNumVc = 0;
  uint16_t numNetworks = 0;

  if (programQpcList_.empty()) {
    return QS_ERROR;
  }

  std::unique_ptr<uint8_t[]> qpcBuffer;
  for (const auto &qpcPath : std::as_const(programQpcList_)) {
    if (readBinaryFile(qpcPath, qpcBuffer, qpcSize)) {
      return QS_ERROR;
    }

    if (copyQpcBuffer(qpcBuffer.get(), qpcBuffer.get(), qpcSize) == nullptr) {
      return QS_ERROR;
    }

    if (!getQPCSegment(qpcBuffer.get(), "network.elf", &networkData,
                       &networkDataSize, offset)) {
      return QS_ERROR;
    }

    std::stringstream is(
        std::string(reinterpret_cast<char *>(networkData), networkDataSize));

    ELFIO::elfio elfReader;
    if (!elfReader.load(is)) {
      return QS_ERROR;
    }

    std::vector<uint8_t> flatbuf_bytes;
    if (elfReader.sections[metadata::networkElfMetadataFBSection]) {
      ELFIO::section *flatSec =
          elfReader.sections[metadata::networkElfMetadataFBSection];
      if (flatSec == nullptr) {
        return QS_ERROR;
      }
      flatbuf_bytes = std::vector<uint8_t>(
          flatSec->get_data(), flatSec->get_data() + flatSec->get_size());
    } else {
      ELFIO::section *metaSec = elfReader.sections["metadata"];
      if (metaSec == nullptr) {
        return QS_ERROR;
      }
      auto metadataOriginal = std::vector<uint8_t>(
          metaSec->get_data(), metaSec->get_data() + metaSec->get_size());
      flatbuf_bytes = metadata::FlatEncode::aicMetadataRawTranslateFlatbuff(
          metadataOriginal);
    }

    std::string metadataError;
    auto metadata = metadata::FlatDecode::readMetadataFlatNativeCPP(
        flatbuf_bytes, metadataError);
    if (!metadataError.empty()) {
      return QS_ERROR;
    }
    reserveNumNsp = std::max<uint64_t>(reserveNumNsp, metadata->numNSPs);
    // For oversubsciption feature, MCID resources need to be addition
    // of all the programs passed.
    reserveNumMcid += metadata->hostMulticastTable->multicastEntries.size();
    reserveNumSem = std::max<uint64_t>(reserveNumSem, metadata->numSemaphores);
    reserveShddrSize = std::max<uint64_t>(reserveShddrSize,
                                          metadata->staticSharedDDRSize +
                                              metadata->dynamicSharedDDRSize);
    reserveL2regionSize =
        std::max<uint64_t>(reserveL2regionSize, metadata->l2CachedDDRSize);
    reserveNumVc = std::max<uint64_t>(reserveNumVc, 1);
    numNetworks++;
  }

  for (auto devId : devList_) {
    QResourceReservationID resResId;

    status = rt_->createResourceReservation(
        devId, numNetworks, reserveNumNsp, reserveNumMcid, reserveNumSem,
        reserveNumVc, reserveShddrSize, reserveL2regionSize, 0, resResId,
        RESOURCE_GROUP_ID_DEFAULT);

    if (status == QS_SUCCESS) {
      resIdList_.push_back(std::make_pair<QID, QResourceReservationID>(
          std::move(devId), std::move(resResId)));
    } else {
      LogErrorG("Failed to create reservation {}, {}", devId, reserveNumNsp);
    }
  }

  // If the res id list is not empty we were able to create some partitions
  // The user will see errors for a failed reservation. So, in case some
  // partitions are available, return success
  if (!resIdList_.empty()) {
    status = QS_SUCCESS;
  }
  return status;
}

QStatus QAicDevicePartition::createReservationsFromConfig() {
  QStatus status = QS_SUCCESS;
  uint16_t numNetworks = 0;
  uint64_t reserveShddrSize = 0;
  uint32_t reserveNumNsp = 0;
  uint32_t reserveNumMcid = 0;
  uint32_t reserveNumSem = 0;
  uint32_t reserveNumVc = 0;
  uint64_t reserveL2regionSize = 0;
  QResourceReservationID resResId;

  for (const auto &[key, value] : partConfigJson_.items()) {
    if (key.compare(gKeyDdrSize) == 0) {
      reserveShddrSize = value;
    } else if (key.compare(gKeyNumNsp) == 0) {
      reserveNumNsp = value;
    } else if (key.compare(gKeyNumMcid) == 0) {
      reserveNumMcid = value;
    } else if (key.compare(gKeyNumSem) == 0) {
      reserveNumSem = value;
    } else if (key.compare(gKeyNumVc) == 0) {
      reserveNumVc = value;
    } else if (key.compare(gKeyL2Region) == 0) {
      reserveL2regionSize = value;
    } else if (key.compare(gKeyNumNetworks) == 0) {
      numNetworks = value;
    } else {
      LogErrorG("Invalid config detected {}", key);
      return QS_ERROR;
    }
  }

  for (auto devId : devList_) {
    status = rt_->createResourceReservation(
        devId, numNetworks, reserveNumNsp, reserveNumMcid, reserveNumSem,
        reserveNumVc, reserveShddrSize, reserveL2regionSize, 0, resResId,
        RESOURCE_GROUP_ID_DEFAULT);

    if (status == QS_SUCCESS) {
      resIdList_.push_back(
          std::make_pair(std::move(devId), std::move(resResId)));
    }
  }

  // If the res id list is not empty we were able to create some partitions
  // The user will see errors for a failed reservation. So, in case some
  // partitions are available, return success
  if (!resIdList_.empty()) {
    status = QS_SUCCESS;
  }
  return status;
}

QStatus QAicDevicePartition::createResourceReservedDevices() {
  QStatus status = QS_SUCCESS;
  for (auto resId : resIdList_) {
    status = rt_->createResourceReservedDevice(resId.first, resId.second);
  }
  return status;
}

QStatus
QAicDevicePartition::initFromPartConfig([[maybe_unused]] int32_t qaicDeviceId) {
  QStatus status = QS_SUCCESS;
  std::ifstream ifs(partConfigFilePath_);
  if (!ifs) {
    LogErrorG("Failed to open partition config file");
    return QS_ERROR;
  }
  ifs >> partConfigJson_;
  ifs.close();

  status = createReservationsFromConfig();
  if (status != QS_SUCCESS) {
    LogErrorG("Failed to create reservations from partition config");
    return status;
  }

  status = createResourceReservedDevices();
  if (status != QS_SUCCESS) {
    LogErrorG("Failed to create reserved devices");
  }

  return status;
}

QStatus
QAicDevicePartition::initFromQpcList([[maybe_unused]] int32_t qaicDeviceId) {
  QStatus status = QS_SUCCESS;
  status = createReservationsFromQpcList();
  if (status != QS_SUCCESS) {
    LogErrorG("Failed to create reservations from qpc binary");
    return status;
  }
  status = createResourceReservedDevices();
  if (status != QS_SUCCESS) {
    LogErrorG("Failed to create reserved devices");
  }
  return status;
}

QStatus QAicDevicePartition::init(int32_t qaicDeviceId) {
  rt_ = qaic::QRuntimeManager::getRuntime();
  if (rt_ == nullptr) {
    return QS_ERROR;
  }

  if (qaicDeviceId == -1) {
    if (populateDevList() != QS_SUCCESS) {
      return QS_ERROR;
    }
  } else {
    devList_.clear();
    devList_.push_back(qaicDeviceId);
  }

  if (partConfigFilePath_.empty()) {
    return initFromQpcList(qaicDeviceId);
  } else {
    return initFromPartConfig(qaicDeviceId);
  }
}

} // namespace qaicpart

using namespace qaicpart;

// clang-format off
static void usage() {
  printf(
       "Usage: qaic-partition [options]\n"
         "  -p, --partition-config <path>      File path to the partition config json.\n"
         "  -d, --aic-device-id                Optional qaic devices id. Default: Apply to all devices\n"
         "  -q, --program-qpc                  Path to the program QPC. this param can be repeated. The\n"
         "                                     reservation will correspond to the largest program requested.\n"
         "                                     'partition-config' takes precedence over this param.\n"
         "  -h, --help                         help\n");
}
// clang-format on

int main(int argc, char **argv) {
  std::string partConfigFilePath("");
  std::vector<std::string> programQpcList;
  std::shared_ptr<QAicDevicePartition> qAicDevicePart(nullptr);

  int32_t deviceId = -1;
  struct option long_options[] = {
      {"partition-config", required_argument, 0, 'p'},
      {"aic-device-id", optional_argument, 0, 'd'},
      {"program-qpc", optional_argument, 0, 'q'},
      {0, 0, 0, 0}};

  int option_index = 0;
  int opt;

  while ((opt = getopt_long(argc, argv, "p:d:q:r:sah", long_options,
                            &option_index)) != -1) {
    switch (opt) {
    case 'p':
      partConfigFilePath = std::string(optarg);
      break;
    case 'd':
      deviceId = atoi(optarg);
      break;
    case 'q':
      programQpcList.push_back(std::string(optarg));
      break;
    case 'h':
    default:
      usage();
      return 0;
    }
  }

  if (partConfigFilePath.empty()) {
    if (programQpcList.empty()) {
      std::cout << "Invalid config" << std::endl;
      usage();
      return 0;
    }
    qAicDevicePart = std::make_shared<QAicDevicePartition>(programQpcList);
  } else {
    qAicDevicePart = std::make_shared<QAicDevicePartition>(partConfigFilePath);
  }

  QStatus status = QS_SUCCESS;
  status = qAicDevicePart->init(deviceId);

  if (status != QS_SUCCESS) {
    std::cout << "Failed to create partitions!" << std::endl;
    return 1;
  }

  // Just pause the process execution. The process will exit when it gets a
  // signal
  // This call will ensure that no cpu cycles are consumed by the process.
  std::cout << "Running... " << std::endl;
  pause();
  return 0;
}
