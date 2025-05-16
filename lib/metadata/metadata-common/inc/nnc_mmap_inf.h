// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
/** @file nnc_mmap
 *  @brief NNC Memory Map Interface Header file
 */
#ifndef NNC_MMAP_INF_H_
#define NNC_MMAP_INF_H_

typedef enum {
  NNC_MMAP_ADDR_TYPE_CONSTANT = 0,
  NNC_MMAP_ADDR_TYPE_DDR = 1,
  NNC_MMAP_ADDR_TYPE_MC = 2,
} nnc_network_address_type;

/**
  @ingroup func_nnc_mmap
  NNC Memory Mapping
   Default algorithm is optimizing on TLB, will achieve less wastage if
   size is close to TLB sizes.
  @param[in,out] Pointer to the va address
  @param[in] 64 bit offset (16 MB aligned offset will give max performance, in
  terms of TLB used)
  @param[in] Size to allocate
  @param[in] addr_type (0- constant, 2- shared DDR, 3- multicast)
  @param[in,out] Pointer to index, value ret can be used to free memory

  @return
  0 on success
  -1 on failure

  @dependencies
  None.
*/
typedef int (*nnc_mmap_fp)(void **va, uint64_t offset, uint32_t size,
                           nnc_network_address_type addr_type,
                           uint32_t *node_index);

/**
  @ingroup func_nnc_munmap
  NNC Memory Un map
  @param[in]  Index to free memory
  @return
  None

  @dependencies
  None.
*/

typedef void (*nnc_munmap_fp)(uint32_t node_index);

#endif /* NNC_MMAP_INF_H_ */
