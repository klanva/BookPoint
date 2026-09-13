#pragma once

#include <cstdint>
#include <cstddef>

class EspClass {
 public:
  void restart() {}
  const char* getChipModel() { return "ESP32-S3"; }
  uint8_t getChipRevision() { return 1; }
  uint32_t getFreeHeap() { return 16 * 1024 * 1024; }
  uint32_t getMinFreeHeap() { return 14 * 1024 * 1024; }
  uint32_t getMaxAllocHeap() { return 16 * 1024 * 1024; }
  uint32_t getHeapSize() { return 16 * 1024 * 1024; }
  uint32_t getFlashChipSize() { return 16 * 1024 * 1024; }
};

inline EspClass ESP;

inline uint32_t getCpuFrequencyMhz() {
  return 240;
}
