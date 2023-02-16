// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QAicOpenRtUnitTestBase.hpp"
#include "QAicOpenRtApi.hpp"
#include "QAicOpenRtQpc.hpp"

namespace QAicOpenRtUnitTest {

class QAicOpenRtApiQpcUnitTest : public QAicOpenRtUnitTestBase {
public:
  QAicOpenRtApiQpcUnitTest(){};
  virtual ~QAicOpenRtApiQpcUnitTest() = default;

  QAicOpenRtApiQpcUnitTest(const QAicOpenRtApiQpcUnitTest &) =
      delete; // Disable Copy Constructor
  QAicOpenRtApiQpcUnitTest &operator=(const QAicOpenRtApiQpcUnitTest &) =
      delete; // Disable Assignment Operator
protected:
  void QpcCreateTest(std::string testBasePath);
  void QpcBufferMappingsTest(std::string testBasePath);
  void QpcIODescTest(std::string testBasePath);
};

void QAicOpenRtApiQpcUnitTest::QpcCreateTest(std::string testBasePath) {
  qaic::openrt::shQpc qpc;
  qaic::shQProgramContainer qpcObj;
  size_t qpcSize;
  shQpcInfo qpcInfo;

  qpc = qaic::openrt::Qpc::Factory(testBasePath);
  ASSERT_TRUE(qpc);

  qpcObj = qpc->getQpc();
  ASSERT_TRUE(qpcObj);

  qpcSize = qpc->size();
  ASSERT_TRUE(qpcSize != 0);
  LogInfo("Qpc Size: {}", qpcSize);

  const uint8_t *qpcBuf = qpc->buffer();
  ASSERT_TRUE(qpcBuf != nullptr);

  qpcInfo = qpc->getInfo();
  ASSERT_TRUE(qpcInfo);
}

void QAicOpenRtApiQpcUnitTest::QpcBufferMappingsTest(std::string testBasePath) {
  qaic::openrt::shQpc qpc;
  BufferMappings bufferMappings;
  BufferMappings bufferMappingsDma;

  qpc = qaic::openrt::Qpc::Factory(testBasePath);
  ASSERT_TRUE(qpc);

  auto logMappingInfo = [this](BufferMappings bufferMappings) {
    for (auto const &m : bufferMappings) {
      std::string bufferDir;
      m.ioType == QAicBufferIoTypeEnum::BUFFER_IO_TYPE_INPUT
          ? bufferDir = "IN "
          : bufferDir = "OUT";
      LogInfo(" Buffer Index: {} , Dir: {} , Size: {} , Name: {}", m.index,
              bufferDir, m.size, m.bufferName);
    }
  };

  bufferMappings = qpc->getBufferMappings();
  ASSERT_TRUE(!bufferMappings.empty());
  logMappingInfo(bufferMappings);

  bufferMappingsDma = qpc->getBufferMappingsDma();
  ASSERT_TRUE(!bufferMappingsDma.empty());
  logMappingInfo(bufferMappingsDma);
}

void QAicOpenRtApiQpcUnitTest::QpcIODescTest(std::string testBasePath) {
  qaic::openrt::shQpc qpc;
  BufferMappings bufferMappings;
  QData iodesc;
  QStatus status;

  qpc = qaic::openrt::Qpc::Factory(testBasePath);
  ASSERT_TRUE(qpc);
  status = qpc->getIoDescriptor(&iodesc);
  ASSERT_TRUE(status == QS_SUCCESS);
}

TEST_F(QAicOpenRtApiQpcUnitTest, QpcCreateTest) {
  QpcCreateTest("/opt/qti-aic/test-data/aic100/v2/4nsp/4nsp-quant-resnet50");
}

TEST_F(QAicOpenRtApiQpcUnitTest, QpcBufferMappingsTest) {
  QpcBufferMappingsTest(
      "/opt/qti-aic/test-data/aic100/v2/4nsp/4nsp-quant-resnet50");
}

TEST_F(QAicOpenRtApiQpcUnitTest, QpcIODescTest) {
  QpcIODescTest("/opt/qti-aic/test-data/aic100/v2/4nsp/4nsp-quant-resnet50");
}

} // namespace QAicOpenRtUnitTest
