// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "AICMetadata.h"

#include "assert.h"
#include "stdio.h"

static void dumpSemaphoreOp(int idx, const AICMDSemaphoreOp *s) {
  char *opStr = "unknown";
  switch (s->semOp) {
  default:
    assert(0 && "Unknown semaphore operation.");
    break;
  case AICMDSemaphoreCmdNOP:
    opStr = "nop";
    break;
  case AICMDSemaphoreCmdINIT:
    opStr = "init";
    break;
  case AICMDSemaphoreCmdINC:
    opStr = "inc";
    break;
  case AICMDSemaphoreCmdDEC:
    opStr = "dec";
    break;
  case AICMDSemaphoreCmdWAITEQ:
    opStr = "waiteq";
    break;
  case AICMDSemaphoreCmdWAITGE:
    opStr = "waitge";
    break;
  case AICMDSemaphoreCmdP:
    opStr = "waitgezerodec";
    break;
  }
  printf("  semaphoreOp[%d] - semOp: %6s semNum: %d semValue: %d", idx, opStr,
         s->semNum, s->semValue);
  char *preOrPostStr = "unknown";
  switch (s->preOrPost) {
  default:
    assert(0 && "Unknown semaphore pre/post.");
    break;
  case AICMDSemaphoreSyncPost:
    preOrPostStr = "post";
    break;
  case AICMDSemaphoreSyncPre:
    preOrPostStr = "pre";
    break;
  }
  printf(" preOrPost: %s", preOrPostStr);
  printf(" inSyncFence: %d outSyncFence: %d\n", s->inSyncFence,
         s->outSyncFence);
}

static void dumpDoorbellOp(int idx, const AICMDDoorbellOp *d) {
  printf("  doorbellOp[%d]  - size: %d mcId: %d offset: %lu data: %d\n", idx,
         d->size, d->mcId, d->offset, d->data);
}

static void dumpDMARequest(int idx, const AICMDDMARequest *r) {
  printf(" DMA Request[%d] -", idx);
  printf(" num: % d", r->num);
  printf(" hostOffset: %lu ", r->hostOffset);
  char *devAddrSpaceStr = "unknown";
  switch (r->devAddrSpace) {
  default:
    assert(0 && "Unknown DMA address space.");
    break;
  case AICMDDMAAddrSpaceMC:
    devAddrSpaceStr = "MC";
    break;
  case AICMDDMAAddrSpaceDDR:
    devAddrSpaceStr = "DDR";
    break;
  }
  printf(" devAddrSpace: %s", devAddrSpaceStr);
  printf(" devOffset: %lu", r->devOffset);
  printf(" size: %d", r->size);
  printf(" mcId: %d", r->mcId);
  char *inOutStr = "unknown";
  switch (r->inOut) {
  default:
    assert(0 && "Unknown DMA in/out param.");
    break;
  case AICMDDMAIn:
    inOutStr = "in";
    break;
  case AICMDDMAOut:
    inOutStr = "out";
    break;
  }
  printf(" inOut: %s", inOutStr);
  printf(" portId: %d\n", r->portId);

  for (int i = 0; i < r->numSemaphoreOps; ++i)
    dumpSemaphoreOp(i, &r->semaphoreOps[i]);
  for (int i = 0; i < r->numDoorbellOps; ++i)
    dumpDoorbellOp(i, &r->doorbellOps[i]);
}

static void dumpSemaphoreInitState(const AICMetadata *m) {
  for (int i = 0, e = m->numSemaphores; i < e; ++i)
    printf("    %3d: 0x%08x\n", i, m->semaphoreInitState[i]);
}

static void dumpL2TCMInitState(const AICMetadata *m) {
  printf(" ");
  for (int i = 0, e = m->L2TCMInitSize / sizeof(uint32_t); i != e; ++i) {
    if (i && (i % 8 == 0))
      printf("\n ");
    printf(" %08x", ((uint32_t *)m->L2TCMInitState)[i]);
  }
  printf("\n");
}

static void dumpDMARequests(const AICMetadata *m) {
  for (int i = 0; i < m->numDMARequests; ++i) {
    printf("\n");
    dumpDMARequest(i, &m->dmaRequests[i]);
  }
}

static void dumpNSPMulticastTable(const AICMetadata *m, int core) {
  assert(core < 16 && "Invalid core."); // FIXME: magic number.
  printf(" nspMulticastTable[%d]:\n", core);

  AICMDNSPMulticastEntryTable *table = &m->nspMulticastTables[core];
  assert(table && "Invalid table");

  printf("    numMulticastEntries: %d\n", table->numMulticastEntries);
  for (int mcid = 0, e = table->numMulticastEntries; mcid != e; ++mcid) {
    AICMDNSPMulticastEntry *entry = &table->multicastEntries[mcid];
    assert(entry && "Invalid entry.");
    printf("    MCID: %2d mask: %4x size: %8d dynamic: %d addrSpace: ", mcid,
           entry->mask, entry->size, entry->dynamic);
    char *addrSpaceStr = "unknown";
    switch (entry->addrSpace) {
    default:
      assert(0 && "Unknown DMA address space.");
      break;
    case AICMDAddrSpaceL2TCM:
      addrSpaceStr = "L2TCM";
      break;
    case AICMDAddrSpaceVTCM:
      addrSpaceStr = " VTCM";
      break;
    }
    printf("%s baseAddrOffset: %10lu\n", addrSpaceStr, entry->baseAddrOffset);
  }
}

