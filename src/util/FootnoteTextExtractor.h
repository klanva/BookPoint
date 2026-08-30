#pragma once

#include <string>

#include <Epub.h>

// Extracts the human-readable text of an EPUB footnote/note target so the
// reader can render it at the bottom of the referencing page ("paper style"
// footnotes).
//
// The target element is located by its id/xml:id/name anchor inside the
// referenced chapter file, and everything up to that element's closing tag is
// captured. Files are streamed from the EPUB archive in small chunks, so a
// huge notes chapter costs one scan pass, not a RAM buffer. The text is
// tag-stripped, entity-decoded, and whitespace-collapsed; extraction stops at
// the caller's byte cap (safely, at a UTF-8 boundary).
//
// `label` is the reference label as shown in the text (e.g. "[14]"). When the
// captured note starts with the same number (books like royallib put a
// number-only block first), that leading number is dropped so it is not
// printed twice.
namespace FootnoteTextExtractor {

std::string extract(const Epub& epub, int currentSpineIndex, const std::string& href, size_t maxBytes = 768,
                    const char* label = nullptr);

// Same, with a small fixed-size per-session cache keyed by href, so flipping
// back and forth between pages does not rescan the notes file. Bounded: the
// oldest entries are dropped once the cache exceeds capacity.
std::string extractCached(const Epub& epub, int currentSpineIndex, const std::string& href, size_t maxBytes = 768,
                          const char* label = nullptr);

void clearCache();

}  // namespace FootnoteTextExtractor
