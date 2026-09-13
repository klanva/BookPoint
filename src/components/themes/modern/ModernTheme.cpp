#include "ModernTheme.h"

#include <Bitmap.h>
#include <GfxRenderer.h>
#include <HalClock.h>
#include <HalGPIO.h>
#include <HalPowerManager.h>
#include <HalStorage.h>
#include <I18n.h>
#include <WiFi.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "RecentBooksStore.h"
#include "components/UITheme.h"
#include "components/icons/bookmark.h"
#include "components/icons/cover.h"
#include "components/icons/folder.h"
#include "components/icons/library.h"
#include "components/icons/recent.h"
#include "components/icons/search32.h"
#include "components/icons/settings2.h"
#include "components/icons/stats.h"
#include "components/icons/transfer.h"
#include "components/icons/wifi.h"
#include "fontIds.h"
#include "stats/BookReadingStats.h"
#include "stats/GlobalReadingStats.h"
#include "stats/ReadingStatsTypes.h"
#include "stats/ReadingStatsUtils.h"
#include "stats/StatsStore.h"

#if FREEINK_CAP_FRONTLIGHT
#include <HalFrontlight.h>
#endif

namespace {
constexpr int kCardCornerRadius = 8;
constexpr int kCoverCornerRadius = 4;
constexpr int kTileCornerRadius = 8;
constexpr int kContentInset = 14;
constexpr int kMainMenuIconSize = 32;
constexpr float kCoverAspect = 2.0f / 3.0f;

// 32x32 Gamepad Icon bitmap (1-bpp, 128 bytes)
static const uint8_t icon_games_32_bits[128] = {
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x7E, 0x7E, 0x00,
    0x01, 0xFF, 0xFF, 0x80,
    0x03, 0xFF, 0xFF, 0xC0,
    0x07, 0xFF, 0xFF, 0xE0,
    0x0F, 0xFF, 0xFF, 0xF0,
    0x1F, 0xFF, 0xFF, 0xF8,
    0x3F, 0x1F, 0xF8, 0xFC,
    0x3E, 0x1F, 0xF8, 0x7C,
    0x7C, 0x07, 0xE0, 0x3E,
    0x7C, 0x01, 0x80, 0x3E,
    0x7C, 0x01, 0x80, 0x3E,
    0x7C, 0x07, 0xE0, 0x3E,
    0x3E, 0x1F, 0xF8, 0x7C,
    0x3F, 0x1F, 0xF8, 0xFC,
    0x1F, 0x8F, 0xF1, 0xF8,
    0x1F, 0x8F, 0xF1, 0xF8,
    0x0F, 0xFF, 0xFF, 0xF0,
    0x0F, 0xFF, 0xFF, 0xF0,
    0x07, 0xFF, 0xFF, 0xE0,
    0x07, 0x81, 0x81, 0xE0,
    0x03, 0x00, 0x00, 0xC0,
    0x03, 0x00, 0x00, 0xC0,
    0x01, 0x00, 0x00, 0x80,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00
};

void drawThemeIcon(const GfxRenderer& renderer, const uint8_t bitmap[], int x, int y, int size, bool black = true) {
  const int rowBytes = (size + 7) / 8;
  for (int row = 0; row < size; row++) {
    for (int col = 0; col < size; col++) {
      const uint8_t byte = bitmap[row * rowBytes + (col >> 3)];
      const bool ink = ((byte >> (7 - (col & 7))) & 1) == 0;
      if (ink) {
        renderer.drawPixel(x + (size - 1 - row), y + col, black);
      }
    }
  }
}

Rect fittedBitmapRect(const Bitmap& bitmap, const Rect& target) {
  if (bitmap.getWidth() <= 0 || bitmap.getHeight() <= 0 || target.width <= 0 || target.height <= 0) {
    return target;
  }
  const float widthScale = static_cast<float>(target.width) / static_cast<float>(bitmap.getWidth());
  const float heightScale = static_cast<float>(target.height) / static_cast<float>(bitmap.getHeight());
  const float scale = std::min(1.0f, std::min(widthScale, heightScale));
  const int drawnW = std::min(target.width, std::max(1, static_cast<int>(std::ceil(bitmap.getWidth() * scale))));
  const int drawnH = std::min(target.height, std::max(1, static_cast<int>(std::ceil(bitmap.getHeight() * scale))));
  return Rect{target.x + (target.width - drawnW) / 2, target.y + (target.height - drawnH) / 2, drawnW, drawnH};
}

std::string coverPathForRect(const RecentBook& book) {
  if (book.coverBmpPath.empty()) return {};
  return UITheme::getCoverThumbPath(book.coverBmpPath, ModernMetrics::values.homeCoverHeight);
}

void drawMissingBookCover(const GfxRenderer& renderer, const Rect& coverRect, const RecentBook& book) {
  renderer.fillRoundedRect(coverRect.x, coverRect.y, coverRect.width, coverRect.height, kCoverCornerRadius,
                           Color::White);
  renderer.drawRoundedRect(coverRect.x, coverRect.y, coverRect.width, coverRect.height, 1, kCoverCornerRadius, true);

  const int iconSize = std::min(32, std::min(coverRect.width, coverRect.height) - 16);
  if (iconSize > 8) {
    renderer.drawIcon(CoverIcon, coverRect.x + (coverRect.width - iconSize) / 2, coverRect.y + 12, iconSize);
  }

  constexpr int textPadding = 8;
  const int textW = std::max(1, coverRect.width - textPadding * 2);
  const char* title = book.title.empty() ? book.path.c_str() : book.title.c_str();
  auto titleLines = renderer.wrappedText(UI_10_FONT_ID, title, textW, 3, EpdFontFamily::BOLD);
  const int lineH = renderer.getLineHeight(UI_10_FONT_ID);
  const int textTop = coverRect.y + 12 + (iconSize > 8 ? iconSize + 8 : 0);
  int textY = textTop;
  for (const auto& line : titleLines) {
    const int lineW = renderer.getTextWidth(UI_10_FONT_ID, line.c_str(), EpdFontFamily::BOLD);
    renderer.drawText(UI_10_FONT_ID, coverRect.x + (coverRect.width - lineW) / 2, textY, line.c_str(), true,
                      EpdFontFamily::BOLD);
    textY += lineH;
  }
}

void drawBookCover(const GfxRenderer& renderer, const Rect& coverRect, const RecentBook& book) {
  bool hasCover = false;
  const std::string coverBmpPath = coverPathForRect(book);
  if (!coverBmpPath.empty() && Storage.exists(coverBmpPath.c_str())) {
    HalFile file;
    if (Storage.openFileForRead("HOME", coverBmpPath, file)) {
      Bitmap bitmap(file);
      if (bitmap.parseHeaders() == BmpReaderError::Ok) {
        const Rect bitmapRect = fittedBitmapRect(bitmap, coverRect);
        renderer.fillRoundedRect(coverRect.x, coverRect.y, coverRect.width, coverRect.height, kCoverCornerRadius,
                                 Color::White);
        renderer.drawBitmap(bitmap, bitmapRect.x, bitmapRect.y, bitmapRect.width, bitmapRect.height);
        renderer.maskRoundedRectOutsideCorners(bitmapRect.x, bitmapRect.y, bitmapRect.width, bitmapRect.height,
                                               kCoverCornerRadius, Color::White);
        renderer.drawRoundedRect(bitmapRect.x, bitmapRect.y, bitmapRect.width, bitmapRect.height, 1,
                                 kCoverCornerRadius, true);
        hasCover = true;
      }
      file.close();
    }
  }
  if (!hasCover) {
    drawMissingBookCover(renderer, coverRect, book);
  }
}

}  // namespace

