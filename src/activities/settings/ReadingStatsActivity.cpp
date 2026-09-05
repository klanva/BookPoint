#include "ReadingStatsActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>
#include <Logging.h>

#include <algorithm>
#include <cstdio>
#include <vector>

#include "stats/StatsStore.h"
#include "stats/ReadingStatsTypes.h"
#include "stats/ReadingStatsUtils.h"
#include "../../RecentBooksStore.h"
#include "../../util/BookCacheUtils.h"
#include "components/UITheme.h"
#include "fontIds.h"
#include "MappedInputManager.h"

namespace {
float pagesPerMinute(const uint32_t totalPagesTurned, const uint32_t totalReadingSeconds) {
  if (totalReadingSeconds <= 60) return 0.0f;
  return static_cast<float>(totalPagesTurned) * 60.0f / static_cast<float>(totalReadingSeconds);
}
}  // namespace

ReadingStatsActivity::ReadingStatsActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                                           const std::string& bookPath, const std::string& bookTitle,
                                           const std::string& bookAuthor, const float bookProgressPercent,
                                           const uint32_t estimatedSecondsLeft)
    : Activity("ReadingStats", renderer, mappedInput),
      bookPath_(bookPath),
      bookTitle_(bookTitle),
      bookAuthor_(bookAuthor),
      bookProgressPercent_(bookProgressPercent),
      estimatedSecondsLeft_(estimatedSecondsLeft),
      launchedWithBook_(!bookPath.empty()) {}

void ReadingStatsActivity::onEnter() {
  Activity::onEnter();
  savedOrientation_ = renderer.getOrientation();
  renderer.setOrientation(GfxRenderer::Orientation::Portrait);

  if (bookPath_.empty()) {
    const auto& recents = RECENT_BOOKS.getBooks();
    if (!recents.empty()) {
      bookPath_ = recents[0].path;
      bookTitle_ = recents[0].title;
      bookAuthor_ = recents[0].author;
    }
  }

  if (!bookPath_.empty()) {
    bookStats_ = StatsStore::loadBookStats(bookPath_, bookTitle_, bookAuthor_);
  }
  globalStats_ = StatsStore::loadGlobalStats();

  ReadingStatsDateTime now;
  if (getCurrentLocalReadingStatsDateTime(now)) {
    todayDayIndex_ = readingStatsDayIndex(now.date);
  }

  page_ = firstPage();
  requestUpdate();
}

void ReadingStatsActivity::onExit() {
  renderer.setOrientation(savedOrientation_);
  Activity::onExit();
}

ReadingStatsActivity::Page ReadingStatsActivity::getPageForTab(const int tabIndex) const {
  return static_cast<Page>(tabIndex);
}

int ReadingStatsActivity::getTabForPage(const Page page) const {
  return static_cast<int>(page);
}

void ReadingStatsActivity::cyclePage(const int delta) {
  const int count = static_cast<int>(Page::PAGE_COUNT);
  int page = (static_cast<int>(page_) + delta + count) % count;
  page_ = static_cast<Page>(page);
  requestUpdate();
}

void ReadingStatsActivity::exportCsv() {
  if (StatsStore::exportCsv()) {
    message_ = std::string(tr(STR_STATS_EXPORTED)) + " " + StatsStore::csvPath();
  } else {
    message_ = tr(STR_STATS_EXPORT_FAILED);
  }
  messageVisible_ = true;
  messageShownAt_ = millis();
  requestUpdate();
}

void ReadingStatsActivity::handleConfirm() {
  switch (page_) {
    case Page::Book:
      StatsStore::setBookFinished(bookPath_, !bookStats_.isFinished);
      bookStats_ = StatsStore::loadBookStats(bookPath_, bookTitle_, bookAuthor_);
      globalStats_ = StatsStore::loadGlobalStats();
      message_ = bookStats_.isFinished ? tr(STR_STATS_MARKED_FINISHED) : tr(STR_STATS_MARKED_UNFINISHED);
      messageVisible_ = true;
      messageShownAt_ = millis();
      requestUpdate();
      break;
    case Page::Device:
      exportCsv();
      break;
    case Page::Activity:
    default:
      break;
  }
}

bool ReadingStatsActivity::hasAnyBookData() const {
  return bookStats_.totalReadingSeconds > 0 || bookStats_.sessionCount > 0 || bookStats_.totalPagesTurned > 0;
}

