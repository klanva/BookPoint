#include "StatsStore.h"

#include <HalStorage.h>
#include <Logging.h>

#include <cstdio>
#include <cstring>
#include <vector>

namespace {

constexpr char STATS_DIR[] = "/.crosspoint/stats";
constexpr char BOOKS_DIR[] = "/.crosspoint/stats/books";
constexpr char GLOBAL_PATH[] = "/.crosspoint/stats/global.bin";
constexpr char GLOBAL_BAK_PATH[] = "/.crosspoint/stats/global.bin.bak";
constexpr char LEGACY_PREVIOUS_GLOBAL[] = "/.inkmod/global_stats.bin";
constexpr char CSV_PATH[] = "/.crosspoint/stats/reading_stats.csv";

constexpr uint8_t BOOK_FILE_VERSION = 1;
constexpr uint8_t GLOBAL_FILE_VERSION = 1;
constexpr size_t BOOK_PAYLOAD_SIZE = 32;
constexpr size_t GLOBAL_FILE_SIZE = 251;
constexpr char BOOK_MAGIC[] = "CPSB";
constexpr char GLOBAL_MAGIC[] = "CPGS";
constexpr size_t MAX_BOOK_FILES = 1024;  // safety cap for CSV iteration

bool g_globalNewerFormatSeen = false;

uint16_t readLe16(const uint8_t* data, const int offset) {
  return static_cast<uint16_t>(data[offset]) | (static_cast<uint16_t>(data[offset + 1]) << 8);
}

uint32_t readLe32(const uint8_t* data, const int offset) {
  return static_cast<uint32_t>(data[offset]) | (static_cast<uint32_t>(data[offset + 1]) << 8) |
         (static_cast<uint32_t>(data[offset + 2]) << 16) | (static_cast<uint32_t>(data[offset + 3]) << 24);
}

void writeLe16(uint8_t* data, const int offset, const uint16_t value) {
  data[offset] = value & 0xFF;
  data[offset + 1] = (value >> 8) & 0xFF;
}

void writeLe32(uint8_t* data, const int offset, const uint32_t value) {
  data[offset] = value & 0xFF;
  data[offset + 1] = (value >> 8) & 0xFF;
  data[offset + 2] = (value >> 16) & 0xFF;
  data[offset + 3] = (value >> 24) & 0xFF;
}

void ensureStatsDirs() {
  if (!Storage.exists(STATS_DIR)) Storage.mkdir(STATS_DIR);
  if (!Storage.exists(BOOKS_DIR)) Storage.mkdir(BOOKS_DIR);
}

// Crash-safe replace: rotate the current file to .bak, write a tmp file,
// fsync, then rename over the destination.
bool replaceFileAtomic(const char* path, const char* backupPath, const uint8_t* data, const size_t len) {
  const std::string tmpPath = std::string(path) + ".tmp";

  if (Storage.exists(tmpPath.c_str()) && !Storage.remove(tmpPath.c_str())) {
    LOG_ERR("STATS", "Could not remove stale tmp file %s", tmpPath.c_str());
    return false;
  }

  {
    HalFile f;
    if (!Storage.openFileForWrite("STATS", tmpPath.c_str(), f)) {
      LOG_ERR("STATS", "Could not open tmp file for write: %s", tmpPath.c_str());
      return false;
    }
    if (f.write(data, len) != len) {
      LOG_ERR("STATS", "Short write to %s", tmpPath.c_str());
      f.close();
      Storage.remove(tmpPath.c_str());
      return false;
    }
    f.flush();
    f.close();
  }

  if (backupPath != nullptr) {
    if (Storage.exists(backupPath) && !Storage.remove(backupPath)) {
      Storage.remove(tmpPath.c_str());
      return false;
    }
    if (Storage.exists(path) && !Storage.rename(path, backupPath)) {
      Storage.remove(tmpPath.c_str());
      return false;
    }
  } else if (Storage.exists(path) && !Storage.remove(path)) {
    Storage.remove(tmpPath.c_str());
    return false;
  }

  if (!Storage.rename(tmpPath.c_str(), path)) {
    LOG_ERR("STATS", "Could not commit %s", path);
    // Roll the backup back so the stats are never left missing entirely.
    if (backupPath != nullptr && Storage.exists(backupPath) && !Storage.exists(path)) {
      Storage.rename(backupPath, path);
    }
    Storage.remove(tmpPath.c_str());
    return false;
  }
  return true;
}

std::string readLenPrefixedString(const uint8_t* data, const size_t dataSize, size_t& offset) {
  if (offset + 2 > dataSize) return {};
  const uint16_t len = readLe16(data, static_cast<int>(offset));
  offset += 2;
  if (offset + len > dataSize) return {};
  std::string out(reinterpret_cast<const char*>(data + offset), len);
  offset += len;
  return out;
}

void writeLenPrefixedString(std::vector<uint8_t>& out, const std::string& value) {
  const uint16_t len = static_cast<uint16_t>(value.size());
  out.push_back(len & 0xFF);
  out.push_back((len >> 8) & 0xFF);
  out.insert(out.end(), value.begin(), value.end());
}

// ---------- per-book container ----------

std::string bookFilePath(const std::string& key) { return std::string(BOOKS_DIR) + "/" + key + ".bin"; }

void serializeBookPayload(const BookReadingStats& stats, uint8_t* p) {
  memset(p, 0, BOOK_PAYLOAD_SIZE);
  writeLe16(p, 0, stats.sessionCount);
  writeLe32(p, 2, stats.totalReadingSeconds);
  writeLe32(p, 6, stats.totalPagesTurned);
  writeLe16(p, 10, stats.avgSecondsPerForwardPage);
  writeLe16(p, 12, stats.paceSampleCount);
  p[14] = stats.isFinished ? 1 : 0;
  p[15] = (stats.startDateManual ? 1u : 0u) | (stats.finishedDateManual ? 2u : 0u) |
          (stats.completedCounted ? 4u : 0u);
  writeLe16(p, 16, stats.startDate.isValid() ? stats.startDate.year : 0);
  p[18] = stats.startDate.isValid() ? stats.startDate.month : 0;
  p[19] = stats.startDate.isValid() ? stats.startDate.day : 0;
  writeLe16(p, 20, stats.finishedDate.isValid() ? stats.finishedDate.year : 0);
  p[22] = stats.finishedDate.isValid() ? stats.finishedDate.month : 0;
  p[23] = stats.finishedDate.isValid() ? stats.finishedDate.day : 0;
  writeLe32(p, 24, stats.lastReadEpoch);
}

void deserializeBookPayload(const uint8_t* p, BookReadingStats& stats) {
  stats.sessionCount = readLe16(p, 0);
  stats.totalReadingSeconds = readLe32(p, 2);
  stats.totalPagesTurned = readLe32(p, 6);
  stats.avgSecondsPerForwardPage = readLe16(p, 10);
  stats.paceSampleCount = readLe16(p, 12);
  stats.isFinished = p[14] != 0;
  const uint8_t flags = p[15];
  stats.startDateManual = (flags & 1u) != 0;
  stats.finishedDateManual = (flags & 2u) != 0;
  stats.completedCounted = (flags & 4u) != 0;
  const auto readDate = [&p](const int offset) {
    ReadingStatsDate date;
    date.year = readLe16(p, offset);
    date.month = p[offset + 2];
    date.day = p[offset + 3];
    if (!date.isValid()) date.clear();
    return date;
  };
  stats.startDate = readDate(16);
  stats.finishedDate = readDate(20);
  stats.lastReadEpoch = readLe32(p, 24);
}

std::vector<uint8_t> serializeBookContainer(const BookReadingStats& stats) {
  std::vector<uint8_t> out;
  out.reserve(16 + stats.bookPath.size() + stats.bookTitle.size() + stats.bookAuthor.size() + BOOK_PAYLOAD_SIZE);
  out.insert(out.end(), BOOK_MAGIC, BOOK_MAGIC + 4);
  out.push_back(BOOK_FILE_VERSION);
  writeLenPrefixedString(out, stats.bookPath);
  writeLenPrefixedString(out, stats.bookTitle);
  writeLenPrefixedString(out, stats.bookAuthor);
  uint8_t payload[BOOK_PAYLOAD_SIZE];
  serializeBookPayload(stats, payload);
  out.insert(out.end(), payload, payload + BOOK_PAYLOAD_SIZE);
  return out;
}

bool loadBookContainer(const char* path, BookReadingStats& stats) {
  HalFile f;
  if (!Storage.openFileForRead("STATS", path, f)) return false;
  const size_t size = f.fileSize();
  if (size < 4 + 1 + 6 + BOOK_PAYLOAD_SIZE || size > 2048) {
    f.close();
    return false;
  }
  std::vector<uint8_t> data(size);
  const int n = f.read(data.data(), size);
  f.close();
  if (n <= 0 || static_cast<size_t>(n) != size) return false;
  if (memcmp(data.data(), BOOK_MAGIC, 4) != 0 || data[4] != BOOK_FILE_VERSION) {
    if (memcmp(data.data(), BOOK_MAGIC, 4) == 0 && data[4] > BOOK_FILE_VERSION) {
      LOG_ERR("STATS", "Book stats %s are from a newer build (v%u); keeping file untouched", path, data[4]);
    }
    return false;
  }

  size_t offset = 5;
  stats.bookPath = readLenPrefixedString(data.data(), size, offset);
  stats.bookTitle = readLenPrefixedString(data.data(), size, offset);
  stats.bookAuthor = readLenPrefixedString(data.data(), size, offset);
  if (offset + BOOK_PAYLOAD_SIZE > size) return false;
  deserializeBookPayload(data.data() + offset, stats);
  return true;
}

// ---------- legacy import ----------
// Reads the legacy global file (v3, 159 bytes) and folds its lifetime totals,
// distribution buckets, streak record, and read-day history into our stores.
// Day history is converted at 1 second per marked day: enough to preserve
// streaks and show activity on the chart without inventing precise numbers.
bool importLegacyGlobal(GlobalReadingStats& target) {
  HalFile f;
  if (!Storage.openFileForRead("STATS", LEGACY_PREVIOUS_GLOBAL, f)) return false;
  const size_t size = f.fileSize();
  if (size != 159) {
    f.close();
    return false;
  }
  uint8_t data[159] = {};
  const int n = f.read(data, sizeof(data));
  f.close();
  if (n != 159 || data[0] != 3) return false;

  LOG_INF("STATS", "Importing reading statistics from previous firmware");
  target.totalSessions += readLe32(data, 1);
  target.totalReadingSeconds += readLe32(data, 5);
  target.totalPagesTurned += readLe32(data, 9);
  target.completedBooks += readLe32(data, 13);
  for (size_t i = 0; i < READING_TIME_BUCKET_COUNT; ++i) {
    target.timeOfDaySeconds[i] += readLe32(data, 17 + static_cast<int>(i) * 4);
  }
  for (size_t i = 0; i < READING_DAY_OF_WEEK_COUNT; ++i) {
    target.dayOfWeekSeconds[i] += readLe32(data, 33 + static_cast<int>(i) * 4);
  }

  const uint32_t legacyAnchor = readLe32(data, 61);
  if (legacyAnchor != 0) {
    const uint16_t legacyLongest = readLe16(data, 157);
    if (legacyLongest > target.longestReadingStreak) target.longestReadingStreak = legacyLongest;

    for (size_t bitIndex = 0; bitIndex < 730 && bitIndex < READING_DAY_HISTORY_DAYS; ++bitIndex) {
      const uint8_t byte = data[65 + bitIndex / 8];
      const bool marked = (byte & (1u << (bitIndex % 8))) != 0;
      if (marked && legacyAnchor >= bitIndex) {
        const uint32_t dayIndex = legacyAnchor - bitIndex;
        const uint32_t ringOffset = (target.anchorDayIndex == 0) ? 0 : target.anchorDayIndex - dayIndex;
        if (target.anchorDayIndex == 0) {
          target.anchorDayIndex = dayIndex;
          target.daySeconds[0] = 1;
        } else if (dayIndex > target.anchorDayIndex) {
          // Older than everything we hold — shift the ring forward.
          const uint32_t shift = dayIndex - target.anchorDayIndex;
          if (shift >= READING_DAY_HISTORY_DAYS) {
            memset(target.daySeconds, 0, sizeof(target.daySeconds));
          } else {
            for (size_t i = READING_DAY_HISTORY_DAYS - 1; i >= shift; --i) {
              target.daySeconds[i] = target.daySeconds[i - shift];
            }
            memset(target.daySeconds, 0, shift * sizeof(target.daySeconds[0]));
          }
          target.anchorDayIndex = dayIndex;
          target.daySeconds[0] = 1;
        } else if (ringOffset < READING_DAY_HISTORY_DAYS && target.daySeconds[ringOffset] == 0) {
          target.daySeconds[ringOffset] = 1;
        }
      }
    }
  }
  return true;
}

// Reads the legacy per-book stats (v4, 69 bytes) from a book cache directory.
bool importLegacyBookStats(const std::string& legacyPath, BookReadingStats& stats) {
  HalFile f;
  if (!Storage.openFileForRead("STATS", legacyPath.c_str(), f)) return false;
  const size_t size = f.fileSize();
  if (size != 69) {
    f.close();
    return false;
  }
  uint8_t data[69] = {};
  const int n = f.read(data, sizeof(data));
  f.close();
  if (n != 69 || data[0] != 4) return false;

  stats.sessionCount = readLe16(data, 1);
  stats.totalReadingSeconds = readLe32(data, 3);
  stats.totalPagesTurned = readLe32(data, 7);
  stats.isFinished = data[11] != 0;
  stats.avgSecondsPerForwardPage = readLe16(data, 12);
  stats.paceSampleCount = readLe16(data, 14);
  const uint8_t flags = data[16];
  stats.startDateManual = (flags & 1u) != 0;
  stats.finishedDateManual = (flags & 2u) != 0;
  const auto readDate = [&data](const int offset) {
    ReadingStatsDate date;
    date.year = readLe16(data, offset);
    date.month = data[offset + 2];
    date.day = data[offset + 3];
    if (!date.isValid()) date.clear();
    return date;
  };
  stats.startDate = readDate(17);
  stats.finishedDate = readDate(21);
  for (size_t i = 0; i < READING_TIME_BUCKET_COUNT; ++i) {
    stats.timeOfDaySeconds[i] = readLe32(data, 25 + static_cast<int>(i) * 4);
  }
  for (size_t i = 0; i < READING_DAY_OF_WEEK_COUNT; ++i) {
    stats.dayOfWeekSeconds[i] = readLe32(data, 41 + static_cast<int>(i) * 4);
  }
  return true;
}

// ---------- CSV ----------

void appendCsvRow(std::string& csv, const char* type, const BookReadingStats* stats) {
  char buf[64];
  csv += type;
  csv += ',';
  auto escape = [&csv](const std::string& s) {
    bool needsQuotes = s.find(',') != std::string::npos || s.find('"') != std::string::npos ||
                       s.find('\n') != std::string::npos;
    if (!needsQuotes) {
      csv += s;
      return;
    }
    csv += '"';
    for (const char c : s) {
      if (c == '"') csv += "\"\"";
      else csv += c;
    }
    csv += '"';
  };
  escape(stats != nullptr ? stats->bookPath : "");
  csv += ',';
  escape(stats != nullptr ? stats->bookTitle : "");
  csv += ',';
  escape(stats != nullptr ? stats->bookAuthor : "");
  csv += ',';
  if (stats != nullptr) {
    snprintf(buf, sizeof(buf), "%u,%lu,%lu,%u,%u,%c,", stats->sessionCount,
             static_cast<unsigned long>(stats->totalReadingSeconds), static_cast<unsigned long>(stats->totalPagesTurned),
             stats->avgSecondsPerForwardPage, stats->paceSampleCount, stats->isFinished ? 'y' : 'n');
    csv += buf;
    const auto fmtDate = [&csv, &buf](const ReadingStatsDate& d) {
      if (!d.isValid()) {
        csv += ',';
        return;
      }
      snprintf(buf, sizeof(buf), "%04u-%02u-%02u,", d.year, d.month, d.day);
      csv += buf;
    };
    fmtDate(stats->startDate);
    fmtDate(stats->finishedDate);
    snprintf(buf, sizeof(buf), "%lu", static_cast<unsigned long>(stats->lastReadEpoch));
    csv += buf;
  }
  csv += "\r\n";
}

}  // namespace

