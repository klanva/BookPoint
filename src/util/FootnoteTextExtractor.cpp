#include "FootnoteTextExtractor.h"

#include <Logging.h>

#include <cctype>
#include <deque>

namespace {

// Decodes the handful of entities that matter for note bodies; unknown ones
// pass through untouched.
void decodeEntities(const std::string& in, std::string& out) {
  for (size_t i = 0; i < in.size(); ++i) {
    if (in[i] != '&') {
      out += in[i];
      continue;
    }
    const size_t semi = in.find(';', i);
    if (semi == std::string::npos || semi > i + 10) {
      out += '&';
      continue;
    }
    const std::string entity = in.substr(i, semi - i + 1);
    if (entity == "&amp;") out += '&';
    else if (entity == "&lt;") out += '<';
    else if (entity == "&gt;") out += '>';
    else if (entity == "&quot;") out += '"';
    else if (entity == "&apos;") out += '\'';
    else if (entity == "&nbsp;") out += ' ';
    else if (entity.size() > 3 && entity[1] == '#') {
      // &#NN; or &#xNN;
      const bool hex = entity[2] == 'x' || entity[2] == 'X';
      int code = 0;
      bool ok = true;
      for (size_t j = hex ? 3 : 2; j + 1 < entity.size() && ok; ++j) {
        const char c = entity[j];
        if (c >= '0' && c <= '9') {
          code = code * (hex ? 16 : 10) + (c - '0');
        } else if (hex && c >= 'a' && c <= 'f') {
          code = code * 16 + (c - 'a' + 10);
        } else if (hex && c >= 'A' && c <= 'F') {
          code = code * 16 + (c - 'A' + 10);
        } else {
          ok = false;
        }
      }
      if (ok && code > 0 && code < 0x110000) {
        if (code < 0x80) {
          out += static_cast<char>(code);
        } else if (code < 0x800) {
          out += static_cast<char>(0xC0 | (code >> 6));
          out += static_cast<char>(0x80 | (code & 0x3F));
        } else if (code < 0x10000) {
          out += static_cast<char>(0xE0 | (code >> 12));
          out += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
          out += static_cast<char>(0x80 | (code & 0x3F));
        } else {
          out += static_cast<char>(0xF0 | (code >> 18));
          out += static_cast<char>(0x80 | ((code >> 12) & 0x3F));
          out += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
          out += static_cast<char>(0x80 | (code & 0x3F));
        }
      } else {
        out += entity;
      }
    } else {
      out += entity;
    }
    i = semi;
  }
}

bool isVoidElementName(const std::string& name) {
  return name == "br" || name == "hr" || name == "img" || name == "input" || name == "meta" || name == "link" ||
         name == "col" || name == "area" || name == "base" || name == "embed" || name == "source" ||
         name == "track" || name == "wbr";
}

// Streams the target item and captures the text content of the element whose
// opening tag carries id="<anchor>". Chunk boundaries are bridged by keeping a
// carry window; capture stops early at the byte cap.
class AnchorScanner : public Print {
 public:
  explicit AnchorScanner(std::string anchor, const size_t maxBytes)
      : anchor_(std::move(anchor)), maxBytes_(maxBytes) {}

  size_t write(const uint8_t* data, const size_t len) override {
    if (done_) return len;
    buf_.append(reinterpret_cast<const char*>(data), len);
    process();
    return len;
  }

  size_t write(const uint8_t b) override { return write(&b, 1); }

  const std::string& result() const { return out_; }
  bool done() const { return done_; }

 private:
  static constexpr size_t CARRY = 512;   // bytes kept unprocessed across chunks
  static constexpr size_t SEARCH_TAIL = 1024;

  void process() {
    size_t pos = 0;

    if (state_ == SEEKING) {
      const size_t found = findAnchorStart();
      if (found == std::string::npos) {
        // Keep only the tail: an id attribute split across reads.
        if (buf_.size() > SEARCH_TAIL) buf_.erase(0, buf_.size() - SEARCH_TAIL);
        return;
      }
      if (!beginCapture(found)) {
        // Opening tag split across reads — keep from the tag start and retry
        // with more data.
        buf_.erase(0, found);
        return;
      }
      pos = capturePos_;
    }

    if (state_ == CAPTURING) {
      if (!captureFrom(pos)) {
        // Drop everything already consumed so a long notes file cannot grow
        // the buffer while we stream toward the anchor element.
        buf_.erase(0, capturePos_);
        capturePos_ = 0;
        return;
      }
    }

    // Only reached when done_.
    buf_.clear();
    buf_.shrink_to_fit();
  }

