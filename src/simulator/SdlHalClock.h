#pragma once

#include <cstdint>
#include <cstddef>
#include <ctime>
#include <chrono>

class HalClock {
 public:
  HalClock() = default;
  void begin() {}
  bool isAvailable() const { return true; }
  bool needsPeriodicNTPSync() const { return false; }
  bool syncFromNTP() { return true; }

  bool getTime(uint8_t& hour, uint8_t& minute) const;
  bool getDate(uint16_t& year, uint8_t& month, uint8_t& day, uint8_t& hour, uint8_t& minute) const;
  bool getDateTime(uint16_t& year, uint8_t& month, uint8_t& day, uint8_t& hour, uint8_t& minute) const {
    return getDate(year, month, day, hour, minute);
  }
  bool formatTime(char* buf, size_t bufSize, uint8_t utcOffsetQuarterHoursBiased = 48, bool use12Hour = false) const;
  bool formatDate(char* buf, size_t bufSize, uint8_t utcOffsetQuarterHoursBiased = 48) const;
};

extern HalClock halClock;
using SdlHalClock = HalClock;
