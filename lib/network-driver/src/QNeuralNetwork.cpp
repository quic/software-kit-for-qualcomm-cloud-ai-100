// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QNeuralNetwork.h"
#include "QDmaElement.h"
#include "QDeviceInterface.h"
#include "QVirtualChannelInterface.h"
#include "QLogger.h"
#include "QMetaData.h"
#include "QNNConstants.h"
#include "QNNImage.h"
#include "QUtil.h"
#include "QDevAic100Interface.h"
#include "QKmdDevice.h"
#include "QOsal.h"
#include <sys/mman.h>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>

#define INVALID_DBC_ID -1U

namespace qaic {

constexpr uint16_t deactivationRetryCount = 5;
//
// QInfHandle
//
QInfHandle::QInfHandle(uint32_t numReqs, uint64_t waitHandle,
                       shQDevInterface devInterface,
                       std::unique_ptr<uint8_t[]> memReq,
                       std::unique_ptr<uint8_t[]> execute,
                       // Number of KMD Buffs is not equal to dma requests
                       std::vector<QBuffer> &kmdQBufs,
                       std::vector<QDirection> &bufDirs, bool hasPartialTensor)
    : numReqs_(numReqs), waitHandle_(waitHandle), devInterface_(devInterface),
      memReq_(std::move(memReq)), execute_(std::move(execute)),
      kmdQBufs_(std::move(kmdQBufs)), bufDirs_(std::move(bufDirs)),
      hasPartialTensor_(hasPartialTensor), numBufs_(kmdQBufs_.size()){};

QInfHandle::~QInfHandle() {
  for (auto &kmdqbuf : kmdQBufs_) {
    if ((kmdqbuf.buf != nullptr) &&
        (kmdqbuf.type == QBufferType::QBUFFER_TYPE_HEAP)) {
      QOsal::qMunmap(kmdqbuf.buf, kmdqbuf.size);
    }
  }
  if (devInterface_) {
    // Note: Specifically not checking error code here
    devInterface_->freeMemReq(memReq_.get(), numReqs_);
  }
}

shQInfHandle QInfHandle::Factory(
    uint32_t numReqs, uint64_t waitHandle, shQDevInterface &devInterface,
    std::unique_ptr<uint8_t[]> memReq, std::unique_ptr<uint8_t[]> execute,
    // Number of KMD Buffs is not equal to dma requests
    std::vector<QBuffer> &kmdQBufs, std::vector<QDirection> &bufDirs,
    bool hasPartialTensor) {
  shQInfHandle obj = shQInfHandle(new (std::nothrow) QInfHandle(
      numReqs, waitHandle, devInterface, std::move(memReq), std::move(execute),
      kmdQBufs, bufDirs, hasPartialTensor));
  if (!obj) {
    return nullptr;
  }
  if (!obj->init()) {
    return nullptr;
  }
  return obj;
}

bool QInfHandle::init() { return (static_cast<bool>(devInterface_)); }

//
// QNeuralnetwork
//
QNeuralnetwork *QNeuralnetwork::
    Factory(QDeviceInterface *device, QNNImageInterface *image,
            QNNConstantsInterface *constants,
            std::unique_ptr<QMetaDataInterface> meta,
            QVirtualChannelInterface *vc, QNAID naID, uint32_t waitTimeoutMs,
            uint32_t numMaxWaitRetries) {
  QNeuralnetwork *obj = new QNeuralnetwork(
      device, image, constants, std::move(meta), vc, naID, waitTimeoutMs,
      numMaxWaitRetries);
  if (obj == nullptr) {
    return nullptr;
  }
  if (!obj->init()) {
    delete obj;
    return nullptr;
  }
  return obj;
}

//  Constructor
QNeuralnetwork::
    QNeuralnetwork(QDeviceInterface *device, QNNImageInterface *image,
                   QNNConstantsInterface *constants,
                   std::unique_ptr<QMetaDataInterface> meta,
                   QVirtualChannelInterface *vc, QNAID naID,
                   uint32_t waitTimeoutMs, uint32_t numMaxWaitRetries)
    : QLogger("QNeuralnetwork"), dev_(device), image_(image),
      constants_(constants), metadata_(std::move(meta)), vc_(vc), naID_(naID),
      elemCount_(
          static_cast<QMetaData *>(metadata_.get())->getReqElementsCount()),
      bufCount_(static_cast<QMetaData *>(metadata_.get())->getBufCount()),
      waitTimeoutMs_(waitTimeoutMs), numMaxWaitRetries_(numMaxWaitRetries),
      submitRetryMax_(50), submitRetryTimeoutUs_(5000), dbcFifoSize_(0),
      dbcQueuedSize_(0), infCount_{0} {};


bool QNeuralnetwork::init() {
  if (dev_ == nullptr) {
    LogError("Failed to initialize PrdNeuralNetwork, invalid device");
    return false;
  }
  devInterface_ = dev_->getDeviceInterface();
  if (!devInterface_) {
    LogError("Failed to initialize PrdNeuralNetwork, invalid device interface");
    return false;
  }
  return true;
}


void QNeuralnetwork::freeInfBuffers(uint8_t *boReqPtr,
                                                    uint32_t reqProcessed,
                                                    std::vector<QBuffer>
                                                        &kmdQBufs) const {
  for (auto &kmdqbuf : kmdQBufs) {
    if ((kmdqbuf.buf != nullptr) &&
        (kmdqbuf.type == QBufferType::QBUFFER_TYPE_HEAP)) {
      QOsal::qMunmap(kmdqbuf.buf, kmdqbuf.size);
      kmdqbuf.buf = nullptr;
    }
  }
  if (devInterface_) {
    devInterface_->freeMemReq(boReqPtr, reqProcessed);
  }
}


std::shared_ptr<QInfHandle> QNeuralnetwork::createInfHandle(std::unique_ptr<uint8_t[]> boReqOwner,
                                     std::unique_ptr<uint8_t[]> executeOwner,
                                     uint32_t &reqProcessed, int waitIndex,
                                     std::vector<QBuffer> &kmdQBufs,
                                     std::vector<QDirection> &dirs,
                                     bool hasPartialTensor) const {
  QStatus status = QS_ERROR;
  int32_t bufIndex = 0, bufIndexNext = -1;
  uint32_t offset = 0, size = 0,    // Size and offsets in to buffers
      offsetNext = 0, sizeNext = 0; // Size and offsets in to buffers
  QMetaData *meta = static_cast<QMetaData *>(metadata_.get());
  const QElemToBufVec &elemToBufVec = meta->getElemToBufVec();
  QBufferType type;
  uint64_t waitHandle = 0;

  auto bufDirs = std::vector<QDirection>(bufCount_);
  uint8_t *boReqPtr = boReqOwner.get();
  uint8_t *executePtr = executeOwner.get();

  auto createBO = reinterpret_cast<qaic_create_bo *>(boReqPtr);
  auto attachSlice = reinterpret_cast<qaic_attach_slice *>(boReqPtr + sizeof(qaic_create_bo));
  auto attachSliceEntry = reinterpret_cast<qaic_attach_slice_entry *>(boReqPtr + sizeof(qaic_create_bo) + sizeof(qaic_attach_slice));
  auto execute = reinterpret_cast<qaic_execute *>(executePtr);
  auto executes = reinterpret_cast<qaic_execute_entry *>(executePtr + sizeof(qaic_execute));
  auto partialExecutes = reinterpret_cast<qaic_partial_execute_entry *>(executePtr + sizeof(qaic_execute));

  int attachSliceEntryProcessed = 0;

  // keep dbc_id as invalid to have proper cleanup in case of error
  attachSlice->hdr.dbc_id = INVALID_DBC_ID;

  for (uint32_t i = 0; i < elemCount_; i++) {
    bool isLast = (i >= elemCount_ - 1);
    if (!isLast) {
      std::tie(bufIndexNext, offsetNext, sizeNext) = elemToBufVec[i + 1];
    }
    std::tie(bufIndex, offset, size) = elemToBufVec[i];

    if (bufIndex > (int)bufCount_) {
      LogError("Out of range buffer Index {} bufIndex, max expected {}",
               bufIndex, bufCount_);
      freeInfBuffers(boReqPtr, reqProcessed, kmdQBufs);
      return nullptr;
    }

    if (hasPartialTensor && !isLast && bufIndex == bufIndexNext) {
      LogError("Partial tensor does not support shared DMA buffers");
      freeInfBuffers(boReqPtr, reqProcessed, kmdQBufs);
      return nullptr;
    }

    if (size != 0) { // Support DMA Requests with zero size
      type = kmdQBufs[bufIndex].type;
    } else {
      type = QBufferType::QBUFFER_TYPE_HEAP;
    }
    attachSliceEntry[attachSliceEntryProcessed].size = size;

    switch (type) {
    case QBufferType::QBUFFER_TYPE_HEAP:
      attachSliceEntry[attachSliceEntryProcessed].offset = offset;
      break;
    default:
      LogError("QBuffer type is not supported");
      freeInfBuffers(boReqPtr, reqProcessed, kmdQBufs);
      return nullptr;
    }

    attachSliceEntryProcessed++;

    // If this element does not extend to the next i/o buffer, or is the
    // last element, or the element is zero-sized
    if ((bufIndex != bufIndexNext) || isLast || size == 0) {
      int start = i + 1 - attachSliceEntryProcessed;
      // Pick up mem_req fields from metadata
      status =
          meta->updateElementsFromMeta(attachSliceEntry, start, attachSliceEntryProcessed);
      if (status != QS_SUCCESS) {
        freeInfBuffers(boReqPtr, reqProcessed, kmdQBufs);
        return nullptr;
      }
      createBO->size = size ? kmdQBufs[bufIndex].size : size;
      attachSlice->hdr.count = attachSliceEntryProcessed;
      attachSlice->hdr.dbc_id = vc_->getVC();
      attachSlice->hdr.handle = 0;
      attachSlice->hdr.dir = static_cast<uint32_t>(dirs[i]);
      attachSlice->hdr.size = size ? kmdQBufs[bufIndex].size : size;
      attachSlice->data = (uint64_t)attachSliceEntry;

      QBuffer zero_buf{0, NULL, 0, 0, QBufferType::QBUFFER_TYPE_HEAP};

      status =
          prepareInfHandleBuf(createBO, size ? kmdQBufs[bufIndex] : zero_buf);
      if (status != QS_SUCCESS) {
        LogError("Dev {} VC {} failed to prepare buffer",
                 (uint32_t)dev_->getID(), (uint32_t)vc_->getVC());
        freeInfBuffers(boReqPtr, reqProcessed, kmdQBufs);
        return nullptr;
      }
      if (hasPartialTensor) {
        partialExecutes[reqProcessed].handle = createBO->handle;
        partialExecutes[reqProcessed].dir = attachSlice->hdr.dir;
        partialExecutes[reqProcessed].resize = 0;
      } else {
        executes[reqProcessed].handle = createBO->handle;
        executes[reqProcessed].dir = attachSlice->hdr.dir;
      }
      if ((int)i == waitIndex) {
        waitHandle = createBO->handle;
      }
      bufDirs[bufIndex] =
          size ? static_cast<QDirection>(attachSlice->hdr.dir) : QDirection::QDIR_NONE;
      reqProcessed++;

      if (!isLast) {
        createBO = reinterpret_cast<qaic_create_bo *>((uint8_t *)createBO + sizeof(*createBO) + sizeof(*attachSlice) + sizeof(*attachSliceEntry) * attachSliceEntryProcessed);
        attachSlice = reinterpret_cast<qaic_attach_slice *>(createBO + 1);
        attachSliceEntry = reinterpret_cast<qaic_attach_slice_entry *>(attachSlice + 1);
        attachSliceEntryProcessed = 0;
        attachSlice->hdr.dbc_id = INVALID_DBC_ID;
      }
    }
  }

  execute->hdr.count = reqProcessed;
  execute->hdr.dbc_id = vc_->getVC();
  execute->data = (uint64_t)executes;

  return QInfHandle::Factory(reqProcessed, waitHandle,
                             dev_->getDeviceInterface(), std::move(boReqOwner),
                             std::move(executeOwner), kmdQBufs, bufDirs,
                             hasPartialTensor);
}

//
// Set up the QInfHandle for one inference
// Buffers provided here do not need to be populated with user pointers, we just
// need the buffer sizes
// Buffer Direction is used to verify the direction of the buffer against
// metadata
//

std::shared_ptr<QInfHandle>
QNeuralnetwork::getInfHandle(const QBuffer *bufs,
                                             uint32_t numBuf,
                                             const QDirection *bufDir
                                             __attribute__((unused)),
                                             bool hasPartialTensor) const {
  QStatus status = QS_ERROR;
  QMetaData *meta = static_cast<QMetaData *>(metadata_.get());
  const QElemToBufVec &elemToBufVec = meta->getElemToBufVec();
  int waitIndex = -1;
  QStatus ret = QS_ERROR;
  std::vector<QDirection> dirs;

  if (numBuf != bufCount_) {
    LogError("Invalid number of buffers, {}, expected {}", numBuf, bufCount_);
    return nullptr;
  }

  dirs.reserve(elemCount_);

  status = meta->getDmaDirections(dirs, elemCount_);
  if (status != QS_SUCCESS) {
    return nullptr;
  }

  // The number of mem_req should be the number of buffers plus the number of
  // zero-sized requests
  uint32_t numReqs = numBuf;

  // find the last output to wait on, and zero-sized requests
  for (uint32_t i = 0; i < elemCount_; i++) {
    if (dirs[i] == QDirection::QDIR_FROM_DEV) {
      waitIndex = i;
    }

    if (std::get<2>(elemToBufVec[i]) == 0) {
      numReqs++;
    }
  }
  if (waitIndex == -1) {
    waitIndex = elemCount_ - 1;
  }

  // List of QBuffers which points to the buffers allocated by KMD
  // These are ordered as user provides buffers
  auto kmdQBufs = std::vector<QBuffer>(numBuf);
  for (uint32_t i = 0; i < numBuf; i++) {
    kmdQBufs[i].size = bufs[i].size;
    kmdQBufs[i].type = bufs[i].type;
    if (bufs[i].type != QBufferType::QBUFFER_TYPE_HEAP) {
      kmdQBufs[i].buf = bufs[i].buf;
      kmdQBufs[i].handle = bufs[i].handle;
      kmdQBufs[i].offset = bufs[i].offset;
    } else {
      kmdQBufs[i].buf = nullptr;
    }
  }

  uint32_t boReqPtrSize = QOsal::getMemReqSize(
      (sizeof(qaic_create_bo) + sizeof(qaic_attach_slice)) * numReqs + sizeof(qaic_attach_slice_entry) * elemCount_);
  auto boReqOwner =
      std::unique_ptr<uint8_t[]>(new (std::nothrow) uint8_t[boReqPtrSize]);

  if (!boReqOwner) {
    LogWarn("Dev {} VC {} no heap memory for mem_req");
    return nullptr;
  }
  uint8_t *boReqPtr = boReqOwner.get();
  memset(boReqPtr, 0, boReqPtrSize);

  uint32_t execUnitSize =
      (hasPartialTensor ? sizeof(qaic_partial_execute_entry)
                        : sizeof(qaic_execute_entry));
  uint32_t executePtrSize = sizeof(qaic_execute) + execUnitSize * numReqs;
  auto executeOwner =
      std::unique_ptr<uint8_t[]>(new (std::nothrow) uint8_t[executePtrSize]);
  if (!executeOwner) {
    LogWarn("Dev {} VC {} no heap memory for execute");
    return nullptr;
  }
  uint8_t *executePtr = executeOwner.get();
  memset(executePtr, 0, executePtrSize);

  uint32_t reqProcessed = 0;
  std::shared_ptr<QInfHandle> infHandle = createInfHandle(
      std::move(boReqOwner), std::move(executeOwner), reqProcessed, waitIndex,
      kmdQBufs, dirs, hasPartialTensor);

  if (!infHandle) {
    freeInfBuffers(boReqPtr, reqProcessed, kmdQBufs);
    return nullptr;
  }

  ret = QOsal::getQPciInfo((uint32_t)dev_->getID(), (QPciInfo *)&qPciInfo_);
  switch (ret) {
  case QS_SUCCESS:
    QOsal::getDbcFifoSize((uint32_t *)&dbcFifoSize_, (QPciInfo *)&qPciInfo_,
                          (uint32_t)vc_->getVC());
    break;
  case QS_UNSUPPORTED:
    LogDebug("getQPciInfo is not supported");
    break;
  case QS_ERROR:
  default:
    LogError("Error in getPciInfo");
    // destructor will free inference buffers for a valid infHandle
    return nullptr;
  }
  return infHandle;
}

//
// load data for IN buffers for a batch
//

QStatus
QNeuralnetwork::loadData(const QInfHandle *infHandle,
                                         const QBuffer
                                             *bufs,
                                         uint32_t numBuf) const {
  if (infHandle == nullptr) {
    return QS_INVAL;
  }

  if (numBuf != infHandle->numBufs_) {
    LogWarn("Dev {} VC {} expect {} buffers got {}", (uint32_t)dev_->getID(),
            (uint32_t)vc_->getVC(), infHandle->numBufs_, numBuf);
    return QS_INVAL;
  }

  for (uint i = 0; i < numBuf; i++) {
    if (infHandle->bufDirs_[i] != QDirection::QDIR_TO_DEV) {
      continue;
    }

    if ((bufs[i].type != QBufferType::QBUFFER_TYPE_HEAP) ||
        infHandle->kmdQBufs_[i].size != bufs[i].size) {
      LogError("Invalid Buffer Size at index {} Size:{}, expected:{}", i,
               bufs[i].size, infHandle->kmdQBufs_[i].size);
      return QS_INVAL;
    }
    if ((bufs[i].size > 0) && (bufs[i].buf != nullptr)) {
      memcpy(infHandle->kmdQBufs_[i].buf, bufs[i].buf, bufs[i].size);
    } else {
      LogError("Invalid Buffer");
      return QS_INVAL;
    }
  }
  return QS_SUCCESS;
}


QStatus QNeuralnetwork::prepareData(const QInfHandle *infHandle,
                                                    const std::vector<uint64_t>
                                                        &dmaBufferSizes) const {
  if (!infHandle) {
    return QS_INVAL;
  }
  if (!infHandle->hasPartialTensor_) {
    return QS_SUCCESS;
  }
  qaic_execute *execute = reinterpret_cast<qaic_execute *>(infHandle->execute_.get());

  assert(dmaBufferSizes.size() == execute->hdr.count);

  QMetaData *meta = static_cast<QMetaData *>(metadata_.get());
  const QElemToBufVec &elemToBufVec = meta->getElemToBufVec();

  auto partialExecutes = reinterpret_cast<qaic_partial_execute_entry *>(execute->data);
  for (uint32_t i = 0; i < execute->hdr.count; i++, partialExecutes++) {
    // update resize field if direction is to device and updated size is not
    // zero
    auto bufIdx = std::get<0>(elemToBufVec[i]);
    assert(bufIdx < dmaBufferSizes.size());
    if (partialExecutes->dir == 1 && dmaBufferSizes[bufIdx] != 0) {
      partialExecutes->resize = dmaBufferSizes[bufIdx];
    }
  }

  return QS_SUCCESS;
}

//
// Enqueues request for execution for a batch
//

QStatus
QNeuralnetwork::enqueueData(const QInfHandle *infHandle) {
  QStatus ret;

  if (infHandle == nullptr) {
    return QS_INVAL;
  }
  ret = runExecute(
      infHandle, reinterpret_cast<const qaic_execute *>(infHandle->execute_.get()),
      infHandle->hasPartialTensor_);
  if (ret != QS_SUCCESS)
    return ret;
  return QS_SUCCESS;
}

//
// wait for given handle (should be last out buffer of a batch)
//

QStatus QNeuralnetwork::wait(const QInfHandle *infHandle) {
  QStatus status = QS_SUCCESS;
  qaic_wait wait = {};
  uint32_t retries = 0;

  if (infHandle == nullptr) {
    return QS_INVAL;
  } else if (infHandle->waitHandle_ <= 0) {
    LogError("Dev {} VC {} invalid wait handle ", (uint32_t)dev_->getID(),
             (uint32_t)vc_->getVC());
    return QS_INVAL;
  }

  wait.handle = infHandle->waitHandle_;
  wait.timeout = waitTimeoutMs_;
  wait.dbc_id = vc_->getVC();

  while (1) {
    if (devInterface_->runDevCmd(QAIC_DEV_CMD_WAIT_EXEC, &wait) != QS_SUCCESS) {
      if ((errno == ETIMEDOUT) && (++retries <= numMaxWaitRetries_)) {
        LogWarn(
            "Dev {} VC {} wait handle {} Kernel wait timeout, retries:{} of {}",
            (uint32_t)dev_->getID(), (uint32_t)vc_->getVC(),
            infHandle->waitHandle_, retries, numMaxWaitRetries_);
        continue;
      }
      LogError("Dev {} VC {} failed to send wait exec IOCTL: {} ",
               (uint32_t)dev_->getID(), (uint32_t)vc_->getVC(),
               QOsal::strerror_safe(errno));
      status = QS_ERROR;
      break;
    } else {
      break;
    }
  }

  submitWaitCv_.notify_one(); // Notify any thread waiting for space in Queue
  return status;
}

//
// returns all in/out buffers for a batch
//

QStatus
QNeuralnetwork::getInfBuffers(const QInfHandle *infHandle,
                                              std::vector<QBuffer> &bufs)
    const {
  if (infHandle == nullptr) {
    return QS_INVAL;
  }
  for (auto &b : infHandle->kmdQBufs_) {
    bufs.push_back(b);
  }
  return QS_SUCCESS;
}


uint32_t QNeuralnetwork::getVcQueueLevel() const {
  QOsal::getDbcQueuedSize((uint32_t *)&dbcQueuedSize_, (QPciInfo *)&qPciInfo_,
                          (uint32_t)vc_->getVC());
  return dbcQueuedSize_;
}


uint32_t QNeuralnetwork::getVcId() const {
  if (vc_ != nullptr) {
    return (uint32_t)vc_->getVC();
  } else {
    return qutil::INVALID_VCID;
  }
}

//
// returns all out buffers for a batch
//

QStatus
QNeuralnetwork::getInfOutputBuffers(const QInfHandle *infHandle,
                                                    std::vector<QBuffer>
                                                        &outBufs) const {
  if (infHandle == nullptr) {
    return QS_INVAL;
  }
  for (uint j = 0; j < infHandle->numBufs_; j++) {
    if (infHandle->bufDirs_[j] == QDirection::QDIR_FROM_DEV) {
      outBufs.push_back(infHandle->kmdQBufs_[j]);
    }
  }

  return QS_SUCCESS;
}

//
// returns all in buffers for a batch
//

QStatus
QNeuralnetwork::getInfInputBuffers(const QInfHandle *infHandle,
                                                   std::vector<QBuffer> &inBufs)
    const {
  if (infHandle == nullptr) {
    return QS_INVAL;
  }
  for (uint j = 0; j < infHandle->numBufs_; j++) {
    if (infHandle->bufDirs_[j] == QDirection::QDIR_TO_DEV) {
      inBufs.push_back(infHandle->kmdQBufs_[j]);
    }
  }

  return QS_SUCCESS;
}

//
// This function is protected with a lock, so execute IOCTLs from different
// threads don't mix together. In the case of EAGAIN we will retry and if all
// reties fail we either return QS_AGAIN if nothing has been enqueued yet,
// or QS_ERROR if part of the requests are queued.
//

QStatus QNeuralnetwork::runExecute(const QInfHandle *infHandle,
                                                   const struct qaic_execute *execute,
                                                   bool isPartial) {
  std::unique_lock<std::mutex> lock(submitWaitMutex_);
  bool success = false;
  uint32_t submitRetryCount = 0;
  uint32_t submitRetryWaitProgressiveUs = submitRetryTimeoutUs_;
  bool submitRetryFirst = true;
  std::chrono::time_point<std::chrono::steady_clock> submitFirstTimePoint;
  ExecProfilingData execProfilingData;
  QStatus ret;

  while (1) {

    if (devInterface_->runDevCmd(isPartial ? QAIC_DEV_CMD_PARTIAL_EXECUTE
                                           : QAIC_DEV_CMD_EXECUTE,
                                 execute) != QS_SUCCESS) {
      if (errno == EAGAIN) {
        auto now = std::chrono::steady_clock::now();
        if (submitRetryFirst) {
          submitFirstTimePoint = now;
          submitRetryFirst = false;

          // Check if the queue size is adequate for this execution
          ret = getExecProfilingData(infHandle, execProfilingData);
          if (ret != QS_SUCCESS) {
            LogError("Failed to retrieve profiling data");
            return ret;
          }
          if (execProfilingData.numInfInQueue > vc_->getQueueSize()) {
            LogError(
                "Dev:{} VC:{} VC queue size is too small to hold all the "
                "requests. VC Queue size {} Queue size atleast expected {}",
                (uint32_t)dev_->getID(), (uint32_t)vc_->getVC(),
                vc_->getQueueSize(), execProfilingData.numInfInQueue);
            return QS_ERROR;
          }
        }
        submitRetryCount++;
        LogDebug("Dev:{} VC:{} Device Busy in runExecute, retry:{} of {}, "
                 "waiting {} ms",
                 (uint32_t)dev_->getID(), (uint32_t)vc_->getVC(),
                 submitRetryCount, submitRetryMax_,
                 submitRetryWaitProgressiveUs);
        if (submitRetryCount >= submitRetryMax_) {
          auto totalWaitTimeMs =
              std::chrono::duration_cast<std::chrono::milliseconds>(
                  now - submitFirstTimePoint)
                  .count();
          LogWarn(
              "Dev:{} VC:{} Device busy, failed to send Execute IOCTL rc:{}, "
              "after {} retries, retry Timeout: {}ms, total wait time {}ms ",
              (uint32_t)dev_->getID(), (uint32_t)vc_->getVC(),
              QOsal::strerror_safe(errno), submitRetryCount,
              submitRetryWaitProgressiveUs / 1000, totalWaitTimeMs);
          return QS_BUSY;
        }
        if (submitWaitCv_.wait_until(lock,
                                     now + std::chrono::microseconds(
                                               submitRetryWaitProgressiveUs)) ==
            std::cv_status::timeout) {
          LogDebug("Timeout");
        } else {
          LogDebug("Predicate");
        }

        ;
        LogDebug("Dev:{} VC:{} Retry Continue", (uint32_t)dev_->getID(),
                 (uint32_t)vc_->getVC());
        // Each time we wait, increase the wait time by the same amount
        // for 20 rounds, this will result in 10x thimes the wait
        // This allows for slower networks to not have to retry 200+ times
        submitRetryWaitProgressiveUs +=
            (submitRetryTimeoutUs_ * submitRetryCount);
        continue;
      }
      LogError("Dev {} VC {} failed to send Execute IOCTL: {}",
               (uint32_t)dev_->getID(), (uint32_t)vc_->getVC(),
               QOsal::strerror_safe(errno));
      return QS_ERROR;
    }
    success = true;
    break;
  }
  if (!success) {
    return QS_ERROR;
  }
  return QS_SUCCESS;
}

//
// Count is the Element Count from MemReq, not the number of buffers
// it prepares buffer for part of batch
//

QStatus
QNeuralnetwork::prepareInfHandleBuf(qaic_create_bo *createBO,
                                                    QBuffer &kbuf) const {

  qaic_attach_slice *attach_slice = reinterpret_cast<qaic_attach_slice *>(createBO + 1);

  if (devInterface_->runDevCmd(QAIC_DEV_CMD_MEM, createBO) != QS_SUCCESS) {
    LogError("Dev {} VC {} failed to send Mem IOCTL: {}",
             (uint32_t)dev_->getID(), (uint32_t)vc_->getVC(),
             QOsal::strerror_safe(errno));
    attach_slice->hdr.dbc_id = INVALID_DBC_ID;
    return QS_INVAL;
  }

  LogDebug("Dev {} VC {} Mem handle {:#016x}", (uint32_t)dev_->getID(),
           (uint32_t)vc_->getVC(), createBO->handle);

  if (kbuf.type == QBufferType::QBUFFER_TYPE_HEAP && createBO->size != 0) {
    int devFd = devInterface_->getDevFd();
    void *base = QOsal::qMmap(0, createBO->size, PROT_READ | PROT_WRITE, MAP_SHARED,
                              devFd, createBO->handle);
    if (base == (void *)-1) {
      LogError("Dev {} VC {} qMmap failed {}", (uint32_t)dev_->getID(),
               (uint32_t)vc_->getVC(), QOsal::strerror_safe(errno));
      return QS_NOMEM;
    }
    kbuf.buf = reinterpret_cast<uint8_t *>(base);
  }

  return QS_SUCCESS;
}

//
// Release the buffer obtained through prepareOneBuf
//

QStatus
QNeuralnetwork::unprepareBuf(uint8_t *boReqPtr, uint32_t count,
                                             std::vector<QBuffer> &kbuf) const {

  int attachSliceEntryProcessed = 0;
  for (uint32_t i = 0; i < count; i++) {
    if ((kbuf[i].buf != nullptr) &&
        (kbuf[i].type == QBufferType::QBUFFER_TYPE_HEAP)) {
      QOsal::qMunmap(kbuf[i].buf, kbuf[i].size);
      kbuf[i].buf = nullptr;
    }

    qaic_create_bo *createBO =
        reinterpret_cast<qaic_create_bo *>(boReqPtr + ((sizeof(qaic_create_bo) + sizeof(qaic_attach_slice)) * i +
                                  sizeof(qaic_attach_slice_entry) * attachSliceEntryProcessed));
    qaic_attach_slice *attach_slice = reinterpret_cast<qaic_attach_slice *>(createBO + 1);
    if (attach_slice->hdr.dbc_id != INVALID_DBC_ID) {
      // Make sure we call free on all memReq, even if rc is != SUCCESS
      drm_gem_close close_bo = {};
      close_bo.handle = createBO->handle;
      QStatus status = devInterface_->runDevCmd(QAIC_DEV_CMD_FREE_MEM, &close_bo);
      if (status != QS_SUCCESS) {
        LogError("Dev {} VC {} failed to send Mem IOCTL {}",
               (uint32_t)dev_->getID(), (uint32_t)vc_->getVC(),
               QOsal::strerror_safe(errno));
      }
    }
    attach_slice->hdr.dbc_id = INVALID_DBC_ID;
    attachSliceEntryProcessed += attach_slice->hdr.count;
  }

  return QS_SUCCESS;
}


QStatus QNeuralnetwork::activateStateChange(QActivationStateType
                                                                stateCmd) {
  std::vector<std::pair<QNAID, QActivationStateType>> stateCmdSet;
  stateCmdSet.emplace_back(std::make_pair(naID_, stateCmd));
  return dev_->sendActivationStateChangeCommand(stateCmdSet);
}

//
// deactivate
//

QStatus QNeuralnetwork::deactivate() {

  bool deactivate_vc = false;
  vc_->decRef();

  if (vc_->getVC() != NNC_VC_RESERVATION_ID_SKIP) {
    if (vc_->getRefCnt() == 0) {
      deactivate_vc = true;
    } else {
      deactivate_vc = false;
    }
  }

  QStatus status;
  uint16_t retryCount = deactivationRetryCount;

  LogInfo("Dev {} VC {} deactivate network with NAID {}",
          (uint32_t)dev_->getID(), (uint32_t)vc_->getVC(), naID_);

  while (retryCount--) {
    status = dev_->deactivate(vc_->getVC(), naID_, deactivate_vc);

    if (status == QS_AGAIN) {
      /* Wait for device to recover */
      std::this_thread::sleep_for(std::chrono::seconds(1));
      LogWarn("Dev {} VC {} NAID {} Deactivate retry Count {}",
              (uint32_t)dev_->getID(), (uint32_t)vc_->getVC(), naID_,
              (deactivationRetryCount - retryCount));
      continue;
    }
    break;
  }

  if (status != QS_SUCCESS) {
    LogError("Dev {} VC {} NAID {} deactivate failed status {}",
             (uint32_t)dev_->getID(), (uint32_t)vc_->getVC(), naID_, status);
    return status;
  }

  static_cast<QNNImage *>(image_)->decRef();
  if (constants_ != nullptr) {
    static_cast<QNNConstants *>(constants_)->decRef();
  }

  delete this;
  return QS_SUCCESS;
}

//
// query ioctl
//

QStatus
QNeuralnetwork::
    getExecProfilingData(const QInfHandle *infHandle,
                         ExecProfilingData &execProfilingData) const {
  qaic_perf_stats perfStats = {};
  int32_t waitIndex = -1;
  uint8_t data[QAIC_MANAGE_MAX_MSG_LENGTH];

  if (infHandle == nullptr) {
    return QS_INVAL;
  }

  if (infHandle->numBufs_ == 0) {
    return QS_INVAL;
  }

  auto perfStatEntry = reinterpret_cast<qaic_perf_stats_entry *>(data);
  auto execute = reinterpret_cast<qaic_execute *>(infHandle->execute_.get());
  auto executeEntry = reinterpret_cast<qaic_execute_entry *>(execute->data);
  auto partialExecuteEntry = reinterpret_cast<qaic_partial_execute_entry *>(execute->data);

  // Populate all the buffer handles
  for (uint32_t i = 0; i < execute->hdr.count; i++) {
    perfStatEntry[i].handle = infHandle->hasPartialTensor_
                                 ? partialExecuteEntry[i].handle
                                 : executeEntry[i].handle;
    if (infHandle->waitHandle_ == perfStatEntry[i].handle) {
      waitIndex = i;
    }
  }

  if (waitIndex == -1) {
    waitIndex = execute->hdr.count - 1;
  }

  // Populate the hdr
  perfStats.hdr.count = execute->hdr.count;
  perfStats.hdr.dbc_id = execute->hdr.dbc_id;
  perfStats.data = (uint64_t)data;

  LogDebug("Count {} dbc_id {} ", (uint32_t)perfStats.hdr.count,
           (uint32_t)perfStats.hdr.dbc_id);

  if (devInterface_->runDevCmd(QAIC_DEV_CMD_QUERY, &perfStats) != QS_SUCCESS) {
    LogError("Dev {} VC {} failed to send query IOCTL: {} ",
             (uint32_t)dev_->getID(), (uint32_t)vc_->getVC(),
             QOsal::strerror_safe(errno));
    return QS_ERROR;
  }

  // Calculate the total request element that will be queue upon execution
  uint32_t total_requests = 0;
  for (uint32_t i = 0; i < execute->hdr.count; i++) {
    total_requests += perfStatEntry[i].num_queue_element;
  }

  execProfilingData.queueDepth = perfStatEntry[0].queue_level_before;
  execProfilingData.numInfInQueue = total_requests;

  if (execProfilingData.queueDepth > 0 && total_requests) {
    execProfilingData.numInfInQueue =
        1 + (execProfilingData.queueDepth / (total_requests));
  } else {
    execProfilingData.numInfInQueue = 1;
  }

  execProfilingData.kernelSubmitToVcLatencyUs =
      perfStatEntry[waitIndex].submit_latency_us;
  execProfilingData.kernelVcToInterruptLatencyUs =
      perfStatEntry[waitIndex].device_latency_us;
  execProfilingData.isValid = true;

  LogDebug("QueueLevel before submission of execute {} Number of elements "
           "queued inside the queue {} Submit Latency {}us"
           " Interrupt Latency {}us",
           execProfilingData.queueDepth, execProfilingData.numInfInQueue,
           execProfilingData.kernelSubmitToVcLatencyUs,
           execProfilingData.kernelVcToInterruptLatencyUs);

  return QS_SUCCESS;
}

} // namespace qaic
