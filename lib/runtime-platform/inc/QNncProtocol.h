// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QNNCPROTOCOL_H
#define QNNCPROTOCOL_H

#include "QAicHostApiInfo.h"
#include "QAicRuntimeTypes.h"
#include "QTypes.h"
#include "dev/aic100/qaic_accel.h"
#include <stdint.h>

#define NNC_MAX_STRING_LENGTH 64
#define NNC_MAX_MSG_LENGTH 256
#define NNC_MAGIC_NUMBER 0x43494151

/** NNC interface (TRANSACTION Protocol) between KMD <--> FW */
#define NNC_TRANSACTION_PROTOCOL_MAJOR_VERSION (5)
#define NNC_TRANSACTION_PROTOCOL_MINOR_VERSION (5)

/** NNC interface (COMMAND Protocol) between LRT <--> FW (using KMD passthrough
 * messages) */
#define NNC_COMMAND_PROTOCOL_MAJOR_VERSION (17)
#define NNC_COMMAND_PROTOCOL_MINOR_VERSION (0)

/**
 * LRT lib and KMD exchange one NNC message at a time.
 * Each NNC message is composed of one or more transactions.
 * A transaction of type PASSTHROUGH encapsulates one and only one command.
 * The high level tiering is like this:
 *    nnc_msg
 *       |-- nnc_transaction
 *               |-- nnc_command (PASSTHROUGH transaction only)
 *
 * There is only one nnc_msg defined. One nnc_msg could contain more then one
 * transactions. One PASSTHROUGH transaction can have only one nnc_command.
 */
typedef enum nnc_transaction_type {
  NNC_TRANSACTION_TYPE_PASSTHROUGH_UK = 1,
  NNC_TRANSACTION_TYPE_PASSTHROUGH_KU = 2,
  NNC_TRANSACTION_TYPE_DMA_XFER_UK = 5,
  NNC_TRANSACTION_TYPE_SETUP_VC_UK = 7,
  NNC_TRANSACTION_TYPE_TEARDOWN_VC_UK = 10,
  NNC_TRANSACTION_TYPE_STATUS_UK = 12,
  NNC_TRANSACTION_TYPE_STATUS_KU = 13,
} nnc_transaction_type_t;

typedef enum nnc_transaction_device_flag_bitpos_type {
  NNC_DEVICE_FEATURE_FLAG_AUTO_SKU_BITPOS = 0,
  NNC_DEVICE_FEATURE_FLAG_DPS_FLAG_BIT = 1,
  NNC_DEVICE_FEATURE_FLAG_HBE_FLAG_BIT = 2,
  NNC_DEVICE_FEATURE_FLAG_HBOOT_FLAG_BIT = 3,
  NNC_DEVICE_FEATURE_FLAG_SSR_FLAG_BIT = 4,
  NNC_DEVICE_FEATURE_FLAG_MQ_FLAG_BIT = 5,
  NNC_DEVICE_FEATURE_FLAG_ECC_DDR_FLAG_BIT = 6,
  NNC_DEVICE_FEATURE_PAGE_RET_FLAG_BIT = 7,
  NNC_DEVICE_FEATURE_SECURE_BOOT_FLAG_BIT = 8,
  NNC_DEVICE_FEATURE_SOC_DEBUG_FLAG_BIT = 9,
  NNC_DEVICE_FEATURE_FLAG_NUM_BITS
} nnc_transaction_device_flag_bitpos_type;

/// SKU Type
typedef enum nnc_transaction_sku_query_type {
  NNC_TRANSACTION_SKU_TYPE_DEV_INVALID_SKU = 0,
  NNC_TRANSACTION_SKU_TYPE_DEV_M_2_SKU,
  NNC_TRANSACTION_SKU_TYPE_DEV_PCIE_SKU,
  NNC_TRANSACTION_SKU_TYPE_DEV_PCIE_PRO_SKU,
  NNC_TRANSACTION_SKU_TYPE_DEV_PCIE_LITE_SKU,
  NNC_TRANSACTION_SKU_TYPE_DEV_PCIE_ULTRA_SKU,
  NNC_TRANSACTION_SKU_TYPE_DEV_AUTO_SKU,
  NNC_TRANSACTION_SKU_TYPE_DEV_PCIE_ULTRA_PLUS_SKU,
  NNC_TRANSACTION_SKU_TYPE_DEV_PCIE_080_SKU,
  NNC_TRANSACTION_SKU_TYPE_DEV_PCIE_ULTRA_080_SKU,
  NNC_TRANSACTION_SKU_TYPE_DEV_MAX_SKU,
  NNC_TRANSACTION_SKU_TYPE_ENUM_MAX = UINT8_MAX,
} nnc_transaction_sku_query_type_t;

/**
 * nnc_commands contains information exchanged between QSM and LRT lib.
 * These commands are transparent to kernel mode driver, and are encapsulated in
 * PASSTHROUGH transactions.
 */
