#include "GamesActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>

#include <memory>

#include "ChessActivity.h"
#include "MinesweeperActivity.h"
#include "Games2048Activity.h"
#include "MappedInputManager.h"
#include "components/UITheme.h"
#include "fontIds.h"

const char* GamesActivity::gameName(const int index) {
  switch (index) {
    case 0: return "2048";
    case 1: return tr(STR_GAME_MINESWEEPER);
    case 2: return tr(STR_GAME_CHESS);
    default: return "";
  }
}

void GamesActivity::onEnter() {
  Activity::onEnter();
  selected_ = 0;
  requestUpdate();
}

void GamesActivity::loop() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Back) || mappedInput.wasBackGesture()) {
    finish();
    return;
  }

  auto launchGame = [this](int index) {
    switch (index) {
      case 0:
        startActivityForResult(std::make_unique<Games2048Activity>(renderer, mappedInput),
                               [this](const ActivityResult&) { requestUpdate(); });
        break;
      case 1:
        startActivityForResult(std::make_unique<MinesweeperActivity>(renderer, mappedInput),
                               [this](const ActivityResult&) { requestUpdate(); });
        break;
      case 2:
        startActivityForResult(std::make_unique<ChessActivity>(renderer, mappedInput),
                               [this](const ActivityResult&) { requestUpdate(); });
        break;
      default:
        break;
    }
  };

  int tx = 0, ty = 0;
  if (mappedInput.wasScreenTapped(tx, ty)) {
    const auto& metrics = UITheme::getInstance().getMetrics();
    const int hintsTop = renderer.getScreenHeight() - metrics.buttonHintsHeight;
    if (ty >= hintsTop) {
      if (tx < renderer.getScreenWidth() / 2) {
        finish();
      } else {
        launchGame(selected_);
      }
      return;
    }

    const int startY = metrics.topPadding + metrics.headerHeight + 20;
    const int rowStep = metrics.listRowHeight + 14;
    for (int i = 0; i < GAME_COUNT; ++i) {
      const int ry = startY + i * rowStep;
      if (ty >= ry - 6 && ty < ry + metrics.listRowHeight + 8) {
        selected_ = i;
        launchGame(i);
        return;
      }
    }
  }

  if (mappedInput.wasScreenTouchDown(tx, ty)) {
    const auto& metrics = UITheme::getInstance().getMetrics();
    const int startY = metrics.topPadding + metrics.headerHeight + 20;
    const int rowStep = metrics.listRowHeight + 14;
    for (int i = 0; i < GAME_COUNT; ++i) {
      const int ry = startY + i * rowStep;
      if (ty >= ry - 6 && ty < ry + metrics.listRowHeight + 8 && i != selected_) {
        selected_ = i;
        requestUpdate();
        return;
      }
    }
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Up)) {
    selected_ = (selected_ + GAME_COUNT - 1) % GAME_COUNT;
    requestUpdate();
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Down)) {
    selected_ = (selected_ + 1) % GAME_COUNT;
    requestUpdate();
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm) ||
      mappedInput.wasReleased(MappedInputManager::Button::Power)) {
    launchGame(selected_);
  }
}

void GamesActivity::render(RenderLock&&) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();

  renderer.clearScreen();
  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, tr(STR_GAMES));

  int y = metrics.topPadding + metrics.headerHeight + 20;
  for (int i = 0; i < GAME_COUNT; ++i) {
    const bool selected = i == selected_;
    if (selected) {
      renderer.fillRect(14, y - 6, pageWidth - 28, metrics.listRowHeight + 8, true);
    }
    const bool invert = selected;
    const char* label = gameName(i);
    const int textW = renderer.getTextAdvanceX(UI_12_FONT_ID, label, EpdFontFamily::REGULAR);
    renderer.drawText(UI_12_FONT_ID, pageWidth / 2 - textW / 2, y + 4, label, !invert);
    y += metrics.listRowHeight + 14;
  }

  const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer();
}
