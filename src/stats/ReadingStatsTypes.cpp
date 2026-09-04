#include "ReadingStatsTypes.h"

#include <HalClock.h>

#include "CrossPointSettings.h"

bool ReadingStatsDate::isValid() const {
  if (year < 2000 || year > 2099) return false;
  const uint8_t monthDays = daysInMonth(year, month);
  return monthDays > 0 && day >= 1 && day <= monthDays;
}

void ReadingStatsDate::clear() {
  year = 0;
  month = 0;
  day = 0;
}

bool isLeapYear(const uint16_t year) { return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0; }

uint8_t daysInMonth(const uint16_t year, const uint8_t month) {
  static constexpr uint8_t DAYS[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (month < 1 || month > 12) return 0;
  if (month == 2 && isLeapYear(year)) return 29;
  return DAYS[month - 1];
}

uint32_t readingStatsDayIndex(const ReadingStatsDate& date) {
  uint32_t dayIndex = 0;
  for (uint16_t year = 2000; year < date.year; ++year) {
    dayIndex += isLeapYear(year) ? 366u : 365u;
  }
  for (uint8_t month = 1; month < date.month; ++month) {
    dayIndex += daysInMonth(date.year, month);
  }
  dayIndex += static_cast<uint32_t>(date.day - 1);
  return dayIndex;
}

bool readingStatsDateFromDayIndex(uint32_t dayIndex, ReadingStatsDate& outDate) {
  outDate = {};
  uint16_t year = 2000;
  while (year <= 2099) {
    const uint32_t yearDays = isLeapYear(year) ? 366u : 365u;
    if (dayIndex < yearDays) break;
    dayIndex -= yearDays;
    year++;
  }
  if (year > 2099) return false;

  uint8_t month = 1;
  while (month <= 12) {
    const uint8_t monthDays = daysInMonth(year, month);
    if (dayIndex < monthDays) break;
    dayIndex -= monthDays;
    month++;
  }
  if (month > 12) return false;

  outDate.year = year;
  outDate.month = month;
  outDate.day = static_cast<uint8_t>(dayIndex + 1u);
  return true;
}

uint8_t readingStatsDayOfWeekIndex(const ReadingStatsDate& date) {
  // 2000-01-01 was a Saturday; shift so Monday = 0.
  return static_cast<uint8_t>((5u + readingStatsDayIndex(date)) % 7u);
}

ReadingTimeBucket readingTimeBucketForHour(const uint8_t hour) {
  if (hour >= 5 && hour < 12) return ReadingTimeBucket::Morning;
  if (hour >= 12 && hour < 17) return ReadingTimeBucket::Afternoon;
  if (hour >= 17 && hour < 21) return ReadingTimeBucket::Evening;
  return ReadingTimeBucket::Night;
}

uint16_t readingSpanDaysElapsed(const ReadingStatsDate& start, const ReadingStatsDate& end) {
  if (!start.isValid() || !end.isValid()) return 0;
  const uint32_t startDay = readingStatsDayIndex(start);
  const uint32_t endDay = readingStatsDayIndex(end);
  if (endDay < startDay) return 0;
  const uint32_t elapsed = endDay - startDay;
  return elapsed > UINT16_MAX ? UINT16_MAX : static_cast<uint16_t>(elapsed);
}

void addSecondsToReadingStatsDateTime(ReadingStatsDateTime& dt, const uint32_t seconds) {
  if (!dt.isValid() || seconds == 0) return;

  uint32_t secondOfDay =
      static_cast<uint32_t>(dt.hour) * 3600u + static_cast<uint32_t>(dt.minute) * 60u + dt.second + seconds;
  const uint32_t daysToAdvance = secondOfDay / (24u * 3600u);
  secondOfDay %= (24u * 3600u);

  if (daysToAdvance > 0) {
    // Advance the date by daysToAdvance days.
    uint32_t remaining = daysToAdvance;
    while (remaining > 0) {
      const uint8_t monthDays = daysInMonth(dt.date.year, dt.date.month);
      if (dt.date.day < monthDays) {
        dt.date.day++;
      } else {
        dt.date.day = 1;
        if (dt.date.month < 12) {
          dt.date.month++;
        } else {
          dt.date.month = 1;
          dt.date.year++;
        }
      }
      remaining--;
    }
  }

  dt.hour = static_cast<uint8_t>(secondOfDay / 3600u);
  secondOfDay %= 3600u;
  dt.minute = static_cast<uint8_t>(secondOfDay / 60u);
  dt.second = static_cast<uint8_t>(secondOfDay % 60u);
}

namespace {
uint32_t secondsUntilNextBucketBoundary(const ReadingStatsDateTime& dt) {
  const uint32_t currentSecondOfDay =
      static_cast<uint32_t>(dt.hour) * 3600u + static_cast<uint32_t>(dt.minute) * 60u + dt.second;
  uint32_t nextBoundary = 24u * 3600u;
  if (dt.hour < 5) {
    nextBoundary = 5u * 3600u;
  } else if (dt.hour < 12) {
    nextBoundary = 12u * 3600u;
  } else if (dt.hour < 17) {
    nextBoundary = 17u * 3600u;
  } else if (dt.hour < 21) {
    nextBoundary = 21u * 3600u;
  }
  return nextBoundary > currentSecondOfDay ? nextBoundary - currentSecondOfDay : 1;
}
}  // namespace

void recordReadingSpanIntoBuckets(uint32_t* timeOfDaySeconds, uint32_t* dayOfWeekSeconds,
                                  const ReadingStatsDateTime& localStart, const uint32_t seconds) {
  if (!localStart.isValid() || seconds == 0) return;

  ReadingStatsDateTime cursor = localStart;
  uint32_t remaining = seconds;
  while (remaining > 0) {
    const uint8_t bucketIndex = static_cast<uint8_t>(readingTimeBucketForHour(cursor.hour));
    const uint8_t weekdayIndex = readingStatsDayOfWeekIndex(cursor.date);
    const uint32_t segment = remaining < secondsUntilNextBucketBoundary(cursor) ? remaining
                                                                                : secondsUntilNextBucketBoundary(cursor);
    timeOfDaySeconds[bucketIndex] += segment;
    dayOfWeekSeconds[weekdayIndex] += segment;
    remaining -= segment;
    addSecondsToReadingStatsDateTime(cursor, segment);
  }
}

bool getCurrentLocalReadingStatsDateTime(ReadingStatsDateTime& outDateTime) {
  uint16_t year = 0;
  uint8_t month = 0;
  uint8_t day = 0;
  uint8_t hour = 0;
  uint8_t minute = 0;
  if (!halClock.getDateTime(year, month, day, hour, minute)) {
    outDateTime = {};
    return false;
  }

  outDateTime.date = {year, month, day};
  outDateTime.hour = hour;
  outDateTime.minute = minute;
  outDateTime.second = 0;
  if (!outDateTime.isValid()) {
    outDateTime = {};
    return false;
  }

  // The clock reports UTC; shift into the user's local time zone.
  const int offsetQuarterHours = static_cast<int>(SETTINGS.clockUtcOffsetQ) - 48;
  int totalMinutes = static_cast<int>(outDateTime.hour) * 60 + static_cast<int>(outDateTime.minute) +
                     offsetQuarterHours * 15;
  while (totalMinutes < 0) {
    // Step back one day.
    if (outDateTime.date.day > 1) {
      outDateTime.date.day--;
    } else if (outDateTime.date.month > 1) {
      outDateTime.date.month--;
      outDateTime.date.day = daysInMonth(outDateTime.date.year, outDateTime.date.month);
    } else {
      outDateTime.date.month = 12;
      outDateTime.date.day = 31;
      outDateTime.date.year--;
    }
    totalMinutes += 24 * 60;
  }
  while (totalMinutes >= 24 * 60) {
    const uint8_t monthDays = daysInMonth(outDateTime.date.year, outDateTime.date.month);
    if (outDateTime.date.day < monthDays) {
      outDateTime.date.day++;
    } else if (outDateTime.date.month < 12) {
      outDateTime.date.month++;
      outDateTime.date.day = 1;
    } else {
      outDateTime.date.month = 1;
      outDateTime.date.day = 1;
      outDateTime.date.year++;
    }
    totalMinutes -= 24 * 60;
  }

  outDateTime.hour = static_cast<uint8_t>(totalMinutes / 60);
  outDateTime.minute = static_cast<uint8_t>(totalMinutes % 60);
  return outDateTime.isValid();
}
