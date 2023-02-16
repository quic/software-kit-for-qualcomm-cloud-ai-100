// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QAicRunner.h"

using namespace qaic;

namespace qaicrunner {
const uint32_t qidDefault = 0;
const uint32_t numInferencesDefault = 1;
const uint32_t startIterationDefault = 0;
const uint32_t numSamplesDefault = 1;
const std::string outputDirDefault = ".";

//------------------------------------------------------------------
// QAIC Runner Example Class Implementation
//------------------------------------------------------------------
QAicRunnerExample::~QAicRunnerExample() {
  if (!validationBufferList_.empty()) {
    for (auto &e : validationBufferList_) {
      delete[] e.buf;
      e.buf = nullptr;
      e.size = 0;
    }
  }
}

void QAicRunnerExample::addToInputFileList(const std::string &inputFile) {
  inputFileList_.push_back(inputFile);
}

void QAicRunnerExample::addToOutputFileList(const std::string &outputFile) {
  outputFileList_.push_back(outputFile);
}

void QAicRunnerExample::setTestBasePath(const std::string &testBasePath) {
  testBasePath_ = testBasePath;
}

void QAicRunnerExample::setVerbosity(const uint32_t &verbosity) {
  switch (verbosity) {
  case 0:
    qLogLevel_ = QL_ERROR;
    break;
  case 1:
    qLogLevel_ = QL_WARN;
    break;
  case 2:
    qLogLevel_ = QL_INFO;
    break;
  case 3:
    qLogLevel_ = QL_DEBUG;
    break;
  default:
    qLogLevel_ = QL_ERROR;
    break;
  }
  verbosityLevel_ = verbosity;
}

void QAicRunnerExample::setQid(QID dev) { dev_ = dev; }

bool QAicRunnerExample::setNumInferences(const uint32_t &numInferences) {
  uint32_t minInfCnt = 1;
  if (numInferences < minInfCnt) {
    std::cout << "Number of inferences too low: " << numInferences
              << " increasing to minimum: " << minInfCnt << std::endl;
    numInferences_ = minInfCnt;
  } else {
    numInferences_ = numInferences;
  }
  return true;
}

int QAicRunnerExample::checkUserParam() {
  if (testBasePath_.empty()) {
    std::cerr << "Error: Missing test data path" << std::endl;
    return -1;
  }

  if (writeOutputProperties_.enabled &&
      ((writeOutputProperties_.startIteration +
        writeOutputProperties_.numSamplesToWrite) > (numInferences_))) {
    std::cerr << "The sum of write-output-start-iter("
              << writeOutputProperties_.startIteration
              << ") and write-output-num-samples("
              << writeOutputProperties_.numSamplesToWrite
              << ") should be <= numInf(" << numInferences_ << ")."
              << std::endl;
    return -1;
  }

  if (verbosityLevel_ > 0) {
    std::cout << "Device: ID " << std::to_string(dev_) << std::endl;
    std::cout << "Test path: " << testBasePath_ << std::endl;
  }

  for (const auto &inputFile : inputFileList_) {
    std::cout << "Input file: " << inputFile << std::endl;
  }
  return 0;
}

QStatus QAicRunnerExample::addBuffersToValidationList() {
  for (auto filepath : outputFileList_) {
    std::ifstream ifs(filepath, std::ios::binary | std::ios::ate);
    if (!ifs) {
      std::cerr << "File not found" << std::endl;
      return QS_ERROR;
    }
    std::ifstream::pos_type pos = ifs.tellg();
    size_t bufSize = static_cast<size_t>(pos);
    QBuffer validationBuf;
    validationBuf.buf = new (std::nothrow) uint8_t[bufSize];
    if (validationBuf.buf == nullptr) {
      std::cerr << "Failed to allocate buffer" << std::endl;
      return QS_ERROR;
    }
    validationBuf.size = bufSize;
    // load output file to validation buffer
    ifs.seekg(0, std::ios::beg);
    ifs.read(reinterpret_cast<char *>(validationBuf.buf), pos);
    ifs.close();
    // add buffer to validation buffer list
    validationBuf.handle = 0;
    validationBuf.offset = 0;
    validationBufferList_.push_back(validationBuf);
  }
  return QS_SUCCESS;
}

QStatus QAicRunnerExample::validateOutput(const std::vector<QBuffer> &ioBuffers,
                                          size_t infIdx) {
  if (qpc_ == nullptr || validationBufferList_.empty()) {
    std::cerr << "qpc is NOT loaded or validationBufferList is NOT populated";
    return QS_ERROR;
  }

  if (ioBuffers.empty()) {
    std::cerr << "Io Buffers not updated";
    return QS_ERROR;
  }

  const BufferMappings &bufferMappings = qpc_->getBufferMappings();
  if (bufferMappings.empty()) {
    std::cerr << "BufferMappings NOT available";
    return QS_ERROR;
  }

  size_t valBufIndex = 0;
  size_t numSuccessfulValidations = 0;
  size_t numFailedValidations = 0;

  for (const auto &bufMapping : bufferMappings) {
    if (bufMapping.ioType == QAicBufferIoTypeEnum::BUFFER_IO_TYPE_OUTPUT) {
      size_t buffIndex = bufMapping.index;
      if (buffIndex >= ioBuffers.size()) {
        std::cerr << fmt::format("Wrong BuffIndex : {} numBuffers {}",
                                 buffIndex, ioBuffers.size());
        return QS_ERROR;
      }
      if (bufMapping.bufferName.compare("aiccyclecounts") == 0) {
        // skip validation
        continue;
      }
      if (valBufIndex >= validationBufferList_.size()) {
        numFailedValidations++;
        std::cerr << fmt::format(
            "Wrong validation buffer index {} numBuffers {}", valBufIndex,
            validationBufferList_.size());
        return QS_ERROR;
      }
      if (ioBuffers.at(buffIndex).size !=
          validationBufferList_.at(valBufIndex).size) {
        numFailedValidations++;
        valBufIndex++;
        continue;
      }
      if (std::equal(ioBuffers.at(buffIndex).buf,
                     ioBuffers.at(buffIndex).buf + ioBuffers.at(buffIndex).size,
                     validationBufferList_.at(valBufIndex).buf)) {
        numSuccessfulValidations++;
      } else {
        numFailedValidations++;
      }
      valBufIndex++;
    }
  }

  if (numFailedValidations != 0) {
    std::cerr << fmt::format(
        "Validation Failed for iteration {}, num failed: {}, num passed: {} \n",
        infIdx, numFailedValidations, numSuccessfulValidations);
    return QS_ERROR;
  } else {
    if (verbosityLevel_ >= 2) {
      std::cout << fmt::format(
          "Validation Successful for iteration {}, num verified buffers: {} \n",
          infIdx, numSuccessfulValidations);
    }
    return QS_SUCCESS;
  }
}

void QAicRunnerExample::setWriteOutputStartIteration(const uint32_t &num) {
  writeOutputProperties_.startIteration = num;
  writeOutputProperties_.enabled = true;
}

bool QAicRunnerExample::setWriteOutputDir(const char *dir) {
  struct stat info;
  bool rc = true;
  if (stat(dir, &info) != 0) {
    if (mkdir(dir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IWOTH) == 0)
      rc = true;
    // Check that it's a directory and that we can write to it
  } else if (((info.st_mode & S_IFDIR) == S_IFDIR) &&
             ((info.st_mode & S_IWUSR) == S_IWUSR)) {
    rc = true;
  } else {
    rc = false;
  }
  if (rc) {
    writeOutputProperties_.outputDir = std::string(dir);
    if (fileWriter_.setOutputPath(writeOutputProperties_.outputDir) !=
        QS_SUCCESS) {
      return false;
    }
    writeOutputProperties_.enabled = true;
  }
  return rc;
}

void QAicRunnerExample::setWriteOutputNumSamples(const uint32_t &num) {
  writeOutputProperties_.numSamplesToWrite = num;
  writeOutputProperties_.enabled = true;
}

void QAicRunnerExample::getLastRunStats(uint64_t &infCompleted, double &infRate,
                                        uint64_t &runtimeUs,
                                        uint32_t &batchSize) {
  infCompleted = numInferences_;
  runtimeUs = lastRunDurationUs_;
  batchSize = (qpc_->getInfo())->program.at(0).batchSize;
  infRate =
      (static_cast<double>((numInferences_ * 1000000)) / lastRunDurationUs_) *
      batchSize;
}

QStatus QAicRunnerExample::init() {
  try {
    QStatus status = QS_ERROR;
    qaic::openrt::Util qaicUtil;
    // validate if device is available
    QDevInfo devInfo;
    status = qaicUtil.getDeviceInfo(dev_, devInfo);
    if (status == QS_SUCCESS) {
      if (devInfo.devStatus != QDevStatus::QDS_READY) {
        std::cerr << "Device:" << dev_ << " not in ready state" << std::endl;
        return QS_ERROR;
      }
    } else {
      std::cerr << "Invalid device:" << std::to_string(dev_) << std::endl;
      return QS_ERROR;
    }

    std::vector<QID> qidList{dev_};

    context_ = qaic::openrt::Context::Factory(&contextProperties_, qidList);

    if (!context_) {
      std::cerr << "Invalid context, failed to create context" << std::endl;
      return QS_ERROR;
    }

    qpc_ = qaic::openrt::Qpc::Factory(testBasePath_);
    if (!qpc_) {
      std::cerr << "Invalid qpc, failed to create qpc object" << std::endl;
      return QS_ERROR;
    }

    context_->setLogLevel(qLogLevel_);

    /* In case inputfile list is empty, inference vector will generate
      buffers with random data as default */
    inferenceVector_ =
        qaic::openrt::InferenceVector::Factory(qpc_, inputFileList_);

    if (!inferenceVector_) {
      std::cerr << "Inference vector creation failed" << std::endl;
      return QS_ERROR;
    }

    qaic::openrt::Program::initProperties(programProperties_);

    program_ = qaic::openrt::Program::Factory(context_, dev_, "QAicRunner",
                                              qpc_, &programProperties_);

    if (!program_) {
      std::cerr << "Program creation failed" << std::endl;
      return QS_ERROR;
    }

    execObj_ = qaic::openrt::ExecObj::Factory(context_, program_);

    if (!execObj_) {
      std::cerr << "ExecObj creation failed" << std::endl;
      return QS_ERROR;
    }

    status = execObj_->setData(inferenceVector_->getVector());

    if (status != QS_SUCCESS) {
      std::cerr << "ExecObj set data failed" << std::endl;
      return QS_ERROR;
    }

    if (!outputFileList_.empty()) {
      status = addBuffersToValidationList();
      if (status != QS_SUCCESS) {
        std::cerr << "Validation buffer creation failed" << std::endl;
        return QS_ERROR;
      }
      validateOutputEnabled_ = true;
    }

  } catch (const qaic::openrt::CoreExceptionInit &e) {
    std::cerr << "Exception Caught during initialization: " << e.what()
              << std::endl;
    return QS_ERROR;
  }
  return QS_SUCCESS;
}

QStatus QAicRunnerExample::run() {
  QStatus status = QS_ERROR;
  QTimePoint startTime, endTime;
  try {
    for (size_t inferenceIndex = 0; inferenceIndex < numInferences_;
         inferenceIndex++) {
      startTime = std::chrono::steady_clock::now();
      status = execObj_->run();
      endTime = std::chrono::steady_clock::now();

      if (status != QS_SUCCESS) {
        std::cerr << "Inference run failed" << std::endl;
        return QS_ERROR;
      }

      lastRunDurationUs_ +=
          std::chrono::duration_cast<std::chrono::microseconds>(endTime -
                                                                startTime)
              .count();

      if (writeOutputProperties_.enabled) {
        if ((inferenceIndex >= writeOutputProperties_.startIteration) &&
            (inferenceIndex < (writeOutputProperties_.numSamplesToWrite +
                               writeOutputProperties_.startIteration))) {
          fileWriter_.writeExecObjData(execObj_, qpc_->getBufferMappings(),
                                       inferenceIndex);
        }
      }

      if (validateOutputEnabled_) {
        std::vector<QBuffer> ioBuffers;
        status = execObj_->getData(ioBuffers);
        if (status != QS_SUCCESS) {
          std::cerr << "Execobj getData failed" << std::endl;
          return QS_ERROR;
        }

        status = validateOutput(ioBuffers, inferenceIndex);
        if (status != QS_SUCCESS) {
          std::cerr << "Output validation failed for iteration "
                    << inferenceIndex + 1 << std::endl;
          return QS_ERROR;
        }
      }
    }
  } catch (const qaic::openrt::CoreExceptionRuntime &e) {
    std::cerr << "Exception Caught during execution: " << e.what() << std::endl;
    return QS_ERROR;
  }

  if (verbosityLevel_ >= 1) {
    std::cout << "Inference run completed successfully!" << std::endl;
    if (validateOutputEnabled_) {
      std::cout << "Output validation successful!" << std::endl;
    }
  }

  return QS_SUCCESS;
}

} // namespace qaicrunner
