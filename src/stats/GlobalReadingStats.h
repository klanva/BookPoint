#pragma once

#include <cstdint>

#include "ReadingStatsTypes.h"

// Device-wide cumulative reading statistics, persisted to
// /.crosspoint/stats/global.bin (atomic write + rotating .bak backup).
//
// In addition to lifetime totals, this keeps a rolling ring of real reading
// seconds per day (READING_DAY_HISTORY_DAYS back from anchorDayIndex) — not
// just a read/didn't-read flag — so the UI can draw an actual activity chart.
struct GlobalReadingStats {
  uint32_t totalSessions = 0;
  uint32_t totalReadingSeconds = 0;
  uint32_t totalPagesTurned = 0;
  uint32_t completedBooks = 0;
  uint16_t longestReadingStreak = 0;

  // Per-day seconds ring: daySeconds[i] is the reading time of
  // (anchorDayIndex - i) days ago. anchorDayIndex == 0 means the ring is empty.
  uint32_t anchorDayIndex = 0;
  uint16_t daySeconds[READING_DAY_HISTORY_DAYS] = {};

  uint32_t timeOfDaySeconds[READING_TIME_BUCKET_COUNT] = {};
  uint32_t dayOfWeekSeconds[READING_DAY_OF_WEEK_COUNT] = {};

  // Attribute a reading span to the buckets and the per-day ring, and grow
  // longestReadingStreak if the ring now shows a longer run of active days.
  void recordReadingSpan(const ReadingStatsDateTime& localStart, uint32_t seconds);

  // Consecutive active days ending today (or yesterday, if today has no time
  // yet — a morning reader shouldn't see the streak reset to zero at midnight).
  // Pass today's day index when known; 0 means "unknown clock" and skips the
  // freshness check.
  uint16_t currentReadingStreak(uint32_t todayDayIndex = 0) const;

  uint16_t displayLongestReadingStreak() const { return longestReadingStreak; }

  // Seconds read on the day `daysAgo` days before the anchor. Returns -1 when
  // that day falls outside the stored ring (or the ring is empty).
  int32_t secondsForDaysAgo(uint32_t daysAgo) const;
};
