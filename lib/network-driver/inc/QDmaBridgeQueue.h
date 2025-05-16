// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QDMA_BRIDGE_QUEUE_H
#define QDMA_BRIDGE_QUEUE_H

#include "QLogger.h"

#include <assert.h>
#include <cstdint>

#include <atomic>
#include <vector>

namespace qaic {

/// The Common layer and the Driver layer use the QDmaBridgeQueue class
/// for data exchange in data path. This class emulates the interfaces of
/// DMA Bridge hardware. Functions in this class are NOT thread-safe.
///
class QDmaBridgeQueue : public QLogger {
public:
  /// Ctor
  /// This class is used by both request queue and response queue.
  /// Here \p lenPwr2 must be no larger than 16.
  QDmaBridgeQueue(uint8_t *base, uint32_t lenPow2, uint32_t itemSize)
      : QLogger("QDmaBridgeQueue"), base_(base), len_(1 << lenPow2),
        itemSize_(itemSize), tp_(0), hp_(0) {
    assert(lenPow2 <= 16);
    assert(lenPow2 > 0);
  }

  /// The queue is full when tail pointer is one less than head pointer.
  bool isFull() const { return (hp_ == ((tp_ + 1) & (len_ - 1))); }

  bool isEmpty() const { return (hp_ == tp_); }

  ///\returns the count of used slots.
  uint32_t count() const { return (tp_ - hp_ + len_) % len_; }

  ///\returns number of available slots
  uint32_t space() const { return len_ - count() - 1; }

  void advanceTP(uint32_t n) {
    assert(n < len_);
    tp_ = (tp_ + n) & (len_ - 1);
  }

  void advanceHP(uint32_t n) {
    assert(n < len_);
    hp_ = (hp_ + n) & (len_ - 1);
  }

  void setTP(uint32_t tp) {
    assert(tp < len_);
    tp_ = tp;
  }

  void setHP(uint32_t hp) {
    assert(hp < len_);
    hp_ = hp;
  }

  uint32_t getTP() const { return tp_; }

  uint32_t getHP() const { return hp_; }

  /// Collect the addresses of the next N free slots and put them in \p vec.
  /// \returns the number of items added to vec. The number may be less than n.
  std::size_t getNextNForProducer(uint32_t n,
                                  std::vector<uint8_t *> &vec) const {
    uint32_t tp = tp_, min = std::min(n, space());
    LogDebug("{} tp_ {} hp_ {} n {}", (itemSize_ == 4 ? "resp" : "req"), tp,
             (uint32_t)hp_, n);
    if (isFull()) {
      return 0;
    }
    for (uint32_t i = 0; i < min; i++) {
      vec.push_back(base_ + tp * itemSize_);
      tp = (tp + 1) & (len_ - 1);
    }
    return vec.size();
  }

  /// Get pointers to items owned by consumer (from HP to TP).
  /// If \p n is  0 then the function will return all consumer
  /// owned items.
  std::size_t getConsumerOwned(uint32_t n, std::vector<uint8_t *> &vec) const {
    uint32_t hp = hp_, i = 0, tp = tp_;
    LogDebug("{} tp_ {} hp_ {} n {}", (itemSize_ == 4 ? "resp" : "req"), tp, hp,
             n);
    if (isEmpty()) {
      return 0;
    }
    while (hp != tp) {
      vec.push_back(base_ + itemSize_ * hp);
      hp = (hp + 1) & (len_ - 1);
      if (n > 0 && ++i == n) {
        break;
      }
    }
    return vec.size();
  }

private:
  uint8_t *base_;
  uint32_t len_;
  uint32_t itemSize_;
  std::atomic<uint32_t> tp_;
  std::atomic<uint32_t> hp_;
};

} // namespace qaic

#endif // QDMA_BRIDGE_QUEUE_H
