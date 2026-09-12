#include "DashboardTheme.h"

#include <Bitmap.h>
#include <GfxRenderer.h>
#include <HalClock.h>
#include <HalPowerManager.h>
#include <HalStorage.h>
#include <I18n.h>

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
#include "fontIds.h"
#include "stats/BookReadingStats.h"
#include "stats/GlobalReadingStats.h"
#include "stats/ReadingStatsTypes.h"
#include "stats/ReadingStatsUtils.h"
#include "stats/StatsStore.h"

namespace {
constexpr int kCoverCornerRadius = 8;
constexpr int kContentInset = 16;
constexpr int kCoverStatsGap = 14;
constexpr float kCoverAspect = 296.0f / 444.0f;  // width / height
constexpr int kMainMenuIconSize = 32;

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

  constexpr int textPadding = 10;
  const int textW = std::max(1, coverRect.width - textPadding * 2);
  const char* title = book.title.empty() ? book.path.c_str() : book.title.c_str();
  auto titleLines = renderer.wrappedText(UI_10_FONT_ID, title, textW, 4, EpdFontFamily::BOLD);
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

void drawSelectionBorder(const GfxRenderer& renderer, const Rect& rect) {
  renderer.drawRect(rect.x + 1, rect.y + 1, rect.width - 2, rect.height - 2);
  renderer.drawRect(rect.x + 2, rect.y + 2, rect.width - 4, rect.height - 4);
}

const uint8_t* iconForMenuItem(UIIcon icon) {
  switch (icon) {
    case UIIcon::Folder:
      return FolderIcon;
    case UIIcon::Recent:
      return RecentIcon;
    case UIIcon::Settings:
      return Settings2Icon;
    case UIIcon::Transfer:
      return TransferIcon;
    case UIIcon::Library:
      return LibraryIcon;
    case UIIcon::Bookmark:
      return BookmarkIcon;
    case UIIcon::Stats:
      return StatsIcon;
    default:
      return nullptr;
  }
}
}  // namespace

void DashboardTheme::drawHeader(const GfxRenderer& renderer, Rect rect, const char* title,
                                const char* subtitle) const {
  if (title != nullptr) {
    BaseTheme::drawHeader(renderer, rect, title, subtitle);
    return;
  }

  // Home Screen Header Strip: Date/Clock ("HH:MM, D MMM") on left, Battery widget (% + icon) on right
  const bool isRu = (I18N.getLanguage() == Language::RU);

  // Clock / Date string
  ReadingStatsDateTime now;
  char timeStr[48] = {0};
  if (getCurrentLocalReadingStatsDateTime(now) && now.isValid()) {
    static const char* const kMonthsEn[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                              "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    static const char* const kMonthsRu[12] = {"янв", "фев", "мар", "апр", "май", "июн",
                                              "июл", "авг", "сен", "окт", "ноя", "дек"};
    const int mIdx = std::clamp(static_cast<int>(now.date.month) - 1, 0, 11);
    const char* mName = isRu ? kMonthsRu[mIdx] : kMonthsEn[mIdx];
    snprintf(timeStr, sizeof(timeStr), "%02u:%02u, %u %s",
             static_cast<unsigned>(now.hour), static_cast<unsigned>(now.minute),
             static_cast<unsigned>(now.date.day), mName);
  } else {
    char timeBuf[16] = {0};
    if (halClock.formatTime(timeBuf, sizeof(timeBuf), SETTINGS.clockUtcOffsetQ, SETTINGS.clockFormat == 1)) {
      snprintf(timeStr, sizeof(timeStr), "%s", timeBuf);
    } else {
      snprintf(timeStr, sizeof(timeStr), "BookPoint");
    }
  }

  const int textY = rect.y + (rect.height - renderer.getLineHeight(UI_10_FONT_ID)) / 2;
  renderer.drawText(UI_10_FONT_ID, rect.x + BaseMetrics::values.contentSidePadding, textY, timeStr, true,
                    EpdFontFamily::BOLD);

  // Battery widget: Percentage + Battery Icon
  const uint16_t percentage = powerManager.getBatteryPercentage();
  char batStr[16];
  snprintf(batStr, sizeof(batStr), "%u%%", static_cast<unsigned>(percentage));
  const int batTextW = renderer.getTextWidth(SMALL_FONT_ID, batStr);
  const int batIconW = 20;
  const int batIconH = 10;
  const int batRight = rect.x + rect.width - BaseMetrics::values.contentSidePadding;
  const int batIconX = batRight - batIconW;
  const int batIconY = rect.y + (rect.height - batIconH) / 2;
  const int batTextX = batIconX - batTextW - 6;
  const int batTextY = rect.y + (rect.height - renderer.getLineHeight(SMALL_FONT_ID)) / 2;

  renderer.drawText(SMALL_FONT_ID, batTextX, batTextY, batStr);
  renderer.drawRect(batIconX, batIconY, batIconW - 2, batIconH);
  renderer.fillRect(batIconX + batIconW - 2, batIconY + 2, 2, batIconH - 4);
  const int fillW = std::clamp(static_cast<int>((batIconW - 4) * percentage / 100), 0, batIconW - 4);
  if (fillW > 0) {
    renderer.fillRect(batIconX + 1, batIconY + 1, fillW, batIconH - 2);
  }

  // Thin header separator line
  renderer.drawLine(rect.x + BaseMetrics::values.contentSidePadding, rect.y + rect.height - 1,
                    rect.x + rect.width - BaseMetrics::values.contentSidePadding, rect.y + rect.height - 1, true);
}

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

