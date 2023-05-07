// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QTYPESRUNTIME_H
#define QTYPESRUNTIME_H

#include <stddef.h>
#include <stdint.h>
#include <limits>
#include <cstring>
#include <vector>
#include <memory>
#include <cstdint>
#include <climits>
#include "QAicTypes.h"
#include <cstdint>
#include <climits>
#include <string>

using QID = int32_t;
using QCtrlChannelID = uint8_t;
using QNNImageID = uint32_t;
using QNNConstantsID = uint32_t;
using QNAID = uint32_t;
using QObjId = uint32_t;

using QResourceReservationID = uint32_t;
using QAicContextID = uint32_t;
using QAicProgramID = uint32_t;
using QAicExecObjID = uint32_t;
using QAicObjId = uint32_t;

#define QAIC_POW_2(n) (((uint64_t)1) << (n))
#define QAIC_SZ_16 QAIC_POW_2(4)
#define QAIC_SZ_32 QAIC_POW_2(5)
#define QAIC_SZ_64 QAIC_POW_2(6)

constexpr int32_t RESOURCE_GROUP_ID_DEFAULT = -1;

constexpr uint8_t QAIC_PROGRAM_PROPERTIES_SUBMIT_NUM_RETRIES_DEFAULT = 5;
/// After ExecObj is submitted to kernel, this is maximum number of retry
/// performed for a failed kernel wait call. When the maximum retries is
/// exceeded an error code is returned.

constexpr uint8_t QAIC_PROGRAM_PROPERTIES_SUBMIT_TIMEOUT_MS_DEFAULT = 0;
/// When an ExecObj is submitted to the kernel for execution, it is placed in
/// a HW queue for execution. After the ExecObj is queued, kernel wait is called
/// to wait for its completion and this value decides how much time kernel
/// waits before throwing a timeout error. Any non-zero value is understood
/// in millisecond and zero means that application is asking the kernel
/// to choose a timeout value. By default kernel sets this value as 5000
/// millisecond and it can be changed using sysfs/module parameter.

/// \brief Buffer Type for inference buffers
enum class QBufferType : uint32_t {
  /// QBUFFER_TYPE_HEAP (default)
  QBUFFER_TYPE_HEAP = 0,
  QBUFFER_TYPE_INVAL = std::numeric_limits<uint32_t>::max()
};

/// \brief Buffer Specific for Dma Transfers
/// handle, offest and type are only considered when type is
/// QBUFFER_TYPE_DMABUF
struct QBuffer {
  /// Total size of memory pointed to by \a buf pointer or \a handle
  size_t size = 0;
  /// Buffer pointer, valid in case of heap memory
  uint8_t *buf = nullptr;
  /// Buffer Handle, valid in case of device allocated
  /// memory such as dma buf
  uint64_t handle = 0;
  /// Offset within handle
  uint32_t offset = 0;
  /// Type of the buffer, see QBufferType
  QBufferType type = QBufferType::QBUFFER_TYPE_INVAL;
};

/// \brief Data type used for queries and no-inference buffers
struct QData {
  /// Size of buffer
  size_t size = 0;
  /// Buffer pointer
  uint8_t *data = nullptr;
  ;
};

/// \brief Data type used to describe the dimensions of one buffer
/// Say a buffer of float with dimension of 2x4x8 has count 3,
/// sizeOfElem (for float) 4 and dims [2, 4, 8].
struct QBufferDimensions {
  /// Number of items in dims array
  uint32_t count = 0;
  /// Size in bytes of the data type for this buffer, say 4 bytes for float
  uint32_t sizeOfElem = 0;
  /// Pointer to list of dimensions
  uint32_t *dims = nullptr;
};

/// \brief Device states
enum class QDevStatus {
  /// Device in initialization (firmware download/boot)
  QDS_INITIALIZING = 0,
  /// Device has exited initialization and is ready for inference activities.
  QDS_READY = 1,
  /// Device in error state
  QDS_ERROR = 2
};

/// \brief Runtime library version information
struct QRTVerInfo {
  /// Runtime Library Major Version
  uint16_t major = 0;
  /// Runtime Library Minor Version
  uint16_t minor = 0;
  /// Runtime Library Patch Version
  const char *patch = nullptr;
  /// Runtime Library Variant
  const char *variant = nullptr;
};

