#pragma once

#include <string>

#include <Epub.h>

// Extracts the human-readable text of an EPUB footnote/note target so the
// reader can render it at the bottom of the referencing page ("paper style"
// footnotes).
//
// The target element is located by its id/anchor inside the referenced chapter
// file. Files are streamed from the EPUB archive in small chunks — a huge
// notes chapter costs a scan pass, not a RAM buffer. Text inside the target
// element is tag-stripped, entity-decoded, and whitespace-collapsed; extraction
// stops early once the caller's byte cap is reached.
namespace FootnoteTextExtractor {

// Returns the decoded plain text of the element with the given href target
// (e.g. "notes.html#n12" or "#n12" — relative to the current chapter).
// Returns an empty string when the target cannot be resolved or contains no
// text. maxBytes caps the returned UTF-8 payload (safely at UTF-8 boundaries).
std::string extract(const Epub& epub, int currentSpineIndex, const std::string& href, size_t maxBytes = 768);

// Small fixed-size per-session cache keyed by href, so flipping back and
// forth between pages does not rescan the notes file. Bounded: oldest entries
// are dropped once the cache exceeds capacity.
std::string extractCached(const Epub& epub, int currentSpineIndex, const std::string& href, size_t maxBytes = 768);
void clearCache();

}  // namespace FootnoteTextExtractor
