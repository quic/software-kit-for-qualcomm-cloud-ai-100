// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QIAIC_H
#define QIAIC_H

#include <memory>
#include <vector>
#include <cstdint>

#include "QLogger.h"
#include "QNNConstantsInterface.h"
#include "QNeuralNetworkInterface.h"
#include "QRuntimeInterface.h"
#include "QTypes.h"
#include "QActivationStateCmd.h"
#include "QDeviceStateInfo.h"

#define LRT_LIB_MAJOR_VERSION 10
#define LRT_LIB_MINOR_VERSION 0

namespace qaic {

class QContextInterface;
class QProgram;
class QProgramGroup;
class QIConstants;
class QIQueue;
class QIEvent;
class QExecObj;
class QContext;

using shQProgram = std::shared_ptr<QProgram>;
using shQProgramGroup = std::shared_ptr<QProgramGroup>;
using shQIConstants = std::shared_ptr<QIConstants>;
using shQIQueue = std::shared_ptr<QIQueue>;
using shQIEvent = std::shared_ptr<QIEvent>;
using shQExecObj = std::shared_ptr<QExecObj>;
using shQContext = std::shared_ptr<QContext>;

using NotifyDeviceStateInfoPriority = uint32_t;
using NotifyDeviceStateInfoFuncCb =
    std::function<void(std::shared_ptr<qaic::QDeviceStateInfo> event)>;
using NotifyDeviceStateInfoPriorityDbEntry =
    std::pair<NotifyDeviceStateInfoPriority,
              std::pair<void *, NotifyDeviceStateInfoFuncCb>>;
} // namespace qaic

#endif // QIAIC_H