void DashboardTheme::drawStatsColumn(const GfxRenderer& renderer, Rect rect, const RecentBook* book,
                                     const BookReadingStats* stats, float progressPercent) const {
  const BookReadingStats emptyStats{};
  const BookReadingStats& bookStats = stats != nullptr ? *stats : emptyStats;
  const bool isRu = (I18N.getLanguage() == Language::RU);

  // Right card container
  renderer.fillRoundedRect(rect.x, rect.y, rect.width, rect.height, kCoverCornerRadius, Color::White);
  renderer.drawRoundedRect(rect.x, rect.y, rect.width, rect.height, 1, kCoverCornerRadius, true);

  int currentY = rect.y + 12;

  // 1. Book Title (bold, wrapped, up to 2 lines)
  if (book != nullptr) {
    const char* titleStr = book->title.empty() ? book->path.c_str() : book->title.c_str();
    auto titleLines = renderer.wrappedText(UI_10_FONT_ID, titleStr, rect.width - 20, 2, EpdFontFamily::BOLD);
    const int titleLineH = renderer.getLineHeight(UI_10_FONT_ID);
    for (const auto& line : titleLines) {
      renderer.drawText(UI_10_FONT_ID, rect.x + 10, currentY, line.c_str(), true, EpdFontFamily::BOLD);
      currentY += titleLineH;
    }

    // Author
    if (!book->author.empty()) {
      const std::string truncAuthor = renderer.truncatedText(SMALL_FONT_ID, book->author.c_str(), rect.width - 20);
      renderer.drawText(SMALL_FONT_ID, rect.x + 10, currentY + 2, truncAuthor.c_str(), true);
      currentY += renderer.getLineHeight(SMALL_FONT_ID) + 4;
    } else {
      currentY += 4;
    }
  }

  // Divider line
  renderer.drawLine(rect.x + 10, currentY, rect.x + rect.width - 10, currentY, true);
  currentY += 8;

  // 2. Progress Bar with Percentage
  const int barW = rect.width - 20;
  const int barH = 6;
  const int barX = rect.x + 10;
  const int barY = currentY;
  renderer.drawRoundedRect(barX, barY, barW, barH, 1, 2, true);

  int pct = (progressPercent >= 0.0f) ? static_cast<int>(progressPercent + 0.5f) : 0;
  if (pct > 100) pct = 100;
  if (pct > 0) {
    const int fillW = std::clamp((barW - 2) * pct / 100, 1, barW - 2);
    renderer.fillRect(barX + 1, barY + 1, fillW, barH - 2);
  }
  currentY += barH + 4;

  // Percentage & Estimated Time Left ("Осталось: X ч Y мин")
  char progLine[64];
  uint32_t estSeconds = 0;
  const bool hasEst = fallbackEstimatedTimeLeft(bookStats, progressPercent, estSeconds);

  char durBuf[32];
  if (hasEst && !bookStats.isFinished && estSeconds > 0) {
    BookReadingStats::formatDuration(estSeconds, durBuf, sizeof(durBuf));
    snprintf(progLine, sizeof(progLine), isRu ? "%d%% • Осталось: %s" : "%d%% • Left: %s", pct, durBuf);
  } else {
    snprintf(progLine, sizeof(progLine), isRu ? "%d%% • Осталось: —" : "%d%% • Left: —", pct);
  }
  renderer.drawText(SMALL_FONT_ID, rect.x + 10, currentY, progLine);
  currentY += renderer.getLineHeight(SMALL_FONT_ID) + 8;

  // Divider line
  renderer.drawLine(rect.x + 10, currentY, rect.x + rect.width - 10, currentY, true);
  currentY += 8;

  // 3. Reading Streak Calendar (7-day mini-calendar row)
  const GlobalReadingStats globalStats = StatsStore::loadGlobalStats();
  ReadingStatsDateTime now;
  uint32_t todayDayIndex = 0;
  uint8_t todayDow = 0;
  if (getCurrentLocalReadingStatsDateTime(now) && now.isValid()) {
    todayDayIndex = readingStatsDayIndex(now.date);
    todayDow = readingStatsDayOfWeekIndex(now.date);
  }
  const uint16_t streak = globalStats.currentReadingStreak(todayDayIndex);

  char streakLabel[40];
  snprintf(streakLabel, sizeof(streakLabel), isRu ? "Серия: %u дн." : "Streak: %u d.", static_cast<unsigned>(streak));
  renderer.drawText(SMALL_FONT_ID, rect.x + 10, currentY, streakLabel, true, EpdFontFamily::BOLD);
  currentY += renderer.getLineHeight(SMALL_FONT_ID) + 4;

  // 7-day row
  static const char* const kDowRu[7] = {"П", "В", "С", "Ч", "П", "С", "В"};
  static const char* const kDowEn[7] = {"M", "T", "W", "T", "F", "S", "S"};

  constexpr int boxSize = 14;
  constexpr int boxGap = 4;
  const int rowW = 7 * boxSize + 6 * boxGap;
  const int rowStartX = rect.x + 10;

  for (int i = 0; i < 7; ++i) {
    const int daysAgo = 6 - i;
    const int bX = rowStartX + i * (boxSize + boxGap);
    const int dow = (todayDow - daysAgo + 70) % 7;
    const char* dowChar = isRu ? kDowRu[dow] : kDowEn[dow];

    // Day letter
    const int dwW = renderer.getTextWidth(SMALL_FONT_ID, dowChar);
    renderer.drawText(SMALL_FONT_ID, bX + (boxSize - dwW) / 2, currentY, dowChar);

    // Box
    const int bY = currentY + renderer.getLineHeight(SMALL_FONT_ID) + 2;
    const int32_t secs = globalStats.secondsForDaysAgo(daysAgo);

    if (secs > 0) {
      renderer.fillRect(bX, bY, boxSize, boxSize);
    } else {
      renderer.drawRect(bX, bY, boxSize, boxSize);
    }

    // Highlight today (daysAgo == 0) with double frame
    if (daysAgo == 0) {
      renderer.drawRect(bX - 2, bY - 2, boxSize + 4, boxSize + 4);
    }
  }
}