namespace StatsStore {

std::string bookStatsKey(const std::string& bookPath) {
  // FNV-1a 64-bit — deterministic across firmware versions.
  uint64_t hash = 1469598103934665603ull;
  for (const char c : bookPath) {
    hash ^= static_cast<uint8_t>(c);
    hash *= 1099511628211ull;
  }
  char buf[20];
  snprintf(buf, sizeof(buf), "%016llx", static_cast<unsigned long long>(hash));
  return buf;
}

BookReadingStats loadBookStats(const std::string& bookPath, const std::string& title, const std::string& author) {
  ensureStatsDirs();
  BookReadingStats stats;
  stats.bookPath = bookPath;
  stats.bookTitle = title;
  stats.bookAuthor = author;

  const std::string path = bookFilePath(bookStatsKey(bookPath));
  BookReadingStats loaded;
  if (loadBookContainer(path.c_str(), loaded)) {
    stats = loaded;
    // Refresh the metadata snapshot (title may have been corrected, etc.).
    stats.bookPath = bookPath;
    stats.bookTitle = title.empty() ? loaded.bookTitle : title;
    stats.bookAuthor = author.empty() ? loaded.bookAuthor : author;
    return stats;
  }

  // One-time import from the legacy per-book location(s).
  const std::string hashPath = std::to_string(std::hash<std::string>{}(bookPath));
  std::string legacy = "/.inkmod/epub_" + hashPath + "/stats.bin";
  if (!importLegacyBookStats(legacy, stats)) {
    legacy = "/.crosspoint/epub_" + hashPath + "/stats.bin";
    importLegacyBookStats(legacy, stats);
  }
  if (stats.sessionCount > 0 || stats.totalReadingSeconds > 0 || stats.totalPagesTurned > 0) {
    LOG_INF("STATS", "Imported per-book stats from %s", legacy.c_str());
  }
  return stats;
}

bool loadBookStatsInto(const std::string& bookPath, BookReadingStats& out) {
  const std::string path = bookFilePath(bookStatsKey(bookPath));
  BookReadingStats loaded;
  if (!loadBookContainer(path.c_str(), loaded)) return false;
  out = loaded;
  return true;
}

void saveBookStats(const BookReadingStats& stats) {
  ensureStatsDirs();
  const std::vector<uint8_t> container = serializeBookContainer(stats);
  const std::string path = bookFilePath(bookStatsKey(stats.bookPath));
  replaceFileAtomic(path.c_str(), nullptr, container.data(), container.size());
}

void setBookFinished(const std::string& bookPath, const bool finished) {
  BookReadingStats stats;
  const std::string path = bookFilePath(bookStatsKey(bookPath));
  if (!loadBookContainer(path.c_str(), stats)) return;
  if (stats.isFinished == finished) return;

  stats.isFinished = finished;
  if (finished) {
    if (!stats.finishedDateManual && !stats.finishedDate.isValid()) {
      ReadingStatsDateTime now;
      if (getCurrentLocalReadingStatsDateTime(now)) stats.finishedDate = now.date;
    }
    // End-of-book completion also counts device-wide, once per book.
    if (!stats.completedCounted) {
      GlobalReadingStats global = loadGlobalStats();
      global.completedBooks++;
      saveGlobalStats(global);
      stats.completedCounted = true;
    }
  } else {
    if (!stats.finishedDateManual) stats.finishedDate.clear();
    if (stats.completedCounted) {
      GlobalReadingStats global = loadGlobalStats();
      if (global.completedBooks > 0) global.completedBooks--;
      saveGlobalStats(global);
      stats.completedCounted = false;
    }
  }
  saveBookStats(stats);
}

GlobalReadingStats loadGlobalStats() {
  ensureStatsDirs();
  GlobalReadingStats stats;

  auto tryLoad = [](const char* path, GlobalReadingStats& out) -> bool {
    HalFile f;
    if (!Storage.openFileForRead("STATS", path, f)) return false;
    const size_t size = f.fileSize();
    if (size != GLOBAL_FILE_SIZE) {
      f.close();
      if (size > GLOBAL_FILE_SIZE) {
        LOG_ERR("STATS", "Global stats %s look newer (%u bytes); refusing to overwrite", path,
                static_cast<unsigned>(size));
        g_globalNewerFormatSeen = true;
      }
      return false;
    }
    uint8_t data[GLOBAL_FILE_SIZE] = {};
    const int n = f.read(data, sizeof(data));
    f.close();
    if (n != GLOBAL_FILE_SIZE) return false;
    if (memcmp(data, GLOBAL_MAGIC, 4) != 0) return false;
    if (data[4] > GLOBAL_FILE_VERSION) {
      LOG_ERR("STATS", "Global stats %s are from a newer build (v%u); refusing to overwrite", path, data[4]);
      g_globalNewerFormatSeen = true;
      return false;
    }
    if (data[4] != GLOBAL_FILE_VERSION) return false;

    out.totalSessions = readLe32(data, 5);
    out.totalReadingSeconds = readLe32(data, 9);
    out.totalPagesTurned = readLe32(data, 13);
    out.completedBooks = readLe32(data, 17);
    out.longestReadingStreak = readLe16(data, 21);
    out.anchorDayIndex = readLe32(data, 23);
    for (size_t i = 0; i < READING_DAY_HISTORY_DAYS; ++i) {
      out.daySeconds[i] = readLe16(data, 27 + static_cast<int>(i) * 2);
    }
    for (size_t i = 0; i < READING_TIME_BUCKET_COUNT; ++i) {
      out.timeOfDaySeconds[i] = readLe32(data, 207 + static_cast<int>(i) * 4);
    }
    for (size_t i = 0; i < READING_DAY_OF_WEEK_COUNT; ++i) {
      out.dayOfWeekSeconds[i] = readLe32(data, 223 + static_cast<int>(i) * 4);
    }
    return true;
  };

  if (tryLoad(GLOBAL_PATH, stats)) return stats;
  if (tryLoad(GLOBAL_BAK_PATH, stats)) {
    LOG_INF("STATS", "Recovered global stats from backup");
    return stats;
  }

  // Fresh device: adopt the legacy statistics once, if present.
  GlobalReadingStats imported;
  if (importLegacyGlobal(imported)) {
    stats = imported;
    saveGlobalStats(stats);
  }
  return stats;
}

void saveGlobalStats(const GlobalReadingStats& stats) {
  if (g_globalNewerFormatSeen) {
    LOG_ERR("STATS", "Refusing to save global stats over a newer-format file");
    return;
  }
  ensureStatsDirs();

  uint8_t data[GLOBAL_FILE_SIZE];
  memset(data, 0, sizeof(data));
  memcpy(data, GLOBAL_MAGIC, 4);
  data[4] = GLOBAL_FILE_VERSION;
  writeLe32(data, 5, stats.totalSessions);
  writeLe32(data, 9, stats.totalReadingSeconds);
  writeLe32(data, 13, stats.totalPagesTurned);
  writeLe32(data, 17, stats.completedBooks);
  writeLe16(data, 21, stats.longestReadingStreak);
  writeLe32(data, 23, stats.anchorDayIndex);
  for (size_t i = 0; i < READING_DAY_HISTORY_DAYS; ++i) {
    writeLe16(data, 27 + static_cast<int>(i) * 2, stats.daySeconds[i]);
  }
  for (size_t i = 0; i < READING_TIME_BUCKET_COUNT; ++i) {
    writeLe32(data, 207 + static_cast<int>(i) * 4, stats.timeOfDaySeconds[i]);
  }
  for (size_t i = 0; i < READING_DAY_OF_WEEK_COUNT; ++i) {
    writeLe32(data, 223 + static_cast<int>(i) * 4, stats.dayOfWeekSeconds[i]);
  }

  replaceFileAtomic(GLOBAL_PATH, GLOBAL_BAK_PATH, data, sizeof(data));
}

bool exportCsv() {
  ensureStatsDirs();

  std::string csv = "type,path,title,author,sessions,total_seconds,total_pages,pace_s_per_page,pace_samples,"
                    "finished,start_date,finish_date,last_read_epoch\r\n";

  char buf[96];
  const GlobalReadingStats global = loadGlobalStats();
  snprintf(buf, sizeof(buf), "%lu,%lu,%lu,%lu", static_cast<unsigned long>(global.totalSessions),
           static_cast<unsigned long>(global.totalReadingSeconds), static_cast<unsigned long>(global.totalPagesTurned),
           static_cast<unsigned long>(global.completedBooks));
  csv += "GLOBAL,,,,,,,,,,";
  csv += buf;
  csv += "\r\n";

  HalFile dir = Storage.open(BOOKS_DIR);
  if (!dir || !dir.isDirectory()) {
    // No per-book entries yet; still write the global row.
    HalFile out;
    if (!Storage.openFileForWrite("STATS", CSV_PATH, out)) return false;
    out.write(csv.c_str(), csv.size());
    out.close();
    return true;
  }

  uint16_t count = 0;
  char name[160];
  std::vector<std::string> paths;
  for (HalFile entry = dir.openNextFile(); entry; entry = dir.openNextFile()) {
    const size_t nameLen = entry.getName(name, sizeof(name));
    entry.close();
    if (nameLen == 0) continue;
    if (strstr(name, ".tmp") != nullptr || strstr(name, ".bak") != nullptr) continue;
    paths.emplace_back(name);
    if (++count >= MAX_BOOK_FILES) break;
  }
  dir.close();

  for (const auto& fileName : paths) {
    BookReadingStats stats;
    const std::string filePath = std::string(BOOKS_DIR) + "/" + fileName;
    if (!loadBookContainer(filePath.c_str(), stats)) continue;
    appendCsvRow(csv, "BOOK", &stats);
  }

  HalFile out;
  if (!Storage.openFileForWrite("STATS", CSV_PATH, out)) return false;
  const size_t written = out.write(csv.c_str(), csv.size());
  out.close();
  return written == csv.size();
}

const char* csvPath() { return CSV_PATH; }

}  // namespace StatsStore
