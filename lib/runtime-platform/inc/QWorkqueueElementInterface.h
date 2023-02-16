// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QWorkqueueElementINTERFACE_H
#define QWorkqueueElementINTERFACE_H

#include "QWorkqueueProcessor.h"
#include "QAicRuntimeTypes.h"

#include <functional>
#include <memory>

namespace qaic {

class QWorkqueueProcessor;

using WorkqueueElementCB = std::function<void()>;

class QWorkqueueElementInterface {
public:
  /// from QWorkqueueElementInterface, this function will be called for
  /// derived class to do work.  It should not block.
  virtual void run() = 0;

  virtual QStatus notifyReady() = 0;

  virtual bool isPending() = 0;

  virtual ~QWorkqueueElementInterface() = default;
};

} // namespace qaic

#endif // QWorkqueueElementINTERFACE_H
