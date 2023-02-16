// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QPRD_KMDVC_H
#define QPRD_KMDVC_H

#include "QVCFactoryInterface.h"
#include "QVirtualChannelInterface.h"

#include <atomic>
#include <condition_variable>
#include <memory>
#include <thread>

namespace qaic {

class QKmdVC : public QVirtualChannelInterface {
public:
  QKmdVC(QID devID, QVCID vc, uint32_t qSizePow2)
      : QVirtualChannelInterface(devID, vc, 0, 0, qSizePow2){};

  void advanceReqQueueTP(uint32_t) override{};
  void advanceRespQueueHP(uint32_t) override{};
  void setNotifier(VCNotify) override{};
  std::size_t getRequestSpace(uint32_t,
                              std::vector<uint8_t *> &) const override {
    return 0;
  }

  std::size_t getResponse(uint32_t, std::vector<uint8_t *> &) const override {
    return 0;
  }

  bool hasRequest() const override { return true; }

  bool hasResponse() const override { return true; }

  ~QKmdVC() = default;
};

} // namespace qaic

#endif // QPRD_KMDVC_H
