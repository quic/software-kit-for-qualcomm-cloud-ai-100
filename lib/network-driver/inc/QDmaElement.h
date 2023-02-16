// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QDMA_ELEMENT_H
#define QDMA_ELEMENT_H

#include "QAicRuntimeTypes.h"
#include "QUtil.h"
#include "QDevAic100Interface.h"

#include "string.h"
#include <array>
#include <cstdint>
#include <memory>
#include <sstream>

namespace qaic {

enum SemOp {
  SemCmdNop = 0,
  SemCmdInitialize = 1,
  SemCmdIncrement = 2,
  SemCmdDecrement = 3,
  SemCmdWaitEqual = 4,
  SemCmdWaitGtrEqual = 5,
  SemCmdP = 6,
  SemCmdUnknown = 7
};

enum SemSync { SemPostSync = 0, SemPreSync = 1 };

constexpr const int32_t IDX_NO_DMA = -1;
const uint32_t NUM_GSM_SEM = 32;
const uint32_t MAX_SEM_VAL = 4095;

struct SemCmd {
  bool semEnable_;
  bool semDmaIBSync_;
  bool semDmaOBSync_;
  SemOp semCmdID_;
  bool semSync_;
  uint8_t semID_;
  uint16_t semValue_;

  SemCmd()
      : semEnable_(0), semDmaIBSync_(0), semDmaOBSync_(0), semCmdID_(SemCmdNop),
        semSync_(0), semID_(0), semValue_(0) {}

  SemCmd(bool enable, bool ib_sync, bool ob_sync, SemOp cmd, bool sync,
         uint8_t index, uint16_t val)
      : semEnable_(enable), semDmaIBSync_(ib_sync), semDmaOBSync_(ob_sync),
        semCmdID_(cmd), semSync_(sync), semID_(index), semValue_(val) {}

  void decode(uint32_t data) {
    semEnable_ = (data & (0x1 << 31)) >> 31;
    semDmaIBSync_ = (data & (0x1 << 30)) >> 30;
    semDmaOBSync_ = (data & (0x1 << 29)) >> 29;
    semCmdID_ = (SemOp)((data & (0x7 << 24)) >> 24);
    semSync_ = (data & (0x1 << 22)) >> 22;
    semID_ = (data & (0x1f << 16)) >> 16;
    semValue_ = data & (0xfff);
  }

  void encode(uint32_t *data) {
    uint32_t temp = (semEnable_ << 31);
    temp = temp | (semDmaIBSync_ << 30);
    temp = temp | (semDmaOBSync_ << 29);
    temp = temp | (semCmdID_ << 24);
    temp = temp | (semSync_ << 22);
    temp = temp | (semID_ << 16);
    temp = temp | (semValue_);
    *data = temp;
  }

  bool isValid() const {
    if (semID_ >= NUM_GSM_SEM) {
      return false;
    }
    if (semCmdID_ >= SemCmdUnknown) {
      return false;
    }
    if (semValue_ > MAX_SEM_VAL) {
      return false;
    }
    return true;
  }

  static const char *semOpStr(SemOp op) {
    switch (op) {
    case SemCmdNop:
      return "Nop";
    case SemCmdInitialize:
      return "Initialize";
    case SemCmdIncrement:
      return "Increment";
    case SemCmdDecrement:
      return "Decrement";
    case SemCmdWaitEqual:
      return "WaitEqual";
    case SemCmdWaitGtrEqual:
      return "WaitGtrEqual";
    case SemCmdP:
      return "P";
    case SemCmdUnknown:
    default:
      return "Unknown";
    }
  }

