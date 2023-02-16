// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QAicOpenRtUnitTestBase.hpp"
#include "QAicOpenRtApi.hpp"
#include "QAic.h"

namespace QAicOpenRtUnitTest {

class QAicOpenRtApiExecObjUnitTest : public QAicOpenRtUnitTestBase {

public:
  QAicOpenRtApiExecObjUnitTest(){};
  virtual ~QAicOpenRtApiExecObjUnitTest() = default;

  QAicOpenRtApiExecObjUnitTest(const QAicOpenRtApiExecObjUnitTest &) =
      delete; // Disable Copy Constructor
  QAicOpenRtApiExecObjUnitTest &
  operator=(const QAicOpenRtApiExecObjUnitTest &) =
      delete; // Disable Assignment Operator

protected:
  enum class TestType { TEST_TYPE_NORMAL = 0, TEST_TYPE_ADVERSARIAL = 1 };
  void TestCreateExecObj(std::string);
  void TestRunInference(std::string, uint32_t,
                        TestType type = TestType::TEST_TYPE_NORMAL);
  void TestRunInferenceProgramPreActivated(std::string, uint32_t);
  void TestRunInferencePartialTensor(std::string, uint32_t);
}; // class QAicOpenRtApiExecObjUnitTest

void QAicOpenRtApiExecObjUnitTest::TestCreateExecObj(std::string testBasePath) {
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

  qaic::openrt::shExecObj execObj;
  execObj = qaic::openrt::ExecObj::Factory(context, program);

  ASSERT_TRUE(execObj);
}

void QAicOpenRtApiExecObjUnitTest::TestRunInference(std::string testBasePath,
                                                    uint32_t numInference,
                                                    TestType type) {
  std::vector<QID> devIds;
  QAicContextProperties properties = 0x00;
  qaic::openrt::Util util;
  QStatus status;
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

  qaic::openrt::shExecObj execObj;
  execObj = qaic::openrt::ExecObj::Factory(context, program);

  ASSERT_TRUE(execObj);

  qaic::openrt::shInferenceVector inferenceVect =
      qaic::openrt::InferenceVector::Factory(qpc);
  ASSERT_TRUE(inferenceVect);

  std::vector<QBuffer> data = inferenceVect->getVector();
  execObj->setData(data);

  for (auto i = 0; i < numInference; i++) {
    LogInfo("Starting inference number {}", i + 1);
    status = execObj->run();
    if (type == TestType::TEST_TYPE_ADVERSARIAL)
      ASSERT_TRUE(status != QS_SUCCESS) << "Adversarial test failed";
    else
      ASSERT_TRUE(status == QS_SUCCESS) << "Inference run fail";
  }
}

void QAicOpenRtApiExecObjUnitTest::TestRunInferenceProgramPreActivated(
    std::string testBasePath, uint32_t numInference) {
  std::vector<QID> devIds;
  QAicContextProperties properties = 0x00;
  qaic::openrt::Util util;
  QStatus status;
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

  ASSERT_TRUE(program->activate() == QS_SUCCESS) << "Program activate failed";

  qaic::openrt::shExecObj execObj;
  execObj = qaic::openrt::ExecObj::Factory(context, program);

  ASSERT_TRUE(execObj);

  qaic::openrt::shInferenceVector inferenceVect =
      qaic::openrt::InferenceVector::Factory(qpc);
  ASSERT_TRUE(inferenceVect);

  std::vector<QBuffer> data = inferenceVect->getVector();
  execObj->setData(data);

  for (auto i = 0; i < numInference; i++) {
    LogInfo("Starting inference number {}", i + 1);
    status = execObj->run();

    ASSERT_TRUE(status == QS_SUCCESS) << "Inference run fail";
  }
}

void QAicOpenRtApiExecObjUnitTest::TestRunInferencePartialTensor(
    std::string testBasePath, uint32_t numInference) {
  std::vector<QID> devIds;
  QAicContextProperties properties = 0x00;
  qaic::openrt::Util util;
  QStatus status;
  ASSERT_TRUE(util.getDeviceIds(devIds) == QS_SUCCESS);
  qaic::openrt::shContext context =
      qaic::openrt::Context::Factory(&properties, devIds);
  ASSERT_TRUE(context != nullptr);

  qaic::openrt::shQpc qpc;

  qpc = qaic::openrt::Qpc::Factory(testBasePath);

  ASSERT_TRUE(qpc);

  BufferMappings bufferMappings = qpc->getBufferMappings();
  bool partialBufferFound = false;

  for (auto const &m : bufferMappings) {
    std::string bufferDir;
    if ((m.ioType == QAicBufferIoTypeEnum::BUFFER_IO_TYPE_INPUT) &&
        (m.isPartialBufferAllowed == true)) {
      partialBufferFound = true;
      break;
    }
  }
  if (!partialBufferFound) {
    LogWarn("Network does not support Partial Tensor. Skipping the test");
    GTEST_SKIP();
    return;
  }

  std::vector<QBuffer> bufferVector;
  bufferVector.resize(qpc->getBufferMappings().size());
  for (uint32_t idx = 0; idx < bufferVector.size(); idx++) {
    std::vector<uint8_t> buf;
    if (bufferMappings.at(idx).isPartialBufferAllowed) {
      buf.resize((bufferMappings[idx].size) / 2);
      bufferVector.at(idx).size = (bufferMappings[idx].size) / 2;
    } else {
      buf.resize(bufferMappings[idx].size);
      bufferVector.at(idx).size = bufferMappings[idx].size;
    }
    bufferVector.at(idx).buf = buf.data();
    bufferVector.at(idx).handle = 0;
    bufferVector.at(idx).type = QBufferType::QBUFFER_TYPE_HEAP;
    bufferVector.at(idx).offset = 0;
  }

  qaic::openrt::shProgram program;
  QAicProgramProperties programProperties;
  qaic::openrt::Program::initProperties(programProperties);

  program = qaic::openrt::Program::Factory(context, devIds.front(), "TestName",
                                           qpc, &programProperties);

  ASSERT_TRUE(program);

  qaic::openrt::shExecObj execObj;
  execObj = qaic::openrt::ExecObj::Factory(context, program);

  ASSERT_TRUE(execObj);

  execObj->setData(bufferVector);

  for (auto i = 0; i < numInference; i++) {
    LogInfo("Starting inference number {}", i + 1);
    status = execObj->run();

    ASSERT_TRUE(status == QS_SUCCESS) << "Inference run fail";
  }
}

TEST_F(QAicOpenRtApiExecObjUnitTest, CreateExecObjTest) {
  TestCreateExecObj(
      "/opt/qti-aic/test-data/aic100/v2/4nsp/4nsp-quant-resnet50");
}

TEST_F(QAicOpenRtApiExecObjUnitTest, RunInferenceTest) {
  TestRunInference("/opt/qti-aic/test-data/aic100/v2/4nsp/4nsp-add",
                   10 /*Num inferences*/);
}

TEST_F(QAicOpenRtApiExecObjUnitTest, RunInferenceTestProgramPreActivated) {
  TestRunInferenceProgramPreActivated(
      "/opt/qti-aic/test-data/aic100/v2/4nsp/4nsp-add", 10 /*Num inferences*/);
}

TEST_F(QAicOpenRtApiExecObjUnitTest, DISABLED_RunInferenceTestPartialTensor) {
  // TODO: Enable once we have a QPC that allows partialtensor
  TestRunInferencePartialTensor(
      "/opt/qti-aic/test-data/aic100/v2/1nsp/1nsp-partial-tensor",
      10 /*Num inferences*/);
}

TEST_F(QAicOpenRtApiExecObjUnitTest, AdversarialInvalidPPPTransform) {
  TestRunInference("/opt/qti-aic/test-data/aic100/v2/4nsp/4nsp-quant-resnet50",
                   1 /*Num inferences*/, TestType::TEST_TYPE_ADVERSARIAL);
}

} // namespace QAicOpenRtContextUnitTest