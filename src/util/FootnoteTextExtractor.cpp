#include "FootnoteTextExtractor.h"

#include <Logging.h>

#include <cctype>
#include <cstring>
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

bool isVoidTagName(const std::string& name) {
  return name == "br" || name == "hr" || name == "img" || name == "input" || name == "meta" || name == "link" ||
         name == "col" || name == "area" || name == "base" || name == "embed" || name == "source" ||
         name == "track" || name == "wbr";
}

// Streams the target item and captures everything inside the element whose
// opening tag carries a matching id/xml:id/name attribute.
//
// Capture runs until that element's own closing tag (same-name depth
// counting), so whatever the surrounding markup looks like, the whole note
// is taken and nothing beyond it. Books structure the note differently and
// all of these end up correct:
//   <a id="n14">text</a>                                  (text in the anchor)
//   <p><a id="n14"></a> text ...</p>                      (empty anchor first)
//   <span id="id17"><div>14</div><p>text</p></span>       (royallib style:
//     a leading block that holds just the note number, stripped later by the
//     caller when it matches the footnote label)
//
// Chunk boundaries are bridged internally; capture stops at the byte cap.
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
      // Extract the anchor element's name and find where its opening tag ends.
      const size_t nameStart = found + 1;
      size_t nameEnd = nameStart;
      while (nameEnd < buf_.size() && !isspace(static_cast<unsigned char>(buf_[nameEnd])) &&
             buf_[nameEnd] != '>' && buf_[nameEnd] != '/') {
        nameEnd++;
      }
      if (nameEnd >= buf_.size()) {
        buf_.erase(0, found);  // tag still streaming in
        return;
      }
      tagName_ = buf_.substr(nameStart, nameEnd - nameStart);
      const size_t tagEnd = buf_.find('>', nameEnd);
      if (tagEnd == std::string::npos) {
        buf_.erase(0, found);  // opening tag still streaming in
        return;
      }
      const bool selfClosing = buf_[tagEnd - 1] == '/';
      if (selfClosing || isVoidTagName(tagName_)) {
        // No element body (e.g. <a id="n14"/>): the note lives outside this
        // tag. depth 0 disables the close-tag check, so capture runs until
        // the byte cap while skipping over same-name tags.
        state_ = CAPTURING;
        depth_ = 0;
        bodyOnly_ = true;
      } else {
        state_ = CAPTURING;
        depth_ = 1;
      }
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

  // Locates the opening tag whose id/xml:id/name attribute matches the
  // anchor. Returns the offset of that tag's '<', or npos.
  //
  // Every pattern already ends with its closing quote character ("id=\"" etc),
  // so valueStart is the first character of the value and the value runs up to
  // the next occurrence of that same quote.
  size_t findAnchorStart() const {
    static const char* patterns[] = {"id=\"", "id='", "xml:id=\"", "xml:id='", "name=\"", "name='"};
    for (const char* pat : patterns) {
      const std::string p = pat;
      const char quote = p.back();  // the closing quote of this pattern
      size_t pos = 0;
      while ((pos = buf_.find(p, pos)) != std::string::npos) {
        const bool boundaryOk =
            pos == 0 || isspace(static_cast<unsigned char>(buf_[pos - 1])) || buf_[pos - 1] == '<';
        const size_t valueStart = pos + p.size();
        if (!boundaryOk || valueStart >= buf_.size()) {
          pos = valueStart;
          continue;
        }
        const size_t close = buf_.find(quote, valueStart);
        if (close == std::string::npos) {
          break;  // value still streaming in; retry with more data
        }
        const bool match = close - valueStart == anchor_.size() &&
                           buf_.compare(valueStart, anchor_.size(), anchor_) == 0;
        if (match) {
          const size_t tagStart = buf_.rfind('<', pos);
          if (tagStart != std::string::npos) return tagStart;
        }
        pos = valueStart;
      }
    }
    return std::string::npos;
  }

  // Consumes text from pos until the anchor element closes or the byte cap
  // is reached. Returns true when finished (done_ set), false when more
  // data is needed.
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

      // Notes often end with a "back to the text" link. Its label is UI
      // chrome, not note content: when a link href contains "back", drop the
      // link text.
      if (strcasecmp(name.c_str(), "a") == 0) {
        if (!closing) {
          if (!skipAnchorText_) {
            std::string tagLow = tag;
            for (char& c : tagLow) {
              if (c >= 'A' && c <= 'Z') c += 32;
            }
            if (tagLow.find("href") != std::string::npos && tagLow.find("back") != std::string::npos) {
              flushText(textChunk);
              textChunk.clear();
              skipAnchorText_ = true;
            }
          }
        } else if (skipAnchorText_) {
          textChunk.clear();
          skipAnchorText_ = false;
          continue;
        }
      }

      if (!bodyOnly_ && !name.empty() && strcasecmp(name.c_str(), tagName_.c_str()) == 0) {
        if (closing) {
          depth_--;
          if (depth_ <= 0) {
            if (!skipAnchorText_) flushText(textChunk);
            finish();
            return true;
          }
        } else if (tag.back() != '/' && !isVoidTagName(name)) {
          depth_++;
        }
      }

      if (out_.size() >= maxBytes_) {
        flushText(textChunk);
        finish();
        return true;
      }
    }
    pos_ = pos;
    if (!skipAnchorText_) flushText(textChunk);
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
    // Some books put the back-link label in plain text without an anchor.
    // "Вернуться" and "назад" in UTF-8, spelled out as bytes to keep the
    // source encoding-agnostic.
    static const uint8_t kBackWords[][20] = {
        {0xD0, 0x92, 0xD0, 0xB5, 0xD1, 0x80, 0xD0, 0xBD, 0xD1, 0x83,
         0xD1, 0x82, 0xD1, 0x8C, 0xD1, 0x81, 0xD1, 0x8F},              // Вернуться
        {0xD0, 0xBD, 0xD0, 0xB0, 0xD0, 0xB7, 0xD0, 0xB0, 0xD0, 0xB4},  // назад
    };
    for (const auto& word : kBackWords) {
      size_t wlen = 0;
      while (wlen < sizeof(word) && word[wlen] != 0) wlen++;
      if (wlen == 0 || collapsed.size() <= wlen) continue;
      const char* w = reinterpret_cast<const char*>(word);
      if (collapsed.compare(collapsed.size() - wlen, wlen, w) != 0) continue;
      size_t cut = collapsed.size() - wlen;
      while (cut > 0 && (static_cast<unsigned char>(collapsed[cut - 1]) & 0xC0) == 0x80) cut--;
      while (cut > 0 && collapsed[cut - 1] == ' ') cut--;
      collapsed.resize(cut);
      break;
    }
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
  bool bodyOnly_ = false;
  bool skipAnchorText_ = false;  // inside a "back to text" link: drop its label
  size_t pos_ = 0;
  bool done_ = false;
};

