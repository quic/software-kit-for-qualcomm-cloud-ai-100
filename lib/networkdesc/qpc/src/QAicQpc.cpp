// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QAicQpc.h"
// #include "crc32.h"
#include "elfio/elfio.hpp"
#include <assert.h>
#include <iostream>
#include <malloc.h>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

typedef struct QAicQpcOffsets QAicQpcOffsets;

static const std::string networkElfFileName("network.elf");
static const std::string networkElfCRCFileName("network.elf.crc");
static const std::string constantsBinaryFileName("constants.bin");
static const std::string constantsBinaryCRCFileName("constants.bin.crc");
static const std::string networkMetadataCRC("metadata.crc");
static const std::string networkFlatbufferMDCRC("metadata_fb.crc");
static const std::string networkElfFileCRCSection("image_crc32");
static const std::string networkElfMetadataSection("metadata");
static const std::string networkElfFlatbufferMDSection("metadata_fb");

static const uint32_t networkElfSWIVSectionType = 0xD3574956;

struct QAicQpcOffsets {
  uint64_t imagesOffset;
  std::vector<uint64_t> *imageNameOffsets;
  std::vector<uint64_t> *imageStartOffsets;
};

struct QAicQpcHandle {
  uint64_t compressionType;
  std::vector<uint8_t> *qpcBuffer;
  QAicQpcOffsets *qpcOffsets;
};

static uint64_t alignTo(uint64_t x, uint64_t m) {
  return ((x + (m - 1)) & ~(m - 1)); // Only works if alignment is 2^n
}

static uint8_t *computeAddr(uint8_t *base, uint64_t offset) {
  return base + offset;
}

static uint8_t *computeAddrByBase(uint8_t *old_base, uint8_t *new_base,
                                  uint8_t *old_address) {
  uint64_t offset = reinterpret_cast<uint64_t>(old_address) -
                    reinterpret_cast<uint64_t>(old_base);
  return new_base + offset;
}

namespace aicqpc {
template <typename T>
size_t writeElement(const T *src, size_t elts, std::vector<uint8_t> &buffer,
                    size_t offset, bool commit) {
  size_t bytes = elts * sizeof(T);
  if (commit) {
    memcpy(&buffer[offset], src, bytes);
  }
  return alignTo(offset + bytes,
                 8); // Return an 8 byte aligned offset for next op
}
} // namespace aicqpc

static uint64_t writeQAicQpcObject(QAicQpcHandle *handle,
                                   const QpcSegment *segments,
                                   size_t numSegments, bool commit) {
  uint64_t writeOffset = 0;
  uint64_t segmentCount;
  QAicQpc aicQpcTemp;

  QAicQpcOffsets *offsets = handle->qpcOffsets;
  // Calculate the header size
  aicQpcTemp.numImages = numSegments;
  aicQpcTemp.hdr.size = (handle->qpcBuffer)->size();
  aicQpcTemp.hdr.compressionType = handle->compressionType;
  aicQpcTemp.hdr.base = reinterpret_cast<uint64_t>((handle->qpcBuffer)->data());

  writeOffset = aicqpc::writeElement(&aicQpcTemp, 1, *handle->qpcBuffer,
                                     writeOffset, commit);
  // Align for calculating QpcSegments
  offsets->imagesOffset = writeOffset;
  for (segmentCount = 0; segmentCount < numSegments; ++segmentCount) {
    writeOffset = aicqpc::writeElement(segments + segmentCount, 1,
                                       *handle->qpcBuffer, writeOffset, commit);
  }

  if (handle->compressionType == SLOWPATH) {
    // Set names offset
    for (segmentCount = 0; segmentCount < numSegments; ++segmentCount) {
      (offsets->imageNameOffsets)->push_back(writeOffset);
      writeOffset = aicqpc::writeElement(
          segments[segmentCount].name, strlen(segments[segmentCount].name) + 1,
          *handle->qpcBuffer, writeOffset, commit);
    }
    // Set image buffer offset
    for (segmentCount = 0; segmentCount < numSegments; ++segmentCount) {
      (offsets->imageStartOffsets)->push_back(writeOffset);
      writeOffset = aicqpc::writeElement(
          segments[segmentCount].start, segments[segmentCount].size,
          *handle->qpcBuffer, writeOffset, commit);
    }
  }

  return writeOffset;
}

