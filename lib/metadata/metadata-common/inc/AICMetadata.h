// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef AIC_METADATA_LIB
#define AIC_METADATA_LIB

#include <stdint.h>

#define AIC_METADATA_MAJOR_VERSION 1
#define AIC_METADATA_MINOR_VERSION 0
#define AIC_METADATA_EXEC_CTX_MAJOR_VERSION 4
#define AIC_METADATA_EXEC_CTX_MINOR_VERSION 0

#define AIC_METADATA_PORTID_BASE 100
// AIC_METADATA_PORTID_BASE and AIC_METADATA_PORTID_BASE + 1 are
// reserved for host input/output.
#define AIC_METADATA_PORTID_HOST_INPUT 100
#define AIC_METADATA_PORTID_HOST_OUTPUT 101

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define sizeof_member(type, member) sizeof(((type *)0)->member)
#define s_assert(exp, msg) _Static_assert(exp, msg)
#else
#define sizeof_member(type, member)
#define s_assert(exp, msg)
#endif

// Activate function
typedef void (*nnc_activate_fp)(void *ctx, uint8_t virtualThreadId,
                                uint32_t stid);

// AIC hardware versions.
typedef enum {
  AIC_HW_VER_1_0 = (1 << 16) | 0,
  AIC_HW_VER_2_0 = (2 << 16) | 0
} AICHardwareVersion;

// Semaphore operation opcodes.
typedef enum {
  AICMDSemaphoreCmdNOP = 0,    // nop.
  AICMDSemaphoreCmdINIT = 1,   // Set sem to val.
  AICMDSemaphoreCmdINC = 2,    // Increment sem.
  AICMDSemaphoreCmdDEC = 3,    // Decrement sem.
  AICMDSemaphoreCmdWAITEQ = 4, // Wait for sem to be == val.
  AICMDSemaphoreCmdWAITGE = 5, // Wait for sem to be >= val.
  AICMDSemaphoreCmdP = 6,      // Wait for sem to be > 0 and decrement.
} AICMDSemaphoreOpcode;

typedef enum {
  AICMDSemaphoreSyncPost = 0,
  AICMDSemaphoreSyncPre = 1,
} AICMDSemaphoreSync;

typedef struct {
  uint16_t semNum;   // Which semaphore does this operation use?
  uint16_t semValue; // Command immediate value (e.g., value to store)
  // Is this semaphore operation done before or after the DMA data
  // transfer?
  uint8_t semOp;     // AICMDSemaphoreOpcode
  uint8_t preOrPost; // AICMDSemaphoreSync: Wait for all previous inbound
                     // transfers to complete before beginning processing
                     // this transfer.
  uint8_t inSyncFence;
  // Wait for all previous outbound transfers to complete before
  // beginning processing this transfer.
  uint8_t outSyncFence;
} AICMDSemaphoreOp;

// Assert that we don't have any implicit padding.
s_assert(sizeof(AICMDSemaphoreOp) ==
             sizeof_member(AICMDSemaphoreOp, semNum) +
                 sizeof_member(AICMDSemaphoreOp, semValue) +
                 sizeof_member(AICMDSemaphoreOp, semOp) +
                 sizeof_member(AICMDSemaphoreOp, preOrPost) +
                 sizeof_member(AICMDSemaphoreOp, inSyncFence) +
                 sizeof_member(AICMDSemaphoreOp, outSyncFence),
         "AICMDNSPMulticastEntryTable has implicit padding.");

// Size of data to be written to doorbell
typedef enum {
  AICMDDoorballOpSize8,
  AICMDDoorballOpSize16,
  AICMDDoorballOpSize32
} AICMDDoorbellOpSize;

typedef struct {
  uint64_t offset; // base address offset to data start
  uint32_t data;   // value to be written to doorbell
  uint16_t mcId;   // reference to host multicast table entry
  uint8_t size;    // AICMDDoorbellOpSize
  uint8_t padding[1];
} AICMDDoorbellOp;