void ModernTheme::drawHeader(const GfxRenderer& renderer, Rect rect, const char* /*title*/,
                             const char* /*subtitle*/) const {
  char timeBuf[16] = {};
  const bool hasTime =
      halClock.formatTime(timeBuf, sizeof(timeBuf), SETTINGS.clockUtcOffsetQ, (SETTINGS.clockFormat == 1));
  if (!hasTime || timeBuf[0] == '\0') {
    strncpy(timeBuf, "BookPoint", sizeof(timeBuf));
  }

  char dateBuf[32] = {};
  const bool hasDate = halClock.formatDate(dateBuf, sizeof(dateBuf), SETTINGS.clockUtcOffsetQ);

  const int textY = rect.y + (rect.height - renderer.getLineHeight(UI_12_FONT_ID)) / 2;
  renderer.drawText(UI_12_FONT_ID, rect.x + kContentInset, textY, timeBuf, true, EpdFontFamily::BOLD);

  if (hasDate && dateBuf[0] != '\0') {
    const int timeW = renderer.getTextWidth(UI_12_FONT_ID, timeBuf, EpdFontFamily::BOLD);
    renderer.drawText(UI_10_FONT_ID, rect.x + kContentInset + timeW + 12, textY + 2, dateBuf, true);
  }

  int rightX = rect.x + rect.width - kContentInset;

  // Battery percentage + icon
  const uint16_t percentage = powerManager.getBatteryPercentage();
  char battBuf[12];
  snprintf(battBuf, sizeof(battBuf), "%u%%", percentage);
  const int battTextW = renderer.getTextWidth(SMALL_FONT_ID, battBuf, EpdFontFamily::BOLD);

  constexpr int battIconW = 20;
  constexpr int battIconH = 11;
  const int battTotalW = battTextW + 4 + battIconW;
  rightX -= battTotalW;

  renderer.drawText(SMALL_FONT_ID, rightX, textY + 3, battBuf, true, EpdFontFamily::BOLD);
  const int battIconX = rightX + battTextW + 4;
  const int battIconY = textY + (renderer.getLineHeight(UI_12_FONT_ID) - battIconH) / 2;
  drawBatteryOutline(renderer, battIconX, battIconY, battIconW, battIconH);
  fillBatteryIcon(renderer, Rect{battIconX, battIconY, battIconW, battIconH}, percentage);
  if (gpio.isUsbConnected()) {
    drawBatteryLightningBolt(renderer, battIconX + (battIconW - 6) / 2, battIconY + 1);
  }

  // WiFi icon if enabled/connected
  if (WiFi.isConnected() || WiFi.getMode() != WIFI_OFF) {
    rightX -= 26;
    renderer.drawIcon(WifiIcon, rightX, textY + 1, 20);
  }

#if FREEINK_CAP_FRONTLIGHT
  if (Frontlight.present() && Frontlight.brightness() > 0) {
    rightX -= 46;
    char flBuf[12];
    snprintf(flBuf, sizeof(flBuf), "FL:%d", Frontlight.brightness());
    renderer.drawText(SMALL_FONT_ID, rightX, textY + 3, flBuf, true);
  }
#endif

  // 1px hairline divider
  renderer.drawLine(rect.x, rect.y + rect.height - 1, rect.x + rect.width - 1, rect.y + rect.height - 1, true);
}