static void fixupQAicQpcObject(QAicQpcHandle *handle, uint8_t *base) {
  QAicQpcOffsets *offsets = handle->qpcOffsets;
  QAicQpc *aicQpc = (QAicQpc *)base;
  uint64_t segmentCount;

  // Set the start of the images array
  aicQpc->images =
      reinterpret_cast<QpcSegment *>(computeAddr(base, offsets->imagesOffset));

  // No fixup needed for the segment contents in case of fastpath
  if (handle->compressionType == SLOWPATH) {
    for (segmentCount = 0; segmentCount < aicQpc->numImages; ++segmentCount) {
      aicQpc->images[segmentCount].name = reinterpret_cast<char *>(
          computeAddr(base, offsets->imageNameOffsets->at(segmentCount)));
      aicQpc->images[segmentCount].start =
          computeAddr(base, offsets->imageStartOffsets->at(segmentCount));
    }
  }
}

static void fixupQAicQpcCopyObject(uint8_t *base) {
  QAicQpc *aicQpc = (QAicQpc *)base;
  uint64_t segmentCount;

  aicQpc->images = reinterpret_cast<QpcSegment *>(
      computeAddrByBase(reinterpret_cast<uint8_t *>(aicQpc->hdr.base), base,
                        reinterpret_cast<uint8_t *>(aicQpc->images)));

  for (segmentCount = 0; segmentCount < aicQpc->numImages; ++segmentCount) {
    aicQpc->images[segmentCount].name =
        reinterpret_cast<char *>(computeAddrByBase(
            reinterpret_cast<uint8_t *>(aicQpc->hdr.base), base,
            reinterpret_cast<uint8_t *>(aicQpc->images[segmentCount].name)));
    aicQpc->images[segmentCount].start =
        computeAddrByBase(reinterpret_cast<uint8_t *>(aicQpc->hdr.base), base,
                          aicQpc->images[segmentCount].start);
  }

  // Fix the base in the hdr
  aicQpc->hdr.base = reinterpret_cast<uint64_t>(base);
}

static QAicQpcOffsets *createQpcOffsetsObject() {
  QAicQpcOffsets *retObj = nullptr;

  retObj = (QAicQpcOffsets *)malloc(sizeof(QAicQpcOffsets));

  if (!retObj) {
    return nullptr;
  }

  retObj->imageNameOffsets = new std::vector<uint64_t>();

  if (retObj->imageNameOffsets == nullptr) {
    free(retObj);
    return nullptr;
  }

  retObj->imageStartOffsets = new std::vector<uint64_t>();

  if (retObj->imageStartOffsets == nullptr) {
    delete retObj->imageNameOffsets;
    free(retObj);
    return nullptr;
  }

  return retObj;
}

static void destroyQpcOffsetsObject(QAicQpcOffsets *qpcOffsetsObj) {
  if (qpcOffsetsObj == nullptr) {
    return;
  }

  delete qpcOffsetsObj->imageNameOffsets;
  delete qpcOffsetsObj->imageStartOffsets;
  free(qpcOffsetsObj);
}

int createQpcHandle(QAicQpcHandle **handle, CompressionType c) {
  *handle = (QAicQpcHandle *)malloc(sizeof(QAicQpcHandle));
  if (NULL == *handle) {
    return -ENOMEM;
  }

  (*handle)->compressionType = static_cast<uint64_t>(c);

  // Default init
  (*handle)->qpcBuffer = new std::vector<uint8_t>();
  (*handle)->qpcOffsets = createQpcOffsetsObject();

  if ((*handle)->qpcOffsets == nullptr) {
    delete (*handle)->qpcBuffer;
    free(*handle);
    *handle = NULL;

    return -ENOMEM;
  }
  return 0;
}