typedef enum nnc_command_type {
  NNC_COMMAND_TYPE_UNDEFINED = 0,
  NNC_COMMAND_TYPE_LOAD_CONSTANTS_REQ = 1,
  NNC_COMMAND_TYPE_LOAD_CONSTANTS_RESP = 2,
  NNC_COMMAND_TYPE_LOAD_ELF_REQ = 3,
  NNC_COMMAND_TYPE_LOAD_ELF_RESP = 4,
  NNC_COMMAND_TYPE_UNLOAD_CONSTANTS_REQ = 5,
  NNC_COMMAND_TYPE_UNLOAD_CONSTANTS_RESP = 6,
  NNC_COMMAND_TYPE_UNLOAD_ELF_REQ = 7,
  NNC_COMMAND_TYPE_UNLOAD_ELF_RESP = 8,
  NNC_COMMAND_TYPE_ACTIVATE_REQ = 9,
  NNC_COMMAND_TYPE_ACTIVATE_RESP = 10,
  NNC_COMMAND_TYPE_DEACTIVATE_REQ = 11,
  NNC_COMMAND_TYPE_DEACTIVATE_RESP = 12,
  NNC_COMMAND_TYPE_STATUS_REQ = 13,
  NNC_COMMAND_TYPE_STATUS_RESP = 14,
  NNC_COMMAND_TYPE_LOAD_CONSTANTS_EX_REQ = 15,
  NNC_COMMAND_TYPE_LOAD_CONSTANTS_PAYLOAD_REQ = 16,
  NNC_COMMAND_TYPE_TERMINATE_REQ = 17,
  NNC_COMMAND_TYPE_TERMINATE_RESP = 18,
  NNC_COMMAND_TYPE_RESERVE_VC_REQ = 19,
  NNC_COMMAND_TYPE_RESERVE_VC_RESP = 20,
  NNC_COMMAND_TYPE_RELEASE_VC_REQ = 22,
  NNC_COMMAND_TYPE_RELEASE_VC_RESP = 23,
  NNC_COMMAND_TYPE_AC_RESERVATION_REQ = 24,
  NNC_COMMAND_TYPE_AC_RESERVATION_RESP = 25,
  NNC_COMMAND_TYPE_AC_RELEASE_REQ = 26,
  NNC_COMMAND_TYPE_AC_RELEASE_RESP = 27,
  NNC_COMMAND_TYPE_CREATE_RESOURCE_GROUP_REQ = 28,
  NNC_COMMAND_TYPE_CREATE_RESOURCE_GROUP_RESP = 29,
  NNC_COMMAND_TYPE_RELEASE_RESOURCE_GROUP_REQ = 30,
  NNC_COMMAND_TYPE_RELEASE_RESOURCE_GROUP_RESP = 31,
  NNC_COMMAND_TYPE_RESOURCE_INFO_REQ =
      32, //!< Request Info on Resource Reservation
  NNC_COMMAND_TYPE_RESOURCE_INFO_RESP =
      33, //!< Response from Info Request on Resource Reservation
  NNC_COMMAND_TYPE_PERFORMANCE_INFO_REQ =
      34, //!< Request Info on performance stats
  NNC_COMMAND_TYPE_PERFORMANCE_INFO_RESP =
      35, //!< Response from Info Request on performance
  NNC_COMMAND_TYPE_ACTIVATE_STATE_CMD_REQ =
      36, //!< State Update Command Request for an Activation
  NNC_COMMAND_TYPE_ACTIVATE_STATE_CMD_RESP =
      37, //!< Returns the result of the acitvate state command request
  NNC_COMMAND_TYPE_DEVICE_CONFIG_REQ =
      38, //!< Request to change Device Configuration
  NNC_COMMAND_TYPE_DEVICE_CONFIG_RESP =
      39, //!< Response to Device Config request
  NNC_COMMAND_TYPE_UPDATE_AC_RESERVATION_REQ =
      44, //!< Request to update resources of Activation Reservation
  NNC_COMMAND_TYPE_UPDATE_AC_RESERVATION_RESP =
      45, //!< Response to update Activation Reservation Request
  NNC_COMMAND_TYPE_MAX = 50, //!< Maximum number of defined request codes
} nnc_command_type_t;

