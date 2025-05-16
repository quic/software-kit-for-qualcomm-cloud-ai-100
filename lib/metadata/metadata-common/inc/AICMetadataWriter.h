// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef AIC_METADATA_WRITER
#define AIC_METADATA_WRITER

#include "AICMetadata.h"

#include "stdio.h"
#include <array>
#include <cassert>
#include <string>
#include <vector>

typedef AICMDSemaphoreOp SemaphoreOp;
typedef std::vector<SemaphoreOp> SemaphoreOps;
typedef AICMDDoorbellOp DoorbellOp;
typedef std::vector<DoorbellOp> DoorbellOps;

class AICMetadataWriter {
private:
  AICMetadata metadata_;

  bool isFinal{false};
  std::vector<uint8_t> metadataBuffer;

  std::string networkName; // Network name

  // Initial state of L2TCM.
  std::vector<uint8_t> L2TCMInitState;

  // Initial state of semaphores.
  std::vector<uint32_t> semaphoreInitState;

  // DMA requests.
  struct DMARequest {
    DMARequest(AICMDDMARequest r, SemaphoreOps &sOps, DoorbellOps &dOps)
        : request(r), semaphoreOps(sOps), doorbellOps(dOps) {}
    AICMDDMARequest request;
    SemaphoreOps semaphoreOps;
    DoorbellOps doorbellOps;
  };
  std::vector<struct DMARequest> DMARequests;

  // NSP multicast tables (one per NSP).
  typedef AICMDNSPMulticastEntry NSPMulticastEntry;
  typedef std::vector<NSPMulticastEntry> NSPMulticastTable;
  std::array<NSPMulticastTable, 16> NSPMulticastTables;

  // Host multicast table.
  typedef AICMDHostMulticastEntry HostMulticastEntry;
  std::vector<HostMulticastEntry> HostMulticastTable;

  // Constant mappings.
  uint32_t constMappingCores{0};

  // Thread descriptors.
  typedef AICMDThreadDescriptor ThreadDescriptor;
  std::vector<ThreadDescriptor> ThreadDescriptors;

  typedef AICMDConstantMapping ConstantMapping;
  std::vector<ConstantMapping> ConstantMappings;

  // Helpers for finalize.
  void finiSemaphoreState();
  void finiL2TCMState();
  void finiDMARequests();
  void finiMulticastTables();
  void finiThreadDescriptors();
  void finiConstantMappings();

  // Write 'elts' elements of type 'T' to the buffer starting at an aligned
  // offset. Returns the offset of the start of the written elements.
  template <typename T> uint32_t write(const T *src, int elts);
  template <typename T> void write(const T *src, int elts, uint32_t offset);

public:
  AICMetadataWriter(uint16_t hwVerMajor, uint16_t hwVerMinor);
  void finalize();
  int getSize() const {
    assert(isFinal);
    return metadataBuffer.size();
  }
  // Returns true if write succeeded, false otherwise.
  bool writeMetadata(const char *outfile) const;
  const std::vector<uint8_t> &getMetadata() const {
    assert(isFinal);
    return metadataBuffer;
  }

  // Number of NSPs.
  int getNumNSPs() const { return metadata_.numNSPs; }
  void setNumNSPs(uint16_t numNSPs) { metadata_.numNSPs = numNSPs; }

  // VTCM size in bytes.
  int getVTCMSize() const { return metadata_.VTCMSize; }
  void setVTCMSize(uint32_t vtcmSize) { metadata_.VTCMSize = vtcmSize; }

  // L2TCM size in bytes.
  int getL2TCMSize() const { return metadata_.L2TCMSize; }
  void setL2TCMSize(uint32_t l2TCMSize) { metadata_.L2TCMSize = l2TCMSize; }

  // Network name.
  std::string getNetworkName() const { return networkName; }
  void setNetworkName(std::string name) { networkName = name; }

  // Number of virtual semaphores.
  int getNumSemaphores() const { return metadata_.numSemaphores; }
  void setNumSemaphores(uint16_t numSemaphores) {
    metadata_.numSemaphores = numSemaphores;
    semaphoreInitState.resize(numSemaphores);
  }

  void initSemaphore(uint32_t semId, uint32_t val) {
    assert(semId < semaphoreInitState.size());
    semaphoreInitState[semId] = val;
  }

