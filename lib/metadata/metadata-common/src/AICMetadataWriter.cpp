// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "AICMetadataWriter.h"

#include "assert.h"
#include "string.h" // memset
#include <stdlib.h>

static uint64_t alignTo(uint64_t x, uint64_t m) { return (x + m - 1) / m * m; }

AICMetadataWriter::AICMetadataWriter(uint16_t hwVersionMajor,
                                     uint16_t hwVersionMinor) {
  memset(&metadata_, 0, sizeof(AICMetadata));
  metadata_.versionMajor = AIC_METADATA_MAJOR_VERSION;
  metadata_.versionMinor = AIC_METADATA_MINOR_VERSION;
  metadata_.execContextMajorVersion = AIC_METADATA_EXEC_CTX_MAJOR_VERSION;
  metadata_.hwVersionMajor = hwVersionMajor;
  metadata_.hwVersionMinor = hwVersionMinor;
  metadata_.exitDoorbellOffset = ~(uint64_t)0;
}

template <typename T>
uint32_t AICMetadataWriter::write(const T *src, int elts) {
  int offset = metadataBuffer.size();
  offset = alignTo(offset, alignof(T));
  size_t bytes = elts * sizeof(T);
  int newSize = offset + bytes;
  metadataBuffer.resize(newSize);
  memcpy(&metadataBuffer[offset], src, bytes);
  return offset;
}

template <typename T>
void AICMetadataWriter::write(const T *src, int elts, uint32_t offset) {
  size_t bytes = elts * sizeof(T);
  assert((offset + bytes) < metadataBuffer.size());
  assert(alignTo(offset, alignof(T)) == offset);
  memcpy(&metadataBuffer[offset], src, bytes);
}

void AICMetadataWriter::finiSemaphoreState() {
  metadata_.semaphoreInitStateOffset =
      !semaphoreInitState.empty()
          ? write(&semaphoreInitState[0], semaphoreInitState.size())
          : 0;
}

void AICMetadataWriter::finiL2TCMState() {
  metadata_.L2TCMInitSize = L2TCMInitState.size();
  metadata_.L2TCMInitStateOffset =
      !L2TCMInitState.empty() ? write(&L2TCMInitState[0], L2TCMInitState.size())
                              : 0;
}

void AICMetadataWriter::finiDMARequests() {
  int numDMARequests = DMARequests.size();

  // Write semaphore/doorbell operations.
  for (int i = 0; i < numDMARequests; ++i) {
    SemaphoreOps &semaphoreOps = DMARequests[i].semaphoreOps;
    DMARequests[i].request.numSemaphoreOps = semaphoreOps.size();
    DMARequests[i].request.semaphoreOpsOffset =
        !semaphoreOps.empty() ? write(&semaphoreOps[0], semaphoreOps.size())
                              : 0;

    DoorbellOps &doorbellOps = DMARequests[i].doorbellOps;
    DMARequests[i].request.numDoorbellOps = doorbellOps.size();
    DMARequests[i].request.doorbellOpsOffset =
        !doorbellOps.empty() ? write(&doorbellOps[0], doorbellOps.size()) : 0;
  }

  // Write DMA requests.
  metadata_.numDMARequests = numDMARequests;
  for (int i = 0; i < numDMARequests; ++i) {
    int offset = write(&DMARequests[i].request, 1);
    if (i == 0)
      metadata_.dmaRequestsOffset = offset;
  }
}

void AICMetadataWriter::finiMulticastTables() {
  // Write the entries.
  std::vector<uint32_t> tableOffsets;
  for (int i = 0, e = getNumNSPs(); i < e; ++i)
    tableOffsets.push_back(
        write(&NSPMulticastTables[i][0], NSPMulticastTables[i].size()));

  // Write NSP multicast table entries.
  AICMDNSPMulticastEntryTable tmpTable;
  memset(&tmpTable, 0, sizeof(AICMDNSPMulticastEntryTable));
  for (int i = 0, e = getNumNSPs(); i < e; ++i) {
    // Write the table.
    tmpTable.numMulticastEntries = NSPMulticastTables[i].size();
    tmpTable.multicastEntriesOffset = tableOffsets[i];
    int offset = write(&tmpTable, 1);
    if (i == 0)
      metadata_.nspMulticastTablesOffset = offset;
  }

  // Write host multicast table.
  AICMDHostMulticastEntryTable hostTable;
  memset(&hostTable, 0, sizeof(AICMDHostMulticastEntryTable));
  hostTable.numMulticastEntries = HostMulticastTable.size();
  hostTable.multicastEntriesOffset =
      write(&HostMulticastTable[0], HostMulticastTable.size());
  metadata_.hostMulticastTableOffset = write(&hostTable, 1);
}

