// Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef FLATBUFF_DRIVER_FLATBUF_ENCODE_H
#define FLATBUFF_DRIVER_FLATBUF_ENCODE_H

#include "AICMetadata.h"
#include "AICMetadataReader.h"
#include "AicMetadataFlat_generated.h"
#include "AICMetadataReader.h"
#include "flatbuffers/idl.h"
#include "flatbuffers/registry.h"
#include "flatbuffers/util.h"

#include <iostream>
#include <fstream>

#ifndef C_DECODER_TESTING
// ifndef isolation because json won't build for the C decoder test code
#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace metadataFlat {

// preconditions:
//      schemafile present and located at "schemafile"
//      *builder is instance of a metadata flatbuffer
// returnvalue:
//      a json version of the metadata flatbuffer is returned as a std::string
static std::string populateMessage(std::string schemafile,
                                   flatbuffers::FlatBufferBuilder *builder) {
  flatbuffers::LoadFile(schemafile.c_str(), false, &schemafile);
  flatbuffers::Parser parser;
  parser.opts.strict_json = true;
  bool ok = parser.Parse(schemafile.c_str());
  if (!ok)
    throw;
  std::string jsongen;
  GenerateText(parser, builder->GetBufferPointer(), &jsongen);
  return jsongen;
}
//------------------------------------------------------------------------
// preconditions:
//      schemafile present and located at "schemafile"
//      "jsonfile" is a valid path
//      *builder is instance of a metadata flatbuffer
// postconditons:
//      a json version of the metadata flatbuffer at *builder is saved at
//      jsonfile
//------------------------------------------------------------------------
[[maybe_unused]] static void
populateMessage(std::string schemafile, std::string jsonfile,
                flatbuffers::FlatBufferBuilder *builder) {
  std::string jsongen = populateMessage(schemafile, builder);
  flatbuffers::SaveFile(jsonfile.c_str(), jsongen.c_str(), jsongen.length(),
                        false);
}
//------------------------------------------------------------------------
// purpose: find out if flatSchemaField is in the message
// precondition: flatSchemaField : introspection information about a message
//    field
//@param message : an instance of an metadata_flatbuffer
//@param returnvalue: true if the flatSchemaField is in the message, false
// otherwise
//------------------------------------------------------------------------

static bool isInMessage(const json &flatSchemaField, json message) {
  nlohmann::json &pointer = message;
  if (flatSchemaField.size() < 2)
    return false;
  // outer iterator over each successive flat schema field
  for (size_t i = 1; i < flatSchemaField.size(); i++) {
    //[0] is the name of the field.  [1] is the field's position in it's table
    //[2] is the type
    const std::string name = flatSchemaField[i][0];
    if (pointer.contains(name)) { // field containing the same name means it's
                                  // in there somewhere
      for (auto &it : pointer.items()) {
        if (it.key() == name) {
          pointer = it.value(); // this algorithm searches each following name
          break;
        }
      }
    } else if (pointer.is_array()) { // array outer names ignored
      continue;
    } else {
      return false;
    } //
  }
  return true;
}
//------------------------------------------------------------------------
// purpose: generate the requiredField[] as a string of names separated by
// seperator example output: AicMetadataFlat_Metadata->dmaRequests->portId
// precondition: schemaEntry : introspection information about a message field.
//@param seperatorOuter: some string to use
//    as a spacer
// returnvalue: string that describes the introspection information as a 1-line
// string
//------------------------------------------------------------------------

