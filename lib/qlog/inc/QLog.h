// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
#ifndef QLOG_H
#define QLOG_H

#include "spdlog/spdlog.h"
#include "spdlog/sinks/ostream_sink.h"
#include "spdlog/sinks/base_sink.h"
#include "spdlog/sinks/dist_sink.h"
#include "spdlog/details/null_mutex.h"
#include <mutex>
#include <sstream>
#include <iostream>
#include <thread>
#include <unordered_map>
#include <string.h>
#include <functional>
#include <memory>

namespace qlog {

typedef enum { QL_DEBUG = 0, QL_INFO = 1, QL_WARN = 2, QL_ERROR = 3 } QLogLevel;

typedef enum {
  QLOG_SUCCESS = 0,
  /// Generic error
  QLOG_ERROR = 500,
} QLogStatus;

/// Caller supplies an implementation of this class in the call to
/// getRuntime() to direct LRT library logs to caller's application.
class QLog {
public:
  enum class LogLevel { Debug = 0, Info = 1, Warn = 2, Error = 3 };
  virtual LogLevel getLogLevel() const = 0;
  /// There may be asynchronous calls to this function
  virtual void log(LogLevel level, const char *str) = 0;
  virtual ~QLog() = default;
};

using loglevel = spdlog::level::level_enum;

#define SPDLOG_TRACE_ON

typedef enum {
  console = 0,
  file = 1,
  all = 2,
} log_sink_enum;

// Levels
// trace 0
// debug 1
// info 2
// warn 3
// err 4
// critical 5
// off 6

// Note, Log can be disabled at compile time
//
// define SPDLOG_ACTIVE_LEVEL to one of those (before including spdlog.h):
// SPDLOG_LEVEL_TRACE,
// SPDLOG_LEVEL_DEBUG,
// SPDLOG_LEVEL_INFO,
// SPDLOG_LEVEL_WARN,
// SPDLOG_LEVEL_ERROR,
// SPDLOG_LEVEL_CRITICAL,
// SPDLOG_LEVEL_OFF

/// Caller supplies this function to process log messages produced by the
/// library
typedef void (*qaicLoggerCallback)(QLogLevel logLevel, const char *str,
                                   void *userData);

typedef struct {
  qaicLoggerCallback logCallback;
  QLogLevel logLevel;
} QLogInfo;

class QLogControl;
using QLogControlShared = std::shared_ptr<QLogControl>;

template <typename Mutex>
class QLogSpdSink : public spdlog::sinks::base_sink<Mutex> {
public:
  QLogSpdSink(qaicLoggerCallback logCbFunction, QLogLevel level, void *userData)
      : logCbFunction_(logCbFunction), level_(level), userData_(userData){};
  void setQLogLevel(QLogLevel level) {
    // std::unique_lock<std::mutex> lk(levelMutex_);
    level_ = level;
  }

protected:
  void sink_it_(const spdlog::details::log_msg &msg) override {

    // log_msg is a struct containing the log entry info like level, timestamp,
    // thread id etc.
    // msg.raw contains pre formatted log

    // Splice out the log level from the msg to send to the logger callback
    QLogLevel msgLevel = QL_ERROR; // Default
    switch (msg.level) {
    case spdlog::level::trace:
    case spdlog::level::debug:
      msgLevel = QL_DEBUG;
      break;
    case spdlog::level::info:
      msgLevel = QL_INFO;
      break;
    case spdlog::level::warn:
      msgLevel = QL_WARN;
      break;
    case spdlog::level::err:
    case spdlog::level::critical:
    default:
      msgLevel = QL_ERROR;
      break;
    }

    if (msgLevel >= level_) {
      // If needed (very likely but not mandatory), the sink formats the message
      // before sending it to its final destination:
      spdlog::memory_buf_t formatted;
      spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
      if (logCbFunction_ != nullptr) {
        logCbFunction_(msgLevel, fmt::to_string(formatted).c_str(), userData_);
      }
    }
  }
  void flush_() override {}

private:
  qaicLoggerCallback logCbFunction_;
  std::atomic<QLogLevel> level_;
  void *userData_;
};

using QLogSink = QLogSpdSink<std::mutex>;
using QLogSinkSh = std::shared_ptr<QLogSink>;
// <RT>
//  |
//   <std::shared_ptr<QLogSink> | Callback function
using LogCbDb =
    std::unordered_map<qaicLoggerCallback, std::shared_ptr<QLogSink>>;

class QLogControl {
public:
  static QLogControlShared &getLogControl();

  // The following public APIS are for use within the library, for testing
  // purpose
  // Control of Log Source Levels
  bool setLogLevel(const std::string &name, spdlog::level::level_enum level);
  void setLogLevel(spdlog::level::level_enum level);
  void setLogLevel(QLogLevel qlogLevel);
  void setDefaultLogLevel(spdlog::level::level_enum level);
  void getLogLevel(const std::string &name, QLogLevel &level);
  void getLogLevel(QLogLevel &level);

  // Console Sink is optional, enable/disable through these APIS
  QLogStatus enableConsoleLog();
  QLogStatus disableConsoleLog();

  QLogSinkSh registerLogger(qaicLoggerCallback logCbFunction, QLogLevel level,
                            void *userData);

  QLogStatus unRegisterLogger(qaicLoggerCallback logCbFunction);

  // Control the level of a Sink
  void setSinkLevel(const log_sink_enum sink, spdlog::level::level_enum level);

  void makeLogger(const std::string name,
                  std::shared_ptr<spdlog::logger> &logger);

  static std::shared_ptr<spdlog::logger> Factory(const std::string name) {
    std::shared_ptr<spdlog::logger> logger;
    getLogControl()->makeLogger(name, logger);
    return logger;
  }

  QLogControl(const QLogControl &) = delete;
  QLogControl &operator=(const QLogControl &) = delete;

  QLogControl();
  static spdlog::level::level_enum getSpdLogLevel(QLogLevel qlogLevel);
  static QLogLevel getQLogLevel(spdlog::level::level_enum level);

private:
  std::shared_ptr<spdlog::sinks::sink> getFileSink();
  std::shared_ptr<spdlog::sinks::sink> getConsoleSink();
  std::shared_ptr<spdlog::sinks::dist_sink_mt> getDistributedSink();
  std::mutex loggerMutex_; // use this Mutex for making Loggers and db
  std::shared_ptr<spdlog::sinks::sink> file_sync_;
  std::shared_ptr<spdlog::sinks::sink> console_sync_;
  std::shared_ptr<spdlog::sinks::dist_sink_mt> distributedSink_;
  spdlog::level::level_enum default_log_level_;
  bool consoleLogEnable_;
  LogCbDb logCbDb_;
};

} // namespace qlog

using namespace qlog;
#endif // QLOG_H
