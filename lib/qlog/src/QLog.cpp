// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QLog.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/stdout_sinks.h"

#include <mutex>

namespace qlog {

// For Manager

//======================================================================
// For Unique Shared Instance of Log Controller
//======================================================================

QLogControlShared &QLogControl::getLogControl() {
  static QLogControlShared logController_ = std::make_shared<QLogControl>();
  return logController_;
}

QLogControl::QLogControl()
    : file_sync_(nullptr), console_sync_(nullptr),
      default_log_level_(
          static_cast<spdlog::level::level_enum>(SPDLOG_ACTIVE_LEVEL)),
      consoleLogEnable_(false) {
  // file_sync_ =
  //    std::make_shared<spdlog::sinks::daily_file_sink_mt>("logfile", 23, 59);
  console_sync_ = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  distributedSink_ = std::make_shared<spdlog::sinks::dist_sink_mt>();
  // Note, Console log should not be enabled by default, this is temporary
  enableConsoleLog();
}

void QLogControl::setSinkLevel(const log_sink_enum sink,
                               spdlog::level::level_enum level) {
  switch (sink) {
  case console:
    console_sync_->set_level(level);
    break;
  case file:
    if (file_sync_) {
      file_sync_->set_level(level);
    }
    break;
  case all:
  default:
    if (file_sync_) {
      file_sync_->set_level(level);
    }
    console_sync_->set_level(level);
    break;
  }
}

void QLogControl::setLogLevel(QLogLevel qLogLevel) {
  spdlog::level::level_enum spdLevel;
  switch (qLogLevel) {
  case QL_DEBUG:
    spdLevel = spdlog::level::debug;
    break;
  case QL_INFO:
    spdLevel = spdlog::level::info;
    break;
  case QL_WARN:
    spdLevel = spdlog::level::warn;
    break;
  case QL_ERROR:
    spdLevel = spdlog::level::err;
    break;
    setLogLevel(spdLevel);
  }
}

bool QLogControl::setLogLevel(const std::string &name,
                              spdlog::level::level_enum level) {
  auto logger = spdlog::get(name);
  if (logger) {
    logger->set_level(level);
    return true;
  }
  return false;
}

void QLogControl::setDefaultLogLevel(spdlog::level::level_enum level) {
  default_log_level_ = level;
}

void QLogControl::setLogLevel(spdlog::level::level_enum level) {
  setDefaultLogLevel(level);
  spdlog::set_level(level);

  for (auto &sink : distributedSink_->sinks()) {
    sink->set_level(level);
  }
}

void QLogControl::getLogLevel(const std::string &name, QLogLevel &level) {
  level = QL_ERROR; // Default level error
  auto logger = spdlog::get(name);
  if (logger) {
    level = getQLogLevel(logger->level());
  }
}

void QLogControl::getLogLevel(QLogLevel &level) {
  level = getQLogLevel(default_log_level_);
}

void QLogControl::makeLogger(const std::string name,
                             std::shared_ptr<spdlog::logger> &logger) {
  std::lock_guard<std::mutex> guard(loggerMutex_);
  logger = spdlog::get(name);
  if (!logger) {
    logger = std::make_shared<spdlog::logger>(
        name, spdlog::sinks_init_list({getDistributedSink()}));

    logger->set_pattern("[%H:%M:%S.%e][%^%l%$][%n]%v");
    logger->set_level(default_log_level_);
    spdlog::register_logger(logger);
    // We want all logs to flushed to the sink
    logger->flush_on(spdlog::level::level_enum::trace);
  }
}

QLogStatus QLogControl::enableConsoleLog() {
  std::unique_lock<std::mutex> lk(loggerMutex_);
  if (consoleLogEnable_ == false) {

    distributedSink_->add_sink(console_sync_);
    consoleLogEnable_ = true;
  }
  return QLOG_SUCCESS;
}

QLogStatus QLogControl::disableConsoleLog() {
  std::unique_lock<std::mutex> lk(loggerMutex_);
  if (consoleLogEnable_ == true) {

    distributedSink_->remove_sink(console_sync_);
    consoleLogEnable_ = false;
  }
  return QLOG_SUCCESS;
}

QLogSinkSh QLogControl::registerLogger(qaicLoggerCallback logCbFunction,
                                       QLogLevel level, void *userData) {
  std::unique_lock<std::mutex> lk(loggerMutex_);
  QLogSinkSh qLogSinkShPtr = nullptr;

  LogCbDb::const_iterator cbIter = logCbDb_.find(logCbFunction);

  if (cbIter != logCbDb_.end()) {
    return cbIter->second;
  }

  qLogSinkShPtr = std::make_shared<QLogSink>(logCbFunction, level, userData);
  if (qLogSinkShPtr == nullptr) {
    return nullptr;
  }

  qLogSinkShPtr->set_level(getSpdLogLevel(level));
  distributedSink_->add_sink(qLogSinkShPtr);
  // distributedSink_.add_sink(qLogSinkShPtr);
  // <RT>
  //  |
  // <std::shared_ptr<QLogSink> | Callback function
  auto insertRet =
      logCbDb_.insert(std::make_pair(logCbFunction, qLogSinkShPtr));

  if (insertRet.second == false) {
    return nullptr;
  }
  return qLogSinkShPtr;
}

QLogStatus QLogControl::unRegisterLogger(qaicLoggerCallback logCbFunction) {
  std::unique_lock<std::mutex> lk(loggerMutex_);
  QLogStatus status = QLOG_ERROR;
  // qLogSinkLocal->set_level(getSpdLogLevel(level));
  // | Callback function  <std::shared_ptr<QLogSink>
  for (auto it = logCbDb_.cbegin(); it != logCbDb_.cend();) {
    if ((it->first == logCbFunction)) {
      status = QLOG_SUCCESS;

      distributedSink_->remove_sink(it->second);
      it = logCbDb_.erase(it);
    } else {
      ++it;
    }
  }
  return status;
}

std::shared_ptr<spdlog::sinks::sink> QLogControl::getFileSink() {
  return file_sync_;
}

std::shared_ptr<spdlog::sinks::sink> QLogControl::getConsoleSink() {
  return console_sync_;
}

std::shared_ptr<spdlog::sinks::dist_sink_mt> QLogControl::getDistributedSink() {
  return distributedSink_;
}

spdlog::level::level_enum QLogControl::getSpdLogLevel(QLogLevel qlogLevel) {
  switch (qlogLevel) {
  case QL_DEBUG:
    return spdlog::level::debug;
    break;
  case QL_INFO:
    return spdlog::level::info;
    break;
  case QL_WARN:
    return spdlog::level::warn;
    break;
  case QL_ERROR:
    return spdlog::level::err;
    break;
  default:
    return spdlog::level::warn;
    break;
  }
}

QLogLevel QLogControl::getQLogLevel(spdlog::level::level_enum spdLogLevel) {
  QLogLevel level = QL_ERROR; // Default
  switch (spdLogLevel) {
  case spdlog::level::trace:
  case spdlog::level::debug:
    level = QL_DEBUG;
    break;
  case spdlog::level::info:
    level = QL_INFO;
    break;
  case spdlog::level::warn:
    level = QL_WARN;
    break;
  case spdlog::level::err:
  case spdlog::level::critical:
  default:
    level = QL_ERROR;
    break;
  }
  return level;
}
} // namespace qlog