// Streams a chapter item and hunts for unlinked footnote markers like >[4] or >4.
class UnlinkedNoteScanner : public Print {
 public:
  explicit UnlinkedNoteScanner(std::string digits, const size_t maxBytes)
      : digits_(std::move(digits)), maxBytes_(maxBytes) {}

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
  std::string digits_;
  size_t maxBytes_;
  std::string buf_;
  std::string out_;
  bool capturing_ = false;
  bool done_ = false;

  void process() {
    if (!capturing_) {
      size_t found = std::string::npos;
      size_t afterMatch = 0;

      for (size_t i = 0; i < buf_.size(); ++i) {
        if (buf_[i] != '>') continue;
        size_t p = i + 1;
        while (p < buf_.size() && (buf_[p] == ' ' || buf_[p] == '\t' || buf_[p] == '\r' || buf_[p] == '\n')) p++;
        if (p >= buf_.size()) break;

        bool hasBracket = false;
        if (buf_[p] == '[') {
          hasBracket = true;
          p++;
          while (p < buf_.size() && (buf_[p] == ' ' || buf_[p] == '\t')) p++;
        }

        if (p + digits_.size() <= buf_.size() && buf_.compare(p, digits_.size(), digits_) == 0) {
          size_t endDig = p + digits_.size();
          if (hasBracket) {
            while (endDig < buf_.size() && (buf_[endDig] == ' ' || buf_[endDig] == '\t')) endDig++;
            if (endDig < buf_.size() && buf_[endDig] == ']') {
              endDig++;
              found = i;
              afterMatch = endDig;
              break;
            }
          } else {
            if (endDig < buf_.size() && (buf_[endDig] == '.' || buf_[endDig] == ')' ||
                                         isspace(static_cast<unsigned char>(buf_[endDig])))) {
              found = i;
              afterMatch = endDig;
              break;
            }
          }
        }
      }

      if (found == std::string::npos) {
        if (buf_.size() > 512) buf_.erase(0, buf_.size() - 512);
        return;
      }

      capturing_ = true;
      buf_.erase(0, afterMatch);
    }

    if (capturing_) {
      size_t pos = 0;
      while (pos < buf_.size()) {
        if (buf_[pos] == '<') {
          size_t tagEnd = buf_.find('>', pos);
          if (tagEnd == std::string::npos) {
            buf_.erase(0, pos);
            return;
          }
          std::string tag = buf_.substr(pos + 1, tagEnd - pos - 1);
          if (strncasecmp(tag.c_str(), "/p", 2) == 0 || strncasecmp(tag.c_str(), "/div", 4) == 0 ||
              strncasecmp(tag.c_str(), "/li", 3) == 0 || strncasecmp(tag.c_str(), "p ", 2) == 0 ||
              strncasecmp(tag.c_str(), "p>", 2) == 0 || strncasecmp(tag.c_str(), "div ", 4) == 0) {
            done_ = true;
            buf_.clear();
            return;
          }
          pos = tagEnd + 1;
        } else {
          char c = buf_[pos++];
          if (isspace(static_cast<unsigned char>(c))) {
            if (!out_.empty() && out_.back() != ' ') out_ += ' ';
          } else {
            out_ += c;
          }
          if (out_.size() >= maxBytes_) {
            done_ = true;
            buf_.clear();
            return;
          }
        }
      }
      buf_.clear();
    }
  }
};

