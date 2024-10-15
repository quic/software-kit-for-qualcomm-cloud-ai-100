// Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QProgramContainer.h"
#include "QLogger.h"
#include "QAic.h"
#include "QOsal.h"
#include "metadataflatbufDecode.hpp"

namespace qaic {

using commonImageMap = std::unordered_map<QID, shQDeviceImageCommon>;

shQDeviceImageCommon
QDeviceImageCommon::Factory(QID device, QProgramContainer *programContainer,
                            QDeviceImageType imageSelect) {
  shQDeviceImageCommon obj = std::shared_ptr<QDeviceImageCommon>(new (
      std::nothrow) QDeviceImageCommon(device, programContainer, imageSelect));
  if (obj) {
    if (obj->load(programContainer) != QS_SUCCESS) {
      return nullptr;
    }
  }
  return obj;
}

QNNConstantsInterface *QDeviceImageCommon::getConstants() {
  if (const_ != nullptr) {
    return const_;
  } else {
    return constDesc_;
  }
}

//-------------------------------------------------------------------
// QDeviceImageCommon Private Methods
//--------------------------------------------------------------------
QDeviceImageCommon::QDeviceImageCommon(QID device,
                                       QProgramContainer *programContainer
                                       __attribute__((unused)),
                                       QDeviceImageType imageSelect)
    : device_(device), imageSelect_(imageSelect), rt_(nullptr),
      constDesc_(nullptr), const_(nullptr) {
  rt_ = getRuntime(this);
}

QDeviceImageCommon::~QDeviceImageCommon() { unload(); }

QStatus QDeviceImageCommon::load(QProgramContainer *programContainer) {

  QStatus status;
  if (programContainer == nullptr) {
    return QS_INVAL;
  }

  if (imageSelect_ & ConstantsImage) {
    if (programContainer->contains(QProgramContainer::ConstantsDescriptor) ==
        false) {
      LogErrorG("QPC does not have constants descriptor");
      return QS_INVAL;
    }
    QBuffer constantsDescriptorBuffer;
    status = programContainer->getBuffer(QProgramContainer::ConstantsDescriptor,
                                         constantsDescriptorBuffer);
    if (status != QS_SUCCESS) {
      LogErrorG("Constants descriptor buffer retrieval failed");
      return QS_INVAL;
    }
    AICConstantDescriptor *constDesc =
        (AICConstantDescriptor *)(constantsDescriptorBuffer.buf);

    // both static and dynamic constants are of size 0, we need not to load
    // descriptor
    if ((constDesc->staticConstantsSize == 0) &&
        (constDesc->dynamicConstantsSize == 0)) {
      return QS_SUCCESS;
    }

    QBuffer staticCompileTimeConstBuf;
    QBuffer dynamicCompileTimeConstBuf;
    programContainer->getConstantsBuffer(staticCompileTimeConstBuf,
                                         dynamicCompileTimeConstBuf);
    constDesc_ = rt_->loadConstantsEx(device_, constantsDescriptorBuffer,
                                      "ConstantsDescriptor", status);
    if (status != QS_SUCCESS) {
      constDesc_ = nullptr;
      goto exit_failure;
    }

    if (staticCompileTimeConstBuf.size > 0) {
      // static compile time constants are loaded at DDR address 0
      status = constDesc_->loadConstantsAtOffset(staticCompileTimeConstBuf, 0);
      if (status != QS_SUCCESS) {
        LogErrorG("Failed to load static compile time constants");
        goto exit_failure;
      }
    }

    if (dynamicCompileTimeConstBuf.size > 0) {
      // dynamic compile time constants are loaded at DDR offset
      // constDesc.staticConstantsSize
      status = constDesc_->loadConstantsAtOffset(
          dynamicCompileTimeConstBuf, constDesc->staticConstantsSize);
      if (status != QS_SUCCESS) {
        LogErrorG("Failed to load dynamic compile time constants");
        goto exit_failure;
      }
    }
  }

  return QS_SUCCESS;
exit_failure:
  (void)unload(); // We already have an error code
  return status;
}

QStatus QDeviceImageCommon::unload() {
  QStatus status = QS_SUCCESS;
  if (constDesc_ != nullptr) {
    if (constDesc_->unload() != QS_SUCCESS) {
      LogErrorG("Failed to unload constants Desc in exit_failure");
      status = QS_ERROR;
    }
    constDesc_ = nullptr;
  }
  if (const_ != nullptr) {
    if (const_->unload() != QS_SUCCESS) {
      LogErrorG("Failed to unload constants in exit_failure");
      status = QS_ERROR;
    }
    const_ = nullptr;
  }
  return status;
}

shQDeviceImage QDeviceImage::Factory(QID device,
                                     QProgramContainer *programContainer,
                                     shQDeviceImageCommon shCommonImage,
                                     QDeviceImageType imageSelect) {

  shQDeviceImage obj = std::shared_ptr<QDeviceImage>(
      new (std::nothrow)
          QDeviceImage(device, programContainer, shCommonImage, imageSelect));
  if (obj) {
    if (obj->load(programContainer) != QS_SUCCESS) {
      return nullptr;
    }
  }
  return obj;
}

QNNImageInterface *QDeviceImage::getNNImage() { return networkImage_; }

QNNConstantsInterface *QDeviceImage::getConstants() {
  if (commonImage_) {
    return commonImage_->getConstants();
  } else {
    return nullptr;
  }
}

//-------------------------------------------------------------------
// QDeviceImage Private Methods
//--------------------------------------------------------------------
QDeviceImage::QDeviceImage(QID device, QProgramContainer *programContainer
                           __attribute__((unused)),
                           shQDeviceImageCommon shCommonImage,
                           QDeviceImageType imageSelect)
    : device_(device), imageSelect_(imageSelect), rt_(nullptr),
      commonImage_(shCommonImage), networkImage_(nullptr) {
  rt_ = getRuntime(this);
}

QDeviceImage::~QDeviceImage() { unload(); }

QStatus QDeviceImage::load(QProgramContainer *programContainer) {
  auto const getMetadataCRC = [&](QProgramContainer *programContainer) {
    uint32_t metadataCRC = 0;
    if (programContainer->contains(QProgramContainer::MetadataCRC)) {
      QBuffer tempBuf;
      if (programContainer->getBuffer(QProgramContainer::MetadataCRC,
                                      tempBuf) == QS_SUCCESS) {
        metadataCRC = *(reinterpret_cast<uint32_t *>(tempBuf.buf));
      }
    }
    return metadataCRC;
  };

  QStatus status;
  if (programContainer == nullptr) {
    return QS_INVAL;
  }

  if ((imageSelect_ & ProgramImage) &&
      (programContainer->contains(QProgramContainer::Network))) {
    QBuffer qbuf;

    auto metadataCRC = getMetadataCRC(programContainer);

    status = programContainer->getBuffer(QProgramContainer::Network, qbuf);

    if (status == QS_SUCCESS) {
      networkImage_ =
          rt_->loadImage(device_, qbuf, "NetworkImage", status, metadataCRC);
      if (status != QS_SUCCESS) {
        networkImage_ = nullptr;
        goto exit_failure;
      }
    }
  }

  return QS_SUCCESS;
exit_failure:
  (void)unload(); // We already have an error code
  return status;
}

QStatus QDeviceImage::unload() {
  QStatus status = QS_SUCCESS;
  if (networkImage_ != nullptr) {
    if (networkImage_->unload() != QS_SUCCESS) {
      LogErrorG("Failed to unload network in exit_failure");
      status = QS_ERROR;
    }
  }
  return status;
}

const char *QProgramContainer::getQpcBuffer() {
  return reinterpret_cast<const char *>(qpcBuf_.get());
}

size_t QProgramContainer::getQpcSize() const { return qpcSize_; }

shQProgramContainer QProgramContainer::open(const uint8_t *qpcBuf,
                                            size_t qpcSize, QStatus &status) {
  shQProgramContainer shProgramContainer =
      std::make_shared<QProgramContainer>(qpcBuf, qpcSize);

  if (!shProgramContainer) {
    status = QS_NOMEM;
    return nullptr;
  }
  if (shProgramContainer->init() != QS_SUCCESS) {
    status = QS_ERROR;
    return nullptr;
  }
  status = QS_SUCCESS;
  return shProgramContainer;
}

QStatus QProgramContainer::close(shQProgramContainer &programContainer) {
  if (!programContainer) {
    LogErrorG("Invalid Object");
    return QS_INVAL;
  }
  return QS_SUCCESS;
}

QProgramContainer::QProgramContainer(const uint8_t *qpcBuf, size_t qpcSize)
    : qpcBufIn_(qpcBuf), qpcSize_(qpcSize) {}

QProgramContainer::~QProgramContainer() {}

bool QProgramContainer::contains(containerElements elemType) {
  switch (elemType) {
  case ConstantsDescriptor:
    return ((constDescBuf_.qb.buf != nullptr) && (constDescBuf_.qb.size != 0) &&
            (constDescBuf_.valid));
  case Constants:
    return (((staticCompileTimeConstBuf_.qb.buf != nullptr) &&
             (staticCompileTimeConstBuf_.qb.size != 0) &&
             (staticCompileTimeConstBuf_.valid)) ||
            ((dynamicCompileTimeConstBuf_.qb.buf != nullptr) &&
             (dynamicCompileTimeConstBuf_.qb.size != 0) &&
             (dynamicCompileTimeConstBuf_.valid)));
  case NetworkDescriptor:
    return ((networkDescBuf_.qb.buf != nullptr) &&
            (networkDescBuf_.qb.size != 0) && (networkDescBuf_.valid));
  case Network:
    return ((progBuf_.qb.buf != nullptr) && (progBuf_.qb.size != 0) &&
            (progBuf_.valid));
  case ConstantsCRC:
    return ((constantsCRCBuf_.qb.buf != nullptr) &&
            (constantsCRCBuf_.qb.size != 0) && (constantsCRCBuf_.valid));
  case MetadataCRC:
    return ((metadataCRCBuf_.qb.buf != nullptr) &&
            (metadataCRCBuf_.qb.size != 0) && (metadataCRCBuf_.valid));
  default:
    LogErrorG("Invalid container element type:{}", elemType);
    break;
  }
  return false;
}

QStatus QProgramContainer::getBuffer(containerElements elemType,
                                     QBuffer &qbuf) {
  QStatus status = QS_ERROR;
  switch (elemType) {
  case ConstantsDescriptor:
    qbuf = constDescBuf_.qb;
    status = QS_SUCCESS;
    break;
  case NetworkDescriptor:
    qbuf = networkDescBuf_.qb;
    status = QS_SUCCESS;
    break;
  case Network:
    qbuf = progBuf_.qb;
    status = QS_SUCCESS;
    break;
  case ConstantsCRC:
    qbuf = constantsCRCBuf_.qb;
    status = QS_SUCCESS;
    break;
  case MetadataCRC:
    qbuf = metadataCRCBuf_.qb;
    status = QS_SUCCESS;
    break;
  case MetaInfo:
    qbuf = metaInfoBuf_.qb;
    status = QS_SUCCESS;
    break;
  default:
    LogErrorG("Invalid container element type:{}", elemType);
    break;
  }
  return status;
}

void QProgramContainer::getConstantsBuffer(
    QBuffer &staticCompileTimeConstBuf, QBuffer &dynamicCompileTimeConstBuf) {
  qutil::initQBuffer(staticCompileTimeConstBuf);
  if (staticCompileTimeConstBuf_.valid) {
    staticCompileTimeConstBuf = staticCompileTimeConstBuf_.qb;
  }
  qutil::initQBuffer(dynamicCompileTimeConstBuf);
  if (dynamicCompileTimeConstBuf_.valid) {
    dynamicCompileTimeConstBuf = dynamicCompileTimeConstBuf_.qb;
  }
  return;
}

QStatus QProgramContainer::getBuffer(containerElements elemType, QData &qdata) {
  QStatus status = QS_ERROR;
  QBuffer qbuf;
  status = getBuffer(elemType, qbuf);
  if (status == QS_SUCCESS) {
    qdata.data = qbuf.buf;
    qdata.size = qbuf.size;
  }
  return status;
}

QStatus QProgramContainer::init() {
  int rc = 0;
  qpcBuf_ = std::unique_ptr<uint8_t[]>(new (std::nothrow) uint8_t[qpcSize_]());
  if (qpcBuf_ == nullptr) {
    return QS_NOMEM;
  }

  if (copyQpcBuffer(qpcBuf_.get(), const_cast<uint8_t *>(qpcBufIn_),
                    qpcSize_) == nullptr) {
    LogErrorG("Failed to copy qpc");
    return QS_ERROR;
  }

  if (getQPCSegment(qpcBuf_.get(), constantsSegmentName_,
                    &(staticCompileTimeConstBuf_.qb.buf),
                    &(staticCompileTimeConstBuf_.qb.size), 0) != true) {
    staticCompileTimeConstBuf_.qb.buf = nullptr;
    staticCompileTimeConstBuf_.qb.size = 0;
  } else {
    if (staticCompileTimeConstBuf_.qb.size != 0) {
      staticCompileTimeConstBuf_.valid = true;
    }
  }

  if (getQPCSegment(qpcBuf_.get(), constantsSegmentName_,
                    &(dynamicCompileTimeConstBuf_.qb.buf),
                    &(dynamicCompileTimeConstBuf_.qb.size), 1) != true) {
    dynamicCompileTimeConstBuf_.qb.buf = nullptr;
    dynamicCompileTimeConstBuf_.qb.size = 0;
  } else {
    if (dynamicCompileTimeConstBuf_.qb.size != 0) {
      dynamicCompileTimeConstBuf_.valid = true;
    }
  }

  // we should add another API in getQPCSegments to get both segments from
  // constants.bin to remove below logic
  // special case when offset is zero, there is no staticCompileTimeConstBuf_
  if (staticCompileTimeConstBuf_.valid && dynamicCompileTimeConstBuf_.valid) {
    if ((staticCompileTimeConstBuf_.qb.buf ==
         dynamicCompileTimeConstBuf_.qb.buf) &&
        (staticCompileTimeConstBuf_.qb.size ==
         dynamicCompileTimeConstBuf_.qb.size)) {
      staticCompileTimeConstBuf_.valid = false;
    }
  }

  if (getQPCSegment(qpcBuf_.get(), constantsDescSegmentName_,
                    &(constDescBuf_.qb.buf), &(constDescBuf_.qb.size),
                    0) != true) {
    LogErrorG("Failed to extract constants descriptor segment from program "
              "container");
    return QS_ERROR;
  } else {
    constDescBuf_.valid = true;
  }

  if (getQPCSegment(qpcBuf_.get(), networkDescSegmentName_,
                    &(networkDescBuf_.qb.buf), &(networkDescBuf_.qb.size),
                    0) != true) {
    LogErrorG("Failed to extract network desriptor segment from program "
              "container {}",
              rc);
    return QS_ERROR;
  } else {
    networkDescBuf_.valid = true;
  }

  if (getQPCSegment(qpcBuf_.get(), networkSegmentName_, &(progBuf_.qb.buf),
                    &(progBuf_.qb.size), 0) != true) {
    LogErrorG("Failed to extract network segment from program container {}",
              rc);
    return QS_ERROR;
  } else {
    progBuf_.valid = true;
  }

  if (getQPCSegment(qpcBuf_.get(), constantsCRCSegmentName_,
                    &(constantsCRCBuf_.qb.buf), &(constantsCRCBuf_.qb.size),
                    0) != true) {
    LogInfoG("No CRC segment for constants");
  } else {
    constantsCRCBuf_.valid = true;
  }

  if (getQPCSegment(qpcBuf_.get(), metadataCRCSegmentName_,
                    &(metadataCRCBuf_.qb.buf), &(metadataCRCBuf_.qb.size),
                    0) != true) {
    LogInfoG("No CRC segment for metadata");
  } else {
    metadataCRCBuf_.valid = true;
  }

  if (!networkDesc_.ParseFromArray(networkDescBuf_.qb.buf,
                                   networkDescBuf_.qb.size)) {
    LogErrorG("Failed to parse network descriptor");
    return QS_ERROR;
  }

  if (networkDesc_.major_version() != AIC_NETWORK_DESCRIPTION_MAJOR_VERSION) {
    LogWarnG("Incompatible Network Descriptor, library supports {}.{} "
              "program requires {}.{}",
              AIC_NETWORK_DESCRIPTION_MAJOR_VERSION,
              AIC_NETWORK_DESCRIPTION_MINOR_VERSION,
              networkDesc_.major_version(), networkDesc_.minor_version());
  }

  if (networkDesc_.minor_version() > AIC_NETWORK_DESCRIPTION_MINOR_VERSION) {
    LogWarnG("Network Descriptor support features that may not be supported by "
             "this library, program network desc version {}.{}, library "
             "supports {}.{}",
             networkDesc_.major_version(), networkDesc_.minor_version(),
             AIC_NETWORK_DESCRIPTION_MAJOR_VERSION,
             AIC_NETWORK_DESCRIPTION_MINOR_VERSION);
  }
  if (!bindingsParser_.init(networkDesc_)) {
    LogErrorG("Failed to initialize the bindingsParser");
    return QS_ERROR;
  }
  if (bindingsParser_.getIoDescriptorPb(ioDescPb_) != QS_SUCCESS ||
      ioDescPb_ == nullptr) {
    LogErrorG("Failed to get IODescriptor");
    return QS_ERROR;
  }
  try {
    ioDescPbBuffer_.resize(ioDescPb_->ByteSizeLong());
  } catch (const std::bad_alloc &ba) {
    LogErrorG("Failed to resize vector for Protocol Buffer {}", ba.what());
    return QS_ERROR;
  }

  if (!ioDescPb_->SerializeToArray(ioDescPbBuffer_.data(),
                                   ioDescPbBuffer_.size())) {
    LogErrorG("Failed to serialize Protocol Buffer for IO Descriptor");
    return QS_ERROR;
  }

  try {

    qpcInfo_ = std::make_shared<QpcInfo>();
    QpcProgramInfo progInfo;

    QAicIoBufferInfo *bufferInfo = nullptr, *bufferInfoDma = nullptr;
    bindingsParser_.getIoBufferInfo(bufferInfo);
    bindingsParser_.getIoBufferInfoDma(bufferInfoDma);
    if (bufferInfo == nullptr || bufferInfoDma == nullptr) {
      LogErrorG("Failed to get IO buffer info");
      return QS_ERROR;
    }

    progInfo.name = networkDesc_.network_name();
    progInfo.programIndex = 0;
    progInfo.batchSize = networkDesc_.batch_size();
    progInfo.numCores = networkDesc_.num_cores();

    BufferMappings &userBuffers = progInfo.bufferMappings;
    BufferMappings &dmaBuffers = progInfo.bufferMappingsDma;

    for (uint32_t j = 0; j < bufferInfo->numBufferMappings; j++) {
      userBuffers.emplace_back(
          bufferInfo->bufferMappings[j].bufferName,
          bufferInfo->bufferMappings[j].index,
          bufferInfo->bufferMappings[j].ioType,
          bufferInfo->bufferMappings[j].size,
          bufferInfo->bufferMappings[j].isPartialBufferAllowed,
          bufferInfo->bufferMappings[j].dataType);
    }
    for (uint32_t j = 0; j < bufferInfoDma->numBufferMappings; j++) {
      dmaBuffers.emplace_back(
          bufferInfoDma->bufferMappings[j].bufferName,
          bufferInfoDma->bufferMappings[j].index,
          bufferInfoDma->bufferMappings[j].ioType,
          bufferInfoDma->bufferMappings[j].size,
          bufferInfoDma->bufferMappings[j].isPartialBufferAllowed,
          bufferInfoDma->bufferMappings[j].dataType);
    }

    qpcInfo_->program.emplace_back(progInfo);

    if (staticCompileTimeConstBuf_.valid || dynamicCompileTimeConstBuf_.valid) {
      QpcConstantsInfo constInfo;
      constInfo.name = constantsSegmentName_;
      constInfo.constantsIndex = 0;
      constInfo.size = staticCompileTimeConstBuf_.qb.size +
                       dynamicCompileTimeConstBuf_.qb.size;
      qpcInfo_->constants.emplace_back(constInfo);
    }
    if (constDescBuf_.valid) {
      QpcConstantsInfo constInfo;
      constInfo.name = constantsDescSegmentName_;
      constInfo.constantsIndex = 1;
      constInfo.size = constDescBuf_.qb.size;
      qpcInfo_->constants.emplace_back(constInfo);
    }
  } catch (std::bad_alloc &ba) {
    LogErrorG("exception bad_alloc caught: {} ", ba.what());
    return QS_ERROR;
  }

  return QS_SUCCESS;
}

const shQpcInfo &QProgramContainer::getInfo() { return qpcInfo_; }

QStatus QProgramContainer::getDeviceImage(QID device, shQDeviceImage &image,
                                          QDeviceImageType imageSelect) {
  shQDeviceImageCommon commonImage = nullptr;
  if (getCommonDeviceImage(device, commonImage, imageSelect) == QS_SUCCESS) {
    std::unique_lock<std::mutex> lock(commonImageLoadedMapMutex_);
    // Now we have the common (constants) image, create an elf image with it.
    image = QDeviceImage::Factory(device, this, commonImage, imageSelect);
    if (!image) {
      LogErrorG("Failed to create Image");
      return QS_ERROR;
    }
    return QS_SUCCESS;
  }
  return QS_ERROR;
}

// if CommonDeviceImage is not present, it creates one and load constants
QStatus
QProgramContainer::getCommonDeviceImage(QID device,
                                        shQDeviceImageCommon &commonImage,
                                        QDeviceImageType imageSelect) {
  std::unique_lock<std::mutex> lock(commonImageLoadedMapMutex_);
  commonImageMap::const_iterator found = commonImageLoadedMap_.find(device);
  if (found == commonImageLoadedMap_.end()) {
    commonImage = QDeviceImageCommon::Factory(device, this, imageSelect);
    if (!commonImage) {
      LogErrorG("Failed to created common image object for constants");
      return QS_ERROR;
    }
    if (!commonImage->isLoaded()) {
      LogErrorG("Failed to load program");
      return QS_ERROR;
    } else {
      commonImageLoadedMap_.insert({device, commonImage});
    }
  } else {
    if (found->second->isLoaded()) {
      commonImage = found->second;
    }
  }
  return QS_SUCCESS;
}

QStatus QProgramContainer::getIoDescriptor(QData *ioDescData) {
  if (ioDescData == nullptr) {
    LogErrorG("Invalid pointer in getIoDescriptor");
    return QS_INVAL;
  }
  // IO Descriptor will be parsed on init
  if ((ioDescPb_ == nullptr) || (ioDescPbBuffer_.size() == 0)) {
    LogErrorG("Invalid state for Qpc, descriptor not parsed: "
              "ioDescPb_:{:p} ioDescPbBuffer:{}",
              (void *)ioDescPb_, ioDescPbBuffer_.size());
    return QS_INVAL;
  }
  ioDescData->size = ioDescPbBuffer_.size();
  ioDescData->data = ioDescPbBuffer_.data();
  return QS_SUCCESS;
}

QStatus QProgramContainer::getBufferByName(const std::string &name,
                                           QData &qdata) {
  if (getQPCSegment(qpcBuf_.get(), name.c_str(), &qdata.data, &qdata.size, 0) ==
      false) {
    return QS_INVAL;
  }
  return QS_SUCCESS;
}

} // namespace qaic
