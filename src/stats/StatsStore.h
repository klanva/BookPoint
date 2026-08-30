#pragma once

#include <string>

#include "BookReadingStats.h"
#include "GlobalReadingStats.h"

// Persistence for the reading statistics module. Everything lives on the SD
// card under /.crosspoint/stats/ so it survives firmware updates, cache
// clears, and (with the CSV snapshot) even a dead firmware.
//
// All saves are crash-safe: data goes to a .tmp file first, is flushed, then
// renamed into place. The previous file is rotated to .bak first, so a crash
// mid-save can cost at most the newest update — never the accumulated data.
// Files from a NEWER format version are never overwritten.
namespace StatsStore {

// Book path -> stable file key. FNV-1a 64 so the mapping never changes,
// independent of the C++ standard library's std::hash seeding.
std::string bookStatsKey(const std::string& bookPath);

// Loads (or imports) the stats for a book. Falls back to the legacy inkMOD
// per-book file inside the book's cache directory when no central file
// exists yet; the title/author are stored in the container on first save.
BookReadingStats loadBookStats(const std::string& bookPath, const std::string& title, const std::string& author);

// Loads the central container without metadata/import fallbacks. Returns
// false when no central file exists yet (out keeps its defaults).
bool loadBookStatsInto(const std::string& bookPath, BookReadingStats& out);

// Saves the per-book stats in the central store.
void saveBookStats(const BookReadingStats& stats);

// Sets/clears the finished flag on a book. Setting it also stamps the finish
// date (unless the user overrode it); clearing it clears the auto date.
void setBookFinished(const std::string& bookPath, bool finished);

GlobalReadingStats loadGlobalStats();
void saveGlobalStats(const GlobalReadingStats& stats);

// Writes a CSV snapshot of the global totals plus every per-book entry to
// /.crosspoint/stats/reading_stats.csv. Called automatically after every
// session commit, and manually from the statistics screen.
bool exportCsv();

const char* csvPath();

}  // namespace StatsStore
