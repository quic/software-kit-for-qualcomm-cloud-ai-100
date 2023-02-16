// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QACTIVATION_STATE_CMD_H
#define QACTIVATION_STATE_CMD_H

namespace qaic {

enum QActivationStateType {
  ACTIVATION_STATE_CMD_INIT = 0,
  ACTIVATION_STATE_CMD_STANDBY = 1,
  ACTIVATION_STATE_CMD_READY = 2,
  ACTIVATION_STATE_CMD_DEACTIVATE = 3,
  ACTIVATION_STATE_CMD_TYPE_MAX = 4,

};

}; // namespace qaic

#endif // QIACTIVATION_STATE_CMD_H
