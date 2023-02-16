// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QRuntimePlatformApi.h"
#include "QRuntimePlatformManager.h"

#include <vector>

namespace qaic {
namespace rtPlatformApi {

shQRuntimePlatformInterface qaicGetRuntimePlatform() {
  return QRuntimePlatformManager::getRuntimePlatform();
}

} // namespace rtPlatformApi
} // namespace qaic
