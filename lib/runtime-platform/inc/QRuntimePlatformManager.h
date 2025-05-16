// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QRUNTIME_PLATFORM_MANAGER_H
#define QRUNTIME_PLATFORM_MANAGER_H

#include "QRuntimePlatformInterface.h"

#include <mutex>

namespace qaic {

class QRuntimePlatformManager {
public:
  static shQRuntimePlatformInterface getRuntimePlatform();

private:
  static shQRuntimePlatformInterface runtimePlatform_;
  static std::mutex m_;

  static shQRuntimePlatformInterface getKmdRuntimePlatform();
};

} // namespace qaic

#endif // QRUNTIME_PLATFORM_MANAGER_H
