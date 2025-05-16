// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QAicOpenRtUnitTestBase.hpp"
#include "QAicOpenRtApi.hpp"
#include "QAicOpenRtInferenceVector.hpp"

namespace QAicOpenRtUnitTest {

class QAicOpenRtInferenceVectorUnitTest : public QAicOpenRtUnitTestBase {
public:
  QAicOpenRtInferenceVectorUnitTest(){};
  ~QAicOpenRtInferenceVectorUnitTest() { cleanup(); }

  QAicOpenRtInferenceVectorUnitTest(const QAicOpenRtInferenceVectorUnitTest &) =
      delete; // Disable Copy Constructor
  QAicOpenRtInferenceVectorUnitTest &
  operator=(const QAicOpenRtInferenceVectorUnitTest &) =
      delete; // Disable Assignment Operator

protected:
  void InferenceVectorCreationTestForRandomData(std::string qpcPath);
  void InferenceVectorCreationTestForZeroFill(std::string qpcPath);
  void InferenceVectorCreationTestForFileSet(std::string qpcPath);
  void InferenceVectorSetBuffersTest(std::string qpcPath);
  void initialSetup(const std::string networkPath);
  void cleanup();
  std::string createTempDir(std::string testName);
  std::string createTempFile(std::string testDir, size_t fileSize);
  void deleteTempDir(std::string testDir);
  QStatus validateInferenceVector(qaic::openrt::shInferenceVector infVector,
                                  std::vector<std::string> fileSet,
                                  uint32_t numInputs);

private:
  qaic::openrt::shContext context_;
  qaic::openrt::shQpc qpc_;
  QStatus setUpStatus_;
  std::vector<QID> devIds_;
};

void QAicOpenRtInferenceVectorUnitTest::cleanup() {
  qpc_ = nullptr;
  context_ = nullptr;
}

void QAicOpenRtInferenceVectorUnitTest::initialSetup(
    const std::string qpcPath) {
  QAicContextProperties properties = 0x00;
  qaic::openrt::Util util;
  setUpStatus_ = QS_ERROR;
  try {
    ASSERT_TRUE(util.getDeviceIds(devIds_) == QS_SUCCESS);
    context_ = qaic::openrt::Context::Factory(&properties, devIds_);
    ASSERT_TRUE(static_cast<bool>(context_));

    qpc_ = qaic::openrt::Qpc::Factory(qpcPath);
    ASSERT_TRUE(static_cast<bool>(qpc_));

  } catch (const std::exception &e) {
    LogError("Exception Caught during test: {}", e.what());
    ASSERT_TRUE(false);
    return;
  }
  setUpStatus_ = QS_SUCCESS;
}

std::string
QAicOpenRtInferenceVectorUnitTest::createTempDir(std::string testName) {
  std::string tempDir = "/tmp/" + testName + "/";
  std::string cleanCmd = "rm -rf " + tempDir;
  if (system(cleanCmd.c_str()) < 0) {
    LogError("Failed to clear directory");
  }
  std::string mkdirCmd = "mkdir -p " + tempDir;
  if (system(mkdirCmd.c_str()) < 0) {
    LogError("Failed to create directory");
  }
  return tempDir;
}

std::string
QAicOpenRtInferenceVectorUnitTest::createTempFile(std::string testDir,
                                                  size_t fileSize) {
  std::ofstream outfile;
  static uint64_t fileCounter = 0;
  fileCounter++;
  std::string fileName = testDir + std::to_string(fileCounter);
  outfile.open(fileName, std::ios::binary | std::ios::out);
  for (size_t i = 0; i < fileSize; i++) {
    outfile << static_cast<char>((fileCounter % 8));
  }
  outfile.flush();
  return fileName;
}

void QAicOpenRtInferenceVectorUnitTest::deleteTempDir(std::string testDir) {
  std::string cleanCmd = "rm -rf " + testDir;
  if (system(cleanCmd.c_str()) < 0) {
    LogError("Failed to clear directory");
  }
}

QStatus QAicOpenRtInferenceVectorUnitTest::validateInferenceVector(
    qaic::openrt::shInferenceVector infVector, std::vector<std::string> fileSet,
    uint32_t numInputs) {
  std::vector<QBuffer> dataVectors;
  dataVectors = infVector->getVector();
  for (uint32_t i = 0; i < numInputs; i++) {
    std::ifstream inputFile(fileSet.at(i));
    std::istream_iterator<uint8_t> start(inputFile), end;
    std::vector<uint8_t> fileData(start, end);
    std::vector<uint8_t> vectorData(
        dataVectors.at(i).buf, dataVectors.at(i).buf + dataVectors.at(i).size);
    if (fileData != vectorData) {
      return QS_ERROR;
    }
  }
  return QS_SUCCESS;
}

void QAicOpenRtInferenceVectorUnitTest::
    InferenceVectorCreationTestForRandomData(std::string qpcPath) {
  initialSetup(qpcPath);
  ASSERT_TRUE(setUpStatus_ == QS_SUCCESS);

  // Test factory with bufferMappings for random data, default source type
  qaic::openrt::shInferenceVector infVector =
      qaic::openrt::InferenceVector::Factory(qpc_->getBufferMappings());
  ASSERT_TRUE(infVector != nullptr);

  // Test factory with qpc object
  qaic::openrt::shInferenceVector infVectorViaQpc =
      qaic::openrt::InferenceVector::Factory(qpc_);
  ASSERT_TRUE(infVectorViaQpc != nullptr);
}

void QAicOpenRtInferenceVectorUnitTest::InferenceVectorCreationTestForZeroFill(
    std::string qpcPath) {
  initialSetup(qpcPath);
  ASSERT_TRUE(setUpStatus_ == QS_SUCCESS);

  // Test factory with bufferMappings for random data
  qaic::openrt::shInferenceVector infVector =
      qaic::openrt::InferenceVector::Factory(
          qpc_->getBufferMappings(), qaic::openrt::InferenceVector::ZERO_FILL);
  ASSERT_TRUE(infVector != nullptr);

  // Test factory with qpc object
  qaic::openrt::shInferenceVector infVectorViaQpc =
      qaic::openrt::InferenceVector::Factory(
          qpc_, qaic::openrt::InferenceVector::ZERO_FILL);
  ASSERT_TRUE(infVectorViaQpc != nullptr);
}

void QAicOpenRtInferenceVectorUnitTest::InferenceVectorCreationTestForFileSet(
    std::string qpcPath) {
  initialSetup(qpcPath);
  ASSERT_TRUE(setUpStatus_ == QS_SUCCESS);
  std::string testDir;
  std::vector<std::string> fileSet;
  QStatus testStatus = QS_ERROR;

  BufferMappings bufferMap = qpc_->getBufferMappings();
  uint32_t numBufferMappingsInputs =
      std::count_if(bufferMap.begin(), bufferMap.end(), [](BufferMapping m) {
        return m.ioType == QAicBufferIoTypeEnum::BUFFER_IO_TYPE_INPUT;
      });

  testDir = createTempDir(testName());
  for (auto &buffer : bufferMap) {
    if (buffer.ioType == QAicBufferIoTypeEnum::BUFFER_IO_TYPE_INPUT) {
      std::string fileName = createTempFile(testDir, buffer.size);
      fileSet.emplace_back(fileName);
    }
  }

  // Test creation with fileset with bufferMappings
  qaic::openrt::shInferenceVector infVector =
      qaic::openrt::InferenceVector::Factory(qpc_->getBufferMappings(),
                                             fileSet);
  ASSERT_TRUE(infVector != nullptr);

  // Test creation with fileset with qpc
  qaic::openrt::shInferenceVector infVectorWithFileSetQpc =
      qaic::openrt::InferenceVector::Factory(qpc_, fileSet);
  ASSERT_TRUE(infVectorWithFileSetQpc != nullptr);

  // Validate the created inference vector for fileset
  testStatus =
      validateInferenceVector(infVector, fileSet, numBufferMappingsInputs);
  ASSERT_TRUE(testStatus == QS_SUCCESS);

  // test loadFileSet
  testStatus = infVector->loadFromFileSet(fileSet);
  ASSERT_TRUE(testStatus == QS_SUCCESS);

  deleteTempDir(testDir);
}

void QAicOpenRtInferenceVectorUnitTest::InferenceVectorSetBuffersTest(
    std::string qpcPath) {
  initialSetup(qpcPath);
  ASSERT_TRUE(setUpStatus_ == QS_SUCCESS);
  QStatus testStatus = QS_ERROR;

  qaic::openrt::shInferenceVector infVector =
      qaic::openrt::InferenceVector::Factory(qpc_->getBufferMappings());
  ASSERT_TRUE(infVector != nullptr);

  // Test Set Input/Ouput Buffers
  std::vector<QBuffer> dataVectors;
  dataVectors = infVector->getVector();

  testStatus = infVector->setBuffers(dataVectors);
  ASSERT_TRUE(testStatus == QS_SUCCESS);

  // Negative Test, number of input buffers not matching
  testStatus = infVector->setInputBuffers(dataVectors);
  ASSERT_TRUE(testStatus != QS_SUCCESS);

  // Positive test
  BufferMappings bufferMap = qpc_->getBufferMappings();
  uint32_t numBufferMappingsInputs =
      std::count_if(bufferMap.begin(), bufferMap.end(), [](BufferMapping m) {
        return m.ioType == QAicBufferIoTypeEnum::BUFFER_IO_TYPE_INPUT;
      });

  std::vector<QBuffer> inputVector(
      dataVectors.begin(), dataVectors.begin() + numBufferMappingsInputs);
  testStatus = infVector->setBuffers(inputVector);
  ASSERT_TRUE(testStatus == QS_SUCCESS);

  // Negative Test, number of output buffers not matching
  testStatus = infVector->setOutputBuffers(dataVectors);
  ASSERT_TRUE(testStatus != QS_SUCCESS);

  // Positive test
  uint32_t numBufferMappingsOutputs =
      std::count_if(bufferMap.begin(), bufferMap.end(), [](BufferMapping m) {
        return m.ioType == QAicBufferIoTypeEnum::BUFFER_IO_TYPE_OUTPUT;
      });

  std::vector<QBuffer> outputVector(
      dataVectors.begin() + numBufferMappingsInputs,
      dataVectors.begin() + numBufferMappingsInputs + numBufferMappingsOutputs);

  testStatus = infVector->setOutputBuffers(outputVector);
  ASSERT_TRUE(testStatus == QS_SUCCESS);
}

//--------------------------------------------------------------------------
// Test Program
//--------------------------------------------------------------------------

TEST_F(QAicOpenRtInferenceVectorUnitTest,
       InferenceVectorCreationTestForRandomData) {
  InferenceVectorCreationTestForRandomData(
      "/opt/qti-aic/test-data/aic100/v2/4nsp/4nsp-quant-resnet50");
}

TEST_F(QAicOpenRtInferenceVectorUnitTest,
       InferenceVectorCreationTestForZeroFill) {
  InferenceVectorCreationTestForZeroFill(
      "/opt/qti-aic/test-data/aic100/v2/4nsp/4nsp-quant-resnet50");
}

TEST_F(QAicOpenRtInferenceVectorUnitTest,
       InferenceVectorCreationTestForFileSet) {
  InferenceVectorCreationTestForFileSet(
      "/opt/qti-aic/test-data/aic100/v2/4nsp/4nsp-quant-resnet50");
}

TEST_F(QAicOpenRtInferenceVectorUnitTest,
       AdversarialInferenceVectorSetBuffersTest) {
  InferenceVectorSetBuffersTest(
      "/opt/qti-aic/test-data/aic100/v2/4nsp/4nsp-quant-resnet50");
}

} // namespace QAicOpenRtUnitTest
