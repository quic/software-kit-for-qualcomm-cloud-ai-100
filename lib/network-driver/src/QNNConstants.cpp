// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QNNConstants.h"
#include "QDeviceInterface.h"

namespace qaic {

QStatus QNNConstants::unload() {
  if (refCnt_ != 0) {
    return QS_BUSY;
  }
  QStatus status = QS_SUCCESS;
  if (constantsID_ != 0) {
    status = dev_->unloadConstants(constantsID_);
  }
  delete this;
  return status;
}

QStatus QNNConstants::loadConstantsAtOffset(const QBuffer &buf,
                                            uint64_t offset) {

  QStatus status = dev_->loadConstantsAtOffset(constantsID_, buf, offset);

  // In case of failure, device will release the resources
  // Set constantsID_ to 0 to skip sending the unload request.
  if (status != QS_SUCCESS) {
    constantsID_ = 0;
  }

  return status;
}

} // namespace qaic
