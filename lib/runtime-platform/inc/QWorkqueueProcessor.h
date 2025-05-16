// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QWORKQUEUEPROCESSOR_H
#define QWORKQUEUEPROCESSOR_H

#include "QWorkqueueElementInterface.h"
#include "QLogger.h"

#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <array>
#include <string>
#include <utility>
namespace qaic {

enum QWorkqueueProcessorTypes {
  CHANNEL_WORKQUEUE,
  SERVICES_WORKQUEUE,
  DIAG_ASYNC_WORKQUEUE,
  QIQUEUE_SUBMIT_WORKQUEUE,
  QIQUEUE_FINISH_WORKQUEUE,
  DEVICE_STATE_WORKQUEUE,
  QAIC_LRT_DATA_WORKQUEUE,
  QContext_WORKQUEUE,
  NUM_WORKQUEUES
};

class QWorkqueueProcessor;
using QWorkqueueElementEntry =
    std::pair<WorkqueueElementCB, QWorkqueueElementInterface *>;
using QWorkqueueProcessorShared = std::shared_ptr<QWorkqueueProcessor>;

class QWorkqueueProcessor : public QLogger {
public:
  QWorkqueueProcessor(std::string name) noexcept;
  virtual ~QWorkqueueProcessor();

  /// Mark wqElement as ready to run
  void ready(WorkqueueElementCB &wqElem, QWorkqueueElementInterface *sourceObj);

  /// Remove all references to wqElement in the local processor database
  /// It is expected that clenanup is called from the workqueue Element
  /// destructor
  void cleanup(QWorkqueueElementInterface *element);

  /// \return true if element has pending execution(s)
  bool isPending(QWorkqueueElementInterface *element);

  bool isPending() { return (workQueue_.size() != 0); }

  /// One-time initialization
  QStatus init();

  /// Exit the processor thread
  void terminate();

  /// Run function for th thread
  void run();

  const std::string &name() { return name_; }

private:
  std::deque<QWorkqueueElementEntry> workQueue_;
  std::condition_variable cv_;
  std::mutex mutex_;
  std::thread thread_;
  bool ready_;
  bool terminate_;
  std::string name_;
};

class QWorkqueueProcessorManager {
public:
  static QWorkqueueProcessorShared
  getWorkqueueProcessor(QWorkqueueProcessorTypes type = CHANNEL_WORKQUEUE);
  static std::array<QWorkqueueProcessorShared, NUM_WORKQUEUES>
      workqueueProcessor_;

private:
  static std::mutex m_;
};

} // namespace qaic

#endif // QWORKQUEUEPROCESSOR_H
