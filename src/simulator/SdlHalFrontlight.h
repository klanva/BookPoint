#pragma once

#include <cstdint>

class HalFrontlight {
 public:
  static HalFrontlight& getInstance() {
    static HalFrontlight instance;
    return instance;
  }

  void begin(uint8_t brightness, uint8_t warmth, bool on) {
    lastBrightness = brightness;
    currentWarmth = warmth;
    lit = on;
  }

  bool present() const { return true; }
  bool hasColorTemperature() const { return true; }

  void setBrightness(uint8_t percent) { lastBrightness = percent; }
  void setWarmth(uint8_t warmPercent) { currentWarmth = warmPercent; }
  void setOn(bool on) { lit = on; }

  void park() {}
  void releaseOnWake() {}

  uint8_t brightness() const { return lastBrightness; }
  uint8_t warmth() const { return currentWarmth; }
  bool isOn() const { return lit; }

 private:
  HalFrontlight() = default;
  uint8_t lastBrightness = 60;
  uint8_t currentWarmth = 50;
  bool lit = false;
};

#define Frontlight HalFrontlight::getInstance()
using SdlHalFrontlight = HalFrontlight;
