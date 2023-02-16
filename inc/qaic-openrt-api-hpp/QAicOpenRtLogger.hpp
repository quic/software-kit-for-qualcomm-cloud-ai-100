// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QAIC_OPENRT_LOGGER_HPP
#define QAIC_OPENRT_LOGGER_HPP

#include "QAicOpenRtApiFwdDecl.hpp"
#include "QAicOpenRtContext.hpp"
#include "QLogger.h"
#include "QContext.h"

namespace qaic {
namespace openrt {

class Logger : public QLogger {
public:
  class proxyLogger;
  friend proxyLogger;

  virtual ~Logger() = default;
  Logger() {}

  Logger(shContext &context) : context_(context) {}

  class proxyLogger {
  public:
    proxyLogger(Logger &logger, QLogLevel level)
        : logger_(logger), level_(level){};

    proxyLogger(const proxyLogger &proxy)
        : logger_(proxy.logger_), level_(proxy.level_){};

    ~proxyLogger() {
      if (logStream_.str().empty()) {
        return;
      }

      logger_.logMessage(logger_.context_.lock(), level_,
                         logStream_.str().c_str());
    };

    std::ostringstream logStream_;
    Logger &logger_;
    QLogLevel level_;

    template <typename T> proxyLogger &operator<<(T const &data) {
      logStream_ << data;
      return *this;
    }

    proxyLogger &operator<<(std::ostream &(*manipulator)(std::ostream &)) {
      manipulator(logStream_);
      return *this;
    }
  };

  // Set log level of the message to String stream
  proxyLogger log(QLogLevel level) { return proxyLogger(*this, level); }

  /// \brief Log Error Message through the QAic Library
  virtual void logError(std::string msg) {
    logMessage(context_.lock(), QL_ERROR, msg.c_str());
  }

  /// \brief Log Info Message through the QAic Library
  virtual void logInfo(std::string msg) {
    logMessage(context_.lock(), QL_INFO, msg.c_str());
  }

  /// \brief Log Warning Message through the QAic Library
  virtual void logWarn(std::string msg) {
    logMessage(context_.lock(), QL_WARN, msg.c_str());
  }

  /// \brief Log Debug Message through the QAic Library
  virtual void logDebug(std::string msg) {
    logMessage(context_.lock(), QL_DEBUG, msg.c_str());
  }

  void setContext(shContext context) { context_ = context; }

private:
  std::weak_ptr<Context> context_;
  static constexpr const char *objType_ = "Logger";

  void logMessage(shContext context, QLogLevel logLevel, const char *message) {
    if (context == nullptr) {
      switch (logLevel) {
      case QL_INFO:
        LogInfoG(message);
        break;
      case QL_WARN:
        LogWarnG(message);
        break;
      case QL_ERROR:
        LogErrorG(message);
        break;
      case QL_DEBUG:
      default:
        LogDebugG(message);
        break;
      }
    } else {
      context->getContext()->logMessage(logLevel, message);
    }
  }
};

} // namespace qaic
} // namespace openrt

#endif // QAIC_OPENRT_LOGGER_HPP