/// Hardware Pci Information OF DEVICES
struct QPciExtInfo {
  /// Card link speed capability
  unsigned int maxLinkSpeed = 0;
  /// Card link width capability
  unsigned int maxLinkWidth = 0;
  /// Negotiated link speed
  unsigned int currLinkSpeed = 0;
  /// Negotiated link width
  unsigned int currLinkWidth = 0;
  /// BAR 2 address
  uint64_t bar2Addr = 0;
  /// BAR 4 address
  uint64_t bar4Addr = 0;
  void clear() {
    maxLinkSpeed = 0;
    maxLinkWidth = 0;
    currLinkSpeed = 0;
    currLinkWidth = 0;
    bar2Addr = 0;
    bar4Addr = 0;
  }
};

struct QPciInfo {
  /// PCI domain number in hexadecimal
  uint16_t domain = 0;
  /// PCI bus number in hexadecimal
  uint8_t bus = 0;
  /// PCI device number in hexadecimal
  uint8_t device = 0;
  /// PCI function number in hexadecimal
  uint8_t function = 0;
  /// Name of the class
  char classname[128]{};
  /// Name of the vendor
  char vendorname[128]{};
  /// Name of the device
  char devicename[128]{};
  /// PCI Extended Attributes Information
  QPciExtInfo pcieExtInfo{};

  void clear() {
    domain = 0;
    bus = 0;
    device = 0;
    function = 0;
    memset(classname, 0, sizeof(classname));
    memset(vendorname, 0, sizeof(vendorname));
    memset(devicename, 0, sizeof(devicename));
    pcieExtInfo.clear();
  }
};

/// \brief Device specific resource information
struct QResourceInfo {
  /// Queries for resources can be made for the entire device, in which case
  /// QResourceReservationID is -1.  If the query for Resource info was
  /// submitted with a particular resource group ID, the resource group ID
  /// is returned as the QResourceReservationID field.
  int32_t QResourceReservationID;
  /// Total DDR in MB
  uint64_t dramTotal;
  /// Free DDR in MB
  uint64_t dramFree;
  /// Total number of virtual channels
  uint8_t vcTotal;
  /// Number of free virtual channels
  uint8_t vcFree;
  /// Total number of NSPs
  uint8_t nspTotal;
  /// Number of free NSPs
  uint8_t nspFree;
  /// Number of MCid
  uint32_t mcidTotal;
  /// Free MCIDs
  uint32_t mcidFree;
  /// Total Semaphore count
  uint32_t semTotal;
  /// Free Semaphore count
  uint32_t semFree;
  /// Total number of physical channels
  uint8_t pcTotal;
  /// Number of dedicated physical channels
  uint8_t pcReserved;
  /// Align to 64 bit
  uint8_t padding[6];
};

/// \brief Device specific performance information
struct QPerformanceInfo {
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
  /// Peak compute on device in ops/second. Assumes all ops are in int8.
  float peakCompute;
  /// Memory bandwidth from DRAM on device in Kbytes/second, averaged over last
  /// ~100 ms;
  float dramBw;
  float reserved1;
  float reserved2;
};

/// \brief Platform specific firmware device features.
enum QFirmwareFeatureBits : uint32_t {
  /// Functional Safety
  QAIC_FW_FEATURE_FUSA = 0,
  QAIC_FW_FEATURE_MAX = 1
};

/// \brief Device telemetry information, like; thermal, power etc.
struct QTelemetryInfo {
  /// SoC temperature from device. Unit: Degrees C * 1000
  /// Value is 0 in case of read error.
  /// cat /sys/bus/pci/devices/<PCI-Address>/hwmon/hwmon<#>/temp2_input
  uint32_t socTemperature;
  /// Instantaneous average board power. Unit: Watts * 1000000
  /// Value is 0 in case of read error.
  /// cat /sys/bus/pci/devices/<PCI-Address>/hwmon/hwmon<#>/power1_input
  uint32_t boardPower;
  /// TDP Cap or max power configured. Unit: Watts * 1000000
  /// Value is 0 in case of read error.
  /// cat /sys/bus/pci/devices/<PCI-Address>/hwmon/hwmon<#>/power1_max
  uint32_t tdpCap;
};

