// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "AICMetadataReader.h"

#include "assert.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"

static uint8_t *computeAddr(uint8_t *bufferBase, uint64_t bufferSize,
                            uint64_t offset) {
  assert(offset <= bufferSize);
  (void)bufferSize;
  return bufferBase + offset;
}

AICMetadata *MDR_readMetadata(void *buff, int buffSizeInBytes, char *errBuff,
                              int errBuffSz) {
  assert(buff && "Expected non-null buffer");
  assert(errBuff && errBuffSz && "Expected non-null error buffer");
  assert((size_t)buffSizeInBytes >= sizeof(AICMetadata));
  uint8_t *bufferBase = (uint8_t *)buff;

  AICMetadata *metadata = (AICMetadata *)buff;

  // Check metadata version.
  if (metadata->versionMajor != AIC_METADATA_MAJOR_VERSION ||
      metadata->versionMinor != AIC_METADATA_MINOR_VERSION) {
    snprintf(errBuff, errBuffSz,
             "Metadata versioning mismatch! Current: v%d.%d, Actual v%d.%d\n",
             AIC_METADATA_MAJOR_VERSION, AIC_METADATA_MINOR_VERSION,
             metadata->versionMajor, metadata->versionMinor);
    return NULL;
  }

  // Update the network name pointer.
  metadata->networkName = (char *)computeAddr(bufferBase, buffSizeInBytes,
                                              metadata->networkNameOffset);

  // Update the semaphore initial state pointer.
  metadata->semaphoreInitState = (uint32_t *)computeAddr(
      bufferBase, buffSizeInBytes, metadata->semaphoreInitStateOffset);

  // Update the L2TCM initial state pointer.
  metadata->L2TCMInitState = (void *)computeAddr(
      bufferBase, buffSizeInBytes, metadata->L2TCMInitStateOffset);

  // Update the dmaRequests pointers.
  metadata->dmaRequests = (AICMDDMARequest *)computeAddr(
      bufferBase, buffSizeInBytes, metadata->dmaRequestsOffset);
  for (int i = 0; i < metadata->numDMARequests; ++i) {
    AICMDDMARequest *dmaRequests = metadata->dmaRequests;
    dmaRequests[i].semaphoreOps = (AICMDSemaphoreOp *)computeAddr(
        bufferBase, buffSizeInBytes, dmaRequests[i].semaphoreOpsOffset);
    dmaRequests[i].doorbellOps = (AICMDDoorbellOp *)computeAddr(
        bufferBase, buffSizeInBytes, dmaRequests[i].doorbellOpsOffset);
  }

  // Update NSP multicast table pointers.
  metadata->nspMulticastTables = (AICMDNSPMulticastEntryTable *)computeAddr(
      bufferBase, buffSizeInBytes, metadata->nspMulticastTablesOffset);

  int numCores = metadata->numNSPs;
  AICMDNSPMulticastEntryTable *nspTable = metadata->nspMulticastTables;
  for (int core = 0; core < numCores; ++core) {
    nspTable[core].multicastEntries = (AICMDNSPMulticastEntry *)computeAddr(
        bufferBase, buffSizeInBytes, nspTable[core].multicastEntriesOffset);
  }

  // Update host multicast table pointers.
  AICMDHostMulticastEntryTable *hostTable =
      (AICMDHostMulticastEntryTable *)computeAddr(
          bufferBase, buffSizeInBytes, metadata->hostMulticastTableOffset);
  metadata->hostMulticastTable = hostTable;
  // Update the pointer to the host table entries.
  hostTable->multicastEntries = (AICMDHostMulticastEntry *)computeAddr(
      bufferBase, buffSizeInBytes, hostTable->multicastEntriesOffset);

  // Update thread descriptor pointers.
  metadata->threadDescriptors = (AICMDThreadDescriptor *)computeAddr(
      bufferBase, buffSizeInBytes, metadata->threadDescriptorsOffset);

  // Update constant mapping pointers.
  metadata->constantMappings = (AICMDConstantMapping *)computeAddr(
      bufferBase, buffSizeInBytes, metadata->constantMappingsOffset);
  return metadata;
}
