// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QNN_IMAGE_H
#define QNN_IMAGE_H

#include <atomic>
#include <memory>
#include "QNNImageInterface.h"

namespace qaic {

class QDeviceInterface;
class QMetaDataInterface;

class QNNImage : public QNNImageInterface {
public:
  QNNImage() = delete;
  QNNImage(QDeviceInterface *dev, std::unique_ptr<QMetaDataInterface> meta,
           const QBuffer &buf, QID qid, QNNImageID imgID)
      : dev_(dev), buf_(buf), qid_(qid), imageID_(imgID), refCnt_(0),
        meta_(std::move(meta)) {}

  QNNImageID getImageID() const override { return imageID_; }

  QID getQID() const override { return qid_; }

  QStatus unload() override;

  QMetaDataInterface *getMetaData() const { return meta_.get(); }

  ~QNNImage() {}

private:
  QDeviceInterface *dev_;
  QBuffer buf_;
  QID qid_;
  QNNImageID imageID_;

  /// The ref count is increased every time the image is used for activation,
  /// and is descreased at deactivation.
  std::atomic<uint32_t> refCnt_;

  /// The metadata stored here is mostly a stencil of the request elements
  /// rendered from the Metadata from compiler. The request elements in this
  /// stencil are missing DDR base and MCID base addresses, which will be
  /// available at network activation. At network activation a new
  /// QMetaDataInterface
  /// object will be created as a clone of the one in QNNImage, but with
  /// DDR/MCID base addresses updated. This new QMetaDataInterface object is
  /// used to
  /// instantiate a QNeuralNetwork object which implements the data path
  /// interface.
  std::unique_ptr<QMetaDataInterface> meta_;

  void incRef() { refCnt_++; }

  void decRef() { refCnt_--; }

  friend class QRuntime;
  friend class QNeuralNetwork;
  friend class QNeuralnetwork;
};

} // namespace qaic

#endif // QNN_IMAGE_H
