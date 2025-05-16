// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QNN_CONSTANTS_H
#define QNN_CONSTANTS_H

#include <atomic>
#include "QNNConstantsInterface.h"

namespace qaic {

class QDeviceInterface;

class QNNConstants : public QNNConstantsInterface {
public:
  QNNConstants(QDeviceInterface *dev, QID qid, QNNConstantsID constantsID)
      : dev_(dev), qid_(qid), constantsID_(constantsID), refCnt_(0) {}

  QNNConstantsID getConstantsID() const override { return constantsID_; }

  QID getQID() const override { return qid_; }

  QStatus unload() override;

  QStatus loadConstantsAtOffset(const QBuffer &buf, uint64_t offset) override;

  ~QNNConstants() {}

private:
  QDeviceInterface *dev_;
  QID qid_;
  QNNConstantsID constantsID_;
  /// The ref count is increased every time the constants used for activation,
  /// and is descreased at deactivation.
  std::atomic<uint32_t> refCnt_;

  void incRef() { refCnt_++; }

  void decRef() { refCnt_--; }

  friend class QRuntime;
  friend class QNeuralNetwork;
  friend class QNeuralnetwork;
};

} // namespace qaic

#endif // QNN_CONSTANTS_H
