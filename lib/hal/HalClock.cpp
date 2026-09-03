#include "HalClock.h"

#include <Logging.h>
#include <WiFi.h>
#include <esp_sntp.h>
#include <sys/time.h>
#include <time.h>

HalClock halClock;  // Singleton instance

void HalClock::begin() {
  _available = _sdkRtc.begin();
  LOG_INF("CLK", _available ? "SDK RTC found" : "RTC not found");
}

bool HalClock::getTime(uint8_t& hour, uint8_t& minute) const {
  if (!_available) {
    // No hardware RTC: read the system clock set by NTP sync (software
    // clock). Same counter getDateTime() uses, zero power cost.
    const time_t now = time(nullptr);
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    if (now < kMinValidEpoch) return false;
    hour = static_cast<uint8_t>(timeinfo.tm_hour);
    minute = static_cast<uint8_t>(timeinfo.tm_min);
    return true;
  }

  const unsigned long now = millis();
  if (_lastPollMs != 0 && (now - _lastPollMs) < CLOCK_POLL_MS) {
    hour = _cachedHour;
    minute = _cachedMinute;
    return true;
  }

  Rtc::DateTime dt;
  if (!_sdkRtc.now(dt)) {
    if (!_hasCachedTime) return false;
    _lastPollMs = now;
    hour = _cachedHour;
    minute = _cachedMinute;
    return true;
  }
  _cachedHour = dt.hour;
  _cachedMinute = dt.minute;
  _lastPollMs = now;
  _hasCachedTime = true;
  hour = _cachedHour;
  minute = _cachedMinute;
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

bool HalClock::getDateTime(uint16_t& year, uint8_t& month, uint8_t& day, uint8_t& hour, uint8_t& minute) const {
  if (_available) {
    Rtc::DateTime dt;
    if (_sdkRtc.now(dt)) {
      year = dt.year;
      month = dt.month;
      day = dt.day;
      hour = dt.hour;
      minute = dt.minute;
      return dt.year >= 2020;
    }
  }

  // Software clock fallback (X4): the ESP32 system time, set by NTP sync and
  // restored from the persisted last-known epoch at boot. Costs nothing to
  // read — it is a plain counter, no radio, no timers.
  const time_t now = time(nullptr);
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  if (now < kMinValidEpoch) return false;

  year = static_cast<uint16_t>(timeinfo.tm_year + 1900);
  month = static_cast<uint8_t>(timeinfo.tm_mon + 1);
  day = static_cast<uint8_t>(timeinfo.tm_mday);
  hour = static_cast<uint8_t>(timeinfo.tm_hour);
  minute = static_cast<uint8_t>(timeinfo.tm_min);
  return true;
}

bool HalClock::hasUsableTime() const {
  uint16_t year;
  uint8_t month, day, hour, minute;
  return getDateTime(year, month, day, hour, minute);
}

bool HalClock::syncFromNTP() {
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

      if (!_available) {
        // No hardware RTC (X4): the SNTP update already set the system
        // clock, which getDateTime() reads back. Nothing else to do — the
        // software clock costs no power and survives light sleep.
        _usingFallbackTime = false;
        LOG_INF("CLK", "System clock set to %04u-%02u-%02u %02u:%02u UTC", year, timeinfo.tm_mon + 1,
                timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min);
        return year >= 2020;
      }

      Rtc::DateTime dt;
      dt.year = year;
      dt.month = static_cast<uint8_t>(timeinfo.tm_mon + 1);
      dt.day = static_cast<uint8_t>(timeinfo.tm_mday);
      dt.hour = static_cast<uint8_t>(timeinfo.tm_hour);
      dt.minute = static_cast<uint8_t>(timeinfo.tm_min);
      dt.second = static_cast<uint8_t>(timeinfo.tm_sec);
      dt.weekday = static_cast<uint8_t>(timeinfo.tm_wday);
      if (_sdkRtc.set(dt)) {
        _lastPollMs = 0;
        _cachedHour = dt.hour;
        _cachedMinute = dt.minute;
        _hasCachedTime = true;
        LOG_INF("CLK", "RTC set to %04u-%02u-%02u %02u:%02u:%02u UTC", dt.year, dt.month, dt.day, dt.hour, dt.minute,
                dt.second);
        return true;
      }
      return false;
    }
    delay(100);
  }

  LOG_ERR("CLK", "NTP sync timed out");
  return false;
}

namespace {

bool isLeapYear(const uint16_t year) { return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0; }

uint8_t daysInMonth(const uint16_t year, const uint8_t month) {
  static const uint8_t days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (month < 1 || month > 12) return 0;
  if (month == 2 && isLeapYear(year)) return 29;
  return days[month - 1];
}

void adjustDateByDays(uint16_t& year, uint8_t& month, uint8_t& day, const int dayDelta) {
  if (dayDelta > 0) {
    if (day < daysInMonth(year, month)) {
      day++;
    } else {
      day = 1;
      if (month < 12)
        month++;
      else {
        month = 1;
        year++;
      }
    }
  } else if (dayDelta < 0) {
    if (day > 1) {
      day--;
    } else {
      if (month > 1)
        month--;
      else {
        month = 12;
        year--;
      }
      day = daysInMonth(year, month);
    }
  }
}

const char* kMonthNames[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

}  // namespace

bool HalClock::seedFallbackTime(time_t epochUtc) {
  if (_available) return false;  // X3 has a real battery-backed RTC; never needed.
  if (epochUtc < kMinValidEpoch) return false;  // guard against a corrupt/zero persisted value.

  const time_t now = time(nullptr);
  if (now >= kMinValidEpoch) return false;  // already have a real value this boot - don't clobber it.

  struct timeval tv {};
  tv.tv_sec = epochUtc;
  tv.tv_usec = 0;
  settimeofday(&tv, nullptr);
  _usingFallbackTime = true;
  LOG_INF("CLK", "No NTP sync yet this boot - seeded software clock from last known synced time");
  return true;
}

bool HalClock::formatDate(char* buf, size_t bufSize, uint8_t utcOffsetQuarterHoursBiased) const {
  if (bufSize < 13u) return false;

  uint16_t year;
  uint8_t month, day, hour, minute;
  if (!getDateTime(year, month, day, hour, minute)) return false;

  if (utcOffsetQuarterHoursBiased > 104) utcOffsetQuarterHoursBiased = 104;
  const int offsetQuarterHours = static_cast<int>(utcOffsetQuarterHoursBiased) - 48;
  const int localMinutes = static_cast<int>(hour) * 60 + static_cast<int>(minute) + offsetQuarterHours * 15;
  // Roll the calendar date across local midnight so "Mon D, YYYY" shows the local day.
  const int dayDelta = localMinutes < 0 ? -1 : (localMinutes >= 1440 ? 1 : 0);
  adjustDateByDays(year, month, day, dayDelta);
  if (month < 1 || month > 12) return false;

  snprintf(buf, bufSize, "%s %u, %u", kMonthNames[month - 1], static_cast<unsigned int>(day),
           static_cast<unsigned int>(year));
  return true;
}