// Strips a leading note number that repeats the reference label, e.g.
// label "[14]" drops the "14" from "14 Вист — карточная игра.".
std::string stripLeadingLabel(const std::string& text, const char* label) {
  if (label == nullptr) return text;
  std::string digits;
  for (const char* p = label; *p != '\0'; ++p) {
    if (*p >= '0' && *p <= '9') digits += *p;
  }
  if (digits.empty()) return text;

  size_t pos = 0;
  while (pos < text.size() && isspace(static_cast<unsigned char>(text[pos]))) pos++;
  const size_t digitStart = pos;
  while (pos < text.size() && text[pos] >= '0' && text[pos] <= '9') pos++;
  if (pos == digitStart) return text;  // no leading number
  if (text.compare(digitStart, pos - digitStart, digits) != 0) return text;
  // Optional separators right after the number: ". )" "- " etc.
  while (pos < text.size() &&
         (text[pos] == '.' || text[pos] == ')' || text[pos] == '-' || text[pos] == ':' ||
          isspace(static_cast<unsigned char>(text[pos])))) {
    pos++;
  }
  return text.substr(pos);
}

}  // namespace

namespace FootnoteTextExtractor {

namespace {
constexpr size_t CACHE_CAPACITY = 12;
std::deque<std::pair<std::string, std::string>> s_cache;
}  // namespace

void clearCache() { s_cache.clear(); }

std::string extract(const Epub& epub, const int currentSpineIndex, const std::string& href, const size_t maxBytes,
                    const char* label) {
  if (href.empty()) return {};

  if (href.rfind("!inline:", 0) == 0) {
    return href.substr(8);
  }

  std::string unlinkedDigits;
  if (href.rfind("#unlinked_", 0) == 0) {
    unlinkedDigits = href.substr(10);
  } else if (label != nullptr) {
    for (const char* p = label; *p != '\0'; ++p) {
      if (*p >= '0' && *p <= '9') unlinkedDigits += *p;
    }
  }

  std::string filePart;
  std::string anchor;
  const auto hashPos = href.find('#');
  if (hashPos != std::string::npos) {
    anchor = href.substr(hashPos + 1);
    filePart = href.substr(0, hashPos);
  }

  // 1. Standard EPUB anchor scan
  if (!anchor.empty() && anchor.rfind("unlinked_", 0) != 0) {
    int targetSpine = currentSpineIndex;
    if (!filePart.empty()) {
      targetSpine = epub.resolveHrefToSpineIndex(href);
      if (targetSpine < 0) targetSpine = epub.resolveHrefToSpineIndex(filePart);
    }
    if (targetSpine >= 0) {
      const std::string targetItem = epub.getSpineItem(targetSpine).href;
      if (!targetItem.empty()) {
        AnchorScanner scanner(anchor, maxBytes);
        epub.readItemContentsToStream(targetItem, scanner, 512, /*allowEarlyStop=*/true);
        if (!scanner.result().empty()) {
          return stripLeadingLabel(scanner.result(), label);
        }
      }
    }
  }

  // 2. Unlinked footnote search by number in text
  if (!unlinkedDigits.empty()) {
    if (currentSpineIndex >= 0 && currentSpineIndex < epub.getSpineItemsCount()) {
      UnlinkedNoteScanner curScanner(unlinkedDigits, maxBytes);
      epub.readItemContentsToStream(epub.getSpineItem(currentSpineIndex).href, curScanner, 512, /*allowEarlyStop=*/true);
      if (curScanner.done() && !curScanner.result().empty()) {
        std::string decoded;
        decodeEntities(curScanner.result(), decoded);
        return stripLeadingLabel(decoded, label);
      }
    }
    const int totalSpine = epub.getSpineItemsCount();
    for (int i = totalSpine - 1; i >= 0; --i) {
      if (i == currentSpineIndex) continue;
      UnlinkedNoteScanner scanner(unlinkedDigits, maxBytes);
      epub.readItemContentsToStream(epub.getSpineItem(i).href, scanner, 512, /*allowEarlyStop=*/true);
      if (scanner.done() && !scanner.result().empty()) {
        std::string decoded;
        decodeEntities(scanner.result(), decoded);
        return stripLeadingLabel(decoded, label);
      }
    }
  }

  return {};
}

std::string extractCached(const Epub& epub, const int currentSpineIndex, const std::string& href,
                          const size_t maxBytes, const char* label) {
  for (auto& entry : s_cache) {
    if (entry.first == href) return entry.second;
  }

  const std::string text = extract(epub, currentSpineIndex, href, maxBytes, label);
  if (!text.empty()) {
    if (s_cache.size() >= CACHE_CAPACITY) s_cache.pop_back();
    s_cache.emplace_front(href, text);
  }
  return text;
}

}  // namespace FootnoteTextExtractor
