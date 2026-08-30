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
    if (entity == "&amp;") {
      out += '&';
    } else if (entity == "&lt;") {
      out += '<';
    } else if (entity == "&gt;") {
      out += '>';
    } else if (entity == "&quot;") {
      out += '"';
    } else if (entity == "&apos;") {
      out += '\'';
    } else if (entity == "&nbsp;") {
      out += ' ';
    } else if (entity.size() > 3 && entity[1] == '#') {
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

bool isInlineTagName(const std::string& name) {
  return name == "a" || name == "span" || name == "b" || name == "i" || name == "em" || name == "strong" ||
         name == "sup" || name == "sub" || name == "small" || name == "u" || name == "s" || name == "code" ||
         name == "cite" || name == "q" || name == "font" || name == "big";
}

// Streams the target item and captures the readable text around the element
// whose opening tag carries id="<anchor>".
//
// Real-world notes come in two shapes and both are handled:
//   <a id="n14">text</a>                 (text inside the anchor)
//   <p><a id="n14"></a> text ... </p>    (empty anchor, text follows it)
// Capture runs from the anchor tag until the first block-level tag (paragraph,
// table cell, heading, div), so one note never bleeds into the next. Inline
// tags (b, i, sup, span, ...) are stripped but their text is kept.
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
  static constexpr size_t SEARCH_TAIL = 1024;  // bytes kept while hunting the anchor

  void process() {
    if (state_ == SEEKING) {
      const size_t found = findAnchorStart();
      if (found == std::string::npos) {
        if (buf_.size() > SEARCH_TAIL) buf_.erase(0, buf_.size() - SEARCH_TAIL);
        return;
      }
      const size_t tagEnd = buf_.find('>', found);
      if (tagEnd == std::string::npos) {
        buf_.erase(0, found);  // opening tag still streaming in
        return;
      }
      state_ = CAPTURING;
      pos_ = tagEnd + 1;
    }

    if (!captureFrom(pos_)) {
      buf_.erase(0, pos_);
      pos_ = 0;
      return;
    }
    buf_.clear();
    buf_.shrink_to_fit();
  }

  // Locates the opening tag whose id/xml:id attribute matches the anchor.
  // Returns the offset of that tag's '<', or npos.
  size_t findAnchorStart() const {
    static const char* patterns[] = {"id=\"", "id='", "xml:id=\"", "xml:id='"};
    for (const char* pat : patterns) {
      const std::string p = pat;
      size_t pos = 0;
      while ((pos = buf_.find(p, pos)) != std::string::npos) {
        const bool boundaryOk =
            pos == 0 || isspace(static_cast<unsigned char>(buf_[pos - 1])) || buf_[pos - 1] == '<';
        const size_t valueStart = pos + p.size();
        if (!boundaryOk || valueStart >= buf_.size()) {
          pos = valueStart;
          continue;
        }
        const char quote = buf_[valueStart];
        if (quote != '"' && quote != '\'') {
          pos = valueStart;
          continue;
        }
        const size_t close = buf_.find(quote, valueStart + 1);
        if (close == std::string::npos) {
          break;  // value still streaming in; retry with more data
        }
        if (buf_.compare(valueStart, close - valueStart, anchor_) == 0) {
          const size_t tagStart = buf_.rfind('<', pos);
          if (tagStart != std::string::npos) return tagStart;
        }
        pos = valueStart;
      }
    }
    return std::string::npos;
  }

  // Consumes text from pos until a block boundary or the byte cap.
  // Returns true when finished (done_ set), false when more data is needed.
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

      const size_t tagEnd = buf_.find('>', lt);
      if (tagEnd == std::string::npos) {
        pos = lt;  // tag still streaming in
        break;
      }
      const std::string tag = buf_.substr(lt + 1, tagEnd - lt - 1);
      pos = tagEnd + 1;

      if (tag.empty()) continue;
      if (tag[0] == '!' || tag[0] == '?') continue;  // comment or declaration

      const bool closing = tag[0] == '/';
      const size_t nameOff = closing ? 1 : 0;  // skip the '/' of a close tag
      size_t nameEnd = tag.find_first_of(" \t\r\n/", nameOff);
      const std::string name =
          tag.substr(nameOff, nameEnd == std::string::npos ? std::string::npos : nameEnd - nameOff);

      if (!isInlineTagName(name)) {
        // First block-level tag after the anchor: the note ends here. The
        // anchor's own container close (</p> and friends) lands here too.
        flushText(textChunk);
        finish();
        return true;
      }

      if (out_.size() >= maxBytes_) {
        flushText(textChunk);
        finish();
        return true;
      }
    }
    pos_ = pos;
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
      out_.resize(maxBytes_ + 64);  // overshoot guard; final clamp in finish()
    }
  }

  void finish() {
    done_ = true;
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
  enum State : uint8_t { SEEKING, CAPTURING } state_ = SEEKING;
  size_t pos_ = 0;
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
    return {};  // no anchor, nothing precise to show
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

  // Small chunks keep allocations bounded; allowEarlyStop lets the stream
  // reader abandon the rest of a huge notes file once we are done.
  AnchorScanner scanner(anchor, maxBytes);
  epub.readItemContentsToStream(targetItem, scanner, 512, /*allowEarlyStop=*/true);
  if (!scanner.result().empty()) return scanner.result();

  // Some books percent-encode anchors inside hrefs. Retry with the decoded
  // form when it differs.
  std::string decoded;
  for (size_t i = 0; i < anchor.size(); ++i) {
    if (anchor[i] == '%' && i + 2 < anchor.size() && isxdigit(static_cast<unsigned char>(anchor[i + 1])) &&
        isxdigit(static_cast<unsigned char>(anchor[i + 2]))) {
      const auto hexVal = [](const char c) {
        return c <= '9' ? c - '0' : (c | 0x20) - 'a' + 10;
      };
      decoded += static_cast<char>(hexVal(anchor[i + 1]) * 16 + hexVal(anchor[i + 2]));
      i += 2;
    } else {
      decoded += anchor[i];
    }
  }
  if (decoded == anchor || decoded.empty()) return scanner.result();
  AnchorScanner retry(decoded, maxBytes);
  epub.readItemContentsToStream(targetItem, retry, 512, /*allowEarlyStop=*/true);
  return retry.result();
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