static int convertNetworkElfSection(
    std::stringstream &is, std::ostringstream &os,
    const std::string &sourceSectionName, const std::string &destSectionName,
    std::function<std::vector<uint8_t>(const uint8_t *, size_t)> convFunction) {
  // Open an ELFIO object to read and write section
  ELFIO::elfio elfEditor;
  if (!elfEditor.load(is)) {
    return -ENOMEM;
  }

  // Find the source section
  int numSections = elfEditor.sections.size();
  int sourceSectionIdx = 0;
  for (const auto &section : elfEditor.sections) {
    if (section->get_name() == sourceSectionName) {
      break;
    }
    sourceSectionIdx++;
  }
  if (sourceSectionIdx == numSections) {
    return -EINVAL;
  }

  // Source Section found. Now converting data.
  ELFIO::section *sourceSec = elfEditor.sections[sourceSectionIdx];
  if (sourceSec == nullptr) {
    return -ENOENT;
  }
  std::vector<uint8_t> destVec{
      convFunction(reinterpret_cast<const uint8_t *>(sourceSec->get_data()),
                   reinterpret_cast<size_t>(sourceSec->get_size()))};

  // If dest section exists, copy and exit.
  if (ELFIO::section *destSec = elfEditor.sections[destSectionName]; destSec) {
    destSec->set_data((const char *)const_cast<const uint8_t *>(destVec.data()),
                      destVec.size());
    if (!elfEditor.save(os)) {
      return -ENOMEM;
    }
    return 0;
  }

  // At this point metadata_fb does not exist, so we have to add that section.
  // ELFIO does not provide a public api to insert new sections between two
  // sections.
  // We can not just add the new section at the end, as it will break tools used
  // by
  // other teams.
  // To insert the new section manually, we serially move data from one section
  // to the next.
  // Reason why the name and name_string_offset are handled seperately:
  //    ELFIO::section::set_name() does not automatically update the string
  //    offset,
  //    as is done for adding a new section. Hence, we need to add the new
  //    section
  //    with the name of destSectionName and move over the name and
  //    name_string_offset
  //    one by one.

  // Data type to hold section data that will be used to insert section from one
  // to the next
  struct elfInfo {
    ELFIO::Elf_Word type;
    ELFIO::Elf_Xword flags;
    ELFIO::Elf_Word info;
    ELFIO::Elf_Word link;
    ELFIO::Elf_Xword addr_align;
    ELFIO::Elf_Xword entry_size;
    ELFIO::Elf64_Addr address;
    ELFIO::Elf_Xword size;
    std::vector<char> data;
    elfInfo() = default;
    elfInfo(ELFIO::Elf_Word type, ELFIO::Elf_Xword flags, ELFIO::Elf_Word info,
            ELFIO::Elf_Word link, ELFIO::Elf_Xword addr_align,
            ELFIO::Elf_Xword entry_size, ELFIO::Elf64_Addr address,
            ELFIO::Elf_Xword size)
        : type(type), flags(flags), info(info), link(link),
          addr_align(addr_align), entry_size(entry_size), address(address),
          size(size) {}
  };
  struct elfName {
    std::string name;
    ELFIO::Elf_Word name_string_offset;
    elfName() = default;
    elfName(std::string name, ELFIO::Elf_Word name_string_offset)
        : name(name), name_string_offset(name_string_offset) {}
  };

  // Need to insert section
  elfInfo tempInfoFirst, tempInfoLast;

  // Store the dest section data to tempFirst
  tempInfoFirst = elfInfo(
      sourceSec->get_type(), sourceSec->get_flags(), sourceSec->get_info(),
      sourceSec->get_link(), sourceSec->get_addr_align(),
      sourceSec->get_entry_size(), sourceSec->get_address(), destVec.size());
  tempInfoFirst.data.assign(destVec.begin(), destVec.end());

  for (int i = sourceSectionIdx + 1; i < numSections;
       i++, tempInfoFirst = tempInfoLast) {
    // Save current section information
    ELFIO::section *section = elfEditor.sections[i];
    if (section == nullptr) {
      return -ENOENT;
    }
    tempInfoLast = elfInfo(section->get_type(), section->get_flags(),
                           section->get_info(), section->get_link(),
                           section->get_addr_align(), section->get_entry_size(),
                           section->get_address(), section->get_size());
    tempInfoLast.data.assign(section->get_data(),
                             section->get_data() + section->get_size());

    // Update current section with data from temp info
    section->set_type(tempInfoFirst.type);
    section->set_flags(tempInfoFirst.flags);
    section->set_info(tempInfoFirst.info);
    section->set_link(tempInfoFirst.link);
    section->set_addr_align(tempInfoFirst.addr_align);
    section->set_entry_size(tempInfoFirst.entry_size);
    section->set_address(tempInfoFirst.address);
    section->set_size(tempInfoFirst.size);
    section->set_data(tempInfoFirst.data.data(), tempInfoFirst.size);
  }

  // Create new section - insert at the last entry
  ELFIO::section *lastSection = elfEditor.sections.add(destSectionName);
  if (lastSection == nullptr) {
    return -ENOMEM;
  }
  numSections++;
  lastSection->set_type(tempInfoFirst.type);
  lastSection->set_flags(tempInfoFirst.flags);
  lastSection->set_info(tempInfoFirst.info);
  lastSection->set_link(tempInfoFirst.link);
  lastSection->set_addr_align(tempInfoFirst.addr_align);
  lastSection->set_entry_size(tempInfoFirst.entry_size);
  lastSection->set_address(tempInfoFirst.address);
  lastSection->set_size(tempInfoFirst.size);
  lastSection->set_data(tempInfoFirst.data.data(), tempInfoFirst.size);

  // Correct the name and name_string_offset
  elfName tempNameFirst, tempNameLast;

  tempNameFirst =
      elfName(lastSection->get_name(), lastSection->get_name_string_offset());
  for (int i = sourceSectionIdx + 1; i < numSections;
       i++, tempNameFirst = tempNameLast) {
    // Save the current section name and offset
    ELFIO::section *section = elfEditor.sections[i];
    if (section == nullptr) {
      return -ENOENT;
    }
    tempNameLast =
        elfName(section->get_name(), section->get_name_string_offset());

    // Update current section name and offset
    section->set_name(tempNameFirst.name);
    section->set_name_string_offset(tempNameFirst.name_string_offset);
  }

  if (!elfEditor.save(os)) {
    return -ENOMEM;
  }

  return 0;
}

