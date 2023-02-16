// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QCOMPONENT_H
#define QCOMPONENT_H

#include "spdlog/spdlog.h"
#include "QAic.h"
//--------------------------------------------------------------------------
// Logging format to print-out FUNCTION names with Line
//--------------------------------------------------------------------------
// clang-format off
//----------------------------------------------------------------------------
// These macros support static functions, user has to pass the logger
// as the first parameter, they should pass the class object derived from
// QComponent
//----------------------------------------------------------------------------
#define LogTraceApiS(component, ...)                                                                              \
  if (component->logger_->should_log(spdlog::level::trace)) {                                                     \
    component->logger_->trace("[{:s}:Id:{:3d},L{:4d}] {}", component->componentName(), component->getId(),        \
                   __LINE__, fmt::format(__VA_ARGS__));                                                           \
  }
#define LogDebugApiS(component,...)                                                                               \
  if (component->logger_->should_log(spdlog::level::debug)) {                                                     \
    component->logger_->debug("[{:s}:Id:{:3d},L{:4d}] {}", component->componentName(), component->getId(),        \
                   __LINE__, fmt::format(__VA_ARGS__));                                                           \
  }
#define LogInfoApiS(component,...)                                                                                \
  if (component->logger_->should_log(spdlog::level::info)) {                                                      \
    component->logger_->info("[{:s}:Id:{:3d},L{:4d}] {}", component->componentName(), component->getId(),         \
                  __LINE__, fmt::format(__VA_ARGS__));                                                            \
  }
#define LogWarnApiS(component,...)                                                                                \
  if (component->logger_->should_log(spdlog::level::warn)) {                                                      \
    component->logger_->warn("[{:s}:Id:{:3d},L:{:4d}] {}", component->componentName(), component->getId(),        \
                  __LINE__, fmt::format(__VA_ARGS__));                                                            \
  }

// Use this API to log and error without sending an error report callback
#define LogErrorApiS(component,...)                                                                               \
  if (component->logger_->should_log(spdlog::level::err)) {                                                       \
    component->logger_->error("[{:s}:Id:{:3d},L:{:4d}] {}", component->componentName(), component->getId(),       \
                   __LINE__, fmt::format(__VA_ARGS__));                                                           \
  }

// For critical Errors
#define LogCriticalApiS(component,type, data, datasize, ...)                                                      \
  {                                                                                                               \
    component->logger_->critical("[{:s}:Id:{:3d},L{:4d}] {}", component->componentName(), component->getId(),     \
                      __LINE__, fmt::format(__VA_ARGS__));                                                        \
  }

//----------------------------------------------------------------------------
// These macros can be use witin an object, they require logger_ be defined
//----------------------------------------------------------------------------
#define LogTraceApi(...)                                                       \
  if (logger_->should_log(spdlog::level::trace)) {                             \
    logger_->trace("[{:s}:Id:{:3d},L{:4d}] {}", componentName(), getId(),      \
                   __LINE__, fmt::format(__VA_ARGS__));                        \
  }
#define LogDebugApi(...)                                                       \
  if (logger_->should_log(spdlog::level::debug)) {                             \
    logger_->debug("[{:s}:Id:{:3d},L{:4d}] {}", componentName(), getId(),      \
                   __LINE__, fmt::format(__VA_ARGS__));                        \
  }
#define LogInfoApi(...)                                                        \
  if (logger_->should_log(spdlog::level::info)) {                              \
    logger_->info("[{:s}:Id:{:3d},L{:4d}] {}", componentName(), getId(),       \
                  __LINE__, fmt::format(__VA_ARGS__));                         \
  }
#define LogWarnApi(...)                                                        \
  if (logger_->should_log(spdlog::level::warn)) {                              \
    logger_->warn("[{:s}:Id:{:3d},L:{:4d}] {}", componentName(), getId(),      \
                  __LINE__, fmt::format(__VA_ARGS__));                         \
  }

// Use this API to log and error without sending an error report callback
#define LogErrorApi(...)                                                       \
  if (logger_->should_log(spdlog::level::err)) {                               \
    logger_->error("[{:s}:Id:{:3d},L:{:4d}] {}", componentName(), getId(),     \
                   __LINE__, fmt::format(__VA_ARGS__));                        \
  }

#define LogErrorApiReport(type, data, datasize, ...)                           \
  if (logger_->should_log(spdlog::level::err)) {                               \
    std::string errStr =                                                       \
        fmt::format("[{:s}:Id:{:3d},L{:4d}] {}", componentName(), getId(),     \
                    __LINE__, fmt::format(__VA_ARGS__));                       \
    logger_->error(errStr);                                                    \
  }

// For critical Errors
#define LogCriticalApi(type, data, datasize, ...)                              \
  {                                                                            \
    logger_->critical("[{:s}:Id:{:3d},L{:4d}] {}", componentName(), getId(),   \
                      __LINE__, fmt::format(__VA_ARGS__));                     \
  }

// clang-format on

namespace qaic {
class QComponent {
public:
  QComponent(const char *name, QAicObjId id, QContextInterface *context);
  inline QAicObjId getId() const { return Id_; }
  inline const char *componentName() const { return componentName_; }

protected:
  QAicObjId Id_;
  const char *componentName_;
  std::shared_ptr<spdlog::logger> logger_;
};

} // namespace qaic
#endif // QCOMPONENT_H
