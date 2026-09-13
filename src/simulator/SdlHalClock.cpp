#include "SdlHalClock.h"
#include <cstdio>
#include <cstring>

HalClock halClock;

bool HalClock::getTime(uint8_t& hour, uint8_t& minute) const {
  std::time_t t = std::time(nullptr);
  std::tm* local = std::localtime(&t);
  if (!local) return false;
  hour = static_cast<uint8_t>(local->tm_hour);
  minute = static_cast<uint8_t>(local->tm_min);
  return true;
}

bool HalClock::getDate(uint16_t& year, uint8_t& month, uint8_t& day, uint8_t& hour, uint8_t& minute) const {
  std::time_t t = std::time(nullptr);
  std::tm* local = std::localtime(&t);
  if (!local) return false;
  year = static_cast<uint16_t>(local->tm_year + 1900);
  month = static_cast<uint8_t>(local->tm_mon + 1);
  day = static_cast<uint8_t>(local->tm_mday);
  hour = static_cast<uint8_t>(local->tm_hour);
  minute = static_cast<uint8_t>(local->tm_min);
  return true;
}

bool HalClock::formatTime(char* buf, size_t bufSize, uint8_t utcOffsetQuarterHoursBiased, bool use12Hour) const {
  if (!buf || bufSize < 6) return false;
  uint8_t h = 0, m = 0;
  if (!getTime(h, m)) return false;

  // Biased quarter hour offset (48 = UTC+0)
  int offsetMinutes = (static_cast<int>(utcOffsetQuarterHoursBiased) - 48) * 15;
  int totalMinutes = h * 60 + m + offsetMinutes;
  while (totalMinutes < 0) totalMinutes += 24 * 60;
  totalMinutes %= (24 * 60);

  int dispH = totalMinutes / 60;
  int dispM = totalMinutes % 60;

  if (use12Hour) {
    bool isPm = (dispH >= 12);
    int h12 = dispH % 12;
    if (h12 == 0) h12 = 12;
    snprintf(buf, bufSize, "%d:%02d %s", h12, dispM, isPm ? "PM" : "AM");
  } else {
    snprintf(buf, bufSize, "%02d:%02d", dispH, dispM);
  }
  return true;
}

bool HalClock::formatDate(char* buf, size_t bufSize, uint8_t utcOffsetQuarterHoursBiased) const {
  if (!buf || bufSize < 11) return false;
  uint16_t y = 0;
  uint8_t mo = 0, d = 0, h = 0, mi = 0;
  if (!getDate(y, mo, d, h, mi)) return false;
  snprintf(buf, bufSize, "%04d-%02d-%02d", y, mo, d);
  return true;
}
