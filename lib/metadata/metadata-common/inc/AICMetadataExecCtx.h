// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef AIC_METADATA_CTX
#define AIC_METADATA_CTX

#include "nnc_err_inf.h"
#include "nnc_mmap_inf.h"
#include "nnc_pmu_inf.h"
#include "nnc_port_inf.h"
#include "nnc_reprog_mcid_inf.h"
#include "nnc_rtld_inf.h"
#include "nnc_udma_inf.h"
#include "nnc_ulog_inf.h"

#include <stdint.h>

typedef void (*nnc_exit_fp)();

/// Data provided by runtime environment to executable upon startup.
/// The "DirectApi" information is guidance for how to initialize as argument to
/// Thread_Execute.
typedef struct AICExecContext_ {

  /// ExecCtx Major and Minor Version for version compliance check
  uint16_t execContextMajorVersion;
  uint16_t execContextMinorVersion;

  uint8_t virtualNSPId;

  /// L2TCM base virtual address
  uint8_t *baseL2TCM;
  ///< DirectApi: -aic-l2tcm-size DDR alloc\n
  ///<  r/w cacheable align=4kB CCCC=0x7\n
  ///<  AICMetadata.L2TCMSize\n
  ///<  Persistent across activations. Current recommendation is 512kB though
  ///<  this may change.

  /// VTCM base virtual address
  uint8_t *baseVTCM;
  ///< r/w uncached align=2048 CCCC=0x6\n
  ///< AICMetadata.VTCMSize\n
  ///< DirectApi: All networks MUST map to the start of VTCM.
  ///<  Any carve-out for CV MUST be at the top end of VTCM space.
  ///<  This requirement derives from HMX HW requirements related to TLB entry
  ///<  crossings.

  /// Constant data base virtual address
  uint8_t *baseConstantDataMem;
  ///< Base address for statically mapped constant weights memory block.
  ///< Includes constants and static placeholders.\n
  ///< r/o uncached align=2048 CCCC=0x6

  /// Shared DDR data base virtual address
  uint8_t *baseSharedDDR;
  ///< Mutable weights memory block, Inputs and Outputs as well
  ///< as intermediate activations that don't end up in VTCM.\n
  ///< r/w uncached align=2048 CCCC=0x6

  /// L2 cacheable only base virtual address
  uint8_t *baseL2CachedDDR;
  ///< cccc=0xf (L1 uncached, L2 cached WB). size=4kB\n
  ///< The first 64B cache line will be locked in the L2 to guarantee latency.

  /// Virtual address GSM semaphore commands are to be written to
  uint8_t *gsmSemAddr;
  ///< DirectApi: nullptr (still used in native flow)

  /// Pointer to array of virtual addresses to use for multicasts.
  void **mcAddresses;
  ///< Indexed by virtual multicast id.\n
  ///< DirectApi: nullptr

  uint64_t startTimeStamp;
  ///< DirectApi: startTimeStamp=0\n
  ///< NOTE: Firmware needs to sync UTIMER with system TOD

  /// Log function pointer. Should be set appropriately for each path.
  nnc_log_fp logFuncPtr;
  /// Function used to exit thread execution
  nnc_exit_fp exitThread;
  ///< See 'exitDoorbellOffset description.\n
  ///< DirectApi: nullptr

  /// Function for setting PMU specific registers
  nnc_pmu_set setPMUReg;
  /// Error handling function
  nnc_err_fatal_fp errFuncPtr;
  /// Report network hang
  nnc_notify_hang_fp notifyHangPtr;
  /// UDMA register reader function
  nnc_udma_read_fp udmaReadFuncPtr;
  /// Memory Map functions
  nnc_mmap_fp mmapFuncPtr;
  ///< DirectApi: nullptr
  nnc_munmap_fp munmapFuncPtr;
  ///< DirectApi: nullptr

  /// Network QDSS STM Port virtual address
  uint8_t *qdssSTMPortVaddr;

  /// Function to get all PMU count registers
  nnc_pmu_get readPMUCnt;

  /// DDR Bandwidth Monitor virtual address
  uint8_t *ddrBWMonRegVaddr;

  /// start of topology index table
  AICPortInfoIndexList *port_info_index;

  /// Function to handle DMA bridge functionality
  nnc_dma_setup dmaSetupFuncPtr;

  /// Network heap
  uint8_t *networkHeapAddr;
  uint64_t networkHeapSize;

  /// Function to support MCID reprogramming
  nnc_reprog_mcid_fp reprogMcidFuncPtr;

  /// RTLD APIs
  /// Note - these are added in 1.8 SDK WITHOUT bumping the ExecCtx version
  /// number, because bumping the number would break 1.7 compatibility, while
  /// adding these APIs here doesn't break 1.7 compatibility. The only current
  /// user of these APIs is QNN-HTP, which won't be advertized as functional
  /// until 1.8 SDK or later so version number checking shouldn't be a problem.
  dlopen_fp dlOpenPtr;
  dlopenbuf_fp dlOpenbufPtr;
  dlclose_fp dlClosePtr;
  dlsym_fp dlSymPtr;
  dladdr_fp dlAddrPtr;
  dlerror_fp dlErrorPtr;
  dlinfo_fp dlInfoPtr;

} AICExecContext;

#endif // AIC_METADATA_CTX
