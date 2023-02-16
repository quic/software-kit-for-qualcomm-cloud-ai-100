// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QAICAPI_EXCEPTIONS_HPP
#define QAICAPI_EXCEPTIONS_HPP

#include "QAicOpenRtApiFwdDecl.hpp"

#include <iostream>
#include <exception>
#include <string>

namespace qaic {

/// \defgroup  qaicapihpp-exceptions
/// \{
/// Exceptions support, this group of classes to defines
/// qaic specific exceptions
/// \}

/// \addtogroup qaicapihpp-exceptions Runtime C++ Exception API
/// \ingroup qaicapihpp
/// \{

/// \brief Object Initialization Exception
/// This exception is thrown when an initialization exceptions occur.
class ExceptionInit : public std::exception {
public:
  explicit ExceptionInit(const std::string &msg)
      : msg_(std::string(baseMsg) + msg) {}
  virtual ~ExceptionInit() = default;

  virtual const char *what() const noexcept { return msg_.c_str(); }
  const std::string &msg() const { return msg_; }

public:
  const std::string msg_;
  static constexpr const char *baseMsg = "Initialization Exception: ";
};

/// \brief Runtime Exception
/// This exception is thrown when objects Runtime failures occur.
class ExceptionRuntime : public std::exception {
public:
  explicit ExceptionRuntime(const std::string &msg) {
    msg_ = std::string(baseMsg) + msg;
  }
  virtual ~ExceptionRuntime() noexcept = default;
  virtual const char *what() const noexcept { return msg_.c_str(); }
  const std::string &msg() const { return msg_; }

private:
  std::string msg_;
  static constexpr const char *baseMsg = "Runtime Exception: ";
};

/// \brief Null Pointer Exception
/// This exception is thrown when unexpected null pointer is encountered.
class ExceptionNullPtr : public std::exception {
public:
  explicit ExceptionNullPtr(const std::string &msg) {
    msg_ = std::string(baseMsg) + msg;
  }
  virtual ~ExceptionNullPtr() noexcept = default;
  virtual const char *what() const noexcept { return msg_.c_str(); }
  const std::string &msg() const { return msg_; }

private:
  std::string msg_;
  static constexpr const char *baseMsg = "Null Pointer Exception: ";
};

namespace openrt {
/// \brief Initialization Exception
/// This exception is thrown when objects initialization fails in a class.
/// The objectName along with the Message will be displayed.
class CoreExceptionInit final : public ExceptionInit {
public:
  explicit CoreExceptionInit(const std::string &name,
                             const std::string &msg = "")
      : ExceptionInit("in object: " + name + " " + msg) {}
  virtual ~CoreExceptionInit() noexcept = default;
};

/// \brief Runtime Exception
/// This exception is thrown  when objects Runtime failures occurs in a class.
/// The objectName along with the Message will be displayed.
class CoreExceptionRuntime : public ExceptionRuntime {
public:
  explicit CoreExceptionRuntime(const std::string &msg)
      : ExceptionRuntime(msg) {}
  virtual ~CoreExceptionRuntime() noexcept = default;
};

/// \brief Null Pointer Runtime Exception
/// This exception is thrown when an unexpected nullptr is encountered
/// The objectName along with the Message will be displayed.
class CoreExceptionNullPtr final : public ExceptionNullPtr {
public:
  explicit CoreExceptionNullPtr(const std::string &name,
                                const std::string &msg = "")
      : ExceptionNullPtr("in object: " + name + " " + msg) {}
  virtual ~CoreExceptionNullPtr() noexcept = default;
};

///\}
} // namespace rt
} // namespace qaic

#endif
