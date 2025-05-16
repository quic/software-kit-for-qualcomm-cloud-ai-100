// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include <stddef.h> // for size_t
#include <memory>   // for shared_ptr, __shared_ptr_access, allocator
#include <string> // for string, basic_string, to_string, operator+, char_traits
#include <vector> // for vector
#include <chrono>
#include <thread>
#include <iostream>
#include <fstream>
#include <map>

#include "ftxui/component/captured_mouse.hpp"     // for ftxui
#include "ftxui/component/component.hpp"          // for Radiobox, Vertical, Checkbox, Horizontal, Renderer, ResizableSplitBottom, ResizableSplitRight
#include "ftxui/component/component_base.hpp"     // for ComponentBase
#include "ftxui/component/screen_interactive.hpp" // for ScreenInteractive
#include "ftxui/dom/elements.hpp"                 // for text, window, operator|, vbox, hbox, Element, flexbox, bgcolor, filler, flex, size, border, hcenter, color, EQUAL, bold, dim, notflex, xflex_grow, yflex_grow, HEIGHT, WIDTH
#include "ftxui/dom/flexbox_config.hpp"           // for FlexboxConfig, FlexboxConfig::AlignContent, FlexboxConfig::JustifyContent, FlexboxConfig::AlignContent::Center, FlexboxConfig::AlignItems, FlexboxConfig::Direction, FlexboxConfig::JustifyContent::Center, FlexboxConfig::Wrap
#include "ftxui/screen/color.hpp"                 // for Color, Color::Black
#include "ftxui/screen/terminal.hpp"

/* AIC header */
#include "QRuntimePlatformApi.h"
#include "QAicOpenRtVersion.hpp"
#include "QUtil.h"
#include "spdlog/spdlog.h"
#include "spdlog/fmt/bundled/chrono.h"

using namespace ftxui;
using namespace qaic;
using namespace qaic::openrt;

// clang-format off
#ifdef DEBUG_FTXUI
std::ofstream logFileHandle;
#define OpenLogfile(fileName) logFileHandle.open(fileName)
#define CloseLogfile(fileName) logFileHandle.close()
#define LogTofile(stdString) logFileHandle << "[" << getCurrentTime() << "] " << stdString << std::endl;
#else
#define OpenLogfile(fileName)
#define CloseLogfile(fileName)
#define LogTofile(stdString)
#endif
// clang-format on

struct DeviceDisplayHeader {
  std::string deviceId;
  std::string statusAndFwVer;
  DeviceDisplayHeader(const std::string &argDeviceId,
                      const std::string &argStatusAndFwVer) {
    deviceId = argDeviceId;
    statusAndFwVer = argStatusAndFwVer;
  }
};

struct ParamDisplayInfo {
  std::string name;
  std::string value;
  int dimx;
  int dimy;
  ParamDisplayInfo(const std::string &argName, const std::string &argValue,
                   int argDimx, int argDimy) {
    name = argName;
    value = argValue;
    dimx = argDimx;
    dimy = argDimy;
  }
};

typedef enum {
  NSP_USED_BY_TOTAL,
  NW_LOADED_BY_ACTIVE,
  DRAM_USED_BY_TOTAL,
  DRAM_BW,
  NSP_FREQUENCY,
  DDR_FREQUENCY,
  TEMPERATURE,
  POWER_BY_TDP_CAP,
} ParamID;

std::mutex deviceInfoMapMutex; // mutex for both the maps below
std::map<QID, DeviceDisplayHeader> displayheaderMap;
std::map<QID, std::map<ParamID, ParamDisplayInfo>> paramMap;

constexpr int32_t allDevices = -1;
// Below two vectors track each other for device selction.
// 1st position is always for allDevices = -1
// std::mutex deviceIdsMutex;
std::vector<std::string> deviceIdStr;
std::vector<QID> deviceIds;
int deviceIdSelectIndex = 0;

