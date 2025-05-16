// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
/** @file nnc_err_inf
 *  @brief NNC Error Fatal Interface Header file
 */
#ifndef NNC_ERR_INF_H_
#define NNC_ERR_INF_H_

#include <stdint.h>

/* Type used when generating the constant error fatal parameters */
typedef struct __attribute__((packed)) {
  const char *fname; /*!< Pointer to source file name */
  uint16_t line;     /*!< Line number that this error occured on */
  const char *fmt;   /*!< Printf style format string */
} err_const_type;

#define _XX_MSG_CONST(xx_fmt)                                                  \
  static const err_const_type xx_msg_const = {__FILE__, __LINE__, (xx_fmt)}

#define ERR_FATAL(fp, format, xx_arg1, xx_arg2, xx_arg3)                       \
  do {                                                                         \
    _XX_MSG_CONST(format);                                                     \
    fp(&xx_msg_const, (uint32_t)(xx_arg1), (uint32_t)(xx_arg2),                \
       (uint32_t)(xx_arg3));                                                   \
  } while (0)

typedef void (*nnc_err_fatal_fp)(const err_const_type *const_blk,
                                 uint32_t code1, uint32_t code2,
                                 uint32_t code3);

// Used to report a network hang (e.g., due to waiting too long for a doorbell).
typedef void (*nnc_notify_hang_fp)(void);

#endif /* NNC_ERR_INF_H_ */
