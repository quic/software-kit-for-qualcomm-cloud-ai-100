// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QWorkqueueElement.h"
#include "QWorkqueueProcessor.h"

#include <memory>

namespace qaic {

QWorkqueueElement::QWorkqueueElement(QWorkqueueProcessorTypes wqType)
    : wq_(nullptr), wqType_(wqType) {
  init();
}

void QWorkqueueElement::init() {
  wq_ = QWorkqueueProcessorManager::getWorkqueueProcessor(wqType_);
  runCb_ = std::bind(&QWorkqueueElement::run, this);
}

QWorkqueueElement::~QWorkqueueElement() {
  // wq_->cleanup(this);
  // wq_.reset();
  // wq_ = nullptr;
  runCb_ = nullptr;
}

QStatus QWorkqueueElement::notifyReady() {
  if ((wq_ != nullptr) && (runCb_ != nullptr)) {
    wq_->ready(runCb_, this);
    return QS_SUCCESS;
  }
  return QS_ERROR;
}

bool QWorkqueueElement::isPending() {
  if (wq_ == nullptr) {
    return false;
  }
  return wq_->isPending(this);
}

} // namespace qaic
