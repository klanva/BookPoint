#include "BookReadingStats.h"

#include <I18n.h>

#include <cstdio>

void BookReadingStats::recordForwardPageRead(uint32_t seconds) {
  if (seconds == 0) return;
  if (seconds > UINT16_MAX) seconds = UINT16_MAX;

  const uint16_t sample = static_cast<uint16_t>(seconds);
  if (paceSampleCount == 0 || avgSecondsPerForwardPage == 0) {
    avgSecondsPerForwardPage = sample;
    paceSampleCount = 1;
    return;
  }

  // Incremental average with a capped window so a single long session cannot
  // dominate the pace forever.
  constexpr uint16_t MAX_PACE_SAMPLES = 1000;
  const uint16_t weight = paceSampleCount < MAX_PACE_SAMPLES ? paceSampleCount : MAX_PACE_SAMPLES;
  const uint32_t next =
      (static_cast<uint32_t>(avgSecondsPerForwardPage) * weight + sample) / (static_cast<uint32_t>(weight) + 1u);
  avgSecondsPerForwardPage = static_cast<uint16_t>(next);
  if (paceSampleCount < MAX_PACE_SAMPLES) paceSampleCount++;
}

void BookReadingStats::recordReadingSpan(const ReadingStatsDateTime& localStart, const uint32_t seconds) {
  recordReadingSpanIntoBuckets(timeOfDaySeconds, dayOfWeekSeconds, localStart, seconds);
}

void BookReadingStats::formatDuration(const uint32_t seconds, char* buf, const size_t len) {
  if (seconds == 0) {
    snprintf(buf, len, "0 %s", tr(STR_STATS_UNIT_MIN));
    return;
  }
  if (seconds < 60) {
    snprintf(buf, len, "< 1 %s", tr(STR_STATS_UNIT_MIN));
    return;
  }
  const uint32_t hours = seconds / 3600;
  const uint32_t minutes = (seconds % 3600) / 60;
  if (hours == 0) {
    snprintf(buf, len, "%lu %s", static_cast<unsigned long>(minutes), tr(STR_STATS_UNIT_MIN));
  } else {
    snprintf(buf, len, "%lu%s %lu %s", static_cast<unsigned long>(hours), tr(STR_STATS_UNIT_HOUR),
             static_cast<unsigned long>(minutes), tr(STR_STATS_UNIT_MIN));
  }
}
