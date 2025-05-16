// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
#ifndef QDEVMQ_H
#define QDEVMQ_H

#include "QDevCmd.h"

namespace qaic {
namespace devcommon {

// Message Queue Transfer Type
typedef enum {
  QAIC_DEV_MQ_TXFR_SEND_N_RECV = 0,
  QAIC_DEV_MQ_TXFR_SEND_ONLY = 1
} QDevMqTxfrTypeEnum;

// Device Command structure
// for Devices with Message Queue based interface
typedef struct {
  // Command Identifier
  unsigned long cmd;
  // Command Data
  uint8_t cmdData[QAIC_DEV_CMD_SIZ_MAX];
} QDevMqCmd;

} // namespace common
} // namespace qaic

#endif // QDEVMQ_H