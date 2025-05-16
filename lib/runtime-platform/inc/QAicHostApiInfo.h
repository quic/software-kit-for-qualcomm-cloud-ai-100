// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
#ifndef QAIC_HOST_API_INFO_H
#define QAIC_HOST_API_INFO_H

/**
 * @file host_api_status_query.h
 * @brief Data structures used to define the content of the QHI payload for NNC
 * status query.
 * @copyright
 * Copyright (c) 2019 by Qualcomm Technologies, Inc.
 * All Rights Reserved. Confidential and Proprietary - Qualcomm Technologies,
 * Inc..
 */

#define StatusQuerySerialSize 16
#define StatusQueryBoardSerialSize 32
#define fwQCImageVersionStringSize 38
#define fwOEMImageVersionStringSize 32
#define fwImageVariantStringSize 16

// Should be in sync with same symbol in SBL
#define SHARED_IMEM_SBL_IMAGE_NAME_LEN 0x18

// This value must match when receiving the results
#define deviceInfoFormatVersion 12

/*****************************************************************************
 *  All the NNC protocol structures should end on a 8byte boundary and all   *
 *  its structure members must be naturally aligned to itself. It is the     *
 *  responsibility of the maintainer/programmer to make sure that the above  *
 *  criterias are met while updating the NNC protocol structures.            *
 * ***************************************************************************/

typedef struct host_api_info_data_header_internal {
  /// Version format for this structure, this must be the first field
  uint8_t format_version;
  uint8_t padding[7];
} __attribute__((packed)) host_api_info_data_header_internal_t;

typedef struct {
  // Total DDR in MB
  uint32_t dramTotal;
  // Free DDR in MB
  uint32_t dramFree;
  // DDR Fragmentation status = 100*(1 - largest_free_block/free_bytes)
  //  0:no frag, 100:high frag
  uint32_t fragmentationStatus;
  // Total number of virtual channels
  uint8_t vcTotal;
  // Number of free virtual channels
  uint8_t vcFree;
  // Total number of NSPs
  uint8_t nspTotal;
  // Number of free NSPs
  uint8_t nspFree;
  // Number of MCid
  uint32_t mcidTotal;
  // Free MCIDs
  uint32_t mcidFree;
  // Total Semaphore count
  uint32_t semTotal;
  // Free Semaphore count
  uint32_t semFree;
  // Total number of physical channels
  uint8_t pcTotal;
  // Number of dedicated physical channels
  uint8_t pcReserved;
  uint8_t padding[6];
} __attribute__((packed)) host_api_resource_info_internal_t;

typedef struct {
  /// NSP Frequency;
  uint32_t nspFrequencyHz;
  /// DDR Frequency;
  uint32_t ddrFrequencyHz;
  /// COMPNOC Frequency;
  uint32_t compFrequencyHz;
  /// MEMNOC Frequency;
  uint32_t memFrequencyHz;
  /// SYSNOC Frequency;
  uint32_t sysFrequencyHz;
  uint32_t padding;
  /// Peak compute on device in ops/second. Assumes all ops are in int8.
  uint64_t peakCompute;
  /// Memory bandwidth from DRAM on device in Kbytes/second, averaged over last
  /// ~100 ms.
  uint64_t dramBw;
  uint64_t reserved1;
  uint64_t reserved2;
} __attribute__((packed)) host_api_performance_info_internal_t;

/// Data structure returned to from device in status query
/// Message format NNC_STATUS_QUERY_DEV
typedef struct {
  /// Firmware version

  uint8_t fwVersion_major;
  uint8_t fwVersion_minor;
  uint8_t fwVersion_patch;
  uint8_t fwVersion_build;

  char fwQCImageVersionString[fwQCImageVersionStringSize];
  char fwOEMImageVersionString[fwQCImageVersionStringSize];
  char fwImageVariantString[fwImageVariantStringSize];

  /// Hardware version
  uint32_t hwVersion;

  /// Serial number
  char serial[StatusQuerySerialSize];

  /// Board serial number
  char board_serial[StatusQueryBoardSerialSize];

  /// The highest compiler version supported by the library
  uint32_t compilerVersion;

  host_api_resource_info_internal_t resource_info;
  host_api_performance_info_internal_t perf_info;

  /// Number of loaded constants
  uint32_t numLoadedConsts;
  /// Number of constants in-use
  uint32_t numConstsInUse;
  /// Number of loaded networks
  uint8_t numLoadedNWs;
  /// Number of active networks
  uint8_t numActiveNWs;

  /// Metadata version
  uint16_t metaVerMaj;
  uint16_t metaVerMin;

  /// API version
  uint16_t nncCommandProtocolMajorVersion;
  uint16_t nncCommandProtocolMinorVersion;

  /* SBL Image Version Name */
  char sbl_image_name[SHARED_IMEM_SBL_IMAGE_NAME_LEN];

  /// Reset required to retire additional DDR pages
  uint8_t need_reset_to_retire_pages;
  uint8_t padding[1];

  /* PVS Image Version Number */
  uint32_t pvs_image_version;
  /* NSP Defective PG Mask */
  uint64_t nsp_pgmask;
  /*TimeSync offset in microsconds*/

  uint64_t qts_time_offset_usecs;

  /// DDR pages that have been retired
  uint32_t num_retired_pages;

  // SKU type
  uint8_t sku_type;
  // Complex ID
  uint8_t complex_id;
  // Padding
  uint8_t padding1[2];
} __attribute__((packed)) host_api_info_dev_data_internal_t;

/// Message Format Type NNC_STATUS_QUERY_NSP
/**
 * Data returned in NSP query version response
 */

#define nnncQCImageVersionStringSize 64
#define nnncOEMImageVersionStringSize 48
#define nnncImageVariantStringSize 48

typedef struct host_api_info_nsp_static_data_internal {
  uint8_t major;
  uint8_t minor;
  uint8_t patch;
  uint8_t build;
  uint32_t padding;
  char qc[nnncQCImageVersionStringSize];
  char oem[nnncOEMImageVersionStringSize];
  char variant[nnncImageVariantStringSize];
} __attribute__((packed)) host_api_info_nsp_static_data_internal_t;

/// Data structure returned to from device in status query
/// Message format NNC_STATUS_QUERY_NSP
typedef struct {
  host_api_info_nsp_static_data_internal_t nsp_fw_version;
} __attribute__((packed)) host_api_info_nsp_data_internal_t;

// This structure should be used by the host to copy all the responses
// into one combined status
typedef struct {
  host_api_info_data_header_internal_t header;
  host_api_info_dev_data_internal_t dev_data;
  host_api_info_nsp_data_internal_t nsp_data;
} __attribute__((packed)) host_api_info_data_internal_t;

#endif
