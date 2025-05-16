// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QNNImage.h"
#include "QDeviceInterface.h"
#include "QMetaDataInterface.h"
#include "QUtil.h"

namespace qaic {

QStatus QNNImage::unload() {
  if (refCnt_ != 0) {
    return QS_BUSY;
  }
  QStatus status = dev_->unloadImage(imageID_);
  delete this;
  return status;
}

} // namespace qaic
