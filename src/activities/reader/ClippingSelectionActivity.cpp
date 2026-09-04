#include "ClippingSelectionActivity.h"

#include <CrossPointSettings.h>
#include <FontCacheManager.h>
#include <GfxRenderer.h>
#include <HalDisplay.h>
#include <I18n.h>
#include <Logging.h>

#include <algorithm>
#include <climits>
#include <cstdlib>
#include <utility>

#include "MappedInputManager.h"
#include "ReaderUtils.h"
#include "activities/ActivityResult.h"
#include "components/UITheme.h"
#include "fontIds.h"

ClippingSelectionActivity::ClippingSelectionActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                                                     Section& section, const int fontId,
                                                     const int marginLeft, const int marginTop)
    : Activity("ClippingSelection", renderer, mappedInput),
      section_(section),
      fontId_(fontId),
      marginLeft_(marginLeft),
      marginTop_(marginTop) {}

void ClippingSelectionActivity::onEnter() {
  Activity::onEnter();
  inputArmed_ = false;
  quietInputFrames_ = 0;

  originalPage_ = std::clamp(section_.currentPage, 0, std::max(0, static_cast<int>(section_.pageCount) - 1));
  currentPage_ = originalPage_;
  loadPage(currentPage_);
  cursor_ = nearestCenterWord();
  anchorPage_ = currentPage_;
  anchorWord_ = cursor_;
  requestUpdate(true);
}

void ClippingSelectionActivity::onExit() {
  section_.currentPage = originalPage_;
  Activity::onExit();
}

bool ClippingSelectionActivity::loadPage(const int pageNumber, const bool selectLastWord) {
  if (pageNumber < 0 || pageNumber >= static_cast<int>(section_.pageCount)) return false;
  const int previousSectionPage = section_.currentPage;
  section_.currentPage = pageNumber;
  auto next = section_.loadPage(pageNumber);
  section_.currentPage = previousSectionPage;
  if (!next) return false;

  page_ = std::move(next);
  words_ = ClippingUtils::collectWords(*page_);
  currentPage_ = pageNumber;
  if (words_.empty()) {
    cursor_ = 0;
  } else {
    cursor_ = selectLastWord ? words_.size() - 1 : 0;
  }
  return true;
}

size_t ClippingSelectionActivity::nearestCenterWord() const {
  if (!page_ || words_.empty()) return 0;
  const int centerX = renderer.getScreenWidth() / 2;
  const int centerY = renderer.getScreenHeight() / 2;
  long best = LONG_MAX;
  size_t bestIndex = 0;

  for (size_t i = 0; i < words_.size(); ++i) {
    const auto& ref = words_[i];
    const auto& line = static_cast<const PageLine&>(*page_->elements[ref.elementIndex]);
    const auto& block = line.getBlock();
    if (!block || ref.wordIndex >= block->wordCount()) continue;

    const int x = marginLeft_ + line.xPos + block->wordXpos(ref.wordIndex);
    const int y = marginTop_ + line.yPos;
    const long dx = x - centerX;
    const long dy = y - centerY;
    const long dist = dx * dx + dy * dy;
    if (dist < best) {
      best = dist;
      bestIndex = i;
    }
  }
  return bestIndex;
}

void ClippingSelectionActivity::orderedRange(int& startPage, size_t& startWord, int& endPage, size_t& endWord) const {
  if (anchorPage_ < currentPage_ || (anchorPage_ == currentPage_ && anchorWord_ <= cursor_)) {
    startPage = anchorPage_;
    startWord = anchorWord_;
    endPage = currentPage_;
    endWord = cursor_;
  } else {
    startPage = currentPage_;
    startWord = cursor_;
    endPage = anchorPage_;
    endWord = anchorWord_;
  }
}

void ClippingSelectionActivity::moveHorizontal(const int delta) {
  if (words_.empty()) return;
  if (delta < 0) {
    if (cursor_ > 0) {
      cursor_--;
      requestUpdate();
    } else if (currentPage_ > 0 && loadPage(currentPage_ - 1, true)) {
      requestUpdate();
    }
  } else if (delta > 0) {
    if (cursor_ + 1 < words_.size()) {
      cursor_++;
      requestUpdate();
    } else if (currentPage_ + 1 < static_cast<int>(section_.pageCount) && loadPage(currentPage_ + 1, false)) {
      requestUpdate();
    }
  }
}

