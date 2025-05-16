// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QProgramCONTAINER_H
#define QProgramCONTAINER_H

#include <unordered_map>

#include "QAicRuntimeTypes.h"
#include "QUtil.h"
#include "QAic.h"
#include "QAicQpc.h"
#include "AICNetworkDesc.pb.h"
#include "QLogger.h"
#include "QBindingsParser.h"

namespace qaic {

class QProgramContainer;
class QDeviceImage;
class QDeviceImageCommon;
using shQProgramContainer = std::shared_ptr<QProgramContainer>;
using shQDeviceImage = std::shared_ptr<QDeviceImage>;
using shQDeviceImageCommon = std::shared_ptr<QDeviceImageCommon>;

enum QDeviceImageType {
  ProgramImage = 0x01,
  ConstantsImage = 0x02,
  AllImages = 0x03,
};

class QDeviceImageCommon : virtual public QLogger {
public:
  static shQDeviceImageCommon Factory(QID device,
                                      QProgramContainer *programContainer,
                                      QDeviceImageType = ConstantsImage);
  virtual ~QDeviceImageCommon();
  QNNConstantsInterface *getConstants();
  bool isLoaded() { return true; }

  QDeviceImageCommon(const QDeviceImageCommon &) =
      delete; // Disable copy constructor
  QDeviceImageCommon &
  operator=(const QDeviceImageCommon &) = delete; // Disable assignment operator

private:
  // Private Constructor for factory pattern
  QDeviceImageCommon(QID device, QProgramContainer *programContainer,
                     QDeviceImageType select);

  QStatus load(QProgramContainer *programContainer);
  QStatus unload();

  QID device_;
  QDeviceImageType imageSelect_;
  QRuntimeInterface *rt_;
  QNNConstantsInterface *constDesc_;
  QNNConstantsInterface *const_;
};

class QDeviceImage : virtual public QLogger {
public:
  static shQDeviceImage Factory(QID device, QProgramContainer *programContainer,
                                shQDeviceImageCommon shCommonImage,
                                QDeviceImageType = AllImages);

  virtual ~QDeviceImage();
  QNNImageInterface *getNNImage();
  QNNConstantsInterface *getConstants();
  bool isLoaded() { return true; }
  QDeviceImage(const QDeviceImage &) = delete; // Disable copy constructor
  QDeviceImage &
  operator=(const QDeviceImage &) = delete; // Disable assignment operator

private:
  // Private Constructor for factory pattern
  QDeviceImage(QID device, QProgramContainer *programContainer,
               shQDeviceImageCommon shCommonImage, QDeviceImageType select);
  QStatus load(QProgramContainer *programContainer);
  QStatus unload();
  QID device_;
  QDeviceImageType imageSelect_;
  QRuntimeInterface *rt_;
  shQDeviceImageCommon commonImage_;
  QNNImageInterface *networkImage_;
};

class QProgramContainer {
public:
  enum containerElements {
    ConstantsDescriptor = 0,
    Constants,
    NetworkDescriptor,
    Network,
    ConstantsCRC,
    MetadataCRC,
    MetaInfo,
  };
  static shQProgramContainer open(const uint8_t *qpcBuf, size_t qpcSize,
                                  QStatus &status);
  static QStatus close(shQProgramContainer &programContainer);

  QProgramContainer(const uint8_t *qpcBuf, size_t qpcSize);
  virtual ~QProgramContainer();
  bool isMq() { return metaInfoBuf_.valid; };
  const shQpcInfo &getInfo();
  QStatus getDeviceImage(QID device, shQDeviceImage &loadObj,
                         QDeviceImageType imageSelect = AllImages);
  QStatus getCommonDeviceImage(QID device, shQDeviceImageCommon &commonImage,
                               QDeviceImageType imageSelect = AllImages);
  QStatus getIoDescriptor(QData *ioDescData);

  bool contains(containerElements elem);

  const char *getQpcBuffer();
  size_t getQpcSize() const;

  QStatus getBuffer(containerElements elem, QBuffer &qdata);
  QStatus getBuffer(containerElements elem, QData &qdata);
  aicnwdesc::networkDescriptor &getNetworkDescriptor() { return networkDesc_; }
  void getConstantsBuffer(QBuffer &staticCompileTimeConstBuf,
                          QBuffer &dynamicCompileTimeConstBuf);
  QStatus getBufferByName(const std::string &name, QData &qdata);

  QProgramContainer(const QProgramContainer &) =
      delete; // Disable Copy Constructor
  QProgramContainer &
  operator=(const QProgramContainer &) = delete; // Disable Assignment Operator

private:
  struct ContainerBuffer {
    QBuffer qb;
    bool valid;
    ContainerBuffer() : valid(false) { qutil::initQBuffer(qb); }
  };
  QStatus init();
  std::unordered_map<QID, shQDeviceImageCommon> commonImageLoadedMap_;
  shQpcInfo qpcInfo_;
  std::vector<QAicQpcProgramInfo> programInfoList_;
  std::vector<QAicQpcConstantsInfo> constantsInfoList_;
  const uint8_t *qpcBufIn_;
  size_t qpcSize_;
  std::unique_ptr<uint8_t[]> qpcBuf_;
  ContainerBuffer constDescBuf_;
  ContainerBuffer staticCompileTimeConstBuf_;
  ContainerBuffer dynamicCompileTimeConstBuf_;
  ContainerBuffer networkDescBuf_;
  QBindingsParser bindingsParser_;
  aicapi::IoDesc *ioDescPb_;
  std::vector<uint8_t> ioDescPbBuffer_;
  ContainerBuffer progBuf_;
  ContainerBuffer metadataCRCBuf_;
  ContainerBuffer constantsCRCBuf_;
  ContainerBuffer metaInfoBuf_;
  QAicQpcHandle *qpcHandle_;
  aicnwdesc::networkDescriptor networkDesc_;
  std::mutex commonImageLoadedMapMutex_;
  static constexpr const char constantsDescSegmentName_[] = "constantsdesc.bin";
  static constexpr const char constantsSegmentName_[] = "constants.bin";
  static constexpr const char networkDescSegmentName_[] = "networkdesc.bin";
  static constexpr const char networkSegmentName_[] = "network.elf";
  static constexpr const char constantsCRCSegmentName_[] = "metadata.crc";
  static constexpr const char metadataCRCSegmentName_[] = "constants.bin.crc";
  static constexpr const char metaInfoSegmentName_[] = "mdp_meta_info";
};

} // namespace qaic

#endif // QProgramContainer_H