void ModernTheme::drawHeroCard(const GfxRenderer& renderer, Rect cardRect, const RecentBook* book, bool isSelected,
                              const BookReadingStats* stats, float progressPercent) const {
  // Card outline / container
  renderer.fillRoundedRect(cardRect.x, cardRect.y, cardRect.width, cardRect.height, kCardCornerRadius, Color::White);
  if (isSelected) {
    renderer.drawRoundedRect(cardRect.x, cardRect.y, cardRect.width, cardRect.height, 2, kCardCornerRadius, true);
    renderer.drawRoundedRect(cardRect.x + 3, cardRect.y + 3, cardRect.width - 6, cardRect.height - 6, 1,
                             kCardCornerRadius - 2, true);
  } else {
    renderer.drawRoundedRect(cardRect.x, cardRect.y, cardRect.width, cardRect.height, 1, kCardCornerRadius, true);
  }

  if (!book) {
    // Empty state
    const int iconSize = 48;
    renderer.drawIcon(CoverIcon, cardRect.x + (cardRect.width - iconSize) / 2, cardRect.y + 80, iconSize);
    const char* emptyTitle = tr(STR_NO_OPEN_BOOK);
    const int emptyW = renderer.getTextWidth(UI_12_FONT_ID, emptyTitle, EpdFontFamily::BOLD);
    renderer.drawText(UI_12_FONT_ID, cardRect.x + (cardRect.width - emptyW) / 2, cardRect.y + 145, emptyTitle, true,
                      EpdFontFamily::BOLD);
    const char* startTitle = tr(STR_START_READING);
    const int startW = renderer.getTextWidth(SMALL_FONT_ID, startTitle);
    renderer.drawText(SMALL_FONT_ID, cardRect.x + (cardRect.width - startW) / 2, cardRect.y + 175, startTitle, true);
    return;
  }

  // 1. Real Cover on the left: 146x219
  constexpr int coverW = 146;
  constexpr int coverH = 219;
  const int coverX = cardRect.x + 18;
  const int coverY = cardRect.y + (cardRect.height - coverH) / 2;
  const Rect coverRect{coverX, coverY, coverW, coverH};
  drawBookCover(renderer, coverRect, *book);

  // 3D spine shadow effect: highlight line, crease line, and drop shadow along bottom and right
  renderer.drawLine(coverRect.x + 3, coverRect.y + 1, coverRect.x + 3, coverRect.y + coverRect.height - 2, false);
  renderer.drawLine(coverRect.x + 5, coverRect.y + 1, coverRect.x + 5, coverRect.y + coverRect.height - 2, true);
  renderer.drawLine(coverRect.x + coverRect.width, coverRect.y + 3, coverRect.x + coverRect.width,
                    coverRect.y + coverRect.height, true);
  renderer.drawLine(coverRect.x + 3, coverRect.y + coverRect.height, coverRect.x + coverRect.width,
                    coverRect.y + coverRect.height, true);
  renderer.drawLine(coverRect.x + coverRect.width + 1, coverRect.y + 4, coverRect.x + coverRect.width + 1,
                    coverRect.y + coverRect.height + 1, true);
  renderer.drawLine(coverRect.x + 4, coverRect.y + coverRect.height + 1, coverRect.x + coverRect.width + 1,
                    coverRect.y + coverRect.height + 1, true);

  // 2. Right-side content
  const int metaX = coverRect.x + coverRect.width + 18;
  const int metaW = std::max(1, cardRect.x + cardRect.width - metaX - 16);
  int curY = cardRect.y + 16;

  // Category badge: "СЕЙЧАС ЧИТАЮ"
  const char* badgeText = tr(STR_NOW_READING);
  const int badgeTextW = renderer.getTextWidth(SMALL_FONT_ID, badgeText, EpdFontFamily::BOLD);
  const int badgeW = badgeTextW + 16;
  renderer.fillRoundedRect(metaX, curY, badgeW, 20, 4, Color::Black);
  renderer.drawText(SMALL_FONT_ID, metaX + 8, curY + 4, badgeText, false, EpdFontFamily::BOLD);
  curY += 28;

  // Book title (up to 3 lines)
  const char* titleStr = book->title.empty() ? book->path.c_str() : book->title.c_str();
  auto titleLines = renderer.wrappedText(UI_12_FONT_ID, titleStr, metaW, 3, EpdFontFamily::BOLD);
  const int lineH = renderer.getLineHeight(UI_12_FONT_ID);
  for (const auto& line : titleLines) {
    renderer.drawText(UI_12_FONT_ID, metaX, curY, line.c_str(), true, EpdFontFamily::BOLD);
    curY += lineH + 2;
  }
  curY += 2;

  // Author
  if (!book->author.empty()) {
    renderer.drawText(UI_10_FONT_ID, metaX, curY, book->author.c_str(), true);
    curY += renderer.getLineHeight(UI_10_FONT_ID) + 8;
  } else {
    curY += 4;
  }

  // Capsule progress bar %
  float pct = progressPercent;
  if (pct < 0.0f && stats != nullptr && stats->totalReadingSeconds > 0) {
    pct = 0.0f;
  }
  const int percentInt = pct >= 0.0f ? std::clamp(static_cast<int>(std::round(pct)), 0, 100) : 0;

  constexpr int barW = 150;
  constexpr int barH = 10;
  renderer.drawRoundedRect(metaX, curY + 2, barW, barH, 1, 5, true);
  if (percentInt > 0) {
    const int fillW = std::max(6, (barW * percentInt) / 100);
    renderer.fillRoundedRect(metaX, curY + 2, fillW, barH, 5, Color::Black);
  }
  char pctBuf[16];
  snprintf(pctBuf, sizeof(pctBuf), "%d%%", percentInt);
  renderer.drawText(UI_10_FONT_ID, metaX + barW + 8, curY, pctBuf, true, EpdFontFamily::BOLD);
  curY += 22;

  // Estimated reading time
  if (stats != nullptr && stats->totalReadingSeconds > 0) {
    char timeStr[64];
    if (percentInt > 0 && percentInt < 100) {
      const int remPct = 100 - percentInt;
      const uint32_t estRemSec = (stats->totalReadingSeconds * remPct) / percentInt;
      const unsigned hours = estRemSec / 3600;
      const unsigned mins = (estRemSec % 3600) / 60;
      if (hours > 0) {
        snprintf(timeStr, sizeof(timeStr), "~%u ч %u мин до конца", hours, mins);
      } else {
        snprintf(timeStr, sizeof(timeStr), "~%u мин до конца", std::max(1u, mins));
      }
    } else {
      const unsigned hours = stats->totalReadingSeconds / 3600;
      const unsigned mins = (stats->totalReadingSeconds % 3600) / 60;
      if (hours > 0) {
        snprintf(timeStr, sizeof(timeStr), "%u ч %u мин прочитано", hours, mins);
      } else {
        snprintf(timeStr, sizeof(timeStr), "%u мин прочитано", mins);
      }
    }
    renderer.drawText(SMALL_FONT_ID, metaX, curY, timeStr, true);
    curY += renderer.getLineHeight(SMALL_FONT_ID) + 6;
  }

  // Reading streak badge
  ReadingStatsDateTime now;
  if (getCurrentLocalReadingStatsDateTime(now) && now.isValid()) {
    const GlobalReadingStats globalStats = StatsStore::loadGlobalStats();
    const uint16_t streak = globalStats.currentReadingStreak(readingStatsDayIndex(now.date));
    if (streak > 0) {
      char streakBuf[32];
      snprintf(streakBuf, sizeof(streakBuf), "Серия: %u дн.", static_cast<unsigned>(streak));
      const int sW = renderer.getTextWidth(SMALL_FONT_ID, streakBuf, EpdFontFamily::BOLD);
      renderer.drawRoundedRect(metaX, curY, sW + 12, 18, 1, 4, true);
      renderer.drawText(SMALL_FONT_ID, metaX + 6, curY + 2, streakBuf, true, EpdFontFamily::BOLD);
      curY += 24;
    }
  }

  // "Продолжить чтение" pill button
  constexpr int btnW = 140;
  constexpr int btnH = 30;
  const int btnY = cardRect.y + cardRect.height - btnH - 14;
  if (isSelected) {
    renderer.fillRoundedRect(metaX, btnY, btnW, btnH, 15, Color::Black);
    renderer.drawText(UI_10_FONT_ID, metaX + 16, btnY + 7, tr(STR_RESUME), false, EpdFontFamily::BOLD);
  } else {
    renderer.drawRoundedRect(metaX, btnY, btnW, btnH, 1, 15, true);
    renderer.drawText(UI_10_FONT_ID, metaX + 16, btnY + 7, tr(STR_RESUME), true, EpdFontFamily::BOLD);
  }
}

