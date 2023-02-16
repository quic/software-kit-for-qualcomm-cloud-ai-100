// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QDevInterface.h"
#include "QDevAic100Interface.h"
#include "QLogger.h"
#include "QNncProtocol.h"

#include <sys/ioctl.h>

namespace qaic {

QDevInterfaceAic100::QDevInterfaceAic100(QID qid, const std::string &devName)
    : QRuntimePlatformDeviceAic100(qid, devName) {}

bool QDevInterfaceAic100::init() { return true; }

QStatus QDevInterfaceAic100::freeMemReq(uint8_t *boReqPtr, uint32_t numReqs) {
  int memReqEntryProcessed = 0;
  drm_gem_close close_bo = {};

  for (uint32_t i = 0; i < numReqs; i++) {
    qaic_create_bo *createBO =
        reinterpret_cast<qaic_create_bo *>(boReqPtr + ((sizeof(qaic_create_bo) + sizeof(qaic_attach_slice)) * i +
                                  sizeof(qaic_attach_slice_entry) * memReqEntryProcessed));
    qaic_attach_slice *attach_slice = reinterpret_cast<qaic_attach_slice *>(createBO + 1);
    if (attach_slice->hdr.dbc_id != invalidBdcId) {
      // Make sure we call free on all memReq, even if rc is != SUCCESS
      close_bo.handle = createBO->handle;
      runDevCmd(QAIC_DEV_CMD_FREE_MEM, &close_bo);
    }
    attach_slice->hdr.dbc_id = invalidBdcId;
    memReqEntryProcessed += attach_slice->hdr.count;
  }

  return QS_SUCCESS;
}

} // namespace qaic
