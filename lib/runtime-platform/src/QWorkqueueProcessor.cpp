// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
#include "QWorkqueueProcessor.h"
#include "QWorkqueueElement.h"
#include "QUtil.h"
#include <chrono>

namespace qaic {

std::array<QWorkqueueProcessorShared, NUM_WORKQUEUES>
    QWorkqueueProcessorManager::workqueueProcessor_;
std::mutex QWorkqueueProcessorManager::m_;

std::string workQueueNames[NUM_WORKQUEUES] = {
    "CHANNEL_WORKQUEUE",        "SERVICES_WORKQUEUE",
    "DIAG_ASYNC_WORKQUEUE",     "QIQUEUE_SUBMIT_WORKQUEUE",
    "QIQUEUE_FINISH_WORKQUEUE", "DEVICE_STATE_WORKQUEUE",
    "QAIC_LRT_DATA_WORKQUEUE",  "QContext_WORKQUEUE"};

//======================================================================
// WorkqueueProcessorManager
//======================================================================
// QWorkqueueProcessorManager::QWorkqueueProcessorManager() {}

QWorkqueueProcessorShared QWorkqueueProcessorManager::getWorkqueueProcessor(
    QWorkqueueProcessorTypes type) {
  std::unique_lock<std::mutex> lk(m_);

  if (type >= QWorkqueueProcessorManager::workqueueProcessor_.size()) {
    return nullptr;
  }
  if (QWorkqueueProcessorManager::workqueueProcessor_[type] == nullptr) {
    QWorkqueueProcessorManager::workqueueProcessor_[type] =
        std::make_unique<QWorkqueueProcessor>(workQueueNames[type]);
  }
  return QWorkqueueProcessorManager::workqueueProcessor_[type];
}

//======================================================================
// WorkqueueProcessor
//======================================================================
QWorkqueueProcessor::QWorkqueueProcessor(std::string name) noexcept
    : ready_(false),
      terminate_(false),
      name_(name) {
  init();
}

QWorkqueueProcessor::~QWorkqueueProcessor() { terminate(); }

void QWorkqueueProcessor::terminate() {
  terminate_ = true;
  cv_.notify_all();
  thread_.join();
}

QStatus QWorkqueueProcessor::init() {
  thread_ = std::thread([this] { this->run(); });
  return QS_SUCCESS;
}

void QWorkqueueProcessor::ready(WorkqueueElementCB &wqElem,
                                QWorkqueueElementInterface *sourceObj) {

  std::unique_lock<std::mutex> lock(mutex_);
  workQueue_.push_back(std::make_pair(wqElem, sourceObj));
  ready_ = true;
  LogTrace("Wk add size:{}", workQueue_.size());
  cv_.notify_all();
}

void QWorkqueueProcessor::run() {

  while (1) {
    std::unique_lock<std::mutex> lock(mutex_);
    while (!ready_ && !terminate_) {
      cv_.wait_for(lock, std::chrono::milliseconds(100));
    }
    if (terminate_) {
      break;
    } else {
      LogTrace("Wk process entry size {} name:{}", workQueue_.size(), name());
      while (!workQueue_.empty()) {
        LogTrace("Wk process pre size {} name:{}", workQueue_.size(), name());
        QWorkqueueElementEntry wqEntry = workQueue_.front();
        workQueue_.pop_front();
        LogTrace("Wk process post size {} name:{}", workQueue_.size(), name());

        lock.unlock();
        wqEntry.first(); // Execute
        lock.lock();
      }
      if (workQueue_.empty()) {
        ready_ = false;
        LogTrace("Wk empty");
      }
    }
  }
}

void QWorkqueueProcessor::cleanup(QWorkqueueElementInterface *element) {
  // Find the elements to remove based on the saved object (item 1 in tuple)
  std::unique_lock<std::mutex> lock(mutex_);
  for (auto it = workQueue_.cbegin(); it != workQueue_.cend();) {
    if (it->second == element) {
      it = workQueue_.erase(it);
    } else {
      ++it;
    }
  }
}

bool QWorkqueueProcessor::isPending(QWorkqueueElementInterface *element) {
  // Find the elements to remove based on the saved object (item 1 in tuple)
  std::unique_lock<std::mutex> lock(mutex_);
  for (auto it = workQueue_.begin(); it != workQueue_.end(); it++) {
    if ((*it).second == element) {
      return true;
    }
  }
  return false;
}

} // namespace qaic