  std::string str() const {
    std::stringstream ss;
    ss << "enable:" << (semEnable_ ? "y" : "n")
       << " ib_sync:" << (semDmaIBSync_ ? "y" : "n")
       << " ob_sync:" << (semDmaOBSync_ ? "y" : "n")
       << " op:" << semOpStr(semCmdID_) << " sync:" << (semSync_ ? "y" : "n")
       << " id:" << (uint32_t)semID_ << " value:0x" << qutil::hex(semValue_);
    return ss.str();
  }
};

/// DMA Command definition
enum DmaTransferType { DmaLinkListTransfer = 0, DmaBulkTransfer = 1 };

enum DmaTransferDirection {
  DmaNoTransfer = 0,
  DmaInbound = 1,
  DmaOutbound = 2
};

enum DoorbellLength {
  DOORBELL_LEN_32 = 0,
  DOORBELL_LEN_16 = 1,
  DOORBELL_LEN_8 = 2
};

struct DmaCmd {
  bool forceMSI_;
  bool genResponseCompletion_;
  DmaTransferType transferType_;
  DmaTransferDirection transferDirection_;

  DmaCmd()
      : forceMSI_(0), genResponseCompletion_(0),
        transferDirection_(DmaNoTransfer) {}

  DmaCmd(bool msi, bool resp, DmaTransferType transfer_type,
         DmaTransferDirection transfer_dir)
      : forceMSI_(msi), genResponseCompletion_(resp),
        transferType_(transfer_type), transferDirection_(transfer_dir) {}

  void decode(uint8_t cmd) {
    forceMSI_ = (cmd & (0x1 << 7)) >> 7;
    genResponseCompletion_ = ((cmd & (0x1 << 4)) >> 4);
    transferType_ = (DmaTransferType)((cmd & (0x1 << 3)) >> 3);
    transferDirection_ = (DmaTransferDirection)(cmd & (0x3));
  }

  void encode(uint8_t *cmd) {
    uint8_t temp = (forceMSI_ << 7);
    temp = temp | (genResponseCompletion_ << 4);
    temp = temp | (transferType_ << 3);
    temp = temp | (transferDirection_);
    *cmd = temp;
  }

  bool isValid() const {
    return !((transferDirection_ != DmaOutbound) &&
             (transferDirection_ != DmaInbound) &&
             (transferDirection_ != DmaNoTransfer));
  }

  std::string str() const {
    std::stringstream ss;
    DmaTransferDirection dir = transferDirection_;
    ss << "dir:"
       << (dir == DmaInbound
               ? "Inbound"
               : (dir == DmaOutbound
                      ? "Outbound"
                      : (dir == DmaNoTransfer ? "No-DMA" : "Invalid")))
       << " msi:" << (forceMSI_ ? "y" : "n")
       << " type:" << (transferType_ == DmaBulkTransfer ? "Bulk" : "LinkedList")
       << " response:" << (genResponseCompletion_ ? "y" : "n");
    return ss.str();
  }
};

/// DMA Request Element
class QDmaRequestElement {
public:
  static uint32_t getSize() { return DmaRequestElementSize_; }

  QDmaRequestElement(const uint8_t *data)
      : data_((uint32_t *)data), bufIndex_(IDX_NO_DMA), dmaVTCM_(true) {
    decode();
  }

  /// \p bufIndex is the index of the buffer this element is assigned to.
  /// A value of IDX_NO_DMA means not assigned.
  QDmaRequestElement(QReqID req, uint8_t seqID,
                     // fields for DMA command
                     bool msi, bool resp, DmaTransferType xferType,
                     DmaTransferDirection xferDirection,
                     // end fields for DMA command
                     uint64_t src, uint64_t dst, uint32_t dmaLen,
                     uint64_t dbAddr, uint8_t dbWrite, uint8_t dbLen,
                     uint32_t dbData, std::array<SemCmd, 4> semCmd,
                     int32_t bufIndex = IDX_NO_DMA, bool dmaVTCM = true)
      : buf_(new uint32_t[DmaRequestElementSize_ / sizeof(uint32_t)]),
        data_(buf_.get()), reqID_(req), seqID_(seqID),
        dmaCmd_(msi, resp, xferType, xferDirection), dmaSrcAddr_(src),
        dmaDstAddr_(dst), dmaLen_(dmaLen), nspDoorbellAddr_(dbAddr),
        nspDoorbellWrite_(dbWrite), nspDoorbellLen_(dbLen),
        nspDoorbellData_(dbData), semCmd_(semCmd), bufIndex_(bufIndex),
        dmaVTCM_(dmaVTCM) {
    encode();
  }

