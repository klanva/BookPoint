#include "ClippingListActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>

#include <algorithm>
#include <cstdio>
#include <utility>
#include <vector>

#include "MappedInputManager.h"
#include "activities/ActivityResult.h"
#include "activities/util/ConfirmationActivity.h"
#include "components/UITheme.h"
#include "fontIds.h"

namespace {
constexpr int ROW_HEIGHT = 72;
constexpr int LIST_GAP = 6;
constexpr unsigned long DELETE_HOLD_MS = 1000;

std::string singleLineSnippet(const char* text) {
  std::string out;
  if (!text) return out;
  out.reserve(160);
  bool pendingSpace = false;
  for (const unsigned char ch : std::string(text)) {
    if (ch == '\r' || ch == '\n' || ch == '\t') {
      pendingSpace = !out.empty();
      continue;
    }
    if (pendingSpace) {
      if (!out.empty() && out.back() != ' ') out.push_back(' ');
      pendingSpace = false;
    }
    out.push_back(static_cast<char>(ch));
  }
  size_t write = 0;
  bool previousSpace = false;
  for (size_t read = 0; read < out.size(); ++read) {
    const bool isSpace = (out[read] == ' ');
    if (isSpace && previousSpace) continue;
    out[write++] = out[read];
    previousSpace = isSpace;
  }
  out.resize(write);
  while (!out.empty() && out.front() == ' ') out.erase(out.begin());
  while (!out.empty() && out.back() == ' ') out.pop_back();
  return out;
}
}  // namespace

void ClippingListActivity::onEnter() {
  Activity::onEnter();
  selectedIndex_ = 0;
  requestUpdate(true);
}

int ClippingListActivity::pageItems() const {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int listStartY = metrics.topPadding + metrics.headerHeight + LIST_GAP;
  const int available = renderer.getScreenHeight() - listStartY - metrics.buttonHintsHeight - 6;
  return std::max(1, available / ROW_HEIGHT);
}

void ClippingListActivity::openDeleteMenu() {
  const auto& items = store_.getClippings();
  if (items.empty() || selectedIndex_ < 0 || selectedIndex_ >= static_cast<int>(items.size())) return;
  const size_t selected = static_cast<size_t>(selectedIndex_);
  const std::string snippet = singleLineSnippet(items[selected].text);

  startActivityForResult(
      std::make_unique<ConfirmationActivity>(renderer, mappedInput, tr(STR_DELETE_CLIPPING), snippet),
      [this, selected](const ActivityResult& result) {
        longPressHandled_ = false;
        if (!result.isCancelled) {
          store_.removeAt(selected);
          const auto size = static_cast<int>(store_.getClippings().size());
          selectedIndex_ = size == 0 ? 0 : std::min(selectedIndex_, size - 1);
        }
        requestUpdate();
      });
}

void ClippingListActivity::loop() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    ActivityResult result;
    result.isCancelled = true;
    setResult(std::move(result));
    finish();
    return;
  }

  const auto& clips = store_.getClippings();
  if (!clips.empty() && !longPressHandled_ && mappedInput.isPressed(MappedInputManager::Button::Confirm) &&
      mappedInput.getHeldTime() >= DELETE_HOLD_MS) {
    longPressHandled_ = true;
    openDeleteMenu();
    return;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    if (longPressHandled_) {
      longPressHandled_ = false;
      return;
    }
    if (!clips.empty() && selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(clips.size())) {
      const auto& clip = clips[static_cast<size_t>(selectedIndex_)];
      setResult(ClippingJumpResult{clip.spineIndex, clip.pageNumber});
      finish();
    }
    return;
  }

  const int total = static_cast<int>(clips.size());
  if (total == 0) return;

  navigator_.onNextRelease([this, total] {
    selectedIndex_ = ButtonNavigator::nextIndex(selectedIndex_, total);
    requestUpdate();
  });
  navigator_.onPreviousRelease([this, total] {
    selectedIndex_ = ButtonNavigator::previousIndex(selectedIndex_, total);
    requestUpdate();
  });
  navigator_.onNextContinuous([this, total] {
    selectedIndex_ = ButtonNavigator::nextIndex(selectedIndex_, total);
    requestUpdate();
  });
  navigator_.onPreviousContinuous([this, total] {
    selectedIndex_ = ButtonNavigator::previousIndex(selectedIndex_, total);
    requestUpdate();
  });
}

void ClippingListActivity::render(RenderLock&&) {
  renderer.clearScreen();
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int listStartY = metrics.topPadding + metrics.headerHeight + LIST_GAP;

  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, renderer.getScreenWidth(), metrics.headerHeight},
                 tr(STR_CLIPPINGS), nullptr);

  const auto& clips = store_.getClippings();
  if (clips.empty()) {
    renderer.drawCenteredText(UI_10_FONT_ID, listStartY + 28, tr(STR_NO_CLIPPINGS));
  } else {
    const int perPage = pageItems();
    const int first = (selectedIndex_ / perPage) * perPage;
    for (int row = 0; row < perPage; ++row) {
      const int index = first + row;
      if (index >= static_cast<int>(clips.size())) break;
      const int y = listStartY + row * ROW_HEIGHT;
      const bool selected = (index == selectedIndex_);
      if (selected) renderer.fillRect(0, y, renderer.getScreenWidth() - 1, ROW_HEIGHT, true);

      const auto& clip = clips[static_cast<size_t>(index)];
      const std::string flattened = singleLineSnippet(clip.text);
      const auto snippet =
          renderer.truncatedText(UI_10_FONT_ID, flattened.c_str(), renderer.getScreenWidth() - 38);
      renderer.drawText(UI_10_FONT_ID, 18, y + 6, snippet.c_str(), !selected);

      char page[32];
      if (clip.endPageNumber > clip.pageNumber) {
        snprintf(page, sizeof(page), "%u-%u", static_cast<unsigned>(clip.pageNumber + 1),
                 static_cast<unsigned>(clip.endPageNumber + 1));
      } else {
        snprintf(page, sizeof(page), "%u", static_cast<unsigned>(clip.pageNumber + 1));
      }
      const int pageW = renderer.getTextWidth(UI_10_FONT_ID, page);
      renderer.drawText(UI_10_FONT_ID, renderer.getScreenWidth() - 18 - pageW, y + 32, page, !selected);

      if (clip.chapterTitle[0] != '\0') {
        const int maxChW = std::max(50, renderer.getScreenWidth() - 18 - pageW - 28);
        const auto chapterText = renderer.truncatedText(UI_10_FONT_ID, clip.chapterTitle, maxChW);
        renderer.drawText(UI_10_FONT_ID, 18, y + 32, chapterText.c_str(), !selected);
      }

      renderer.drawText(UI_10_FONT_ID, 18, y + 52,
                        renderer.truncatedText(UI_10_FONT_ID, tr(STR_CLIPPING_DELETE_HINT), renderer.getScreenWidth() - 36).c_str(),
                        !selected);
    }
  }

  const auto labels = mappedInput.mapLabels(tr(STR_BACK), clips.empty() ? "" : tr(STR_OPEN),
                                            tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer();
}
