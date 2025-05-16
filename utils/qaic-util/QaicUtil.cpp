// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QRuntimePlatformApi.h"
#include "QAicOpenRtVersion.hpp"
#include "QLogger.h"
#include "QAicRuntimeTypes.h"
#include "QUtil.h"
#include "QOsal.h"
#include "QLog.h"

#include <getopt.h>
#include <iostream>
#include <unistd.h>

int txUIDisplayTable(uint32_t tableRefreshRate,
                     const std::vector<QID> &devList); /* External function */

using namespace qaic;
using namespace qaic::openrt;

static void decodeQDevInfo(QDevInfo *devInfo) {
  std::cout << qutil::str(*devInfo);
}

static std::string getAicVersionInfo() {
  return fmt::format("OPEN_RUNTIME.{}.{}.{}_{}", VersionInfo::majorVersion(),
                     VersionInfo::minorVersion(), VersionInfo::patchVersion(),
                     VersionInfo::variant());
}

static int queryDevice(shQRuntimePlatformInterface rt, QID qid) {
  QDevInfo devInfo;
  QStatus status = QS_SUCCESS;

  std::memset(&devInfo, 0, sizeof(QDevInfo));
  status = rt->queryStatus(qid, devInfo);
  if (status == QS_NODEV) {
    std::cout << "Failed to query Device ID: " << qid << std::endl;
    return status;
  }

  decodeQDevInfo(&devInfo);

  // Get the board serial. We will remove this code on the next aic api major
  // rev
  QBoardInfo boardInfo;
  status = rt->getBoardInfo(qid, boardInfo);
  if (status == QS_SUCCESS) {
    std::cout << fmt::format("\tBoard serial:{}", boardInfo.boardSerial)
              << std::endl;
  }

  QTelemetryInfo telemetryInfo;
  status = rt->getTelemetryInfo(qid, telemetryInfo);

  if (status == QS_SUCCESS) {
    std::cout << fmt::format("\tSoC temperature(degree C):{:.2f}",
                             ((float)telemetryInfo.socTemperature / 1000))
              << std::endl;
    std::cout << fmt::format("\tBoard power(Watts):{:.2f}",
                             ((float)telemetryInfo.boardPower / 1000000))
              << std::endl;
    std::cout << fmt::format("\tTDP Cap(Watts):{:.2f}",
                             ((float)telemetryInfo.tdpCap / 1000000))
              << std::endl;
  }
  return 0;
}

// clang-format off
static void usage() {
  printf(
       "Usage: qaic-util [options]\n"
         "  -d, --aic-device-id <id>                   AIC device ID. -d option can be used multiple times \n"
         "  -q, --query [-d QID]                       Query command to get device information, default All \n"
         "  -t, --table <refresh-rate-sec> [-d QID]    Display device information in tabular format. Data is refreshed\n"
         "                                             after every <refresh-rate-sec>. Ctrl+C to exit\n"
         "  -h, --help                                 help\n"
         );
}
// clang-format on

int main(int argc, char **argv) {
  int opt = 0, option_index = 0;
  std::vector<QID> devList;
  std::vector<QID> allDevList;
  const QID defaultDeviceQID = 0;
  bool isQuery = true, reservePC = false;
  uint32_t numPhyChannels = 0;
  uint32_t tableRefreshRate = 0;
  int rc = 0;

  struct option options[] = {{"aic-device-id", required_argument, 0, 'd'},
                             {"query", no_argument, 0, 'q'},
                             {"table", required_argument, 0, 't'},
                             {"help", no_argument, 0, 'h'},
                             {0, 0, 0, 0}};

  while ((opt = getopt_long(argc, argv, "c:d:t:qh", options, &option_index)) !=
         -1) {
    switch (opt) {
    case 'd':
      devList.push_back((QID)atoi(optarg));
      break;
    case 'q':
      isQuery = true;
      break;
    case 't':
      tableRefreshRate = atoi(optarg);
      break;
    case 'h':
    default: /* '?' */
      usage();
      return -1;
    }
  }

  shQRuntimePlatformInterface rt = rtPlatformApi::qaicGetRuntimePlatform();
  if (!rt) {
    std::cerr << "Failed to get Runtime Platform" << std::endl;
    return -1;
  } else {
    rt->getDeviceIds(allDevList);
  }

  if (!devList.empty()) {
    std::sort(allDevList.begin(), allDevList.end());
    for (auto &dev : devList) {
      if (!(std::binary_search(allDevList.begin(), allDevList.end(), dev))) {
        std::cerr << "Device " << dev << " invalid" << std::endl;
        return -1;
      }
    }
  } else {
    devList = allDevList;
  }

  if (tableRefreshRate) {
    /* Table dispaly runs in loop until Ctrl+C is pressed */
    return txUIDisplayTable(tableRefreshRate, devList);
  }

  if (isQuery) {
    for (const auto &id : devList) {
      queryDevice(rt, id);
    }
  }
  return rc;
}