/// \brief Device board information that contains static information
/// from the device
struct QBoardInfo {
  /// Firmware major version number.
  uint8_t fwVersionMajor;
  /// Firmware minor version number.
  /// Firmware updates with minor version updates are backwards compatible.
  uint8_t fwVersionMinor;
  /// Firmware patch version number.
  /// Firmware updates with patch  version updates are backwards compatible.
  uint8_t fwVersionPatch;
  uint8_t fwVersionBuild;
  /// QFirmwareFeatureBitset
  /// Firmware feature bitset.
  uint8_t fwFeaturesBitmap;
  /// Null terminate string describing the FW image version.
  char fwQCImageVersionString[QAIC_SZ_64];
  /// Null terminated string provided by OEM.
  char fwOEMImageVersionString[QAIC_SZ_64];
  /// Null terminated string describing the variant.
  /// This may contain items such as "Debug" or "Release".
  char fwImageVariantString[QAIC_SZ_64];
  /// Null terminated stirng describing the set of features
  /// included in this FW release.
  char fwImageFeaturesString[QAIC_SZ_64];
  /// Null terminated string describing the
  /// secondary boot loader image.
  char sblImageString[QAIC_SZ_32];
  /// PVS Image Version Number
  uint32_t pvsImageVersion;
  /// Hardware version
  uint32_t hwVersion;
  /// Soc serial number
  char serial[QAIC_SZ_16];
  /// Board serial number
  char boardSerial[QAIC_SZ_32];
  /// Reserved field
  uint32_t reserved;

  QBoardInfo() { std::memset(this, 0, sizeof(QBoardInfo)); }
};

/// \brief Data structure returned to from device in status query
/// Message format NNC_STATUS_QUERY_DEV
struct QHostApiInfoDevData {
  uint8_t formatVersion;
  /// Firmware major version number.
  uint8_t fwVersionMajor;
  /// Firmware minor version number.
  /// Firmware updates with minor version updates are backwards compatible.
  uint8_t fwVersionMinor;
  /// Firmware patch version number.
  /// Firmware updates with patch  version updates are backwards compatible.
  uint8_t fwVersionPatch;
  uint8_t fwVersionBuild;
  /// QFirmwareFeatureBitset
  uint8_t fwFeaturesBitmap;
  /// Null terminate string describing the FW image version.
  char fwQCImageVersionString[QAIC_SZ_64];
  /// Null terminated string provided by OEM.
  char fwOEMImageVersionString[QAIC_SZ_64];
  /// Null terminated string describing the variant.
  /// This may contain items such as "Debug" or "Release".
  char fwImageVariantString[QAIC_SZ_64];
  /// Null terminated stirng describing the set of features
  /// included in this FW release.
  char fwImageFeaturesString[QAIC_SZ_64];
  /// Null terminated string describing the
  /// secondary boot loader image.
  char sblImageString[QAIC_SZ_32];

  /// PVS Image Version Number
  uint32_t pvsImageVersion;

  /// Hardware version
  uint32_t hwVersion;

  /// Soc serial number
  char serial[QAIC_SZ_16];

  /// Reserved
  uint32_t reserved;

  /// Fragmentation status. Interpretation of the value TBD.
  uint32_t fragmentationStatus;

  /// \brief Metadata is an internal format generated at network compilation
  /// time that describes how to configure the network.
  /// Metadata major version.
  uint16_t metaVerMaj;
  /// Metadata minor version.
  /// Any change in meta minor version will imply incompatibility.
  uint16_t metaVerMin;

  /// \brief Neural Network Command Protocol, this describes the interface
  /// version between the host runtime and the firmware used
  /// to send and receive commands for Neural Network Control.
  /// .e.g Control Path Protocol.
  /// Neural Network Command Protocol major version.
  uint16_t nncCommandProtocolMajorVersion;
  /// Neural Network Command Protocol minor version.
  uint16_t nncCommandProtocolMinorVersion;

  /// NSP Defective PG Mask
  uint64_t nspPgMask;

  /// Number of loaded constants
  uint32_t numLoadedConsts;

  /// Number of constants in-use
  uint32_t numConstsInUse;

  /// Number of loaded networks
  uint8_t numLoadedNWs;

  /// Number of active networks
  uint8_t numActiveNWs;

  /// The Resource Info provides details on available resources
  /// The same resourceInfo can be obtained with a specific query
  QResourceInfo resourceInfo;

  /// Device Performance Info
  QPerformanceInfo performanceInfo;

