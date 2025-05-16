// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QAICQPC_H
#define QAICQPC_H

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <map>
#include <vector>
#include <string>
#include <memory>
#include <functional>

// The major version number is checked to verify compatibility.  Therefore,
// inteface breaking changes must increment this version number.  The minor
// version number is unchecked and is used for documentation purposes only.
#define AIC_NETWORK_DESCRIPTION_MAJOR_VERSION 9
#define AIC_NETWORK_DESCRIPTION_MINOR_VERSION 0

#define AICQPC_MAJOR_VERSION 0
#define AICQPC_MINOR_VERSION 1
#define AICQPC_MAGIC_NUMBER 0x00A1CD7C

typedef struct QAicQpcHandle QAicQpcHandle;
typedef struct QpcHeader QpcHeader;
typedef struct QpcSegment QpcSegment;
typedef struct QAicQpc QAicQpc;

// This is a added once per AICQPC
struct QpcHeader {
  uint32_t magicNumber;
  uint8_t majorVersion;
  uint8_t minorVersion;
  uint16_t compressionType;
  uint64_t size;
  uint64_t base;
  QpcHeader()
      : magicNumber(AICQPC_MAGIC_NUMBER), majorVersion(AICQPC_MAJOR_VERSION),
        minorVersion(AICQPC_MINOR_VERSION), size(0) {}
};

// The segments name should be one of the following
//  "constantsdesc.bin" - Constants desc
//  "networkdesc.bin" - Network desc
//  "network.elf" - Network binary
//  "constants.bin" - Compile time constants binary

// The "offset" argument for constans.bin is = size of staticConstants
// The "offset" argument will be used to find the start of dynamic compile
// time constants start.
// Example:
//  struct QpcSegment *constants = ...... //some init
//  uint8_t* staticConstBuf = constants->start;
//  uint64_t staticConstBufSz = constants->offset;
//  uint8_t* dynamicConstBuf = constants->start + constants->offset;
//  uint64_t dynamicConstBufSz = constants->size - constants->offset;

// Segment header is per image that's going
// to be added to AICQPC
struct QpcSegment {
  uint64_t size;
  uint64_t offset; // This parameter is only relevant for constants.bin
  char *name;
  uint8_t *start;
  QpcSegment(uint64_t sz, uint64_t o, char *n, uint8_t *s)
      : size(sz), offset(o), name(n), start(s) {}
};

enum CompressionType {
  FASTPATH = 0, // Fast path, no compression
  SLOWPATH = 1  // Slow path for offline use compressed
};

struct QAicQpc {
  QpcHeader hdr;
  uint64_t numImages;
  QpcSegment *images; //  []Images
};

enum QpcSegmentKind { QPC_SEGMENT_BUFFER, QPC_SEGMENT_FILE };

// Segment descriptor used when generating a QPC.
struct QpcSegmentDesc {
  QpcSegment segment;
  QpcSegmentKind kind;
  std::string filePath;
  QpcSegmentDesc(uint64_t sz, uint64_t o, char *n, uint8_t *s)
      : segment(sz, o, n, s), kind(QPC_SEGMENT_BUFFER), filePath("") {}
  QpcSegmentDesc(uint64_t o, char *n, const char *f)
      : segment(0, o, n, nullptr), kind(QPC_SEGMENT_FILE), filePath(f) {}
};

// Create an empty QPC handle object
int createQpcHandle(QAicQpcHandle **handle, CompressionType c);

// Build up a QPC object from a qpc segment array.
int buildFromSegments(QAicQpcHandle *handle, const QpcSegment *segments,
                      size_t numSegments);

// Write QPC object to qpcPath from QPC segment descriptor array.
int buildFromSegments(const std::vector<QpcSegmentDesc> &segmentVec,
                      const std::string &qpcPath);

// Get serialized QPC object. This buffer is valid only till the handle is
// not destroyed
int getSerializedQpc(QAicQpcHandle *handle, uint8_t **serializedQpc,
                     size_t *serializedQpcSize);

// Get QPC object associated with the handle. This buffer is valid only till
// the handle is not destroyed
int getQpc(QAicQpcHandle *handle, QAicQpc **qpc);

// Build up a QPC object from serialized qpc object
int buildFromByteArray(QAicQpcHandle *handle, const uint8_t *serializedQpc);

// Build up a QPC object from another QAicQpc object
int buildFromQpc(QAicQpcHandle *handle, const QAicQpc *qpc);

// Free up the handle resources.
void destroyQpcHandle(QAicQpcHandle *handle);

// Generally the user should be able to maintain a QAicQpc handle, in case
// this is not possible the following function provide a way to copy the
// qpc buffer.
// The signature is similar to a memmove function with following functionality
//- The destination should be alloced to the size of serializedQpcSize and
// should be
//  8 byte aligned.
//- If the source and destination are the same, the function only fixes up the
//  addresses
uint8_t *copyQpcBuffer(uint8_t *destination, uint8_t *source, size_t destSize);

// This function does not allocate anything. Just assigns the value of segBuf
// from QPC
// return "true" if the srcBuf is QPC and the out vals are assigned false in
// every other case.
// [IN] source - QPC buffer
// [IN] segName - Segment name
// [OUT] segBuf, segSize
bool getQPCSegment(const uint8_t *source, const char *segName, uint8_t **segBuf,
                   size_t *segSize, size_t offset);

//  This function iterates over the vector of segments and returns an iterator
//  to the requested segment if present or end() iterator if absent
//  This function does not modify anything
std::vector<QpcSegment>::iterator
getQPCSegment(std::vector<QpcSegment> &segmentVector, std::string segmentName);

// This function takes qpc as input, and does the following:
//    Finds network.elf. Inside it:
//      Finds srcSectionName section
//      Applies convFunction to get data
//      Creates destSectionName section and stores that data
int convertNetworkElfSection(
    QAicQpc *qpc, QAicQpcHandle *&handle, uint8_t *&serialQpc,
    size_t &serialQpcSz, const std::string &srcSectionName,
    const std::string &destSectionName,
    std::function<std::vector<uint8_t>(const uint8_t *, size_t)> convFunction);

// This function returns a section data from network.elf in qpc
std::unique_ptr<uint8_t[]>
getNetworkElfSectionDataFromQpc(QAicQpc *, std::string, size_t &);
#endif
