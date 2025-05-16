// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
#ifndef QLOG_INTERFACE_H
#define QLOG_INTERFACE_H

#include "QLog.h"

namespace qlog {

/// Caller supplies an implementation of this class in the call to
/// getRuntime() to direct LRT library logs to caller's application.
class QLogInterface {
public:
  enum class LogLevel { Debug = 0, Info = 1, Warn = 2, Error = 3 };
  virtual LogLevel getLogLevel() const = 0;
  /// There may be asynchronous calls to this function
  virtual void log(LogLevel level, const char *str) = 0;
  virtual ~QLogInterface() = default;
};

} // namespace qlog

using namespace qlog;
#endif // QLOG_INTERFACE