  QDmaRequestElement(const QDmaRequestElement &req, uint64_t ddrBase,
                     uint64_t mcIDBase) {
    bufIndex_ = req.bufIndex_;
    dmaVTCM_ = req.dmaVTCM_;
    buf_ = nullptr;

    if (req.buf_) {
      // req owns the data
      uint32_t size = DmaRequestElementSize_ / sizeof(uint32_t);
      buf_ = std::unique_ptr<uint32_t[]>(new uint32_t[size]);
      memset(buf_.get(), 0, DmaRequestElementSize_);
      memcpy(buf_.get(), req.buf_.get(), DmaRequestElementSize_);
      data_ = buf_.get();
    } else if (req.data_) {
      // req only has a pointer to the data
      data_ = req.data_;
    }
    if (data_) {
      decode();
    }
    updateElement(ddrBase, mcIDBase);
  }

  void encodeMem(qaic_attach_slice_entry *slice_ent) const {

    slice_ent->dev_addr = 0;
    if (dmaCmd_.transferDirection_ != DmaNoTransfer) {
      slice_ent->dev_addr = dmaCmd_.transferDirection_ == DmaInbound ? dmaDstAddr_ : dmaSrcAddr_;
    }

    slice_ent->db_len = 0;
    slice_ent->db_data = 0;
    slice_ent->db_addr = 0;
    if (nspDoorbellWrite_ != 0) {
      slice_ent->db_len = nspDoorbellSizeBytes_ * 8;
      slice_ent->db_data = nspDoorbellData_;
      slice_ent->db_addr = nspDoorbellAddr_;
    }

    qaic_sem *sem = &slice_ent->sem0;
    for (int i = 0; i < 4; i++) {
      sem[i].val = semCmd_[i].semValue_;
      sem[i].index = semCmd_[i].semID_;
      sem[i].presync = semCmd_[i].semSync_;
      sem[i].cmd = semCmd_[i].semCmdID_;
      sem[i].flags = (semCmd_[i].semDmaIBSync_ ? SEM_INSYNCFENCE : 0) |
                     (semCmd_[i].semDmaOBSync_ ? SEM_OUTSYNCFENCE : 0);
    }
  }

  /// Update this element with base addresses for DDR and MCID
  void updateElement(uint64_t ddrBase, uint64_t mcIDBase) {
    if (buf_ == nullptr) {
      return;
    }
    if (dmaCmd_.transferDirection_ == DmaInbound) {
      dmaDstAddr_ += dmaVTCM_ ? mcIDBase : ddrBase;
    } else if (dmaCmd_.transferDirection_ == DmaOutbound) {
      dmaSrcAddr_ += dmaVTCM_ ? mcIDBase : ddrBase;
    }
    if (mcIDBase != 0 && nspDoorbellWrite_ != 0) {
      nspDoorbellAddr_ += mcIDBase;
    }
    encode();
  }

  /// return the host side address, depending if it's an inbound or outbound
  /// transfer
  uint64_t getDmaHostAddr() {
    if (dmaCmd_.transferDirection_ == DmaInbound) {
      return dmaSrcAddr_;
    } else if (dmaCmd_.transferDirection_ == DmaOutbound) {
      return dmaDstAddr_;
    }
    return 0;
  }

  void updateDmaHostAddr(uint64_t dmaMappedHostAddr) {
    if (dmaCmd_.transferDirection_ == DmaInbound) {
      dmaSrcAddr_ = dmaMappedHostAddr;
      data_[2] = dmaSrcAddr_ & 0xffffffff;
      data_[3] = dmaSrcAddr_ >> 32;
    } else if (dmaCmd_.transferDirection_ == DmaOutbound) {
      dmaDstAddr_ = dmaMappedHostAddr;
      data_[4] = dmaDstAddr_ & 0xffffffff;
      data_[5] = dmaDstAddr_ >> 32;
    }
  }

