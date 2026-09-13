#pragma once

#include <cstdint>
#include <cstddef>

class NetworkUDP {
 public:
  bool begin(uint16_t) { return true; }
  void stop() {}
  int parsePacket() { return 0; }
  int read(uint8_t*, size_t) { return 0; }
  bool beginPacket(const char*, uint16_t) { return true; }
  size_t write(const uint8_t*, size_t) { return 0; }
  bool endPacket() { return true; }
};
