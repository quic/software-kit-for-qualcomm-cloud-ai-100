// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QAicOpenRtUnitTestBase.hpp"
#include "QAicOpenRtApi.hpp"
#include "QAic.h"

namespace QAicOpenRtUnitTest {

class QAicOpenRtApiProgramUnitTest : public QAicOpenRtUnitTestBase {

public:
  QAicOpenRtApiProgramUnitTest(){};
  virtual ~QAicOpenRtApiProgramUnitTest() = default;

  QAicOpenRtApiProgramUnitTest(const QAicOpenRtApiProgramUnitTest &) =
      delete; // Disable Copy Constructor
  QAicOpenRtApiProgramUnitTest &
  operator=(const QAicOpenRtApiProgramUnitTest &) =
      delete; // Disable Assignment Operator
protected:
  void TestCreateProgram(std::string);
  void TestLoadActivateProgram(std::string);

}; // class QAicOpenRtApiProgramUnitTest

void QAicOpenRtApiProgramUnitTest::TestCreateProgram(std::string testBasePath) {
  std::vector<QID> devIds;
  QAicContextProperties properties = 0x00;
  qaic::openrt::Util util;
  ASSERT_TRUE(util.getDeviceIds(devIds) == QS_SUCCESS);
  qaic::openrt::shContext context =
      qaic::openrt::Context::Factory(&properties, devIds);
  ASSERT_TRUE(context != nullptr);

  qaic::openrt::shQpc qpc;

  qpc = qaic::openrt::Qpc::Factory(testBasePath);

  ASSERT_TRUE(qpc);

  qaic::openrt::shProgram program;
  QAicProgramProperties programProperties;
  qaic::openrt::Program::initProperties(programProperties);

  program = qaic::openrt::Program::Factory(context, devIds.front(), "TestName",
                                           qpc, &programProperties);

  ASSERT_TRUE(program);
}

void QAicOpenRtApiProgramUnitTest::TestLoadActivateProgram(
    std::string testBasePath) {
  std::vector<QID> devIds;
  QAicContextProperties properties = 0x00;
  qaic::openrt::Util util;
  QResourceInfo devResourceInfo;
  uint8_t nspFreeInit = 0;
  ASSERT_TRUE(util.getDeviceIds(devIds) == QS_SUCCESS);
  qaic::openrt::shContext context =
      qaic::openrt::Context::Factory(&properties, devIds);
  ASSERT_TRUE(context != nullptr);

  qaic::openrt::shQpc qpc;

  qpc = qaic::openrt::Qpc::Factory(testBasePath);

  ASSERT_TRUE(qpc);

  qaic::openrt::shProgram program;
  QAicProgramProperties programProperties;
  qaic::openrt::Program::initProperties(programProperties);

  program = qaic::openrt::Program::Factory(context, devIds.front(), "TestName",
                                           qpc, &programProperties);

  ASSERT_TRUE(program);

  ASSERT_TRUE(program->load() == QS_SUCCESS) << "Program load failed";

  util.getResourceInfo(devIds.front(), devResourceInfo);

  nspFreeInit = devResourceInfo.nspFree;

  ASSERT_TRUE(program->activate() == QS_SUCCESS) << "Program activate failed";

  util.getResourceInfo(devIds.front(), devResourceInfo);

  ASSERT_TRUE(nspFreeInit > devResourceInfo.nspFree)
      << "No NSPs reserved on program activation";

  ASSERT_TRUE(program->deactivate() == QS_SUCCESS)
      << "Program deactivate failed";

  util.getResourceInfo(devIds.front(), devResourceInfo);

  ASSERT_TRUE(nspFreeInit == devResourceInfo.nspFree)
      << "NSPs not freed back on program de-activation";

  ASSERT_TRUE(program->unload() == QS_SUCCESS) << "Program unload failed";
}

TEST_F(QAicOpenRtApiProgramUnitTest, CreateProgramTest) {
  TestCreateProgram(
      "/opt/qti-aic/test-data/aic100/v2/4nsp/4nsp-quant-resnet50");
}

TEST_F(QAicOpenRtApiProgramUnitTest, LoadActivateProgramTest) {
  TestLoadActivateProgram(
      "/opt/qti-aic/test-data/aic100/v2/4nsp/4nsp-quant-resnet50");
}

} // namespace QAicOpenRtContextUnitTest