void ReadingStatsActivity::loop() {
  if (messageVisible_ && millis() - messageShownAt_ > 2500) {
    messageVisible_ = false;
    requestUpdate();
  }

  // Handle swipe gestures (touch fallback)
  const auto swipe = mappedInput.wasSwipe();
  if (swipe == MappedInputManager::SwipeDir::Left) {
    cyclePage(+1);
    return;
  }
  if (swipe == MappedInputManager::SwipeDir::Right) {
    cyclePage(-1);
    return;
  }

  // Handle screen taps (touch fallback)
  int tx = 0, ty = 0;
  if (mappedInput.wasScreenTapped(tx, ty)) {
    // Top tab pills
    if (ty >= tabBarY_ && ty < tabBarY_ + tabBarH_) {
      const int count = getTabCount();
      for (int i = 0; i < count; ++i) {
        if (tx >= tabRects_[i].x && tx < tabRects_[i].x + tabRects_[i].width) {
          const Page targetPage = getPageForTab(i);
          if (page_ != targetPage) {
            page_ = targetPage;
            requestUpdate();
          }
          return;
        }
      }
    }

    // Action button
    if (actionButtonVisible_ && tx >= actionButtonRect_.x && tx < actionButtonRect_.x + actionButtonRect_.width &&
        ty >= actionButtonRect_.y && ty < actionButtonRect_.y + actionButtonRect_.height) {
      handleConfirm();
      return;
    }
  }

  // Physical buttons (optimized for Xteink X4 hardware buttons)
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    finish();
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Up) ||
      mappedInput.wasReleased(MappedInputManager::Button::Left) ||
      mappedInput.wasReleased(MappedInputManager::Button::PageBack) ||
      mappedInput.wasReleased(MappedInputManager::Button::NavPrevious)) {
    cyclePage(-1);
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Down) ||
      mappedInput.wasReleased(MappedInputManager::Button::Right) ||
      mappedInput.wasReleased(MappedInputManager::Button::PageForward) ||
      mappedInput.wasReleased(MappedInputManager::Button::NavNext)) {
    cyclePage(+1);
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm) ||
      mappedInput.wasReleased(MappedInputManager::Button::Power)) {
    handleConfirm();
    return;
  }
}

void ReadingStatsActivity::renderTabBar(const int x, const int y, const int width, const int height) {
  const int count = getTabCount();
  tabBarY_ = y;
  tabBarH_ = height;
  const int gap = 6;
  const int pillW = (width - (count - 1) * gap) / count;

  for (int i = 0; i < count; ++i) {
    const int pillX = x + i * (pillW + gap);
    tabRects_[i] = {pillX, y, pillW, height};

    const Page tabPg = getPageForTab(i);
    const bool active = (tabPg == page_);

    const char* label = "";
    if (tabPg == Page::Book) {
      label = I18N.get(StrId::STR_BOOK);
    } else if (tabPg == Page::Device) {
      label = I18N.get(StrId::STR_DEVICE);
    } else {
      label = I18N.get(StrId::STR_STATS_ACTIVITY);
    }

    const std::string safeLabel = renderer.truncatedText(UI_10_FONT_ID, label, pillW - 12);
    const int textH = renderer.getLineHeight(UI_10_FONT_ID);

    if (active) {
      renderer.fillRoundedRect(pillX, y, pillW, height, 6, Color::Black);
      const int textW = renderer.getTextAdvanceX(UI_10_FONT_ID, safeLabel.c_str(), EpdFontFamily::BOLD);
      renderer.drawText(UI_10_FONT_ID, pillX + (pillW - textW) / 2, y + (height - textH) / 2, safeLabel.c_str(), false,
                        EpdFontFamily::BOLD);
    } else {
      renderer.drawRoundedRect(pillX, y, pillW, height, 1, 6, true);
      const int textW = renderer.getTextAdvanceX(UI_10_FONT_ID, safeLabel.c_str(), EpdFontFamily::REGULAR);
      renderer.drawText(UI_10_FONT_ID, pillX + (pillW - textW) / 2, y + (height - textH) / 2, safeLabel.c_str(), true,
                        EpdFontFamily::REGULAR);
    }
  }
}