enum nnc_status_code {
  NNC_STATUS_PA_L2_INIT_ERROR = -118,          //!< L2Init mapping failed
  NNC_STATUS_PA_GSM_CONF_MMAP_ERROR = -117,    //!< GSM mapping failed
  NNC_STATUS_ERROR_NO_NW_TO_DEACTIVATE = -116, //!< Nw already deactivated
  NNC_STATUS_PD_MMC_MAP_ERROR = -115, //!< MMC mapping failed in deactivate
  NNC_STATUS_DE_CLN_ACK_FAILED_ERROR =
      -114, //!< Wait from User PD failed in Deactivate
  NNC_STATUS_DE_MQ_SEND_FAILED_ERROR =
      -113, //!< Failed to send msg to PD in Deactivate
  NNC_STATUS_M_WAIT_ON_S_SYNC_CMD_TIMEOUT_ERROR =
      -112, //!< Master NSP Timeout while waiting for CMD/State
  NNC_STATUS_M_WAIT_ON_S_SYNC_CMD_FAILED_ERROR =
      -111, //!< Master NSP Timeout while waiting for CMD/State
  NNC_STATUS_PA_MCAST_RANGE_CHANGE_ERROR =
      -110, //!< Failed to change mcast range
  NNC_STATUS_PA_SET_MCAST_ERROR =
      -109, //!< Failed to set Mcast range in Pre-activate
  NNC_STATUS_BW_MON_MMAP_ERROR = -108, //!< Failed to map bw monitor region
  NNC_STATUS_PA_MN_RSP_W_ERROR = -107,
  NNC_STATUS_NW_MAP_ERROR = -106,       //!< Failed to create nnc mapping
  NNC_STATUS_MMCADDR_MAP_ERROR = -105,  //!< Failed to create mmcaddr mapping
  NNC_STATUS_MQ_SEND_ERROR = -104,      //!< Failed to send MQ
  NNC_STATUS_PD_SIG_WAIT_ERROR = -103,  //!< Wait from PD failed in activate
  NNC_STATUS_VER_MISMATCH_ERROR = -102, //!< NNC exec context version mismatch
  NNC_STATUS_MMAP_ERROR = -101,         //!< Failed to map region
  NNC_STATUS_AC_SYNC_ERROR = -100,
  NNC_STATUS_DPS_NOT_ENABLED = -66, //!< DPS not enabled on device
  NNC_STATUS_NSP_MCID_COUNT_EXCEED_MAX_LIMIT =
      -65, //!< MCID count exceeds more than max supported limit
  NNC_STATUS_PROTO_NOT_SUPPORTED = -64, //!< Invalid protocol type
  NNC_STATUS_GET_ACT_RECORD_ERROR =
      -63, //!< Failed to get activation record from lut
  NNC_STATUS_RETRIEVAL_NSP_4M_METADATA_ERROR =
      -62,                                   //!< Parse NSP from metadata failed
  NNC_STATUS_RETRIEVAL_METADATA_ERROR = -61, //!< Failed to parse metadata
  NNC_SWITCH_CMD_VCID_INVALID = -60,         //!< Invalid vcid
  NNC_STATUS_SWITCH_AC_STATE_ERROR = -59,    //!< Switch cmd sanity check failed
  NNC_STATUS_SWITCH_CMD_SEQ_MISMATCH_ERROR =
      -58, //!< Switch cmd sequence failed
  NNC_STATUS_SWITCH_PARAM_ERROR =
      -57, //!< Switch cmd param error(Invalid number of switch cmd)
  NNC_STATUS_NUMNSP_RETRIEVAL_ERROR =
      -56, //!< Failed to retrieve NSP count from metadata
  NNC_STATUS_VCID_RETRIEVAL_ERROR =
      -55, //!< Failed to retrieve vcid from activation record
  NNC_STATUS_META_RETRIEVAL_ERROR = -54, //!< Failed to parse metadata
  NNC_STATUS_RETRIEVAL_PNSP_ID_ERROR =
      -53, //!< Failed to retrive physical nsp id from ac record
  NNC_STATUS_VC_SETUP_ERROR = -52, //!< Failed to setup dma bridge
  NNC_STATUS_GSM_CONF_ERROR = -51, //!< Failed to configure GSM
  NNC_STATUS_DYN_CONST_RESOURCE_NOT_FOUND =
      -50, //!< Failed to parse dynamic const from metadata
  NNC_STATUS_L2TCM_INIT_STATE_INVALID_ERROR =
      -49, //!< Failed to get L2TCM Meminfo data from metadata
  NNC_STATUS_RETRIEVAL_L2TCM_INIT_STATE_ERROR =
      -48, //!< Failed to get L2TCM Meminfo from metadata
  NNC_STATUS_MCID_CONFIG_ERROR = -47, //!< Failed to configure mcid
  NNC_STATUS_RETRIEVAL_PRO_IMG_ERROR =
      -46, //!< Failed to parse Program ELF info from metadata
  NNC_STATUS_LUT_UPDATE_FAILED_ERROR = -45, //!< Failed to update activation lut
  NNC_STATUS_XPU_ERROR = -44,               //!< XPU program error
  /* FuSa Auto specific errors */
  NNC_STATUS_SAFETY_SWIV_ELF_ERROR =
      -43, //!< Safety SWIV check error on network elf image verification
  NNC_STATUS_SAFETY_SWIV_CONST_ERROR =
      -42, //!< Safety SWIV check error on constant image verification
  NNC_STATUS_SAFETY_SWIV_METADATA_ERROR =
      -41, //!< Safety SWIV check error on metadata image verification
  NNC_STATUS_SAFETY_BCS_MHI_ERROR = -40, //!< Safety BCS check error on MHI path
  NNC_STATUS_SAFETY_BCS_GLINK_ERROR =
      -39, //!< Safety BCS check error on Glink path
  NNC_STATUS_SAFETY_BCS_METADATA_ERROR =
      -38, //!< Safety BCS check error on Metadata corruption
  NNC_STATUS_SAFETY_BCS_AC_RECORD_ERROR =
      -37, //!< Safety BCS check error on Activation record corruption
  NNC_STATUS_SS_UNDER_SSR_ERROR =
      -36, //!< Operation failed due to NSP under SSR
  NNC_STATUS_VC_CONFIGURE_ERROR = -35,     //!< VC configuration/enable error
  NNC_STATUS_QUEUE_TOO_SMALL = -34,        //!< P2P VC queue size too small
  NNC_STATUS_IATU_ALLOC_ERROR = -22,       //!< Failed to allocate IATU resource
  NNC_STATUS_OATU_ALLOC_ERROR = -21,       //!< Failed to allocate OATU resource
  NNC_STATUS_DATABASE_IDS_EXHAUSTED = -20, //!< Database ID's got exhausted
  NNC_STATUS_DATABASE_ID_NOT_FOUND = -19,  //!< Database ID not found in Queue
  NNC_STATUS_RESERVATION_IDS_EXHAUSTED =
      -18,                             //!< Reservation ID's got exhausted
  NNC_STATUS_CMD_LENGTH_ERROR = -17,   //!< Unexpected command length
  NNC_STATUS_NSP_RESPONSE_ERROR = -16, //!< NSP response timeout /error
  NNC_STATUS_SEM_ALLOC_ERROR =
      -15, //!< Failed to allocate GSM semaphore resource
  NNC_STATUS_VC_ALLOC_ERROR = -14,  //!< Failed to allocate virtual channel
  NNC_STATUS_NSP_ALLOC_ERROR = -13, //!< Failed to allocate NSP resources
  NNC_STATUS_MCID_ALLOC_ERROR =
      -12, //!< Failed to allocate multicast ID resources
  NNC_STATUS_RESOURCE_NOT_FOUND = -11, //!< Requested Resource Not found>
  NNC_STATUS_BUFFER_TOO_SMALL = -10,   //!< Buffer too small
  NNC_STATUS_OPERATION_NOT_PERMITTED =
      -9,                          //!< NNC permission denied for invalid handle
                                   //! or attempt to release a resource in use
  NNC_STATUS_NOT_INITIALIZED = -8, //!< NNC not yet initialized.
  NNC_STATUS_TIMED_OUT_WAITING =
      -7, //!< Timed out waiting for a state change to occur.
  NNC_STATUS_PARAMETER_ERROR = -6, //!< Error caused by an invalid parameter
                                   //! passed to an internal function.
  NNC_STATUS_METADATA_ERROR =
      -5, //!< Version mismatch / error processing the ELF metadata
  NNC_STATUS_MEM_ALLOC_ERROR = -4,   //!< Memory resource exhausted.
  NNC_STATUS_UNDEFINED_REQUEST = -3, //!< The request specified is not defined.
  NNC_STATUS_DMA_ERROR =
      -2,                 //!< Configuration of the DMA virtual channel failed.
  NNC_STATUS_ERROR = -1,  //!< General error occurred.
  NNC_STATUS_SUCCESS = 0, //!< The specified request was successful.
};
typedef int32_t nnc_status_code_t;

typedef uint32_t activation_id_type_t;

#define NNC_VC_RESERVATION_ID_DEFAULT (-1)
#define NNC_VC_RESERVATION_ID_SKIP (0xFE)

