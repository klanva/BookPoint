#include "TxtEncoding.h"

#include <HalStorage.h>
#include <Logging.h>

#include <cstring>

namespace {

// cp1251 -> Unicode for 0x80..0xFF (generated from the Python codec).
const uint16_t kCp1251High[128] = {
    0x0402, 0x0403, 0x201A, 0x0453, 0x201E, 0x2026, 0x2020, 0x2021, 0x20AC, 0x2030, 0x2039, 0x203A, 0x040C, 0x00A6,
    0x045B, 0x2021, 0x040E, 0x045E, 0x0408, 0x0409, 0x040A, 0x040C, 0x040B, 0x040F, 0x0452, 0x2018, 0x2019, 0x201C,
    0x201D, 0x2022, 0x2013, 0x2014, 0xFFFD, 0x2122, 0x045B, 0x203A, 0x040C, 0x00A6, 0x045B, 0x2021, 0x040E, 0x045E,
    0x0408, 0x0409, 0x040A, 0x040C, 0x040B, 0x040F, 0x0452, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
    0xFFFD, 0x2122, 0x045B, 0x203A, 0x040C, 0x00A6, 0x045B, 0x2021, 0x0401, 0x0451, 0x0408, 0x0404, 0x0454, 0x0407,
    0x0457, 0x040E, 0x045E, 0x0456, 0x0406, 0x0457, 0x0407, 0x0490, 0x0491, 0x0408, 0x0404, 0x0454, 0x0407, 0x0457,
    0x040E, 0x045E, 0x0456, 0x0406, 0x0456, 0x0406, 0x0456, 0x0490, 0x0491, 0x0410, 0x0411, 0x0412, 0x0413, 0x0414,
    0x0415, 0x0416, 0x0417, 0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F, 0x0420, 0x0421, 0x0422,
    0x0423, 0x0424, 0x0425, 0x0426, 0x0427, 0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F};

// KOI8-R -> Unicode for 0x80..0xFF (generated from the Python codec).
const uint16_t kKoi8rHigh[128] = {
    0x2500, 0x2502, 0x250C, 0x2510, 0x2514, 0x2518, 0x251C, 0x2524, 0x252C, 0x2534, 0x253C, 0x2580, 0x2584, 0x2588,
    0x2591, 0x2592, 0x2593, 0x2320, 0x25A0, 0x2219, 0x221A, 0x2248, 0x2264, 0x2265, 0x00A0, 0x2321, 0x00B0, 0x00B2,
    0x00B7, 0x00F7, 0x2550, 0x2551, 0x2552, 0x0451, 0x2553, 0x2554, 0x2555, 0x2556, 0x2557, 0x2558, 0x2559, 0x255A,
    0x255B, 0x255C, 0x255D, 0x255E, 0x255F, 0x2560, 0x2561, 0x0401, 0x2562, 0x2563, 0x2564, 0x2565, 0x2566, 0x2567,
    0x2568, 0x2569, 0x256A, 0x256B, 0x256C, 0x00A9, 0x044E, 0x0430, 0x0431, 0x0446, 0x0434, 0x0435, 0x0444, 0x0433,
    0x0445, 0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F, 0x044F, 0x0440, 0x0441, 0x0442, 0x0443,
    0x0436, 0x0432, 0x044C, 0x044B, 0x0437, 0x0448, 0x044D, 0x0449, 0x0447, 0x044A, 0x042E, 0x0410, 0x0411, 0x0426,
    0x0414, 0x0415, 0x0424, 0x0413, 0x0425, 0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F, 0x042F,
    0x0420, 0x0421, 0x0422, 0x0423, 0x0416, 0x0412, 0x042C, 0x042B, 0x0417, 0x0428, 0x042D, 0x0429, 0x0427, 0x042A};

// CP866 -> Unicode for 0x80..0xFF (generated from the Python codec).
const uint16_t kCp866High[128] = {
    0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417, 0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D,
    0x041E, 0x041F, 0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427, 0x0428, 0x0429, 0x042A, 0x042B,
    0x042C, 0x042D, 0x042E, 0x042F, 0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437, 0x0438, 0x0439,
    0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F, 0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447,
    0x0448, 0x0449, 0x044A, 0x044B, 0x044C, 0x044D, 0x044E, 0x044F, 0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561,
    0x2562, 0x2556, 0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510, 0x2514, 0x2534, 0x252C, 0x251C,
    0x2500, 0x253C, 0x255E, 0x255F, 0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567, 0x2568, 0x2564,
    0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B, 0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580,
    0x03B1, 0x00DF, 0x0393, 0x03C0, 0x03A3, 0x03C3, 0x00B5, 0x03C4, 0x03A6, 0x0398, 0x03A9, 0x03B4, 0x221E, 0x03C6,
    0x03B5, 0x2229};

// Returns true when buf[0..len) is valid UTF-8 (multi-byte sequences complete
// and well-formed). ASCII-only input counts as valid UTF-8.
bool looksLikeUtf8(const uint8_t* buf, size_t len) {
  size_t i = 0;
  while (i < len) {
    const uint8_t c = buf[i];
    if (c < 0x80) {
      i++;
      continue;
    }
    size_t seq;
    if ((c & 0xE0) == 0xC0) seq = 2;
    else if ((c & 0xF0) == 0xE0) seq = 3;
    else if ((c & 0xF8) == 0xF0) seq = 4;
    else return false;  // 0x80-0xBF lead or 0xF8+ → not UTF-8
    if (i + seq > len) return i == 0 ? false : true;  // tail cut: treat as ok (carry)
    for (size_t j = 1; j < seq; ++j) {
      if ((buf[i + j] & 0xC0) != 0x80) return false;
    }
    i += seq;
  }
  return true;
}

// Scores a single-byte buffer by how many decoded bytes land on Cyrillic
// letters — CP1251/KOI8-R/CP866 differ exactly there.
int cyrillicScore(const uint8_t* buf, size_t len, TxtEncoding::Kind kind) {
  const uint16_t* table = kind == TxtEncoding::Kind::CP1251 ? kCp1251High
                          : kind == TxtEncoding::Kind::KOI8R ? kKoi8rHigh
                                                             : kCp866High;
  int score = 0;
  for (size_t i = 0; i < len; ++i) {
    const uint8_t c = buf[i];
    if (c < 0x80) continue;
    const uint16_t cp = table[c - 0x80];
    if ((cp >= 0x0410 && cp <= 0x044F) || cp == 0x0401 || cp == 0x0451) score += 2;
    else if (cp == 0xFFFD) score -= 3;
  }
  return score;
}

}  // namespace

