// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QRuntimeManager.h"
#include "QRuntime.h"

namespace qaic {

QRuntimeInterface *getRuntime(QLogInterface *logger) {
  QRuntimeManager::setLogger(logger);
  return QRuntimeManager::getRuntime();
}

void getRuntimeVersion(QRTVerInfo &version) {
  QRuntime::getRuntimeVersion(version);
}

} // namespace qaic
