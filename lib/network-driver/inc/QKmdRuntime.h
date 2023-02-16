// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QKMD_RUNTIME_H
#define QKMD_RUNTIME_H

#include <memory>
#include "QRuntimeInterface.h"

namespace qaic {

class QRuntimeInterface;

std::unique_ptr<QRuntimeInterface> getKmdRuntime();

} // namespace qaic

#endif // QKMD_RUNTIME_H
