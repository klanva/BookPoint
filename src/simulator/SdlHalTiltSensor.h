#pragma once

#include <cstdint>

namespace CrossPointOrientation {
enum Value : uint8_t { PORTRAIT = 0, LANDSCAPE_CW = 1, INVERTED = 2, LANDSCAPE_CCW = 3 };
}

namespace CrossPointTiltPageTurn {
enum Value : uint8_t { TILT_OFF = 0, TILT_NORMAL = 1, TILT_INVERTED = 2 };
}

class HalTiltSensor;
extern HalTiltSensor halTiltSensor;

class HalTiltSensor {
 public:
  HalTiltSensor() = default;

  void begin() {}
  bool wake() { return true; }
  bool deepSleep() { return true; }
  bool isAvailable() const { return false; }

  void update(const uint8_t, const uint8_t, const bool) {}
  bool wasTiltedForward() { return false; }
  bool wasTiltedBack() { return false; }
  bool hadActivity() { return false; }
  void clearPendingEvents() {}
};

using SdlHalTiltSensor = HalTiltSensor;
