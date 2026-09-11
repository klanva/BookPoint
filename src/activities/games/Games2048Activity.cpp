#include "Games2048Activity.h"

#include <GfxRenderer.h>
#include <I18n.h>

#include <cstdlib>

#include "components/UITheme.h"
#include "fontIds.h"
#include "MappedInputManager.h"

namespace {
constexpr int kBoardPx = 320;               // play area side (centered horizontally)
constexpr int kCell = kBoardPx / 4;         // 80
constexpr int kCellGap = 6;
constexpr int kTopY = 90;
constexpr char kPersistPath[] = "/.crosspoint/2048_best.txt";
}  // namespace

void Games2048Activity::onEnter() {
  Activity::onEnter();
  best = 0;
  HalFile f;
  if (Storage.openFileForRead("2048", kPersistPath, f)) {
    char buf[16] = {};
    const int n = f.read(buf, sizeof(buf) - 1);
    f.close();
    if (n > 0) best = static_cast<uint32_t>(strtoul(buf, nullptr, 10));
  }
  memset(board, 0, sizeof(board));
  score = 0;
  gameOver = false;
  wonShown = false;
  spawnTile();
  spawnTile();
  requestUpdate();
}

bool Games2048Activity::spawnTile() {
  int empty[16][2];
  int count = 0;
  for (int r = 0; r < 4; ++r)
    for (int c = 0; c < 4; ++c)
      if (board[r][c] == 0) {
        empty[count][0] = r;
        empty[count][1] = c;
        count++;
      }
  if (count == 0) return false;
  const int pick = rand() % count;
  board[empty[pick][0]][empty[pick][1]] = (rand() % 10 == 0) ? 4 : 2;
  return true;
}

// Slides one row to the left; returns true when the row changed.
bool Games2048Activity::slideLeft() {
  bool moved = false;
  for (int r = 0; r < 4; ++r) {
    int write = 0;
    for (int c = 0; c < 4; ++c) {
      if (board[r][c] == 0) continue;
      const uint16_t v = board[r][c];
      board[r][c] = 0;
      if (write > 0 && board[r][write - 1] == v) {
        board[r][write - 1] = static_cast<uint16_t>(v * 2);
        score += static_cast<uint32_t>(v) * 2;
        moved = true;
      } else {
        board[r][write++] = v;
        if (write - 1 != c) moved = true;
      }
    }
  }
  return moved;
}

void Games2048Activity::rotateRight() {
  uint16_t tmp[4][4];
  for (int r = 0; r < 4; ++r)
    for (int c = 0; c < 4; ++c) tmp[c][3 - r] = board[r][c];
  memcpy(board, tmp, sizeof(board));
}

bool Games2048Activity::moveUp() {
  rotateRight();
  rotateRight();
  rotateRight();
  const bool moved = slideLeft();
  rotateRight();
  return moved;
}

bool Games2048Activity::canMove() const {
  for (int r = 0; r < 4; ++r)
    for (int c = 0; c < 4; ++c) {
      if (board[r][c] == 0) return true;
      if (c < 3 && board[r][c] == board[r][c + 1]) return true;
      if (r < 3 && board[r][c] == board[r + 1][c]) return true;
    }
  return false;
}

void Games2048Activity::loop() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    // Persist best score.
    HalFile f;
    if (Storage.openFileForWrite("2048", kPersistPath, f)) {
      char buf[16];
      const int len = snprintf(buf, sizeof(buf), "%u", static_cast<unsigned>(best));
      f.write(reinterpret_cast<const uint8_t*>(buf), len);
      f.close();
    }
    finish();
    return;
  }

  if (gameOver) {
    if (mappedInput.wasReleased(MappedInputManager::Button::Confirm) ||
        mappedInput.wasReleased(MappedInputManager::Button::Power)) {
      onEnter();  // restart
    }
    return;
  }

  bool moved = false;
  if (mappedInput.wasReleased(MappedInputManager::Button::Left)) {
    moved = slideLeft();
  } else if (mappedInput.wasReleased(MappedInputManager::Button::Right)) {
    rotateRight();
    rotateRight();
    moved = slideLeft();
    rotateRight();
    rotateRight();
  } else if (mappedInput.wasReleased(MappedInputManager::Button::Up)) {
    moved = moveUp();
  } else if (mappedInput.wasReleased(MappedInputManager::Button::Down)) {
    rotateRight();
    moved = slideLeft();
    rotateRight();
    rotateRight();
    rotateRight();
  } else {
    return;
  }

  if (moved) {
    spawnTile();
    if (!canMove()) gameOver = true;
    requestUpdate();
  }
}

void Games2048Activity::drawCell(const int row, const int col, const int x, const int y, const int size) const {
  renderer.drawRect(x, y, size, size, true);
  const uint16_t v = board[row][col];
  if (v == 0) return;
  char buf[8];
  snprintf(buf, sizeof(buf), "%u", static_cast<unsigned>(v));
  const int digits = strlen(buf);
  const int fontId = digits <= 2 ? UI_12_FONT_ID : UI_10_FONT_ID;
  const int textW = renderer.getTextAdvanceX(fontId, buf, EpdFontFamily::BOLD);
  const int textH = renderer.getLineHeight(fontId);
  renderer.drawText(fontId, x + (size - textW) / 2, y + (size - textH) / 2, buf, true, EpdFontFamily::BOLD);
}

void Games2048Activity::render(RenderLock&&) {
  const int pageWidth = renderer.getScreenWidth();
  const int boardX = (pageWidth - kBoardPx) / 2;

  renderer.clearScreen();
  const auto& metrics = UITheme::getInstance().getMetrics();
  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, "2048");

  char buf[64];
  snprintf(buf, sizeof(buf), "%s: %u   Best: %u", tr(STR_2048_SCORE), static_cast<unsigned>(score),
           static_cast<unsigned>(best));
  renderer.drawCenteredText(UI_10_FONT_ID, metrics.topPadding + metrics.headerHeight + 12, buf, true);

  const int top = kTopY;
  for (int r = 0; r < 4; ++r)
    for (int c = 0; c < 4; ++c)
      drawCell(r, c, boardX + c * kCell + kCellGap / 2, top + r * kCell + kCellGap / 2, kCell - kCellGap);

  if (gameOver) {
    renderer.fillRect(boardX, top + 40, kBoardPx, 80, true);
    renderer.drawCenteredText(UI_12_FONT_ID, top + 55, tr(STR_2048_OVER), false, EpdFontFamily::BOLD);
    renderer.drawCenteredText(UI_10_FONT_ID, top + 80, tr(STR_2048_RESTART), false);
  }

  renderer.displayBuffer();
}