void ClippingSelectionActivity::moveVertical(const int direction) {
  if (!page_ || words_.empty()) return;

  const auto& currentRef = words_[cursor_];
  const auto& currentLine = static_cast<const PageLine&>(*page_->elements[currentRef.elementIndex]);
  const auto& currentBlock = currentLine.getBlock();
  if (!currentBlock || currentRef.wordIndex >= currentBlock->wordCount()) return;
  const int currentX = marginLeft_ + currentLine.xPos + currentBlock->wordXpos(currentRef.wordIndex);

  int bestIndex = -1;
  int bestYDiff = INT_MAX;
  int bestXDiff = INT_MAX;

  for (size_t i = 0; i < words_.size(); ++i) {
    const auto& ref = words_[i];
    if (ref.elementIndex == currentRef.elementIndex) continue;
    const auto& line = static_cast<const PageLine&>(*page_->elements[ref.elementIndex]);
    const int yDiff = direction < 0 ? (currentLine.yPos - line.yPos) : (line.yPos - currentLine.yPos);
    if (yDiff <= 0) continue;

    const auto& block = line.getBlock();
    if (!block || ref.wordIndex >= block->wordCount()) continue;
    const int x = marginLeft_ + line.xPos + block->wordXpos(ref.wordIndex);
    const int xDiff = std::abs(x - currentX);

    if (yDiff < bestYDiff || (yDiff == bestYDiff && xDiff < bestXDiff)) {
      bestYDiff = yDiff;
      bestXDiff = xDiff;
      bestIndex = static_cast<int>(i);
    }
  }

  if (bestIndex >= 0) {
    cursor_ = static_cast<size_t>(bestIndex);
    requestUpdate();
    return;
  }

  jumpPage(direction < 0 ? -1 : 1);
}

void ClippingSelectionActivity::jumpPage(const int direction) {
  const int targetPage = currentPage_ + direction;
  if (targetPage < 0 || targetPage >= static_cast<int>(section_.pageCount)) return;
  if (loadPage(targetPage, direction < 0)) {
    requestUpdate();
  }
}

void ClippingSelectionActivity::saveSelection() {
  if (!page_ || words_.empty()) return;

  int startPage = 0;
  int endPage = 0;
  size_t startWord = 0;
  size_t endWord = 0;
  orderedRange(startPage, startWord, endPage, endWord);

  ClippingSelectionResult result;
  result.startPageNumber = static_cast<uint16_t>(startPage);
  result.endPageNumber = static_cast<uint16_t>(endPage);
  result.startWordIndex = static_cast<uint16_t>(std::min<size_t>(startWord, UINT16_MAX));
  result.endWordIndex = static_cast<uint16_t>(std::min<size_t>(endWord, UINT16_MAX));

  std::string text;
  text.reserve(512);
  for (int pageNumber = startPage; pageNumber <= endPage && text.size() < 512; ++pageNumber) {
    const int previousSectionPage = section_.currentPage;
    section_.currentPage = pageNumber;
    auto page = section_.loadPage(pageNumber);
    section_.currentPage = previousSectionPage;
    if (!page) continue;
    const auto pageWords = ClippingUtils::collectWords(*page);
    if (pageWords.empty()) continue;

    const size_t first = pageNumber == startPage ? startWord : 0;
    const size_t last = pageNumber == endPage ? endWord : pageWords.size() - 1;
    char fragment[513] = {};
    if (ClippingUtils::extractText(*page, pageWords, std::min(first, pageWords.size() - 1),
                                   std::min(last, pageWords.size() - 1), fragment, sizeof(fragment))) {
      if (!text.empty() && text.size() < 512) text.push_back('\n');
      const size_t remaining = 512 - std::min<size_t>(text.size(), 512);
      if (remaining > 0) text.append(fragment, std::min(strlen(fragment), remaining));
    }
  }

  if (text.empty()) return;
  result.text = std::move(text);
  setResult(std::move(result));
  finish();
}