// Assert that we don't have any implicit padding.
s_assert(sizeof(AICMDDoorbellOp) ==
             sizeof_member(AICMDDoorbellOp, offset) +
                 sizeof_member(AICMDDoorbellOp, data) +
                 sizeof_member(AICMDDoorbellOp, mcId) +
                 sizeof_member(AICMDDoorbellOp, size) +
                 sizeof_member(AICMDDoorbellOp, padding),
         "AICMDDoorbellOp has implicit padding.");

typedef enum {
  AICMDDMAAddrSpaceMC,  // Multicast address
  AICMDDMAAddrSpaceDDR, // DDR virtual address
} AICMDDMAEntryAddrSpace;

typedef enum {
  AICMDDMAIn = 0,
  AICMDDMAOut = 1,
} AICMDDMADirection;

// Request elements are processed as follows:
//  1. sync fence
//  2. pre-semaphores
//  3. data transfer
//  4. post-semaphores
//  5. doorbells
//
// DMA requests that use different semaphores and that don't use
// inSyncFence or outSyncFence may be processed in any order.
//
typedef struct {
  // AICMDSemaphoreOp[]
  union {
    uint64_t semaphoreOpsOffset;
    AICMDSemaphoreOp *semaphoreOps;
  };

  // AICMDDoorbellOp[]
  union {
    uint64_t doorbellOpsOffset;
    AICMDDoorbellOp *doorbellOps;
  };

  uint64_t hostOffset; // offset into host-side buffer to data start
  uint64_t devOffset;  // offset into device-side buffer to data start
  uint32_t size;       // size of data to be transferred
  uint16_t num;  // input number that corresponds to host buffer argument list
                 // position
  uint16_t mcId; // Multicast ID; only used if devAddrSpace ==
                 // AICMDDMAAddrSpaceMC
  uint8_t numSemaphoreOps;
  uint8_t numDoorbellOps;
  uint8_t devAddrSpace; // AICMDDMAEntryAddrSpace
  uint8_t inOut;        // AICMDDMADirection: inbound or outbound (from AIC
                        // device perspective).
  uint16_t portId;      // Each DMA buffer is assigned a portId which is used to
                        // identify a source or destination endpoint in a
                        // topology involving multiple sub-networks. Source and
                        // destination endpoints from the same sub-network have
                        // different portIds. portIds for endpoints in a network
                        // start from AIC_METADATA_PORTID_BASE.
  uint8_t padding[2];
} AICMDDMARequest;

// Assert that we don't have any implicit padding.
s_assert(sizeof(AICMDDMARequest) ==
             sizeof_member(AICMDDMARequest, semaphoreOpsOffset) +
                 sizeof_member(AICMDDMARequest, doorbellOpsOffset) +
                 sizeof_member(AICMDDMARequest, hostOffset) +
                 sizeof_member(AICMDDMARequest, devOffset) +
                 sizeof_member(AICMDDMARequest, size) +
                 sizeof_member(AICMDDMARequest, num) +
                 sizeof_member(AICMDDMARequest, mcId) +
                 sizeof_member(AICMDDMARequest, numSemaphoreOps) +
                 sizeof_member(AICMDDMARequest, numDoorbellOps) +
                 sizeof_member(AICMDDMARequest, devAddrSpace) +
                 sizeof_member(AICMDDMARequest, inOut) +
                 sizeof_member(AICMDDMARequest, portId) +
                 sizeof_member(AICMDDMARequest, padding),
         "AICMDDMARequest has implicit padding");

// L2TCM/VTCM
typedef enum {
  AICMDAddrSpaceL2TCM,
  AICMDAddrSpaceVTCM,
} AICMDMulticastEntryAddrSpace;

