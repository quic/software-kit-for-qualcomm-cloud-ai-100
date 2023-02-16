// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QINEURAL_NETWORK_H
#define QINEURAL_NETWORK_H

#include "QActivationStateCmd.h"

namespace qaic {

struct QInfHandle;

struct ExecProfilingData {
  ExecProfilingData()
      : isValid(false), queueDepth(0), numInfInQueue(0),
        kernelSubmitToVcLatencyUs(0), kernelVcToInterruptLatencyUs(0) {}
  void reset() {
    isValid = false;
    queueDepth = 0;
    numInfInQueue = 0;
    kernelSubmitToVcLatencyUs = 0;
    kernelVcToInterruptLatencyUs = 0;
  }
  bool isValid;
  uint32_t queueDepth;    // How many items are in the queue before
                          // submission of execute ioctl
  uint32_t numInfInQueue; // Number inferences in the queue after the inference
                          // submission to the queue
  uint32_t kernelSubmitToVcLatencyUs; // Time in kernel between kernel receiving
                                      // IOCTL and submitting to VC
  uint32_t kernelVcToInterruptLatencyUs; // Time in kernel between submitting to
                                         // VC and receiving interrupt
};

/// This class contains information of a successfully activated network.
/// The main purpose of the class is to run inference and to deactivate
/// the network.
class QNeuralNetworkInterface {
public:
  /// Initialization should be called after construction
  virtual bool init() = 0;

  /// Deactivate the network. This function blocks till all the input/output
  /// data have been drained, and the deactivate request to driver has
  /// returned. The deactivate function also destroys the
  /// QNeuralNetworkInterface
  /// object.
  virtual QStatus deactivate() = 0;

  virtual QStatus activateStateChange(QActivationStateType stateCmd) = 0;

  /// GetInfHandle
  virtual std::shared_ptr<QInfHandle>
  getInfHandle(const QBuffer *bufs, uint32_t numBuf, const QDirection *bufDir,
               bool hasPartialTensor) const = 0;

  virtual QStatus loadData(const QInfHandle *infHandle, const QBuffer *bufs,
                           uint32_t numBuf) const = 0;

  virtual QStatus
  prepareData(const QInfHandle *infHandle,
              const std::vector<uint64_t> &dmaBufferSizes) const = 0;
  virtual QStatus enqueueData(const QInfHandle *infHandle) = 0;

  virtual ~QNeuralNetworkInterface() = default;

  virtual QStatus wait(const QInfHandle *infHandle) = 0;
  // Get buffers allocated in getInfHandle()

  virtual QStatus getInfBuffers(const QInfHandle *infHandle,
                                std::vector<QBuffer> &outBufs) const = 0;
  // Get the output buffers only
  virtual QStatus getInfOutputBuffers(const QInfHandle *infHandle,
                                      std::vector<QBuffer> &outBufs) const = 0;
  virtual QStatus getInfInputBuffers(const QInfHandle *infHandle,
                                     std::vector<QBuffer> &outBufs) const = 0;
  virtual uint64_t getInfCount() const = 0;
  virtual uint32_t getVcQueueSize() const = 0;
  virtual uint32_t getVcQueueLevel() const = 0;
  virtual uint32_t getVcId() const = 0;
  virtual QStatus
  getExecProfilingData(const QInfHandle *infHandle,
                       ExecProfilingData &execProfiling) const = 0;
  virtual QNAID getId() const = 0;
};

} // namespace qaic
#endif // QINEURAL_NETWORK_H