void ReadingStatsActivity::drawBarChart(const int x, const int y, const int width, const int height,
                                        const uint16_t* values, const int count, const int highlightIndex) const {
  if (count <= 0) return;

  uint16_t maxValue = 1;
  for (int i = 0; i < count; ++i) {
    if (values[i] > maxValue) maxValue = values[i];
  }

  const int slotWidth = width / count;
  const int barWidth = slotWidth > 8 ? slotWidth - 4 : (slotWidth > 4 ? slotWidth - 2 : slotWidth);
  const int chartActualW = count * slotWidth;
  const int offsetX = x + (width - chartActualW) / 2;
  const int baseline = y + height;

  for (int i = 0; i < count; ++i) {
    const int barX = offsetX + i * slotWidth + (slotWidth - barWidth) / 2;
    int barHeight = static_cast<int>((static_cast<uint32_t>(values[i]) * height) / maxValue);
    if (values[i] > 0 && barHeight < 3) barHeight = 3;

    if (barHeight > 0) {
      renderer.fillRect(barX, baseline - barHeight, barWidth, barHeight, true);
    } else {
      renderer.fillRect(barX, baseline - 2, barWidth, 2, true);
    }

    if (i == highlightIndex) {
      // Crisp solid 2px accent underline under the baseline for the current day
      renderer.fillRect(barX - 1, baseline + 2, barWidth + 2, 2, true);
    }
  }
  renderer.drawLine(offsetX, baseline, offsetX + chartActualW, baseline, 1, true);
}

std::string ReadingStatsActivity::formatDay(const int daysAgo, const uint32_t anchorDayIndex) {
  ReadingStatsDate date;
  if (anchorDayIndex == 0 || !readingStatsDateFromDayIndex(anchorDayIndex - daysAgo, date)) return "-";
  char buf[16];
  snprintf(buf, sizeof(buf), "%02u.%02u", date.day, date.month);
  return buf;
}

static void drawCenteredLabel(const GfxRenderer& renderer, const int fontId, const int x, const int w, const int y,
                              const char* text, const bool bold = false) {
  const int textWidth = renderer.getTextWidth(fontId, text, bold ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR);
  renderer.drawText(fontId, x + (w - textWidth) / 2, y, text, true,
                    bold ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR);
}

static void drawStatCell(const GfxRenderer& renderer, const int x, const int w, const int y, const int h,
                         const char* value, const char* label) {
  const int valueLineH = renderer.getLineHeight(UI_12_FONT_ID);
  const int labelLineH = renderer.getLineHeight(SMALL_FONT_ID);
  const int totalTextH = valueLineH + 3 + labelLineH;
  const int textY = y + (h - totalTextH) / 2;
  drawCenteredLabel(renderer, UI_12_FONT_ID, x, w, textY, value, true);
  drawCenteredLabel(renderer, SMALL_FONT_ID, x, w, textY + valueLineH + 3, label);
}

