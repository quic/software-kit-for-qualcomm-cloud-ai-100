// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QPROGRAM_INFO_H
#define QPROGRAM_INFO_H

namespace qaic {

enum ProgramState {
  STATE_PROGRAM_CREATED = 0,
  STATE_PROGRAM_LOADED,
  STATE_PROGRAM_READY,
  STATE_PROGRAM_LOAD_ERROR,
  STATE_PROGRAM_ACTIVATE_ERROR,
  STATE_PROGRAM_DEVICE_ERROR,
  STATE_PROGRAM_TRANSITION
};

struct QProgramInfo {
  ProgramState state;
};

} // namespace qaic

#endif // QPROGRAM_INFO_H