// NSP multicast entry.
typedef struct {
  uint64_t baseAddrOffset; // Offset from the VTCM/L2TCM base address.  The
                           // offset should be 4k aligned.
  uint32_t mask; // Bitvector used by the sender to specify the NSP virtual ids
                 // of the receivers.
  uint32_t size; // Size of region targeted using this multicast id.
  uint8_t addrSpace; // AICMDMulticastEntryAddrSpace: Specifies base
                     // address, vTCM or L2TCM, used by the receiver.
  uint8_t dynamic;   // When set indicates entry is dynamically mapped.
  uint8_t padding[6];
} AICMDNSPMulticastEntry;

// Assert that we don't have any implicit padding.
s_assert(sizeof(AICMDNSPMulticastEntry) ==
             sizeof_member(AICMDNSPMulticastEntry, baseAddrOffset) +
                 sizeof_member(AICMDNSPMulticastEntry, mask) +
                 sizeof_member(AICMDNSPMulticastEntry, size) +
                 sizeof_member(AICMDNSPMulticastEntry, addrSpace) +
                 sizeof_member(AICMDNSPMulticastEntry, dynamic) +
                 sizeof_member(AICMDNSPMulticastEntry, padding),
         "AICMDNSPMulticastEntry has implicit padding");

// NSP multicast table.
typedef struct {
  union {
    uint64_t multicastEntriesOffset;
    AICMDNSPMulticastEntry *multicastEntries;
  };
  uint16_t numMulticastEntries;
  uint8_t padding[6];
} AICMDNSPMulticastEntryTable;

// Assert that we don't have any implicit padding.
s_assert(sizeof(AICMDNSPMulticastEntryTable) ==
             sizeof_member(AICMDNSPMulticastEntryTable,
                           multicastEntriesOffset) +
                 sizeof_member(AICMDNSPMulticastEntryTable,
                               numMulticastEntries) +
                 sizeof_member(AICMDNSPMulticastEntryTable, padding),
         "AICMDNSPMulticastEntryTable has implicit padding");

// Host multicast entry. Host entries are special because the host can only send
// multicasts, not receive.
typedef struct {
  uint32_t mask; // Bitvector used by host specify the NSP virtual ids of the
                 // receivers.
  uint32_t size; // Size of region targeted using this multicast id.
} AICMDHostMulticastEntry;

// Assert that we don't have any implicit padding.
s_assert(sizeof(AICMDHostMulticastEntry) ==
             sizeof_member(AICMDHostMulticastEntry, mask) +
                 sizeof_member(AICMDHostMulticastEntry, size),
         "AICMDHostMulticastEntry has implicit padding");

// Host multicast table.
typedef struct {
  union {
    uint64_t multicastEntriesOffset;
    AICMDHostMulticastEntry *multicastEntries;
  };
  uint16_t numMulticastEntries;
  uint8_t padding[6];
} AICMDHostMulticastEntryTable;

// Assert that we don't have any implicit padding.
s_assert(sizeof(AICMDHostMulticastEntryTable) ==
             sizeof_member(AICMDHostMulticastEntryTable,
                           multicastEntriesOffset) +
                 sizeof_member(AICMDHostMulticastEntryTable,
                               numMulticastEntries) +
                 sizeof_member(AICMDHostMulticastEntryTable, padding),
         "AICMDHostMulticastEntryTable has implicit padding");

typedef enum {
  AICMDThreadHMX = (1 << 0),
  AICMDThreadHVX = (1 << 1),
} AICMDThreadType;

// Description of threads to be setup and where to transfer control for each
// thread upon start.
typedef struct {
  // This 'entryPoint' function expects to be passed a const pointer to a
  // AICExecContext via the standard ABI mechanism:
  //   void entryPoint(const AICExecContext*, uint8_t virtualThreadId,
  //                   uint32_t stid);
  union {
    nnc_activate_fp entryPoint;
    uint64_t entryPointPadding;
  };

  uint8_t typeMask; // AICMDThreadType mask
  // Note: For v68 we need HVX threads to be divided evenly between HW threads
  // 0,1,2 and 3,4,5. This will be handled by the QuRT scheduler.
  uint8_t padding[7];
} AICMDThreadDescriptor;

