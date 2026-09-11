#pragma once

#include <cstdint>
#include <string>

// Text encoding detection and conversion for the TXT reader.
//
// Detects UTF-8 (with BOM), Windows-1251, KOI8-R and CP866 by scoring decoded
// Cyrillic frequency against invalid UTF-8 sequences. Legacy single-byte
// encodings are converted to UTF-8 in a bounded scratch pass; the reader then
// works with plain UTF-8 everywhere.
namespace TxtEncoding {

enum class Kind : uint8_t { UTF8, CP1251, KOI8R, CP866 };

// Scans up to the first 64 KB of the file. Returns the detected kind.
Kind detect(const std::string& path);

// Convert one buffer in place. `buf` must have room for `capacity` bytes;
// `len` is the input length in bytes; `*len` becomes the output length.
// A carry of up to 3 trailing bytes of a split multibyte sequence is left in
// the buffer and reported via `*carryLen` so the caller can prepend it to the
// next chunk. ASCII and valid UTF-8 input pass through unchanged.
void decodeInPlace(uint8_t* buf, size_t capacity, size_t* len, Kind kind, size_t* carryLen);

// Appends a codepoint as UTF-8 at buf[(*write)++]. Exposed for the in-place
// decoder; 2-3 bytes per codepoint.

// Name of the encoding for the diagnostics/log line.
void appendUtf8Into(uint8_t* buf, size_t* write, uint16_t cp);

const char* kindName(Kind kind);

// Detects the encoding of `srcPath`; when it is not UTF-8, streams the file
// through a converter into `dstPath` (UTF-8). Returns true when dstPath holds
// UTF-8 content (converted now, already converted, or source was UTF-8).
// `srcSize` guards the cache: a changed size triggers re-conversion.
bool ensureUtf8Copy(const std::string& srcPath, const std::string& dstPath, size_t srcSize);

}  // namespace TxtEncoding
