#include "MinimalTheme.h"

#include <Bitmap.h>
#include <GfxRenderer.h>
#include <HalStorage.h>
#include <I18n.h>

#include <algorithm>
#include <cstdio>
#include <string>

#include "RecentBooksStore.h"
#include "components/UITheme.h"
#include "components/icons/cover.h"
#include "fontIds.h"
#include "stats/BookReadingStats.h"
#include "stats/GlobalReadingStats.h"
#include "stats/ReadingStatsTypes.h"

namespace {
constexpr int kCoverCornerRadius = 12;
constexpr int kProgressBarHeight = 8;
constexpr int kProgressBlockGap = 12;
constexpr int kProgressBarGap = 6;
constexpr int kProgressLabelGap = 6;
constexpr int kStatsFooterInset = 24;

// Centered 3:5 cover matching the "Minimal / four tiles" look from inkMOD.
Rect coverRectForScreen(const GfxRenderer& renderer, const Rect& rect) {
  const int coverW = MinimalMetrics::homeCoverWidth;
  const int coverH = MinimalMetrics::values.homeCoverHeight;
  return Rect{(renderer.getScreenWidth() - coverW) / 2, rect.y, coverW, coverH};
}

Rect fittedBitmapRect(const Bitmap& bitmap, const Rect& target) {
  if (bitmap.getWidth() <= 0 || bitmap.getHeight() <= 0 || target.width <= 0 || target.height <= 0) {
    return target;
  }
  const float widthScale = static_cast<float>(target.width) / static_cast<float>(bitmap.getWidth());
  const float heightScale = static_cast<float>(target.height) / static_cast<float>(bitmap.getHeight());
  const float scale = std::min(1.0f, std::min(widthScale, heightScale));
  const int drawnW = std::min(target.width, std::max(1, static_cast<int>(bitmap.getWidth() * scale)));
  const int drawnH = std::min(target.height, std::max(1, static_cast<int>(bitmap.getHeight() * scale)));
  return Rect{target.x + (target.width - drawnW) / 2, target.y + (target.height - drawnH) / 2, drawnW, drawnH};
}

std::string coverPathForRect(const RecentBook& book) {
  if (book.coverBmpPath.empty()) return {};
  return UITheme::getCoverThumbPath(book.coverBmpPath, MinimalMetrics::values.homeCoverHeight);
}

void drawMissingBookCover(const GfxRenderer& renderer, const Rect& coverRect, const RecentBook& book) {
  renderer.fillRoundedRect(coverRect.x, coverRect.y, coverRect.width, coverRect.height, kCoverCornerRadius,
                           Color::White);
  renderer.drawRoundedRect(coverRect.x, coverRect.y, coverRect.width, coverRect.height, 1, kCoverCornerRadius, true);

  constexpr int iconSize = 40;
  renderer.drawIcon(CoverIcon, coverRect.x + (coverRect.width - iconSize) / 2,
                    coverRect.y + coverRect.height / 4 - iconSize / 2, iconSize);

  constexpr int textPadding = 16;
  const int textW = std::max(1, coverRect.width - textPadding * 2);
  const char* title = book.title.empty() ? book.path.c_str() : book.title.c_str();
  auto titleLines = renderer.wrappedText(UI_12_FONT_ID, title, textW, 3, EpdFontFamily::BOLD);
  const int lineH = renderer.getLineHeight(UI_12_FONT_ID);
  int textY = coverRect.y + coverRect.height / 2;
  for (const auto& line : titleLines) {
    const int lineW = renderer.getTextWidth(UI_12_FONT_ID, line.c_str(), EpdFontFamily::BOLD);
    renderer.drawText(UI_12_FONT_ID, coverRect.x + (coverRect.width - lineW) / 2, textY, line.c_str(), true,
                      EpdFontFamily::BOLD);
    textY += lineH;
  }
}

void drawBookCover(const GfxRenderer& renderer, const Rect& coverRect, const RecentBook& book,
                   const Color backgroundColor) {
  bool hasCover = false;
  const std::string coverBmpPath = coverPathForRect(book);
  if (!coverBmpPath.empty() && Storage.exists(coverBmpPath.c_str())) {
    HalFile file;
    if (Storage.openFileForRead("HOME", coverBmpPath, file)) {
      Bitmap bitmap(file);
      if (bitmap.parseHeaders() == BmpReaderError::Ok) {
        const Rect bitmapRect = fittedBitmapRect(bitmap, coverRect);
        renderer.fillRoundedRect(coverRect.x, coverRect.y, coverRect.width, coverRect.height, kCoverCornerRadius,
                                 backgroundColor);
        renderer.fillRoundedRect(bitmapRect.x, bitmapRect.y, bitmapRect.width, bitmapRect.height, kCoverCornerRadius,
                                 Color::White);
        renderer.drawBitmap(bitmap, bitmapRect.x, bitmapRect.y, bitmapRect.width, bitmapRect.height);
        renderer.maskRoundedRectOutsideCorners(bitmapRect.x, bitmapRect.y, bitmapRect.width, bitmapRect.height,
                                               kCoverCornerRadius, backgroundColor);
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

// Duration line + a progress bar with a right-aligned percent label below the
// cover. Uses a light-gray fill on white (home) or plain inverted bars on black
// (sleep screen).
void drawProgressBlock(const GfxRenderer& renderer, const Rect& coverRect, const BookReadingStats* stats,
                       const float progressPercent, const bool inverted) {
  if ((stats == nullptr || stats->totalReadingSeconds == 0) && progressPercent < 0.0f) return;

  const int barW = coverRect.width;
  const int barX = coverRect.x;
  const bool textBlack = !inverted;
  const int lineH = renderer.getLineHeight(UI_10_FONT_ID);

  char duration[32] = {0};
  if (stats != nullptr && stats->totalReadingSeconds > 0) {
    BookReadingStats::formatDuration(stats->totalReadingSeconds, duration, sizeof(duration));
  }

  const int durationY = coverRect.y + coverRect.height + kProgressBlockGap;
  if (duration[0] != '\0') {
    renderer.drawText(UI_10_FONT_ID, barX, durationY, duration, textBlack);
  }

  if (progressPercent < 0.0f) return;

  const int progress = std::clamp(static_cast<int>(progressPercent + 0.5f), 0, 100);
  const int barY = durationY + lineH + kProgressBarGap;
  const int fillW = (barW * progress) / 100;
  if (inverted) {
    renderer.drawRect(barX, barY, barW, kProgressBarHeight, false);
    if (fillW > 0) renderer.fillRect(barX, barY, fillW, kProgressBarHeight, false);
  } else {
    renderer.fillRectDither(barX, barY, barW, kProgressBarHeight, Color::LightGray);
    if (fillW > 0) renderer.fillRectDither(barX, barY, fillW, kProgressBarHeight, Color::DarkGray);
  }

  char progressLabel[12];
  snprintf(progressLabel, sizeof(progressLabel), "%d%%", progress);
  const int labelW = renderer.getTextWidth(UI_10_FONT_ID, progressLabel);
  renderer.drawText(UI_10_FONT_ID, barX + barW - labelW, barY + kProgressBarHeight + kProgressLabelGap, progressLabel,
                    textBlack);
}
}  // namespace

void MinimalTheme::drawRecentBookCover(GfxRenderer& renderer, Rect rect, const std::vector<RecentBook>& recentBooks,
                                       int selectorIndex, bool& coverRendered, bool& coverBufferStored,
                                       bool& bufferRestored, std::function<bool()> storeCoverBuffer,
                                       const BookReadingStats* stats, float progressPercent) const {
  (void)selectorIndex;
  (void)bufferRestored;

  const Rect coverRect = coverRectForScreen(renderer, rect);
  if (recentBooks.empty()) {
    renderer.drawRoundedRect(coverRect.x, coverRect.y, coverRect.width, coverRect.height, 1, kCoverCornerRadius, true);
    const char* line1 = tr(STR_NO_OPEN_BOOK);
    const char* line2 = tr(STR_START_READING);
    const int lineH = renderer.getLineHeight(UI_12_FONT_ID);
    const int line1W = renderer.getTextWidth(UI_12_FONT_ID, line1, EpdFontFamily::BOLD);
    const int line2W = renderer.getTextWidth(UI_10_FONT_ID, line2);
    renderer.drawText(UI_12_FONT_ID, coverRect.x + (coverRect.width - line1W) / 2,
                      coverRect.y + coverRect.height / 2 - lineH, line1, true, EpdFontFamily::BOLD);
    renderer.drawText(UI_10_FONT_ID, coverRect.x + (coverRect.width - line2W) / 2, coverRect.y + coverRect.height / 2,
                      line2, true);
    coverRendered = false;
    coverBufferStored = false;
    return;
  }

  if (!coverRendered) {
    drawBookCover(renderer, coverRect, recentBooks[0], Color::White);
    coverBufferStored = storeCoverBuffer();
    coverRendered = coverBufferStored;
  }

  drawProgressBlock(renderer, coverRect, stats, progressPercent, false);
}

void MinimalTheme::drawSleepScreen(const GfxRenderer& renderer, const RecentBook& book, const BookReadingStats* stats,
                                   const float progressPercent) const {
  renderer.clearScreen(0x00);

  const Rect contentRect{0, MinimalMetrics::values.homeTopPadding, renderer.getScreenWidth(),
                         MinimalMetrics::values.homeCoverTileHeight};
  const Rect coverRect = coverRectForScreen(renderer, contentRect);
  drawBookCover(renderer, coverRect, book, Color::Black);
  drawProgressBlock(renderer, coverRect, stats, progressPercent, true);
}

void MinimalTheme::drawStatsSleepScreen(const GfxRenderer& renderer, const RecentBook& book,
                                        const BookReadingStats* stats, const GlobalReadingStats* globalStats,
                                        const float progressPercent) const {
  drawSleepScreen(renderer, book, stats, progressPercent);

  if (globalStats == nullptr) return;

  // Simple text footer under the cover: current streak + total reading time.
  ReadingStatsDateTime today;
  const uint16_t streak =
      getCurrentLocalReadingStatsDateTime(today) ? globalStats->currentReadingStreak(readingStatsDayIndex(today.date))
                                                 : 0;
  char buf[96];
  if (streak > 0) {
    snprintf(buf, sizeof(buf), "%s: %u %s", tr(STR_STATS_STREAK), static_cast<unsigned>(streak), tr(STR_STATS_DAYS));
  } else {
    snprintf(buf, sizeof(buf), "%s: 0", tr(STR_STATS_STREAK));
  }

  const int pageWidth = renderer.getScreenWidth();
  const int lineH = renderer.getLineHeight(UI_10_FONT_ID);
  const int y = MinimalMetrics::values.homeTopPadding + MinimalMetrics::values.homeCoverTileHeight + 8;
  const int textW = renderer.getTextWidth(UI_10_FONT_ID, buf);
  renderer.drawText(UI_10_FONT_ID, (pageWidth - textW) / 2, y, buf, false);

  if (stats != nullptr && stats->totalReadingSeconds > 0) {
    char timeBuf[48];
    BookReadingStats::formatDuration(stats->totalReadingSeconds, timeBuf, sizeof(timeBuf));
    snprintf(buf, sizeof(buf), "%s: %s", tr(STR_STATS_TOTAL_TIME), timeBuf);
    const int timeW = renderer.getTextWidth(UI_10_FONT_ID, buf);
    renderer.drawText(UI_10_FONT_ID, (pageWidth - timeW) / 2, y + lineH + 4, buf, false);
  }
}