void ReadingStatsActivity::renderBookPage(const int x, int y, const int contentWidth) {
  // --- 1. Hero Book Card ---
  const int heroH = 76;
  renderer.drawRoundedRect(x, y, contentWidth, heroH, 1, 8, true);

  std::string title = bookStats_.bookTitle.empty() ? bookTitle_ : bookStats_.bookTitle;
  if (title.empty()) title = tr(STR_UNNAMED);
  renderer.drawText(UI_12_FONT_ID, x + 12, y + 8,
                    renderer.truncatedText(UI_12_FONT_ID, title.c_str(), contentWidth - 24, EpdFontFamily::BOLD).c_str(), true,
                    EpdFontFamily::BOLD);

  std::string author = !bookStats_.bookAuthor.empty() ? bookStats_.bookAuthor : bookAuthor_;
  if (!author.empty()) {
    renderer.drawText(UI_10_FONT_ID, x + 12, y + 27,
                      renderer.truncatedText(UI_10_FONT_ID, author.c_str(), contentWidth - 24).c_str(), true);
  }

  // Progress bar
  const int percent = bookProgressPercent_ >= 0.0f ? static_cast<int>(bookProgressPercent_ + 0.5f) : 0;
  char pctBuf[16];
  snprintf(pctBuf, sizeof(pctBuf), "%d%%", percent);
  const int pctW = renderer.getTextAdvanceX(UI_12_FONT_ID, pctBuf, EpdFontFamily::BOLD);
  const int barW = contentWidth - 24 - pctW - 10;
  const int barY = y + 49;
  renderer.drawRoundedRect(x + 12, barY, barW, 9, 1, 4, true);
  if (percent > 0) {
    const int fillW = (barW - 2) * std::min(percent, 100) / 100;
    if (fillW > 0) renderer.fillRect(x + 13, barY + 1, fillW, 7, true);
  }
  renderer.drawText(UI_12_FONT_ID, x + 12 + barW + 10, barY - 3, pctBuf, true, EpdFontFamily::BOLD);

  y += heroH + 10;

  // --- 2. 3x2 KPI Grid Card (Reference Style) ---
  const int statsCardH = 116;
  renderer.drawRoundedRect(x, y, contentWidth, statsCardH, 1, 8, true);
  renderer.drawLine(x, y + statsCardH / 2, x + contentWidth, y + statsCardH / 2, true);

  const int thirdW = contentWidth / 3;
  const int rowH = statsCardH / 2;
  char buf[64];

  // Row 1, Col 1: Sessions
  snprintf(buf, sizeof(buf), "%u", bookStats_.sessionCount);
  drawStatCell(renderer, x, thirdW, y, rowH, buf, tr(STR_STATS_SESSIONS));

  // Row 1, Col 2: Total Time
  BookReadingStats::formatDuration(bookStats_.totalReadingSeconds, buf, sizeof(buf));
  drawStatCell(renderer, x + thirdW, thirdW, y, rowH, buf, tr(STR_STATS_TOTAL_TIME));

  // Row 1, Col 3: Progress
  if (bookProgressPercent_ >= 0.0f) {
    snprintf(buf, sizeof(buf), "%d%%", percent);
  } else {
    snprintf(buf, sizeof(buf), "—");
  }
  drawStatCell(renderer, x + thirdW * 2, contentWidth - thirdW * 2, y, rowH, buf, tr(STR_STATS_PROGRESS));

  // Row 2, Col 1: Average Session
  const uint32_t avgSecs = bookStats_.sessionCount > 0 ? bookStats_.totalReadingSeconds / bookStats_.sessionCount : 0;
  BookReadingStats::formatDuration(avgSecs, buf, sizeof(buf));
  drawStatCell(renderer, x, thirdW, y + rowH, rowH, buf, tr(STR_STATS_AVG_SESSION));

  // Row 2, Col 2: Time Left
  uint32_t timeLeftSec = estimatedSecondsLeft_;
  if (timeLeftSec == 0 && percent < 100) {
    fallbackEstimatedTimeLeft(bookStats_, bookProgressPercent_, timeLeftSec);
  }
  if (timeLeftSec > 0 && percent < 100) {
    BookReadingStats::formatDuration(timeLeftSec, buf, sizeof(buf));
  } else {
    snprintf(buf, sizeof(buf), "—");
  }
  drawStatCell(renderer, x + thirdW, thirdW, y + rowH, rowH, buf, tr(STR_STATS_TIME_LEFT));

  // Row 2, Col 3: Pages per Minute
  snprintf(buf, sizeof(buf), "%.1f", pagesPerMinute(bookStats_.totalPagesTurned, bookStats_.totalReadingSeconds));
  drawStatCell(renderer, x + thirdW * 2, contentWidth - thirdW * 2, y + rowH, rowH, buf, tr(STR_STATS_PAGES_PER_MIN));

  y += statsCardH + 10;

  // --- 3. Reading Timeline & Pace Details Card ---
  const int detailsH = 104;
  renderer.drawRoundedRect(x, y, contentWidth, detailsH, 1, 8, true);
  renderer.drawLine(x, y + detailsH / 2, x + contentWidth, y + detailsH / 2, true);

  const int halfW = contentWidth / 2;
  const int detailRowH = detailsH / 2;

  // Detail Row 1, Col 1: Started Date
  const auto formatDate = [](const ReadingStatsDate& d) {
    if (!d.isValid()) return std::string("—");
    char dateBuf[16];
    snprintf(dateBuf, sizeof(dateBuf), "%02u.%02u.%04u", d.day, d.month, d.year);
    return std::string(dateBuf);
  };
  drawStatCell(renderer, x, halfW, y, detailRowH, formatDate(bookStats_.startDate).c_str(), tr(STR_STATS_STARTED));

  // Detail Row 1, Col 2: Finish Date / Estimate
  std::string finishStr = "—";
  if (bookStats_.isFinished && bookStats_.finishedDate.isValid()) {
    finishStr = formatDate(bookStats_.finishedDate);
  } else if (!bookStats_.isFinished && percent < 100) {
    ReadingStatsDateTime now;
    ReadingStatsDate finishDate;
    if (timeLeftSec > 0 && getCurrentLocalReadingStatsDateTime(now) &&
        estimateFinishDateFromDailyPace(bookStats_, now, timeLeftSec, finishDate)) {
      char dateBuf[24];
      formatFinishDate(finishDate, dateBuf, sizeof(dateBuf));
      finishStr = dateBuf;
    }
  }
  const char* finishLabel = bookStats_.isFinished ? tr(STR_STATS_FINISHED) : tr(STR_STATS_FINISH_ESTIMATE);
  drawStatCell(renderer, x + halfW, contentWidth - halfW, y, detailRowH, finishStr.c_str(), finishLabel);

  // Detail Row 2, Col 1: Reading Pace
  if (bookStats_.avgSecondsPerForwardPage > 0) {
    snprintf(buf, sizeof(buf), "%u с/стр", bookStats_.avgSecondsPerForwardPage);
  } else {
    snprintf(buf, sizeof(buf), "—");
  }
  drawStatCell(renderer, x, halfW, y + detailRowH, detailRowH, buf, tr(STR_STATS_PACE));

  // Detail Row 2, Col 2: Total Pages Read
  snprintf(buf, sizeof(buf), "%lu", static_cast<unsigned long>(bookStats_.totalPagesTurned));
  drawStatCell(renderer, x + halfW, contentWidth - halfW, y + detailRowH, detailRowH, buf, tr(STR_STATS_PAGES_TURNED));

  y += detailsH + 14;

  // --- 4. Interactive Action Button ---
  const int btnH = 40;
  actionButtonRect_ = {x, y, contentWidth, btnH};
  actionButtonVisible_ = true;

  const char* btnText = bookStats_.isFinished ? tr(STR_STATS_MARK_UNFINISHED) : tr(STR_STATS_MARK_FINISHED);
  const std::string safeBtnText = renderer.truncatedText(UI_12_FONT_ID, btnText, contentWidth - 24, EpdFontFamily::BOLD);
  const int tw = renderer.getTextAdvanceX(UI_12_FONT_ID, safeBtnText.c_str(), EpdFontFamily::BOLD);
  const int th = renderer.getLineHeight(UI_12_FONT_ID);

  if (bookStats_.isFinished) {
    renderer.drawRoundedRect(x, y, contentWidth, btnH, 2, 8, true);
    renderer.drawText(UI_12_FONT_ID, x + (contentWidth - tw) / 2, y + (btnH - th) / 2, safeBtnText.c_str(), true,
                      EpdFontFamily::BOLD);
  } else {
    renderer.fillRoundedRect(x, y, contentWidth, btnH, 8, Color::Black);
    renderer.drawText(UI_12_FONT_ID, x + (contentWidth - tw) / 2, y + (btnH - th) / 2, safeBtnText.c_str(), false,
                      EpdFontFamily::BOLD);
  }
}

