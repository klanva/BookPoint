#include "GlobalReadingStats.h"

void GlobalReadingStats::recordReadingSpan(const ReadingStatsDateTime& localStart, const uint32_t seconds) {
  if (!localStart.isValid() || seconds == 0) return;

  recordReadingSpanIntoBuckets(timeOfDaySeconds, dayOfWeekSeconds, localStart, seconds);

  // Fold the span into the per-day ring, splitting at midnight.
  ReadingStatsDateTime cursor = localStart;
  uint32_t remaining = seconds;
  while (remaining > 0) {
    const uint32_t dayIndex = readingStatsDayIndex(cursor.date);

    if (anchorDayIndex == 0) {
      anchorDayIndex = dayIndex;
    } else if (dayIndex > anchorDayIndex) {
      // Shift the ring so the newest day is always at index 0. Days that fall
      // off the end of the ring are gone — lifetime totals keep their seconds.
      const uint32_t shift = dayIndex - anchorDayIndex;
      if (shift >= READING_DAY_HISTORY_DAYS) {
        for (size_t i = 0; i < READING_DAY_HISTORY_DAYS; ++i) daySeconds[i] = 0;
      } else {
        for (size_t i = READING_DAY_HISTORY_DAYS - 1; i >= shift; --i) {
          daySeconds[i] = daySeconds[i - shift];
        }
        for (size_t i = 0; i < shift; ++i) daySeconds[i] = 0;
      }
      anchorDayIndex = dayIndex;
    }

    const uint32_t secondsUntilMidnight =
        (24u * 3600u) -
        (static_cast<uint32_t>(cursor.hour) * 3600u + static_cast<uint32_t>(cursor.minute) * 60u + cursor.second);
    const uint32_t segment = remaining < secondsUntilMidnight ? remaining : secondsUntilMidnight;

    const uint32_t ringOffset = anchorDayIndex - dayIndex;
    if (ringOffset < READING_DAY_HISTORY_DAYS) {
      const uint32_t accumulated = static_cast<uint32_t>(daySeconds[ringOffset]) + segment;
      daySeconds[ringOffset] = static_cast<uint16_t>(accumulated > READING_DAY_SECONDS_MAX ? READING_DAY_SECONDS_MAX
                                                                                          : accumulated);
    }

    remaining -= segment;
    addSecondsToReadingStatsDateTime(cursor, segment);
  }

  const uint16_t streak = currentReadingStreak();
  if (streak > longestReadingStreak) longestReadingStreak = streak;
}

uint16_t GlobalReadingStats::currentReadingStreak(const uint32_t todayDayIndex) const {
  if (anchorDayIndex == 0) return 0;
  if (daySeconds[0] == 0) return 0;  // ring exists but newest stored day has no time

  // If the newest stored day is not today and not yesterday, the streak is
  // already broken (or the clock was unset for a while — same result).
  if (todayDayIndex != 0 && anchorDayIndex + 1u < todayDayIndex) return 0;

  uint16_t streak = 0;
  while (streak < READING_DAY_HISTORY_DAYS && daySeconds[streak] > 0) streak++;
  return streak;
}

int32_t GlobalReadingStats::secondsForDaysAgo(const uint32_t daysAgo) const {
  if (anchorDayIndex == 0) return -1;
  // Days older than the anchor itself (future days, clock went backwards).
  if (daysAgo >= READING_DAY_HISTORY_DAYS) return -1;
  if (daysAgo > anchorDayIndex) return -1;
  return daySeconds[daysAgo];
}