static std::string generateFieldString(const json &schemaEntry,
                                       const std::string &seperatorOuter) {
  std::string returnval;
  for (size_t i = 0; i < schemaEntry.size(); i++) {
    std::string sentence = schemaEntry[i][0];
    returnval += (sentence + seperatorOuter);
  }
  return returnval.substr(0, returnval.length() - seperatorOuter.length());
}
//------------------------------------------------------------------------
// purpose: read the fields contained in the text file majorSchemaFilename
// precondition: text file majorSchemaFilename is valid and contains fields
// returnvalue: vector of the strings in majorSchemaFilename text file
//------------------------------------------------------------------------
[[maybe_unused]] static std::vector<std::string>
getMajorSchemaFields(const std::string &majorSchemaFilename) {
  std::vector<std::string> majorSchemaFields;
  std::ifstream textfile(majorSchemaFilename);
  if (textfile.is_open()) {
    std::string line;
    while (std::getline(textfile, line)) {
      majorSchemaFields.push_back(line.substr(0, line.length() - 1));
    }
    textfile.close();
  }
  return majorSchemaFields;
}
//------------------------------------------------------------------------
// purpose: filter fields from candidateSchema that are in baseSchema
// precondition: baseSchema, candidateSchema are vectors of strings
// returnvalue: candidateSchema without any strings that were in baseSchema
//------------------------------------------------------------------------
[[maybe_unused]] static std::vector<std::string>
filterBaseVersionFields(const std::vector<std::string> &candidateSchema,
                        const std::vector<std::string> &baseSchema) {
  std::vector<std::string> returnval;
  for (std::string field : candidateSchema) {
    if (!std::any_of(baseSchema.begin(), baseSchema.end(),
                     [field](std::string i) { return i == field; })) {
      returnval.push_back(field);
    }
  }
  return returnval;
}
//------------------------------------------------------------------------
// purpose: find the schema fields used in this instance of metadataFlat
// precondition: messageJsonPath, schemaPath are valid json filepaths
// returnvalue: vector of strings of each requiredField
//------------------------------------------------------------------------
[[maybe_unused]] static std::vector<std::string>
generateRequiredFields(const std::string &messageJsonPath,
                       const std::string &schemaPath) {
  std::ifstream m(messageJsonPath);
  json message_json = json::parse(m);
  std::ifstream s(schemaPath);
  json schema_json = json::parse(s);
  std::vector<std::string> requiredFields;
  for (auto &i : schema_json) {
    if (isInMessage(i, message_json)) {
      requiredFields.push_back(generateFieldString(i, "->"));
    }
  }
  return requiredFields;
}
} // namespace metadataFlat

#endif // end #ifndef C_DECODER_TESTING
namespace metadata {

class FlatEncode {
public:
  //------------------------------------------------------------------------
  // purpose: unread (raw) aicMetadata bytes -> flatbuf
  //------------------------------------------------------------------------

  static const std::vector<uint8_t>
  aicMetadataRawTranslateFlatbuff(std::vector<uint8_t> &aicMetadata_bytes) {
    std::array<char, 0x100> metadataError;
    auto original_metadata =
        MDR_readMetadata(aicMetadata_bytes.data(), aicMetadata_bytes.size(),
                         metadataError.data(), metadataError.size());
    if (original_metadata == nullptr) {
      std::cout << "Error parsing AICmetadata" << metadataError.data();
      throw;
    }
    return {
      (uint8_t*)original_metadata,
      (uint8_t*)original_metadata +
      aicMetadata_bytes.size()
    };
  }

  //------------------------------------------------------------------------
  // purpose: same as other version, but returns an STL vector instead
  //------------------------------------------------------------------------

  static const std::vector<uint8_t>
  aicMetadataTranslateFlatbuff(const std::vector<uint8_t> &aic_metadata) {
    std::vector<std::string> requiredFields;
    auto *data = (AICMetadata *)&aic_metadata[0];
    auto temp = aicMetadataTranslateFlatbuff(*data, aic_metadata.size(),
                                             requiredFields);
    std::vector<uint8_t> returnval(temp.GetBufferPointer(),
                                   temp.GetBufferPointer() + temp.GetSize());
    return returnval;
  }

  //------------------------------------------------------------------------
  // purpose: same as other version, but returns an STL vector instead
  //------------------------------------------------------------------------
  static const std::vector<uint8_t>
  aicMetadataTranslateFlatbuff(const std::vector<uint8_t> &aic_metadata,
                               const std::vector<std::string> &requiredFields) {
    auto temp = aicMetadataTranslateFlatbuff(
        *(AICMetadata *)&aic_metadata[0], aic_metadata.size(), requiredFields);
    std::vector<uint8_t> returnval(temp.GetBufferPointer(),
                                   temp.GetBufferPointer() + temp.GetSize());
    return returnval;
  }

