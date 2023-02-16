// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QPRD_NEURAL_NETWORK_H
#define QPRD_NEURAL_NETWORK_H

#include "QLogger.h"
#include "QAicRuntimeTypes.h"
#include "QDevAic100Interface.h"
#include "QNeuralNetworkInterface.h"
#include "QNNConstantsInterface.h"
#include "QActivationStateCmd.h"
#include "QNNImageInterface.h"

#include <memory>
#include <mutex>
#include <unordered_map>

namespace qaic {

class QDeviceInterface;
class QVirtualChannelInterface;
class QMetaDataInterface;

struct QInfHandle;
using shQInfHandle = std::shared_ptr<QInfHandle>;
struct QInfHandle {
public:
  static shQInfHandle
  Factory(uint32_t numReqs, uint64_t waitHandle, shQDevInterface &devInterface,
          std::unique_ptr<uint8_t[]> memReq, std::unique_ptr<uint8_t[]> execute,
          // Number of KMD Buffs is not equal to dma requests
          std::vector<QBuffer> &kmdQBufs, std::vector<QDirection> &bufDirs,
          bool hasPartialTensor = false);

  ~QInfHandle();
  bool init();

private:
  QInfHandle(uint32_t numReqs, uint64_t waitHandle,
             shQDevInterface devInterface, std::unique_ptr<uint8_t[]> memReq,
             std::unique_ptr<uint8_t[]> execute,
             // Number of KMD Buffs is not equal to dma requests
             std::vector<QBuffer> &kmdQBufs, std::vector<QDirection> &bufDirs,
             bool hasPartialTensor);

  // Constructor Args
  uint32_t numReqs_;
  uint64_t waitHandle_;
  shQDevInterface devInterface_;
  std::unique_ptr<uint8_t[]> memReq_;
  std::unique_ptr<uint8_t[]> execute_;
  std::vector<QBuffer> kmdQBufs_;
  std::vector<QDirection> bufDirs_;
  bool hasPartialTensor_;
  // Local Data
  uint32_t numBufs_;

  friend class QNeuralnetwork;
};

/// This class is the frontend of the data path. The API to run an inference and
/// deactivate a network is implemented here.

class QNeuralnetwork : public QNeuralNetworkInterface, public QLogger {
public:
  static QNeuralnetwork *Factory(QDeviceInterface *device,
                                 QNNImageInterface *image,
                                 QNNConstantsInterface *constants,
                                 std::unique_ptr<QMetaDataInterface> meta,
                                 QVirtualChannelInterface *vc, QNAID naID,
                                 uint32_t waitTimeoutMs,
                                 uint32_t numMaxWaitRetries);

  ~QNeuralnetwork() = default;

  /// Initialization should be called after construction
  virtual bool init() override;

  QStatus deactivate() override;
  virtual QStatus activateStateChange(QActivationStateType stateCmd) override;
  virtual std::shared_ptr<QInfHandle>
  getInfHandle(const QBuffer *bufs, uint32_t numBuf, const QDirection *bufDir,
               bool hasPartialTensor = false) const override;
  virtual QStatus loadData(const QInfHandle *infHandle, const QBuffer *bufs,
                           uint32_t numBuf) const override;
  virtual QStatus
  prepareData(const QInfHandle *infHandle,
              const std::vector<uint64_t> &dmaBufferSizes) const override;
  virtual QStatus enqueueData(const QInfHandle *infHandle) override;
  virtual QStatus wait(const QInfHandle *infHandle) override;
  // Get buffers allocated in getInfHandle()
  virtual QStatus getInfBuffers(const QInfHandle *infHandle,
                                std::vector<QBuffer> &bufs) const override;
  virtual QStatus
  getInfOutputBuffers(const QInfHandle *infHandle,
                      std::vector<QBuffer> &outBufs) const override;
  virtual QStatus
  getInfInputBuffers(const QInfHandle *infHandle,
                     std::vector<QBuffer> &outBufs) const override;
  virtual uint64_t getInfCount() const override { return infCount_; };
  virtual uint32_t getVcQueueSize() const override { return dbcFifoSize_; };
  virtual uint32_t getVcQueueLevel() const override;
  virtual uint32_t getVcId() const override;
  virtual QStatus
  getExecProfilingData(const QInfHandle *infHandle,
                       ExecProfilingData &execProfilingData) const override;

  virtual QNAID getId() const override { return naID_; };

private:
  QNeuralnetwork() = delete;
  QNeuralnetwork(const QNeuralnetwork &) = delete;
  QNeuralnetwork &operator=(const QNeuralnetwork &) = delete;
  QNeuralnetwork(QDeviceInterface *device, QNNImageInterface *image,
                 QNNConstantsInterface *constants,
                 std::unique_ptr<QMetaDataInterface> meta,
                 QVirtualChannelInterface *vc, QNAID naID,
                 uint32_t waitTimeoutMs, uint32_t numMaxWaitRetries);

  std::shared_ptr<QInfHandle>
  createInfHandle(std::unique_ptr<uint8_t[]> boReqOwner,
                  std::unique_ptr<uint8_t[]> executeOwner,
                  uint32_t &reqProcessed, int waitIndex,
                  std::vector<QBuffer> &kmdQBufs, std::vector<QDirection> &dirs,
                  bool hasPartialTensor) const;
  QStatus unprepareBuf(uint8_t *boReqPtr, uint32_t count,
                       std::vector<QBuffer> &kbuf) const;
  QStatus runExecute(const QInfHandle *infHandle, const qaic_execute *execute,
                     bool isPartial = false);
  QStatus prepareInfHandleBuf(qaic_create_bo *createBO, QBuffer &kbuf) const;
  void freeInfBuffers(uint8_t *boReqPtr, uint32_t reqProcessed,
                      std::vector<QBuffer> &kmdQBufs) const;

  // Constructor Arguments
  QDeviceInterface *dev_;
  QNNImageInterface *image_;
  QNNConstantsInterface *constants_;
  std::unique_ptr<QMetaDataInterface> metadata_;
  QVirtualChannelInterface *vc_;
  QNAID naID_;
  uint32_t elemCount_;
  uint32_t bufCount_;
  QPciInfo qPciInfo_;

  const uint32_t waitTimeoutMs_;
  const uint32_t numMaxWaitRetries_;

  // Initialized Locals
  const uint32_t submitRetryMax_;
  const uint32_t submitRetryTimeoutUs_;
  uint32_t dbcFifoSize_;
  uint32_t dbcQueuedSize_;
  std::atomic<uint64_t> infCount_;
  shQDevInterface devInterface_;

  // Uninitialized Locals
  std::condition_variable submitWaitCv_;
  std::mutex submitWaitMutex_;
};

} // namespace qaic

#endif // QPRD_NEURAL_NETWORK_H