void ModernTheme::drawMiniBookCard(const GfxRenderer& renderer, Rect miniRect, const RecentBook& book,
                                  bool isSelected) const {
  if (isSelected) {
    renderer.fillRoundedRect(miniRect.x, miniRect.y, miniRect.width, miniRect.height, 8, Color::Black);
    renderer.drawRoundedRect(miniRect.x, miniRect.y, miniRect.width, miniRect.height, 2, 8, false);
  } else {
    renderer.fillRoundedRect(miniRect.x, miniRect.y, miniRect.width, miniRect.height, 8, Color::White);
    renderer.drawRoundedRect(miniRect.x, miniRect.y, miniRect.width, miniRect.height, 1, 8, true);
  }

  // Mini cover thumbnail on the left: 44x66
  constexpr int mCoverW = 44;
  constexpr int mCoverH = 66;
  const Rect mCoverRect{miniRect.x + 8, miniRect.y + 8, mCoverW, mCoverH};
  drawBookCover(renderer, mCoverRect, book);

  // Title and author on the right
  const int textX = mCoverRect.x + mCoverRect.width + 8;
  const int textW = std::max(1, miniRect.x + miniRect.width - textX - 8);
  const char* titleStr = book.title.empty() ? book.path.c_str() : book.title.c_str();
  auto lines = renderer.wrappedText(SMALL_FONT_ID, titleStr, textW, 2, EpdFontFamily::BOLD);
  int textY = miniRect.y + 8;
  const int lH = renderer.getLineHeight(SMALL_FONT_ID);
  for (const auto& line : lines) {
    renderer.drawText(SMALL_FONT_ID, textX, textY, line.c_str(), !isSelected, EpdFontFamily::BOLD);
    textY += lH;
  }

  if (!book.author.empty()) {
    renderer.drawText(SMALL_FONT_ID, textX, textY + 2, book.author.c_str(), !isSelected);
  }

  // Progress bar & percentage
  BookReadingStats bStats;
  int pct = 0;
  if (StatsStore::loadBookStatsInto(book.path, bStats)) {
    if (bStats.isFinished) {
      pct = 100;
    } else if (bStats.totalPagesTurned > 0) {
      pct = std::clamp(static_cast<int>(bStats.totalPagesTurned % 100), 5, 95);
    }
  }

  const int pBarW = miniRect.width - 16;
  const int pBarH = 6;
  const int pBarX = miniRect.x + 8;
  const int pBarY = miniRect.y + miniRect.height - 14;

  char pctBuf[16];
  snprintf(pctBuf, sizeof(pctBuf), "%d%%", pct);
  renderer.drawText(SMALL_FONT_ID, pBarX, pBarY - 14, pctBuf, !isSelected, EpdFontFamily::BOLD);

  renderer.drawRoundedRect(pBarX, pBarY, pBarW, pBarH, 1, 3, !isSelected);
  if (pct > 0) {
    const int fillW = std::max(4, (pBarW * pct) / 100);
    renderer.fillRoundedRect(pBarX, pBarY, fillW, pBarH, 3, isSelected ? Color::White : Color::Black);
  }
}

