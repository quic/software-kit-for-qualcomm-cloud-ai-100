// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QAIC_OPENRT_VERSION_H
#define QAIC_OPENRT_VERSION_H

namespace qaic {
namespace openrt {

class VersionInfo {
public:
  static uint32_t minorVersion() { return minor_; }
  static uint32_t majorVersion() { return major_; }
  static uint32_t patchVersion() { return patch_; }

  static const char *variant() { return variant_; }

private:
  static constexpr int major_ = 1;
  static constexpr int minor_ = 0;
  static constexpr int patch_ = 0;
#if defined(DEBUG)
  static constexpr const char *variant_ = "DBG";
#else
  static constexpr const char *variant_ = "REL";
#endif
};

} // namespace qaic
} // namespace openrt
#endif // QAIC_OPENRT_VERSION_H