  //------------------------------------------------------------------------
  // purpose: translate an AICMetadata* to the flatbuffer version of the same
  // precondition: valid AICMetadata* supplied, the external byte length of
  // AICMetadata*, and location of text file
  //        with the required_fields strings, one per line
  // returnvalue: populated metadata_flatbuffer
  //------------------------------------------------------------------------

  static flatbuffers::FlatBufferBuilder
  aicMetadataTranslateFlatbuff(AICMetadata data,
                               uint64_t raw_struct_version_length,
                               const std::string &required_fields_filename) {
    std::vector<std::string> requiredFields;
    std::ifstream textfile(required_fields_filename);
    if (textfile.is_open()) {
      std::string line;
      while (std::getline(textfile, line)) {
        requiredFields.push_back(line);
      }
      textfile.close();
    }
    return aicMetadataTranslateFlatbuff(data, raw_struct_version_length,
                                        requiredFields);
  }

  //------------------------------------------------------------------------
  // purpose: same as other version, but required_fields_filename is not
  // required
  //------------------------------------------------------------------------
  static flatbuffers::FlatBufferBuilder
  aicMetadataTranslateFlatbuff(AICMetadata data,
                               uint64_t raw_struct_version_length) {
    std::vector<std::string> requiredFields;
    return aicMetadataTranslateFlatbuff(data, raw_struct_version_length,
                                        requiredFields);
  }

private:
  //------------------------------------------------------------------------
  // Precondition:  *aic_metadata contains NSPMulticast fields not in
  // metadata_as_flat Postcondition: metadata_as_flat contains the same fields
  //------------------------------------------------------------------------
  static auto translateRequiredFields(std::vector<std::string> requiredFields,
                                      flatbuffers::FlatBufferBuilder &builder) {

    std::vector<flatbuffers::Offset<flatbuffers::String>> FlatV;
    for (std::string field : requiredFields) {
      auto flatstring = builder.CreateString(field);
      FlatV.push_back(flatstring);
    }
    return builder.CreateVector(FlatV);
  }

  //------------------------------------------------------------------------
  // Precondition:  *aic_metadata contains semaphore fields not in
  // metadata_as_flat Postcondition: metadata_as_flat contains the same fields
  //------------------------------------------------------------------------
  static auto translateSemaphoreInitState(const AICMetadata &metadata) {
    std::vector<uint32_t> returnval;
    for (int i = 0, e = metadata.numSemaphores; i < e; ++i)
      returnval.push_back(metadata.semaphoreInitState[i]);
    return returnval;
  }

  //------------------------------------------------------------------------
  // Precondition:  *aic_metadata contains L2TCMInitState fields not in
  // metadata_as_flat Postcondition: metadata_as_flat contains the same fields
  //------------------------------------------------------------------------
  static auto translateL2TCMInitState(const AICMetadata &metadata) {
    std::vector<uint8_t> returnval;
    for (int i = 0, e = metadata.L2TCMInitSize; i != e; ++i) {
      returnval.push_back(((uint8_t *)metadata.L2TCMInitState)[i]);
    }
    return returnval;
  }

  //------------------------------------------------------------------------
  // Precondition:  *aic_metadata contains SemaphoreOp fields not in
  // metadata_as_flat Postcondition: metadata_as_flat contains the same fields
  //------------------------------------------------------------------------
  static auto translateSemaphoreOp(const AICMDSemaphoreOp *s,
                                   flatbuffers::FlatBufferBuilder &builder) {
    auto thisop = AicMetadataFlat::CreateAICMDSemaphoreOp(
        builder, s->semNum, s->semValue, s->semOp, s->preOrPost, s->inSyncFence,
        s->outSyncFence);
    return thisop;
  }