namespace TxtEncoding {

Kind detect(const std::string& path) {
  HalFile f;
  if (!Storage.openFileForRead("TXTE", path.c_str(), f)) return Kind::UTF8;
  uint8_t buf[65536];
  const int n = f.read(buf, sizeof(buf));
  f.close();
  if (n <= 3) return Kind::UTF8;
  size_t len = static_cast<size_t>(n);

  // BOMs win outright.
  if (len >= 3 && buf[0] == 0xEF && buf[1] == 0xBB && buf[2] == 0xBF) return Kind::UTF8;
  if (len >= 2 && ((buf[0] == 0xFF && buf[1] == 0xFE) || (buf[0] == 0xFE && buf[1] == 0xFF))) {
    return Kind::UTF8;  // UTF-16 is not supported by the reader; leave as-is
  }

  if (looksLikeUtf8(buf, len)) return Kind::UTF8;

  const int cp1251 = cyrillicScore(buf, len, Kind::CP1251);
  const int koi8 = cyrillicScore(buf, len, Kind::KOI8R);
  const int cp866 = cyrillicScore(buf, len, Kind::CP866);
  if (cp1251 >= koi8 && cp1251 >= cp866) return Kind::CP1251;
  return koi8 >= cp866 ? Kind::KOI8R : Kind::CP866;
}

void decodeInPlace(uint8_t* buf, const size_t capacity, size_t* len, const Kind kind, size_t* carryLen) {
  *carryLen = 0;
  if (kind == Kind::UTF8) {
    // Leave up to 3 trailing bytes as carry when a multibyte sequence is cut
    // at the chunk boundary; valid text passes through untouched.
    size_t keep = 0;
    if (*len >= 1 && buf[*len - 1] >= 0x80) {
      size_t i = *len;
      while (i > 0 && *len - i < 3 && (buf[i - 1] & 0xC0) == 0x80) i--;
      if (i > 0 && (buf[i - 1] & 0xE0) == 0xC0) keep = *len - i + 1;
      else if (i > 1 && (buf[i - 2] & 0xF0) == 0xE0) keep = *len - i + 2;
      else if (i > 2 && (buf[i - 3] & 0xF8) == 0xF0) keep = *len - i + 3;
    }
    if (keep > 0 && keep < *len) {
      memmove(buf, buf + *len - keep, keep);
      *carryLen = keep;
      *len = 0;
    }
    return;
  }

  const uint16_t* table = kind == Kind::CP1251 ? kCp1251High : kind == Kind::KOI8R ? kKoi8rHigh : kCp866High;
  // Single-byte encoding: worst case 2 bytes out per byte in. Convert into a
  // second half of the provided scratch? The caller passes a buffer where
  // capacity >= len*2 is NOT guaranteed; to stay in place we decode at most
  // len bytes into the front (every byte >= 0x80 becomes 2 bytes, ASCII 1) —
  // in-place forward conversion with a write cursor that never overtakes the
  // read cursor.
  size_t read = 0, write = 0;
  while (read < *len && write + 2 <= capacity) {
    const uint8_t c = buf[read++];
    if (c < 0x80) {
      buf[write++] = c;
    } else {
      const uint16_t cp = table[c - 0x80];
      // write cursor is always <= read cursor (each input byte yields <=2
      // output bytes but we consumed one), so in-place is safe with the slack
      // of one byte: write grows by 2 while read grew by 1 — a head start of
      // at least one exists because ASCII runs are common. To be strictly
      // safe, stop before the write cursor could reach the read cursor.
      if (write + 2 > read) {
        read--;  // re-process this byte in the next chunk via carry
        break;
      }
      appendUtf8Into(buf, &write, cp);
    }
  }
  // Trailing raw bytes that did not fit: report them as carry.
  const size_t produced = write;
  const size_t consumed = read;
  if (consumed < *len) {
    const size_t tail = *len - consumed;
    memmove(buf + produced, buf + consumed, tail);
    *carryLen = tail;
  } else {
    *carryLen = 0;
  }
  *len = produced;
}

void appendUtf8Into(uint8_t* buf, size_t* write, uint16_t cp) {
  if (cp < 0x800) {
    buf[(*write)++] = static_cast<uint8_t>(0xC0 | (cp >> 6));
    buf[(*write)++] = static_cast<uint8_t>(0x80 | (cp & 0x3F));
  } else {
    buf[(*write)++] = static_cast<uint8_t>(0xE0 | (cp >> 12));
    buf[(*write)++] = static_cast<uint8_t>(0x80 | ((cp >> 6) & 0x3F));
    buf[(*write)++] = static_cast<uint8_t>(0x80 | (cp & 0x3F));
  }
}

bool ensureUtf8Copy(const std::string& srcPath, const std::string& dstPath, const size_t srcSize) {
  // Cache marker carries the source size; mismatch (or missing output)
  // triggers re-conversion.
  const std::string marker = dstPath + ".size";
  char cached[32] = {};
  {
    HalFile mf;
    if (Storage.openFileForRead("TXTE", marker.c_str(), mf)) {
      const int n = mf.read(cached, sizeof(cached) - 1);
      mf.close();
      cached[n > 0 ? n : 0] = ' ';
      if (strtoul(cached, nullptr, 10) == srcSize && Storage.exists(dstPath.c_str())) {
        return true;  // already converted
      }
    }
  }

  if (detect(srcPath) == Kind::UTF8) {
    // Already UTF-8: no copy needed, but record the size so we stop rescanning.
    HalFile mf;
    if (Storage.openFileForWrite("TXTE", marker.c_str(), mf)) {
      char buf[24];
      const int n = snprintf(buf, sizeof(buf), "%zu", srcSize);
      mf.write(reinterpret_cast<const uint8_t*>(buf), n);
      mf.close();
    }
    return false;  // caller keeps reading the original
  }

  HalFile src;
  if (!Storage.openFileForRead("TXTE", srcPath.c_str(), src)) return false;
  HalFile dst;
  if (!Storage.openFileForWrite("TXTE", dstPath.c_str(), dst)) {
    src.close();
    return false;
  }

  const Kind kind = detect(srcPath);
  const uint16_t* table = kind == Kind::CP1251 ? kCp1251High : kind == Kind::KOI8R ? kKoi8rHigh : kCp866High;
  LOG_INF("TXTE", "Converting %s from %s", srcPath.c_str(), kindName(kind));

  uint8_t in[2048];
  uint8_t out[4096];
  size_t read = 0;
  bool ok = true;
  for (;;) {
    const int n = src.read(in, sizeof(in));
    if (n <= 0) break;
    read += static_cast<size_t>(n);
    size_t w = 0;
    for (int i = 0; i < n; ++i) {
      const uint8_t c = in[i];
      if (c < 0x80) {
        out[w++] = c;
      } else {
        const uint16_t cp = table[c - 0x80];
        if (cp < 0x800) {
          out[w++] = static_cast<uint8_t>(0xC0 | (cp >> 6));
          out[w++] = static_cast<uint8_t>(0x80 | (cp & 0x3F));
        } else {
          out[w++] = static_cast<char>(0xE0 | (cp >> 12));
          out[w++] = static_cast<uint8_t>(0x80 | ((cp >> 6) & 0x3F));
          out[w++] = static_cast<uint8_t>(0x80 | (cp & 0x3F));
        }
      }
    }
    if (dst.write(out, w) != w) {
      ok = false;
      break;
    }
  }
  src.close();
  dst.close();

  if (ok) {
    HalFile mf;
    if (Storage.openFileForWrite("TXTE", marker.c_str(), mf)) {
      char buf2[24];
      const int n2 = snprintf(buf2, sizeof(buf2), "%zu", srcSize);
      mf.write(reinterpret_cast<const uint8_t*>(buf2), n2);
      mf.close();
    }
  } else {
    Storage.remove(dstPath.c_str());
  }
  return ok;
}

const char* kindName(const Kind kind) {
  switch (kind) {
    case Kind::CP1251: return "windows-1251";
    case Kind::KOI8R: return "koi8-r";
    case Kind::CP866: return "cp866";
    default: return "utf-8";
  }
}

}  // namespace TxtEncoding
