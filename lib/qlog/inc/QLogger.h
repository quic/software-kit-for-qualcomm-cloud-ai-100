// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
#ifndef QLOGGER_H
#define QLOGGER_H

#include "QLog.h"
#include "QLogInterface.h"
#include "spdlog/spdlog.h"

namespace qlog {

#define QLOG_COMPONENT_LOG_FUNCTION_LINE

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
// LOG Macros
// There are 3 sets of Macros
// Main:  LogTrace, LogDebug, LogInfo, LogWarn, LogError
// Static: 'S' subscript  LogTraceS, LogDebugS, LogInfoS, LogWarnS, LogErrorS
// Global: 'G' subscript  LogTraceG, LogDebugG, LogInfoG, LogWarnG, LogErrorG
//
// Main sould be used when ever possible.
// Object should inherit from the QLogger logging class with virtual
// such that if there are multiple inheritence, there is only 1 logger
// A log will be taken only in the constructor
//
// Static is for static functions that have a QLogger object, the macros
// will take the object as the first argument.  ex: LogInfoS(object, ...)
//
// Global is for static functions that do not have a QLogger object, when
// using this, for each log event a mutex will be locked, thus it's less
// efficient.
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------

#if defined(QLOG_COMPONENT_LOG_FUNCTION_LINE)
//--------------------------------------------------------------------------
// Logging format to print-out FUNCTION names with Line
//--------------------------------------------------------------------------

#ifdef LRT_BUILD_TYPE_DEBUG
#define LogTrace(...)                                                          \
  if (logger_->should_log(spdlog::level::trace)) {                             \
    logger_->trace("[{}:#{}] {}", __FUNCTION__, __LINE__,                      \
                   fmt::format(__VA_ARGS__));                                  \
  }

#define LogDebug(...)                                                          \
  if (logger_->should_log(spdlog::level::debug)) {                             \
    logger_->debug("[{}:#{}] {}", __FUNCTION__, __LINE__,                      \
                   fmt::format(__VA_ARGS__));                                  \
  }
#else
#define LogTrace(...)
#define LogDebug(...)
#endif

#define LogInfo(...)                                                           \
  if (logger_->should_log(spdlog::level::info)) {                              \
    logger_->info("[{}:#{}] {}", __FUNCTION__, __LINE__,                       \
                  fmt::format(__VA_ARGS__));                                   \
  }
#define LogWarn(...)                                                           \
  if (logger_->should_log(spdlog::level::warn)) {                              \
    logger_->warn("[{}:#{}] {}", __FUNCTION__, __LINE__,                       \
                  fmt::format(__VA_ARGS__));                                   \
  }
#define LogError(...)                                                          \
  if (logger_->should_log(spdlog::level::err)) {                               \
    logger_->error("[{}:#{}] {}", __FUNCTION__, __LINE__,                      \
                   fmt::format(__VA_ARGS__));                                  \
  }
#define LogCritical(...)                                                       \
  {                                                                            \
    logger_->critical("[{}:#{}] {}", __FUNCTION__, __LINE__,                   \
                      fmt::format(__VA_ARGS__));                               \
  }
#elif defined(QLOG_COMPONENT_LOG_FUNCTION)

//--------------------------------------------------------------------------
// Logging format to print-out only FUNCTION names for Info and Warn
//--------------------------------------------------------------------------

#ifdef LRT_BUILD_TYPE_DEBUG
#define LogTrace(...)                                                          \
  if (logger_->should_log(spdlog::level::trace)) {                             \
    logger_->trace("[{}:#{}] {}", __FUNCTION__, __LINE__,                      \
                   fmt::format(__VA_ARGS__));                                  \
  }

#define LogDebug(...)                                                          \
  if (logger_->should_log(spdlog::level::debug)) {                             \
    logger_->debug("[{}] {}", __FUNCTION__, fmt::format(__VA_ARGS__));         \
  }
#else
#define LogTrace(...)
#define LogDebug(...)
#endif

#define LogInfo(...)                                                           \
  if (logger_->should_log(spdlog::level::info)) {                              \
    logger_->info("[{}] {}", __FUNCTION__, fmt::format(__VA_ARGS__));          \
  }
#define LogWarn(...)                                                           \
  if (logger_->should_log(spdlog::level::warn)) {                              \
    logger_->warn("[{}] {}", __FUNCTION__, fmt::format(__VA_ARGS__));          \
  }
#define LogError(...)                                                          \
  if (logger_->should_log(spdlog::level::err)) {                               \
    logger_->error("[{}:#{}] {}", __FUNCTION__, __LINE__,                      \
                   fmt::format(__VA_ARGS__));                                  \
  }
#define LogCritical(...)                                                       \
  {                                                                            \
    logger_->critical("[{}:#{}] {}", __FUNCTION__, __LINE__,                   \
                      fmt::format(__VA_ARGS__));                               \
  }

#else

#ifdef LRT_BUILD_TYPE_DEBUG
#define LogTrace(...) logger_->trace(__VA_ARGS__);
#define LogDebug(...) logger_->debug(__VA_ARGS__);
#else
#define LogTrace(...)
#define LogDebug(...)
#endif

#define LogInfo(...) logger_->info(__VA_ARGS__);
#define LogWarn(...) logger_->warn(__VA_ARGS__);
#define LogError(...) logger_->error(__VA_ARGS__);
#define LogCritical(...) logger_->critical(__VA_ARGS__);
#endif

// Static version
#if defined(QLOG_COMPONENT_LOG_FUNCTION_LINE)
//--------------------------------------------------------------------------
// Logging format to print-out FUNCTION names with Line
//--------------------------------------------------------------------------

#ifdef LRT_BUILD_TYPE_DEBUG
#define LogTraceS(component, ...)                                              \
  if (component->logger_->should_log(spdlog::level::trace)) {                  \
    component->logger_->trace("[{}:#{}] {}", __FUNCTION__, __LINE__,           \
                              fmt::format(__VA_ARGS__));                       \
  }

#define LogDebugS(component, ...)                                              \
  if (component->logger_->should_log(spdlog::level::debug)) {                  \
    component->logger_->debug("[{}:#{}] {}", __FUNCTION__, __LINE__,           \
                              fmt::format(__VA_ARGS__));                       \
  }
#else
#define LogTraceS(component, ...)
#define LogDebugS(component, ...)
#endif

#define LogInfoS(component, ...)                                               \
  if (component->logger_->should_log(spdlog::level::info)) {                   \
    component->logger_->info("[{}:#{}] {}", __FUNCTION__, __LINE__,            \
                             fmt::format(__VA_ARGS__));                        \
  }
#define LogWarnS(component, ...)                                               \
  if (component->logger_->should_log(spdlog::level::warn)) {                   \
    component->logger_->warn("[{}:#{}] {}", __FUNCTION__, __LINE__,            \
                             fmt::format(__VA_ARGS__));                        \
  }
#define LogErrorS(component, ...)                                              \
  if (component->logger_->should_log(spdlog::level::err)) {                    \
    component->logger_->error("[{}:#{}] {}", __FUNCTION__, __LINE__,           \
                              fmt::format(__VA_ARGS__));                       \
  }
#define LogCriticalS(component, ...)                                           \
  {                                                                            \
    component->logger_->critical("[{}:#{}] {}", __FUNCTION__, __LINE__,        \
                                 fmt::format(__VA_ARGS__));                    \
  }
#elif defined(QLOG_COMPONENT_LOG_FUNCTION)

//--------------------------------------------------------------------------
// Logging format to print-out only FUNCTION names for Info and Warn
//--------------------------------------------------------------------------

#ifdef LRT_BUILD_TYPE_DEBUG
#define LogTraceS(component, ...)                                              \
  if (component->logger_->should_log(spdlog::level::trace)) {                  \
    component->logger_->trace("[{}:#{}] {}", __FUNCTION__, __LINE__,           \
                              fmt::format(__VA_ARGS__));                       \
  }

#define LogDebugS(component, ...)                                              \
  if (component->logger_->should_log(spdlog::level::debug)) {                  \
    component->logger_->debug("[{}] {}", __FUNCTION__,                         \
                              fmt::format(__VA_ARGS__));                       \
  }
#else
#define LogTraceS(component, ...)
#define LogDebugS(component, ...)
#endif

#define LogInfoS(component, ...)                                               \
  if (component->logger_->should_log(spdlog::level::info)) {                   \
    component->logger_->info("[{}] {}", __FUNCTION__,                          \
                             fmt::format(__VA_ARGS__));                        \
  }
#define LogWarnS(component, ...)                                               \
  if (component->logger_->should_log(spdlog::level::warn)) {                   \
    component->logger_->warn("[{}] {}", __FUNCTION__,                          \
                             fmt::format(__VA_ARGS__));                        \
  }
#define LogErrorS(component, ...)                                              \
  if (component->logger_->should_log(spdlog::level::err)) {                    \
    component->logger_->error("[{}:#{}] {}", __FUNCTION__, __LINE__,           \
                              fmt::format(__VA_ARGS__));                       \
  }
#define LogCriticalS(component, ...)                                           \
  {                                                                            \
    logger_->critical("[{}:#{}] {}", __FUNCTION__, __LINE__,                   \
                      fmt::format(__VA_ARGS__));                               \
  }

#else

#ifdef LRT_BUILD_TYPE_DEBUG
#define LogTraceS(component, ...) component->logger_->trace(__VA_ARGS__);
#define LogDebugS(component, ...) component->logger_->debug(__VA_ARGS__);
#else
#define LogTraceS(component, ...)
#define LogDebugS(component, ...)
#endif

#define LogInfoS(component, ...) component->logger_->info(__VA_ARGS__);
#define LogWarnS(component, ...) component->logger_->warn(__VA_ARGS__);
#define LogErrorS(component, ...) component->logger_->error(__VA_ARGS__);
#define LogCriticalS(component, ...) component->logger_->critical(__VA_ARGS__);
#endif

#define QGLOBALLOGGER_FUNCTION

#if defined(QGLOBALLOGGER_LINE)
//--------------------------------------------------------------------------
// Logging format to print-out FUNCTION names with Line
//--------------------------------------------------------------------------

#ifdef LRT_BUILD_TYPE_DEBUG
#define LogTraceG(...)                                                         \
  if (QLogger::getGlobalLogger()->should_log(spdlog::level::trace)) {          \
    QLogger::getGlobalLogger()->trace("[{}:#{}] {}", __FUNCTION__, __LINE__,   \
                                      fmt::format(__VA_ARGS__));               \
  }

#define LogDebugG(...)                                                         \
  if (QLogger::getGlobalLogger())->should_log(spdlog::level::debug)) {         \
    QLogger::getGlobalLogger())->debug("[{}:#{}] {}", __FUNCTION__, __LINE__,   \
                                      fmt::format(__VA_ARGS__));               \
    }
