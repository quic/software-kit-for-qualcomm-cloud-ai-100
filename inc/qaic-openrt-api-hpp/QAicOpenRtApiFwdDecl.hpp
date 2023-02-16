// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
#ifndef QAICAPI_FWD_DECL_HPP
#define QAICAPI_FWD_DECL_HPP

#include "QAicRuntimeTypes.h"

#include <functional>
#include <memory>

namespace qaic {
namespace openrt {

class Util;
class Lib;
class Logger;
using shLogger = std::shared_ptr<Logger>;

// For QAicApiCore
class Context;
using shContext = std::shared_ptr<Context>;
// using contextLogFunction = std::function<void(QLogLevel, const char *)>;
// using contextErrorHandler = std::function<void(
//     QAicContextID contextId, const char *errInfo, QAicErrorType_t errType,
//     const void *errData, size_t errDataSize, void *userData)>;
class Program;
using shProgram = std::shared_ptr<Program>;
class Constants;
using shConstants = std::shared_ptr<Constants>;
class Queue;
using shQueue = std::shared_ptr<Queue>;
class ExecObj;
using shExecObj = std::shared_ptr<ExecObj>;
// using ExecObjCompletionCallback =
//     std::function<void(ExecObj *execObj, ExecObjInfo *execObjInfo)>;
class Qpc;
using shQpc = std::shared_ptr<Qpc>;

// For QAicApiUtil
class QpcFile;
using shQpcFile = std::shared_ptr<QpcFile>;

class InferenceVector;
using shInferenceVector = std::shared_ptr<InferenceVector>;

class FileWriter;

} // namespace openrt
} // namespace qaic
#endif
