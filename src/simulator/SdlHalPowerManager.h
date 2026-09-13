#pragma once

#include "Arduino.h"
#include "HalGPIO.h"
#include <cstdint>

class HalPowerManager;
extern HalPowerManager powerManager;

class HalPowerManager {
 public:
  HalPowerManager() = default;

  void begin() {}
  void setPowerSaving(bool) {}
  bool tryLightSleepSlice(const HalGPIO&) { return false; }
  void startDeepSleep(HalGPIO&) const {}

  uint16_t getBatteryPercentage() const { return 88; }
  uint16_t getBatteryVoltageMv() const { return 3950; }

  class Lock {
   public:
    explicit Lock() = default;
    ~Lock() = default;
    Lock(const Lock&) = delete;
    Lock& operator=(const Lock&) = delete;
    Lock(Lock&&) = delete;
    Lock& operator=(Lock&&) = delete;
  };
};

using SdlHalPowerManager = HalPowerManager;
