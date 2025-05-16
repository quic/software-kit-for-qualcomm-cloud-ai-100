// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
#ifndef QDEVCMD_H
#define QDEVCMD_H

#include <mqueue.h>

namespace qaic {

#define QAIC_DEV_CMD_SIZ_MAX 4096
#define INVALID_FILE_DEV_HANDLE -1

enum QDevInterfaceCmdEnum {
  QAIC_DEV_CMD_MANAGE = 1,
  QAIC_DEV_CMD_MEM = 2,
  QAIC_DEV_CMD_PRIME_FD = 3,
  QAIC_DEV_CMD_MMAP_DEV = 4,
  QAIC_DEV_CMD_FREE_MEM = 5,
  QAIC_DEV_CMD_EXECUTE = 6,
  QAIC_DEV_CMD_WAIT_EXEC = 7,
  QAIC_DEV_CMD_QUERY = 8,
  QAIC_DEV_CMD_PARTIAL_EXECUTE = 9,
  QAIC_DEV_CMD_PART_DEV = 10,
};

// File based Device Descriptor
typedef struct { int fd; } QDevFileHandle;

// Message Queue Descriptor
typedef struct { mqd_t mqd; } QDevMqHandle;

// Number of POSIX Message Queues per Device
enum QDevMqTypeEnum {
  QAIC_DEV_MQ_SEND = 0,
  QAIC_DEV_MQ_RECV = 1,
  QAIC_DEV_MQ_MAX = 2
};

// Device Handle/Descriptor
typedef struct {
  // For File based Devices
  QDevFileHandle fileDev;
  // For POSIX Message Queue based Devices
  QDevMqHandle mqDev[QAIC_DEV_MQ_MAX];
} QDevHandle;

// Device Command Type
enum QDevCmdTypeEnum {
  QDEV_CMD_TYPE_WRITE_ONLY = 0,
  QDEV_CMD_TYPE_WRITE_AND_READ = 1,
};

// Device command
typedef struct {
  // Command Request/Identifier
  unsigned long cmdReq;
  // Command and Response Buffer
  const void *cmdRspBuf;
  // Command buffer Size
  size_t cmdSize;
  // Command Type
  QDevCmdTypeEnum cmdType;
} QDevCmd;

} // namespace qaic

#endif // QDEVCMD_H