enum nnc_activation_state_type {
  NNC_ACTIVATION_STATE_CMD_STANDBY =
      0, //!< Transition Activation to Standby -> DCVS UP
  NNC_ACTIVATION_STATE_CMD_READY =
      1, //!< Transition Activation to Ready -> DCVS UP
  NNC_ACTIVATION_STATE_CMD_DEACTIVATE =
      2, //!< Transition to Deactivate and Low POWER -> DCVS DOWN
  NNC_ACTIVATION_STATE_INVAL = 3, //!< Transition Activation to Ready
  NNC_ACTIVATION_STATE_CMD_TYPE_MAX =
      3, //!< Maximum number of defined Activation Request
};
typedef int32_t nnc_activation_state_type_t;

typedef enum nnc_cmd_device_config_options {
  NNC_DEV_CONFIG_DMA_PC = 1, //!< DMA Physical channel config
  NNC_DEV_CONFIG_MAX =
      NNC_DEV_CONFIG_DMA_PC, //!< Maximum number of defined config options
} nnc_cmd_device_config_options_t;

/*****************************************************************************
 *  All the NNC protocol structures should end on a 8byte boundary and all   *
 *  its structure members must be naturally aligned to itself. It is the     *
 *  responsibility of the maintainer/programmer to make sure that the above  *
 *  criterias are met while updating the NNC protocol structures.            *
 * ***************************************************************************/

typedef struct qaic_manage_trans_hdr manage_msg_header_t;
typedef struct qaic_manage_msg manage_msg_t;
typedef struct qaic_manage_trans_hdr nnc_transaction_header_t;
typedef struct qaic_manage_trans_passthrough nnc_transaction_t;
typedef struct qaic_manage_trans_dma_xfer nnc_transaction_dma_xfer_t;
typedef struct qaic_manage_trans_activate_to_dev nnc_transaction_vc_setup_uk_t;
typedef struct qaic_manage_trans_activate_from_dev nnc_transaction_vc_setup_ku_t;
typedef struct qaic_manage_trans_deactivate nnc_transaction_vc_teardown_uk_t;
typedef struct qaic_manage_trans_status_to_dev nnc_transaction_status_request_t;
typedef struct qaic_manage_trans_status_from_dev nnc_transaction_status_response_t;


typedef struct nnc_cmd {
  /** Command as defined in nnc_command_type */
  uint32_t type;
  uint32_t padding;
  uint8_t data[0];
} __attribute__((packed)) nnc_cmd_t;

/** Passthrough request/response share the same format */
typedef struct nnc_transaction_passthrough {
  nnc_transaction_header_t header;
  nnc_cmd_t cmd;
} __attribute__((packed)) nnc_transaction_passthrough_t;

/**
 * Requests the loading of the weights and biases that will be used later when
 * activating the network.
 */
typedef struct nnc_cmd_load_constants_request {
  uint32_t type;
  /** Host value used for debugging and referenced by other modules running in
   * QSM (e.g. DM). */
  uint32_t process_id;
  /** Tag is used to correlate to data specified dma_xfer transaction */
  uint32_t tag;
  uint32_t constants_crc;
  uint64_t data_length;
  /** Null terminated ASCII string for the name of the constants. Set the first
   * byte to NULL if not used.*/
  char constants_name[NNC_MAX_STRING_LENGTH];
} __attribute__((packed)) nnc_cmd_load_constants_request_t;

/**
 * The load constants request EX request is used to deliver the constants
 * descriptor which is derived from metadata and it describes the sizes of the
 * constants sections and related attributes. There are no companion DMA
 * transfer transactions that go with the EX request.
 */
typedef struct nnc_cmd_load_constants_ex_request {
  uint32_t type;
  /** Host value used for debugging and referenced by other modules running in
   * QSM (e.g. DM). */
  uint32_t process_id;

  /** Null terminated ASCII string for the name of the constants. Set the first
   * byte to NULL if not used.*/
  char constants_name[NNC_MAX_STRING_LENGTH];

  /* AICConstantDescriptor from metadata */
  uint32_t descriptor_len;
  uint32_t padding;
  uint8_t descriptor[];
} __attribute__((packed)) nnc_cmd_load_constants_ex_request_t;

/**
 * Load constants to the region set up by nnc_cmd_load_constants_ex_request
 */
typedef struct nnc_cmd_load_constants_payload_request {
  uint32_t type;
  /** Host value used for debugging and referenced by other modules running in
   * QSM (e.g. DM). */
  uint32_t process_id;
  /** Tag is used to correlate to data specified dma_xfer transaction */
  uint32_t tag;
  uint32_t constants_id;
  uint64_t data_length;
  uint64_t offset;
} __attribute__((packed)) nnc_cmd_load_constants_payload_request_t;

/**
 * Response to nnc_cmd_load_constants_request and
 * nnc_cmd_load_constants_payload_request.
 */
typedef struct nnc_cmd_load_constants_response {
  uint32_t type;
  /**
   * Unique ID of the successfully loaded constants. A value of zero is used if
   * there was an error. See status_code for information on the nature of the
   * error.
   */
  uint32_t constants_id;
  /** Status of the nnc_cmd_load_request. */
  nnc_status_code_t status_code;
  uint32_t padding;
} __attribute__((packed)) nnc_cmd_load_constants_response_t;

/**
 * Requests a load of the network ELF file and metadata at the specified host
 * address.
 */
typedef struct nnc_cmd_load_elf_request {
  uint32_t type;
  /** Tag is used to associate the request with a DMA_XFER transation with the
   * same tag */
  uint32_t tag;
  /** ID of the process used by the host OS.*/
  uint32_t process_id;
  uint32_t metadata_crc;
  /** Null terminated ASCII string for the name of the network. Set the first
   * byte to NULL if not used.*/
  char network_name[NNC_MAX_STRING_LENGTH];
  /** Size of network elf */
  uint64_t data_length;
  /**
   * This CRC is the CRC-32 of the section metadata_fb in network.ef
   */
  uint32_t metadata_flatbuffer_crc;
  uint8_t padding[4];
} __attribute__((packed)) nnc_cmd_load_elf_request_t;

/**
 * Response returned to the host after an nnc_cmd_load_elf_request.
 */
typedef struct nnc_cmd_load_elf_response {
  uint32_t type;
  /**
   * Unique ID of the successfully loaded constants. A value of zero is used if
   * there was an error. See status_code for information on the nature of the
   * error.
   */
  uint32_t elf_id;
  /** Status of the nnc_cmd_load_request. */
  nnc_status_code_t status_code;
  uint32_t padding;
} __attribute__((packed)) nnc_cmd_load_elf_response_t;