  //------------------------------------------------------------------------
  // Precondition:  *aic_metadata contains AICMDDoorbellOp fields not in
  // metadata_as_flat Postcondition: metadata_as_flat contains the same fields
  //------------------------------------------------------------------------
  static auto translateDoorbellOp(const AICMDDoorbellOp *d,
                                  flatbuffers::FlatBufferBuilder &builder) {
    auto thisop = AicMetadataFlat::CreateAICMDDoorbellOp(
        builder, d->offset, d->data, d->mcId, d->size);
    return thisop;
  }

  //------------------------------------------------------------------------
  // Precondition:  *aic_metadata contains ThreadDescriptors fields not in
  // metadata_as_flat Postcondition: metadata_as_flat contains the same fields
  //------------------------------------------------------------------------
  static auto
  translateThreadDescriptors(const AICMetadata &metadata,
                             flatbuffers::FlatBufferBuilder &builder) {
    std::vector<flatbuffers::Offset<AicMetadataFlat::AICMDThreadDescriptor>>
        returnval;
    for (int td = 0, tde = metadata.numThreadDescriptors; td < tde; ++td) {
      const AICMDThreadDescriptor *desc = &metadata.threadDescriptors[td];
      auto AICMDThreadDescript = AicMetadataFlat::CreateAICMDThreadDescriptor(
          builder, (uint64_t)desc->entryPoint, desc->typeMask);
      returnval.push_back(AICMDThreadDescript);
    }
    return returnval;
  }

  //------------------------------------------------------------------------
  // Precondition:  *aic_metadata contains onstantMappings fields not in
  // metadata_as_flat Postcondition: metadata_as_flat contains the same fields
  //------------------------------------------------------------------------
  static auto
  translateConstantMappings(const AICMetadata &metadata,
                            flatbuffers::FlatBufferBuilder &builder) {
    std::vector<flatbuffers::Offset<AicMetadataFlat::AICMDConstantMapping>>
        returnval;
    for (int cm = 0, cme = metadata.numConstantMappings; cm < cme; ++cm) {
      const AICMDConstantMapping *mapping = &metadata.constantMappings[cm];
      auto AICMDConstantMap = AicMetadataFlat::CreateAICMDConstantMapping(
          builder, mapping->constantDataBaseOffset, mapping->coreMask,
          mapping->size);
      returnval.push_back(AICMDConstantMap);
    }
    return returnval;
  }

  //------------------------------------------------------------------------
  // Precondition:  *aic_metadata contains hostMulticastEntries fields not in
  // metadata_as_flat Postcondition: metadata_as_flat contains the same fields
  //------------------------------------------------------------------------
  static auto
  translatehostMulticastEntries(const AICMetadata &data,
                                flatbuffers::FlatBufferBuilder &builder) {
    std::vector<flatbuffers::Offset<AicMetadataFlat::AICMDHostMulticastEntry>>
        HostMulticastEntriesV;
    AICMDHostMulticastEntryTable *table = data.hostMulticastTable;
    for (int mcid = 0, e = table->numMulticastEntries; mcid != e; ++mcid) {
      AICMDHostMulticastEntry *entry = &table->multicastEntries[mcid];
      auto anentry = AicMetadataFlat::CreateAICMDHostMulticastEntry(
          builder, entry->mask, entry->size);
      HostMulticastEntriesV.push_back(anentry);
    }
    auto hostMulticastTable =
        AicMetadataFlat::CreateAICMDHostMulticastEntryTable(
            builder, builder.CreateVector(HostMulticastEntriesV));

    return hostMulticastTable;
  }

