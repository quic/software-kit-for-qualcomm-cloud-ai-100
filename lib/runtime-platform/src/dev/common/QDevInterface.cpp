// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QDevInterface.h"
#include "QDevAic100Interface.h"

#include <string>
namespace qaic {

shQDevInterface QDevInterface::Factory(QDevInterfaceEnum qDevInterfaceEnum,
                                       QID qid, const std::string &devName) {
  shQDevInterface obj = nullptr;
  switch (qDevInterfaceEnum) {
  case QAIC_DEV_INTERFACE_AIC100: {
    obj = shQDevInterface(new (std::nothrow) QDevInterfaceAic100(qid, devName));
    if (!obj) {
      return nullptr;
    }
    obj->init();
  } break;
  default:
    break;
  }
  return obj;
}

} // namespace qaic
