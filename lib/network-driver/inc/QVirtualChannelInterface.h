// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QIVIRTUAL_CHANNEL_H
#define QIVIRTUAL_CHANNEL_H

#include "QDmaBridgeQueue.h"
#include "QAicRuntimeTypes.h"
#include "QLogger.h"
#include "QTypes.h"
#include "QDmaElement.h"

#include <functional>
#include <memory>
#include <vector>

namespace qaic {

class QDmaBridgeQueue;

using VCNotify = std::function<void(QStatus)>;

/// This class exposes the virtual channel interface to be consumed by
/// the frontend. This class wraps the request and response queues and
/// related queue operations. Backend driver implements this interface
/// to customize the virtual channel behavior for the particular driver.
/// For example, in advanceReqQueueTP() the driver may start processing
/// requests in the queue, rather than just move the tail pointer.
///
class QVirtualChannelInterface {
public:
  QVirtualChannelInterface(QID devID, QVCID vc, uint8_t *reqQBase,
                           uint8_t *respQBase, uint32_t qSizePow2)
      : devID_(devID), vc_(vc), refCnt_(0),
        reqQ_(std::make_unique<QDmaBridgeQueue>(reqQBase, qSizePow2,
                                                QDmaRequestElement::getSize())),
        respQ_(std::make_unique<QDmaBridgeQueue>(
            respQBase, qSizePow2, QDmaResponseElement::getSize())),
        queueSize_(1 << qSizePow2), pcMask_(0){};

  /// Obtain the ID of the virtual channel
  QVCID getVC() const { return vc_; }

  /// Advance the tail pointer of the request queue. Override this function
  /// to add actions that could be triggered by the TP advancement.
  virtual void advanceReqQueueTP(uint32_t n) { reqQ_->advanceTP(n); }

  /// Advance the head pointer of the response queue. Overrie this function
  /// to add actions that could be triggered by the HP advancement.
  virtual void advanceRespQueueHP(uint32_t n) { respQ_->advanceHP(n); }

  /// The QVirtualChannelInterface object will use the \p notifyCB to notify
  /// the frontend that there are items in response queue.
  virtual void setNotifier(VCNotify){};

  /// Asking for the next N request spaces from request queue
  /// \returns number of spaces which may be less than asked for in \p n.
  virtual std::size_t getRequestSpace(uint32_t n,
                                      std::vector<uint8_t *> &vec) const {
    return reqQ_->getNextNForProducer(n, vec);
  }

  /// Ask for the next N responses from response queue
  virtual std::size_t getResponse(uint32_t n,
                                  std::vector<uint8_t *> &vec) const {
    return respQ_->getConsumerOwned(n, vec);
  }

  /// Check if request queue is empty
  virtual bool hasRequest() const { return !reqQ_->isEmpty(); }

  /// Check if response queue is empty
  virtual bool hasResponse() const { return !respQ_->isEmpty(); }

  virtual ~QVirtualChannelInterface() = default;

  uint32_t getRefCnt() const { return refCnt_.load(); }

  void incRef() { refCnt_++; }
  void decRef() { refCnt_--; }
  void setPcMask(uint32_t pcMask) { pcMask_ = pcMask; }

  virtual uint32_t getQueueSize() const { return queueSize_; }

protected:
  /// The QID of the device the virtual channels belongs to.
  /// In this class the i
  const QID devID_;
  /// The virtual channel number
  const QVCID vc_;

  /// When created this is 1, if it's a reservation and multiple activations use
  /// this
  /// channel, reference count is incremented
  /// channel is torn-down only when last reference is removed.
  std::atomic<uint32_t> refCnt_;

  /// The request queue
  std::unique_ptr<QDmaBridgeQueue> reqQ_;
  /// The response queue
  std::unique_ptr<QDmaBridgeQueue> respQ_;

  /// Queue size of the virtual channel
  const uint32_t queueSize_;

  // The Physical(DMA) Channel mask to which VC is mapped to
  uint32_t pcMask_;
};

} // namespace qaic

#endif // QIVIRTUAL_CHANNEL_H