  uint16_t getReqID() const { return reqID_; }
  int32_t getBufIndex() const { return bufIndex_; }
  uint64_t getDmaDst() const { return dmaDstAddr_; }
  uint64_t getDmaSrc() const { return dmaSrcAddr_; }
  uint64_t getDBAddr() const { return nspDoorbellAddr_; }
  uint32_t getDmaSize() const { return dmaLen_; }

  /// Only copy over the element and update the request ID
  void encodeElement(uint8_t *elem, QReqID reqID, bool copy = true) const {
    if (copy) {
      memcpy(elem, (uint8_t *)data_, DmaRequestElementSize_);
    }
    *(uint16_t *)elem = reqID;
  }

  /// Encode \p elem with data in this element and updating request ID with
  /// \p reqID and src/dst address with info in \p qbuf. Copy the data from
  /// this element to \p elem if \p copy is true. Otherwise only request ID
  /// and src or dst address is updated.
  /// \returns false if the supplied buffer is invalid
  bool encodeElement(uint8_t *elem, QReqID reqID, const QBuffer &qbuf,
                     bool &needResponse, bool copy = true) const {
    encodeElement(elem, reqID, copy);
    needResponse = dmaCmd_.genResponseCompletion_;
    if (dmaCmd_.transferDirection_ == DmaNoTransfer) {
      return true;
    }
    uint32_t *data = (uint32_t *)elem;
    // The buffer size must be no smaller than specified in metadata
    if (!qbuf.buf || qbuf.size < data[6]) {
      return false;
    }
    uint64_t addr = (uint64_t)qbuf.buf;
    if (dmaCmd_.transferDirection_ == DmaInbound) {
      data[2] = addr & 0xffffffff;
      data[3] = addr >> 32;
    } else if (dmaCmd_.transferDirection_ == DmaOutbound) {
      data[4] = addr & 0xffffffff;
      data[5] = addr >> 32;
    }

    return true;
  }

  bool isResponseRequired() const { return dmaCmd_.genResponseCompletion_; }
  DmaTransferDirection getDmaDirection() const {
    return dmaCmd_.transferDirection_;
  }

  bool isValid() const {
    if (!dmaCmd_.isValid()) {
      return false;
    }
    for (auto &sem : semCmd_) {
      if (!sem.isValid()) {
        return false;
      }
    }
    return true;
  }
  const uint32_t *getData() { return data_; }

  std::string str() {
    std::stringstream ss;
    ss << "req:" << (uint32_t)reqID_ << " seq:" << (uint32_t)seqID_ << " DMA("
       << dmaCmd_.str() << ") src:0x" << qutil::hex(dmaSrcAddr_) << " dst:0x"
       << qutil::hex(dmaDstAddr_) << " len:" << dmaLen_ << " doorbell(addr:0x"
       << qutil::hex(nspDoorbellAddr_)
       << " size(bytes):" << nspDoorbellSizeBytes_
       << " write:" << (uint32_t)nspDoorbellWrite_ << " data:0x"
       << qutil::hex(nspDoorbellData_) << ")";
    for (int i = 0; i < 4; i++) {
      ss << " Sem[" << i << "](" << semCmd_[i].str() << ")";
    }
    ss << " bufIndex:" << bufIndex_ << " dmaVTCM:" << (dmaVTCM_ ? "y" : "n");
    return ss.str();
  }

private:
  void decode() {
    reqID_ = data_[0] & 0xffff;
    seqID_ = (uint8_t)((data_[0] & 0xff << 16) >> 16);
    dmaCmd_.decode((uint8_t)((data_[0] & (0xff << 24)) >> 24));
    dmaSrcAddr_ = ((uint64_t)(data_[3])) << 32 | data_[2];
    dmaDstAddr_ = ((uint64_t)(data_[5])) << 32 | data_[4];
    dmaLen_ = data_[6];
    nspDoorbellAddr_ = ((uint64_t)(data_[9]) << 32) | data_[8];
    nspDoorbellWrite_ = (uint8_t)(data_[10] & 0x80) >> 7;
    nspDoorbellLen_ = (uint8_t)data_[10] & 0x3;

    if (nspDoorbellWrite_ != 0) {
      switch (nspDoorbellLen_) {
      case 0:
        nspDoorbellSizeBytes_ = 4;
        break;
      case 1:
        nspDoorbellSizeBytes_ = 2;
        break;
      case 2:
        nspDoorbellSizeBytes_ = 1;
        break;
      default:
        nspDoorbellSizeBytes_ = 0;
      }
    } else {
      nspDoorbellSizeBytes_ = 0;
    }
    nspDoorbellData_ = data_[11];

    for (uint i = 0; i < semCmd_.size(); i++)
      semCmd_[i].decode(data_[i + 12]);
  }

