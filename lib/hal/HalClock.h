#pragma once

#include <Arduino.h>
#include <Rtc.h>
#include <time.h>

class HalClock;
extern HalClock halClock;  // Singleton

class HalClock {
  bool _available = false;
  // True when the software clock (X4: no hardware RTC) is currently running on a
  // value seeded by seedFallbackTime() rather than a real hardware RTC read or
  // this boot's own NTP sync. Always false on X3 (real battery-backed RTC).
  bool _usingFallbackTime = false;
  mutable Rtc _sdkRtc;
  mutable uint8_t _cachedHour = 0;
  mutable uint8_t _cachedMinute = 0;
  mutable bool _hasCachedTime = false;
  mutable unsigned long _lastPollMs = 0;

  static constexpr unsigned long CLOCK_POLL_MS = 10000;  // 10 seconds
  // Software clock (X4) reads are considered "unsynced" below this epoch value
  // (2024-01-01 00:00:00 UTC). The ESP32 boots with its internal clock near
  // zero, so this filters out that default before the first successful NTP sync.
  static constexpr time_t kMinValidEpoch = 1704067200;

 public:
  // Call after BoardConfig has selected the active device.
  void begin();

  // True if an RTC is present on this device
  bool isAvailable() const { return _available; }

  // True on devices without a hardware RTC (X4). Their software clock is backed
  // by the ESP32's internal RC oscillator, which drifts minutes per day, so it
  // must be re-synced from NTP on every (re)connect rather than just once.
  // Always false on X3 (DS3231-class RTC, ~2 ppm, one sync is enough).
  bool needsPeriodicNTPSync() const { return !_available; }

  // Get current hour (0-23) and minute (0-59).
  // Returns false if RTC is not available.
  bool getTime(uint8_t& hour, uint8_t& minute) const;

  // Full date and time (UTC). Prefers the hardware RTC; on devices without
  // one (X4) falls back to the ESP32 system clock, which is set by NTP sync
  // and survives light sleep. Returns false when neither source has a
  // plausible time (never synced).
  bool getDateTime(uint16_t& year, uint8_t& month, uint8_t& day, uint8_t& hour, uint8_t& minute) const;

  // True when either the hardware RTC or the system clock carries a plausible
  // time (year >= 2020).
  bool hasUsableTime() const;

  // Format time into a caller-provided buffer.
  // 24h mode produces "HH:MM" (needs >=6 bytes); 12h mode produces "H:MM AM"/"HH:MM PM" (needs >=9 bytes).
  // utcOffsetQuarterHoursBiased: biased quarter-hour offset (48 = UTC+0, 0 = UTC-12, 104 = UTC+14).
  // use12Hour: when true, format as 12-hour clock with AM/PM suffix.
  // Returns false if RTC is not available.
  bool formatTime(char* buf, size_t bufSize, uint8_t utcOffsetQuarterHoursBiased = 48, bool use12Hour = false) const;

  // Sync the clock from an NTP server. Requires WiFi to be connected.
  // Blocks for up to ~5s while waiting for SNTP response.
  // Works on devices without a hardware RTC too: the ESP32 system clock is
  // set as a software clock (no battery cost — it is just a counter).
  // Returns true if a usable time was obtained.
  //
  // Debouncing (skip if already synced once) is enforced by the caller, not here,
  // so the HAL stays free of any app-layer settings dependency.
  bool syncFromNTP();

  // Seed the software clock (X4 only) from a UTC epoch persisted from a previous
  // successful NTP sync (the caller owns that persistence). Intended as a
  // best-effort fallback so date/time-dependent UI (e.g. the clock / sleep screen)
  // has *something* to show on boots where no WiFi network is reachable to get a
  // fresh time, rather than showing nothing at all. No-op on X3 (real battery-
  // backed RTC) and when a real time value (this boot's own NTP sync) is already
  // available or the persisted epoch is invalid. Returns true if applied.
  bool seedFallbackTime(time_t epochUtc);

  // True if getTime()/getDateTime() are currently returning a value applied by
  // seedFallbackTime() rather than one confirmed by a hardware RTC read or this
  // boot's own NTP sync. Always false on X3. Callers that want to flag displayed
  // time/dates as approximate can check this.
  bool isUsingFallbackTime() const { return _usingFallbackTime; }

  // Format date into a caller-provided buffer as "Mon D, YYYY" (e.g. "Aug 31, 2026").
  // utcOffsetQuarterHoursBiased matches formatTime so the date rolls over at local
  // midnight. Returns false if no usable time is available.
  bool formatDate(char* buf, size_t bufSize, uint8_t utcOffsetQuarterHoursBiased = 48) const;
};