  // Locates the opening tag whose id attribute matches. Returns the offset of
  // '<' for that tag, or npos.
  size_t findAnchorStart() const {
    static const char* patterns[] = {"id=\"", "id='", "id="};
    size_t best = std::string::npos;
    for (const char* pat : patterns) {
      size_t pos = 0;
      const std::string p = pat;
      while ((pos = buf_.find(p, pos)) != std::string::npos) {
        // Attribute position sanity: preceded by whitespace or tag start.
        const bool boundaryOk = pos == 0 || isspace(static_cast<unsigned char>(buf_[pos - 1])) || buf_[pos - 1] == '<';
        const size_t valueStart = pos + p.size();
        if (boundaryOk && valueStart < buf_.size()) {
          const char quote = buf_[valueStart];
          const bool quoted = quote == '"' || quote == '\'';
          size_t valueEnd;
          if (quoted) {
            const size_t close = buf_.find(quote, valueStart + 1);
            if (close == std::string::npos) {
              pos = valueStart;
              continue;  // value still streaming; retry next chunk
            }
            valueEnd = close;
          } else {
            valueEnd = buf_.find_first_of(" \t\r\n>", valueStart);
            if (valueEnd == std::string::npos) {
              pos = valueStart;
              continue;
            }
          }
          if (buf_.compare(valueStart, valueEnd - valueStart, anchor_) == 0) {
            // Walk back to the '<' that opens this tag.
            const size_t tagStart = buf_.rfind('<', pos);
            if (tagStart != std::string::npos && (best == std::string::npos || tagStart < best)) best = tagStart;
            break;
          }
          pos = valueStart;
          continue;
        }
        pos = valueStart;
      }
    }
    return best;
  }

  // Sets up the capture state from the anchor tag start. Returns false when
  // the opening tag has not fully arrived yet.
  bool beginCapture(const size_t tagStart) {
    const size_t nameStart = tagStart + 1;
    if (nameStart >= buf_.size()) return false;
    size_t nameEnd = nameStart;
    while (nameEnd < buf_.size() && !isspace(static_cast<unsigned char>(buf_[nameEnd])) && buf_[nameEnd] != '>' &&
           buf_[nameEnd] != '/') {
      nameEnd++;
    }
    if (nameEnd >= buf_.size()) return false;
    tagName_ = buf_.substr(nameStart, nameEnd - nameStart);

    const size_t tagEnd = buf_.find('>', nameEnd);
    if (tagEnd == std::string::npos) return false;

    // Self-closing or void elements have no text body to capture; treat as
    // immediately done with empty text (caller falls back to the list view).
    const bool selfClosing = buf_[tagEnd - 1] == '/';
    if (selfClosing || isVoidElementName(tagName_)) {
      done_ = true;
      return true;
    }

    depth_ = 1;
    state_ = CAPTURING;
    capturePos_ = tagEnd + 1;
    return true;
  }

  bool captureFrom(size_t pos) {
    std::string textChunk;
    while (pos < buf_.size()) {
      const size_t lt = buf_.find('<', pos);
      if (lt == std::string::npos) {
        textChunk.append(buf_, pos, std::string::npos);
        pos = buf_.size();
        break;
      }
      textChunk.append(buf_, pos, lt - pos);

      // Find the end of this tag; wait for it when still streaming.
      const size_t tagEnd = buf_.find('>', lt);
      if (tagEnd == std::string::npos) {
        pos = lt;
        break;
      }
      const std::string tag = buf_.substr(lt + 1, tagEnd - lt - 1);
      if (!tag.empty() && tag[0] == '/') {
        // Closing tag: matching name closes the target element.
        if (tag.size() > 1 && tag.compare(1, std::string::npos, tagName_) == 0) {
          depth_--;
          if (depth_ == 0) {
            pos = tagEnd + 1;
            capturePos_ = pos;
            flushText(textChunk);
            finish();
            return true;
          }
        }
      } else if (!tag.empty() && tag.back() != '/') {
        // Opening tag: same-name tags nest (li, div, section, td...).
        size_t nameEnd = tag.find_first_of(" \t\r\n");
        const std::string name = nameEnd == std::string::npos ? tag : tag.substr(0, nameEnd);
        if (name == tagName_ && !isVoidElementName(name)) {
          depth_++;
        }
      }
      pos = tagEnd + 1;

      if (out_.size() >= maxBytes_) {
        flushText(textChunk);
        finish();
        return true;
      }
    }
    capturePos_ = pos;
    flushText(textChunk);
    if (out_.size() >= maxBytes_) {
      finish();
      return true;
    }
    return false;
  }