void ClippingSelectionActivity::loop() {
  if (!inputArmed_) {
    const bool anyHeld =
        mappedInput.isPressed(MappedInputManager::Button::Back) ||
        mappedInput.isPressed(MappedInputManager::Button::Confirm) ||
        mappedInput.isPressed(MappedInputManager::Button::Power) ||
        mappedInput.isPressed(MappedInputManager::Button::Left) ||
        mappedInput.isPressed(MappedInputManager::Button::Right) ||
        mappedInput.isPressed(MappedInputManager::Button::Up) ||
        mappedInput.isPressed(MappedInputManager::Button::Down) ||
        mappedInput.isPressed(MappedInputManager::Button::PageBack) ||
        mappedInput.isPressed(MappedInputManager::Button::PageForward);

    if (anyHeld || mappedInput.wasAnyPressed() || mappedInput.wasAnyReleased()) {
      quietInputFrames_ = 0;
      return;
    }
    if (++quietInputFrames_ < 1) return;
    inputArmed_ = true;
    return;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    ActivityResult result;
    result.isCancelled = true;
    setResult(std::move(result));
    finish();
    return;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm) ||
      mappedInput.wasReleased(MappedInputManager::Button::Power)) {
    if (words_.empty()) return;
    if (!selecting_) {
      selecting_ = true;
      anchorPage_ = currentPage_;
      anchorWord_ = cursor_;
      requestUpdate();
    } else {
      saveSelection();
    }
    return;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Left)) {
    if (mappedInput.getHeldTime() >= PAGE_JUMP_HOLD_MS) jumpPage(-1);
    else moveHorizontal(-1);
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Right)) {
    if (mappedInput.getHeldTime() >= PAGE_JUMP_HOLD_MS) jumpPage(1);
    else moveHorizontal(1);
    return;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Up)) {
    const int steps = mappedInput.getHeldTime() >= PAGE_JUMP_HOLD_MS ? FAST_VERTICAL_STEPS : 1;
    for (int i = 0; i < steps; ++i) moveVertical(-1);
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Down)) {
    const int steps = mappedInput.getHeldTime() >= PAGE_JUMP_HOLD_MS ? FAST_VERTICAL_STEPS : 1;
    for (int i = 0; i < steps; ++i) moveVertical(1);
    return;
  }
}

void ClippingSelectionActivity::render(RenderLock&&) {
  const bool darkMode = (SETTINGS.screenInverted != 0);
  const bool foregroundBlack = !darkMode;

  auto drawSelectionScreen = [this, darkMode, foregroundBlack]() {
    renderer.clearScreen(darkMode ? 0x00 : 0xFF);

    if (page_) {
      page_->render(renderer, fontId_, marginLeft_, marginTop_);
    }

    if (page_ && !words_.empty()) {
      if (selecting_) {
        int startPage = 0;
        int endPage = 0;
        size_t startWord = 0;
        size_t endWord = 0;
        orderedRange(startPage, startWord, endPage, endWord);

        if (currentPage_ >= startPage && currentPage_ <= endPage) {
          size_t first = 0;
          size_t last = words_.size() - 1;
          if (currentPage_ == startPage) first = std::min(startWord, words_.size() - 1);
          if (currentPage_ == endPage) last = std::min(endWord, words_.size() - 1);
          if (first > last) std::swap(first, last);

          for (size_t i = first; i <= last; ++i) {
            ClippingUtils::drawWordHighlight(renderer, *page_, words_[i], fontId_, marginLeft_, marginTop_,
                                             foregroundBlack);
          }
        }
      } else {
        ClippingUtils::drawWordHighlight(renderer, *page_, words_[cursor_], fontId_, marginLeft_, marginTop_,
                                         foregroundBlack, true);
      }
    }

    const auto& metrics = UITheme::getInstance().getMetrics();
    const int hintsTop = renderer.getScreenHeight() - metrics.buttonHintsHeight;

    renderer.fillRect(0, hintsTop, renderer.getScreenWidth(), metrics.buttonHintsHeight, darkMode);

    char pageLabel[32];
    snprintf(pageLabel, sizeof(pageLabel), "%d/%u", currentPage_ + 1, static_cast<unsigned>(section_.pageCount));
    const int labelY = std::max(0, hintsTop - renderer.getLineHeight(UI_10_FONT_ID) - 4);
    renderer.fillRect(0, labelY - 2, renderer.getScreenWidth(), renderer.getLineHeight(UI_10_FONT_ID) + 4, darkMode);
    renderer.drawCenteredText(UI_10_FONT_ID, labelY, pageLabel, foregroundBlack);

    const auto labels =
        mappedInput.mapLabels(tr(STR_BACK), selecting_ ? tr(STR_CLIPPING_DONE) : tr(STR_CLIPPING_START),
                              tr(STR_DIR_LEFT), tr(STR_DIR_RIGHT));
    GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  };

  if (page_) {
    if (auto* fontCache = renderer.getFontCacheManager()) {
      auto prewarmScope = fontCache->createPrewarmScope();
      page_->render(renderer, fontId_, marginLeft_, marginTop_);
      prewarmScope.endScanAndPrewarm();
      drawSelectionScreen();
      renderer.displayBuffer(HalDisplay::FAST_REFRESH);
      return;
    }
  }

  drawSelectionScreen();
  renderer.displayBuffer(HalDisplay::FAST_REFRESH);
}
