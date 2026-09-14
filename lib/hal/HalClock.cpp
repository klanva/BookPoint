#include "HalClock.h"

#include <Logging.h>
#include <WiFi.h>
#include <esp_sntp.h>
#include <sys/time.h>
#include <time.h>

#include <cassert>

HalClock halClock;  // Singleton instance

namespace {
constexpr uint16_t kBaseYear = 2000;
// Software clock reads are considered "unsynced" below this epoch value
// (2024-01-01 00:00:00 UTC) — filters out the boot default before sync.
constexpr time_t kMinValidEpoch = 1704067200;
constexpr const char* kMonthNames[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                       "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

bool isLeapYear(const uint16_t year) { return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0; }

uint8_t daysInMonth(const uint16_t year, const uint8_t month) {
  static const uint8_t days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (month < 1 || month > 12) return 0;
  if (month == 2 && isLeapYear(year)) return 29;
  return days[month - 1];
}

bool isValidDate(const uint16_t year, const uint8_t month, const uint8_t day) {
  if (year < kBaseYear || year > 2099) return false;
  const uint8_t monthDays = daysInMonth(year, month);
  return monthDays > 0 && day >= 1 && day <= monthDays;
}

void adjustDateByDays(uint16_t& year, uint8_t& month, uint8_t& day, const int dayDelta) {
  if (dayDelta > 0) {
    const uint8_t monthDays = daysInMonth(year, month);
    if (day < monthDays) {
      day++;
    } else {
      day = 1;
      if (month < 12) {
        month++;
      } else {
        month = 1;
        year++;
      }
    }
  } else if (dayDelta < 0) {
    if (day > 1) {
      day--;
    } else {
      if (month > 1) {
        month--;
      } else {
        month = 12;
        year--;
      }
      day = daysInMonth(year, month);
    }
  }
}

// Convert UTC calendar components to epoch seconds (POSIX timegm equivalent)
time_t calendarToEpochUtc(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second) {
  if (!isValidDate(year, month, day) || hour >= 24 || minute >= 60 || second >= 60) return 0;
  static const uint16_t kDaysBeforeMonth[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
  uint32_t days = 0;
  for (uint16_t y = 1970; y < year; ++y) {
    days += isLeapYear(y) ? 366 : 365;
  }
  days += kDaysBeforeMonth[month - 1];
  if (month > 2 && isLeapYear(year)) {
    days += 1;
  }
  days += (day - 1);
  return static_cast<time_t>(days * 86400ULL + hour * 3600ULL + minute * 60ULL + second);
}

}  // namespace

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

extern "C" SemaphoreHandle_t getSharedI2cBusMutex();

namespace {
class ScopedI2CBusLock {
  bool _locked = false;
 public:
  ScopedI2CBusLock() {
    SemaphoreHandle_t m = getSharedI2cBusMutex();
    if (m != nullptr) {
      xSemaphoreTakeRecursive(m, portMAX_DELAY);
      _locked = true;
    }
  }
  ~ScopedI2CBusLock() {
    if (_locked) {
      SemaphoreHandle_t m = getSharedI2cBusMutex();
      if (m != nullptr) {
        xSemaphoreGiveRecursive(m);
      }
      _locked = false;
    }
  }
};
}  // namespace

void HalClock::begin() {
  const auto& sensors = BoardConfig::ACTIVE.sensors;
  const bool hasHwRtc = (sensors.rtcAddr != 0 && sensors.rtcType != BoardConfig::RtcType::None) || gpio.deviceIsX3();

  if (!hasHwRtc) {
    _useHardwareRtc = false;
    _available = true;
    LOG_INF("CLK", "No hardware RTC on this device profile - using software clock");
    return;
  }

  // Attempt to bring up the hardware RTC driver (BM8563/PCF8563 at 0x51 on X4 Pro)
  ScopedI2CBusLock i2cLock;
  if (_rtc.begin()) {
    _useHardwareRtc = true;
    _available = true;
    const uint8_t rtcAddr = sensors.rtcAddr != 0 ? sensors.rtcAddr : 0x51;
    LOG_INF("CLK", "Hardware RTC found and initialized at I2C 0x%02X", rtcAddr);

    // Read initial date/time from hardware RTC
    freeink::Rtc::DateTime dt;
    if (_rtc.now(dt) && isValidDate(dt.year, dt.month, dt.day)) {
      _cachedHour = dt.hour;
      _cachedMinute = dt.minute;
      _cachedYear = dt.year;
      _cachedMonth = dt.month;
      _cachedDay = dt.day;
      _hasCachedTime = true;
      _hasCachedDate = true;
      _lastPollMs = millis();
      _usingFallbackTime = false;

      // Immediately synchronize ESP32 system time from hardware RTC
      const time_t epochUtc = calendarToEpochUtc(dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);
      if (epochUtc >= kMinValidEpoch) {
        struct timeval tv {};
        tv.tv_sec = epochUtc;
        tv.tv_usec = 0;
        settimeofday(&tv, nullptr);
        LOG_INF("CLK", "System clock synchronized from hardware RTC: %04d-%02d-%02d %02d:%02d:%02d UTC",
                dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);
      }
      return;
    } else {
      LOG_INF("CLK", "Hardware RTC present at 0x%02X, but oscillator stopped or time uninitialized", rtcAddr);
      _usingFallbackTime = true;
      return;
    }
  }

  // Hardware RTC failed to ACK on the bus — fall back to software clock
  LOG_ERR("CLK", "Hardware RTC at 0x%02X did not ACK - falling back to software clock", sensors.rtcAddr);
  _useHardwareRtc = false;
  _available = true;
}

bool HalClock::getTime(uint8_t& hour, uint8_t& minute) const {
  if (!_available) return false;

  const unsigned long now = millis();

  if (_useHardwareRtc) {
    if (_lastPollMs != 0 && (now - _lastPollMs) < CLOCK_POLL_MS && _hasCachedTime) {
      hour = _cachedHour;
      minute = _cachedMinute;
      return true;
    }

    freeink::Rtc::DateTime dt;
    ScopedI2CBusLock i2cLock;
    if (const_cast<freeink::Rtc&>(_rtc).now(dt)) {
      _cachedHour = dt.hour;
      _cachedMinute = dt.minute;
      _cachedYear = dt.year;
      _cachedMonth = dt.month;
      _cachedDay = dt.day;
      _hasCachedTime = true;
      _hasCachedDate = isValidDate(dt.year, dt.month, dt.day);
      _lastPollMs = now;
      _usingFallbackTime = false;

      hour = _cachedHour;
      minute = _cachedMinute;
      return true;
    }

    if (_hasCachedTime) {
      _lastPollMs = now;
      hour = _cachedHour;
      minute = _cachedMinute;
      return true;
    }

    const time_t sysNow = time(nullptr);
    if (sysNow >= kMinValidEpoch) {
      struct tm timeinfo;
      gmtime_r(&sysNow, &timeinfo);
      hour = static_cast<uint8_t>(timeinfo.tm_hour);
      minute = static_cast<uint8_t>(timeinfo.tm_min);
      return true;
    }
    return false;
  }

  // Software clock mode (no hardware RTC)
  const time_t sysNow = time(nullptr);
  if (sysNow < kMinValidEpoch) return false;
  struct tm timeinfo;
  gmtime_r(&sysNow, &timeinfo);
  hour = static_cast<uint8_t>(timeinfo.tm_hour);
  minute = static_cast<uint8_t>(timeinfo.tm_min);
  return true;
}

bool HalClock::formatTime(char* buf, size_t bufSize, uint8_t utcOffsetQuarterHoursBiased, bool use12Hour) const {
  if (bufSize < (use12Hour ? 9u : 6u)) return false;
  uint8_t h, m;
  if (!getTime(h, m)) return false;

  // Apply UTC offset: convert biased value to signed quarter-hours.
  // Clamp against corrupted persisted values so display time can't drift outside [-12:00, +14:00].
  if (utcOffsetQuarterHoursBiased > 104) utcOffsetQuarterHoursBiased = 104;
  int offsetQuarterHours = static_cast<int>(utcOffsetQuarterHoursBiased) - 48;
  int totalMinutes = static_cast<int>(h) * 60 + static_cast<int>(m) + offsetQuarterHours * 15;

  // Wrap around 24 hours
  totalMinutes = ((totalMinutes % 1440) + 1440) % 1440;

  const int hour24 = totalMinutes / 60;
  const int min = totalMinutes % 60;
  if (use12Hour) {
    const bool pm = hour24 >= 12;
    int hour12 = hour24 % 12;
    if (hour12 == 0) hour12 = 12;
    snprintf(buf, bufSize, "%d:%02d %s", hour12, min, pm ? "PM" : "AM");
  } else {
    snprintf(buf, bufSize, "%02d:%02d", hour24, min);
  }
  return true;
}

bool HalClock::getDate(uint16_t& year, uint8_t& month, uint8_t& day, uint8_t& hour, uint8_t& minute) const {
  if (!_available) return false;

  const unsigned long now = millis();

  if (_useHardwareRtc) {
    if (_lastPollMs != 0 && (now - _lastPollMs) < CLOCK_POLL_MS && _hasCachedDate) {
      year = _cachedYear;
      month = _cachedMonth;
      day = _cachedDay;
      hour = _cachedHour;
      minute = _cachedMinute;
      return true;
    }

    freeink::Rtc::DateTime dt;
    ScopedI2CBusLock i2cLock;
    if (const_cast<freeink::Rtc&>(_rtc).now(dt) && isValidDate(dt.year, dt.month, dt.day)) {
      _cachedHour = dt.hour;
      _cachedMinute = dt.minute;
      _cachedYear = dt.year;
      _cachedMonth = dt.month;
      _cachedDay = dt.day;
      _hasCachedTime = true;
      _hasCachedDate = true;
      _lastPollMs = now;
      _usingFallbackTime = false;

      year = _cachedYear;
      month = _cachedMonth;
      day = _cachedDay;
      hour = _cachedHour;
      minute = _cachedMinute;
      return true;
    }

    if (_hasCachedDate) {
      _lastPollMs = now;
      year = _cachedYear;
      month = _cachedMonth;
      day = _cachedDay;
      hour = _cachedHour;
      minute = _cachedMinute;
      return true;
    }

    const time_t sysNow = time(nullptr);
    if (sysNow >= kMinValidEpoch) {
      struct tm timeinfo;
      gmtime_r(&sysNow, &timeinfo);
      year = static_cast<uint16_t>(timeinfo.tm_year + 1900);
      month = static_cast<uint8_t>(timeinfo.tm_mon + 1);
      day = static_cast<uint8_t>(timeinfo.tm_mday);
      hour = static_cast<uint8_t>(timeinfo.tm_hour);
      minute = static_cast<uint8_t>(timeinfo.tm_min);
      return isValidDate(year, month, day);
    }
    return false;
  }

  // Software clock mode
  const time_t sysNow = time(nullptr);
  if (sysNow < kMinValidEpoch) return false;
  struct tm timeinfo;
  gmtime_r(&sysNow, &timeinfo);
  year = static_cast<uint16_t>(timeinfo.tm_year + 1900);
  month = static_cast<uint8_t>(timeinfo.tm_mon + 1);
  day = static_cast<uint8_t>(timeinfo.tm_mday);
  hour = static_cast<uint8_t>(timeinfo.tm_hour);
  minute = static_cast<uint8_t>(timeinfo.tm_min);
  return isValidDate(year, month, day);
}

bool HalClock::formatDate(char* buf, size_t bufSize, uint8_t utcOffsetQuarterHoursBiased) const {
  if (bufSize < 13u) return false;

  uint16_t year;
  uint8_t month, day, hour, minute;
  if (!getDate(year, month, day, hour, minute)) return false;

  if (utcOffsetQuarterHoursBiased > 104) utcOffsetQuarterHoursBiased = 104;
  const int offsetQuarterHours = static_cast<int>(utcOffsetQuarterHoursBiased) - 48;
  const int localMinutes = static_cast<int>(hour) * 60 + static_cast<int>(minute) + offsetQuarterHours * 15;
  const int dayDelta = localMinutes < 0 ? -1 : (localMinutes >= 1440 ? 1 : 0);
  adjustDateByDays(year, month, day, dayDelta);
  if (!isValidDate(year, month, day)) return false;

  snprintf(buf, bufSize, "%s %u, %u", kMonthNames[month - 1], static_cast<unsigned int>(day),
           static_cast<unsigned int>(year));
  return true;
}

bool HalClock::writeDateTimeToRTC(uint16_t year, uint8_t month, uint8_t day, uint8_t weekday, uint8_t hour,
                                  uint8_t minute, uint8_t second) {
  assert(hour < 24);
  assert(minute < 60);
  assert(second < 60);
  assert(isValidDate(year, month, day));

  if (!_useHardwareRtc) return false;

  ScopedI2CBusLock i2cLock;
  freeink::Rtc::DateTime dt;
  dt.year = year;
  dt.month = month;
  dt.day = day;
  dt.weekday = weekday % 7;
  dt.hour = hour;
  dt.minute = minute;
  dt.second = second;

  if (!_rtc.set(dt)) {
    LOG_ERR("CLK", "Failed to write date/time to hardware RTC");
    return false;
  }

  // Synchronize ESP32 internal system clock to match
  const time_t epochUtc = calendarToEpochUtc(year, month, day, hour, minute, second);
  if (epochUtc >= kMinValidEpoch) {
    struct timeval tv {};
    tv.tv_sec = epochUtc;
    tv.tv_usec = 0;
    settimeofday(&tv, nullptr);
  }

  _lastPollMs = millis();
  _cachedHour = hour;
  _cachedMinute = minute;
  _cachedYear = year;
  _cachedMonth = month;
  _cachedDay = day;
  _hasCachedTime = true;
  _hasCachedDate = true;
  _usingFallbackTime = false;
  return true;
}

bool HalClock::setDateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second) {
  if (!isValidDate(year, month, day) || hour >= 24 || minute >= 60 || second >= 60) return false;

  // Sakamoto's algorithm: calculate day of week (0=Sunday .. 6=Saturday)
  static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  int y = year - (month < 3);
  uint8_t weekday = static_cast<uint8_t>((y + y/4 - y/100 + y/400 + t[month-1] + day) % 7);

  if (_useHardwareRtc) {
    return writeDateTimeToRTC(year, month, day, weekday, hour, minute, second);
  }

  const time_t epochUtc = calendarToEpochUtc(year, month, day, hour, minute, second);
  if (epochUtc >= kMinValidEpoch) {
    struct timeval tv {};
    tv.tv_sec = epochUtc;
    tv.tv_usec = 0;
    settimeofday(&tv, nullptr);
    _lastPollMs = millis();
    _cachedHour = hour;
    _cachedMinute = minute;
    _cachedYear = year;
    _cachedMonth = month;
    _cachedDay = day;
    _hasCachedTime = true;
    _hasCachedDate = true;
    _usingFallbackTime = false;
    return true;
  }
  return false;
}

bool HalClock::seedFallbackTime(time_t epochUtc) {
  if (_useHardwareRtc && !_usingFallbackTime && _hasCachedTime) {
    return false;  // Physical RTC already has running, reliable time
  }
  if (epochUtc < kMinValidEpoch) return false;

  const time_t now = time(nullptr);
  if (now >= kMinValidEpoch && !_usingFallbackTime) return false;

  struct timeval tv {};
  tv.tv_sec = epochUtc;
  tv.tv_usec = 0;
  settimeofday(&tv, nullptr);
  _usingFallbackTime = true;
  LOG_INF("CLK", "Seeded clock from persisted fallback time");
  return true;
}

bool HalClock::syncFromNTP() {
  if (!_available) return false;

  if (WiFi.status() != WL_CONNECTED) {
    LOG_ERR("CLK", "WiFi not connected, cannot sync NTP");
    return false;
  }

  LOG_INF("CLK", "Starting NTP sync...");
  configTzTime("UTC0", "pool.ntp.org", "time.nist.gov");

  // Wait for SNTP sync to complete (up to 5 seconds)
  constexpr int maxAttempts = 50;
  for (int i = 0; i < maxAttempts; i++) {
    if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
      time_t now = time(nullptr);
      struct tm timeinfo;
      gmtime_r(&now, &timeinfo);

      const uint16_t year = static_cast<uint16_t>(timeinfo.tm_year + 1900);
      const uint8_t month = static_cast<uint8_t>(timeinfo.tm_mon + 1);
      const uint8_t day = static_cast<uint8_t>(timeinfo.tm_mday);
      const uint8_t weekday = static_cast<uint8_t>(timeinfo.tm_wday);

      if (_useHardwareRtc) {
        if (writeDateTimeToRTC(year, month, day, weekday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec)) {
          LOG_INF("CLK", "Hardware RTC synchronized to %04d-%02d-%02d %02d:%02d:%02d UTC", year, month, day,
                  timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
          _usingFallbackTime = false;
          return true;
        }
        LOG_ERR("CLK", "Failed to write NTP time to hardware RTC");
        return false;
      } else {
        _usingFallbackTime = false;
        LOG_INF("CLK", "System clock set to %04d-%02d-%02d %02d:%02d:%02d UTC (software clock)", year, month, day,
                timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        return true;
      }
    }
    delay(100);
  }

  LOG_ERR("CLK", "NTP sync timed out");
  return false;
}

