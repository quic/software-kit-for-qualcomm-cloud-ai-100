// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QAicOpenRtUnitTestBase.hpp"
#include "QAicOpenRtApi.hpp"
#include "QAic.h"

namespace QAicOpenRtUnitTest {

class QAicOpenRtApiContextUnitTest : public QAicOpenRtUnitTestBase {

public:
  QAicOpenRtApiContextUnitTest(){};
  virtual ~QAicOpenRtApiContextUnitTest() = default;

  QAicOpenRtApiContextUnitTest(const QAicOpenRtApiContextUnitTest &) =
      delete; // Disable Copy Constructor
  QAicOpenRtApiContextUnitTest &
  operator=(const QAicOpenRtApiContextUnitTest &) =
      delete; // Disable Assignment Operator

protected:
  void TestContextLoggingLevel();
  void TestCreateContext();
}; // class QAicOpenRtApiContextUnitTest

void QAicOpenRtApiContextUnitTest::TestCreateContext() {
  std::vector<QID> devIds;
  QAicContextProperties properties = 0x00;
  qaic::openrt::Util util;
  ASSERT_TRUE(util.getDeviceIds(devIds) == QS_SUCCESS);
  qaic::openrt::shContext context =
      qaic::openrt::Context::Factory(&properties, devIds);
  ASSERT_TRUE(context != nullptr);
}

void QAicOpenRtApiContextUnitTest::TestContextLoggingLevel() {
  std::vector<QID> devIds;
  QAicContextProperties properties = 0x00;
  qaic::openrt::Util util;
  QLogLevel logLevel;
  ASSERT_TRUE(util.getDeviceIds(devIds) == QS_SUCCESS);
  qaic::openrt::shContext context =
      qaic::openrt::Context::Factory(&properties, devIds);
  ASSERT_TRUE(context != nullptr);

  context->setLogLevel(QL_ERROR);
  context->getLogLevel(logLevel);
  ASSERT_TRUE(logLevel == QL_ERROR) << "Got log level " << logLevel;

  context->setLogLevel(QL_WARN);
  context->getLogLevel(logLevel);
  ASSERT_TRUE(logLevel == QL_WARN) << "Got log level " << logLevel;

  context->setLogLevel(QL_INFO);
  context->getLogLevel(logLevel);
  ASSERT_TRUE(logLevel == QL_INFO) << "Got log level " << logLevel;
}

TEST_F(QAicOpenRtApiContextUnitTest, ChangeLogLevelTest) {
  TestContextLoggingLevel();
}

TEST_F(QAicOpenRtApiContextUnitTest, CreateContextTest) { TestCreateContext(); }

} // namespace QAicOpenRtContextUnitTest