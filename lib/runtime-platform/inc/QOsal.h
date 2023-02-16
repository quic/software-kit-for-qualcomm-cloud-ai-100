// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QOSAL_H
#define QOSAL_H

#include "QAicRuntimeTypes.h"
#include "QPlatformCommon.h"
#include "dev/common/QDevCmd.h"
#include "dev/common/QDevMq.h"
#include "dev/common/QRuntimePlatformDeviceInterface.h"
#include "dev/aic100/qaic_accel.h"
#include "QOsalUtils.h"

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <poll.h>
#include <string>
#include <unordered_map>
#include <sys/utsname.h>
#include <list>
#include <vector>

typedef struct {
  uint16_t major;
  uint16_t minor;
  uint16_t patch;
} kmdModuleVersion;

namespace qaic {

/*
 * Following string is used for udev enumeration (Currently Linux only)
 */
using DevList = std::unordered_map<QID, QPciInfo>;
/*
 * The map contains the mapping between udev parent and child devices.
 * For the real qaic device name we can have multiple derived devices.
 */
using UdevMap = std::unordered_multimap<std::string, std::string>;

constexpr char QAicDeviceSubsystem[] = "accel";

namespace QOsal {
int eventfd(unsigned int initval, int flags);
uint32_t enumAicDevices(DevList &devList);
int getDbcFifoSize(uint32_t *fifoSize, QPciInfo *qPciInfo, uint32_t dbcID);
int getDbcQueuedSize(uint32_t *queued, QPciInfo *dev, uint32_t dbcID);
QStatus getQPciInfo(uint32_t qid, QPciInfo *qPciInfo);
int ppoll(struct pollfd *fds, nfds_t nfds, const struct timespec *timeout_ts,
          const sigset_t *sigmask);
int cpu_setaffinity(int cpu);
int getDevicePath(std::string &path, QPciInfo &dev);
void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
int munmap(void *addr, size_t length);
void *qMmap(void *addr, size_t length, int prot, int flags, int fd, uint64_t handle);
int qMunmap(void *addr, size_t length);
int initPlatform();
int getCtrlChannelPath(std::string &path, const QPciInfo &pciInfo_,
                       QCtrlChannelID ccID);
QStatus allocUserDmabuf(uint8_t **buf, uint32_t size, uint64_t *hdl);
void freeUserDmabuf(void *vaddr);
uint32_t getMemReqSize(size_t size);
int createUdevMap(UdevMap &udev_map,
                  const std::string key = QAicDeviceSubsystem,
                  uint8_t parentSearchStep = 1);
QStatus getPlatformNspDeviceList(std::list<std::string> &devList);
int uname(struct utsname *buf);
ssize_t readlink(const char *path, char *buf, size_t len);
int getPid();
bool validateCrc32(uint8_t *crcBuffer, int crcBufferSize, uint32_t validateCrc);

int eventfd(unsigned int initval, int flags);
size_t strlcpy(char *dst, const char *src, size_t size);
size_t strlcat(char *dst, const char *src, size_t size);
uint32_t enumAicDevices(DevList &devList);
int getDbcFifoSize(uint32_t *fifoSize, QPciInfo *qPciInfo, uint32_t dbcID);
int getDbcQueuedSize(uint32_t *queued, QPciInfo *dev, uint32_t dbcID);
QStatus getQPciInfo(uint32_t qid, QPciInfo *qPciInfo);
int ppoll(struct pollfd *fds, nfds_t nfds, const struct timespec *timeout_ts,
          const sigset_t *sigmask);
int cpu_setaffinity(int cpu);
int getDevicePath(std::string &path, QPciInfo &dev);
int initPlatform();
QStatus getTelemetryInfo(QTelemetryInfo &telemetryInfo,
                         const std::string &pciInfoString);
int getCtrlChannelPath(std::string &path, const QPciInfo &pciInfo_,
                       QCtrlChannelID ccID);
QStatus allocUserDmabuf(uint8_t **buf, uint32_t size, uint64_t *hdl);
void freeUserDmabuf(void *vaddr);
int shm_open(const char *name, int oflag, mode_t mode);
uint32_t getMemReqSize(size_t size);
QStatus getPlatformNspDeviceList(std::list<std::string> &devList);
int uname(struct utsname *buf);
ssize_t readlink(const char *path, char *buf, size_t len);
int getPid();
bool validateCrc32(uint8_t *crcBuffer, int crcBufferSize, uint32_t validateCrc);
const std::string strerror_safe(int errnum);
const std::string getConfigFilePath();

int mqSendRecvSync(int sendMqd, int respMqd, const char *in_data,
                   char *out_data, size_t data_siz,
                   devcommon::QDevMqTxfrTypeEnum trType);
QStatus runMqCmd(QDevHandle &devHandle, QDevCmd &devCmd);
QStatus runIoctlCmd(int devFd, QDevCmd &devCmd);

QStatus openDevice(QDevInterfaceEnum devType, const std::string &devPath,
                   QDevHandle &devHandle);
QStatus runDeviceCmd(QDevInterfaceEnum devType, const QDevHandle &devHandle,
                     QDevCmd &cmd);
int closeDevice(QDevInterfaceEnum devType, QDevHandle &devHandle);
}; // namespace QOsal
} // namespace qaic
#endif // QOSAL_H