  QHostApiInfoDevData() { std::memset(this, 0, sizeof(QHostApiInfoDevData)); }
};

/// \brief Data structure for storing NSP version information.
struct QHostApiInfoNspVersion {
  /// Neural Network Processor major version
  uint8_t major;
  /// Neural Network Processor minor version
  uint8_t minor;
  /// Neural Network Processor patch version
  uint8_t patch;
  /// Neural Network Processor build version
  uint8_t build;
  /// Neural Network Processor description, null terminated string.
  char qc[QAIC_SZ_64];
  /// Neural Network Processor oem description, null terminated string.
  char oem[QAIC_SZ_64];
  /// Neural Network Processor variant description, null terminated string.
  char variant[QAIC_SZ_64];

  QHostApiInfoNspVersion() {
    std::memset(this, 0, sizeof(QHostApiInfoNspVersion));
  }
};

/// \brief Data structure returned to from device in status query
/// Message format NNC_STATUS_QUERY_NSP
struct QHostApiInfoNspData {
  /// Version format for this structure
  QHostApiInfoNspVersion nspFwVersion;
};

/// \brief Device  Information
struct QDevInfo {
  /// Device State
  QDevStatus devStatus;
  /// Device PCI info
  QPciInfo pciInfo;
  /// Device ID
  QID qid;
  /// Device Info
  QHostApiInfoDevData devData;
  /// Neural Network Controller Info
  QHostApiInfoNspData nspData;
  /// Device Node name
  char name[QAIC_SZ_64];
};

/// \brief Direction of data transfer between host and device
enum class QDirection {
  QDIR_BI = 0,
  QDIR_TO_DEV = 1,
  QDIR_FROM_DEV = 2,
  QDIR_NONE = 3,
};

/// \brief Define the type of buffer from the user's perspective
enum class BufferType { BUFFER_TYPE_USER = 0, BUFFER_TYPE_DMA = 1 };

/// Define the direction of a buffer from the user's perspective
/// An input buffer is from user to device
/// An output buffer is from device to user
enum class QAicBufferIoTypeEnum {
  BUFFER_IO_TYPE_INVALID = 0, // Initialized value
  BUFFER_IO_TYPE_INPUT = 1,   // Input Buffer
  BUFFER_IO_TYPE_OUTPUT = 2,  // Output Buffer
  BUFFER_IO_TYPE_INVAL = 3,   // Invalid
};
/// \brief Define the Data Format, dimensions and characteristics for an input
/// or output buffer

/// Define the format of buffer
enum class QAicBufferDataTypeEnum {
  BUFFER_DATA_TYPE_FLOAT = 0,   // 32-bit float type (float)
  BUFFER_DATA_TYPE_FLOAT16 = 1, // 16-bit float type (half, fp16)
  BUFFER_DATA_TYPE_INT8Q = 2,   // 8-bit quantized type (int8_t)
  BUFFER_DATA_TYPE_UINT8Q = 3,  // unsigned 8-bit quantized type (uint8_t)
  BUFFER_DATA_TYPE_INT16Q = 4,  // 16-bit quantized type (int16_t)
  BUFFER_DATA_TYPE_INT32Q = 5,  // 32-bit quantized type (int32_t)
  BUFFER_DATA_TYPE_INT32I = 6,  // 32-bit index type (int32_t)
  BUFFER_DATA_TYPE_INT64I = 7,  // 64-bit index type (int64_t)
  BUFFER_DATA_TYPE_INT8 = 8,    // 8-bit type (int8_t)
  BUFFER_DATA_TYPE_INVAL = 9,
};

/// \brief Define properties for context
enum class QAicContextPropertiesBitfields {
  /// Default Context properties
  QAIC_CONTEXT_DEFAULT = 0x00,
};
using QAicContextProperties = uint32_t;

/// Define queue properties as created
enum class QAicQueuePropertiesBitField {
  QAIC_QUEUE_PROPERTIES_ENABLE_SINGLE_THREADED_QUEUES = 0x0,
  QAIC_QUEUE_PROPERTIES_ENABLE_MULTI_THREADED_QUEUES =
      0x01, // start numThreadsPerQueue at init
  QAIC_QUEUE_PROPERTIES_DEFAULT =
      QAIC_QUEUE_PROPERTIES_ENABLE_MULTI_THREADED_QUEUES,
};

