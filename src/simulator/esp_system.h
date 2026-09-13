#pragma once

#include <cstdint>
#include <chrono>

inline void esp_restart() {}
inline uint32_t esp_get_free_heap_size() { return 16 * 1024 * 1024; }

inline uint32_t esp_random() {
  static uint32_t state = 0x9e3779b9U;
  auto now = std::chrono::steady_clock::now().time_since_epoch().count();
  state = state * 1664525U + 1013904223U + static_cast<uint32_t>(now);
  return state;
}
