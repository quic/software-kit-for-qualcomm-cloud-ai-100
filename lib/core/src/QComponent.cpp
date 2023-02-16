// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QComponent.h"
#include "QContext.h"

namespace qaic {

QComponent::QComponent(const char *name, QAicObjId id,
                       QContextInterface *context)
    : Id_(id), componentName_(name) {
  QLogControl::getLogControl()->makeLogger(context->getContextName(), logger_);
}

} // namespace qaic