/**
 * Unload previously loaded network weights and biases.
 */
typedef struct nnc_cmd_unload_constants_request {
  uint32_t type;
  /** Unique ID of previously loaded network weights and biases. */
  uint32_t constants_id;
} __attribute__((packed)) nnc_cmd_unload_constants_request_t;

/**
 * Unload previously loaded network weights and biases.
 */
typedef struct nnc_cmd_unload_constants_response {
  uint32_t type;
  /** Status of the nnc_cmd_unload_constants_request. */
  nnc_status_code_t status_code;
} __attribute__((packed)) nnc_cmd_unload_constants_response_t;

/**
 * Requests the unload of a previously loaded network.
 */
typedef struct nnc_cmd_unload_elf_request {
  uint32_t type;
  /** Unique ID of the previously loaded elf that is to be unloaded. */
  uint32_t elf_id;
} __attribute__((packed)) nnc_cmd_unload_elf_request_t;

/**
 * Response returned to the host after an nnc_cmd_unload_elf_request.
 */
typedef struct nnc_cmd_unload_elf_response {
  uint32_t type;
  /** Status of the nnc_cmd_unload_elf_request. */
  nnc_status_code_t status_code;
} __attribute__((packed)) nnc_cmd_unload_elf_response_t;

/**
 * Requests activation of a previously loaded elf, specified by elf_id.
 *
 * @note
 * This request will configure the data path and associated VC ID to be used for
 * sending/receiving network I/O.
 */
typedef struct nnc_cmd_activate_request {
  uint32_t type;
  /**
   * ID of the process used by the host OS. This ID provided by host application
   * and may not be trusted.
   */
  uint32_t process_id;
  /** Identifies the previously loaded network weights and biases. */
  uint32_t constants_id;
  /** Identifies the previously loaded network ELF. */
  uint32_t elf_id;
  /** // None, Resource Group or Activation Reservation */
  uint32_t reservation_type;
  /**  Resource Group or Activation Reservation */
  int32_t resource_res_id;
  /** VC reservation -1 for none */
  uint32_t opt_vc_setup; // Set to 1 if VC setup transaction is included
  int32_t vc_res_id;
  /** Define the initial activation state for this activation */
  nnc_activation_state_type_t initial_activation_state;
  /** Type nnc_dma_option_type_t of physical DMA channel mapping requested */
  uint32_t dma_type;

  /** Activation can be done for
   * - No oversubscription
   * - Oversubscription with Data Path Switching
   * - Oversubscription with Control Path Switching
   * Only in the case of Oversubscription with Data Path Switching is the
   * NNC Doorbell needed.
   * Activation process must know if this is required, this option is given
   * in the opt_support_datapath_switching
   * Set opt_support_datapath_switching to 1 if for datapath switching
   * the activation response will include valid info in the datapath doorbell
   */
  uint8_t opt_support_datapath_switching;
  /* Align to 64 bit boundary */
  uint8_t padding[7];
} __attribute__((packed)) nnc_cmd_activate_request_t;

typedef enum nnc_doorbell_length_enum {
  NNC_DOORBELL_LEN_32 = 0,
  NNC_DOORBELL_LEN_16 = 1,
  NNC_DOORBELL_LEN_8 = 2,
  NNC_DOORBELL_LEN_INVALID = 3
} nnc_doorbell_length_enum_t;

/**
 * Response returned to the host after an nnc_activation_request.
 */
typedef struct nnc_cmd_activate_response {
  uint32_t type;
  /** Status of the nnc_activate_request. */
  nnc_status_code_t status_code;
  /**
   * Identifies the network just activated, if successful. A zero value
   * indicates an error occurred during activation. See the status_code
   * for more information on the error.
   */
  uint32_t activation_id;
  /**
   * ID of the bi-directional virtual channel to send and receive bulk data from
   * the running network.
   */
  uint32_t virtual_channel_id;
  uint64_t ddr_base;
  uint64_t dynamic_ddr_base;
  uint64_t mcid_base;
  /* mcid base address used for cmdObj dma buffers */
  uint64_t mcid_base_dma_cmd;
  /* Type of VC mapping nnc_dma_option_type_t, may differ from requested type */
  uint32_t dma_type;
  /**
   * Dedicated physical channel mask, valid if
   * dma_type = NNC_DMA_OPTION_DEDICATED
   */
  uint32_t pc_mask;
  /* Data path command NSP polling location address */
  uint64_t cmd_nsp_base_polling_addr;
  /* Size of each element in NSP Polling info array */
  uint32_t cmd_nsp_polling_info_size;
  /* Data path command nsp polling data */
  uint32_t cmd_nsp_polling_data;
  /* Data path command doorbell */
  uint64_t cmd_doorbell_addr;
  /* Data path command doorbell length, how much data is valid in doorbell_data
     This value is one of the nnc_doorbell_length_enum */
  uint8_t cmd_doorbell_length;
  /* Field set to true if the cmd_doorbell data is valid, otherwise data should
   * be ignored*/
  uint8_t cmd_doorbell_valid;
  /* Bit mask of Physical NSP IDs*/
  uint32_t nspMask;
  /* Align to 64 bits */
  uint8_t padding[2];

} __attribute__((packed)) nnc_cmd_activate_response_t;

/**
 * Requests deactivation of a previously activated network. No resources used by
 * this networ will be released automatically.  Use nnc_cmd_unload_elf_request
 * and nnc_cmd_unload_constants_request to request the unloading of the
 * resources used by this network.
 */
typedef struct nnc_cmd_deactivate_request {
  uint32_t type;
  /**
   * Identifies the currently active network that should be deactivated.
   */
  uint32_t activation_id;
  uint32_t opt_vc_teardown; // Set to 1 if VC teardown transaction is included
  uint32_t padding;
} __attribute__((packed)) nnc_cmd_deactivate_request_t;

/**
 * Response returned to the host after an nnc_deactivation_request
 */
typedef struct nnc_cmd_deactivate_response {
  uint32_t type;
  /**
   * Success indicates that all previously allocated resources have been
   * successfully returned to the pool.
   */
  nnc_status_code_t status_code;
} __attribute__((packed)) nnc_cmd_deactivate_response_t;

