// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QRUNTIMEMANAGER_H
#define QRUNTIMEMANAGER_H

#include <memory>
#include <mutex>
#include "QLogInterface.h"

namespace qaic {

class QRuntimeInterface;

class QRuntimeManager {
public:
  static QRuntimeInterface *getRuntime();
  /// This is a backdoor for testing
  static void setRuntime(std::unique_ptr<QRuntimeInterface> rt);
  static void setLogger(qlog::QLogInterface *logger);
  static qlog::QLogInterface *getLogger();
  static bool checkFWVersions(QRuntimeInterface *rt);

private:
  static std::unique_ptr<QRuntimeInterface> runtime_;
  static qlog::QLogInterface *logger_;
  static std::mutex m_;
};

} // namespace qaic

#endif // QRUNTIMEMANAGER_H