static void serializeFromVector(QAicQpcHandle *handle,
                                std::vector<QpcSegment> &segmentVector) {
  // Clear the buffer
  (handle->qpcBuffer)->clear();
  uint64_t qpcBufferSize = 0;
  qpcBufferSize =
      writeQAicQpcObject(handle, &segmentVector[0], segmentVector.size(),
                         false); // Try run no commits
  // Resize the buffer
  (handle->qpcBuffer)->resize(qpcBufferSize);
  // commit the changes
  qpcBufferSize =
      writeQAicQpcObject(handle, &segmentVector[0], segmentVector.size(), true);

  fixupQAicQpcObject(handle, (handle->qpcBuffer)->data());
}

int convertNetworkElfSection(
    QAicQpc *qpc, QAicQpcHandle *&handle, uint8_t *&serialQpc,
    size_t &serialQpcSz, const std::string &sourceSectionName,
    const std::string &destSectionName,
    std::function<std::vector<uint8_t>(const uint8_t *, size_t)> convFunction) {
  std::vector<QpcSegment> segmentVector{qpc->images,
                                        qpc->images + qpc->numImages};

  // Find network elf
  std::vector<QpcSegment>::iterator networkElfIt =
      getQPCSegment(segmentVector, networkElfFileName);
  if (networkElfIt == segmentVector.end()) {
    return -ENOENT;
  }

  // Get the qpc segment data
  std::stringstream is(
      std::string((char *)networkElfIt->start, networkElfIt->size));

  // Conversion of section
  std::ostringstream os;
  if (int rc = convertNetworkElfSection(is, os, sourceSectionName,
                                        destSectionName, convFunction);
      rc != 0) {
    return rc;
  }
  std::string networkElf{os.str()};

  // Delete the elf QPC segment and add the new segment as needed
  QpcSegment newElf(
      networkElf.size(), 0, networkElfIt->name,
      reinterpret_cast<uint8_t *>(const_cast<char *>(networkElf.data())));

  segmentVector.erase(networkElfIt);
  segmentVector.push_back(newElf);

  // Serialize
  if (int rc = createQpcHandle(
          &handle, static_cast<CompressionType>(qpc->hdr.compressionType));
      rc != 0) {
    return rc;
  }
  serializeFromVector(handle, segmentVector);
  if (int rc = getSerializedQpc(handle, &serialQpc, &serialQpcSz); rc != 0) {
    return rc;
  }

  return 0;
}

int buildFromSegments(QAicQpcHandle *handle, const QpcSegment *segments,
                      size_t numSegments) {
  // Calculate QPC buffer size and resize it
  std::vector<QpcSegment> segmentVector;
  std::string networkElf; // This is only needed till this function returns
  // This is only needed to store allocations for checksum segments
  // In case of slowpath we make a new copy of the buffers so, this is
  // not needed after buildFromSegments returns
  std::deque<uint32_t> placeholderBuffer;

  if (handle == nullptr || segments == nullptr) {
    return -EINVAL;
  }

  segmentVector.assign(segments, segments + numSegments);

  serializeFromVector(handle, segmentVector);

  return 0;
}

