// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QMonitorNamedMutex.h"
#include "QOsalUtils.h"
#include <errno.h>

namespace qaic {

QMonitorNamedMutex::QMonitorNamedMutex(std::string name)
    : QLogger("QMonitorNamedMutex"), shm_desc_(-1), name_(name) {}

QMonitorNamedMutex::~QMonitorNamedMutex() {
  munmap(value_, sizeof(int64_t));
  close(shm_desc_);
}

QStatus QMonitorNamedMutex::init() {
  std::size_t size = sizeof(int64_t); // Enough to store the PID
  mode_t old_umask = umask(0);
  // We will create this on the system once and if needed need to delete
  // this manually
  shm_desc_ = QOsal::shm_open(name_.c_str(), O_CREAT | O_RDWR, 0666);

  umask(old_umask);

  if (shm_desc_ == -1) {
    LogError("Failed to open shared region with errno {}", errno);
    return QS_ERROR;
  }

  if (ftruncate(shm_desc_, size) == -1) {
    LogError("Failed to access region with errno {}", errno);
    return QS_ERROR;
  }

  value_ = (int64_t *)mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED,
                           shm_desc_, 0);

  if (value_ == MAP_FAILED) {
    LogError("Failed to map shared region with errno {}", errno);
    close(shm_desc_);
    shm_desc_ = -1;
    return QS_ERROR;
  }

  return QS_SUCCESS;
}

int64_t QMonitorNamedMutex::getOwnerPID() { return *value_; }

void QMonitorNamedMutex::setOwnerPID() {
  *value_ = getpid();
  msync(value_, sizeof(int64_t), MS_SYNC);
}

void QMonitorNamedMutex::lock() {
  // This is a blocking call
  if (-1 == lockf(shm_desc_, F_LOCK, 0)) {
    LogError("Failed to lock QMonitorNamedMutex with errno {}", errno);
    return;
  }
  setOwnerPID();
}

bool QMonitorNamedMutex::try_lock() {
  if (-1 == lockf(shm_desc_, F_TLOCK, 0)) {
    return false;
  }

  setOwnerPID();
  return true;
}

void QMonitorNamedMutex::unlock() {
  // This is a blocking call
  if (-1 == lockf(shm_desc_, F_ULOCK, 0)) {
    LogError("Failed to unlock QMonitorNamedMutex with errno {}", errno);
    return;
  }
}

std::unique_ptr<QMonitorNamedMutex>
QMonitorNamedMutexBuilder::buildNamedMutex(std::string name) {
  std::unique_ptr<QMonitorNamedMutex> retMtx =
      std::make_unique<QMonitorNamedMutex>(name);

  if (QS_SUCCESS == retMtx->init()) {
    return retMtx;
  } else {
    return nullptr;
  }
}
} // namespace qaic
