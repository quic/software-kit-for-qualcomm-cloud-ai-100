// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
#ifndef NNC_PORT_INF_H_
#define NNC_PORT_INF_H_

#include <stdint.h>

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define sizeof_member(type, member) sizeof(((type *)0)->member)
#define s_assert(exp, msg) _Static_assert(exp, msg)
#else
#define sizeof_member(type, member)
#define s_assert(exp, msg)
#endif

/* Port direction */
typedef enum {
  AIC_PORT_IN = 0,
  AIC_PORT_OUT = 1,
} AICPortDir;

/* Port type */
typedef enum {
  AIC_PORT_HOST = 0,
  AIC_PORT_P2P = 1,
  AIC_PORT_DRT = 2,
} AICPortType;

/* semAddress and dbAddress signify input/output based
 * on port direction
 */
typedef struct {
  /* VA of GSM space to perform semaphore operation */
  union {
    uint64_t semAddressOffset;
    uint8_t *semAddress;
  };
  /* VA of the DB location */
  uint32_t dbAddress;
  /* Virtual Semaphore Number */
  uint16_t semNum;
  /* AICPortDir */
  uint16_t portDir;
  /* AICPortType */
  uint16_t portType;
  uint8_t padding[6];
} AICPortInfo;

// Assert that we don't have any implicit padding.
s_assert(sizeof(AICPortInfo) ==
             sizeof_member(AICPortInfo, semAddressOffset) +
                 sizeof_member(AICPortInfo, dbAddress) +
                 sizeof_member(AICPortInfo, semNum) +
                 sizeof_member(AICPortInfo, portDir) +
                 sizeof_member(AICPortInfo, portType) +
                 sizeof_member(AICPortInfo, padding),
         "AICPortInfo has implicit padding.");

typedef struct {
  uint32_t numEntry;
  uint32_t padding;
  union {
    uint64_t portInfoListOffset;
    AICPortInfo *portInfoList;
  };
} AICPortInfoList;

// Assert that we don't have any implicit padding.
s_assert(sizeof(AICPortInfoList) ==
             sizeof_member(AICPortInfoList, numEntry) +
                 sizeof_member(AICPortInfoList, portInfoListOffset) +
                 sizeof_member(AICPortInfoList, padding),
         "AICPortInfoList has implicit padding.");

typedef struct {
  uint32_t numEntry;
  uint32_t padding;
  union {
    uint64_t portInfoIndexListOffset;
    AICPortInfoList *portInfoIndexList;
  };
} AICPortInfoIndexList;

// Assert that we don't have any implicit padding.
s_assert(sizeof(AICPortInfoIndexList) ==
             sizeof_member(AICPortInfoIndexList, numEntry) +
                 sizeof_member(AICPortInfoIndexList, portInfoIndexListOffset) +
                 sizeof_member(AICPortInfoIndexList, padding),
         "AICPortInfoIndexList has implicit padding.");

/**
  @ingroup func_dma_setup
  Function to increment TP in the DMA bridge

  In case of Multi-AIC, NSP FW drives the P2P VC channel to send output on
  VC channel allocated for the output port. The output port is choosen based
  on index parameter (topology index). So, When network calls this function,
  depending on the index (topology index) parameter, will drive the VC channel.
  Note:
  1) This function is called by network at the beginnig of each inference to
     drive the output via VC channel.
  2) Network should call this function only once per inference.
  3) This call is non-blocking call and should take minimal possible time to
     drive the VC.

  @param[in]  topology index

  @return
  0 - success
  -1 - failure

  @dependencies
  None.
*/

typedef int (*nnc_dma_setup)(uint32_t index);

#endif
