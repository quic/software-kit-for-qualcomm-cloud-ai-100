// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QAICYPES_H
#define QAICYPES_H

#include <stddef.h>
#include <stdint.h>

/// \brief The QSTATUS param is used to determine the status of an operation
typedef enum {
  QS_SUCCESS = 0,
  /// Permission denied
  QS_PERM = 1,
  /// Try again
  QS_AGAIN = 2,
  /// Not enough memory
  QS_NOMEM = 3,
  /// Device/resource busy
  QS_BUSY = 4,
  /// No such device
  QS_NODEV = 5,
  /// Invalid argument
  QS_INVAL = 6,
  /// No space
  QS_NOSPC = 7,
  /// Bad file descriptor
  QS_BADFD = 8,
  /// Timeout
  QS_TIMEDOUT = 9,
  /// Protocol Error
  QS_PROTOCOL = 10,
  /// Unsupported
  QS_UNSUPPORTED = 11,
  /// CRC Data Transfer Error
  QS_DATA_CRC_ERROR = 12,
  /// Generic device error
  QS_DEV_ERROR = 300,
  /// Generic error
  QS_ERROR = 500,
} QStatus;

/// \brief Data type supported by AIC Device
typedef enum {
  FLOAT_TYPE = 0,    /// 32-bit float type (float)
  FLOAT_16_TYPE = 1, /// 16-bit float type (half, fp16)
  INT8_Q_TYPE = 2,   /// 8-bit quantized type (int8_t)
  UINT8_Q_TYPE = 3,  /// unsigned 8-bit quantized type (uint8_t)
  INT16_Q_TYPE = 4,  /// 16-bit quantized type (int16_t)
  INT32_Q_TYPE = 5,  /// 32-bit quantized type (int32_t)
  INT32_I_TYPE = 6,  /// 32-bit index type (int32_t)
  INT64_I_TYPE = 7,  /// 64-bit index type (int64_t)
  INT8_TYPE = 8,     /// Int8 type (bool)
  TYPE_INVALID = 0xff,
} QAicIoDataTypeEnum;

/// \brief Error Handle type
typedef uint32_t QAicErrorHandle;

#endif // QAICTYPES_H
