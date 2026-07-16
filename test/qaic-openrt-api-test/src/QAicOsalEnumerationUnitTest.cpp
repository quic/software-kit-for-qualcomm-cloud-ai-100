// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include <cstdint>

#include <gtest/gtest.h>

#include "QOsal.h"

// Unit tests for qaic::QOsal::isSupportedAicDeviceForEnumeration(), the PCI
// device-id allow-list applied while enumerating physical Cloud AI devices
// (QOsal::enumAicDevices). These are pure-logic checks and require no hardware.

namespace {

TEST(QOsalEnumerationTest, SupportedDeviceIdsAreAccepted) {
  EXPECT_TRUE(qaic::QOsal::isSupportedAicDeviceForEnumeration(0xA080)); // AI80
  EXPECT_TRUE(qaic::QOsal::isSupportedAicDeviceForEnumeration(0xA100)); // AI100
  EXPECT_TRUE(qaic::QOsal::isSupportedAicDeviceForEnumeration(0xA110)); // AI200
}

TEST(QOsalEnumerationTest, UnsupportedDeviceIdsAreRejected) {
  // Exact-match allow-list: neighbors of the supported ids and clearly-invalid
  // values must be rejected (guards against an accidental range/mask match).
  EXPECT_FALSE(qaic::QOsal::isSupportedAicDeviceForEnumeration(0x0000));
  EXPECT_FALSE(qaic::QOsal::isSupportedAicDeviceForEnumeration(0xA07F));
  EXPECT_FALSE(qaic::QOsal::isSupportedAicDeviceForEnumeration(0xA081));
  EXPECT_FALSE(qaic::QOsal::isSupportedAicDeviceForEnumeration(0xA0FF));
  EXPECT_FALSE(qaic::QOsal::isSupportedAicDeviceForEnumeration(0xA101));
  EXPECT_FALSE(qaic::QOsal::isSupportedAicDeviceForEnumeration(0xA111));
  EXPECT_FALSE(qaic::QOsal::isSupportedAicDeviceForEnumeration(0xFFFF));
}

} // namespace
