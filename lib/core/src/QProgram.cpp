// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QUtil.h"
#include "QAicRuntimeTypes.h"
#include "QContext.h"
#include "QProgram.h"
#include "QBindingsParser.h"
#include "QLogger.h"
#include "elfio/elfio.hpp"
#include "metadataflatbufEncode.hpp"
#include "metadataflatbufDecode.hpp"

namespace qaic {

const QAicObjId ObjIdBase = 10;
static std::atomic<QAicObjId> NextUniqueObjId{ObjIdBase};
static std::mutex ObjIdLock;

constexpr QAicProgramProperties QProgram::defaultProperties_;

QAicObjId getNextObjId() {
  std::lock_guard<std::mutex> guard(ObjIdLock);
  if (NextUniqueObjId < ObjIdBase) {
    NextUniqueObjId = ObjIdBase;
  }
  return NextUniqueObjId++;
}

QStatus QProgram::initProperties(QAicProgramProperties *properties) {
  if (properties == nullptr) {
    return QS_INVAL;
  }
  *properties = defaultProperties_;
  return QS_SUCCESS;
}

shQProgram QProgram::createProgram(shQContext context,
                                   const QAicProgramProperties &properties,
                                   QID dev, const char *name,
                                   shQProgramContainer qpcObj,
                                   QStatus &status) {

  status = QS_SUCCESS;
  shQProgram shProgram =
      std::make_shared<QProgram>(context, properties, dev, name, qpcObj);
  if (!shProgram) {
    status = QS_NOMEM;
    return nullptr;
  }
  if (shProgram->init() != true) {
    status = QS_ERROR;
    return nullptr;
  }
  context->registerProgram(shProgram);
  return shProgram;
}

QStatus QProgram::releaseProgram() {
  context_->unRegisterProgram(this);
  return QS_SUCCESS;
}

QProgram::QProgram(shQContext &context, const QAicProgramProperties &properties,
                   QID dev, const char *name, shQProgramContainer qpcObj)
    : QComponent("Program", NextUniqueObjId++, context.get()),
      QIAicApiContext(context), programProperties_(properties), dev_(dev),
      ioDescPb_(nullptr), bufferInfo_(nullptr), rt_(nullptr),
      bufferInfoDma_(nullptr), metadata_(nullptr),
      programQpcUserData_{0, nullptr}, programContainer_(qpcObj),
      networkData_{0, nullptr}, networkDescData_{0, nullptr},
      programBuffer_(nullptr), programDeviceRefCount_(0), initialized_(false),
      hasPartialTensor_(false), isManuallyActivated_(false), userName_(name) {
  rt_ = context_->rt();
}

QProgram::~QProgram() {}

QNeuralNetworkInterface *QProgram::nn() {
  std::unique_lock<std::mutex> lock(programMutex_);
  QProgramDevice *progDev = getProgramDevice();
  if (progDev == nullptr) {
    return nullptr;
  }
  return progDev->nn();
}

QStatus QProgram::getProgramInfo(QAicProgramInfo &info) {
  std::unique_lock<std::mutex> lock(programMutex_);
  QProgramDevice *progDev = getProgramDevice();
  if (progDev == nullptr) {
    return QS_ERROR;
  }
  progDev->getProgramInfo(info);
  return QS_SUCCESS;
}

QStatus QProgram::getInferenceCompletedCount(uint64_t &count) {
  std::unique_lock<std::mutex> lock(programMutex_);
  QProgramDevice *progDev = getProgramDevice();
  if (progDev == nullptr) {
    count = 0;
    return QS_ERROR;
  }
  return progDev->getInferenceCompletedCount(count);
}

QStatus QProgram::getDeviceQueueLevel(uint32_t &fillLevel,
                                      uint32_t &queueSize) {
  std::unique_lock<std::mutex> lock(programMutex_);
  QProgramDevice *progDev = getProgramDevice();
  if (progDev == nullptr) {
    fillLevel = 0;
    queueSize = 0;
    return QS_ERROR;
  }
  return progDev->getDeviceQueueLevel(fillLevel, queueSize);
}

bool QProgram::isManuallyActivated() { return isManuallyActivated_; }

const char *QProgram::getName() const { return name_.c_str(); }

QProgramContainer *QProgram::getContainer() {
  if (!programContainer_) {
    return nullptr;
  }
  return programContainer_.get();
}

QStatus QProgram::load() {
  std::unique_lock<std::mutex> lock(programMutex_);
  QProgramDevice *progDev = getProgramDevice();
  if (progDev == nullptr) {
    return QS_ERROR;
  }
  return progDev->load();
}

QStatus QProgram::unload() {
  std::unique_lock<std::mutex> lock(programMutex_);
  QProgramDevice *progDev = getProgramDevice();
  if (progDev == nullptr) {
    return QS_ERROR;
  }
  return progDev->unload();
}

bool QProgram::isActive() {
  std::unique_lock<std::mutex> lock(programMutex_);
  QProgramDevice *progDev = getProgramDevice();
  if (progDev == nullptr) {
    return false;
  }
  return progDev->isActive();
}

// From API
QStatus QProgram::processActivateCmd(QAicProgramActivationCmd cmd) {
  QProgramActivationCmd icmd;
  switch (cmd) {
  case QAicProgramActivationCmd::QAIC_PROGRAM_CMD_ACTIVATE_FULL:
    icmd = QProgram_CMD_ACTIVATE_FULL;
    isManuallyActivated_ = true;
    break;
  case QAicProgramActivationCmd::QAIC_PROGRAM_CMD_DEACTIVATE_FULL:
    icmd = QProgram_CMD_DEACTIVATE_FULL;
    break;
  default:
    icmd = QIPROGRRAM_CMD_INVALID;
    break;
  }
  return processActivateCmd(icmd);
}

QStatus QProgram::processActivateCmd(QProgramActivationCmd cmd) {
  std::unique_lock<std::mutex> lock(programMutex_);
  auto progDev = getProgramDevice();
  if (progDev != nullptr) {
    return progDev->processActivateCmd(cmd);
  }
  return QS_ERROR;
}

// Gets an activation for this program on device specified, if
QStatus QProgram::getActivationHandle(QID qid __attribute__((unused)),
                                      QProgramDevice *&programDevice,
                                      ActivationType acType) {
  QStatus status = QS_SUCCESS;
  // First ensure that we get a unique programDevice, if not
  // initialized create it with the factory
  std::unique_lock<std::mutex> programDeviceInitLock(programDeviceInitMutex_);
  if (!programDevice_) {
    std::unique_ptr<QProgramDevice> progDev =
        QProgramDevice::Factory(context_, this, dev_);
    if (!progDev) {
      LogError("Failed to create programDevice");
      return QS_ERROR;
    }
    programDevice_ = std::move(progDev);
  }
  programDeviceRefCount_++;
  programDeviceInitLock.unlock();

  std::unique_lock<std::mutex> lock(programMutex_);
  if (!programDevice_->isReady()) {
    programDevice_->load();

    // Need to activate
    QProgramActivationCmd acCmd = QProgram_CMD_ACTIVATE_FULL;
    switch (acType) {
    case ACTIVATION_TYPE_FULL_ACTIVATION:
      acCmd = QProgram_CMD_ACTIVATE_FULL;
      break;
    default:
      LogErrorApi("unsupported activation type");
      break;
    }
    status = programDevice_->processActivateCmd(acCmd);

    if (status == QS_SUCCESS) {
      programDevice = programDevice_.get();
    } else {
      LogErrorApi("Failed to get Program Device Activation Handle");
      programDevice = nullptr;
      return status;
    }
  } else {
    // return existing activation
    programDevice = programDevice_.get();
  }

  LogDebugApi("Program Device, QID:{}", dev_);
  return QS_SUCCESS;
}

void QProgram::setdefaultActivationType(ActivationType activationType) {
  defaultActivationType_ = activationType;
}

QStatus QProgram::putActivationHandle(QID qid __attribute__((unused))) {
  // first  second
  //| QID
  //      first             second
  //     | < ProgramDevice | RefCnt>
  QStatus status = QS_SUCCESS;
  // Lock for creation of programDevice_
  std::unique_lock<std::mutex> programDeviceInitLock(programDeviceInitMutex_);
  if (!programDevice_) {
    // Not found, nothing to do
    return QS_SUCCESS;
  } else {
    if (programDeviceRefCount_ > 0) {
      programDeviceRefCount_--;
    }
    if (programDeviceRefCount_ == 0) {
      programDeviceInitLock.unlock();
      // Lock for operations on programDevice e.g activation/deactivation
      std::unique_lock<std::mutex> lock(programMutex_);
      // Deactivate
      status = programDevice_->processActivateCmd(QProgram_CMD_DEACTIVATE_FULL);
      if (status != QS_SUCCESS) {
        LogErrorApi("Failed to deactivate in putActivationHandle processing");
        return status;
      }
    }
  }
  return QS_SUCCESS;
}

void QProgram::getUserBufferDirections(const QDirection *&dir,
                                       uint32_t &numBufs) const {
  dir = userBufferQDirections_.data();
  numBufs = userBufferQDirections_.size();
};

QStatus QProgram::getDmaBufferSizes(QBuffer *buf, uint32_t numBufs) {
  if (numBufs != (uint32_t)networkDesc_.dma_buffers().size()) {
    LogErrorG("Unexpected number of buffes, networkDesc specifies {} buffes, "
              "called with {}",
              networkDesc_.dma_buffers().size(), numBufs);
    return QS_ERROR;
  }
  for (int32_t i = 0; i < networkDesc_.dma_buffers().size(); i++) {
    buf[i].size = networkDesc_.dma_buffers(i).size();
  }
  return QS_SUCCESS;
}

const QData &QProgram::getProgramBuffer() { return networkData_; }

QStatus QProgram::getIoDescriptor(const aicapi::IoDesc **ioDesc) {
  if (ioDesc == nullptr) {
    return QS_INVAL;
  }
  // IO Descriptor will be parsed on init
  if (ioDescPb_ == nullptr) {
    LogErrorG("Invalid state for Program, descriptor not parsed");
    return QS_ERROR;
  }
  *ioDesc = ioDescPb_;
  return QS_SUCCESS;
}

QStatus QProgram::getIoDescriptor(QData *ioDescData) {
  if (ioDescData == nullptr) {
    LogErrorG("Invalid pointer in getIoDescriptor");
    return QS_INVAL;
  }
  // IO Descriptor will be parsed on init
  if ((ioDescPb_ == nullptr) || (ioDescPbBuffer_.size() == 0)) {
    LogErrorG("Invalid state for Program, descriptor not parsed: "
              "ioDescPbBuffer:{}",
              ioDescPbBuffer_.size());
    return QS_INVAL;
  }
  ioDescData->size = ioDescPbBuffer_.size();
  ioDescData->data = ioDescPbBuffer_.data();
  return QS_SUCCESS;
}

QStatus QProgram::getIoBufferInfo(QAicIoBufferInfo **bufferInfo) const {
  if (bufferInfo == nullptr) {
    LogErrorG("Invalid pointer in getIoBufferInfo");
    return QS_INVAL;
  }

  if (bufferInfo_ == nullptr) {
    LogErrorApi("Buffer info not initialized");
    return QS_ERROR;
  }
  *bufferInfo = bufferInfo_;
  return QS_SUCCESS;
}

QStatus QProgram::getIoBufferInfoDma(QAicIoBufferInfo **bufferInfoDma) const {
  if (bufferInfoDma == nullptr) {
    LogErrorG("Invalid pointer in getIoBufferInfo");
    return QS_INVAL;
  }

  if (bufferInfoDma_ == nullptr) {
    LogErrorApi("Buffer info not initialized");
    return QS_ERROR;
  }
  *bufferInfoDma = bufferInfoDma_;
  return QS_SUCCESS;
}

//----------------------------------------------------------------------
// Private Methods
//----------------------------------------------------------------------
bool QProgram::init() {
  QStatus status = QS_SUCCESS;

  if (!rt_->isValidDevice(dev_)) {
    LogErrorApi("Invalid Device: {}", dev_);
    return false;
  }

  //-----------------------------------------------------
  // Extract Network and Network Descriptor from QPC
  //-----------------------------------------------------
  uint32_t qpcMagic __attribute__((unused)) = AICQPC_MAGIC_NUMBER;
  if ((!programContainer_) && (programQpcUserData_.data != nullptr)) {
    programContainer_ = QProgramContainer::open(
        programQpcUserData_.data, programQpcUserData_.size, status);
  }

  if ((!programContainer_) && (programQpcUserData_.data == nullptr)) {
    LogErrorApi("Invalid QPC Pointer:");
    return false;
  }

  if (programContainer_->getBuffer(QProgramContainer::Network, networkData_) !=
      QS_SUCCESS) {
    LogErrorApi("Unable to retrieve network.bin in QPC");
    return false;
  }

  if (programContainer_->getBuffer(QProgramContainer::NetworkDescriptor,
                                   networkDescData_) != QS_SUCCESS) {
    LogErrorApi("Unable to retrieve networkdesc.bin in QPC");
    return false;
  }

  networkDescDataInit_ = networkDescData_;

  // Make a local copy of the Program as the caller is not expected to retain
  // the buffer past the creation of the program
  if ((programProperties_.selectMask &
       static_cast<uint32_t>(QAicProgramPropertiesBitfields::
                                 QAIC_PROGRAM_PROPERTIES_USE_APP_BUFFER)) ==
      0) {

    programBuffer_ = std::make_unique<uint8_t[]>(networkData_.size);
    if (programBuffer_ == nullptr) {
      LogErrorApi("Failed to allocate memory for QPC, size:{}",
                  networkData_.size);
      return false;
    }
    memcpy(programBuffer_.get(), networkData_.data, networkData_.size);
    networkData_.data = programBuffer_.get();
  }

  // We failed to find a ndescBuf. The code path for a non qpc null buffer
  // is an error so return a nullptr
  if (networkData_.data == nullptr) {
    LogWarnApi("QPC does not have a networkBuf image");
    return false;
  }

  std::stringstream is(
      std::string((char *)networkData_.data, networkData_.size));

  ELFIO::elfio elfReader;
  if (!elfReader.load(is)) {
    LogError("Failed to load/parse ELF data");
    return false;
  }

  if (elfReader.sections[metadata::networkElfMetadataFBSection]) {
    ELFIO::section *flatSec =
        elfReader.sections[metadata::networkElfMetadataFBSection];
    if (flatSec == nullptr) {
      LogError("Error parsing meta_flat");
      return false;
    }
    metadataBuffer_ = std::vector<uint8_t>(
        flatSec->get_data(), flatSec->get_data() + flatSec->get_size());
  } else {
    ELFIO::section *metaSec = elfReader.sections["metadata"];
    if (metaSec == nullptr) {
      LogError("Error parsing meta");
      return false;
    }
    metadataBuffer_ = std::vector<uint8_t>( metaSec->get_data(), metaSec->get_data() + metaSec->get_size());
  }

  metadataBufferInit_ = metadataBuffer_;
  metadataBufferRaw_ = metadataBuffer_;

  bool ret = updateInternalData();
  if (ret) {
    auto getBaseName = [](std::string wholeName) {
      return wholeName.substr(wholeName.find_last_of("/\\") + 1);
    };
    name_ = userName_ ? getBaseName(userName_)
                      : std::string("user") + "_" +
                            getBaseName(networkDesc_.network_name()) + "_" +
                            std::to_string(Id_);
  }
  return ret;
}

//
// Following procedure will be reused when network descriptor and metadata are
// updated
//
bool QProgram::updateInternalData() {
  QStatus status = QS_ERROR;
  // Skip for MQ program which does not have metadata
  if (!metadataBuffer_.empty()) {
    std::string metadataErrors;
    metadata_ = metadata::FlatDecode::readMetadataFlatNativeCPP(metadataBuffer_,
                                                                metadataErrors);
    if (metadata_ == nullptr) {
      std::cout << "metadata is null at the beginning" << std::endl;
    }
    if (!metadataErrors.empty()) {
      LogError("Error parsing metadata_flat {:s}", metadataErrors);
      return false;
    }
  }

  if (!networkDesc_.ParseFromArray(networkDescData_.data,
                                   networkDescData_.size)) {
    LogError("Failed to parse network desccriptor");
    return false;
  }

  if (networkDesc_.major_version() != AIC_NETWORK_DESCRIPTION_MAJOR_VERSION) {
    LogWarn("Incompatible Network Descriptor, library supports {}.{} "
             "program requires {}.{}",
             AIC_NETWORK_DESCRIPTION_MAJOR_VERSION,
             AIC_NETWORK_DESCRIPTION_MINOR_VERSION,
             networkDesc_.major_version(), networkDesc_.minor_version());
  }

  if (networkDesc_.minor_version() > AIC_NETWORK_DESCRIPTION_MINOR_VERSION) {
    LogWarn("Network Descriptor support features that may not be supported by "
            "this library, program network desc version {}.{}, library "
            "supports {}.{}",
            networkDesc_.major_version(), networkDesc_.minor_version(),
            AIC_NETWORK_DESCRIPTION_MAJOR_VERSION,
            AIC_NETWORK_DESCRIPTION_MINOR_VERSION);
  }

  if (!bindingsParser_.init(networkDesc_)) {
    LogErrorApi("Failed to initialize bindingsParser");
    return false;
  }

  status = bindingsParser_.getIoBufferInfo(bufferInfo_);
  if ((bufferInfo_ == nullptr) || (status != QS_SUCCESS)) {
    LogErrorApi("Failed to get IoBufferInfo");
    return false;
  }

  LogDebug("IO Buffer Info for program {}: \n{}", getProgramName(),
           strIoBufferInfo(bufferInfo_));

  status = bindingsParser_.getIoBufferInfoDma(bufferInfoDma_);
  if ((bufferInfoDma_ == nullptr) || (status != QS_SUCCESS)) {
    LogErrorApi("Failed to get IoBufferInfoDma");
    return false;
  }

  status = bindingsParser_.getIoDescriptorPb(ioDescPb_);
  if ((ioDescPb_ == nullptr) || (status != QS_SUCCESS)) {
    LogErrorApi("Failed to get IoDescriptor PB");
    return false;
  }

  try {
    ioDescPbBuffer_.resize(ioDescPb_->ByteSizeLong());
  } catch (const std::bad_alloc &ba) {
    LogErrorApi("Failed to resize vector for Protocol Buffer {}", ba.what());
    return false;
  }

  if (!ioDescPb_->SerializeToArray(ioDescPbBuffer_.data(),
                                   ioDescPbBuffer_.size())) {
    LogErrorApi("Failed to serialize Protocol Buffer for IO Descriptor");
    return false;
  }

  dmaBufferQDirections_.clear();
  for (auto &dmaBuf : networkDesc_.dma_buffers()) {
    if (dmaBuf.dir() == aicnwdesc::In) {
      dmaBufferQDirections_.push_back(QDirection::QDIR_TO_DEV);
    } else {
      dmaBufferQDirections_.push_back(QDirection::QDIR_FROM_DEV);
    }
  }

  userBufferQDirections_.clear();
  for (auto &userBuf : networkDesc_.inputs()) {
    userBufferQDirections_.push_back(QDirection::QDIR_TO_DEV);
    if (userBuf.is_partial_allowed()) {
      hasPartialTensor_ = true;
    }
  }

  for (auto i = 0; i < networkDesc_.outputs().size(); i++) {
    userBufferQDirections_.push_back(QDirection::QDIR_FROM_DEV);
  }
  return true;
}

// Private function protected by programMutex, no other mutex needed to access
QProgramDevice *QProgram::getProgramDevice() {
  std::unique_lock<std::mutex> lock(programDeviceInitMutex_);
  if (!programDevice_) {
    // Not found
    std::unique_ptr<QProgramDevice> progDev =
        QProgramDevice::Factory(context_, this, dev_);
    if (progDev) {
      programDevice_ = std::move(progDev);
      return programDevice_.get();
    } else {
      LogError("Failed to get program device");
      return nullptr;
    }
  } else {
    return programDevice_.get();
  }
}

const std::string
QProgram::strIoBindings(const aicapi::IoBinding *binding) const {
  std::stringstream ss;
  std::string dataTypeStr;
  if (binding == nullptr) {
    return "";
  }
  switch (binding->type()) {
  case aicapi::FLOAT_TYPE:
    dataTypeStr = "Float Type";
    break;
  case aicapi::FLOAT_16_TYPE:
    dataTypeStr = "Float 16 Type";
    break;
  case aicapi::INT8_Q_TYPE:
    dataTypeStr = "Int 8 Quant Type";
    break;
  case aicapi::UINT8_Q_TYPE:
    dataTypeStr = "UInt 8 Quant Type";
    break;
  case aicapi::INT16_Q_TYPE:
    dataTypeStr = "Int 16 Quant Type";
    break;
  case aicapi::INT32_Q_TYPE:
    dataTypeStr = "Int 32 Quant Type";
    break;
  case aicapi::INT32_I_TYPE:
    dataTypeStr = "Int 32 Index Type";
    break;
  case aicapi::INT64_I_TYPE:
    dataTypeStr = "Int 64 Index Type";
    break;
  case aicapi::INT8_TYPE:
    dataTypeStr = "Bool Type";
    break;
  default:
    dataTypeStr = "Unknown Type";
    break;
  }
  ss << fmt::format("  type:{:<17s}", dataTypeStr);
  ss << fmt::format("  quantized scale:{:<2}", binding->quant_scale());
  ss << fmt::format("  quantized offset:{:<2}", binding->quant_offset());
  ss << fmt::format("  dims size:{:<2d}", binding->dims().size());
  for (int32_t i = 0; i < binding->dims().size(); i++) {
    ss << fmt::format("  dims[{}]:{:<2d}", i, binding->dims(i));
  }
  return ss.str();
}

const std::string
QProgram::strBufferMappings(const QAicBufferMappings *mapping) const {
  std::stringstream ss;
  std::string ioTypeStr;
  switch (mapping->ioType) {
  case QAicBufferIoTypeEnum::BUFFER_IO_TYPE_INPUT:
    ioTypeStr = "input ";
    break;
  case QAicBufferIoTypeEnum::BUFFER_IO_TYPE_OUTPUT:
    ioTypeStr = "output";
    break;
  default:
    ioTypeStr = "unknown type";
    break;
  }
  ss << fmt::format("  number:{}", mapping->index);
  ss << fmt::format("  name:{:20s}", mapping->bufferName);
  ss << fmt::format("  type:{}", ioTypeStr);
  ss << fmt::format("  size:{}", mapping->size);
  ss << fmt::format("  partial:{}",
                    mapping->isPartialBufferAllowed ? "yes" : "no");

  return ss.str();
}

std::string QProgram::strIoBufferInfo(const QAicIoBufferInfo *ioBufferInfo) {
  std::stringstream ss;
  if (ioBufferInfo == nullptr) {
    ss << "Error, invalid ioDesc";
    return ss.str();
  }
  for (uint32_t i = 0; i < ioBufferInfo->numBufferMappings; i++) {
    ss << fmt::format("Buffer: index[{}]", i);
    ss << strBufferMappings(&ioBufferInfo->bufferMappings[i]) << std::endl;
  }
  return ss.str();
}

//
// Assign new metadata and parse it
//
QStatus QProgram::updateProgramData(const std::vector<uint8_t> &newMeta,
                                    const std::vector<uint8_t> &newNwDesc) {
  if (!newMeta.empty()) {
    metadataBuffer_ = newMeta;
    metadataBufferRaw_ = newMeta;
  }
  updatedNwDescData_ = newNwDesc;

  networkDescData_.data = updatedNwDescData_.data();
  networkDescData_.size = updatedNwDescData_.size();

  return updateInternalData() ? QS_SUCCESS : QS_ERROR;
}
} // namespace qaic