int getSerializedQpc(QAicQpcHandle *handle, uint8_t **serializedQpc,
                     size_t *serializedQpcSize) {
  if (handle == nullptr || serializedQpcSize == nullptr ||
      serializedQpc == nullptr) {
    return -EINVAL;
  }

  *serializedQpc = (handle->qpcBuffer)->data();
  *serializedQpcSize = (handle->qpcBuffer)->size();

  return 0;
}

int getQpc(QAicQpcHandle *handle, QAicQpc **qpc) {
  if (handle == nullptr || qpc == nullptr) {
    return -EINVAL;
  }

  *qpc = reinterpret_cast<QAicQpc *>((handle->qpcBuffer)->data());

  return 0;
}

int buildFromByteArray(QAicQpcHandle *handle, const uint8_t *serializedQpc) {
  const QAicQpc *aicQpc;
  if (handle == nullptr || serializedQpc == nullptr) {
    return -EINVAL;
  }
  aicQpc = reinterpret_cast<const QAicQpc *>(serializedQpc);

  return buildFromQpc(handle, aicQpc);
}

int buildFromQpc(QAicQpcHandle *handle, const QAicQpc *qpc) {
  if (handle == nullptr || qpc == nullptr) {
    return -EINVAL;
  }

  (void)buildFromSegments(handle, qpc->images, qpc->numImages);

  return 0;
}

void destroyQpcHandle(QAicQpcHandle *handle) {
  if (handle == nullptr) {
    return;
  }
  destroyQpcOffsetsObject(handle->qpcOffsets);
  delete handle->qpcBuffer;
  free(handle);
}

// Helper function. Generally the user should be able to maintain a QAicQpc
// handle,
// But the following function will provide a copied and fixed up qpc buffer
// The signature is similar to a memmove function.
// The detination should be alloced to the size of serializedQpcSize and should
// be
// 8 byte aligned.
uint8_t *copyQpcBuffer(uint8_t *destination, uint8_t *source, size_t destSize) {
  if (source == nullptr || destination == nullptr || destSize == 0) {
    return nullptr;
  }
  // check is source is valid and is a qpc image
  uint32_t qpcMagic = AICQPC_MAGIC_NUMBER;

  if (memcmp(source, (void *)&qpcMagic, sizeof(uint32_t)) != 0) {
    return nullptr;
  }

  // check if destSize is correct
  QAicQpc *qpc = reinterpret_cast<QAicQpc *>(source);

  if (qpc->hdr.size != destSize) {
    return nullptr;
  }

  // We cannot copy and fixup a FASTPATH buffer
  if (qpc->hdr.compressionType == FASTPATH) {
    return nullptr;
  }

  // User is expecting fixup only
  if (source != destination) {
    (void)memmove(destination, source, destSize);
  }

  fixupQAicQpcCopyObject(destination);

  return destination;
}

bool getQPCSegment(const uint8_t *source, const char *segName, uint8_t **segBuf,
                   size_t *segSize, size_t offset) {
  bool retVal = false;
  uint32_t qpcMagic = AICQPC_MAGIC_NUMBER;

  if (segName == nullptr || segBuf == nullptr) {
    return retVal; // false
  }

  if (source != nullptr) {
    // Check if the user passed in a QPC image
    if (memcmp(source, (void *)&qpcMagic, sizeof(uint32_t)) == 0) {
      // This is a qpc image. Get the constantsdesc
      const QAicQpc *qpc = reinterpret_cast<const QAicQpc *>(source);

      for (uint64_t count = 0; count < qpc->numImages; count++) {
        QpcSegment *segment = &(qpc->images[count]);
        if (strcmp(segName, segment->name) == 0) {

          // segment->offset is always 0 for all images sans
          // constants.bin. At this point for compile time
          // constants we only have 2 sections.
          if (segment->offset > 0 && offset == 0) {
            // Load static compile time constants.
            *segBuf = segment->start;
            *segSize = segment->offset;
          } else {
            *segBuf = segment->start + segment->offset;
            *segSize = segment->size - segment->offset;
          }

          retVal = true;
          break;
        }
      }
    }
  }

  return retVal;
}