void AICMetadataWriter::finiThreadDescriptors() {
  int numThreads = ThreadDescriptors.size();
  metadata_.numThreadDescriptors = numThreads;
  if (numThreads)
    metadata_.threadDescriptorsOffset =
        write(&ThreadDescriptors[0], numThreads);
}

void AICMetadataWriter::finiConstantMappings() {
  int numMappings = ConstantMappings.size();
  metadata_.numConstantMappings = numMappings;
  metadata_.constantMappingsOffset =
      !ConstantMappings.empty() ? write(&ConstantMappings[0], numMappings) : 0;

  // Clear the mapped cores.
  constMappingCores = 0;
}

void AICMetadataWriter::finalize() {
  assert(getNumNSPs() && "Must set numNSPs");
  assert(getVTCMSize() && "Must set VTCMSize");
  assert(getL2TCMSize() && "Must set L2TCMSize");

  // FIXME: Reserve metadataBuffer space to avoid extra memory allocation
  // overhead.

  // Write base metadata.  The various offsets are unknown at this point, so
  // we'll have to come back and fix them up after the fact.
  write(&metadata_, 1);

  // Write network name.
  if (networkName.empty())
    networkName = "unnamed";
  metadata_.networkNameOffset =
      write(networkName.c_str(), networkName.length() + 1);

  // Write the semaphore initial state buffer.
  finiSemaphoreState();

  // Write the L2TCM initial state buffer.
  finiL2TCMState();

  // Write DMA requests.
  finiDMARequests();

  // Write multicast tables.
  finiMulticastTables();

  // Write thread descriptors.
  finiThreadDescriptors();

  // Write constant mappings.
  finiConstantMappings();

  // Fixup the offsets by writing the updated metadata.
  write(&metadata_, 1, /*offset=*/0);

  assert(getL2TCMInitSize() <= getL2TCMSize());
  assert(metadata_.exitDoorbellOffset < (uint64_t)getL2TCMInitSize());

  isFinal = true;
}

bool AICMetadataWriter::writeMetadata(const char *outfile) const {
  assert(isFinal && "Metadata not finalized.");
  assert(getNumNSPs() && "Must set numNSPs");
  assert(getVTCMSize() && "Must set VTCMSize");
  assert(getL2TCMSize() && "Must set L2TCMSize");
  assert(outfile && "null outfile");

  FILE *fd = fopen(outfile, "wb");
  if (fd == nullptr) {
    assert(!"writeMetadata: File open failed");
    return false;
  }
  size_t bytes = metadataBuffer.size();
  if (fwrite(metadataBuffer.data(), 1, bytes, fd) != bytes) {
    assert(!"writeMetadata: fwrite failed");
    fclose(fd);
    return false;
  }
  if (fclose(fd) != 0) {
    assert(!"writeMetadata: fclose failed");
    return false;
  }
  return true;
}

void AICMetadataWriter::addSemaphoreOp(SemaphoreOps &semaphoreOps,
                                       uint8_t semOp, uint16_t semNum,
                                       uint16_t semValue, uint8_t preOrPost,
                                       uint8_t inSyncFence,
                                       uint8_t outSyncFence) {
  bool validSemaphoreOp;
  switch (semOp) {
  default:
    validSemaphoreOp = false;
    break;
  case AICMDSemaphoreCmdNOP:    // nop.
  case AICMDSemaphoreCmdINIT:   // Set sem to val.
  case AICMDSemaphoreCmdINC:    // Increment sem.
  case AICMDSemaphoreCmdDEC:    // Decrement sem.
  case AICMDSemaphoreCmdWAITEQ: // Wait for sem to be == val.
  case AICMDSemaphoreCmdWAITGE: // Wait for sem to be >= val.
  case AICMDSemaphoreCmdP:      // Wait for sem to be > 0 and decrement.
    validSemaphoreOp = true;
    break;
  }
  assert(validSemaphoreOp && "Invalid DMA semaphore operation");
  (void)validSemaphoreOp;

  SemaphoreOp semaphoreOp;
  memset(&semaphoreOp, 0, sizeof(SemaphoreOp));
  semaphoreOp.semOp = semOp;
  semaphoreOp.semNum = semNum;
  semaphoreOp.semValue = semValue;
  semaphoreOp.preOrPost = preOrPost;
  semaphoreOp.inSyncFence = inSyncFence;
  semaphoreOp.outSyncFence = outSyncFence;
  semaphoreOps.push_back(semaphoreOp);
}

