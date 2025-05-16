// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
#ifndef NNC_REPROG_MCID_INF_H_
#define NNC_REPROG_MCID_INF_H_

typedef enum {
  NNC_ADDR_SPACE_TYPE_L2TCM = 0,
  NNC_ADDR_SPACE_TYPE_VTCM = 1
} nnc_addresss_space_type;

/**
  @ingroup func_nnc_reprog_mcid
  NNC MCID Reprogramming
   Reprogram the MCID to different port mask and base address offset.
  @param[in] MCID to be reprogrammed
  @param[in] 4K aligned base address offset (-1 : Skip programming base address
  offset in slave)
  @param[in] port mask to be reprogrammed (mask 0 : Slave, non-zero : Mask and
  base address offset)
  @param[in] addr_type (0- L2TCM, 1- VTCM)

  @return
  0 on success
  -1 on failure/timeout in programming MCID

  @dependencies
  None.
*/
typedef int (*nnc_reprog_mcid_fp)(uint16_t mcid, uint64_t base_addr_off,
                                  uint32_t mask,
                                  nnc_addresss_space_type addr_type);

#endif /* NNC_REPROG_MCID_INF_H_ */