std::vector<QpcSegment>::iterator
getQPCSegment(std::vector<QpcSegment> &segmentVector, std::string segmentName) {
  for (auto it = segmentVector.begin(); it != segmentVector.end(); it++) {
    if (strcmp(it->name, segmentName.c_str()) == 0) {
      return it;
    }
  }
  return segmentVector.end();
}

static int binaryFileToBuffer(const std::string &filePath,
                              std::vector<uint8_t> &buf) {
  std::ifstream ifs(filePath, std::ifstream::ate | std::ifstream::binary);
  if (!ifs) {
    std::cout
        << "Error: buildFromSegments failed. Unable to read segment from file: "
        << filePath << std::endl;
    return -EINVAL;
  }
  size_t fileSize = static_cast<std::size_t>(ifs.tellg());
  buf.resize(fileSize);
  ifs.seekg(std::ios_base::beg);
  ifs.read((char *)buf.data(), buf.size());
  ifs.close();
  return 0;
}

template <typename T>
void write(std::ofstream &qpcFile, const T *src, int elts) {
  uint64_t pos = qpcFile.tellp();
  while (alignTo(pos, alignof(T)) != pos) {
    qpcFile.write(0, 1);
    ++pos;
  }
  size_t bytes = elts * sizeof(T);
  qpcFile.write(reinterpret_cast<const char *>(src), bytes);
}

/// Incrementally write constants from inputFile to qpcFile
int writeConstants(std::ofstream &qpcFile, const std::string &inputFilePath,
                   std::vector<QpcSegment> &segments, int idx) {

  std::ifstream inputFile(inputFilePath,
                          std::ifstream::ate | std::ifstream::binary);
  if (!inputFile) {
    std::cout
        << "Error: buildFromSegments failed. Unable to read segment from file: "
        << inputFilePath << std::endl;
    return -EINVAL;
  }

  size_t fileSize = static_cast<std::size_t>(inputFile.tellg());

  // Load in constants.bin 1MB at a time
  inputFile.seekg(std::ios_base::beg);
  constexpr int numBytesToLoad = 1024 * 1024;
  std::vector<uint8_t> segmentBuf(numBytesToLoad);
  while (inputFile.read((char *)segmentBuf.data(), numBytesToLoad)) {
    write(qpcFile, segmentBuf.data(), segmentBuf.size());
  }

  // Now load any trailing bytes that didn't fit into buffer
  int numBytesRemaining = fileSize % numBytesToLoad;
  if (numBytesRemaining > 0) {
    inputFile.clear();
    inputFile.seekg(fileSize - numBytesRemaining);
    segmentBuf.resize(numBytesRemaining);
    while (inputFile.read((char *)segmentBuf.data(), segmentBuf.size())) {
      write(qpcFile, segmentBuf.data(), segmentBuf.size());
    }
  }

  // Record the size of this segment
  segments[idx].start = segmentBuf.data();
  segments[idx].size = fileSize;

  inputFile.close();

  return 0;
}

