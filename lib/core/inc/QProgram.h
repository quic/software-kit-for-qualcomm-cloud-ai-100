// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
#ifndef QPROGRAM_H
#define QPROGRAM_H

#include "QAicRuntimeTypes.h"
#include "QAic.h"
#include "QLog.h"
#include "QProgramTypes.h"
#include "QContext.h"
#include "QKmdRuntime.h"
#include "QRuntimeManager.h"
#include "QProgramContainer.h"
#include "QBindingsParser.h"
#include "AICNetworkDesc.pb.h"
#include "QComponent.h"
#include "QNeuralNetworkInterface.h"
#include "QProgramInfo.h"
#include "QProgramDevice.h"
#include "QContext.h"
#include "QAicApi.pb.h"
#include "metadataflatbufDecode.hpp"

namespace qaic {

class QExecObj;
class QProgram;
using shQProgram = std::shared_ptr<QProgram>;

class QIConstants;
using shQIConstants = std::shared_ptr<QIConstants>;

QAicObjId getNextObjId();

class QProgram : public virtual QComponent,
                 public QIAicApiContext,
                 public std::enable_shared_from_this<QProgram> {
  friend class QExecObj;
  friend class QProgramDevice;

public:
  // Specify type of activaiton requested
  enum ActivationType {
    ACTIVATION_TYPE_STANDBY = 1,
    ACTIVATION_TYPE_FULL_ACTIVATION = 2,
    ACTIVATION_TYPE_INITIAL_DEFAULT = 3
  };

  static QStatus initProperties(QAicProgramProperties *properties);

  static shQProgram createProgram(shQContext context,
                                  const QAicProgramProperties &properties,
                                  QID dev, const char *name,
                                  shQProgramContainer qpcObj, QStatus &status);

  QProgram(shQContext &context, const QAicProgramProperties &properties,
           QID dev, const char *name, shQProgramContainer qpcObj);

  virtual QStatus releaseProgram();
  virtual ~QProgram();
  virtual QStatus load();
  virtual bool isActive();
  virtual QStatus unload();
  virtual QStatus processActivateCmd(QProgramActivationCmd cmd);
  virtual const char *getName() const;
  virtual QStatus getInferenceCompletedCount(uint64_t &count);
  virtual QStatus getProgramInfo(QAicProgramInfo &info);

  QProgramContainer *getContainer();
  QStatus processActivateCmd(QAicProgramActivationCmd cmd);
  QStatus
  getActivationHandle(QID dev, QProgramDevice *&programDevice,
                      ActivationType acType = ACTIVATION_TYPE_FULL_ACTIVATION);
  QStatus putActivationHandle(QID dev);

  void setdefaultActivationType(ActivationType activationType);
  QStatus getIoDescriptor(const aicapi::IoDesc **ioDesc);
  QStatus getIoDescriptor(QData *ioDescData);
  QStatus getIoBufferInfo(QAicIoBufferInfo **bufferInfo) const;
  QStatus getIoBufferInfoDma(QAicIoBufferInfo **bufferInfoDma) const;
  bool hasConstants() { return !!constants_; }
  std::string strIoBufferInfo(const QAicIoBufferInfo *ioBufferInfo);
  const std::string strIoBindings(const aicapi::IoBinding *binding) const;
  const shQContext &context() { return context_; }
  QStatus getDeviceQueueLevel(uint32_t &fillLevel, uint32_t &queueSize);

  bool isManuallyActivated();
  AicMetadataFlat::MetadataT getMetadata() const {
    if (metadata_)
      return *metadata_;
    AicMetadataFlat::MetadataT dummy;
    return dummy;
  }

  QProgramDevice *getProgramDevice();
  const aicnwdesc::networkDescriptor *getNetworkDesc() const {
    return &networkDesc_;
  }

  QID getQid() const { return dev_; }

  std::string getNetworkName() const { return networkDesc_.network_name(); }

  // Retrieve the original unparsed metadata
  const std::vector<uint8_t> &getInitMetadata() const {
    return metadataBufferInit_;
  }
  const QData &getInitNwDescData() const { return networkDescData_; }
  QBuffer getInitMetadataBuf() {
    return {.size = metadataBufferInit_.size(),
            .buf = metadataBufferInit_.data()};
  };
  QBuffer getRawMetadataBuf() {
    return {.size = metadataBufferRaw_.size(),
            .buf = metadataBufferRaw_.data()};
  };
  QStatus updateProgramData(const std::vector<uint8_t> &newMeta,
                            const std::vector<uint8_t> &newNwDesc);

protected:
  QNeuralNetworkInterface *nn();
  void getUserBufferDirections(const QDirection *&dir, uint32_t &numBufs) const;
  QStatus getDmaBufferSizes(QBuffer *dir, uint32_t numBufs);
  const QData &getProgramBuffer();
  bool hasPartialTensor() const { return hasPartialTensor_; };
  const QAicProgramProperties programProperties_;
  bool updateInternalData();
  QID dev_;
  shQIConstants constants_;
  QBuffer constantsQBuffer_;
  std::string name_;
  aicapi::IoDesc *ioDescPb_;
  std::vector<uint8_t> ioDescPbBuffer_;
  QAicIoBufferInfo *bufferInfo_;
  QRuntimeInterface *rt_;
  aicnwdesc::networkDescriptor networkDesc_;
  QBindingsParser bindingsParser_;
  std::mutex programMutex_;

  QProgram(const QProgram &) = delete;            // Disable Copy Constructor
  QProgram &operator=(const QProgram &) = delete; // Disable Assignment Operator

private:
  bool init();
  QStatus activate(QID dev);
  QStatus deactivate(QID dev);
  QStatus parseIoDescriptor();
  uint32_t calcIoSize(const aicnwdesc::IODescriptor &desc);
  template <typename T> uint32_t writeBindingElement(const T *src, int elts);
  uint32_t writeBinding(const aicnwdesc::IODescriptor &protoDesc, uint8_t *to,
                        uint32_t maxSize);

  const std::string strBufferMappings(const QAicBufferMappings *mapping) const;

  QAicIoBufferInfo *bufferInfoDma_;

  std::unique_ptr<AicMetadataFlat::MetadataT> metadata_;
  std::vector<uint8_t> metadataBuffer_;
  std::vector<uint8_t> metadataBufferRaw_; // unparsed metadataBuffer_
  std::vector<uint8_t> metadataBufferInit_;
  std::vector<QDirection> userBufferQDirections_;
  std::vector<QDirection> dmaBufferQDirections_;
  const QData programQpcUserData_; // Buffers passed by user
  shQProgramContainer programContainer_;
  QProgramGroup *programGroup_;
  QData networkData_;
  QData networkDescData_;
  QData networkDescDataInit_;

  std::unique_ptr<uint8_t[]> programBuffer_; // Keeps a copy of the QPC

  std::mutex programDeviceInitMutex_;
  uint32_t programDeviceRefCount_;
  std::unique_ptr<QProgramDevice> programDevice_;
  bool initialized_;
  bool hasPartialTensor_;
  bool isManuallyActivated_;
  ActivationType defaultActivationType_;

  static constexpr QAicProgramProperties defaultProperties_ = {
      static_cast<uint32_t>(QAicProgramPropertiesBitfields::
                                QAIC_PROGRAM_PROPERTIES_SELECT_MASK_DEFAULT),
      QAIC_PROGRAM_PROPERTIES_SUBMIT_NUM_RETRIES_DEFAULT,
      QAIC_PROGRAM_PROPERTIES_SUBMIT_TIMEOUT_MS_DEFAULT, 0};
  const char *userName_;
  std::vector<uint8_t> updatedNwDescData_;
};

} // namespace qaic

#endif // QPROGRAM_H