void DashboardTheme::drawDashboardRow(const GfxRenderer& renderer, Rect rect, const RecentBook* book, bool hasBook,
                                      const BookReadingStats* stats, float progressPercent) const {
  if (!hasBook || book == nullptr) {
    drawCoverPanel(renderer, rect, nullptr, false);
    return;
  }

  constexpr int kStatsW = 200;
  const Rect coverRect = coverRectForRow(rect, kStatsW);
  const int statsX = coverRect.x + coverRect.width + kCoverStatsGap;
  const int statsRight = rect.x + rect.width - kContentInset;
  const int statsW = std::max(1, statsRight - statsX);

  drawCoverPanel(renderer, coverRect, book, true);
  drawStatsColumn(renderer, Rect{statsX, coverRect.y, statsW, coverRect.height}, book, stats, progressPercent);
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

void DashboardTheme::drawButtonMenu(GfxRenderer& renderer, Rect rect, int buttonCount, int selectedIndex,
                                    const std::function<std::string(int index)>& buttonLabel,
                                    const std::function<UIIcon(int index)>& rowIcon) const {
  const auto& metrics = DashboardMetrics::values;
  for (int i = 0; i < buttonCount; ++i) {
    const int tileW = rect.width - metrics.contentSidePadding * 2;
    const Rect tileRect{rect.x + metrics.contentSidePadding,
                        rect.y + i * (metrics.menuRowHeight + metrics.menuSpacing), tileW,
                        metrics.menuRowHeight};

    const bool selected = selectedIndex == i;
    if (selected) {
      renderer.fillRoundedRect(tileRect.x, tileRect.y, tileRect.width, tileRect.height, kCoverCornerRadius,
                               Color::LightGray);
    }

    const std::string labelStr = buttonLabel(i);
    int textX = tileRect.x + 16;
    const int lineH = renderer.getLineHeight(UI_12_FONT_ID);
    const int textY = tileRect.y + (metrics.menuRowHeight - lineH) / 2;

    const bool isSearchItem = (labelStr == tr(STR_SEARCH_BOOKS) || labelStr == tr(STR_SEARCH));

    if (isSearchItem) {
      renderer.drawIcon(icon_search_32_bits, textX, textY, kMainMenuIconSize);
      textX += kMainMenuIconSize + 10;
    } else if (rowIcon != nullptr) {
      const UIIcon icon = rowIcon(i);
      const uint8_t* iconBitmap = iconForMenuItem(icon);
      if (iconBitmap != nullptr) {
        renderer.drawIcon(iconBitmap, textX, textY, kMainMenuIconSize);
        textX += kMainMenuIconSize + 10;
      }
    }

    renderer.drawText(UI_12_FONT_ID, textX, textY, labelStr.c_str(), true);
  }
}