static void initializeParamMap(const std::vector<QID> &devList) {
  std::unique_lock<std::mutex> lock(deviceInfoMapMutex);
  for (const auto &qid : devList) {
    // clang-format off
    displayheaderMap.insert(std::pair(qid, (DeviceDisplayHeader(
      fmt::format("DeviceID-{}", qid), "Status: Invalid --- FW Version: Invalid"))));

    paramMap.emplace(qid, (std::map<ParamID, ParamDisplayInfo>()));
    paramMap[qid].emplace(
      NSP_USED_BY_TOTAL, (ParamDisplayInfo("NSP Used / Total","Invalid", 20, 3)));
    paramMap[qid].emplace(
      NW_LOADED_BY_ACTIVE, (ParamDisplayInfo("NW Loaded / Active","Invalid", 20, 3)));
    paramMap[qid].emplace(
      DRAM_USED_BY_TOTAL, (ParamDisplayInfo("DRAM Used / Total","Invalid", 20, 3)));
    paramMap[qid].emplace(
      DRAM_BW, (ParamDisplayInfo("DRAM Bandwidth","Invalid", 20, 3)));
    paramMap[qid].emplace(
      NSP_FREQUENCY, (ParamDisplayInfo("NSP Frequency","Invalid", 20, 3)));
    paramMap[qid].emplace(
      DDR_FREQUENCY, (ParamDisplayInfo("DDR Frequency","Invalid", 20, 3)));
    paramMap[qid].emplace(
      TEMPERATURE, (ParamDisplayInfo("Temperature","Invalid", 20, 3)));
    paramMap[qid].emplace(
      POWER_BY_TDP_CAP, (ParamDisplayInfo("Power / TDP Cap","Invalid", 20, 3)));
    // clang-format on
  }
}

static void updateDeviceInfoPerQid(QID qid) {

  QDevInfo devInfo;
  std::memset(&devInfo, 0, sizeof(QDevInfo));
  shQRuntimePlatformInterface rt = rtPlatformApi::qaicGetRuntimePlatform();
  if (QS_SUCCESS == rt->queryStatus(qid, devInfo)) {
    std::unique_lock<std::mutex> lock(deviceInfoMapMutex);

    std::string fwVersionBuildStr = "";
    if ((uint32_t)devInfo.devData.fwVersionBuild != UCHAR_MAX) {
      fwVersionBuildStr =
          fmt::format(".{}", (uint32_t)devInfo.devData.fwVersionBuild);
    }
    displayheaderMap.at(qid).statusAndFwVer = fmt::format(
        "Status: {} --- FW Version: {}.{}.{}{}",
        qutil::devStatusToStr(devInfo.devStatus),
        (uint32_t)devInfo.devData.fwVersionMajor,
        (uint32_t)devInfo.devData.fwVersionMinor,
        (uint32_t)devInfo.devData.fwVersionPatch, fwVersionBuildStr);

    paramMap.at(qid).at(NSP_USED_BY_TOTAL).value = fmt::format(
        "{} / {}", (uint32_t)(devInfo.devData.resourceInfo.nspTotal -
                              devInfo.devData.resourceInfo.nspFree),
        (uint32_t)devInfo.devData.resourceInfo.nspTotal);
    paramMap.at(qid).at(NW_LOADED_BY_ACTIVE).value =
        fmt::format("{} / {}", (uint32_t)devInfo.devData.numLoadedNWs,
                    (uint32_t)devInfo.devData.numActiveNWs);
    paramMap.at(qid).at(DRAM_USED_BY_TOTAL).value = fmt::format(
        "{} MB / {} MB", (uint32_t)(devInfo.devData.resourceInfo.dramTotal -
                                    devInfo.devData.resourceInfo.dramFree),
        (uint32_t)devInfo.devData.resourceInfo.dramTotal);
    paramMap.at(qid).at(DRAM_BW).value =
        fmt::format("{:.2f} KBps", devInfo.devData.performanceInfo.dramBw);
    paramMap.at(qid).at(NSP_FREQUENCY).value = fmt::format(
        "{:.2f} Mhz",
        ((float)devInfo.devData.performanceInfo.nspFrequencyHz / 1000000));
    paramMap.at(qid).at(DDR_FREQUENCY).value = fmt::format(
        "{:.2f} Mhz",
        ((float)devInfo.devData.performanceInfo.ddrFrequencyHz / 1000000));
    ;
  } else {
    std::unique_lock<std::mutex> lock(deviceInfoMapMutex);
    displayheaderMap.at(qid).statusAndFwVer =
        "Status: Invalid --- FW Version: Invalid";

    paramMap.at(qid).at(NSP_USED_BY_TOTAL).value = "Invalid";
    paramMap.at(qid).at(NW_LOADED_BY_ACTIVE).value = "Invalid";
    paramMap.at(qid).at(DRAM_USED_BY_TOTAL).value = "Invalid";
    paramMap.at(qid).at(DRAM_BW).value = "Invalid";
    paramMap.at(qid).at(NSP_FREQUENCY).value = "Invalid";
    paramMap.at(qid).at(DDR_FREQUENCY).value = "Invalid";
  }

  QTelemetryInfo telemetryInfo;
  if (QS_SUCCESS == rt->getTelemetryInfo(qid, telemetryInfo)) {
    std::unique_lock<std::mutex> lock(deviceInfoMapMutex);
    paramMap.at(qid).at(TEMPERATURE).value =
        fmt::format("{:.2f} C", ((float)telemetryInfo.socTemperature / 1000));
    paramMap.at(qid).at(POWER_BY_TDP_CAP).value = fmt::format(
        "{:.2f} W / {:.2f} W", ((float)telemetryInfo.boardPower / 1000000),
        ((float)telemetryInfo.tdpCap / 1000000));
  } else {
    std::unique_lock<std::mutex> lock(deviceInfoMapMutex);
    paramMap.at(qid).at(TEMPERATURE).value = "Invalid";
    paramMap.at(qid).at(POWER_BY_TDP_CAP).value = "Invalid";
  }
}

