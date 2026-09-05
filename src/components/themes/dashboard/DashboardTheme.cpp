#include "DashboardTheme.h"

#include <Bitmap.h>
#include <GfxRenderer.h>
#include <HalStorage.h>
#include <I18n.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "RecentBooksStore.h"
#include "components/UITheme.h"
#include "components/icons/cover.h"
#include "fontIds.h"
#include "stats/BookReadingStats.h"
#include "stats/ReadingStatsUtils.h"

namespace {
constexpr int kCoverCornerRadius = 8;
constexpr int kContentInset = 20;
constexpr int kCoverStatsGap = 15;
constexpr int kStatsRowCount = 6;
constexpr int kStatsValueLabelGap = 1;
// CrossInk's real cover is 296x444 (2:3-ish). Kept as the target aspect ratio;
// the actual pixel size is solved from the row height budgeted above.
constexpr float kCoverAspect = 296.0f / 444.0f;  // width / height

Rect coverRectForRow(const Rect& rect, const int statsW) {
  const int maxCoverW = rect.width - kContentInset * 2 - statsW - kCoverStatsGap;
  int coverH = rect.height;
  int coverW = std::min(maxCoverW, static_cast<int>(coverH * kCoverAspect));
  coverW = std::max(1, coverW);
  coverH = std::min(rect.height, static_cast<int>(coverW / kCoverAspect));
  return Rect{rect.x + kContentInset, rect.y, coverW, coverH};
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
  // The home screen pre-generates a thumbnail at DashboardMetrics::values.homeCoverHeight;
  // reuse that height so the "[HEIGHT]" token resolves to a file that exists.
  return UITheme::getCoverThumbPath(book.coverBmpPath, DashboardMetrics::values.homeCoverHeight);
}

void drawMissingBookCover(const GfxRenderer& renderer, const Rect& coverRect, const RecentBook& book) {
  renderer.fillRoundedRect(coverRect.x, coverRect.y, coverRect.width, coverRect.height, kCoverCornerRadius,
                           Color::White);
  renderer.drawRoundedRect(coverRect.x, coverRect.y, coverRect.width, coverRect.height, 1, kCoverCornerRadius, true);

  const int iconSize = std::min(32, std::min(coverRect.width, coverRect.height) - 20);
  if (iconSize > 8) {
    renderer.drawIcon(CoverIcon, coverRect.x + (coverRect.width - iconSize) / 2, coverRect.y + 14, iconSize);
  }

  constexpr int textPadding = 12;
  const int textW = std::max(1, coverRect.width - textPadding * 2);
  const char* title = book.title.empty() ? book.path.c_str() : book.title.c_str();
  auto titleLines = renderer.wrappedText(UI_10_FONT_ID, title, textW, 5, EpdFontFamily::BOLD);
  const int lineH = renderer.getLineHeight(UI_10_FONT_ID);
  const int textTop = coverRect.y + 14 + (iconSize > 8 ? iconSize + 10 : 0);
  const int availableH = std::max(0, coverRect.y + coverRect.height - textTop);
  int textY = textTop + std::max(0, (availableH - static_cast<int>(titleLines.size()) * lineH) / 2);
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

float pagesPerMinute(const uint32_t totalPagesTurned, const uint32_t totalReadingSeconds) {
  if (totalReadingSeconds <= 60) return 0.0f;
  return static_cast<float>(totalPagesTurned) * 60.0f / static_cast<float>(totalReadingSeconds);
}

int statsBlockHeight(const GfxRenderer& renderer) {
  return renderer.getLineHeight(UI_12_FONT_ID) + kStatsValueLabelGap + renderer.getLineHeight(SMALL_FONT_ID);
}

int statsBlockTop(const Rect& rect, const int index, const int blockH, const int rowCount) {
  const int remainingH = std::max(0, rect.height - blockH * rowCount);
  const int gapCount = rowCount - 1;
  const int gap = gapCount > 0 ? remainingH / gapCount : 0;
  const int remainder = gapCount > 0 ? remainingH % gapCount : 0;
  return rect.y + index * (blockH + gap) + std::min(index, remainder);
}

void drawStatsRow(const GfxRenderer& renderer, const int rightX, const int y, const char* value, const char* label) {
  const int valueLineH = renderer.getLineHeight(UI_12_FONT_ID);
  const int valueW = renderer.getTextWidth(UI_12_FONT_ID, value, EpdFontFamily::BOLD);
  renderer.drawText(UI_12_FONT_ID, rightX - valueW, y, value, true, EpdFontFamily::BOLD);
  const int labelW = renderer.getTextWidth(SMALL_FONT_ID, label);
  renderer.drawText(SMALL_FONT_ID, rightX - labelW, y + valueLineH + kStatsValueLabelGap, label);
}

void drawSelectionBorder(const GfxRenderer& renderer, const Rect& rect) {
  renderer.drawRect(rect.x + 1, rect.y + 1, rect.width - 2, rect.height - 2);
  renderer.drawRect(rect.x + 2, rect.y + 2, rect.width - 4, rect.height - 4);
}
}  // namespace

void DashboardTheme::drawCoverPanel(const GfxRenderer& renderer, Rect rect, const RecentBook* book,
                                    bool hasBook) const {
  if (!hasBook || book == nullptr) {
    renderer.drawRoundedRect(rect.x, rect.y, rect.width, rect.height, 1, kCoverCornerRadius, true);
    const int lineH = renderer.getLineHeight(UI_12_FONT_ID);
    const int y = rect.y + (rect.height - lineH - renderer.getLineHeight(UI_10_FONT_ID)) / 2;
    const char* line1 = tr(STR_NO_OPEN_BOOK);
    const char* line2 = tr(STR_START_READING);
    const int line1W = renderer.getTextWidth(UI_12_FONT_ID, line1);
    const int line2W = renderer.getTextWidth(UI_10_FONT_ID, line2);
    renderer.drawText(UI_12_FONT_ID, rect.x + (rect.width - line1W) / 2, y, line1);
    renderer.drawText(UI_10_FONT_ID, rect.x + (rect.width - line2W) / 2, y + lineH, line2);
    return;
  }
  drawBookCover(renderer, rect, *book);
}

void DashboardTheme::drawStatsColumn(const GfxRenderer& renderer, Rect rect, const BookReadingStats* stats,
                                     float progressPercent) const {
  const BookReadingStats emptyStats{};
  const BookReadingStats& bookStats = stats != nullptr ? *stats : emptyStats;

  // Background and border card for statistics, filling the available area
  renderer.fillRoundedRect(rect.x, rect.y, rect.width, rect.height, kCoverCornerRadius, Color::White);
  renderer.drawRoundedRect(rect.x, rect.y, rect.width, rect.height, 1, kCoverCornerRadius, true);

  const int colW = rect.width / 2;
  const int rowH = rect.height / 3;

  // Internal divider lines
  renderer.drawLine(rect.x + colW, rect.y, rect.x + colW, rect.y + rect.height, true);
  renderer.drawLine(rect.x, rect.y + rowH, rect.x + rect.width, rect.y + rowH, true);
  renderer.drawLine(rect.x, rect.y + rowH * 2, rect.x + rect.width, rect.y + rowH * 2, true);

  char value[40];
  uint32_t estimatedSeconds = 0;
  const bool hasEstimate = fallbackEstimatedTimeLeft(bookStats, progressPercent, estimatedSeconds);

  auto drawCell = [&](int cX, int cY, const char* val, const char* lbl) {
    const int valLineH = renderer.getLineHeight(UI_12_FONT_ID);
    const int lblLineH = renderer.getLineHeight(SMALL_FONT_ID);
    const int totalH = valLineH + kStatsValueLabelGap + lblLineH;
    const int startY = cY + (rowH - totalH) / 2;

    renderer.drawText(UI_12_FONT_ID, cX + 10, startY, val, true, EpdFontFamily::BOLD);
    renderer.drawText(SMALL_FONT_ID, cX + 10, startY + valLineH + kStatsValueLabelGap,
                      renderer.truncatedText(SMALL_FONT_ID, lbl, colW - 14).c_str());
  };

  // Row 0, Col 0: Total Reading Time
  BookReadingStats::formatDuration(bookStats.totalReadingSeconds, value, sizeof(value));
  drawCell(rect.x, rect.y, value, tr(STR_STATS_TOTAL_TIME));

  // Row 0, Col 1: Time Left
  if (hasEstimate && !bookStats.isFinished) {
    BookReadingStats::formatDuration(estimatedSeconds, value, sizeof(value));
  } else {
    snprintf(value, sizeof(value), "—");
  }
  drawCell(rect.x + colW, rect.y, value, tr(STR_STATS_TIME_LEFT));

  // Row 1, Col 0: Progress %
  if (progressPercent >= 0.0f) {
    snprintf(value, sizeof(value), "%d%%", static_cast<int>(progressPercent + 0.5f));
  } else {
    snprintf(value, sizeof(value), "—");
  }
  drawCell(rect.x, rect.y + rowH, value, tr(STR_STATS_PROGRESS));

  // Row 1, Col 1: Reading Pace (pages/min)
  snprintf(value, sizeof(value), "%.1f", pagesPerMinute(bookStats.totalPagesTurned, bookStats.totalReadingSeconds));
  drawCell(rect.x + colW, rect.y + rowH, value, tr(STR_STATS_PAGES_PER_MIN));

  // Row 2, Col 0: Sessions Count
  snprintf(value, sizeof(value), "%u", static_cast<unsigned>(bookStats.sessionCount));
  drawCell(rect.x, rect.y + rowH * 2, value, tr(STR_STATS_SESSIONS));

  // Row 2, Col 1: Average Session Duration
  const uint32_t avgSeconds =
      bookStats.sessionCount > 0 ? bookStats.totalReadingSeconds / bookStats.sessionCount : 0;
  BookReadingStats::formatDuration(avgSeconds, value, sizeof(value));
  drawCell(rect.x + colW, rect.y + rowH * 2, value, tr(STR_STATS_AVG_SESSION));
}

void DashboardTheme::drawDashboardRow(const GfxRenderer& renderer, Rect rect, const RecentBook* book, bool hasBook,
                                      const BookReadingStats* stats, float progressPercent) const {
  if (!hasBook || book == nullptr) {
    drawCoverPanel(renderer, rect, nullptr, false);
    return;
  }

  constexpr int kStatsW = 130;
  const Rect coverRect = coverRectForRow(rect, kStatsW);
  const int statsX = coverRect.x + coverRect.width + kCoverStatsGap;
  const int statsRight = rect.x + rect.width - kContentInset;
  const int statsW = std::max(1, statsRight - statsX);

  drawCoverPanel(renderer, coverRect, book, true);
  drawStatsColumn(renderer, Rect{statsX, coverRect.y, statsW, coverRect.height}, stats, progressPercent);
}

void DashboardTheme::drawRecentBookCover(GfxRenderer& renderer, Rect rect, const std::vector<RecentBook>& recentBooks,
                                         int selectorIndex, bool& coverRendered, bool& coverBufferStored,
                                         bool& bufferRestored, std::function<bool()> storeCoverBuffer,
                                         const BookReadingStats* stats, float progressPercent) const {
  const bool hasBook = !recentBooks.empty();
  const bool bookSelected = hasBook && selectorIndex == 0;

  if (!hasBook) {
    drawDashboardRow(renderer, rect, nullptr, false, nullptr, -1.0f);
    coverRendered = false;
    coverBufferStored = false;
    return;
  }

  if (!coverRendered) {
    drawDashboardRow(renderer, rect, &recentBooks[0], true, stats, progressPercent);
    // Store the buffer BEFORE drawing the selection border so the cached image
    // never has a stale border baked into it.
    coverBufferStored = storeCoverBuffer();
    coverRendered = coverBufferStored;
    if (bookSelected) {
      drawSelectionBorder(renderer, rect);
    }
    return;
  }

  if (bufferRestored && bookSelected) {
    drawSelectionBorder(renderer, rect);
  }
}

void DashboardTheme::drawDashboardSleepScreen(const GfxRenderer& renderer, const RecentBook& book,
                                              const BookReadingStats* stats, float progressPercent) const {
  renderer.clearScreen();
  const Rect rect{0, DashboardMetrics::values.homeTopPadding, renderer.getScreenWidth(),
                  DashboardMetrics::values.homeCoverTileHeight};
  drawDashboardRow(renderer, rect, &book, true, stats, progressPercent);
}