  //------------------------------------------------------------------------
  // Precondition:  *aic_metadata contains DMARequests fields not in
  // metadata_as_flat Postcondition: metadata_as_flat contains the same fields
  //------------------------------------------------------------------------
  static auto translateDMARequests(const AICMetadata &metadata,
                                   flatbuffers::FlatBufferBuilder &builder) {
    std::vector<flatbuffers::Offset<AicMetadataFlat::AICMDDMARequest>>
        AICMDDMARequestV;
    for (int i = 0; i < metadata.numDMARequests; ++i) {
      auto r = &(metadata.dmaRequests[i]);

      std::vector<flatbuffers::Offset<AicMetadataFlat::AICMDSemaphoreOp>>
          semaphoreOps;
      for (int j = 0; j < r->numSemaphoreOps; ++j)
        semaphoreOps.push_back(
            translateSemaphoreOp(&r->semaphoreOps[j], builder));
      auto semaphoreOpsV = builder.CreateVector(semaphoreOps);
      if (semaphoreOps.empty())
        semaphoreOpsV = 0;
      std::vector<flatbuffers::Offset<AicMetadataFlat::AICMDDoorbellOp>>
          doorbellOps;
      for (int j = 0; j < r->numDoorbellOps; ++j)
        doorbellOps.push_back(translateDoorbellOp(&r->doorbellOps[j], builder));
      auto doorbellOpsV = builder.CreateVector(doorbellOps);
      if (doorbellOps.empty())
        doorbellOpsV = 0;
      auto foo = AicMetadataFlat::CreateAICMDDMARequest(
          builder, semaphoreOpsV, doorbellOpsV, r->hostOffset, r->devOffset,
          r->size, r->num, r->mcId, r->devAddrSpace, r->inOut, r->portId);

      AICMDDMARequestV.push_back(foo);
    }
    auto dmaRequests = builder.CreateVector(AICMDDMARequestV);
    return dmaRequests;
  }

