// Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QAicRuntimeTypes.h"
#include "QNncProtocol.h"
#include "QOsal.h"
#include "QTypes.h"
#include "QUtil.h"
#include <iomanip>
#include <iostream>

namespace qaic {

namespace qutil {

const uint8_t MAX_NUM_NSP = 16;

const char *statusStr(const QStatus &status) {
  switch (status) {
  case QS_SUCCESS:
    return "SUCCESS";
  case QS_PERM:
    return "PERM";
  case QS_AGAIN:
    return "AGAIN";
  case QS_NOMEM:
    return "NOMEM";
  case QS_BUSY:
    return "BUSY";
  case QS_NODEV:
    return "NODEV";
  case QS_INVAL:
    return "INVAL";
  case QS_BADFD:
    return "BADFD";
  case QS_NOSPC:
    return "NOSPC";
  case QS_TIMEDOUT:
    return "TIMEOUT";
  case QS_PROTOCOL:
    return "PROTOCOL";
  case QS_UNSUPPORTED:
    return "UNSUPPORTED";
  case QS_DATA_CRC_ERROR:
    return "DATA_CRC_ERROR";
  case QS_DEV_ERROR:
    return "DEV_ERROR";
  case QS_ERROR:
    return "GENERIC_ERROR";
  }
  return "UNKNOWN";
}

std::string hex(uint64_t n) {
  std::stringstream ss;
  ss << std::hex << n;
  return ss.str();
}

std::string uint32VerToStr(uint32_t version) {
  std::stringstream ss;
  ss << (version >> 24) << "."                // major
     << ((version & 0x00FF0000) >> 16) << "." // minor
     << (version & 0xFFFF);                   // build
  return ss.str();
}

const char *devStatusToStr(QDevStatus devStatus) {
  switch (devStatus) {
  case QDevStatus::QDS_INITIALIZING:
    return "Initializing";
  case QDevStatus::QDS_READY:
    return "Ready";
  case QDevStatus::QDS_ERROR:
    return "Error";
  }
  return "Unknown";
}

std::string str(uint8_t fwFeaturesBitmap) {
  std::stringstream ss;
  if (fwFeaturesBitmap != 0) {
    ss << fmt::format("\n\tFW Features:");
  }
  if (fwFeaturesBitmap & (0x01 << static_cast<uint32_t>(
                              QFirmwareFeatureBits::QAIC_FW_FEATURE_FUSA))) {
    ss << fmt::format("[FUSA]");
  }
  return ss.str();
}

std::string getSkuTypeStr(const uint8_t skuType) {
  static constexpr std::array<const char *,
                              NNC_TRANSACTION_SKU_TYPE_DEV_MAX_SKU>
      enumToStringMap = {
          "Invalid",   "M.2",
          "PCIe",      "PCIe Pro",
          "PCIe Lite", "PCIe Ultra",
          "Auto",      "PCIe Ultra Plus",
          "PCIe 080",  "PCIe Ultra 080",
      };
  if (skuType >= NNC_TRANSACTION_SKU_TYPE_DEV_MAX_SKU) {
    return "Invalid";
  }
  return enumToStringMap[skuType];
}

std::string str(const QDevInfo &info) {
  std::stringstream ss;
  const QPciInfo *pci = &info.pciInfo;
  // Copy Device Data
  ss << fmt::format("QID {}", (uint32_t)info.qid);
  ss << fmt::format("\n\tStatus:{}", devStatusToStr(info.devStatus));
  ss << fmt::format("\n\tPCI Address:{:04x}:{:02x}:{:02x}.{:01x}",
                    (uint32_t)pci->domain, (uint32_t)pci->bus,
                    (uint32_t)pci->device, (uint32_t)pci->function);
  // Copy Device Data
  ss << fmt::format("\n\tPCI Info:{} {} {}", (char *)&pci->classname[0],
                    (char *)&pci->vendorname[0], (char *)&pci->devicename[0]);
  if (pci->pcieExtInfo.maxLinkSpeed != 0) {
    ss << fmt::format(
        "\n\tMax Link Speed:{}",
        (char *)pcieLinkSpeed(pci->pcieExtInfo.maxLinkSpeed).c_str());
    ss << fmt::format("\n\tMax Link Width:x{}", pci->pcieExtInfo.maxLinkWidth);
    ss << fmt::format(
        "\n\tCurrent Link Speed:{}",
        (char *)pcieLinkSpeed(pci->pcieExtInfo.currLinkSpeed).c_str());
    ss << fmt::format("\n\tCurrent Link Width:x{}",
                      pci->pcieExtInfo.currLinkWidth);
  }
  ss << fmt::format("\n\tDev Link:{} ", info.name);
  if (info.devStatus != QDevStatus::QDS_READY) {
    ss << fmt::format("\n");
    return ss.str();
  }

  ss << fmt::format("\n\tHW Version:{}.{}.{}.{}",
                    (uint32_t)((info.devData.hwVersion & 0xF0000000) >> 28),
                    (uint32_t)((info.devData.hwVersion & 0x0FFF0000) >> 16),
                    (uint32_t)((info.devData.hwVersion & 0xff00) >> 8),
                    (uint32_t)(info.devData.hwVersion & 0xff));
  ss << fmt::format("\n\tHW Serial:{}", (char *)info.devData.serial);
  ss << fmt::format("\n\tFW Version:{}.{}.{}",
                    (uint32_t)info.devData.fwVersionMajor,
                    (uint32_t)info.devData.fwVersionMinor,
                    (uint32_t)info.devData.fwVersionPatch);
  if ((uint32_t)info.devData.fwVersionBuild != UCHAR_MAX) {
    ss << fmt::format(".{}", (uint32_t)info.devData.fwVersionBuild);
  }
  ss << fmt::format("\n\tFW QC_IMAGE_VERSION:{}",
                    (char *)info.devData.fwQCImageVersionString);
  ss << fmt::format("\n\tFW OEM_IMAGE_VERSION:{}",
                    (char *)info.devData.fwOEMImageVersionString);
  ss << fmt::format("\n\tFW IMAGE_VARIANT:{}",
                    (char *)info.devData.fwImageVariantString);

  if (info.devData.fwFeaturesBitmap != 0) {
    ss << fmt::format("\n\tFW Features:");
  }
  if (info.devData.fwFeaturesBitmap &
      (0x01 << static_cast<uint32_t>(
           QFirmwareFeatureBits::QAIC_FW_FEATURE_FUSA))) {
    ss << fmt::format("[FUSA]");
  }

  ss << fmt::format("\n\tNSP Version:{}.{}.{}",
                    (uint32_t)info.nspData.nspFwVersion.major,
                    (uint32_t)info.nspData.nspFwVersion.minor,
                    (uint32_t)info.nspData.nspFwVersion.patch);
  if ((uint32_t)info.nspData.nspFwVersion.build != UCHAR_MAX) {
    ss << fmt::format(".{}", (uint32_t)info.nspData.nspFwVersion.build);
  }
  ss << fmt::format("\n\tNSP QC_IMAGE_VERSION:{}",
                    (char *)info.nspData.nspFwVersion.qc);
  ss << fmt::format("\n\tNSP OEM_IMAGE_VERSION:{}",
                    (char *)info.nspData.nspFwVersion.oem);
  ss << fmt::format("\n\tNSP IMAGE_VARIANT:{}",
                    (char *)info.nspData.nspFwVersion.variant);
  ss << fmt::format("\n\tDram Total:{} MB",
                    (uint32_t)info.devData.resourceInfo.dramTotal);
  ss << fmt::format("\n\tDram Free:{} MB",
                    (uint32_t)info.devData.resourceInfo.dramFree);
  ss << fmt::format("\n\tDram Fragmentation:{:03.2f}%",
                    (float)(info.devData.fragmentationStatus / UINT32_MAX));
  ss << fmt::format("\n\tVc Total:{}",
                    (uint32_t)info.devData.resourceInfo.vcTotal);
  ss << fmt::format("\n\tVc Free:{}",
                    (uint32_t)info.devData.resourceInfo.vcFree);
  ss << fmt::format("\n\tPc Total:{}",
                    (uint32_t)info.devData.resourceInfo.pcTotal);
  ss << fmt::format("\n\tPc Reserved:{}",
                    (uint32_t)info.devData.resourceInfo.pcReserved);
  ss << fmt::format("\n\tNsp Total:{}",
                    (uint32_t)info.devData.resourceInfo.nspTotal);
  ss << fmt::format("\n\tNsp Free:{}",
                    (uint32_t)info.devData.resourceInfo.nspFree);
  ss << fmt::format("\n\tLast 100ms Average Dram Bw:{} KBps",
                    (float)info.devData.performanceInfo.dramBw);
  ss << fmt::format("\n\tMCID Total:{}",
                    (uint32_t)info.devData.resourceInfo.mcidTotal);
  ss << fmt::format("\n\tMCID Free:{}",
                    (uint32_t)info.devData.resourceInfo.mcidFree);
  ss << fmt::format("\n\tSemaphore Total:{}",
                    (uint32_t)info.devData.resourceInfo.semTotal);
  ss << fmt::format("\n\tSemaphore Free:{}",
                    (uint32_t)info.devData.resourceInfo.semFree);
  ss << fmt::format("\n\tConstants Loaded:{}",
                    (uint32_t)info.devData.numLoadedConsts);
  ss << fmt::format("\n\tConstants In-Use:{}",
                    (uint32_t)info.devData.numConstsInUse);
  ss << fmt::format("\n\tNetworks Loaded:{}",
                    (uint32_t)info.devData.numLoadedNWs);
  ss << fmt::format("\n\tNetworks Active:{}",
                    (uint32_t)info.devData.numActiveNWs);
  ss << fmt::format(
      "\n\tNSP Frequency(Mhz):{:.2f}",
      ((float)info.devData.performanceInfo.nspFrequencyHz / 1000000));
  ss << fmt::format(
      "\n\tDDR Frequency(Mhz):{:.2f}",
      ((float)info.devData.performanceInfo.ddrFrequencyHz / 1000000));
  ss << fmt::format(
      "\n\tCOMPNOC Frequency(Mhz):{:.2f}",
      ((float)info.devData.performanceInfo.compFrequencyHz / 1000000));
  ss << fmt::format(
      "\n\tMEMNOC Frequency(Mhz):{:.2f}",
      ((float)info.devData.performanceInfo.memFrequencyHz / 1000000));
  ss << fmt::format(
      "\n\tSYSNOC Frequency(Mhz):{:.2f}",
      ((float)info.devData.performanceInfo.sysFrequencyHz / 1000000));

  ss << fmt::format("\n\tMetadata Version:{}.{}",
                    (uint32_t)info.devData.metaVerMaj,
                    (uint32_t)info.devData.metaVerMin);

  ss << fmt::format("\n\tNNC Command Protocol Version:{}.{}",
                    (uint32_t)info.devData.nncCommandProtocolMajorVersion,
                    (uint32_t)info.devData.nncCommandProtocolMinorVersion);

  ss << fmt::format("\n\tSBL Image:{}", info.devData.sblImageString);
  ss << fmt::format("\n\tPVS Image Version:{}",
                    (uint32_t)info.devData.pvsImageVersion);
  // Please keep the format consistent. No space after ":".

  ss << fmt::format("\n\tNSP Defective PG Mask: 0x{:X}",
                    (uint32_t)info.devData.nspPgMask);

  ss << fmt::format("\n\tSku Type:{}",
                    getSkuTypeStr(info.devData.resourceInfo.skuType));
  ss << std::endl;
  return ss.str();
}

std::string str(const QBoardInfo &info) {
  std::stringstream ss;

  ss << fmt::format("\n\tFW Version:{}.{}.{}", (uint32_t)info.fwVersionMajor,
                    (uint32_t)info.fwVersionMinor,
                    (uint32_t)info.fwVersionPatch);
  if ((uint32_t)info.fwVersionBuild != UCHAR_MAX) {
    ss << fmt::format(".{}", (uint32_t)info.fwVersionBuild);
  }
  ss << fmt::format("\n\tFW QC_IMAGE_VERSION:{}",
                    (char *)info.fwQCImageVersionString);
  ss << fmt::format("\n\tFW OEM_IMAGE_VERSION:{}",
                    (char *)info.fwOEMImageVersionString);
  ss << fmt::format("\n\tFW IMAGE_VARIANT:{}",
                    (char *)info.fwImageVariantString);
  ss << fmt::format("\n\tHW Version:{}.{}.{}.{}",
                    (uint32_t)((info.hwVersion & 0xF0000000) >> 28),
                    (uint32_t)((info.hwVersion & 0x0FFF0000) >> 16),
                    (uint32_t)((info.hwVersion & 0xff00) >> 8),
                    (uint32_t)(info.hwVersion & 0xff));

  ss << fmt::format("\n\tBoard serial:{}", (char *)info.boardSerial);
  ss << fmt::format("\n\tPVS Image Version:{}", (uint32_t)info.pvsImageVersion);
  ss << std::endl;
  return ss.str();
}

std::string str(const QResourceInfo &info) {
  std::stringstream ss;
  ss << fmt::format("\n Mem Total             :{:8d}", info.dramTotal);
  ss << fmt::format("\n Mem Free              :{:8d}", info.dramFree);
  ss << fmt::format("\n Nsp Total             :{:8d}", info.nspTotal);
  ss << fmt::format("\n Nsp Free              :{:8d}", info.nspFree);
  ss << fmt::format("\n Sem Total             :{:8d}", info.semTotal);
  ss << fmt::format("\n Sem Free              :{:8d}", info.semFree);
  ss << fmt::format("\n Vc Total              :{:8d}", info.vcTotal);
  ss << fmt::format("\n Vc Free               :{:8d}", info.vcFree);
  ss << fmt::format("\n Pc Total              :{:8d}", info.pcTotal);
  ss << fmt::format("\n Pc Reserved           :{:8d}", info.pcReserved);
  ss << fmt::format("\n Mcid Total            :{:8d}", info.mcidTotal);
  ss << fmt::format("\n Mcid Free             :{:8d}", info.mcidFree);
  ss << std::endl;
  return ss.str();
}

std::string str(const QPerformanceInfo &info) {
  std::stringstream ss;
  ss << fmt::format("\n NSP Frequency (MHz)      :{:8d}",
                    info.nspFrequencyHz / 1000000);
  ss << fmt::format("\n COMPNOC Frequency (MHz)  :{:8d}",
                    info.compFrequencyHz / 1000000);
  ss << fmt::format("\n DDR Frequency (MHz)      :{:8d}",
                    info.ddrFrequencyHz / 1000000);
  ss << fmt::format("\n MEMNOC Frequency (MHz)   :{:8d}",
                    info.memFrequencyHz / 1000000);
  ss << fmt::format("\n SYSNOC Frequency (MHz)   :{:8d}",
                    info.sysFrequencyHz / 1000000);
  ss << fmt::format("\n Peak Compute (ops/s)     :{:8.2f}", info.peakCompute);
  ss << fmt::format("\n Last 100ms Average DRAM BW (KBytes/s)   :{:8.2f}",
                    info.dramBw);
  ss << std::endl;
  return ss.str();
}

std::string uint32ToPCIeStr(uint32_t pcie) {
  std::stringstream ss;
  ss << (pcie >> 24) << ":"                // Domain
     << ((pcie & 0x00FF0000) >> 16) << ":" // Bus
     << ((pcie & 0x0000FF00) >> 8) << "."  // Device
     << (pcie & 0x000000FF);               // Function
  return ss.str();
}

std::string pcieLinkSpeed(uint32_t speed) {
  std::stringstream ss;
  switch (speed) {
  case 1:
    ss << "2.5GT/s";
    break;
  case 2:
    ss << "5GT/s";
    break;
  case 3:
    ss << "8GT/s";
    break;
  case 4:
    ss << "16GT/s";
    break;
  case 5:
    ss << "32GT/s";
    break;
  case 6:
    ss << "64GT/s";
    break;
  default:
    ss << "Unknown";
    break;
  }
  return ss.str();
}

std::string qPciInfoToPCIeStr(QPciInfo pciInfo) {
  std::stringstream ss;
  ss << std::setfill('0') << std::setw(4) << std::setbase(16) << pciInfo.domain
     << ":" << std::setw(2) << std::setbase(16) << unsigned(pciInfo.bus)
     << ":" // Bus
     << std::setw(2) << std::setbase(16) << unsigned(pciInfo.device)
     << "." // Device
     << std::setw(1) << std::setbase(16)
     << unsigned(pciInfo.function); // Function
  return ss.str();
}
void convertToHostFormat(const host_api_info_data_header_internal_t &source,
                         QHostApiInfoDevData &target) {
  target.formatVersion = source.format_version;
}

void populateFwFeatures(const QDeviceFeatureBitset &deviceFeatures,
                        char *target, size_t maxSize, uint8_t &targetBitMap) {
  if (deviceFeatures[QFirmwareFeatureBits::QAIC_FW_FEATURE_FUSA] == true) {
    targetBitMap |= 0x01 << QFirmwareFeatureBits::QAIC_FW_FEATURE_FUSA;
    (void)QOsal::strlcpy(target, "[FUSA]", maxSize);
  }
}

void convertToHostFormat(const host_api_info_dev_data_internal_t &source,
                         const QDeviceFeatureBitset &deviceFeatures,
                         QHostApiInfoDevData &target) {

  memset(target.fwQCImageVersionString, 0,
         sizeof(target.fwQCImageVersionString));
  memset(target.fwOEMImageVersionString, 0,
         sizeof(target.fwOEMImageVersionString));
  memset(target.fwImageVariantString, 0, sizeof(target.fwImageVariantString));
  memset(target.fwImageFeaturesString, 0, sizeof(target.fwImageFeaturesString));
  memset(target.sblImageString, 0, sizeof(target.sblImageString));

  target.fwVersionMajor = source.fwVersion_major;
  target.fwVersionMinor = source.fwVersion_minor;
  target.fwVersionPatch = source.fwVersion_patch;
  target.fwVersionBuild = source.fwVersion_build;
  target.fwFeaturesBitmap = 0;

  (void)QOsal::strlcpy(target.fwQCImageVersionString,
                       source.fwQCImageVersionString,
                       sizeof(target.fwQCImageVersionString));
  (void)QOsal::strlcpy(target.fwOEMImageVersionString,
                       source.fwOEMImageVersionString,
                       sizeof(target.fwOEMImageVersionString));
  (void)QOsal::strlcpy(target.fwImageVariantString, source.fwImageVariantString,
                       sizeof(target.fwImageVariantString));

  populateFwFeatures(deviceFeatures, target.fwImageFeaturesString,
                     sizeof(target.fwImageFeaturesString),
                     target.fwFeaturesBitmap);

  (void)QOsal::strlcpy(target.sblImageString, source.sbl_image_name,
                       sizeof(target.sblImageString));

  target.hwVersion = source.hwVersion;

  (void)QOsal::strlcpy(target.serial, source.serial, sizeof(target.serial));
  target.fragmentationStatus = source.resource_info.fragmentationStatus;

  target.numLoadedConsts = source.numLoadedConsts;
  target.numConstsInUse = source.numConstsInUse;
  target.numLoadedNWs = source.numLoadedNWs;
  target.numActiveNWs = source.numActiveNWs;

  target.metaVerMaj = source.metaVerMaj;
  target.metaVerMin = source.metaVerMin;

  target.nncCommandProtocolMajorVersion = source.nncCommandProtocolMajorVersion;
  target.nncCommandProtocolMinorVersion = source.nncCommandProtocolMinorVersion;

  target.pvsImageVersion = source.pvs_image_version;
  target.nspPgMask = source.nsp_pgmask;

  target.resourceInfo.dramTotal = source.resource_info.dramTotal;
  target.resourceInfo.dramFree = source.resource_info.dramFree;
  target.resourceInfo.vcTotal = source.resource_info.vcTotal;
  target.resourceInfo.vcFree = source.resource_info.vcFree;
  target.resourceInfo.pcTotal = source.resource_info.pcTotal;
  target.resourceInfo.pcReserved = source.resource_info.pcReserved;
  target.resourceInfo.nspTotal = source.resource_info.nspTotal;
  target.resourceInfo.nspFree = source.resource_info.nspFree;
  target.resourceInfo.mcidTotal = source.resource_info.mcidTotal;
  target.resourceInfo.mcidFree = source.resource_info.mcidFree;
  target.resourceInfo.semTotal = source.resource_info.semTotal;
  target.resourceInfo.semFree = source.resource_info.semFree;

  target.performanceInfo.peakCompute = source.perf_info.peakCompute;
  target.performanceInfo.dramBw = source.perf_info.dramBw;
  target.performanceInfo.nspFrequencyHz = source.perf_info.nspFrequencyHz;
  target.performanceInfo.ddrFrequencyHz = source.perf_info.ddrFrequencyHz;
  target.performanceInfo.compFrequencyHz = source.perf_info.compFrequencyHz;
  target.performanceInfo.memFrequencyHz = source.perf_info.memFrequencyHz;
  target.performanceInfo.sysFrequencyHz = source.perf_info.sysFrequencyHz;

  target.resourceInfo.skuType = source.sku_type;
}

void convertToHostFormat(const host_api_info_nsp_static_data_internal_t &source,
                         QHostApiInfoNspVersion &target) {
  target.major = source.major;
  target.minor = source.minor;
  target.patch = source.patch;
  target.build = source.build;

  (void)QOsal::strlcpy(target.qc, source.qc, sizeof(target.qc));
  (void)QOsal::strlcpy(target.oem, source.oem, sizeof(target.oem));
  (void)QOsal::strlcpy(target.variant, source.variant, sizeof(target.variant));
}

void convertToHostFormat(const host_api_info_nsp_data_internal_t &source,
                         QHostApiInfoNspData &target) {
  convertToHostFormat(source.nsp_fw_version, target.nspFwVersion);
}

void convertToHostFormat(const host_api_info_dev_data_internal_t &source,
                         const QDeviceFeatureBitset &deviceFeatures,
                         QBoardInfo &target) {

  memset(target.fwQCImageVersionString, 0,
         sizeof(target.fwQCImageVersionString));
  memset(target.fwOEMImageVersionString, 0,
         sizeof(target.fwOEMImageVersionString));
  memset(target.fwImageVariantString, 0, sizeof(target.fwImageVariantString));
  memset(target.fwImageFeaturesString, 0, sizeof(target.fwImageFeaturesString));
  memset(target.sblImageString, 0, sizeof(target.sblImageString));

  target.fwVersionMajor = source.fwVersion_major;
  target.fwVersionMinor = source.fwVersion_minor;
  target.fwVersionPatch = source.fwVersion_patch;
  target.fwVersionBuild = source.fwVersion_build;
  target.fwFeaturesBitmap = 0;

  // Other features need to append to this string

  (void)QOsal::strlcpy(target.fwQCImageVersionString,
                       source.fwQCImageVersionString,
                       sizeof(target.fwQCImageVersionString));
  (void)QOsal::strlcpy(target.fwOEMImageVersionString,
                       source.fwOEMImageVersionString,
                       sizeof(target.fwOEMImageVersionString));
  (void)QOsal::strlcpy(target.fwImageVariantString, source.fwImageVariantString,
                       sizeof(target.fwImageVariantString));

  populateFwFeatures(deviceFeatures, target.fwImageFeaturesString,
                     sizeof(target.fwImageFeaturesString),
                     target.fwFeaturesBitmap);

  target.hwVersion = source.hwVersion;

  (void)QOsal::strlcpy(target.serial, source.serial, sizeof(target.serial));
  (void)QOsal::strlcpy(target.boardSerial, source.board_serial,
                       sizeof(target.boardSerial));
}
} // namespace qutil

} // namespace qaic