void ReadingStatsActivity::renderDevicePage(const int x, int y, const int contentWidth) {
  // --- 1. Hero Total Reading Time ---
  const int heroH = 74;
  renderer.drawRoundedRect(x, y, contentWidth, heroH, 1, 8, true);
  renderer.drawText(UI_10_FONT_ID, x + 12, y + 10,
                    renderer.truncatedText(UI_10_FONT_ID, tr(STR_STATS_TOTAL_TIME), contentWidth - 24).c_str());
  char buf[96];
  BookReadingStats::formatDuration(globalStats_.totalReadingSeconds, buf, sizeof(buf));
  renderer.drawText(UI_12_FONT_ID, x + 12, y + 32,
                    renderer.truncatedText(UI_12_FONT_ID, buf, contentWidth - 24, EpdFontFamily::BOLD).c_str(), true,
                    EpdFontFamily::BOLD);

  y += heroH + 8;

  // --- 2. Streaks Card ---
  const int streakH = 66;
  const int halfW = contentWidth / 2;
  renderer.drawRoundedRect(x, y, contentWidth, streakH, 1, 6, true);
  renderer.drawLine(x + halfW, y + 10, x + halfW, y + 56, true);

  renderer.drawText(UI_10_FONT_ID, x + 12, y + 10,
                    renderer.truncatedText(UI_10_FONT_ID, tr(STR_STATS_CURRENT_STREAK), halfW - 20).c_str());
  char streakBuf[32];
  snprintf(streakBuf, sizeof(streakBuf), "%u %s", globalStats_.currentReadingStreak(todayDayIndex_),
           tr(STR_STATS_DAYS));
  renderer.drawText(UI_12_FONT_ID, x + 12, y + 30,
                    renderer.truncatedText(UI_12_FONT_ID, streakBuf, halfW - 20, EpdFontFamily::BOLD).c_str(), true,
                    EpdFontFamily::BOLD);

  renderer.drawText(UI_10_FONT_ID, x + halfW + 12, y + 10,
                    renderer.truncatedText(UI_10_FONT_ID, tr(STR_STATS_LONGEST_STREAK), halfW - 24).c_str());
  char longestBuf[32];
  snprintf(longestBuf, sizeof(longestBuf), "%u %s", globalStats_.displayLongestReadingStreak(), tr(STR_STATS_DAYS));
  renderer.drawText(UI_12_FONT_ID, x + halfW + 12, y + 30,
                    renderer.truncatedText(UI_12_FONT_ID, longestBuf, halfW - 24, EpdFontFamily::BOLD).c_str(), true,
                    EpdFontFamily::BOLD);

  y += streakH + 8;

  // --- 3. 2x2 Lifetime Tiles ---
  const int gap = 8;
  const int tileW = (contentWidth - gap) / 2;
  const int tileH = 54;
  const int x1 = x;
  const int x2 = x + tileW + gap;

  // Tile 1: Books Finished
  renderer.drawRoundedRect(x1, y, tileW, tileH, 1, 6, true);
  renderer.drawText(UI_10_FONT_ID, x1 + 10, y + 8,
                    renderer.truncatedText(UI_10_FONT_ID, tr(STR_STATS_BOOKS_FINISHED), tileW - 20).c_str());
  snprintf(buf, sizeof(buf), "%lu", static_cast<unsigned long>(globalStats_.completedBooks));
  renderer.drawText(UI_12_FONT_ID, x1 + 10, y + 26,
                    renderer.truncatedText(UI_12_FONT_ID, buf, tileW - 20, EpdFontFamily::BOLD).c_str(), true,
                    EpdFontFamily::BOLD);

  // Tile 2: Total Pages
  renderer.drawRoundedRect(x2, y, tileW, tileH, 1, 6, true);
  renderer.drawText(UI_10_FONT_ID, x2 + 10, y + 8,
                    renderer.truncatedText(UI_10_FONT_ID, tr(STR_STATS_PAGES_TURNED), tileW - 20).c_str());
  snprintf(buf, sizeof(buf), "%lu", static_cast<unsigned long>(globalStats_.totalPagesTurned));
  renderer.drawText(UI_12_FONT_ID, x2 + 10, y + 26,
                    renderer.truncatedText(UI_12_FONT_ID, buf, tileW - 20, EpdFontFamily::BOLD).c_str(), true,
                    EpdFontFamily::BOLD);

  const int yRow2 = y + tileH + gap;

  // Tile 3: Total Sessions
  renderer.drawRoundedRect(x1, yRow2, tileW, tileH, 1, 6, true);
  renderer.drawText(UI_10_FONT_ID, x1 + 10, yRow2 + 8,
                    renderer.truncatedText(UI_10_FONT_ID, tr(STR_STATS_SESSIONS), tileW - 20).c_str());
  snprintf(buf, sizeof(buf), "%lu", static_cast<unsigned long>(globalStats_.totalSessions));
  renderer.drawText(UI_12_FONT_ID, x1 + 10, yRow2 + 26,
                    renderer.truncatedText(UI_12_FONT_ID, buf, tileW - 20, EpdFontFamily::BOLD).c_str(), true,
                    EpdFontFamily::BOLD);

  // Tile 4: Average per Session
  renderer.drawRoundedRect(x2, yRow2, tileW, tileH, 1, 6, true);
  renderer.drawText(UI_10_FONT_ID, x2 + 10, yRow2 + 8,
                    renderer.truncatedText(UI_10_FONT_ID, tr(STR_STATS_AVG_SESSION), tileW - 20).c_str());
  const uint32_t avgSec = globalStats_.totalSessions > 0
                              ? static_cast<uint32_t>(globalStats_.totalReadingSeconds / globalStats_.totalSessions)
                              : 0;
  BookReadingStats::formatDuration(avgSec, buf, sizeof(buf));
  renderer.drawText(UI_12_FONT_ID, x2 + 10, yRow2 + 26,
                    renderer.truncatedText(UI_12_FONT_ID, buf, tileW - 20, EpdFontFamily::BOLD).c_str(), true,
                    EpdFontFamily::BOLD);

  y = yRow2 + tileH + 10;

  // --- 4. Interactive Action Button ---
  const int btnH = 42;
  actionButtonRect_ = {x, y, contentWidth, btnH};
  actionButtonVisible_ = true;

  renderer.drawRoundedRect(x, y, contentWidth, btnH, 2, 8, true);
  const char* btnText = tr(STR_STATS_EXPORT);
  const std::string safeBtnText = renderer.truncatedText(UI_12_FONT_ID, btnText, contentWidth - 24, EpdFontFamily::BOLD);
  const int tw = renderer.getTextAdvanceX(UI_12_FONT_ID, safeBtnText.c_str(), EpdFontFamily::BOLD);
  const int th = renderer.getLineHeight(UI_12_FONT_ID);
  renderer.drawText(UI_12_FONT_ID, x + (contentWidth - tw) / 2, y + (btnH - th) / 2, safeBtnText.c_str(), true,
                    EpdFontFamily::BOLD);
}