struct QAicQueueProperties {
  QAicQueuePropertiesBitField properties;
  uint32_t numThreadsPerQueue;
};

/// \brief Define program properties as created, properties are a bitmask
/// USE_APP_BUFFER: If selected the caller must retain a valid buffer
/// for the duration of the qaicProgram Object, i.e, until releaseProgram is
/// called.
enum class QAicProgramPropertiesBitfields {
  /// This is the default mask that will be applied if
  /// properties are null, or if the user calls
  /// qaicProgramPropertiesInitDefault

  QAIC_PROGRAM_PROPERTIES_SELECT_MASK_DEFAULT = 0,
  /// if this property is enabled
  /// the caller must retain a valid pointer of the program for
  /// the duration of of the program until releaseProgram is called
  QAIC_PROGRAM_PROPERTIES_USE_APP_BUFFER = 0x08,
};

struct QAicProgramProperties {
  uint32_t selectMask; // QAicProgramPropertiesBitfields
  /// ExecObj submissions enter a limited size queue, if the queue is full
  /// the submission must wait until an inference is completed to make room
  /// in the queue.  Depending on the inference runtime, the following values
  /// may need to be adjusted.
  /// Submission will be retried if a timout is reached, until maximum number
  /// of retires is reached, once this is reached, the submission is considered
  /// failed.
  /// For short running networks and quick failure detection, these values may
  /// be smaller, for long running networks, the values may need to be increased
  /// Use the default initialization function.
  /// If a null pointer is given at the program creation, default values will be
  /// used
  uint32_t SubmitNumRetries;
  uint32_t SubmitRetryTimeoutMs;
  uint64_t reserved02;
};

/// Define execObj properties as created
enum class QAicExecObjPropertiesBitField {
  QAIC_EXECOBJ_PROPERTIES_AUTO_LOAD_ACTIVATE = 0x04,
  QAIC_EXECOBJ_PROPERTIES_DEFAULT = QAIC_EXECOBJ_PROPERTIES_AUTO_LOAD_ACTIVATE,
};
using QAicExecObjProperties = uint32_t;

/// Define the Error occurrence type.
enum class QAicErrorType {
  QAIC_ERROR_CONTEXT_CREATION = 0x100, // Error occurred during context creation
  QAIC_ERROR_CONTEXT_RUNTIME = 0x101,  // Error occurred during context runtime
  QAIC_ERROR_PROGRAM_CREATION = 0x200, // Error occurred during program creation
  QAIC_ERROR_PROGRAM_RUNTIME_LOAD_FAILED =
      0x201, // Error occurred during program load
  QAIC_ERROR_PROGRAM_RUNTIME_ACTIVATION_FAILED =
      0x202, // Error occurred during program activation
  QAIC_ERROR_CONSTANTS_CREATION =
      0x300, // Error occurred during constants creation
  QAIC_ERROR_CONSTANTS_RUNTIME =
      0x301,                         // Error occurred during constants runtime
  QAIC_ERROR_QUEUE_CREATION = 0x400, // Error occurred during queue creation
  QAIC_ERROR_QUEUE_RUNTIME = 0x401,  // Error occurred during queue runtime
  QIAC_ERROR_EXECOBJ_CREATION = 0x500, // Error occurred during execObj creation
  QIAC_ERROR_EXECOBJ_RUNTIME = 0x501,  // Error occurred during execObj runtime
  QIAC_ERROR_EVENT_CREATION = 0x600,   // Error occurred during event creation
  QIAC_ERROR_EVENT_RUNTIME = 0x601,    // Error occurred during event runtime
};

enum class QAicProgramStatus {
  /// Initial state of the program after creation
  QAIC_PROGRAM_INIT = 0,
  /// Program is currently loaded in device memory
  QAIC_PROGRAM_LOADED = 1,
  /// Program is currently activated, all resources to run the program have been
  /// assigned and initialized successfully
  QAIC_PROGRAM_FULLY_ACTIVATED = 2,
  /// All numbers above this are errors
  QAIC_PROGRAM_ERROR = 100,
  /// Program load error occurred
  QIAC_PROGRAM_LOAD_ERROR = 101,
  /// Program activate error
  QIAC_PROGRAM_ACTIVATE_ERROR = 102,
};

