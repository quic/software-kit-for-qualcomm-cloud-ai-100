// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QPROGRAM_TYPES_H
#define QPROGRAM_TYPES_H

namespace qaic {
class QProgram;

enum QProgramActivationCmd {
  QProgram_CMD_ACTIVATE_FULL = 0,   /// Command to fully activate
  QProgram_CMD_DEACTIVATE_FULL = 1, /// Command to fully deactivate
  QIPROGRRAM_CMD_INVALID = 2
};

} // namespace qaic

#endif