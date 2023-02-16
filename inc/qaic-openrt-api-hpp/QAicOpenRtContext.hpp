// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QAIC_OPENRT_CONTEXT_HPP
#define QAIC_OPENRT_CONTEXT_HPP

#include "QLogger.h"
#include "QAicOpenRtExceptions.hpp"
#include "QAicRuntimeTypes.h"
#include "QContext.h"

namespace qaic {
namespace openrt {

/// \brief Primary object that links all other core objects, this
/// object should be created first.  User should provide callbacks for
/// logging and error handling.
/// \par Use the Factory method to create a shared objects, which will ensure
/// proper initialization and destruction of the object.
class Context;
using shContext = std::shared_ptr<Context>;

class Context : private QLogger {
public:
  /// \brief Create a shared Context object
  /// \param[in] properties Context properties
  /// \param[in] devList The list of devices used by this context
  /// in error handler callback
  /// \return Shared Context pointer
  /// \exception CoreExceptionNullPtr
  /// - When memory allocation for object fails
  /// \exception CoreExceptionInit
  /// - When context creation fails due to invalid property
  /// - When context creation fails due to out of memory
  /// - When context creation fails due to internal error
  static shContext Factory(QAicContextProperties *properties,
                           const std::vector<QID> &devList) {

    shContext obj = shContext(new (std::nothrow) Context(properties, devList));
    if (!obj) {
      throw ExceptionNullPtr(objType_);
    }

    // Init will throw an exception on failure
    obj->init();
    return obj;
  }

  /// \brief Set Logging Level
  /// \param[in] logLevel New logging level
  QStatus setLogLevel(QLogLevel logLevel) {
    return context_->setLogLevel(logLevel);
  }

  /// \brief Get Logging Level
  /// \param[out] logLevel Current logging level
  QStatus getLogLevel(QLogLevel &logLevel) {
    return context_->getLogLevel(logLevel);
  }

  shQContext getContext() const noexcept { return context_; }

  /// \brief Destructor ensures that context object is released
  /// and that all associated resources are released
  ~Context() { context_ = nullptr; }

  Context(const Context &) = delete;            // Disable Copy Constructor
  Context &operator=(const Context &) = delete; // Disable Assignment Operator
private:
  Context(QAicContextProperties *properties, const std::vector<QID> &devList)
      : devList_(devList), properties_(properties){};
  void init() {
    QStatus status;
    context_ = QContext::createContext(properties_, devList_.size(),
                                       devList_.data(), status);
    if (status != QS_SUCCESS) {
      throw CoreExceptionInit(objType_, "Failed to create Context");
    }
  }

  std::vector<QID> devList_;
  shQContext context_ = nullptr;
  QAicContextProperties *properties_ = nullptr;
  static constexpr const char *objType_ = "Context";
};
///\}
} // namespace rt
} // namespace qaic

#endif // QAIC_OPENRT_CONTEXT_HPP
