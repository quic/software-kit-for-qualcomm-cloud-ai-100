// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
/** @file nnc_ulog_inf
 *  @brief NNC ULOG INTERFACE to Network Header file
 */
#ifndef NNC_ULOG_INF_H_
#define NNC_ULOG_INF_H_

/* Various log masks*/
typedef enum {
  NNC_LOG_MASK_NONE = 0x0,
  NNC_LOG_MASK_FATAL = 0x1,
  NNC_LOG_MASK_ERROR = 0x2,
  NNC_LOG_MASK_WARN = 0x4,
  NNC_LOG_MASK_INFO = 0x8,
  NNC_LOG_MASK_DEBUG = 0x10,
  NNC_LOG_MASK_INVALID = 0x7f
} nnc_log_masks;

/* Use NNC_NARG to get number of arguments*/
#define NNC_NARG(...) NNC_NARG_(__VA_ARGS__, NNC_RSEQ_N())
#define NNC_NARG_(...) NNC_ARG_N(__VA_ARGS__)
#define NNC_ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, COUNT, ...) COUNT
#define NNC_RSEQ_N() 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0

#define LOG64_DATA(x) (uint32_t) x, (uint32_t)((uint64_t)x >> 32)

/*NN Log macro used by network to log prints*/
/**
 * <!-- nn_log -->
 *
 * @brief Log data in the printf format.
 *
 * Log printf data to a RealTime log.  The printf formating is
 * not executed until the log is read. This makes a very performant call,
 * but also means all strings must be present when the log is read.
 * This function does the most work for ULog logging.  It's been optimized to
 * write directly to the buffer if possible.
 *
 * @param mask : MAsk for the log.
 * @param dataCount : The number of parameters being printed (Not
 *                    including the formatStr).  Limited to 10
 *                    parameters.
 * @param formatStr : The format string for the printf.
 * @param ... : The parameters to print.
 *
 * @return void
 */

#define NN_LOG(fp, mask, formatStr, ...)                                       \
  fp(mask, NNC_NARG(__VA_ARGS__), formatStr, ##__VA_ARGS__)

/* Function pointer for logging, passed from the execute structure*/
typedef void (*nnc_log_fp)(unsigned int mask, unsigned int dataCount,
                           const char *formatStr, ...);

#endif /* NNC_ULOG_INF_H_ */