static void dumpHostMulticastTable(const AICMetadata *m) {
  printf("  hostMulticastTable:\n");
  AICMDHostMulticastEntryTable *table = m->hostMulticastTable;
  printf("    numMulticastEntries: %d\n", table->numMulticastEntries);
  for (int mcid = 0, e = table->numMulticastEntries; mcid != e; ++mcid) {
    AICMDHostMulticastEntry *entry = &table->multicastEntries[mcid];
    assert(entry && "Invalid entry.");
    printf("    MCID: %2d mask: %4x size: %8d\n", mcid, entry->mask,
           entry->size);
  }
}

static void dumpThreadDescriptors(const AICMetadata *m) {
  printf("\nThread Descriptors:\n");
  printf(" numThreadDescriptors: %d\n", m->numThreadDescriptors);
  for (int td = 0, tde = m->numThreadDescriptors; td < tde; ++td) {
    const AICMDThreadDescriptor *desc = &m->threadDescriptors[td];

    char *typeStr = "none";
    if ((desc->typeMask & AICMDThreadHMX) && (desc->typeMask & AICMDThreadHVX))
      typeStr = "hmx+hvx";
    else if (desc->typeMask & AICMDThreadHMX)
      typeStr = "hmx";
    else if (desc->typeMask & AICMDThreadHVX)
      typeStr = "hvx";

    printf("  ThreadDesc[%d]: entryPoint 0x%p typeMask: %7s\n", td,
           desc->entryPoint, typeStr);
  }
}

static void dumpConstantMappings(const AICMetadata *m) {
  printf("\nConstant Mappings:\n");
  printf(" numConstantMappings: %d\n", m->numConstantMappings);
  for (int cm = 0, cme = m->numConstantMappings; cm < cme; ++cm) {
    const AICMDConstantMapping *mapping = &m->constantMappings[cm];
    printf("  ConstantMapping[%d]: mask: %x constantDataBaseOffset: %lu size: "
           "%d\n",
           cm, mapping->coreMask, mapping->constantDataBaseOffset,
           mapping->size);
  }
}

static void dump(const AICMetadata *m) {
  printf("AIC Hardware Version v%d.%d\n", m->hwVersionMajor, m->hwVersionMinor);
  printf("AIC Metadata Version v%d.%d\n", m->versionMajor, m->versionMinor);
  printf("AIC ExecContext Major Version v%d\n", m->execContextMajorVersion);
  printf(" numNSPs: %d\n", m->numNSPs);
  printf(" VTCMSize: %d\n", m->VTCMSize);
  printf(" L2TCMSize: %d\n", m->L2TCMSize);
  printf(" singleVTCMPage: %d\n", m->singleVTCMPage);
  printf(" networkName: %s\n", m->networkName);
  printf(" numSemaphores: %d\n", m->numSemaphores);
  printf("  semaphore initial state:\n");
  dumpSemaphoreInitState(m);

  printf(" L2TCMInitSize: %d\n", m->L2TCMInitSize);
  printf("  L2TCM initial state:\n");
  dumpL2TCMInitState(m);

  printf("\n StaticSharedDDRSize: %lu\n", m->staticSharedDDRSize);
  printf(" StaticSharedDDRECCEnabled: %u\n", m->staticSharedDDRECCEnabled);
  printf(" DynamicSharedDDRSize: %lu\n", m->dynamicSharedDDRSize);
  printf(" DynamicSharedDDRECCEnabled: %u\n", m->dynamicSharedDDRECCEnabled);
  printf(" Total shared DDR size: %lu\n",
         m->staticSharedDDRSize + m->dynamicSharedDDRSize);

  printf("\n StaticConstantsSize: %lu\n", m->staticConstantsSize);
  printf(" StaticConstantsECCEnabled: %u\n", m->staticConstantsECCEnabled);
  printf(" DynamicConstantsSize: %lu\n", m->dynamicConstantsSize);
  printf(" DynamicConstantsECCEnabled: %u\n", m->dynamicConstantsECCEnabled);
  printf(" Total constants size: %lu\n",
         m->staticConstantsSize + m->dynamicConstantsSize);

  printf("\n NetworkHeapSize: %lu\n", m->networkHeapSize);

  // DMA requests
  printf("\nDMARequests:\n");
  printf(" numDMARequests: %d\n", m->numDMARequests);
  dumpDMARequests(m);

  // NSP multicast tables.
  printf("\nMulticast Tables:\n");
  for (int core = 0; core < m->numNSPs; ++core)
    dumpNSPMulticastTable(m, core);

  // Host multicast table.
  dumpHostMulticastTable(m);

  // Thread descriptors.
  dumpThreadDescriptors(m);

  // Constant mappings.
  dumpConstantMappings(m);

  printf("\n exitDoorbellOffset: %ld\n", m->exitDoorbellOffset);

  // FP instructions.
  printf("\nhasHvxFP: %s\n", m->hasHvxFP ? "true" : "false");
  printf("hasHmxFP: %s\n", m->hasHmxFP ? "true" : "false");
}

void AICMetadata_dump(const AICMetadata *MD) { dump(MD); }