void ModernTheme::drawRecentShelf(const GfxRenderer& renderer, Rect shelfRect,
                                 const std::vector<RecentBook>& recentBooks, int selectorIndex) const {
  if (recentBooks.size() <= 1) return;

  // Header: NO emojis!
  renderer.drawText(SMALL_FONT_ID, shelfRect.x + 4, shelfRect.y + 2, tr(STR_RECENT_SHELF), true, EpdFontFamily::BOLD);

  const int count = std::min(3, static_cast<int>(recentBooks.size()) - 1);
  constexpr int gap = 10;
  const int slotW = 144;
  const int slotH = shelfRect.height - 24;
  const int slotY = shelfRect.y + 22;

  for (int i = 0; i < count; ++i) {
    const int slotX = shelfRect.x + i * (slotW + gap);
    const bool isSelected = (selectorIndex == (i + 1));
    drawMiniBookCard(renderer, Rect{slotX, slotY, slotW, slotH}, recentBooks[i + 1], isSelected);
  }
}

void ModernTheme::drawRecentBookCover(GfxRenderer& renderer, Rect rect,
                                      const std::vector<RecentBook>& recentBooks, int selectorIndex,
                                      bool& coverRendered, bool& coverBufferStored, bool& bufferRestored,
                                      std::function<bool()> storeCoverBuffer, const BookReadingStats* stats,
                                      float progressPercent) const {
  const bool hasBook = !recentBooks.empty();
  const bool heroSelected = hasBook && selectorIndex == 0;

  // 1. Hero Card: [14..465, 50..360] -> width 452, height 310
  constexpr int heroCardH = 310;
  const Rect heroRect{kContentInset, rect.y, rect.width - 2 * kContentInset, heroCardH};

  // 2. Recent Shelf: [14..465, 372..540] -> width 452, height 168
  const bool hasShelf = recentBooks.size() > 1;
  const int shelfY = 372;
  const int shelfH = 168;
  const Rect shelfRect{kContentInset, shelfY, rect.width - 2 * kContentInset, shelfH};

  if (!coverRendered) {
    drawHeroCard(renderer, heroRect, hasBook ? &recentBooks[0] : nullptr, heroSelected, stats, progressPercent);
    if (hasShelf) {
      drawRecentShelf(renderer, shelfRect, recentBooks, selectorIndex);
    }
    coverBufferStored = storeCoverBuffer();
    coverRendered = coverBufferStored;
    return;
  }

  // Fast path: Redraw selection state
  drawHeroCard(renderer, heroRect, hasBook ? &recentBooks[0] : nullptr, heroSelected, stats, progressPercent);
  if (hasShelf) {
    drawRecentShelf(renderer, shelfRect, recentBooks, selectorIndex);
  }
}