typedef struct nnc_cmd_status_request {
  uint32_t type;
  /** Specify type of info requested host_api_info_nsp_data_internal_t */
  uint8_t info_type;
  uint8_t padding[3];
} __attribute__((packed)) nnc_cmd_status_query_request_t;

typedef struct nnc_cmd_status_query_response {
  uint32_t type;
  /**
   * Success indicates that all previously allocated resources have been
   * successfully returned to the pool.
   */
  nnc_status_code_t status_code;
  host_api_info_data_internal_t info;
} __attribute__((packed)) nnc_cmd_status_query_response_t;

/**
 * Requests virtual channel reservation.
 *
 * @note
 * This request will configure the data path and associated VC ID to be used for
 * sending/receiving network I/O.
 */
typedef struct nnc_cmd_create_vc_reservation_request {
  uint32_t type;
  /**
   * ID of the process used by the host OS. This ID provided by host application
   * and may not be trusted.
   */
  uint32_t process_id;
  /** Identifies the previously loaded network weights and biases. 0 means no
   * constants. */
  /* Number of Semaphores to be used with this Virtual Channel*/
  /* Semaphores must be assigned in the VC programming */
  uint32_t num_sem;
  int32_t source_grp_id;
  /** Type nnc_dma_option_type_t of physical DMA channel mapping requested */
  uint32_t dma_option;
  /* Size of DDR block for datapath command and response*/
  uint32_t mem_cmd_block_size;
  /* Set to true (1) to include assignment of special use semaphore */
  uint8_t assign_special_use_sem;
  /* padding to align to 64 bit */
  uint8_t padding[7];
} __attribute__((packed)) nnc_cmd_create_vc_reservation_request_t;

/**
 * Response returned to the host after an nnc_cmd_create_vc_reservation_request.
 */
typedef struct nnc_cmd_create_vc_reservation_response {
  uint32_t type;
  /** Status of the nnc_reserve_vc_request. */
  nnc_status_code_t status_code;

  /**
   * Reservation ID to be used in activation reservation
   */
  uint32_t vcresid;
  /**
   * ID of the bi-directional virtual channel to send and receive bulk data from
   * the running network.
   */
  uint32_t virtual_channel_id;
  /* Type of VC mapping nnc_dma_option_type_t, may differ from requested type */
  uint32_t dma_type;
  /**
   * Dedicated physical channel mask, valid if
   * dma_type = NNC_DMA_OPTION_DEDICATED
   */
  uint32_t pc_mask;
  /* boolean value set to true if special_use_id included, othewise ignore
   * special_use_sem_id field */
  uint8_t special_use_sem_id_included;
  /* Align to 64 bits. */
  uint8_t padding;
  /* Datapath switching semaphore ID */
  uint16_t special_use_sem_id;
  /* Size of DDR block for datapath command and response */
  uint32_t mem_cmd_block_size;
  /* Address of DDR block for datapath command and response */
  uint64_t mem_cmd_block_base_addr;
} __attribute__((packed)) nnc_cmd_create_vc_reservation_response_t;

/**
 * Requests vc reservation release  of a previously reserved VC.
 */
typedef struct nnc_cmd_release_vc_reservation_request {
  uint32_t type;
  /**
   * Identifies the currently active reservation.
   */
  uint32_t vcresid;
} __attribute__((packed)) nnc_cmd_release_vc_reservation_request_t;

/**
 * Response returned to the host after an nnc_release_vc_request
 */
typedef struct nnc_cmd_release_vc_reservation_response {
  uint32_t type;
  /**
   * Success indicates that all previously allocated resources have been
   * successfully returned to the pool.
   */
  nnc_status_code_t status_code;
} __attribute__((packed)) nnc_cmd_release_vc_reservation_response_t;

/**
 * Requests create activation reservation.
 *
 * @note
 * This request will configure the data path and associated VC ID to be used for
 * sending/receiving network I/O.
 */
typedef struct nnc_cmd_create_activation_reservation_request {
  uint32_t type;
  /**
   * ID of the process used by the host OS. This ID provided by host application
   * and may not be trusted.
   */
  uint32_t process_id;
  /**  Identifies process */
  int32_t source_grp_id;
  uint32_t padding;

  uint64_t shddr_size;
  uint32_t num_nsp;
  uint32_t num_mcid;
  uint64_t l2region_size;
  /* Number of networks for which the activation reservation is done */
  uint16_t num_networks;
  uint8_t padding2[6];
} __attribute__((packed)) nnc_cmd_create_activation_reservation_request_t;

/**
 * Response returned to the host after a
 * nnc_cmd_create_activation_reservation_request
 */
typedef struct nnc_cmd_create_activation_reservation_response {
  uint32_t type;
  /** Status of the nnc_reserve_vc_request. */
  nnc_status_code_t status_code;

  /**
   * Reservation ID to be used in activation reservation
   */
  int32_t ac_res_id;
  /**
   * ID of the activation reservation
   */
  uint32_t padding;

} __attribute__((packed)) nnc_cmd_create_activation_reservation_response_t;

/**
 * Requests release activation reservation.
 */
typedef struct nnc_cmd_create_activation_reservation_release_request {
  uint32_t type;
  /**
   * Identifies the currently active reservation.
   */
  int32_t ac_res_id;
} __attribute__((packed))
nnc_cmd_create_activation_reservation_release_request_t;

/**
 * Response returned to the host after a
 * nnc_cmd_create_activation_reservation_release_request
 */
typedef struct nnc_cmd_create_activation_reservation_release_response {
  uint32_t type;
  /**
   * Success indicates that all previously allocated resources have been
   * successfully returned to the pool.
   */
  nnc_status_code_t status_code;
} __attribute__((packed))
nnc_cmd_create_activation_reservation_release_response_t;

/**
 * Requests create resource group
 */
typedef struct nnc_cmd_create_resource_group_request {
  uint32_t type;
  /**
   * ID of the process used by the host OS. This ID provided by host application
   * and may not be trusted.
   */
  uint32_t process_id;
  /** Identifies process */
  /**
   *  res_grp_id must be a valid Id generated from an active reservation, or
   * set to -1 for the default system resource
   * pool
   */
  int32_t source_grp_id;
  /** Resource Information */
  uint32_t num_nsp;
  uint32_t num_mcid;
  uint32_t num_sem;
  uint32_t num_vc;
  uint32_t padding;
  uint64_t shddr_size;
  uint64_t l2region_size;
  uint64_t mem_size;
  /* Number of networks for which the resource reservation is done */
  uint16_t num_networks;
  uint8_t padding2[6];
} __attribute__((packed)) nnc_cmd_create_resource_group_request_t;

