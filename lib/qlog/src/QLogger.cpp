// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QLogger.h"
#include "QLog.h"

namespace qlog {

std::shared_ptr<spdlog::logger>
    QLogger::commonLogger_(QLogControl::Factory("LogCommon"));

QLogger::QLogger() : loggerName_("LogCommon") { logger_ = getGlobalLogger(); }

QLogger::QLogger(const std::string &name) : loggerName_(name) {
  QLogControl::getLogControl()->makeLogger(name, logger_);
}

void QLogger::dropLogger() {
  logger_->flush();
  spdlog::drop(loggerName_);
}

std::shared_ptr<spdlog::logger> &QLogger::getLogger() { return logger_; }

std::shared_ptr<spdlog::logger> &QLogger::getGlobalLogger() {
  std::unique_lock<std::mutex> commonloggerMutex_;
  if (!commonLogger_) {
    QLogControl::getLogControl()->makeLogger("LogCommon", commonLogger_);
  }
  return commonLogger_;
}

QLogInterface::LogLevel QLogger::getLogLevel() const {
  spdlog::level::level_enum spdLevel = logger_->level();
  switch (spdLevel) {
  case spdlog::level::debug:
    return QLogInterface::LogLevel::Debug;
  case spdlog::level::info:
    return QLogInterface::LogLevel::Info;
  case spdlog::level::warn:
    return QLogInterface::LogLevel::Warn;
  case spdlog::level::err:
  default:
    return QLogInterface::LogLevel::Error;
  }
}

void QLogger::setLogLevel(LogLevel level) {
  switch (level) {
  case QLogInterface::LogLevel::Debug:
    logger_->set_level(spdlog::level::debug);
    break;
  case QLogInterface::LogLevel::Info:
    logger_->set_level(spdlog::level::info);
    break;
  case QLogInterface::LogLevel::Warn:
    logger_->set_level(spdlog::level::warn);
    break;
  case QLogInterface::LogLevel::Error:
    logger_->set_level(spdlog::level::err);
    break;
  default:
    logger_->set_level(spdlog::level::warn);
  }
}

QLogInterface::LogLevel QLogger::getGlobalLogLevel() const {
  spdlog::level::level_enum spdLevel = getGlobalLogger()->level();
  switch (spdLevel) {
  case spdlog::level::debug:
    return QLogInterface::LogLevel::Debug;
  case spdlog::level::info:
    return QLogInterface::LogLevel::Info;
  case spdlog::level::warn:
    return QLogInterface::LogLevel::Warn;
  case spdlog::level::err:
  default:
    return QLogInterface::LogLevel::Error;
  }
}

void QLogger::log(LogLevel level, const char *str) {
  spdlog::level::level_enum spdLevel;
  switch (level) {
  case QLogInterface::LogLevel::Debug:
    spdLevel = spdlog::level::debug;
    break;
  case QLogInterface::LogLevel::Info:
    spdLevel = spdlog::level::info;
    break;
  case QLogInterface::LogLevel::Warn:
    spdLevel = spdlog::level::warn;
    break;
  case QLogInterface::LogLevel::Error:
  default:
    spdLevel = spdlog::level::err;
    break;
  }
  logger_->log(spdLevel, str);
}

} // namespace qlog
