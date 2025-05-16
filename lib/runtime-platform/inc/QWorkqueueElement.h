// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QWORKQUEUEELEMENT_H
#define QWORKQUEUEELEMENT_H

#include "QWorkqueueProcessor.h"
#include "QWorkqueueElementInterface.h"
#include "QAicRuntimeTypes.h"

#include <functional>
#include <memory>

namespace qaic {

/// This class defines the interface to the Qmonitor services
class QWorkqueueElement : public QWorkqueueElementInterface {
public:
  /// from QWorkqueueElementInterface
  virtual QStatus notifyReady() override;

  /// from QWorkqueueElementInterface
  virtual bool isPending() override;

  QWorkqueueElement(QWorkqueueProcessorTypes wqType);
  virtual ~QWorkqueueElement();

  void wqDeletePendingRequests() { wq_->cleanup(this); }

private:
  void init();
  WorkqueueElementCB runCb_;
  QWorkqueueProcessorShared wq_;
  QWorkqueueProcessorTypes wqType_;
};

} // namespace qaic

#endif // QWorkqueueElement_H