/**
 * Response to create resource group
 */
typedef struct nnc_cmd_create_resource_group_response {
  uint32_t type;
  /** Status of the nnc_reserve_vc_request. */
  nnc_status_code_t status_code;
  /**
   * Resource group ID
   */
  int32_t res_grp_id;
  uint32_t padding;
} __attribute__((packed)) nnc_cmd_create_resource_group_response_t;

/**
 * Requests release resource group
 */
typedef struct nnc_cmd_release_resource_group_request {
  uint32_t type;
  /**
   * Resource Group ID
   */
  int32_t res_grp_id;
} __attribute__((packed)) nnc_cmd_release_resource_group_request_t;

/**
 * Response returned to the host from nnc_cmd_release_resource_group_request
 */
typedef struct nnc_cmd_release_resource_group_response {
  uint32_t type;
  /**
   * Success indicates that all previously allocated resources have been
   * successfully returned to the pool.
   */
  nnc_status_code_t status_code;
} __attribute__((packed)) nnc_cmd_release_resource_group_response_t;

typedef struct nnc_cmd_resource_info_request {
  uint32_t type;
  /**
   * ID of the process used by the host OS. This ID provided by host application
   * and may not be trusted.
   */
  uint32_t process_id;
  /**  resouce_group_id is only valid if resource_type is
   *NNC_RESOURCE_TYPE_RESOUCE_GROUP
   *   otherwise system resource info is returned
   **/
  int32_t res_grp_id;
  uint32_t padding;
} __attribute__((packed)) nnc_cmd_resource_info_request_t;

/**
 * Response to create resource group
 */
typedef struct nnc_cmd_resource_info_response {
  uint32_t type;
  int32_t res_grp_id;
  host_api_resource_info_internal_t resource_info;
  nnc_status_code_t status_code;
  uint32_t padding;
} __attribute__((packed)) nnc_cmd_resource_info_response_t;

/**
 * Query performance related info only
 */
typedef struct nnc_cmd_performance_info_request {
  uint32_t type;
  uint32_t padding;
} __attribute__((packed)) nnc_cmd_performance_info_request_t;

/**
 * Response to performance info query cmd
 */
typedef struct nnc_cmd_performance_info_response {
  uint32_t type;
  nnc_status_code_t status_code;
  host_api_performance_info_internal_t performance_info;
} __attribute__((packed)) nnc_cmd_performance_info_response_t;

/**
 * Activation State Command Request
 * This command allows the caller to request a state transition on an existing
 * Activation
 */
#define NNC_ACTIVATION_CMD_TYPE_MAX_COMMANDS (8)
typedef struct nnc_activation_state_cmd {
  activation_id_type_t acid;
  nnc_activation_state_type_t cmd;
} __attribute__((packed)) nnc_activaiton_state_cmd_t;

typedef struct nnc_activation_state_cmd_request {
  uint32_t type;
  uint32_t num_cmds;
  nnc_activaiton_state_cmd_t ac_cmd[NNC_ACTIVATION_CMD_TYPE_MAX_COMMANDS];
} __attribute__((packed)) nnc_activation_state_cmd_request_t;

/**
 *  Activation State Command Request
 */
typedef struct nnc_activation_state_cmd_response {
  uint32_t type;
  nnc_status_code_t status_code;
} __attribute__((packed)) nnc_activation_state_cmd_response_t;

/**
 *  Configure physical DMA channels to be dedicated to single virtual channel
 */
typedef struct nnc_cmd_device_config_request {
  uint32_t type;
  /**
  * ID of the process used by the host OS. This ID provided by host application
  * and may not be trusted.
  */
  uint32_t process_id;
  /** DMA Physical channel config */
  nnc_cmd_device_config_options_t config_options;
  uint32_t num_pc_dedicated;
} __attribute__((packed)) nnc_cmd_device_config_request_t;

typedef struct nnc_cmd_device_config_response {
  uint32_t type;
  /** Status of the nnc_device_config_request. */
  nnc_status_code_t status_code;
} __attribute__((packed)) nnc_cmd_device_config_response_t;

typedef struct nnc_cmd_update_ac_reservation_request {
  uint32_t type;
  /**
   * ID of the process used by the host OS. This ID provided by host application
   * and may not be trusted.
   */
  uint32_t process_id;
  /**
   * Reservation ID to be used in update of activation reservation
   */
  int32_t ac_res_id;
  /**
   * Specifies the number of nsps for the active reservation.
   */
  uint32_t num_nsp;
  /** reserved */
  uint64_t reserved;
} __attribute__((packed)) nnc_cmd_update_ac_reservation_request_t;

typedef struct nnc_cmd_update_ac_reservation_response {
  uint32_t type;
  /** Status of the nnc_update_ac_reservation_request. */
  nnc_status_code_t status_code;
} __attribute__((packed)) nnc_cmd_update_ac_reservation_response_t;


