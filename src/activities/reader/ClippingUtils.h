#pragma once

#include <Epub/Page.h>
#include <GfxRenderer.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "ClippingStore.h"

namespace ClippingUtils {

struct WordRef {
  int elementIndex = -1;
  uint16_t wordIndex = 0;
};

inline std::vector<WordRef> collectWords(const Page& page) {
  std::vector<WordRef> words;
  words.reserve(96);
  for (size_t elementIndex = 0; elementIndex < page.elements.size(); ++elementIndex) {
    const auto& element = page.elements[elementIndex];
    if (!element || element->getTag() != TAG_PageLine) continue;
    const auto& line = static_cast<const PageLine&>(*element);
    const auto& block = line.getBlock();
    if (!block) continue;
    const uint16_t count = block->wordCount();
    for (uint16_t wordIndex = 0; wordIndex < count; ++wordIndex) {
      const char* w = block->wordText(wordIndex);
      if (w && *w) {
        words.push_back({static_cast<int>(elementIndex), wordIndex});
      }
    }
  }
  return words;
}

inline bool extractText(const Page& page, const std::vector<WordRef>& words, size_t start, size_t end,
                        char* out, size_t outSize) {
  if (!out || outSize == 0 || words.empty()) return false;
  if (start > end) std::swap(start, end);
  if (end >= words.size()) return false;
  out[0] = '\0';
  size_t used = 0;

  int previousElement = -1;
  for (size_t i = start; i <= end; ++i) {
    const auto& ref = words[i];
    if (ref.elementIndex < 0 || ref.elementIndex >= static_cast<int>(page.elements.size())) continue;
    const auto& line = static_cast<const PageLine&>(*page.elements[ref.elementIndex]);
    const auto& block = line.getBlock();
    if (!block || ref.wordIndex >= block->wordCount()) continue;
    const char* word = block->wordText(ref.wordIndex);
    if (!word || !*word) continue;
    const size_t wordLen = strlen(word);

    const char separator = (used > 0 && previousElement != ref.elementIndex) ? '\n' : ' ';
    if (used > 0) {
      if (used + 1 >= outSize) break;
      out[used++] = separator;
    }
    size_t copy = std::min(wordLen, outSize - used - 1);
    while (copy > 0 && copy < wordLen &&
           (static_cast<uint8_t>(word[copy]) & 0xC0) == 0x80) {
      --copy;
    }
    if (copy == 0) break;
    memcpy(out + used, word, copy);
    used += copy;
    out[used] = '\0';
    previousElement = ref.elementIndex;
    if (used + 1 >= outSize) break;
  }
  return used > 0;
}

inline void drawWordHighlight(GfxRenderer& renderer, const Page& page, const WordRef& ref, int fontId,
                              int marginLeft, int marginTop, bool foregroundBlack, bool cursorOnly = false) {
  if (ref.elementIndex < 0 || ref.elementIndex >= static_cast<int>(page.elements.size())) return;
  const auto& element = page.elements[ref.elementIndex];
  if (!element || element->getTag() != TAG_PageLine) return;
  const auto& line = static_cast<const PageLine&>(*element);
  const auto& block = line.getBlock();
  if (!block || ref.wordIndex >= block->wordCount()) return;

  const char* word = block->wordText(ref.wordIndex);
  if (!word) return;
  const auto style = block->wordStyle(ref.wordIndex);
  const int x = marginLeft + line.xPos + block->wordXpos(ref.wordIndex);
  const int y = marginTop + line.yPos;
  const int width = std::max(4, renderer.getTextAdvanceX(fontId, word, style));
  const int height = std::max(4, renderer.getLineHeight(fontId));

  if (cursorOnly) {
    renderer.drawRect(x - 2, y - 2, width + 4, height + 4, 1, foregroundBlack);
    return;
  }
  renderer.fillRect(x - 2, y - 2, width + 4, height + 4, foregroundBlack);
  renderer.drawText(fontId, x, y, word, !foregroundBlack, style);
}

inline void drawSavedHighlights(GfxRenderer& renderer, const Page& page, const std::vector<Clipping>& clippings,
                                int fontId, int marginLeft, int marginTop, bool foregroundBlack) {
  if (clippings.empty()) return;
  for (const auto& clipping : clippings) {
    if (clipping.text[0] == '\0') continue;
    for (const auto& element : page.elements) {
      if (!element || element->getTag() != TAG_PageLine) continue;
      const auto& line = static_cast<const PageLine&>(*element);
      const auto& block = line.getBlock();
      if (!block) continue;
      const uint16_t count = block->wordCount();
      for (uint16_t w = 0; w < count; ++w) {
        const char* word = block->wordText(w);
        if (!word || !*word || strlen(word) < 3) continue;
        if (strstr(clipping.text, word) != nullptr) {
          const int x = marginLeft + line.xPos + block->wordXpos(w);
          const int y = marginTop + line.yPos;
          const int width = std::max(4, renderer.getTextAdvanceX(fontId, word, block->wordStyle(w)));
          const int height = renderer.getLineHeight(fontId);
          renderer.fillRect(x, y + height - 1, width, 1, foregroundBlack);
        }
      }
    }
  }
}

}  // namespace ClippingUtils