// Assert that we don't have any implicit padding.
s_assert(sizeof(AICMDThreadDescriptor) ==
             sizeof_member(AICMDThreadDescriptor, entryPointPadding) +
                 sizeof_member(AICMDThreadDescriptor, typeMask) +
                 sizeof_member(AICMDThreadDescriptor, padding),
         "AICMDThreadDescriptor has implicit padding");

// Map constant data of a given size, starting at a given base offset from the
// provided constant data buffer to the virtual memory range starting at
// baseAddress for virtual NSPs set to 1 in coreMask.
//
// Notes:
// * The shared constant buffer is loaded into DDR once prior to activation.
// * At activation a handle to this shared constant buffer is provided in
// addition to the network image.
// * A very common case is that the data will be shared between all NSPs and
// thus the coreMask will be all 1s.
typedef struct {
  uint64_t
      constantDataBaseOffset; // offset from constant data buffer base (EA =
                              // constantDataBase + constantDataBaseoffset)
  uint32_t coreMask;
  uint32_t size;
} AICMDConstantMapping;

// Assert that we don't have any implicit padding.
s_assert(sizeof(AICMDConstantMapping) ==
             sizeof_member(AICMDConstantMapping, constantDataBaseOffset) +
                 sizeof_member(AICMDConstantMapping, coreMask) +
                 sizeof_member(AICMDConstantMapping, size),
         "AICMDConstantMapping has implicit padding");

