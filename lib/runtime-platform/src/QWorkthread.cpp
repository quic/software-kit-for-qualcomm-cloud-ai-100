// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include <chrono>
#include <condition_variable>

#include "QWorkthread.h"
#include "QLogger.h"
#include "QUtil.h"

namespace qaic {

//======================================================================
// WorkqueueProcessor
//======================================================================
QWorkthread::QWorkthread() noexcept : ready_(false), terminate_(false) {
  init();
}

QWorkthread::~QWorkthread() {}

void QWorkthread::terminate() {
  terminate_ = true;
  cv_.notify_one();
  thread_.join();
}

void QWorkthread::start() {
  std::unique_lock<std::mutex> lock(mutex_);
  ready_ = true;
  cv_.notify_one();
}

void QWorkthread::init() {
  thread_ = std::thread([this] { this->workthread(); });
}

void QWorkthread::notifyReady() {
  std::unique_lock<std::mutex> lock(mutex_);
  ready_ = true;
  cv_.notify_one();
}

void QWorkthread::release() { terminate(); }

// Private
void QWorkthread::workthread() {
  while (1) {
    std::unique_lock<std::mutex> lock(mutex_);
    while (!ready_ && !terminate_) {
      cv_.wait_for(lock, std::chrono::milliseconds(100));
    }
    if (terminate_) {
      break;
    } else {
      ready_ = false;
      lock.unlock();
      run();
      lock.lock();
    }
  }
}

} // namespace qaic
