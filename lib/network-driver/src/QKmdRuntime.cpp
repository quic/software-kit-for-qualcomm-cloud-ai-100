// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QKmdRuntime.h"
#include "QOsal.h"
#include "QImageParser.h"
#include "QRuntime.h"
#include "QRuntimePlatformKmdDeviceFactory.h"
#include "QUtil.h"
#include "QKmdDeviceFactory.h"
#include "QKmdVCFactory.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdio.h>
#include <string.h>
#include <vector>

extern "C" {
#include <pci/pci.h>
}

namespace qaic {

//
// Get runtime for Kernel mode driver
//
std::unique_ptr<QRuntimeInterface> getKmdRuntime() {

  auto vcFactory = std::make_shared<QKmdVCFactory>();
  if (vcFactory == nullptr) {
    return nullptr;
  }

  QOsal::initPlatform();

  DevList devList;
  QOsal::enumAicDevices(devList);

  UdevMap udevMap;
  QOsal::createUdevMap(udevMap);

  auto devFactory = std::unique_ptr<QDeviceFactoryInterface>(
      new QKmdDeviceFactory(vcFactory, devList, udevMap));
  if (devFactory == nullptr) {
    return nullptr;
  }
  if (devFactory->initDevices() != QS_SUCCESS) {
    return nullptr;
  }

  std::unique_ptr<QImageParserInterface> imgParser =
      std::make_unique<QImageParser>();
  if (imgParser == nullptr) {
    return nullptr;
  }

  return std::make_unique<QRuntime>(std::move(devFactory),
                                    std::move(imgParser));
}

} // namespace qaic
