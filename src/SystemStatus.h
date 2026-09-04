#pragma once

#include <Arduino.h>
#include <BoardConfig.h>
#include <HalStorage.h>
#include <Logging.h>
#include <WiFi.h>
#include <esp_heap_caps.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>

#include "HalGPIO.h"
#include "HalPowerManager.h"

inline const char* displayControllerName(BoardConfig::DisplayController controller) {
  switch (controller) {
    case BoardConfig::DisplayController::SSD1677:
      return "SSD1677";
    case BoardConfig::DisplayController::UC8253:
      return "UC8253";
    case BoardConfig::DisplayController::UC8279:
      return "UC8279";
    case BoardConfig::DisplayController::UC8179:
      return "UC8179";
    case BoardConfig::DisplayController::ED2208:
      return "ED2208";
    case BoardConfig::DisplayController::LgfxEpd:
      return "LovyanGFX";
    case BoardConfig::DisplayController::IT8951:
      return "IT8951";
    default:
      return "Unknown";
  }
}

#ifndef CROSSPOINT_DISPLAY_SDK
#define CROSSPOINT_DISPLAY_SDK "FreeInk SDK"
#endif

struct SystemStatus {
  const char* version;
  const char* displaySdk;
  const char* deviceType;
  const char* boardProfile;
  const char* displayController;
  uint16_t displayWidth;
  uint16_t displayHeight;
  std::string chipVersion;
  uint32_t cpuFreqMHz;
  std::string ip;
  std::string wifiMode;
  int rssi;
  std::string macAddress;
  uint32_t freeHeapBytes;
  uint32_t minFreeHeapBytes;
  uint32_t maxAllocHeapBytes;
  uint64_t flashBytes;
  uint64_t flashAppPartitionSize;
  uint16_t batteryPercent;
  bool charging;
  uint32_t uptimeSeconds;
  uint64_t sdTotalBytes;
  uint64_t sdUsedBytes;
  uint64_t sdFreeBytes;
  uint64_t fontCacheTotalBytes;

  static SystemStatus collectFast() {
    SystemStatus s;
    s.version = CROSSPOINT_VERSION;
    s.displaySdk = CROSSPOINT_DISPLAY_SDK;
    s.deviceType = BoardConfig::ACTIVE.name;
    s.boardProfile = BoardConfig::ACTIVE.name;
    s.displayWidth = BoardConfig::ACTIVE.displayWidth;
    s.displayHeight = BoardConfig::ACTIVE.displayHeight;
    s.displayController = displayControllerName(BoardConfig::ACTIVE.displayController);
    s.chipVersion = ESP.getChipModel();
    s.chipVersion += " rev ";
    s.chipVersion += std::to_string(ESP.getChipRevision());
    s.cpuFreqMHz = static_cast<uint32_t>(getCpuFrequencyMhz());
    s.freeHeapBytes = ESP.getFreeHeap();
    s.minFreeHeapBytes = ESP.getMinFreeHeap();
    s.maxAllocHeapBytes = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    s.flashBytes = static_cast<uint64_t>(ESP.getFlashChipSize());

    const esp_partition_t* running = esp_ota_get_running_partition();
    s.flashAppPartitionSize = running ? static_cast<uint64_t>(running->size) : 0;
    s.batteryPercent = powerManager.getBatteryPercentage();
    s.charging = gpio.isUsbConnected();
    s.uptimeSeconds = millis() / 1000;
    s.macAddress = WiFi.macAddress().c_str();

    s.sdTotalBytes = 0;
    s.sdUsedBytes = 0;
    s.sdFreeBytes = 0;

    const esp_partition_t* part =
        esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, "spiffs");
    s.fontCacheTotalBytes = part ? static_cast<uint64_t>(part->size) : 0;

    const wifi_mode_t mode = WiFi.getMode();
    const bool isAP = (mode == WIFI_MODE_AP) || (mode == WIFI_MODE_APSTA);
    if (isAP) {
      s.wifiMode = "AP";
      s.ip = WiFi.softAPIP().toString().c_str();
      s.rssi = 0;
    } else if (WiFi.status() == WL_CONNECTED) {
      s.wifiMode = "STA";
      s.ip = WiFi.localIP().toString().c_str();
      s.rssi = WiFi.RSSI();
    } else {
      s.wifiMode = "Off";
      s.ip = "-";
      s.rssi = 0;
    }

    return s;
  }

  static void fillSdStatus(SystemStatus& s) {
    s.sdTotalBytes = Storage.sdTotalBytes();
    s.sdUsedBytes = Storage.sdUsedBytes();
    s.sdFreeBytes = Storage.sdFreeBytes();
  }

  static SystemStatus collect() {
    SystemStatus s = collectFast();
    fillSdStatus(s);
    return s;
  }
};