  void encode() {
    uint8_t dma_cmd;
    data_[0] = (data_[0] & 0xffff0000) | (reqID_ & 0xffff);
    data_[0] = (data_[0] & 0xff00ffff) | (seqID_ << 16);
    dmaCmd_.encode(&dma_cmd);
    data_[0] = (data_[0] & 0x00ffffff) | ((dma_cmd & 0xff) << 24);
    data_[1] = 0;
    data_[2] = dmaSrcAddr_ & 0xffffffff;
    data_[3] = dmaSrcAddr_ >> 32;
    data_[4] = dmaDstAddr_ & 0xffffffff;
    data_[5] = dmaDstAddr_ >> 32;
    data_[6] = dmaLen_;
    data_[7] = 0;
    data_[8] = nspDoorbellAddr_ & 0xffffffff;
    data_[9] = nspDoorbellAddr_ >> 32;
    data_[10] = ((nspDoorbellWrite_ & 0x1) << 7) | (nspDoorbellLen_ & 0x3);
    data_[11] = nspDoorbellData_;

    for (uint i = 0; i < semCmd_.size(); i++) {
      semCmd_[i].encode(&data_[i + 12]);
    }
  }

  static const uint32_t DmaRequestElementSize_ = 64;

  std::unique_ptr<uint32_t[]> buf_;
  uint32_t *data_;

  QReqID reqID_;
  uint8_t seqID_;
  DmaCmd dmaCmd_;
  uint64_t dmaSrcAddr_;
  uint64_t dmaDstAddr_;
  uint32_t dmaLen_;
  uint64_t nspDoorbellAddr_;
  uint8_t nspDoorbellWrite_;
  uint8_t nspDoorbellLen_;
  uint32_t nspDoorbellSizeBytes_;
  uint32_t nspDoorbellData_;
  std::array<SemCmd, 4> semCmd_;

  /// The user data buffer this element is assigned to.
  /// IDX_NO_DMA means not assigned or there is no DMA for this element.
  int32_t bufIndex_;
  /// True if DMA src/dst is in VTCM, false in DDR
  bool dmaVTCM_;
};

inline std::ostream &operator<<(std::ostream &strm, QDmaRequestElement &desc) {
  strm << desc.str();
  return strm;
}

/// DMA Response Element
class QDmaResponseElement {
public:
  static uint32_t getSize() { return DmaResponseElementSize_; }

  QDmaResponseElement(uint8_t *data) : data_((uint32_t *)data) { decode(); }

  QDmaResponseElement(uint8_t *data, uint16_t redID, uint16_t complCode)
      : reqID_(redID), completionCode_(complCode), data_((uint32_t *)data) {
    encode();
  }

  uint16_t getReqID() const { return reqID_; }

private:
  void encode() { data_[0] = reqID_ | completionCode_ << 16; }

  void decode() {
    reqID_ = data_[0] & 0xffff;
    completionCode_ = ((data_[0] & 0xffff << 16) >> 16);
  }

  static const uint32_t DmaResponseElementSize_ = 4;
  uint16_t reqID_;
  uint16_t completionCode_;

  uint32_t *data_;
};

} // namespace qaic

#endif // QDMA_ELEMENT_H
