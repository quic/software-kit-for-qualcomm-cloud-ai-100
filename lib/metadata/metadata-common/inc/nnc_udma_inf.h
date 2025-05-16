// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
/** @file nnc_udma_inf
 *  @brief NNC UDMA DM Register Interface Header file
 */
#ifndef NNC_UDMA_INF_H_
#define NNC_UDMA_INF_H_

/* UDMA DM registers */
typedef enum {
  NNC_UDMA_DM0 = 0,
  NNC_UDMA_DM1 = 1,
  NNC_UDMA_DM2 = 2,
  NNC_UDMA_DM3 = 3,
  NNC_UDMA_DM4 = 4,
  NNC_UDMA_DM5 = 5,
} nnc_udma_reg_ids;

/**
 * <!-- nnc_udma_read -->
 *
 * @brief Function pointer for reading UDMA DM registers
 *
 * @param reg_id : UDMA register ID.
 * @param reg_value : Return register value.
 *
 * @return void
 */
typedef void (*nnc_udma_read_fp)(unsigned int reg_id, unsigned int *val);

#endif /* NNC_UDMA_INF_H_ */
