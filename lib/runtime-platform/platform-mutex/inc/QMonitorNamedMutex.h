// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QMONITOR_NAMED_MUTEX_H
#define QMONITOR_NAMED_MUTEX_H

#include "QLogger.h"
#include "QAicRuntimeTypes.h"
#include <fcntl.h>
#include <iostream>

namespace qaic {
class QMonitorNamedMutex : public QLogger {
public:
  typedef QStatus native_handle_type;

  QMonitorNamedMutex() = delete;
  QMonitorNamedMutex(const QMonitorNamedMutex &other) = delete;
  QMonitorNamedMutex &operator=(const QMonitorNamedMutex &other) = delete;
  virtual ~QMonitorNamedMutex();

  QMonitorNamedMutex(std::string name);

  QStatus init();
  int64_t getOwnerPID();
  void lock();
  bool try_lock();
  void unlock();
  native_handle_type native_handle() { return QS_UNSUPPORTED; }

private:
  void setOwnerPID();
  int64_t *value_;
  int shm_desc_;
  std::string name_;
};

class QMonitorNamedMutexBuilder {
public:
  static std::unique_ptr<QMonitorNamedMutex> buildNamedMutex(std::string name);
};
} // namespace qaic
#endif // QMONITOR_NAMED_MUTEX_H