#else
#define LogTraceG(...)
#define LogDebugG(...)
#endif

#define LogInfoG(...)                                                          \
  if (QLogger::getGlobalLogger()->should_log(spdlog::level::info)) {           \
    QLogger::getGlobalLogger()->info("[{}:#{}] {}", __FUNCTION__, __LINE__,    \
                                     fmt::format(__VA_ARGS__));                \
  }

#define LogInfoGAlways(...)                                                    \
  QLogger::getGlobalLogger()->info("[{}:#{}] {}", __FUNCTION__, __LINE__,      \
                                   fmt::format(__VA_ARGS__));

#define LogWarnG(...)                                                          \
  if (QLogger::getGlobalLogger()->should_log(spdlog::level::warn)) {           \
    QLogger::getGlobalLogger()->warn("[{}:#{}] {}", __FUNCTION__, __LINE__,    \
                                     fmt::format(__VA_ARGS__));                \
  }
#define LogErrorG(...)                                                         \
  if (QLogger::getGlobalLogger()->should_log(spdlog::level::err)) {            \
    QLogger::getGlobalLogger()->error("[{}:#{}] {}", __FUNCTION__, __LINE__,   \
                                      fmt::format(__VA_ARGS__));               \
  }
#define LogCriticalG(...)                                                      \
  {                                                                            \
    QLogger::getGlobalLogger()->critical("[{}:#{}] {}", __FUNCTION__,          \
                                         __LINE__, fmt::format(__VA_ARGS__));  \
  }
