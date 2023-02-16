// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QDeviceStateInfo.h"

namespace qaic {

std::string QDeviceStateInfo::deviceEventName() {
  std::string eventName;
  switch (deviceEvent_) {
  case DEVICE_UP:
    eventName = "DEVICE_UP";
    break;
  case DEVICE_DOWN:
    eventName = "DEVICE_DOWN";
    break;
  case VC_UP:
    eventName = "VC_UP";
    break;
  case VC_DOWN:
    eventName = "VC_DOWN";
    break;
  case VC_UP_WAIT_TIMER_EXPIRY:
    eventName = "VC_UP_WAIT_TIMER_EXPIRY";
    break;
  default:
    eventName = "INVALID_EVENT";
    break;
  }
  return eventName;
}

} // namespace qaic
