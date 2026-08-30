#pragma once

#include <cstddef>
#include <cstdint>

// Date/time primitives and pure calendar math for the reading statistics module.
// Days are indexed from 2000-01-01 (index 0) so they fit in a uint32_t and can
// be stored/compared without strings.

constexpr size_t READING_TIME_BUCKET_COUNT = 4;  // morning / afternoon / evening / night
constexpr size_t READING_DAY_OF_WEEK_COUNT = 7;  // Monday = 0
constexpr size_t READING_DAY_HISTORY_DAYS = 90;  // per-day seconds ring length
constexpr uint16_t READING_DAY_SECONDS_MAX = 65535;

enum class ReadingTimeBucket : uint8_t { Morning = 0, Afternoon, Evening, Night };

struct ReadingStatsDate {
  uint16_t year = 0;
  uint8_t month = 0;  // 1-12
  uint8_t day = 0;    // 1-31

  bool isValid() const;
  void clear();
};

struct ReadingStatsDateTime {
  ReadingStatsDate date;
  uint8_t hour = 0;
  uint8_t minute = 0;
  uint8_t second = 0;

  bool isValid() const { return date.isValid(); }
};

bool isLeapYear(uint16_t year);
uint8_t daysInMonth(uint16_t year, uint8_t month);
uint32_t readingStatsDayIndex(const ReadingStatsDate& date);  // days since 2000-01-01
bool readingStatsDateFromDayIndex(uint32_t dayIndex, ReadingStatsDate& outDate);
uint8_t readingStatsDayOfWeekIndex(const ReadingStatsDate& date);  // Monday = 0
ReadingTimeBucket readingTimeBucketForHour(uint8_t hour);

// Advances dt by seconds, rolling across bucket/day boundaries as needed.
void addSecondsToReadingStatsDateTime(ReadingStatsDateTime& dt, uint32_t seconds);

// Splits a reading span into the four time-of-day buckets and the weekday
// buckets. Handles spans crossing midnight/bucket boundaries.
void recordReadingSpanIntoBuckets(uint32_t* timeOfDaySeconds, uint32_t* dayOfWeekSeconds,
                                  const ReadingStatsDateTime& localStart, uint32_t seconds);

// Current local wall-clock time derived from the firmware clock (RTC on X3,
// system time on X4) shifted by the user's UTC offset setting. Returns false
// when the device has no usable time (never synced, X4 offline).
bool getCurrentLocalReadingStatsDateTime(ReadingStatsDateTime& outDateTime);