void ModernTheme::drawButtonMenu(GfxRenderer& renderer, Rect rect, int /*buttonCount*/, int selectedIndex,
                                 const std::function<std::string(int index)>& /*buttonLabel*/,
                                 const std::function<UIIcon(int index)>& /*rowIcon*/) const {
  // Top 1px hairline divider at Y = 720
  renderer.drawLine(rect.x, rect.y, rect.x + rect.width - 1, rect.y, true);

  struct DockColumn {
    const char* label;
    const uint8_t* icon;
  };
  const bool isRu = (I18N.getLanguage() == Language::RU);
  const DockColumn dockItems[5] = {
      {isRu ? "Библиотека" : "Library", LibraryIcon},
      {isRu ? "Поиск" : "Search", icon_search_32_bits},
      {isRu ? "Статистика" : "Stats", StatsIcon},
      {isRu ? "Приложения" : "Apps", icon_games_32_bits},
      {isRu ? "Настройки" : "Settings", Settings2Icon},
  };

  constexpr int cols = 5;
  const int colW = rect.width / cols;  // 480 / 5 = 96
  const int colY = rect.y + 1;
  const int colH = rect.height - 1;

  for (int c = 0; c < cols; ++c) {
    const int colX = rect.x + c * colW;
    const bool isSelected = (selectedIndex == c);
    const auto& item = dockItems[c];

    if (isSelected) {
      // Inverted E-Ink highlight feedback
      renderer.fillRoundedRect(colX + 3, colY + 3, colW - 6, colH - 6, kTileCornerRadius, Color::Black);
      if (item.icon != nullptr) {
        drawThemeIcon(renderer, item.icon, colX + (colW - kMainMenuIconSize) / 2, colY + 10, kMainMenuIconSize,
                      false);
      }
      const int textW = renderer.getTextWidth(SMALL_FONT_ID, item.label, EpdFontFamily::BOLD);
      renderer.drawText(SMALL_FONT_ID, colX + (colW - textW) / 2, colY + 48, item.label, false,
                        EpdFontFamily::BOLD);
    } else {
      // Unselected dock item
      renderer.fillRoundedRect(colX + 3, colY + 3, colW - 6, colH - 6, kTileCornerRadius, Color::White);
      if (item.icon != nullptr) {
        renderer.drawIcon(item.icon, colX + (colW - kMainMenuIconSize) / 2, colY + 10, kMainMenuIconSize);
      }
      const int textW = renderer.getTextWidth(SMALL_FONT_ID, item.label, EpdFontFamily::BOLD);
      renderer.drawText(SMALL_FONT_ID, colX + (colW - textW) / 2, colY + 48, item.label, true,
                        EpdFontFamily::BOLD);
    }

    // Vertical 1px hairline divider between unselected columns
    if (c > 0 && !isSelected && selectedIndex != c - 1) {
      renderer.drawLine(colX, colY + 14, colX, colY + colH - 14, true);
    }
  }
}
