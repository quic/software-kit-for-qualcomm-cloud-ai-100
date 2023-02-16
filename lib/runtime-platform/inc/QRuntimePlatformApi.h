// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QRUNTIME_PLATFORM_API_H
#define QRUNTIME_PLATFORM_API_H

#include "QAicRuntimeTypes.h"
#include "QRuntimePlatformInterface.h"
#include <vector>

namespace qaic {
namespace rtPlatformApi {

shQRuntimePlatformInterface qaicGetRuntimePlatform();

} // namespace rtPlatformApi
} // namespace qaic
#endif // QRUNTIME_PLATFORM_API_H
