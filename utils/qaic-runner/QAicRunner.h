// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QAIC_RUNNER_H_
#define QAIC_RUNNER_H_

#include "QAicOpenRtExceptions.hpp"
#include "QAicRuntimeTypes.h"
#include "QAicOpenRtApi.hpp"
#include "QLog.h"
#include <getopt.h>
#include <iostream>
#include <string>
#include <vector>

namespace qaicrunner {
using QTimePoint = std::chrono::time_point<std::chrono::steady_clock>;

extern const uint32_t qidDefault;
extern const uint32_t numInferencesDefault;
extern const uint32_t startIterationDefault;
extern const uint32_t numSamplesDefault;
extern const std::string outputDirDefault;

struct QAicRunnerWriteOutputProperties {
  bool enabled;
  uint32_t startIteration;
  uint32_t numSamplesToWrite;
  std::string outputDir;
  QAicRunnerWriteOutputProperties()
      : enabled(false), startIteration(startIterationDefault),
        numSamplesToWrite(numSamplesDefault), outputDir(outputDirDefault) {}
};

class QAicRunnerExample {
public:
  ~QAicRunnerExample();
  void addToInputFileList(const std::string &inputFile);
  void addToOutputFileList(const std::string &outputFile);
  void setTestBasePath(const std::string &testBasePath);
  void setVerbosity(const uint32_t &verbosity);
  void setQid(QID qid);
  bool setNumInferences(const uint32_t &numInferences);
  int checkUserParam();
  void setWriteOutputStartIteration(const uint32_t &num);
  bool setWriteOutputDir(const char *);
  void setWriteOutputNumSamples(const uint32_t &num);
  void getLastRunStats(uint64_t &infCompleted, double &infRate,
                       uint64_t &runtimeUs, uint32_t &batchSize);
  QStatus init();
  QStatus run();

private:
  QLogLevel qLogLevel_ = QL_ERROR;
  uint32_t verbosityLevel_ = 0;
  uint32_t numInferences_ = numInferencesDefault;
  QID dev_ = 0;
  std::string testBasePath_;
  std::vector<std::string> inputFileList_;
  std::vector<std::string> outputFileList_;
  std::vector<QBuffer> validationBufferList_;
  bool validateOutputEnabled_ = false;
  QAicRunnerWriteOutputProperties writeOutputProperties_;
  QAicContextProperties contextProperties_ = static_cast<QAicContextProperties>(
      QAicContextPropertiesBitfields::QAIC_CONTEXT_DEFAULT);
  qaic::openrt::shContext context_;
  qaic::openrt::shQpc qpc_;
  qaic::openrt::shInferenceVector inferenceVector_;
  qaic::openrt::shProgram program_;
  QAicProgramProperties programProperties_;
  qaic::openrt::shExecObj execObj_;
  qaic::openrt::FileWriter fileWriter_;
  QStatus addBuffersToValidationList();
  QStatus validateOutput(const std::vector<QBuffer> &ioBuffers, size_t infIdx);
  uint64_t lastRunDurationUs_ = 0;
}; // QAicRunnerExample

} // namespace qaicrunner

#endif // QAIC_RUNNER_H_