#elif defined(QGLOBALLOGGER_FUNCTION)

//--------------------------------------------------------------------------
// Logging format to print-out only FUNCTION names for Info and Warn
//--------------------------------------------------------------------------

#ifdef LRT_BUILD_TYPE_DEBUG
#define LogTraceG(...)                                                         \
  if (QLogger::getGlobalLogger()->should_log(spdlog::level::trace)) {          \
    QLogger::getGlobalLogger()->trace("[{}:#{}] {}", __FUNCTION__, __LINE__,   \
                                      fmt::format(__VA_ARGS__));               \
  }

#define LogDebugG(...)                                                         \
  if (QLogger::getGlobalLogger()->should_log(spdlog::level::debug)) {          \
    QLogger::getGlobalLogger()->debug("[{}] {}", __FUNCTION__,                 \
                                      fmt::format(__VA_ARGS__));               \
  }
#else
#define LogTraceG(...)
#define LogDebugG(...)
#endif

#define LogInfoG(...)                                                          \
  if (QLogger::getGlobalLogger()->should_log(spdlog::level::info)) {           \
    QLogger::getGlobalLogger()->info("[{}] {}", __FUNCTION__,                  \
                                     fmt::format(__VA_ARGS__));                \
  }

#define LogInfoGAlways(...)                                                    \
  QLogger::getGlobalLogger()->info("[{}] {}", __FUNCTION__,                    \
                                   fmt::format(__VA_ARGS__));