  void flushText(const std::string& raw) {
    if (raw.empty()) return;
    decodeEntities(raw, out_);
    if (out_.size() > maxBytes_ + 64) {
      // Overshoot guard while streaming; final clamp happens in finish().
      out_.resize(maxBytes_ + 64);
    }
  }

  void finish() {
    done_ = true;
    // Collapse whitespace runs and trim.
    std::string collapsed;
    collapsed.reserve(out_.size());
    bool inSpace = false;
    for (const char c : out_) {
      if (c == ' ' || c == '\r' || c == '\n' || c == '\t') {
        inSpace = true;
        continue;
      }
      if (inSpace && !collapsed.empty()) collapsed += ' ';
      inSpace = false;
      collapsed += c;
    }
    while (!collapsed.empty() && collapsed.back() == ' ') collapsed.pop_back();
    // Clamp at a UTF-8 boundary.
    if (collapsed.size() > maxBytes_) {
      size_t cut = maxBytes_;
      while (cut > 0 && (static_cast<unsigned char>(collapsed[cut]) & 0xC0) == 0x80) cut--;
      collapsed.resize(cut);
    }
    out_ = std::move(collapsed);
  }

  std::string anchor_;
  size_t maxBytes_;
  std::string buf_;
  std::string out_;
  std::string tagName_;
  enum State : uint8_t { SEEKING, CAPTURING } state_ = SEEKING;
  int depth_ = 0;
  size_t capturePos_ = 0;
  bool done_ = false;
};

}  // namespace

namespace FootnoteTextExtractor {

namespace {
constexpr size_t CACHE_CAPACITY = 12;
std::deque<std::pair<std::string, std::string>> s_cache;
}  // namespace

void clearCache() { s_cache.clear(); }

std::string extract(const Epub& epub, const int currentSpineIndex, const std::string& href, const size_t maxBytes) {
  if (href.empty()) return {};

  std::string filePart;
  std::string anchor;
  const auto hashPos = href.find('#');
  if (hashPos != std::string::npos) {
    anchor = href.substr(hashPos + 1);
    filePart = href.substr(0, hashPos);
  } else {
    return {};  // no anchor — nothing precise to show
  }
  if (anchor.empty()) return {};

  int targetSpine = currentSpineIndex;
  if (!filePart.empty()) {
    targetSpine = epub.resolveHrefToSpineIndex(href);
    if (targetSpine < 0) targetSpine = epub.resolveHrefToSpineIndex(filePart);
    if (targetSpine < 0) {
      LOG_DBG("FNX", "Could not resolve footnote href %s", href.c_str());
      return {};
    }
  }

  const std::string targetItem = epub.getSpineItem(targetSpine).href;
  if (targetItem.empty()) return {};

  AnchorScanner scanner(anchor, maxBytes);
  // Small chunks: bounded allocations, and allowEarlyStop lets the stream
  // reader abandon the rest of a huge notes file once we are done.
  epub.readItemContentsToStream(targetItem, scanner, 512, /*allowEarlyStop=*/true);
  return scanner.result();
}

std::string extractCached(const Epub& epub, const int currentSpineIndex, const std::string& href,
                          const size_t maxBytes) {
  for (auto& entry : s_cache) {
    if (entry.first == href) return entry.second;
  }

  const std::string text = extract(epub, currentSpineIndex, href, maxBytes);
  if (!text.empty()) {
    if (s_cache.size() >= CACHE_CAPACITY) s_cache.pop_back();
    s_cache.emplace_front(href, text);
  }
  return text;
}

}  // namespace FootnoteTextExtractor