// Data provided to the runtime environment to configure network image execution
// environment.
typedef struct {
  // Metadata versioning - The metadata version number is used by the runtime to
  // verify metadata binary/semantic compatibility.  The semantics of the
  // metadata can change as a result of how the compiler/runtime interprets the
  // metadata.
  union {
    struct {
      uint16_t versionMajor;
      uint16_t versionMinor;
    };
    uint64_t versionPadding;
  };

  // Use ONNX/whatever metadata domain, version, graph name to encode unique
  // name for AIC runtime.
  union {
    uint64_t networkNameOffset;
    char *networkName;
  };

  // Init state of semaphores.  Number of elements in array == numSemaphores.
  union {
    uint64_t semaphoreInitStateOffset;
    uint32_t *semaphoreInitState;
  };

  // Before any NSP is activated, the L2TCM of all NSPs must be initialized to
  // some known state.  This buffer contains the initial values that need to be
  // copied into each NSPs L2TCM.  The size of the buffer == L2TCMInitSize.
  union {
    uint64_t L2TCMInitStateOffset;
    void *L2TCMInitState;
  };

  // AICMDDMARequest[]
  union {
    uint64_t dmaRequestsOffset;
    AICMDDMARequest *dmaRequests;
  };

  // AICMDNSPMulticastEntryTable[]: One AICMDNSPMulticastEntryTable per
  // NSP.
  union {
    uint64_t nspMulticastTablesOffset;
    AICMDNSPMulticastEntryTable *nspMulticastTables;
  };

  // AICMDHostMulticastEntryTable[]: Host-side multicast table.
  union {
    uint64_t hostMulticastTableOffset;
    AICMDHostMulticastEntryTable *hostMulticastTable;
  };

  // AICMDThreadDescriptor[]
  union {
    uint64_t threadDescriptorsOffset;
    AICMDThreadDescriptor *threadDescriptors;
  };

  // AICMDConstantMapping[]: This table is used to specify the static mapping of
  // the constant shared buffer into the NSPs virtual address space. There is a
  // single mapping for each NSP. Each entry in the table can map one or more
  // NSPs based on the mask bits. The common case will be a single table entry
  // that maps all NSPs. If multiple entries exist the mask bit for any NSP must
  // not be set in more than one entry (otherwise, the NSP has multiple
  // mappings). These regions are uncached and read-only. The firmware provides
  // the base address of the region in the execution context. This region needs
  // to be aligned to 256 bytes.
  union {
    uint64_t constantMappingsOffset;
    AICMDConstantMapping *constantMappings;
  };

  // The staticSharedDDRSize specifies the size of a statically mapped shared
  // region of DDR memory. The dynamicSharedDDRSize specifies the size of a
  // dynamically mapped shared region of DDR memory. Thus, the total size of the
  // shared DDR region is staticSharedDDRSize + dynamicSharedDDRSize. This
  // region is uncached and readable/writable by all NSPs. The firmware provides
  // the base address of the statically mapped region in the execution context.
  // The entire region needs to be aligned to 2048 bytes. All other writable
  // memory regions specified in the network ELF (i.e.  .data/.bss/ etc.) are
  // not shared across NSPs.  A separate copy of these data sections is
  // allocated for each NSP the network is activated on.
  uint64_t staticSharedDDRSize;
  uint64_t dynamicSharedDDRSize;

  // The staticConstantsSize specifies the size of the statically mapped region
  // of the constant shared buffer. The dynamicConstantsSize specifies the size
  // of the dynamically mapped region of the constant shared buffer. Thus, the
  // total size of the constant shared buffer is staticConstantsSize +
  // dynamicConstantsSize. The constants are laid out in the input buffer such
  // that the first constantsSize bytes are statically mapped (as defined by the
  // constantMappings). The remaining dynamicConstantsSize bytes are dynamically
  // mapped by the compiler.
  uint64_t staticConstantsSize;
  uint64_t dynamicConstantsSize;

  // Exit doorbell offset: This is an offset into the network's L2TCM memory
  // that is used to signal the network image that it should terminate
  // execution.  The offset refers to one byte of memory, that when set to 1
  // signals the threads of the network image that they should exit at the next
  // synchronization point.  The network image threads exit by means of calling
  // the 'exitThread' function passed in via the AICExecContext.
  uint64_t exitDoorbellOffset;

  // This is a tiny DDR region used a workaround for NSP pause errata.
  // https://qti-jira.qualcomm.com/jira/browse/QDSP-31616
  // It is mapped as cccc=0xf (L1 uncached, L2 cached WB).
  // The first 64B cache line will be locked in the L2 to guarantee latency.
  uint32_t l2CachedDDRSize;

  // Describes the size of the initialization data for L2TCM.  Irrespective of
  // this size, the assumption is that all of L2TCM is made available for the
  // network to use.  The L2TCM allocation needs to be aligned to 4k bytes.
  // NOTE: Slave port writes to the range [L2TCM_Base, L2TCM_Base+64kB) will
  // cause a wake from pause event as programmed by root-image.
  // (Event_Reg_High/Lo)
  uint32_t L2TCMInitSize;

  // Execution context version - This allows LRT/QSM not to have to change when
  // a new OS interface function is added.
  uint16_t execContextMajorVersion;

  // Number of NSPs network is compiled for.
  uint16_t numNSPs;

  // Number of virtual semaphores 0..N-1.  The translation from virtual to
  // physical semaphore number is done by the GSM.
  uint16_t numSemaphores;

  uint16_t numDMARequests;
  uint16_t numThreadDescriptors;
  uint16_t numConstantMappings;

  // Hardware version.
  uint8_t hwVersionMajor;
  uint8_t hwVersionMinor;

  // Is hardware ECC enabled?
  uint8_t staticSharedDDRECCEnabled;
  uint8_t dynamicSharedDDRECCEnabled;
  uint8_t staticConstantsECCEnabled;
  uint8_t dynamicConstantsECCEnabled;

  // When non-zero the network has been compiled using a single VTCM page.  When
  // zero the network has been compiled using 2 VTCM pages.
  uint8_t singleVTCMPage;

  // Indicate the presence of FP instructions
  uint8_t hasHvxFP;
  uint8_t hasHmxFP;

  uint8_t padding[3];

  // VTCM size required for this network.
  uint32_t VTCMSize;

  // Describes the size of the L2TCM.
  uint32_t L2TCMSize;

  // Size of the network heap
  uint64_t networkHeapSize;

} AICMetadata;

