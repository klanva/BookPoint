#pragma once

#include "BookReadingStats.h"
#include "ReadingStatsTypes.h"

// Reading statistics helpers ported from inkMOD's ReadingStatsUtils /
// BookStatsView. Kept as free functions so both the on-device statistics
// screen and the home screen can share the exact same finish-date estimate.

// Time-left estimate from reading time already spent scaled by the fraction
// of the book still unread (inkMOD's time-based fallback when no page pace
// exists). Returns false for a book that is barely started or already done.
bool fallbackEstimatedTimeLeft(const BookReadingStats& stats, float progressPercent, uint32_t& seconds);

// Projects the book's remaining reading time onto the calendar using the
// book's average reading seconds per calendar day (totalReadingSeconds spread
// over the elapsed days since startDate). Writes the estimated local date into
// outDate and returns false when no sensible estimate exists (no clock, no
// start date, no remaining time, or an already-finished book).
bool estimateFinishDateFromDailyPace(const BookReadingStats& stats, const ReadingStatsDateTime& today,
                                     uint32_t estimatedReadingSeconds, ReadingStatsDate& outDate);

// Formats a ReadingStatsDate as "dd.mm.yyyy" into buf. Writes "-" when the
// date is invalid.
void formatFinishDate(const ReadingStatsDate& date, char* buf, size_t len);