void ReadingStatsActivity::renderActivityPage(const int x, int y, const int contentWidth) {
  // --- 1. 14-Day Activity Bar Chart Card ---
  const int chartCardH = 144;
  renderer.drawRoundedRect(x, y, contentWidth, chartCardH, 1, 8, true);
  renderer.drawText(UI_10_FONT_ID, x + 12, y + 10,
                    renderer.truncatedText(UI_10_FONT_ID, tr(STR_STATS_LAST_14_DAYS), contentWidth - 24, EpdFontFamily::BOLD).c_str(),
                    true, EpdFontFamily::BOLD);

  uint16_t days[14];
  for (int i = 0; i < 14; ++i) {
    const int32_t secs = globalStats_.secondsForDaysAgo(13 - i);
    days[i] = secs > 0 ? static_cast<uint16_t>(secs) : 0;
  }
  const int chartH = 66;
  const int chartW = contentWidth - 24;
  drawBarChart(x + 12, y + 34, chartW, chartH, days, 14, 13);

  // Date labels
  renderer.drawText(UI_10_FONT_ID, x + 12, y + 34 + chartH + 6,
                    formatDay(13, globalStats_.anchorDayIndex).c_str());
  const std::string lastLabel = formatDay(0, globalStats_.anchorDayIndex);
  const int lastW = renderer.getTextAdvanceX(UI_10_FONT_ID, lastLabel.c_str(), EpdFontFamily::BOLD);
  renderer.drawText(UI_10_FONT_ID, x + contentWidth - 12 - lastW, y + 34 + chartH + 6, lastLabel.c_str(), true,
                    EpdFontFamily::BOLD);

  y += chartCardH + 8;

  // --- 2. Weekday Distribution Card ---
  const int dowCardH = 104;
  renderer.drawRoundedRect(x, y, contentWidth, dowCardH, 1, 6, true);
  renderer.drawText(UI_10_FONT_ID, x + 12, y + 10,
                    renderer.truncatedText(UI_10_FONT_ID, tr(STR_STATS_BY_WEEKDAY), contentWidth - 24, EpdFontFamily::BOLD).c_str(),
                    true, EpdFontFamily::BOLD);

  static const StrId dowIds[READING_DAY_OF_WEEK_COUNT] = {
      StrId::STR_STATS_DOW_MON, StrId::STR_STATS_DOW_TUE, StrId::STR_STATS_DOW_WED, StrId::STR_STATS_DOW_THU,
      StrId::STR_STATS_DOW_FRI, StrId::STR_STATS_DOW_SAT, StrId::STR_STATS_DOW_SUN};
  const int dowChartH = 42;
  uint16_t dow[READING_DAY_OF_WEEK_COUNT];
  for (size_t i = 0; i < READING_DAY_OF_WEEK_COUNT; ++i) dow[i] = globalStats_.dayOfWeekSeconds[i];
  drawBarChart(x + 12, y + 32, contentWidth - 24, dowChartH, dow, READING_DAY_OF_WEEK_COUNT, -1);

  const int dowSlotW = (contentWidth - 24) / READING_DAY_OF_WEEK_COUNT;
  const int dowOffsetX = x + 12 + ((contentWidth - 24) - (READING_DAY_OF_WEEK_COUNT * dowSlotW)) / 2;
  for (size_t i = 0; i < READING_DAY_OF_WEEK_COUNT; ++i) {
    const char* label = I18N.get(dowIds[i]);
    const std::string safeLabel = renderer.truncatedText(UI_10_FONT_ID, label, dowSlotW);
    const int labelW = renderer.getTextAdvanceX(UI_10_FONT_ID, safeLabel.c_str(), EpdFontFamily::REGULAR);
    const int slotCenter = dowOffsetX + dowSlotW * i + dowSlotW / 2;
    renderer.drawText(UI_10_FONT_ID, slotCenter - labelW / 2, y + 32 + dowChartH + 4, safeLabel.c_str());
  }

  y += dowCardH + 8;

  // --- 3. Time of Day Distribution Card ---
  const int todCardH = 96;
  renderer.drawRoundedRect(x, y, contentWidth, todCardH, 1, 6, true);
  renderer.drawText(UI_10_FONT_ID, x + 12, y + 10,
                    renderer.truncatedText(UI_10_FONT_ID, tr(STR_STATS_BY_TIME_OF_DAY), contentWidth - 24, EpdFontFamily::BOLD).c_str(),
                    true, EpdFontFamily::BOLD);

  static const StrId todIds[READING_TIME_BUCKET_COUNT] = {StrId::STR_STATS_MORNING, StrId::STR_STATS_AFTERNOON,
                                                         StrId::STR_STATS_EVENING, StrId::STR_STATS_NIGHT};
  const int todChartH = 34;
  uint16_t tod[READING_TIME_BUCKET_COUNT];
  for (size_t i = 0; i < READING_TIME_BUCKET_COUNT; ++i) tod[i] = globalStats_.timeOfDaySeconds[i];
  drawBarChart(x + 12, y + 32, contentWidth - 24, todChartH, tod, READING_TIME_BUCKET_COUNT, -1);

  const int todSlotW = (contentWidth - 24) / READING_TIME_BUCKET_COUNT;
  const int todOffsetX = x + 12 + ((contentWidth - 24) - (READING_TIME_BUCKET_COUNT * todSlotW)) / 2;
  for (size_t i = 0; i < READING_TIME_BUCKET_COUNT; ++i) {
    const char* label = I18N.get(todIds[i]);
    const std::string safeLabel = renderer.truncatedText(UI_10_FONT_ID, label, todSlotW);
    const int labelW = renderer.getTextAdvanceX(UI_10_FONT_ID, safeLabel.c_str(), EpdFontFamily::REGULAR);
    const int slotCenter = todOffsetX + todSlotW * i + todSlotW / 2;
    renderer.drawText(UI_10_FONT_ID, slotCenter - labelW / 2, y + 32 + todChartH + 4, safeLabel.c_str());
  }
}

