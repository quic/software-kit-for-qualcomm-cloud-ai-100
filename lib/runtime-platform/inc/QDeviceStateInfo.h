// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QDeviceStateInfo_H
#define QDeviceStateInfo_H

#include "QAicRuntimeTypes.h"
#include "QUtil.h"

#include <string>
#include <memory>

namespace qaic {

class QDeviceStateMonitor;

enum QDeviceStateInfoEvent {
  INVALID_EVENT,
  DEVICE_UP,
  DEVICE_DOWN,
  VC_UP,
  VC_DOWN,
  VC_UP_WAIT_TIMER_EXPIRY
};

class QDeviceStateInfo {

public:
  QDeviceStateInfo(QID qid, std::string deviceSBDF)
      : qid_(qid), deviceSBDF_(deviceSBDF), deviceEvent_(INVALID_EVENT),
        vcId_(qutil::INVALID_VCID) {}
  virtual ~QDeviceStateInfo() = default;

  QID qid() { return qid_; }
  std::string deviceSBDF() { return deviceSBDF_; }
  QDeviceStateInfoEvent deviceEvent() { return deviceEvent_; }
  uint32_t vcId() { return vcId_; }
  std::string deviceEventName();

private:
  void setDeviceEvent(QDeviceStateInfoEvent deviceEvent) {
    deviceEvent_ = deviceEvent;
  }
  void setVcId(uint32_t vcId) { vcId_ = vcId; }

  QID qid_;
  std::string deviceSBDF_;
  QDeviceStateInfoEvent deviceEvent_;
  uint32_t vcId_; // Value is valid for VC_UP/VC_DOWN event

  friend class QDeviceStateMonitor;
  friend std::shared_ptr<QDeviceStateInfo>
      getDeviceStateInfo(QDeviceStateInfoEvent, QID, uint32_t);
};

} // namespace qaic

#endif // QDeviceStateInfo_H