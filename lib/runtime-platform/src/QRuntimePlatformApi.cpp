// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
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
