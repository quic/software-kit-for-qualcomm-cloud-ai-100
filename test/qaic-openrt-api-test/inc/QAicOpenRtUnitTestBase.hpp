// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include <iostream>
#include <gtest/gtest.h>
#include <unistd.h>
#include <libgen.h>
#include "QLog.h"
#include "QLogger.h"
#include "QAicRuntimeTypes.h"

// using namespace qlog;

class QAicTestConfig {
public:
  QAicTestConfig() = default;
  static QAicTestConfig &get() {
    static QAicTestConfig qaicTestConfig_;
    return qaicTestConfig_;
  }

  void InitTest(int argc, char *argv[]);

  QID getQID() { return qid_; }
  void usage(const char *name) {
    std::cout << name << std::endl;
    std::cout << " -d   : specify QID. [default: 0] " << std::endl;
    std::cout << " -v   : verbose" << std::endl;
    std::cout << " -s   : silence verbosity" << std::endl;
    std::cout << " -h   : help\n" << std::endl;
    std::cout << "Note: To use a custom library path, set env var "
                 "QAIC_LIB_PATH to the base path of the lib"
              << std::endl;
  }

  uint32_t getVerbosity() { return verbose_; }

private:
  const QID defaultQid_ = 0;
  QID qid_ = defaultQid_;
  uint32_t verbose_ = 0;
}; //

//----------------------------------------------
// Base Class for All OpenRt Unit Tests
//----------------------------------------------
class QAicOpenRtUnitTestBase : public ::testing::Test, virtual public QLogger {
public:
  QAicOpenRtUnitTestBase(){};
  virtual ~QAicOpenRtUnitTestBase() = default;
  void setup() {
    char execPath[PATH_MAX] = {"\0"};
    if (readlink("/proc/self/exe", execPath, PATH_MAX - 1) == -1) {
      LogError("Failed to get PID");
    }
    baseDataPath_ = std::string(dirname(execPath));
  }

  void teardown(){};
  std::string &getBasePath();

protected:
  const std::string testName() {
    return ::testing::UnitTest::GetInstance()->current_test_info()->name();
  }

private:
  std::string baseDataPath_;
};