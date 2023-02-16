// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include <iostream>
#include <gtest/gtest.h>
#include <unistd.h>
#include <libgen.h>
#include "QAicRuntimeTypes.h"
#include "QRuntimePlatformApi.h"
#include "QAicOpenRtVersion.hpp"
#include "QLog.h"
#include "QAicOpenRtUnitTestBase.hpp"

using namespace qaic;

void QAicTestConfig::InitTest(int argc, char *argv[]) {

  QStatus status;
  int opt = 0;
  loglevel logLevel = loglevel::warn;
  QLogControlShared logControl = QLogControl::getLogControl();
  logControl->enableConsoleLog();
  optind = 1;
  while ((opt = getopt(argc, argv, "d:hvs")) != -1) {
    switch (opt) {
    case 'v':
      verbose_++;
      break;
    case 's':
      if (verbose_ > 0) {
        verbose_--;
      }
      break;

    case 'h':
      usage(argv[0]);
      exit(0);
    case 'd':
      qid_ = std::atoi(optarg);
      break;
    }
  }

  switch (verbose_) {
  case 0:
    logLevel = loglevel::warn;
    break;
  case 1:
    logLevel = loglevel::info;
    break;
  case 2:
    logLevel = loglevel::debug;
    break;
  default:
    if (verbose_ > 2) {
      logLevel = loglevel::trace;
    }
  }

  // Set level for any new logger we create
  logControl->setDefaultLogLevel(logLevel);
  // Set level for any existing loggers
  logControl->setLogLevel(logLevel);
  // set level for a particular loger
  logControl->setSinkLevel(log_sink_enum::console, logLevel);
  QDevInfo devInfo;

  shQRuntimePlatformInterface rt = rtPlatformApi::qaicGetRuntimePlatform();

  if (!rt) {
    std::cerr << "Failed to get Runtime Platform" << std::endl;
  }
  rt->queryStatus(qid_, devInfo);
  if (status == QS_NODEV) {
    std::cout << "Failed to query Device ID: " << qid_ << std::endl;
    exit(1);
  }
}