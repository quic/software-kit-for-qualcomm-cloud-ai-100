// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
/** @file nnc_pmu_inf
 *  @brief NNC PMU Register Interface Header file
 */
#ifndef NNC_PMU_INF_H_
#define NNC_PMU_INF_H_

#define MAX_PMU_COUNTERS 8

/* PMU register definitions */
typedef enum {
  NNC_PMUCNT0 = 0,
  NNC_PMUCNT1 = 1,
  NNC_PMUCNT2 = 2,
  NNC_PMUCNT3 = 3,
  NNC_PMUCFG = 4,
  NNC_PMUEVTCFG = 5,
  NNC_PMUCNT4 = 6,
  NNC_PMUCNT5 = 7,
  NNC_PMUCNT6 = 8,
  NNC_PMUCNT7 = 9,
  NNC_PMUEVTCFG1 = 10,
  NNC_PMUSTID0 = 11,
  NNC_PMUSTID1 = 12,
} nnc_pmu_reg_ids;

/**
 * <!-- nnc_pmu_set -->
 *
 * @brief Function pointer for setting PMU registers
 *
 * @param reg_id : PMU register ID.
 * @param reg_value : New value to set the register to.
 *
 * @return void
 */
typedef void (*nnc_pmu_set)(int reg_id, unsigned int reg_value);
/**
 * <!-- nnc_pmu_get -->
 *
 * @brief Function pointer for getting PMU registers
 *
 * @param counters : List of PMU counters' values.
 *
 * @return void
 */
typedef void (*nnc_pmu_get)(uint32_t counters[MAX_PMU_COUNTERS]);

#endif /* NNC_PMU_INF_H_ */