enum class QAicProgramActivationCmd {
  /// Command to fully activate
  QAIC_PROGRAM_CMD_ACTIVATE_FULL = 0,
  /// Command to fully deactivate
  QAIC_PROGRAM_CMD_DEACTIVATE_FULL = 1,
  /// Reserved
  QAIC_PROGRAM_CMD_RESERVED = 100,
  QAIC_PROGRAM_CMD_INVAL = 101
};

/// \brief Defines the Buffer mappings.
struct QAicBufferMappings {
  char *bufferName;                // char[] name
  uint32_t index;                  // Maps to QBuffer index in array of buffers
  QAicBufferIoTypeEnum ioType;     // QAicBufferIoTypeEnum
  uint32_t size;                   // Buffer Size in bytes
  bool isPartialBufferAllowed;     // Allow buffer smaller than size
  QAicBufferDataTypeEnum dataType; // IO format type
};

struct QAicIoBufferInfo {
  uint32_t numBufferMappings;
  QAicBufferMappings *bufferMappings; // BufferMappings[]
};

/// \brief Provides ability to retrieve basic information about
/// a QPC, without requiring a context.
/// User can use qaicOpenQpc, and qaicGetQpcInfo, this will return
/// the below information structure.
/// Note the Buffer Mappings can be obtained here or for individual programs
/// through the program object
struct QAicQpcProgramInfo {
  char name[QAIC_SZ_64]; // Name identifying segment
  uint32_t programIndex; // Numeral index uniquely identifying the program
                         // within the QPC.
  uint32_t numCores;     // Number of cores required to run program
  uint32_t size;         // Buffer Size in bytes
  QAicIoBufferInfo ioBufferInfo;    // Buffer Mappings
  QAicIoBufferInfo ioBufferInfoDma; // Buffer Mappings DMA
  int32_t batchSize;                // Program batch size
};

struct QAicQpcConstantsInfo {
  char name[QAIC_SZ_64];   // Name to identify segment
  uint32_t constantsIndex; // Numeral index uniquely identifying the program
                           // within the QPC.
  uint32_t size;           // Buffer Size in bytes
};

// struct QAicQpcInfo {
//   uint32_t numPrograms;
//   QAicQpcProgramInfo *programInfo; // QAicQpcProgramInfo[]
//   uint32_t numConstants;
//   QAicQpcConstantsInfo *constantsInfo; // QAicQpcConstantsInfo[]
// };

struct QAicProgramInfo {
  QAicProgramStatus status;
  /// ID Assigned to program when loaded on device only valid in Loaded State
  /// 0 for Multi-Qaic program.
  uint32_t loadedID;
  /// Host ID used for tracking errors on this program
  QAicProgramID programID;
  /// Number of NSPs required to run program.
  /// 0 for Multi-Qaic Program.
  uint32_t numNsp;
  /// Number of Multicast IDs required to run program
  /// 0 for Multi-Qaic Program.
  uint32_t numMcid;
  /// Batch Size for this program, for each execObj enqueued
  uint32_t batchSize;
  /// Network compiled with AIC stats
  /// Possible values: 0 = false, 1 = true
  uint8_t isAicStatsAvailable;
  uint8_t reserved1;
  uint8_t reserved2;
  uint8_t reserved3;
};

/// \brief Defines buffer mapping. This information is
/// required to create an inferenceVector
struct BufferMapping {
  std::string bufferName;          /// String Name identifying the buffer
  uint32_t index;                  /// Maps to QBuffer index in array of buffers
  QAicBufferIoTypeEnum ioType;     /// QAicBufferIoTypeEnum
  uint32_t size;                   /// Buffer Size in bytes
  bool isPartialBufferAllowed;     /// Allow buffer smaller than size
  QAicBufferDataTypeEnum dataType; // IO format type
  BufferMapping(const char *_name, uint32_t _index,
                QAicBufferIoTypeEnum _ioType, uint32_t _size, bool partial,
                QAicBufferDataTypeEnum format)
      : bufferName(std::string(_name)), index(_index), ioType(_ioType),
        size(_size), isPartialBufferAllowed(partial), dataType(format){};
  BufferMapping()
      : bufferName("Invalid"), index(0),
        ioType(QAicBufferIoTypeEnum::BUFFER_IO_TYPE_INVAL), size(0),
        dataType(QAicBufferDataTypeEnum::BUFFER_DATA_TYPE_INVAL){};
};
using BufferMappings = std::vector<BufferMapping>;

