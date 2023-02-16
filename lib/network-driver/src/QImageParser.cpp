// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QImageParser.h"
#include "metadataflatbufEncode.hpp"
#include "metadataflatbufDecode.hpp"
#include "QDmaElement.h"
#include "QMetaData.h"
#include "elfio/elfio.hpp"

#include "assert.h"
#include <memory>
#include <sstream>

namespace qaic {

const uint64_t McidAddrMultiFac = 0x800000;

std::unique_ptr<QMetaDataInterface> QImageParser::parseImage(const QBuffer &buf,
                                                             QStatus &status) {

  assert(buf.buf && buf.size > 0);
  status = QS_INVAL;

  // Extract the metadata from the ELF file. Parsing the metadata will modify
  // the content
  std::stringstream is(std::string((char *)buf.buf, buf.size));

  ELFIO::elfio elfReader;
  if (!elfReader.load(is)) {
    LogError("Failed to load ELF image");
    return nullptr;
  }

  std::vector<uint8_t> metadataFlat;
  if (elfReader.sections[metadata::networkElfMetadataFBSection]) {
    ELFIO::section *flatSec =
        elfReader.sections[metadata::networkElfMetadataFBSection];
    if (flatSec == nullptr) {
      LogError("Error parsing meta_flat");
      return nullptr;
    }
    metadataFlat = std::vector<uint8_t>(
        flatSec->get_data(), flatSec->get_data() + flatSec->get_size());
  } else {
    ELFIO::section *metaSec = elfReader.sections["metadata"];
    if (metaSec == nullptr) {
      LogError("Error parsing meta");
      return nullptr;
    }
    auto metadataOriginal = std::vector<uint8_t>(
        metaSec->get_data(), metaSec->get_data() + metaSec->get_size());
    metadataFlat =
        metadata::FlatEncode::aicMetadataRawTranslateFlatbuff(metadataOriginal);
  }

  LogDebug("metadata size {}", metadataFlat.size());
  return parse(metadataFlat.data(), metadataFlat.size(), status);
}

//
// Parse from metadata buffer.
//
std::unique_ptr<QMetaDataInterface>
QImageParser::parseBuffer(const QBuffer &buf, QStatus &status) {
  assert(buf.buf && buf.size > 0);
  status = QS_INVAL;

  std::unique_ptr<uint8_t[]> metaBuf =
      std::unique_ptr<uint8_t[]>(new uint8_t[buf.size]);
  memcpy(metaBuf.get(), buf.buf, buf.size);

  return parse(metaBuf.get(), buf.size, status);
}

//
// The real parser.
//
std::unique_ptr<QMetaDataInterface>
QImageParser::parse(uint8_t *metaBuf, uint32_t metaSize, QStatus &status) {
  std::string metadataError;
  std::vector<uint8_t> flatbuf_bytes(metaBuf, metaBuf + metaSize);
  auto md = metadata::FlatDecode::readMetadataFlatNativeCPP(flatbuf_bytes,
                                                            metadataError);
  if (!metadataError.empty()) {
    LogError("{}", metadataError);
    status = QS_INVAL;
    return nullptr;
  }

  // Partially configured request elements are saved in stencil.
  // The request ID and one of the src/dst addresses are not configured.
  QReqElemStencil stencil;
  int32_t bufMaxIndex = -1;
  QElemToBufVec elemToBufVec;

  for (auto &req : md->dmaRequests) {
    int32_t bufIndex = -1;
    DmaTransferDirection dmaDirection = DmaNoTransfer;
    DmaTransferType dmaType = DmaBulkTransfer;
    bool msi = false, needResponse = false, dmaVTCM = true;
    uint8_t dbWrite = 0, dbLen = 0, seqID = 0;
    uint32_t dmaLen = 0, dbData = 0;
    uint64_t srcAddr = 0, dstAddr = 0, addr = 0, dbAddr = 0;
    QReqID reqID = 0;

    bufIndex = req->num;
    if (bufIndex > bufMaxIndex) {
      bufMaxIndex = bufIndex;
    }
    // DMA
    if (req->size) {
      dmaLen = req->size;

      // The address is only an offset. The base address will be added after
      // QSM returns them at network activation.
      dmaVTCM = req->devAddrSpace != AICMDDMAAddrSpaceDDR;
      addr = dmaVTCM ? (McidAddrMultiFac * req->mcId) + req->devOffset
                     : req->devOffset;
      elemToBufVec.push_back(
          std::make_tuple(bufIndex, req->hostOffset, req->size));

      if (req->inOut == AICMDDMAIn) {
        dmaDirection = DmaInbound;
        dstAddr = addr;
      } else {
        dmaDirection = DmaOutbound;
        srcAddr = addr;
        // Only generate response for the last request.
        // The assumption is metadata always put the last outbound request
        // at the end of the list.
        if (req == md->dmaRequests.back()) {
          needResponse = true;
        }
      }
    } else {
      // No DMA
      elemToBufVec.push_back(
          std::make_tuple(bufIndex, 0, 0)); // bufIndex, offset, size
    }
    // Doorbell
    if (req->doorbellOps.size() == 1) {
      auto &dbOp = req->doorbellOps[0];
      // The address is just an offset. The base address will be added after QSM
      //  returns it at network activation.
      dbAddr = (McidAddrMultiFac * dbOp->mcId) + dbOp->offset;
      dbData = dbOp->data;
      dbWrite = 1;
      switch (dbOp->size) {
      case AICMDDoorballOpSize8:
        dbLen = DOORBELL_LEN_8;
        break;
      case AICMDDoorballOpSize16:
        dbLen = DOORBELL_LEN_16;
        break;
      case AICMDDoorballOpSize32:
        dbLen = DOORBELL_LEN_32;
        break;
      }
    }
    // Semaphores
    std::array<SemCmd, 4> semCmd;
    for (uint32_t j = 0; j < req->semaphoreOps.size(); j++) {
      if (j >= semCmd.size()) {
        break;
      }
      auto &semOp = req->semaphoreOps[j];
      semCmd[j] =
          SemCmd(true, (bool)semOp->inSyncFence, (bool)semOp->outSyncFence,
                 SemOp(semOp->semOp),
                 (semOp->preOrPost == AICMDSemaphoreSyncPost) ? SemPostSync
                                                              : SemPreSync,
                 semOp->semNum, semOp->semValue);
    }
    // Save to stencil
    stencil.emplace_back(new QDmaRequestElement(
        reqID, seqID, msi, needResponse, dmaType, dmaDirection, srcAddr,
        dstAddr, dmaLen, dbAddr, dbWrite, dbLen, dbData, semCmd, bufIndex,
        dmaVTCM));
  }

  auto meta = std::unique_ptr<QMetaData>(
      new QMetaData(bufMaxIndex + 1, elemToBufVec, std::move(stencil)));

  LogDebug("{}", meta->getStencilStr());

  status = QS_SUCCESS;
  return meta;
}

} // namespace qaic
