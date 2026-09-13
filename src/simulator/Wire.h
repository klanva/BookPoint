#pragma once

#include <cstdint>
#include <cstddef>

class TwoWire {
 public:
  void begin() {}
  void beginTransmission(uint8_t) {}
  uint8_t endTransmission(bool = true) { return 0; }
  uint8_t requestFrom(uint8_t, uint8_t) { return 0; }
  int read() { return -1; }
  size_t write(uint8_t) { return 1; }
};

inline TwoWire Wire;
