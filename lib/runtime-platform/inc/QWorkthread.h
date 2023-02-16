// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QWORKTHREAD_H
#define QWORKTHREAD_H

#include "QLogger.h"
#include "QAicRuntimeTypes.h"

#include <functional>
#include <memory>

namespace qaic {

class QWorkthread {
public:
  QWorkthread() noexcept;
  virtual ~QWorkthread();
  virtual void run() = 0;
  void notifyReady();
  void start();
  void release();

private:
  void init();
  void workthread();
  void terminate();
  std::condition_variable cv_;
  std::mutex mutex_;
  bool ready_;
  bool terminate_;
  std::thread thread_;
};

} // namespace qaic

#endif // QWORKTHREAD_H