  // L2TCM init region size in bytes.
  int getL2TCMInitSize() const { return L2TCMInitState.size(); }
  void initL2TCMResize(uint32_t newSize) { L2TCMInitState.resize(newSize); }
  void initL2TCMByte(uint64_t offset, uint8_t byte) {
    assert(offset < L2TCMInitState.size());
    L2TCMInitState[offset] = byte;
  }
  void initL2TCMWord(uint64_t offset, uint32_t word) {
    for (int i = 0; i < 4; i++, word >>= 8) {
      uint8_t b = word & 0xff;
      L2TCMInitState[offset++] = b;
    }
  }

  // Shared DDR
  uint64_t getStaticSharedDDRSize() const {
    return metadata_.staticSharedDDRSize;
  }
  void setStaticSharedDDRSize(uint64_t size) {
    metadata_.staticSharedDDRSize = size;
  }

  uint64_t getDynamicSharedDDRSize() const {
    return metadata_.dynamicSharedDDRSize;
  }
  void setDynamicSharedDDRSize(uint64_t size) {
    metadata_.dynamicSharedDDRSize = size;
  }

  bool getStaticSharedDDRECCEnabled() const {
    return metadata_.staticSharedDDRECCEnabled;
  }
  void setStaticSharedDDRECCEnabled(bool v) {
    metadata_.staticSharedDDRECCEnabled = v;
  }

  bool getDynamicSharedDDRECCEnabled() const {
    return metadata_.dynamicSharedDDRECCEnabled;
  }
  void setDynamicSharedDDRECCEnabled(bool v) {
    metadata_.dynamicSharedDDRECCEnabled = v;
  }

  // Constants
  uint64_t getStaticConstantsSize() const {
    return metadata_.staticConstantsSize;
  }
  void setStaticConstantsSize(uint64_t size) {
    metadata_.staticConstantsSize = size;
  }

  uint64_t getDynamicConstantsSize() const {
    return metadata_.dynamicConstantsSize;
  }
  void setDynamicConstantsSize(uint64_t size) {
    metadata_.dynamicConstantsSize = size;
  }

  bool getStaticConstantsECCEnabled() const {
    return metadata_.staticConstantsECCEnabled;
  }
  void setStaticConstantsECCEnabled(bool v) {
    metadata_.staticConstantsECCEnabled = v;
  }

  bool getDynamicConstantsECCEnabled() const {
    return metadata_.dynamicConstantsECCEnabled;
  }
  void setDynamicConstantsECCEnabled(bool v) {
    metadata_.dynamicConstantsECCEnabled = v;
  }

  bool getSingleVTCMPage() const { return metadata_.singleVTCMPage; }
  void setSingleVTCMPage(bool v) { metadata_.singleVTCMPage = v; }

  // Host<->NSP DMA requests.
  void addSemaphoreOp(SemaphoreOps &semaphoreOps, uint8_t semOp,
                      uint16_t semNum, uint16_t semValue, uint8_t preOrPost,
                      uint8_t inSyncFence, uint8_t outSyncFence);
  void addDoorbellOp(DoorbellOps &doorbellsOps, uint8_t sizeEnum, uint16_t mcId,
                     uint64_t offset, uint32_t data);
  void addDMARequest(uint16_t num, uint64_t hostOffset, uint8_t devAddrSpace,
                     uint64_t devOffset, uint32_t size, uint8_t inOut,
                     uint16_t portID, uint16_t mcId, SemaphoreOps &semaphoreOps,
                     DoorbellOps &doorbellOps);

  // Add NSP/host multicast entries.
  void addNSPMulticastEntry(int core, uint8_t dynamic, uint32_t mask,
                            uint32_t size, uint8_t addrSpace,
                            uint64_t baseAddrOffset);
  void addHostMulticastEntry(uint32_t mask, uint32_t size);

  // Add thread descriptor.
  void addThreadDescriptor(nnc_activate_fp entryPoint, bool hasHMX,
                           bool hasHVX);

  // Add constant mapping.
  void addConstantMapping(uint32_t mask, uint64_t offset, uint32_t size);

  void setExitDoorbellOffset(uint64_t offset) {
    metadata_.exitDoorbellOffset = offset;
  }

  // Network heap
  uint64_t getNetworkHeapSize() const { return metadata_.networkHeapSize; }
  void setNetworkHeapSize(uint64_t size) { metadata_.networkHeapSize = size; }

  // set FP presence
  void setHasHvxFP(bool hasHvx) { metadata_.hasHvxFP = hasHvx ? 1 : 0; }
  void setHasHmxFP(bool hasHmx) { metadata_.hasHmxFP = hasHmx ? 1 : 0; }
};

#endif // AIC_METADATA_WRITER