/// \brief Defines Program Info which can be obtained from a QPC
struct QpcProgramInfo {
  BufferMappings bufferMappings;
  BufferMappings bufferMappingsDma;
  std::string name = "Invalid"; /// Name identifying segment
  uint32_t programIndex =
      0; /// Index uniquely identifying the program within the QPC
  uint32_t numCores = 0; /// Number of cores required to run program
  uint32_t size = 0;     /// Program size in bytes
  int32_t batchSize = 0; /// Program batch size
  QpcProgramInfo(const char *_name, uint32_t _programIndex, uint32_t _numCores,
                 uint32_t _size, int32_t _batchSize)
      : name(std::string(_name)), programIndex(_programIndex),
        numCores(_numCores), size(_size), batchSize(_batchSize){};
  QpcProgramInfo(){};
};

/// \brief Defines Constants Info which can be obtained from a QPC
struct QpcConstantsInfo {
  std::string name;
  uint32_t constantsIndex;
  uint32_t size;
  QpcConstantsInfo(const char *_name, uint32_t _constantsIndex, uint32_t _size)
      : name(std::string(_name)), constantsIndex(_constantsIndex),
        size(_size){};
  QpcConstantsInfo() : name("Invalid"), constantsIndex(0), size(0){};
};

struct QpcInfo {
  std::vector<QpcProgramInfo> program;
  std::vector<QpcConstantsInfo> constants;
};

using shQpcInfo = std::shared_ptr<QpcInfo>;

/// \brief Defines Static Program Info.  This information is provided
/// through the program class
/// This information does not change after program creation, thus access
/// is optimized as device query is not required to get the info
struct ProgramInfo {
  QAicProgramID uniqueProgramID = 0; /// Unique program ID assigned by library
  QAicProgramID userDefinedProgramID = 0; /// Program ID defined by the user
  uint32_t numNsp = 0;  /// Number of NSPs required to run program
  uint32_t numMcid = 0; /// Number of Multicast IDs required to run program
  uint32_t batchSize =
      0; /// Batch Size for this program, for each execObj enqueued
  bool isAicStatsAvailable = false; /// Network compiled with AIC stats
  ProgramInfo() {}
  ProgramInfo(QAicProgramInfo &aicInfo)
      : uniqueProgramID(aicInfo.programID),
        userDefinedProgramID(aicInfo.programID), numNsp(aicInfo.numNsp),
        numMcid(aicInfo.numMcid), batchSize(aicInfo.batchSize),
        isAicStatsAvailable(aicInfo.isAicStatsAvailable){};
  ProgramInfo(QAicProgramInfo &aicInfo, QAicProgramID _userDefinedProgramID)
      : uniqueProgramID(aicInfo.programID),
        userDefinedProgramID(_userDefinedProgramID), numNsp(aicInfo.numNsp),
        numMcid(aicInfo.numMcid), batchSize(aicInfo.batchSize),
        isAicStatsAvailable(aicInfo.isAicStatsAvailable){};
  void init(QAicProgramInfo &aicInfo, QAicProgramID _userDefinedProgramID) {
    uniqueProgramID = aicInfo.programID;
    userDefinedProgramID = _userDefinedProgramID;
    numNsp = aicInfo.numNsp;
    numMcid = aicInfo.numMcid;
    batchSize = aicInfo.batchSize;
    isAicStatsAvailable = aicInfo.isAicStatsAvailable;
  }
};

/// \brief Describes dimensions of one buffer
/// In the form of (element size, [dimensiom[0], ...])
/// Element size is the size of the buffer's data type, say 4 bytes for float
using BufferDimensions = std::pair<uint32_t, std::vector<uint32_t>>;

struct ExecObjInfo {
  uint32_t execObjIndex;
  uint32_t activationIndex;
  uint32_t programId;
  std::string programName;
  QStatus status = QS_INVAL;
  ExecObjInfo() : status(QS_INVAL){};
  ExecObjInfo(uint32_t _execObjIndex, uint32_t _activationIndex,
              uint32_t _programId, std::string _programName)
      : execObjIndex(_execObjIndex), activationIndex(_activationIndex),
        programId(_programId), programName(_programName){};
};

#endif // QRUNTIMETYPES_H
