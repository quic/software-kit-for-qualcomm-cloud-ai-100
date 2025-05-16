
// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QTYPES_H
#define QTYPES_H

#include <cinttypes>
#include <bitset>

namespace qaic {

inline constexpr uint32_t QDEVICE_FEATURE_MAX_BITS = 64;
using QVCID = uint8_t;
using QReqID = uint16_t;
using QAcResID = uint32_t;
using QDeviceFeatureBitset = std::bitset<QDEVICE_FEATURE_MAX_BITS>;

}; // namespace qaic

#endif // QITYPES