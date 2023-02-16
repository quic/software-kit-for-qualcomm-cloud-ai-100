// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QOsal.h"
#include "QLogger.h"
#include "QUtil.h"
#include "QDevCmd.h"
#include "QDevMq.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <algorithm>
#include <dirent.h>
#include <errno.h>
#include <fstream>
#include <map>
#include <iterator>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <sys/eventfd.h>
#include <errno.h>
#include <sys/mman.h>
#include <mutex>
#include <libudev.h>
#include <sys/utsname.h>
#include <sys/ioctl.h>

extern "C" {
#include <pci/pci.h>
}

const uint32_t QAicPciVendor = 0x17CB;
const uint32_t QAicPciDevice = 0xA100;

namespace qaic {

namespace QOsal {
static std::mutex mtx;
static std::vector<QPciInfo> devListLocal;

// Invalid device name if the pcie device failed to initialize
const std::string QAicInvalidDevice = "accelinvalid";
const std::string QAicMhiUciSubsystem = "mhi_qaic_ctrl";
const std::string QAicPciSubsystem = "pci";

int eventfd(unsigned int initval, int flags) {
  return ::eventfd(initval, flags);
}

//
// Return a list of PCI addresses for currently installed AIC devices.
// The addresses are sorted and the order is used to assign QID. The devices
// would be still in firmware bootup, or in a malfunctioning state. But QID
// assignment should be just based on PCI addresses.
//
uint32_t enumAicDevices(DevList &devList) {

  std::lock_guard<std::mutex> lck(mtx);
  if (devListLocal.empty()) {
    struct pci_access *pacc;
    struct pci_dev *dev;
    struct pci_cap *cap;
    char key[16];
    // Use std::map to sort the devices
    std::map<std::string, QPciInfo> pciMap;

    pacc = pci_alloc();
    pci_init(pacc);
    pci_scan_bus(pacc);
    char classbuf[128], vendbuf[128], devbuf[128];

    char *devicename;
    char *classname;
    char *vendorname;
    pciMap.clear();
    for (dev = pacc->devices; dev; dev = dev->next) {
      pci_fill_info(dev, PCI_FILL_IDENT | PCI_FILL_BASES | PCI_FILL_CLASS);
      if (dev->vendor_id == QAicPciVendor && dev->device_id == QAicPciDevice) {
        std::snprintf(key, sizeof(key), "%04d%02d%02d%02d", dev->domain,
                      dev->bus, dev->dev, dev->func);

        classname = pci_lookup_name(pacc, classbuf, sizeof(classbuf),
                                    PCI_LOOKUP_CLASS, dev->device_class);
        vendorname =
            pci_lookup_name(pacc, vendbuf, sizeof(vendbuf), PCI_LOOKUP_VENDOR,
                            dev->vendor_id, dev->device_id);
        devicename =
            pci_lookup_name(pacc, devbuf, sizeof(devbuf), PCI_LOOKUP_DEVICE,
                            dev->vendor_id, dev->device_id);

        pciMap[key].clear();
        pciMap[key].domain = (uint16_t)dev->domain;
        pciMap[key].bus = dev->bus;
        pciMap[key].device = dev->dev;
        pciMap[key].function = dev->func;
        pciMap[key].pcieExtInfo.bar2Addr =
            dev->base_addr[2] & PCI_ADDR_MEM_MASK;
        pciMap[key].pcieExtInfo.bar4Addr =
            dev->base_addr[4] & PCI_ADDR_MEM_MASK;
        memcpy(pciMap[key].classname, classname, sizeof(pciMap[key].classname));
        memcpy(pciMap[key].vendorname, vendorname,
               sizeof(pciMap[key].vendorname));
        memcpy(pciMap[key].devicename, devicename,
               sizeof(pciMap[key].devicename));
        pciMap[key].pcieExtInfo.bar2Addr =
            dev->base_addr[2] & PCI_ADDR_MEM_MASK;
        pciMap[key].pcieExtInfo.bar4Addr =
            dev->base_addr[4] & PCI_ADDR_MEM_MASK;
        cap = pci_find_cap(dev, PCI_CAP_ID_EXP, PCI_CAP_NORMAL);
        if (cap) {
          unsigned int y;
          y = pci_read_word(dev, (cap->addr) + PCI_EXP_LNKCAP);
          pciMap[key].pcieExtInfo.maxLinkSpeed = y & PCI_EXP_LNKCAP_SPEED;
          pciMap[key].pcieExtInfo.maxLinkWidth =
              (y & PCI_EXP_LNKCAP_WIDTH) >> 4;

          y = pci_read_word(dev, (cap->addr) + PCI_EXP_LNKSTA);
          pciMap[key].pcieExtInfo.currLinkSpeed = y & PCI_EXP_LNKSTA_SPEED;
          pciMap[key].pcieExtInfo.currLinkWidth =
              (y & PCI_EXP_LNKSTA_WIDTH) >> 4;
        }
      }
    }
    pci_cleanup(pacc);

    for (auto &entry : pciMap) {
      devListLocal.push_back(entry.second);
    }
  }

  QID qid = 0;
  for (auto &entry : devListLocal) {
    devList[qid] = entry;
    qid++;
  }
  return devList.size();
}

int getDbcFifoSize(uint32_t *fifoSize, QPciInfo *dev, uint32_t dbcID) {
  char postfix[64];
  std::string path = "/sys/kernel/debug/qaic/";
  std::string fifoSZ;

  std::snprintf(postfix, sizeof(postfix), "%04x:%02x:%02x.%x/dbc%03d/fifo_size",
                dev->domain, dev->bus, dev->device, dev->function, dbcID);
  path += postfix;

  std::ifstream file(path, std::ifstream::in);
  if (!file.is_open()) {
    return -1;
  }
  std::getline(file, fifoSZ);
  file.close();

  *fifoSize = std::stoi(fifoSZ);

  return 0;
}

int getDbcQueuedSize(uint32_t *queued, QPciInfo *dev, uint32_t dbcID) {
  char postfix[64];
  std::string path = "/sys/kernel/debug/qaic/";
  std::string qed;

  std::snprintf(postfix, sizeof(postfix), "%04x:%02x:%02x.%x/dbc%03d/queued",
                dev->domain, dev->bus, dev->device, dev->function, dbcID);
  path += postfix;

  std::ifstream file(path, std::ifstream::in);
  if (!file.is_open()) {
    return -1;
  }
  std::getline(file, qed);
  file.close();

  *queued = std::stoi(qed);

  return 0;
}

QStatus getQPciInfo(uint32_t qid, QPciInfo *qPciInfo) {
  DevList devList;

  enumAicDevices(devList);

  if (devList.size() == 0) {
    return QS_ERROR;
  }

  memcpy((void *)qPciInfo, &devList[qid], sizeof(*qPciInfo));

  return QS_SUCCESS;
}

int ppoll(struct pollfd *fds, nfds_t nfds, const struct timespec *timeout_ts,
          const sigset_t *sigmask) {
  return ::ppoll(fds, nfds, timeout_ts, sigmask);
}

int cpu_setaffinity(int cpu) {
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(cpu, &cpuset);
  pthread_t current_thread = pthread_self();
  if (pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset)) {
    return -1;
  } else {
    return 0;
  }
}

int createUdevMap(UdevMap &udevMap, const std::string key,
                  uint8_t parentSearchStep) {
  struct udev *udev = nullptr;
  struct udev_enumerate *e;
  struct udev_list_entry *entry;

  udev = udev_new();

  if (udev == nullptr) {
    return -1;
  }
  e = udev_enumerate_new(udev);
  udev_enumerate_add_match_subsystem(e, key.c_str());
  udev_enumerate_scan_devices(e);

  udev_list_entry_foreach(entry, udev_enumerate_get_list_entry(e)) {
    struct udev_device *device;
    device =
        udev_device_new_from_syspath(udev, udev_list_entry_get_name(entry));
    if (device != nullptr) {
      const char *name = udev_device_get_sysname(device);
      uint8_t depth =
          parentSearchStep; /* Default is 1. Used to get the parent */
      const char *parent_name = nullptr;
      const char *parent_subsys = udev_device_get_subsystem(device);

      /* Setup a dummy ref to the parent */
      struct udev_device *parent = device;

      while (depth > 0 &&
             (strcmp(parent_subsys, QAicPciSubsystem.c_str()) != 0)) {
        parent = udev_device_get_parent(parent);
        parent_name = udev_device_get_sysname(parent);
        parent_subsys = udev_device_get_subsystem(parent);
        depth--;
      }

      udevMap.insert(std::make_pair<std::string, std::string>(parent_name, name));
      udev_device_unref(device);
    }
  }
  udev_enumerate_unref(e);
  udev_unref(udev);

  return 0;
}

int getDevicePath(std::string &path, QPciInfo &dev) {
  std::string device_name = QAicInvalidDevice;
  std::string pcie_sbdf = qutil::qPciInfoToPCIeStr(dev);
  std::string pathPrefix = "/dev/accel/";

  if (!path.empty()) {
    path = pathPrefix + path;
    return 0;
  }

  UdevMap udevMap;
  if (0 == createUdevMap(udevMap)) {
    UdevMap::const_iterator aic_device_iter = udevMap.find(pcie_sbdf);
    if (aic_device_iter != udevMap.end()) {
      device_name = aic_device_iter->second;
    }
  }
  path = pathPrefix + device_name + "\0";
  return 0;
}

QStatus getPlatformNspDeviceList(
    std::list<std::string> __attribute__((unused)) & devList) {
  return QS_UNSUPPORTED;
}

int initPlatform() { return 0; }

int uname(struct utsname *buf) { return ::uname(buf); }

QStatus getTelemetryInfo(QTelemetryInfo &telemetryInfo,
                         const std::string &pciInfoString) {
  std::string basePath = [&pciInfoString] {
    std::string devicesPath = "/sys/bus/pci/devices";
    std::string hwmonPath =
        fmt::format("{}/{}/{}", devicesPath, pciInfoString, "hwmon");

    DIR *dir;
    struct dirent *ent;
    std::string subDirectory = "";
    if ((dir = opendir(hwmonPath.c_str())) != nullptr) {
      /* Check directories within directory and look for hwmon */
      while ((ent = readdir(dir)) != nullptr) {
        if (strstr(ent->d_name, "hwmon") != nullptr) {
          subDirectory = std::string(ent->d_name);
          break;
        }
      }
      closedir(dir);
    }
    return fmt::format("{}/{}/", hwmonPath, subDirectory);
  }();

  auto readTelemetryData = [](const std::string &path) -> uint32_t {
    std::string data;
    std::ifstream ifs(path);
    if (ifs.is_open()) {
      getline(ifs, data);
      ifs.close();
      return std::stoi(data);
    }
    return 0; // Return Zero in case of file read error
  };

  /// cat /sys/bus/pci/devices/<PCI-Address>/hwmon/hwmon<#>/temp2_input
  telemetryInfo.socTemperature = readTelemetryData(basePath + "temp2_input");

  /// cat /sys/bus/pci/devices/<PCI-Address>/hwmon/hwmon<#>/power1_input
  telemetryInfo.boardPower = readTelemetryData(basePath + "power1_input");

  /// cat /sys/bus/pci/devices/<PCI-Address>/hwmon/hwmon<#>/power1_max
  telemetryInfo.tdpCap = readTelemetryData(basePath + "power1_max");

  return QS_SUCCESS;
}

int getCtrlChannelPath(std::string &path, const QPciInfo &pciInfo,
                       QCtrlChannelID ccID) {
  std::string pcie_sbdf = qutil::qPciInfoToPCIeStr(pciInfo);
  std::string pathPrefix = "/dev/";
  UdevMap udevChannelMap;
  path = "Invalid";

  std::string aic100_channels[] = {
      "LOOPBACK", "LOOPBACK",  "SAHARA",    "SAHARA",  "DIAG",
      "DIAG",     "SSR",       "SSR",       "QDSS",    "QDSS",
      "CONTROL",  "CONTROL",   "LOGGING",   "LOGGING", "STATUS",
      "STATUS",   "TELEMETRY", "TELEMETRY", "DEBUG",   "DEBUG"};

  if (0 == createUdevMap(udevChannelMap, QAicMhiUciSubsystem.c_str(), 3)) {
    auto range = udevChannelMap.equal_range(pcie_sbdf);
    for (auto it = range.first; it != range.second; ++it) {
      /* Find the right device name */
      if (std::string::npos != it->second.find(aic100_channels[ccID])) {
        path = pathPrefix + it->second;
        break;
      }
    }
  }

  std::ifstream testOpen(path);
  if (testOpen) {
    return 0;
  }

  return 1;
}

QStatus allocUserDmabuf(uint8_t __attribute__((unused)) * *buf,
                        uint32_t __attribute__((unused)) size,
                        __attribute__((unused)) uint64_t *hdl) {

  return QS_UNSUPPORTED;
}

void freeUserDmabuf(void __attribute__((unused)) * vaddr) {}

uint32_t getMemReqSize(size_t size) { return (uint32_t)size; }

ssize_t readlink(const char *path, char *buf, size_t len) {
  return ::readlink(path, buf, len);
}

int getPid() { return static_cast<int>(syscall(SYS_gettid)); }

bool validateCrc32(uint8_t __attribute__((unused)) * crcBuffer,
                   int __attribute__((unused)) crcBufferSize,
                   uint32_t __attribute__((unused)) validateCrc) {
  return 0;
}

const std::string strerror_safe(int errnum) {
  std::vector<char> buf(256);
  return (strerror_r(errnum, buf.data(), buf.size()));
}

const std::string getConfigFilePath() {
  std::string configFilePath = "/opt/qti-aic/config/qaic_config.json";
  return configFilePath;
}

// Message Queue synchronous send and receive
// or send only
int mqSendRecvSync(int sendMqd, int respMqd, const char *in_data,
                   char *out_data, size_t data_siz,
                   devcommon::QDevMqTxfrTypeEnum trType) {
  int rc = -1;
  int rcvBytes = -1;

  if ((sendMqd != -1) && (respMqd != -1)) {
    rc = mq_send(sendMqd, in_data, data_siz, 0);

    if ((rc != -1) &&
        // If we need to wait for a response
        (trType == devcommon::QAIC_DEV_MQ_TXFR_SEND_N_RECV)) {
      rcvBytes = mq_receive(respMqd, out_data, data_siz, NULL);
      if (rcvBytes < 0) {
        rc = -1;
      }
    }
  } else {
    LogErrorG("Invalid mq descriptors");
  }

  return rc;
}

// Send Device command via Message Queue interface
QStatus runMqCmd(QDevHandle &devHandle, QDevCmd &devCmd) {
  QStatus status = QS_SUCCESS;
  devcommon::QDevMqCmd devMqCmd;
  auto sendMqd = devHandle.mqDev[QAIC_DEV_MQ_SEND].mqd;
  auto recvMqd = devHandle.mqDev[QAIC_DEV_MQ_RECV].mqd;
  devcommon::QDevMqTxfrTypeEnum txfrType;
  int rc = -1;

  if (devCmd.cmdRspBuf == nullptr) {
    LogErrorG("Invalid cmd buffer");
    return QS_ERROR;
  }

  devMqCmd.cmd = devCmd.cmdReq;
  std::memcpy(devMqCmd.cmdData, devCmd.cmdRspBuf, devCmd.cmdSize);

  switch (devCmd.cmdType) {
  case QDEV_CMD_TYPE_WRITE_ONLY:
    txfrType = devcommon::QAIC_DEV_MQ_TXFR_SEND_ONLY;
    break;
  case QDEV_CMD_TYPE_WRITE_AND_READ:
    txfrType = devcommon::QAIC_DEV_MQ_TXFR_SEND_N_RECV;
    break;
  default:
    LogErrorG("Unsupported cmdType {}", devCmd.cmdType);
    status = QS_ERROR;
    break;
  }

  if (status != QS_ERROR) {
    rc = mqSendRecvSync(sendMqd, recvMqd, (const char *)&devMqCmd,
                        (char *)devCmd.cmdRspBuf, devCmd.cmdSize, txfrType);
    if (rc < 0) {
      LogErrorG("mqSendRecvSync failed err: {}", strerror_safe(errno));
      status = QS_INVAL;
    }
  }

  return status;
}

// Send Device command via ioctl interface
QStatus runIoctlCmd(int devFd, QDevCmd &devCmd) {
  QStatus status = QS_SUCCESS;
  int rc = -1;

  if (devFd < 0) {
    return QS_INVAL;
  }

  if (devCmd.cmdRspBuf == nullptr) {
    LogErrorG("Invalid data for ioctl");
    return QS_ERROR;
  }

  rc = ioctl(devFd, devCmd.cmdReq, devCmd.cmdRspBuf);
  if (rc < 0) {
    status = QS_INVAL;
  }

  return status;
}

QStatus openDevice(QDevInterfaceEnum devType, const std::string &devPath,
                   QDevHandle &devHandle) {

  std::string mqName;
  QStatus status = QS_SUCCESS;

  switch (devType) {
  case QAIC_DEV_INTERFACE_AIC100:
    devHandle.fileDev.fd = open(devPath.c_str(), O_RDWR | O_CLOEXEC);
    if (devHandle.fileDev.fd == -1) {
      if (errno == ENOENT) {
        // The file does not exist on the filesystem
        status = QS_INVAL;
      } else {
        status = QS_ERROR;
      }
    }
    break;
  default:
    LogErrorG("Unhandled requested devType {}", devType);
    status = QS_ERROR;
    break;
  }

  return status;
}

QStatus runDeviceCmd(QDevInterfaceEnum devType, const QDevHandle &devHandle,
                     QDevCmd &devCmd) {
  QStatus status = QS_SUCCESS;

  switch (devType) {
  case QAIC_DEV_INTERFACE_AIC100:
    status = runIoctlCmd(devHandle.fileDev.fd, devCmd);
    break;
  default:
    LogErrorG("Unhandled requested devType {}", devType);
    status = QS_ERROR;
    break;
  }

  return status;
}

int closeDevice(QDevInterfaceEnum devType, QDevHandle &devHandle) {
  int ret = -1;

  switch (devType) {
  case QAIC_DEV_INTERFACE_AIC100:
    ret = close(devHandle.fileDev.fd);
    break;
  default:
    LogErrorG("Unhandled requested devType {}", devType);
    break;
  }

  return ret;
}


void *qMmap(void *addr, size_t length, int prot, int flags, int fd, uint64_t handle) {
  qaic_mmap_bo mmap_bo = {};
  QDevCmd devCmd;

  mmap_bo.handle = static_cast<uint32_t>(handle);

  devCmd.cmdReq = DRM_IOCTL_QAIC_MMAP_BO;
  devCmd.cmdSize = sizeof(mmap_bo);
  devCmd.cmdType = QDEV_CMD_TYPE_WRITE_AND_READ;
  devCmd.cmdRspBuf = reinterpret_cast<void *>(&mmap_bo);
  QStatus status = runIoctlCmd(fd, devCmd);
  if (status != QS_SUCCESS) {
    LogErrorG("Failed to send MMAP IOCTL for cmd object: {}",
              strerror_safe(errno));
    return reinterpret_cast<void *>(-1);
  }
  off_t mmap_off = static_cast<off_t>(mmap_bo.offset);

  return mmap(addr, length, prot, flags, fd, mmap_off);
}

int qMunmap(void *addr, size_t length) {
  return munmap(addr, length);
}

} // namespace QOsal
} // namespace qaic