void ReadingStatsActivity::render(RenderLock&&) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();
  const int marginX = 20;
  const int contentWidth = pageWidth - 2 * marginX;

  renderer.clearScreen();
  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, tr(STR_READING_STATS));

  const int tabBarY = metrics.topPadding + metrics.headerHeight + 6;
  const int tabBarH = 32;
  renderTabBar(marginX, tabBarY, contentWidth, tabBarH);

  actionButtonVisible_ = false;
  int contentY = tabBarY + tabBarH + 8;

  const bool noData = (!hasBookPage() || !hasAnyBookData()) && globalStats_.totalReadingSeconds == 0 &&
                      globalStats_.totalSessions == 0;
  if (noData) {
    renderer.drawCenteredText(UI_12_FONT_ID, pageHeight / 2, tr(STR_STATS_NO_DATA), true);
  } else {
    switch (page_) {
      case Page::Book:
        renderBookPage(marginX, contentY, contentWidth);
        break;
      case Page::Device:
        renderDevicePage(marginX, contentY, contentWidth);
        break;
      case Page::Activity:
        renderActivityPage(marginX, contentY, contentWidth);
        break;
      default:
        break;
    }
  }

  // Toast notification
  if (messageVisible_) {
    const int textW = renderer.getTextAdvanceX(UI_10_FONT_ID, message_.c_str(), EpdFontFamily::BOLD);
    const int toastW = std::min(contentWidth, textW + 36);
    const int toastH = 34;
    const int toastX = (pageWidth - toastW) / 2;
    const int toastY = pageHeight - metrics.buttonHintsHeight - 38;
    renderer.fillRoundedRect(toastX, toastY, toastW, toastH, 8, Color::Black);
    const int th = renderer.getLineHeight(UI_10_FONT_ID);
    renderer.drawText(UI_10_FONT_ID, toastX + (toastW - textW) / 2, toastY + (toastH - th) / 2, message_.c_str(),
                      false, EpdFontFamily::BOLD);
  }

  const char* confirmLabel = "";
  switch (page_) {
    case Page::Book:
      confirmLabel = bookStats_.isFinished ? tr(STR_STATS_MARK_UNFINISHED) : tr(STR_STATS_MARK_FINISHED);
      break;
    case Page::Device:
      confirmLabel = tr(STR_STATS_EXPORT);
      break;
    case Page::Activity:
    default:
      confirmLabel = "";
      break;
  }
  const auto labels = mappedInput.mapLabels(tr(STR_BACK), confirmLabel, tr(STR_DIR_LEFT), tr(STR_DIR_RIGHT));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer();
}
