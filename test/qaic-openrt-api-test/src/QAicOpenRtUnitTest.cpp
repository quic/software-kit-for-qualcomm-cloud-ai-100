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

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  QAicTestConfig::get().InitTest(argc, argv);
  return RUN_ALL_TESTS();
}