#define LogWarnG(...)                                                          \
  if (QLogger::getGlobalLogger()->should_log(spdlog::level::warn)) {           \
    QLogger::getGlobalLogger()->warn("[{}] {}", __FUNCTION__,                  \
                                     fmt::format(__VA_ARGS__));                \
  }
#define LogErrorG(...)                                                         \
  if (QLogger::getGlobalLogger()->should_log(spdlog::level::err)) {            \
    QLogger::getGlobalLogger()->error("[{}:#{}] {}", __FUNCTION__, __LINE__,   \
                                      fmt::format(__VA_ARGS__));               \
  }
#define LogCriticalG(...)                                                      \
  {                                                                            \
    QLogger::getGlobalLogger()->critical("[{}:#{}] {}", __FUNCTION__,          \
                                         __LINE__, fmt::format(__VA_ARGS__));  \
  }

#else

#ifdef LRT_BUILD_TYPE_DEBUG
#define LogTraceG(...) QGlobalLogger::getLogger()->trace(__VA_ARGS__);
#define LogDebugG(...) QGlobalLogger::getLogger()->debug(__VA_ARGS__);
#else
#define LogTraceG(...)
#define LogDebugG(...)
#endif

#define LogInfoG(...) QGlobalLogger::getLogger()->info(__VA_ARGS__);
#define LogWarnG(...) QGlobalLogger::getLogger()->warn(__VA_ARGS__);
#define LogErrorG(...) QGlobalLogger::getLogger()->error(__VA_ARGS__);
#define LogCriticalG(...) QGlobalLogger::getLogger()->critical(__VA_ARGS__);
#endif

class QLogger : public QLogInterface {
public:
  QLogger(const std::string &name);
  QLogger();
  // Default Copy Constructor
  QLogger(const QLogger &) = default;
  // from QLogInterface
  virtual LogLevel getLogLevel() const override;
  virtual void log(LogLevel level, const char *str) override;
  void setLogLevel(LogLevel level);
  void dropLogger();
  LogLevel getGlobalLogLevel() const;
  std::shared_ptr<spdlog::logger> &getLogger();
  static std::shared_ptr<spdlog::logger> &getGlobalLogger();
  std::shared_ptr<spdlog::logger> logger_;
  virtual ~QLogger() = default;
  static std::shared_ptr<spdlog::logger> commonLogger_;
  static std::mutex commonloggerMutex_;
  QLogger &operator=(const QLogger &) = delete; // Disable Assignment Operator
private:
  const std::string loggerName_;
};

} // namespace qlog

using namespace qlog;
#endif // QLOGGER_H