// Builds the QPC from \p segments.  Writes QPC to \p outputPath.
int buildFromSegments(const std::vector<QpcSegmentDesc> &segmentVec,
                      const std::string &qpcPath) {
  if (segmentVec.empty()) {
    std::cout
        << "Error: buildFromSegments failed. No segment descriptors provided."
        << std::endl;
    return -EINVAL;
  }

  std::unordered_set<std::string> segmentNames;
  for (auto &sd : segmentVec) {
    auto s = sd.segment;
    if (segmentNames.count(s.name)) {
      std::cout
          << "Error: buildFromSegments failed. Redundant segment provided: "
          << std::string(s.name) << std::endl;
      return -EINVAL;
    }
    segmentNames.insert(s.name);
  }

  // Verify the required segments have been provided.
  if (segmentNames.count(networkElfFileName) == 0) {
    std::cout << "Error: buildFromSegments failed. Missing "
              << networkElfFileName << " segment." << std::endl;
    return -EINVAL;
  }

  // Make a copy of the segments, so we can modify the offsets.
  std::vector<QpcSegment> segments;
  segments.reserve(segmentVec.size());
  // Maps the segment name to a segment buffer that gets loaded from disk.
  std::unordered_map<std::string, std::vector<uint8_t>>
      segmentNameToSegmentBuffer;
  // Keep track of the path to the constants.bin file so it can be loaded later
  std::string constantsFilePath;
  for (auto &sd : segmentVec) {
    auto &s = sd.segment;
    segments.emplace_back(s.size, s.offset, s.name, s.start);
    if (sd.kind == QPC_SEGMENT_FILE) {
      // Don't buffer constants, write them out directly later
      if (s.name == constantsBinaryFileName) {
        constantsFilePath = sd.filePath;
        continue;
      }

      assert(!sd.filePath.empty());
      auto &segmentBuf = segmentNameToSegmentBuffer[s.name];
      int res = binaryFileToBuffer(sd.filePath, segmentBuf);
      if (res != 0)
        return res;

      assert(s.start == nullptr);
      segments.back().start = segmentBuf.data();
      assert(s.size == 0);
      segments.back().size = segmentBuf.size();
    }
  }

  // Write the QPC to disk.
  std::ofstream qpcFile(qpcPath,
                        std::ios::out | std::ios::trunc | std::ios::binary);
  if (!qpcFile) {
    std::cout << "Error: Unable to open QPC for writing: " << qpcPath
              << std::endl;
    return -EBADF;
  }

  int numSegments = segments.size();

  // Write base QPC. The various offsets are unknown at this point, so
  // we'll have to come back and fix them up after the fact.
  QAicQpc qpc;
  qpc.hdr.size = 0;
  qpc.hdr.compressionType = SLOWPATH;
  qpc.hdr.base = 0;
  qpc.numImages = numSegments;
  qpc.images = nullptr;
  write(qpcFile, &qpc, 1);

  uint64_t imagesOffset = qpcFile.tellp();
  std::vector<uint64_t> segmentNameOffsets(numSegments, 0);
  std::vector<uint64_t> segmentDataOffsets(numSegments, 0);

  // Write the segments.  The various offsets are unknown at this point, so
  // we'll have to come back and fix them up after the fact.
  for (auto &s : segments)
    write(qpcFile, &s, 1);

  // Write the segment names.
  for (int i = 0; i < numSegments; ++i) {
    segmentNameOffsets[i] = qpcFile.tellp();
    const char *segName = segments[i].name;
    write(qpcFile, segName, strlen(segName) + 1);
  }

  // Write the segment data.
  for (int i = 0; i < numSegments; ++i) {
    segmentDataOffsets[i] = qpcFile.tellp();
    // If this segment is the constants file, write it directly from
    // the binary file (it hasn't been stored in the segments buffer).
    if (segments[i].name == constantsBinaryFileName) {
      int res = writeConstants(qpcFile, constantsFilePath, segments, i);
      if (res != 0)
        return res;
    } else {
      write(qpcFile, segments[i].start, segments[i].size);
    }
  }

  // Update the size of the QPC.
  qpc.hdr.size = qpcFile.tellp();
  // Update the offset to the images.
  qpc.images = (QpcSegment *)imagesOffset;

  // Update the offsets in the segments.
  for (int i = 0; i < numSegments; ++i) {
    segments[i].name = reinterpret_cast<char *>(segmentNameOffsets[i]);
    segments[i].start = reinterpret_cast<uint8_t *>(segmentDataOffsets[i]);
  }

  // Go to the beginning and write the updated QPC.
  qpcFile.seekp(std::ios_base::beg);
  write(qpcFile, &qpc, 1);

  // Write the segments with the updated offsets.
  for (auto &s : segments)
    write(qpcFile, &s, 1);

  qpcFile.close();
  return 0;
}

std::unique_ptr<uint8_t[]>
getNetworkElfSectionDataFromQpc(QAicQpc *qpc, std::string sectionName,
                                size_t &size) {
  std::vector<QpcSegment> segmentVector{qpc->images,
                                        qpc->images + qpc->numImages};
  // Find network elf
  std::vector<QpcSegment>::iterator networkElfIt =
      getQPCSegment(segmentVector, networkElfFileName);
  if (networkElfIt == segmentVector.end()) {
    return nullptr;
  }
  // Get the qpc segment data
  std::stringstream is(
      std::string((char *)networkElfIt->start, networkElfIt->size));

  ELFIO::elfio elfReader;
  if (!elfReader.load(is)) {
    return nullptr;
  }

  ELFIO::section *elfSec;
  if ((elfSec = elfReader.sections[sectionName]) == nullptr) {
    return nullptr;
  }
  size = elfSec->get_size();
  std::unique_ptr<uint8_t[]> buf{std::make_unique<uint8_t[]>(size)};
  std::memcpy(buf.get(), elfSec->get_data(), size);
  return buf;
}
