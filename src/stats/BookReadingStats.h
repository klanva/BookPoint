#pragma once

#include <cstdint>
#include <string>

#include "ReadingStatsTypes.h"

// Per-book reading statistics. Stored centrally on the SD card under
// /.crosspoint/stats/books/<hash>.bin — deliberately outside the per-book
// epub_* cache directories, so clearing the reading cache never touches
// accumulated statistics.
//
// The container embeds the book path, title, and author so exports stay
// meaningful even after the book file itself is deleted.
struct BookReadingStats {
  uint16_t sessionCount = 0;              // Times this book was opened (sessions >= 60 s)
  uint32_t totalReadingSeconds = 0;       // Accumulated active reading time
  uint32_t totalPagesTurned = 0;          // Forward page turns after the dwell threshold
  uint16_t avgSecondsPerForwardPage = 0;  // Running average pace (for time-left estimates)
  uint16_t paceSampleCount = 0;           // Number of pace samples folded into the average
  bool isFinished = false;                // End of book reached, or manually marked
  bool completedCounted = false;          // isFinished already folded into the global counter
  bool startDateManual = false;           // User override for the start date
  bool finishedDateManual = false;        // User override for the finish date
  ReadingStatsDate startDate;             // First qualifying reading date
  ReadingStatsDate finishedDate;          // Auto or manual finish date
  uint32_t lastReadEpoch = 0;             // UTC epoch of the last session end (0 = unknown)
  uint32_t timeOfDaySeconds[READING_TIME_BUCKET_COUNT] = {};
  uint32_t dayOfWeekSeconds[READING_DAY_OF_WEEK_COUNT] = {};

  // Container metadata (not part of the numeric payload).
  std::string bookPath;
  std::string bookTitle;
  std::string bookAuthor;

  // Fold one forward-page dwell sample into the running pace average.
  void recordForwardPageRead(uint32_t seconds);

  // Attribute reading time to the time-of-day / weekday buckets.
  void recordReadingSpan(const ReadingStatsDateTime& localStart, uint32_t seconds);

  static void formatDuration(uint32_t seconds, char* buf, size_t len);
};