// All NNC protocol structure must end on a 8byte boundary.
static_assert((sizeof(nnc_cmd_t) % 8) == 0 , "nnc_cmd_t should be 8byte aligned.");
static_assert((sizeof(nnc_transaction_passthrough_t) % 8) == 0 , "nnc_transaction_passthrough_t should be 8byte aligned.");
static_assert((sizeof(nnc_cmd_load_constants_request_t) % 8) == 0 , "nnc_cmd_load_constants_request_t should be 8byte aligned.");
static_assert((sizeof(nnc_cmd_load_constants_ex_request_t) % 8) == 0 , "nnc_cmd_load_constants_ex_request_t should be 8byte aligned.");
static_assert((sizeof(nnc_cmd_load_constants_payload_request_t) % 8) == 0 , "nnc_cmd_load_constants_payload_request_t should be 8byte aligned.");
static_assert((sizeof(nnc_cmd_load_constants_response_t) % 8) == 0 , "nnc_cmd_load_constants_response_t should be 8byte aligned.");
static_assert((sizeof(nnc_cmd_load_elf_request_t) % 8) == 0 , "nnc_cmd_load_elf_request_t should be 8byte aligned.");
static_assert((sizeof(nnc_cmd_load_elf_response_t) % 8) == 0 , "nnc_cmd_load_elf_response_t should be 8byte aligned.");
static_assert((sizeof(nnc_cmd_unload_constants_request_t) % 8) == 0 , "nnc_cmd_unload_constants_request_t should be 8byte aligned.");
static_assert((sizeof(nnc_cmd_unload_constants_response_t) % 8) == 0 , "nnc_cmd_unload_constants_response_t should be 8byte aligned.");
static_assert((sizeof(nnc_cmd_unload_elf_request_t) % 8) == 0 , "nnc_cmd_unload_elf_request_t should be 8byte aligned.");
static_assert((sizeof(nnc_cmd_unload_elf_response_t) % 8) == 0 , "nnc_cmd_unload_elf_response_t should be 8byte aligned.");
static_assert((sizeof(nnc_cmd_activate_request_t) % 8) == 0 , "nnc_cmd_activate_request_t should be 8byte aligned.");
static_assert((sizeof(nnc_cmd_activate_response_t) % 8) == 0 , "nnc_cmd_activate_response_t should be 8byte aligned.");
static_assert((sizeof(nnc_cmd_deactivate_request_t) % 8) == 0 , "nnc_cmd_deactivate_request_t should be 8byte aligned.");
static_assert((sizeof(nnc_cmd_deactivate_response_t) % 8) == 0 , "nnc_cmd_deactivate_response_t should be 8byte aligned.");
static_assert((sizeof(nnc_cmd_status_query_request_t) % 8) == 0 , "nnc_cmd_status_query_request_t should be 8byte aligned.");
static_assert((sizeof(nnc_cmd_status_query_response_t) % 8) == 0 , "nnc_cmd_status_query_response_t should be 8byte aligned.");
static_assert((sizeof(nnc_cmd_create_vc_reservation_request_t) % 8) == 0 , "nnc_cmd_create_vc_reservation_request_t should be 8byte aligned.");
static_assert((sizeof(nnc_cmd_create_vc_reservation_response_t) % 8) == 0 , "nnc_cmd_create_vc_reservation_response_t should be 8byte aligned.");
static_assert((sizeof(nnc_cmd_release_vc_reservation_request_t) % 8) == 0 , "nnc_cmd_release_vc_reservation_request_t should be 8byte aligned.");
static_assert((sizeof(nnc_cmd_release_vc_reservation_response_t) % 8) == 0 , "nnc_cmd_release_vc_reservation_response_t should be 8byte aligned.");
static_assert((sizeof(nnc_cmd_create_activation_reservation_request_t) % 8) == 0 , "nnc_cmd_create_activation_reservation_request_t should be 8byte aligned.");
static_assert((sizeof(nnc_cmd_create_activation_reservation_response_t) % 8) == 0 , "nnc_cmd_create_activation_reservation_response_t should be 8byte aligned.");
static_assert((sizeof(nnc_cmd_create_activation_reservation_release_request_t) % 8) == 0 , "nnc_cmd_create_activation_reservation_release_request_t should be 8byte aligned.");
static_assert((sizeof(nnc_cmd_create_activation_reservation_release_response_t) % 8) == 0 , "nnc_cmd_create_activation_reservation_release_response_t should be 8byte aligned.");
static_assert((sizeof(nnc_cmd_create_resource_group_request_t) % 8) == 0 , "nnc_cmd_create_resource_group_request_t should be 8byte aligned.");
static_assert((sizeof(nnc_cmd_create_resource_group_response_t) % 8) == 0 , "nnc_cmd_create_resource_group_response_t should be 8byte aligned.");
static_assert((sizeof(nnc_cmd_release_resource_group_request_t) % 8) == 0 , "nnc_cmd_release_resource_group_request_t should be 8byte aligned.");
static_assert((sizeof(nnc_cmd_release_resource_group_response_t) % 8) == 0 , "nnc_cmd_release_resource_group_response_t should be 8byte aligned.");
static_assert((sizeof(nnc_cmd_resource_info_request_t) % 8) == 0 , "nnc_cmd_resource_info_request_t should be 8byte aligned.");
static_assert((sizeof(nnc_cmd_resource_info_response_t) % 8) == 0 , "nnc_cmd_resource_info_response_t should be 8byte aligned.");
static_assert((sizeof(nnc_cmd_performance_info_request_t) % 8) == 0 , "nnc_cmd_performance_info_request_t should be 8byte aligned.");
static_assert((sizeof(nnc_cmd_performance_info_response_t) % 8) == 0 , "nnc_cmd_performance_info_response_t should be 8byte aligned.");
static_assert((sizeof(nnc_activaiton_state_cmd_t) % 8) == 0 , "nnc_activaiton_state_cmd_t should be 8byte aligned.");
static_assert((sizeof(nnc_activation_state_cmd_request_t) % 8) == 0 , "nnc_activation_state_cmd_request_t should be 8byte aligned.");
static_assert((sizeof(nnc_activation_state_cmd_response_t) % 8) == 0 , "nnc_activation_state_cmd_response_t should be 8byte aligned.");
static_assert((sizeof(nnc_cmd_device_config_request_t) % 8) == 0 , "nnc_cmd_device_config_request_t should be 8byte aligned.");
static_assert((sizeof(nnc_cmd_device_config_response_t) % 8) == 0 , "nnc_cmd_device_config_response_t should be 8byte aligned.");
static_assert((sizeof(nnc_cmd_update_ac_reservation_request_t) % 8) == 0 , "nnc_cmd_update_ac_reservation_request_t should be 8byte aligned.");
static_assert((sizeof(nnc_cmd_update_ac_reservation_response_t) % 8) == 0 , "nnc_cmd_update_ac_reservation_response_t should be 8byte aligned.");
static_assert((qaic::QDEVICE_FEATURE_MAX_BITS >= NNC_DEVICE_FEATURE_FLAG_NUM_BITS ) , "nnc_transaction_device_flag_bitpos_type incompatible.");

// clang-format on

#endif // QNNCPROTOCOL_H