void AICMetadataWriter::addDoorbellOp(DoorbellOps &doorbellsOps,
                                      uint8_t sizeEnum, uint16_t mcId,
                                      uint64_t offset, uint32_t data) {
  bool validDoorbellOp;
  switch (sizeEnum) {
  default:
    validDoorbellOp = false;
    break;
  case AICMDDoorballOpSize8:
    [[fallthrough]];
  case AICMDDoorballOpSize16:
    [[fallthrough]];
  case AICMDDoorballOpSize32:
    validDoorbellOp = true;
    break;
  }
  assert(validDoorbellOp && "Invalid DMA doorbell operation");
  (void)validDoorbellOp;
  AICMDDoorbellOp doorbell;
  memset(&doorbell, 0, sizeof(AICMDDoorbellOp));
  doorbell.size = sizeEnum;
  doorbell.mcId = mcId;
  doorbell.offset = offset;
  doorbell.data = data;
  doorbellsOps.push_back(doorbell);
}

void AICMetadataWriter::addDMARequest(uint16_t num, uint64_t hostOffset,
                                      uint8_t devAddrSpace, uint64_t devOffset,
                                      uint32_t size, uint8_t inOut,
                                      uint16_t portId, uint16_t mcId,
                                      SemaphoreOps &semaphoreOps,
                                      DoorbellOps &doorbellOps) {
  assert((devAddrSpace == AICMDDMAAddrSpaceMC ||
          devAddrSpace == AICMDDMAAddrSpaceDDR) &&
         "Invalid DMA address space.");

  AICMDDMARequest request;
  memset(&request, 0, sizeof(AICMDDMARequest));
  request.num = num;
  request.hostOffset = hostOffset;
  request.devAddrSpace = devAddrSpace;
  request.devOffset = devOffset;
  request.size = size;
  request.inOut = inOut;
  request.portId = portId;
  request.mcId = mcId;
  request.numSemaphoreOps = semaphoreOps.size();
  request.semaphoreOpsOffset = 0;
  request.numDoorbellOps = doorbellOps.size();
  request.doorbellOpsOffset = 0;
  DMARequests.push_back(DMARequest(request, semaphoreOps, doorbellOps));
}

void AICMetadataWriter::addNSPMulticastEntry(int core, uint8_t dynamic,
                                             uint32_t mask, uint32_t size,
                                             uint8_t addrSpace,
                                             uint64_t baseAddrOffset) {
  assert(metadata_.numNSPs &&
         "Must specify the number of cores before adding a mc entry.");
  assert(core < metadata_.numNSPs);
  NSPMulticastEntry mcEntry;
  memset(&mcEntry, 0, sizeof(NSPMulticastEntry));
  mcEntry.dynamic = dynamic;
  mcEntry.mask = mask;
  mcEntry.size = size;
  mcEntry.addrSpace = addrSpace;
  mcEntry.baseAddrOffset = baseAddrOffset;
  NSPMulticastTables[core].push_back(mcEntry);
}

void AICMetadataWriter::addHostMulticastEntry(uint32_t mask, uint32_t size) {
  HostMulticastEntry mcEntry;
  memset(&mcEntry, 0, sizeof(HostMulticastEntry));
  mcEntry.mask = mask;
  mcEntry.size = size;
  HostMulticastTable.push_back(mcEntry);
}

void AICMetadataWriter::addThreadDescriptor(nnc_activate_fp entryPoint,
                                            bool hasHMX, bool hasHVX) {
  assert(entryPoint && "Invalid entryPoint.");
  ThreadDescriptor threadDesc;
  memset(&threadDesc, 0, sizeof(ThreadDescriptor));
  threadDesc.entryPoint = entryPoint;
  threadDesc.typeMask = 0;
  if (hasHMX)
    threadDesc.typeMask |= AICMDThreadHMX;
  if (hasHVX)
    threadDesc.typeMask |= AICMDThreadHVX;
  ThreadDescriptors.push_back(threadDesc);
}

void AICMetadataWriter::addConstantMapping(uint32_t mask, uint64_t offset,
                                           uint32_t size) {
  assert((constMappingCores & mask) == 0 &&
         "NSPs are only allowed a single mapping.");
  constMappingCores |= mask;

  ConstantMapping mapping;
  memset(&mapping, 0, sizeof(ConstantMapping));
  mapping.coreMask = mask;
  mapping.constantDataBaseOffset = offset;
  mapping.size = size;
  ConstantMappings.push_back(mapping);
}