static void updateDeviceInfoMap() {
  // std::unique_lock<std::mutex> lock(deviceIdsMutex);
  QID deviceIdSelected = deviceIds[deviceIdSelectIndex];
  if (deviceIdSelected == allDevices) {
    // Skip first entry in deviceIds array
    for (auto it = deviceIds.begin() + 1; it != deviceIds.end(); ++it) {
      updateDeviceInfoPerQid(*it);
    }
  } else {
    updateDeviceInfoPerQid(deviceIdSelected);
  }
}

static std::string getAicVersionInfo() {
  return fmt::format("OPEN_RUNTIME.{}.{}.{}_{}", VersionInfo::majorVersion(),
                     VersionInfo::minorVersion(), VersionInfo::patchVersion(),
                     VersionInfo::variant());
}

static std::string getCurrentTime() {
  std::time_t t = std::time(nullptr);
  return fmt::format("{:year %Y month %m day %d - %H:%M:%S}",
                     fmt::localtime(t));
}

std::mutex deviceInfoThreadMutex;
std::condition_variable deviceInfoThreadCV;
std::atomic_bool stopUpdateDeviceInfoThread;

/* Entry Function */
int txUIDisplayTable(uint32_t tableRefreshRate,
                     const std::vector<QID> &devList) {
  if (devList.size() == 0) {
    std::cout << "Device list is zero" << std::endl;
    return -1;
  }

  OpenLogfile("txUILog.txt");
  deviceIdStr.push_back("All-Devices");
  deviceIds.push_back(allDevices);
  for (const auto &qid : devList) {
    deviceIdStr.push_back(fmt::format("DeviceID-{}", qid));
    deviceIds.push_back(qid);
  }

  initializeParamMap(devList);
  updateDeviceInfoMap();
  std::string rtLibVersionStr = getAicVersionInfo();

  auto screen = ScreenInteractive::Fullscreen();

  stopUpdateDeviceInfoThread.store(false, std::memory_order_relaxed);
  auto updateDeviceInfo = [&](int refreshRate) {
    while (1) {
      std::unique_lock<std::mutex> lock(deviceInfoThreadMutex);
      deviceInfoThreadCV.wait_for(
          lock, std::chrono::seconds(refreshRate), []() {
            return stopUpdateDeviceInfoThread.load(std::memory_order_relaxed);
          });
      if (stopUpdateDeviceInfoThread.load(std::memory_order_relaxed)) {
        break;
      }
      updateDeviceInfoMap();
      screen.PostEvent(Event::Custom);
    }
  };

  std::thread updateDeviceInfoThread(updateDeviceInfo, tableRefreshRate);

  auto radioboxDeviceSelection = Radiobox(&deviceIdStr, &deviceIdSelectIndex);

  auto makeBox = [&](const std::string &title, const std::string &textValue,
                     size_t dimx, size_t dimy) {
    auto element = window(text(title) | bold,
                          text(textValue) | center | align_right | bold) |
                   size(WIDTH, EQUAL, dimx) | size(HEIGHT, EQUAL, dimy);
    return element;
  };

  [[maybe_unused]] auto makeBlankBox = [&](size_t dimx, size_t dimy) {
    auto element = window(text(""), text("") | center | bold) | borderEmpty |
                   size(WIDTH, EQUAL, dimx) | size(HEIGHT, EQUAL, dimy);
    return element;
  };

  QID deviceInfoLayoutQid = 0;

  auto getDeviceInfoBoxTitle = [&] {
    std::unique_lock<std::mutex> lock(deviceInfoMapMutex);
    auto &hdrEntry = displayheaderMap.at(deviceInfoLayoutQid);
    return fmt::format("{} --- {}", hdrEntry.deviceId, hdrEntry.statusAndFwVer);
  };

  auto getDeviceInfoBoxElements = [&] {
    Elements elements;
    std::unique_lock<std::mutex> lock(deviceInfoMapMutex);
    auto &paramMapEntry = paramMap.at(deviceInfoLayoutQid);
    for (auto &it : paramMapEntry) {
      elements.push_back(makeBox(it.second.name, it.second.value,
                                 it.second.dimx, it.second.dimy));
    }
    return elements;
  };

  auto contentRenderer = Renderer([&] {
    auto group = flexbox(getDeviceInfoBoxElements()) | xflex |
                 size(HEIGHT, GREATER_THAN, 6) | size(WIDTH, LESS_THAN, 100);
    return group;
  });

  auto getDeviceInfoGrid = [&] {
    std::vector<Elements> rows;
    // Elements elements;
    // std::unique_lock<std::mutex> lock(deviceIdsMutex);
    QID deviceIdSelected = deviceIds[deviceIdSelectIndex];

    if (deviceIdSelected == allDevices) {
      // Skip first entry in deviceIds array
      for (auto it = deviceIds.begin() + 1; it != deviceIds.end(); ++it) {
        deviceInfoLayoutQid = *it;
        std::vector<Element> cols;
        cols.push_back({
            window(text(getDeviceInfoBoxTitle()), contentRenderer->Render()),
        });
        rows.push_back(cols);
      }
    } else {
      deviceInfoLayoutQid = deviceIdSelected;
      std::vector<Element> cols;
      cols.push_back({
          window(text(getDeviceInfoBoxTitle()), contentRenderer->Render()),
      });
      rows.push_back(cols);
    }

    return gridbox(rows);
  };

  float focus_y = 0.0f;
  auto slider_y = Slider("Scroll-Up-Down", &focus_y, 0.f, 1.f, 0.01f);

  auto main_container = Container::Vertical({
      slider_y, radioboxDeviceSelection,
  });

  auto main_renderer = Renderer(main_container, [&] {

    auto title = "Date & time: " + getCurrentTime() +
                 "  ---  LRT Image Version: " + rtLibVersionStr;

    return vbox(
        {text(title), separator() | size(WIDTH, LESS_THAN, 120),
         hbox({
             vbox({hbox({
                 window(text("Device Selection"),
                        radioboxDeviceSelection->Render() | vscroll_indicator |
                            frame | size(HEIGHT, LESS_THAN, 10)),
             })}),
             vbox(slider_y->Render(), separator(),
                  getDeviceInfoGrid() | focusPositionRelative(0.0f, focus_y) |
                      frame | flex),
         })});
  });

  screen.Loop(main_renderer);
  // Ctrl + C, stops screen loop and program control comes here
  {
    std::unique_lock<std::mutex> lock(deviceInfoThreadMutex);
    stopUpdateDeviceInfoThread.store(true, std::memory_order_relaxed);
    lock.unlock();
    deviceInfoThreadCV.notify_one();
    updateDeviceInfoThread.join();
  }

  CloseLogfile("txUILog.txt");
  return 0;
}
