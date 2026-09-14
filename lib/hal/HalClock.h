#pragma once

#include <Arduino.h>
#include <Rtc.h>
#include <Wire.h>

#include "HalGPIO.h"

class HalClock;
extern HalClock halClock;  // Singleton

class HalClock {
  bool _available = false;
  // True when a physical RTC chip was found (BM8563/PCF8563 on X4 Pro, DS3231 on X3).
  // False means we fall back to a software clock backed by the ESP32's internal timekeeping,
  // which is populated by syncFromNTP() and needs a fresh NTP sync after real power loss.
  bool _useHardwareRtc = false;
  // True when the clock is currently running on a fallback value (e.g. seeded from flash)
  // rather than a confirmed hardware RTC read or a fresh NTP sync.
  mutable bool _usingFallbackTime = false;
  freeink::Rtc _rtc;

  mutable uint8_t _cachedHour = 0;
  mutable uint8_t _cachedMinute = 0;
  mutable uint16_t _cachedYear = 2000;
  mutable uint8_t _cachedMonth = 1;
  mutable uint8_t _cachedDay = 1;
  mutable bool _hasCachedTime = false;
  mutable bool _hasCachedDate = false;
  mutable unsigned long _lastPollMs = 0;

  static constexpr unsigned long CLOCK_POLL_MS = 10000;  // 10 seconds

 public:
  // Call after gpio.begin() and powerManager.begin()
  void begin();

  // True if a usable clock source exists on this device: hardware RTC or software clock.
  bool isAvailable() const { return _available; }

  // True if this device has a physical hardware RTC chip detected and functioning.
  bool hasHardwareRtc() const { return _available && _useHardwareRtc; }

  // True if this device has no battery-backed RTC chip and instead relies on the
  // software clock, which loses its value on power loss and needs an NTP sync.
  bool needsPeriodicNTPSync() const { return _available && !_useHardwareRtc; }

  // Get current hour (0-23) and minute (0-59) in UTC.
  // Returns false if no clock source is available, or the software clock hasn't been synced yet.
  bool getTime(uint8_t& hour, uint8_t& minute) const;

  // Format time into a caller-provided buffer.
  // 24h mode produces "HH:MM" (needs >=6 bytes); 12h mode produces "H:MM AM"/"HH:MM PM" (needs >=9 bytes).
  // utcOffsetQuarterHoursBiased: biased quarter-hour offset (48 = UTC+0, 0 = UTC-12, 104 = UTC+14).
  // use12Hour: when true, format as 12-hour clock with AM/PM suffix.
  // Returns false if no time value is available.
  bool formatTime(char* buf, size_t bufSize, uint8_t utcOffsetQuarterHoursBiased = 48, bool use12Hour = false) const;

  // Returns the raw clock date/time in UTC before user timezone offset is applied.
  bool getDateTime(uint16_t& year, uint8_t& month, uint8_t& day, uint8_t& hour, uint8_t& minute) const {
    return getDate(year, month, day, hour, minute);
  }

  // Format date into caller-provided buffer as "Mon D, YYYY" with timezone offset applied.
  bool formatDate(char* buf, size_t bufSize, uint8_t utcOffsetQuarterHoursBiased = 48) const;

  // Set date and time (in UTC). Writes to hardware RTC if available and updates ESP32 system clock.
  bool setDateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second);

  // Sync the clock from an NTP server. Requires WiFi to be connected.
  // Writes to hardware RTC if present, and updates ESP32 system clock.
  bool syncFromNTP();

  // Seed clock from a persisted UTC epoch if no hardware RTC time is available.
  bool seedFallbackTime(time_t epochUtc);

  // True if time/date is currently running on an approximate fallback.
  bool isUsingFallbackTime() const { return _usingFallbackTime; }

 private:
  bool getDate(uint16_t& year, uint8_t& month, uint8_t& day, uint8_t& hour, uint8_t& minute) const;
  bool writeDateTimeToRTC(uint16_t year, uint8_t month, uint8_t day, uint8_t weekday, uint8_t hour, uint8_t minute,
                          uint8_t second);
};