// Assert that we don't have any implicit padding.
s_assert(sizeof(AICMetadata) ==
             sizeof_member(AICMetadata, versionPadding) +
                 sizeof_member(AICMetadata, networkNameOffset) +
                 sizeof_member(AICMetadata, semaphoreInitStateOffset) +
                 sizeof_member(AICMetadata, L2TCMInitStateOffset) +
                 sizeof_member(AICMetadata, dmaRequestsOffset) +
                 sizeof_member(AICMetadata, nspMulticastTablesOffset) +
                 sizeof_member(AICMetadata, hostMulticastTableOffset) +
                 sizeof_member(AICMetadata, threadDescriptorsOffset) +
                 sizeof_member(AICMetadata, constantMappingsOffset) +
                 sizeof_member(AICMetadata, staticSharedDDRSize) +
                 sizeof_member(AICMetadata, dynamicSharedDDRSize) +
                 sizeof_member(AICMetadata, staticConstantsSize) +
                 sizeof_member(AICMetadata, dynamicConstantsSize) +
                 sizeof_member(AICMetadata, exitDoorbellOffset) +
                 sizeof_member(AICMetadata, l2CachedDDRSize) +
                 sizeof_member(AICMetadata, execContextMajorVersion) +
                 sizeof_member(AICMetadata, numNSPs) +
                 sizeof_member(AICMetadata, numSemaphores) +
                 sizeof_member(AICMetadata, L2TCMInitSize) +
                 sizeof_member(AICMetadata, numDMARequests) +
                 sizeof_member(AICMetadata, numThreadDescriptors) +
                 sizeof_member(AICMetadata, numConstantMappings) +
                 sizeof_member(AICMetadata, hwVersionMajor) +
                 sizeof_member(AICMetadata, hwVersionMinor) +
                 sizeof_member(AICMetadata, staticSharedDDRECCEnabled) +
                 sizeof_member(AICMetadata, dynamicSharedDDRECCEnabled) +
                 sizeof_member(AICMetadata, staticConstantsECCEnabled) +
                 sizeof_member(AICMetadata, dynamicConstantsECCEnabled) +
                 sizeof_member(AICMetadata, singleVTCMPage) +
                 sizeof_member(AICMetadata, hasHvxFP) +
                 sizeof_member(AICMetadata, hasHmxFP) +
                 sizeof_member(AICMetadata, padding) +
                 sizeof_member(AICMetadata, VTCMSize) +
                 sizeof_member(AICMetadata, L2TCMSize) +
                 sizeof_member(AICMetadata, networkHeapSize),
         "AICMetadata has implicit padding");

typedef struct {
  uint64_t staticConstantsSize;
  uint64_t dynamicConstantsSize;
  uint8_t staticConstantsECCEnabled;
  uint8_t dynamicConstantsECCEnabled;
  uint8_t padding[6];
} AICConstantDescriptor;

// Assert that we don't have any implicit padding.
s_assert(sizeof(AICConstantDescriptor) ==
             sizeof_member(AICConstantDescriptor, staticConstantsSize) +
                 sizeof_member(AICConstantDescriptor, dynamicConstantsSize) +
                 sizeof_member(AICConstantDescriptor,
                               staticConstantsECCEnabled) +
                 sizeof_member(AICConstantDescriptor,
                               dynamicConstantsECCEnabled) +
                 sizeof_member(AICConstantDescriptor, padding),
         "AICConstantDescriptor has implicit padding");

#undef sizeof_member
#undef s_assert

#ifdef __cplusplus
extern "C" {
#endif
void AICMetadata_dump(const AICMetadata *MD);
#ifdef __cplusplus
} // End of extern "C"
#endif

#endif // AIC_METADATA