  //------------------------------------------------------------------------
  // Precondition:  *aic_metadata contains NSPMulticast fields not in
  // metadata_as_flat Postcondition: metadata_as_flat contains the same fields
  //------------------------------------------------------------------------
  static auto
  translateNSPMulticastTables(const AICMetadata &metadata,
                              flatbuffers::FlatBufferBuilder &builder) {
    std::vector<
        flatbuffers::Offset<AicMetadataFlat::AICMDNSPMulticastEntryTable>>
        nspMulticastTablesV;
    for (int core = 0; core < metadata.numNSPs; ++core) {
      AICMDNSPMulticastEntryTable *table = &metadata.nspMulticastTables[core];
      std::vector<flatbuffers::Offset<AicMetadataFlat::AICMDNSPMulticastEntry>>
          MulticastEntries;
      for (int mcid = 0, e = table->numMulticastEntries; mcid != e; ++mcid) {
        AICMDNSPMulticastEntry *entry = &table->multicastEntries[mcid];
        auto flatentry = AicMetadataFlat::CreateAICMDNSPMulticastEntry(
            builder, entry->baseAddrOffset, entry->mask, entry->size,
            entry->addrSpace, entry->dynamic);
        MulticastEntries.push_back(flatentry);
      }
      auto AICMDNSPMulticastEntryTableV =
          builder.CreateVector(MulticastEntries);
      auto foo = AicMetadataFlat::CreateAICMDNSPMulticastEntryTable(
          builder, AICMDNSPMulticastEntryTableV);
      nspMulticastTablesV.push_back(foo);
    }
    auto nspMulticastTables = builder.CreateVector(nspMulticastTablesV);
    return nspMulticastTables;
  }
  //------------------------------------------------------------------------
  // Precondition:  *aic_metadata contains simple scalar fields not in
  // metadatabuilder Postcondition: metadatabuilder contains the same fields
  //------------------------------------------------------------------------
  static void
  translateScalars(const AICMetadata &metadata,
                   AicMetadataFlat::MetadataBuilder &metadatabuilder) {
    metadatabuilder.add_versionMajor(metadata.versionMajor);
    metadatabuilder.add_versionMinor(metadata.versionMinor);
    metadatabuilder.add_staticSharedDDRSize(metadata.staticSharedDDRSize);
    metadatabuilder.add_dynamicSharedDDRSize(metadata.dynamicSharedDDRSize);
    metadatabuilder.add_staticConstantsSize(metadata.staticConstantsSize);
    metadatabuilder.add_dynamicConstantsSize(metadata.dynamicConstantsSize);
    metadatabuilder.add_exitDoorbellOffset(metadata.exitDoorbellOffset);
    metadatabuilder.add_l2CachedDDRSize(metadata.l2CachedDDRSize);
    metadatabuilder.add_L2TCMInitSize(metadata.L2TCMInitSize);
    metadatabuilder.add_execContextMajorVersion(
        metadata.execContextMajorVersion);
    metadatabuilder.add_numNSPs(metadata.numNSPs);
    metadatabuilder.add_numSemaphores(metadata.numSemaphores);
    metadatabuilder.add_hwVersionMajor(metadata.hwVersionMajor);
    metadatabuilder.add_hwVersionMinor(metadata.hwVersionMinor);
    metadatabuilder.add_staticSharedDDRECCEnabled(
        metadata.staticSharedDDRECCEnabled);
    metadatabuilder.add_dynamicSharedDDRECCEnabled(
        metadata.dynamicSharedDDRECCEnabled);
    metadatabuilder.add_staticConstantsECCEnabled(
        metadata.staticConstantsECCEnabled);
    metadatabuilder.add_dynamicConstantsECCEnabled(
        metadata.dynamicConstantsECCEnabled);
    metadatabuilder.add_singleVTCMPage(metadata.singleVTCMPage);
    metadatabuilder.add_VTCMSize(metadata.VTCMSize);
    metadatabuilder.add_L2TCMSize(metadata.L2TCMSize);
    metadatabuilder.add_networkHeapSize(metadata.networkHeapSize);
    metadatabuilder.add_hasHmxFP(metadata.hasHmxFP);
    metadatabuilder.add_hasHvxFP(metadata.hasHvxFP);
  }
  //------------------------------------------------------------------------
  // Precondition:  *aic_metadata contains a set of fields fields not in
  // metadata_as_flat Postcondition: metadata_as_flat contains the same fields
  //------------------------------------------------------------------------
  static flatbuffers::FlatBufferBuilder
  aicMetadataTranslateFlatbuff(const AICMetadata &metadata,
                               uint64_t raw_struct_version_length,
                               const std::vector<std::string> &requiredFields) {

    flatbuffers::FlatBufferBuilder builder;
    builder.ForceDefaults(true);
    auto name = builder.CreateString(metadata.networkName);
    auto requiredFieldsFlat = translateRequiredFields(requiredFields, builder);
    auto semaphoreInitState =
        builder.CreateVector(translateSemaphoreInitState(metadata));
    auto L2TCMInitState =
        builder.CreateVector(translateL2TCMInitState(metadata));

    auto dmaRequests = translateDMARequests(metadata, builder);

    auto nspMulticastTables = translateNSPMulticastTables(metadata, builder);

    auto hostMulticastTable = translatehostMulticastEntries(metadata, builder);

    auto ThreadDescriptors =
        builder.CreateVector(translateThreadDescriptors(metadata, builder));

    auto AICMDConstantMappings =
        builder.CreateVector(translateConstantMappings(metadata, builder));
    AicMetadataFlat::MetadataBuilder metadatabuilder(builder);

    translateScalars(metadata, metadatabuilder);

    metadatabuilder.add_requiredFields(requiredFieldsFlat);
    metadatabuilder.add_networkName(name);
    metadatabuilder.add_semaphoreInitState(semaphoreInitState);
    metadatabuilder.add_L2TCMInitState(L2TCMInitState);
    if (metadata.numDMARequests != 0)
      metadatabuilder.add_dmaRequests(dmaRequests);
    metadatabuilder.add_nspMulticastTables(nspMulticastTables);
    metadatabuilder.add_hostMulticastTable(hostMulticastTable);
    if (metadata.numThreadDescriptors != 0)
      metadatabuilder.add_threadDescriptors(ThreadDescriptors);
    if (metadata.numConstantMappings != 0)
      metadatabuilder.add_constantMappings(AICMDConstantMappings);
    metadatabuilder.add_raw_struct_version_length(raw_struct_version_length);

    auto AICMetadata_flat = metadatabuilder.Finish();
    builder.Finish(AICMetadata_flat);
    return builder;
  }
};

} // namespace metadata
#endif // FLATBUFF_DRIVER_FLATBUF_ENCODE_H
