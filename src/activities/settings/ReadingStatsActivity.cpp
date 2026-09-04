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
#include "../../util/BookCacheUtils.h"
#include "components/UITheme.h"
#include "fontIds.h"
#include "MappedInputManager.h"

ReadingStatsActivity::ReadingStatsActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                                           const std::string& bookPath, const std::string& bookTitle,
                                           const std::string& bookAuthor, const float bookProgressPercent,
                                           const uint32_t estimatedSecondsLeft)
    : Activity("ReadingStats", renderer, mappedInput),
      bookPath_(bookPath),
      bookTitle_(bookTitle),
      bookAuthor_(bookAuthor),
      bookProgressPercent_(bookProgressPercent),
      estimatedSecondsLeft_(estimatedSecondsLeft) {}

void ReadingStatsActivity::onEnter() {
  Activity::onEnter();

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

ReadingStatsActivity::Page ReadingStatsActivity::getPageForTab(const int tabIndex) const {
  if (hasBookPage()) {
    return static_cast<Page>(tabIndex);
  }
  return tabIndex == 0 ? Page::Device : Page::Activity;
}

int ReadingStatsActivity::getTabForPage(const Page page) const {
  if (hasBookPage()) {
    return static_cast<int>(page);
  }
  return page == Page::Activity ? 1 : 0;
}

void ReadingStatsActivity::cyclePage(const int delta) {
  const int count = static_cast<int>(Page::PAGE_COUNT);
  int page = static_cast<int>(page_);
  int first = hasBookPage() ? 0 : 1;
  do {
    page = (page + delta + count) % count;
  } while (page < first);
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
    case Page::Activity:
      exportCsv();
      break;
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

  // Handle swipe gestures
  const auto swipe = mappedInput.wasSwipe();
  if (swipe == MappedInputManager::SwipeDir::Left) {
    cyclePage(+1);
    return;
  }
  if (swipe == MappedInputManager::SwipeDir::Right) {
    cyclePage(-1);
    return;
  }

  // Handle screen taps
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

  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    finish();
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Up) ||
      mappedInput.wasReleased(MappedInputManager::Button::Left)) {
    cyclePage(-1);
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Down) ||
      mappedInput.wasReleased(MappedInputManager::Button::Right)) {
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

    if (active) {
      renderer.fillRoundedRect(pillX, y, pillW, height, 6, Color::Black);
      const int textW = renderer.getTextAdvanceX(UI_10_FONT_ID, label, EpdFontFamily::BOLD);
      const int textH = renderer.getLineHeight(UI_10_FONT_ID);
      renderer.drawText(UI_10_FONT_ID, pillX + (pillW - textW) / 2, y + (height - textH) / 2, label, false,
                        EpdFontFamily::BOLD);
    } else {
      renderer.drawRoundedRect(pillX, y, pillW, height, 1, 6, true);
      const int textW = renderer.getTextAdvanceX(UI_10_FONT_ID, label, EpdFontFamily::REGULAR);
      const int textH = renderer.getLineHeight(UI_10_FONT_ID);
      renderer.drawText(UI_10_FONT_ID, pillX + (pillW - textW) / 2, y + (height - textH) / 2, label, true,
                        EpdFontFamily::REGULAR);
    }
  }
}

void ReadingStatsActivity::drawBarChart(const int x, const int y, const int width, const int height,
                                        const uint16_t* values, const int count, const int highlightIndex) const {
  uint16_t maxValue = 1;
  for (int i = 0; i < count; ++i) {
    if (values[i] > maxValue) maxValue = values[i];
  }

  const int slotWidth = width / (count > 0 ? count : 1);
  const int barWidth = slotWidth > 8 ? slotWidth - 4 : (slotWidth > 4 ? slotWidth - 2 : slotWidth);
  const int baseline = y + height;

  for (int i = 0; i < count; ++i) {
    const int barX = x + i * slotWidth + (slotWidth - barWidth) / 2;
    int barHeight = static_cast<int>((static_cast<uint32_t>(values[i]) * height) / maxValue);
    if (values[i] > 0 && barHeight < 3) barHeight = 3;

    if (barHeight > 0) {
      renderer.fillRect(barX, baseline - barHeight, barWidth, barHeight, true);
    } else {
      renderer.fillRect(barX, baseline - 2, barWidth, 2, true);
    }

    if (i == highlightIndex) {
      renderer.drawRect(barX - 2, y - 2, barWidth + 4, height + 4, 1, true);
    }
  }
  renderer.drawLine(x, baseline, x + width, baseline, 1, true);
}

std::string ReadingStatsActivity::formatDay(const int daysAgo, const uint32_t anchorDayIndex) {
  ReadingStatsDate date;
  if (anchorDayIndex == 0 || !readingStatsDateFromDayIndex(anchorDayIndex - daysAgo, date)) return "-";
  char buf[16];
  snprintf(buf, sizeof(buf), "%02u.%02u", date.day, date.month);
  return buf;
}

void ReadingStatsActivity::renderBookPage(const int x, int y, const int contentWidth) {
  // --- 1. Hero Book Card ---
  const int heroH = 88;
  renderer.drawRoundedRect(x, y, contentWidth, heroH, 1, 8, true);

  std::string title = bookStats_.bookTitle.empty() ? bookTitle_ : bookStats_.bookTitle;
  if (title.empty()) title = tr(STR_UNNAMED);
  renderer.drawText(UI_12_FONT_ID, x + 12, y + 10,
                    renderer.truncatedText(UI_12_FONT_ID, title.c_str(), contentWidth - 24).c_str(), true,
                    EpdFontFamily::BOLD);

  std::string author = !bookStats_.bookAuthor.empty() ? bookStats_.bookAuthor : bookAuthor_;
  if (!author.empty()) {
    renderer.drawText(UI_10_FONT_ID, x + 12, y + 30,
                      renderer.truncatedText(UI_10_FONT_ID, author.c_str(), contentWidth - 24).c_str(), true);
  }

  // Progress bar
  const int percent = bookProgressPercent_ >= 0.0f ? static_cast<int>(bookProgressPercent_ + 0.5f) : 0;
  char pctBuf[16];
  snprintf(pctBuf, sizeof(pctBuf), "%d%%", percent);
  const int pctW = renderer.getTextAdvanceX(UI_12_FONT_ID, pctBuf, EpdFontFamily::BOLD);
  const int barW = contentWidth - 24 - pctW - 10;
  const int barY = y + 54;
  renderer.drawRoundedRect(x + 12, barY, barW, 9, 1, 4, true);
  if (percent > 0) {
    const int fillW = (barW - 2) * std::min(percent, 100) / 100;
    if (fillW > 0) renderer.fillRect(x + 13, barY + 1, fillW, 7, true);
  }
  renderer.drawText(UI_12_FONT_ID, x + 12 + barW + 10, barY - 3, pctBuf, true, EpdFontFamily::BOLD);

  y += heroH + 8;

  // --- 2. 2x2 KPI Mini-Cards Grid ---
  const int gap = 8;
  const int tileW = (contentWidth - gap) / 2;
  const int tileH = 54;
  const int x1 = x;
  const int x2 = x + tileW + gap;
  char buf[64];

  // Tile 1: Total Time
  renderer.drawRoundedRect(x1, y, tileW, tileH, 1, 6, true);
  renderer.drawText(UI_10_FONT_ID, x1 + 10, y + 8, tr(STR_STATS_TOTAL_TIME));
  BookReadingStats::formatDuration(bookStats_.totalReadingSeconds, buf, sizeof(buf));
  renderer.drawText(UI_12_FONT_ID, x1 + 10, y + 26, buf, true, EpdFontFamily::BOLD);

  // Tile 2: Pace
  renderer.drawRoundedRect(x2, y, tileW, tileH, 1, 6, true);
  renderer.drawText(UI_10_FONT_ID, x2 + 10, y + 8, tr(STR_STATS_PACE));
  if (bookStats_.avgSecondsPerForwardPage > 0) {
    snprintf(buf, sizeof(buf), "%u %s", bookStats_.avgSecondsPerForwardPage, tr(STR_STATS_UNIT_SEC_PER_PAGE));
  } else {
    snprintf(buf, sizeof(buf), "—");
  }
  renderer.drawText(UI_12_FONT_ID, x2 + 10, y + 26, buf, true, EpdFontFamily::BOLD);

  const int yRow2 = y + tileH + gap;

  // Tile 3: Pages Turned
  renderer.drawRoundedRect(x1, yRow2, tileW, tileH, 1, 6, true);
  renderer.drawText(UI_10_FONT_ID, x1 + 10, yRow2 + 8, tr(STR_STATS_PAGES_TURNED));
  snprintf(buf, sizeof(buf), "%lu", static_cast<unsigned long>(bookStats_.totalPagesTurned));
  renderer.drawText(UI_12_FONT_ID, x1 + 10, yRow2 + 26, buf, true, EpdFontFamily::BOLD);

  // Tile 4: Sessions
  renderer.drawRoundedRect(x2, yRow2, tileW, tileH, 1, 6, true);
  renderer.drawText(UI_10_FONT_ID, x2 + 10, yRow2 + 8, tr(STR_STATS_SESSIONS));
  snprintf(buf, sizeof(buf), "%u", bookStats_.sessionCount);
  renderer.drawText(UI_12_FONT_ID, x2 + 10, yRow2 + 26, buf, true, EpdFontFamily::BOLD);

  y = yRow2 + tileH + 8;

  // --- 3. Timeline & Estimates Card ---
  const int timelineH = 86;
  renderer.drawRoundedRect(x, y, contentWidth, timelineH, 1, 6, true);

  // Row 1: Time Left + Finish Estimate
  renderer.drawText(UI_10_FONT_ID, x + 12, y + 8, tr(STR_STATS_TIME_LEFT));
  if (estimatedSecondsLeft_ > 0 && bookProgressPercent_ >= 0.0f && bookProgressPercent_ < 100.0f) {
    BookReadingStats::formatDuration(estimatedSecondsLeft_, buf, sizeof(buf));
    std::string leftStr = std::string("~ ") + buf;
    renderer.drawText(UI_12_FONT_ID, x + 12, y + 24, leftStr.c_str(), true, EpdFontFamily::BOLD);
  } else {
    renderer.drawText(UI_12_FONT_ID, x + 12, y + 24, "—", true, EpdFontFamily::BOLD);
  }

  renderer.drawText(UI_10_FONT_ID, x + contentWidth / 2, y + 8, tr(STR_STATS_FINISH_ESTIMATE));
  std::string finishDateStr = "—";
  if (!bookStats_.isFinished && bookProgressPercent_ >= 0.0f && bookProgressPercent_ < 100.0f) {
    uint32_t estimateSec = estimatedSecondsLeft_;
    if (estimateSec == 0) {
      fallbackEstimatedTimeLeft(bookStats_, bookProgressPercent_, estimateSec);
    }
    ReadingStatsDateTime now;
    ReadingStatsDate finishDate;
    if (estimateSec > 0 && getCurrentLocalReadingStatsDateTime(now) &&
        estimateFinishDateFromDailyPace(bookStats_, now, estimateSec, finishDate)) {
      char dateBuf[24];
      formatFinishDate(finishDate, dateBuf, sizeof(dateBuf));
      finishDateStr = dateBuf;
    }
  }
  renderer.drawText(UI_12_FONT_ID, x + contentWidth / 2, y + 24, finishDateStr.c_str(), true, EpdFontFamily::BOLD);

  renderer.drawLine(x + 12, y + 46, x + contentWidth - 12, y + 46, true);

  // Row 2: Started / Finished Date
  const auto formatDate = [](const ReadingStatsDate& d) {
    if (!d.isValid()) return std::string("—");
    char dateBuf[16];
    snprintf(dateBuf, sizeof(dateBuf), "%02u.%02u.%04u", d.day, d.month, d.year);
    return std::string(dateBuf);
  };
  const char* dateLabel = bookStats_.isFinished ? tr(STR_STATS_FINISHED) : tr(STR_STATS_STARTED);
  renderer.drawText(UI_10_FONT_ID, x + 12, y + 54, dateLabel);
  const std::string dateVal = (bookStats_.isFinished && bookStats_.finishedDate.isValid())
                                  ? formatDate(bookStats_.finishedDate)
                                  : formatDate(bookStats_.startDate);
  renderer.drawText(UI_12_FONT_ID, x + contentWidth / 2, y + 52, dateVal.c_str(), true, EpdFontFamily::BOLD);

  y += timelineH + 10;

  // --- 4. Interactive Action Button ---
  const int btnH = 42;
  actionButtonRect_ = {x, y, contentWidth, btnH};
  actionButtonVisible_ = true;

  const char* btnText = bookStats_.isFinished ? tr(STR_STATS_MARK_UNFINISHED) : tr(STR_STATS_MARK_FINISHED);
  if (bookStats_.isFinished) {
    renderer.drawRoundedRect(x, y, contentWidth, btnH, 2, 8, true);
    const int tw = renderer.getTextAdvanceX(UI_12_FONT_ID, btnText, EpdFontFamily::BOLD);
    const int th = renderer.getLineHeight(UI_12_FONT_ID);
    renderer.drawText(UI_12_FONT_ID, x + (contentWidth - tw) / 2, y + (btnH - th) / 2, btnText, true,
                      EpdFontFamily::BOLD);
  } else {
    renderer.fillRoundedRect(x, y, contentWidth, btnH, 8, Color::Black);
    const int tw = renderer.getTextAdvanceX(UI_12_FONT_ID, btnText, EpdFontFamily::BOLD);
    const int th = renderer.getLineHeight(UI_12_FONT_ID);
    renderer.drawText(UI_12_FONT_ID, x + (contentWidth - tw) / 2, y + (btnH - th) / 2, btnText, false,
                      EpdFontFamily::BOLD);
  }
}

void ReadingStatsActivity::renderDevicePage(const int x, int y, const int contentWidth) {
  // --- 1. Hero Total Reading Time ---
  const int heroH = 74;
  renderer.drawRoundedRect(x, y, contentWidth, heroH, 1, 8, true);
  renderer.drawText(UI_10_FONT_ID, x + 12, y + 10, tr(STR_STATS_TOTAL_TIME));
  char buf[96];
  BookReadingStats::formatDuration(globalStats_.totalReadingSeconds, buf, sizeof(buf));
  renderer.drawText(UI_12_FONT_ID, x + 12, y + 32, buf, true, EpdFontFamily::BOLD);

  y += heroH + 8;

  // --- 2. Streaks Card ---
  const int streakH = 66;
  renderer.drawRoundedRect(x, y, contentWidth, streakH, 1, 6, true);
  renderer.drawLine(x + contentWidth / 2, y + 10, x + contentWidth / 2, y + 56, true);

  renderer.drawText(UI_10_FONT_ID, x + 12, y + 10, tr(STR_STATS_CURRENT_STREAK));
  char streakBuf[32];
  snprintf(streakBuf, sizeof(streakBuf), "%u %s", globalStats_.currentReadingStreak(todayDayIndex_),
           tr(STR_STATS_DAYS));
  renderer.drawText(UI_12_FONT_ID, x + 12, y + 30, streakBuf, true, EpdFontFamily::BOLD);

  renderer.drawText(UI_10_FONT_ID, x + contentWidth / 2 + 12, y + 10, tr(STR_STATS_LONGEST_STREAK));
  char longestBuf[32];
  snprintf(longestBuf, sizeof(longestBuf), "%u %s", globalStats_.displayLongestReadingStreak(), tr(STR_STATS_DAYS));
  renderer.drawText(UI_12_FONT_ID, x + contentWidth / 2 + 12, y + 30, longestBuf, true, EpdFontFamily::BOLD);

  y += streakH + 8;

  // --- 3. 2x2 Lifetime Tiles ---
  const int gap = 8;
  const int tileW = (contentWidth - gap) / 2;
  const int tileH = 54;
  const int x1 = x;
  const int x2 = x + tileW + gap;

  // Tile 1: Books Finished
  renderer.drawRoundedRect(x1, y, tileW, tileH, 1, 6, true);
  renderer.drawText(UI_10_FONT_ID, x1 + 10, y + 8, tr(STR_STATS_BOOKS_FINISHED));
  snprintf(buf, sizeof(buf), "%lu", static_cast<unsigned long>(globalStats_.completedBooks));
  renderer.drawText(UI_12_FONT_ID, x1 + 10, y + 26, buf, true, EpdFontFamily::BOLD);

  // Tile 2: Total Pages
  renderer.drawRoundedRect(x2, y, tileW, tileH, 1, 6, true);
  renderer.drawText(UI_10_FONT_ID, x2 + 10, y + 8, tr(STR_STATS_PAGES_TURNED));
  snprintf(buf, sizeof(buf), "%lu", static_cast<unsigned long>(globalStats_.totalPagesTurned));
  renderer.drawText(UI_12_FONT_ID, x2 + 10, y + 26, buf, true, EpdFontFamily::BOLD);

  const int yRow2 = y + tileH + gap;

  // Tile 3: Total Sessions
  renderer.drawRoundedRect(x1, yRow2, tileW, tileH, 1, 6, true);
  renderer.drawText(UI_10_FONT_ID, x1 + 10, yRow2 + 8, tr(STR_STATS_SESSIONS));
  snprintf(buf, sizeof(buf), "%lu", static_cast<unsigned long>(globalStats_.totalSessions));
  renderer.drawText(UI_12_FONT_ID, x1 + 10, yRow2 + 26, buf, true, EpdFontFamily::BOLD);

  // Tile 4: Average per Session
  renderer.drawRoundedRect(x2, yRow2, tileW, tileH, 1, 6, true);
  renderer.drawText(UI_10_FONT_ID, x2 + 10, yRow2 + 8, tr(STR_STATS_AVG_SESSION));
  const uint32_t avgSec = globalStats_.totalSessions > 0
                              ? static_cast<uint32_t>(globalStats_.totalReadingSeconds / globalStats_.totalSessions)
                              : 0;
  BookReadingStats::formatDuration(avgSec, buf, sizeof(buf));
  renderer.drawText(UI_12_FONT_ID, x2 + 10, yRow2 + 26, buf, true, EpdFontFamily::BOLD);

  y = yRow2 + tileH + 10;

  // --- 4. Interactive Action Button ---
  const int btnH = 42;
  actionButtonRect_ = {x, y, contentWidth, btnH};
  actionButtonVisible_ = true;

  renderer.drawRoundedRect(x, y, contentWidth, btnH, 2, 8, true);
  const char* btnText = tr(STR_STATS_EXPORT);
  const int tw = renderer.getTextAdvanceX(UI_12_FONT_ID, btnText, EpdFontFamily::BOLD);
  const int th = renderer.getLineHeight(UI_12_FONT_ID);
  renderer.drawText(UI_12_FONT_ID, x + (contentWidth - tw) / 2, y + (btnH - th) / 2, btnText, true,
                    EpdFontFamily::BOLD);
}

void ReadingStatsActivity::renderActivityPage(const int x, int y, const int contentWidth) {
  // --- 1. 14-Day Activity Bar Chart Card ---
  const int chartCardH = 144;
  renderer.drawRoundedRect(x, y, contentWidth, chartCardH, 1, 8, true);
  renderer.drawText(UI_10_FONT_ID, x + 12, y + 10, tr(STR_STATS_LAST_14_DAYS), true, EpdFontFamily::BOLD);

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
  const int lastW = renderer.getTextAdvanceX(UI_10_FONT_ID, lastLabel.c_str(), EpdFontFamily::REGULAR);
  renderer.drawText(UI_10_FONT_ID, x + contentWidth - 12 - lastW, y + 34 + chartH + 6, lastLabel.c_str());

  y += chartCardH + 8;

  // --- 2. Weekday Distribution Card ---
  const int dowCardH = 104;
  renderer.drawRoundedRect(x, y, contentWidth, dowCardH, 1, 6, true);
  renderer.drawText(UI_10_FONT_ID, x + 12, y + 10, tr(STR_STATS_BY_WEEKDAY), true, EpdFontFamily::BOLD);

  static const StrId dowIds[READING_DAY_OF_WEEK_COUNT] = {
      StrId::STR_STATS_DOW_MON, StrId::STR_STATS_DOW_TUE, StrId::STR_STATS_DOW_WED, StrId::STR_STATS_DOW_THU,
      StrId::STR_STATS_DOW_FRI, StrId::STR_STATS_DOW_SAT, StrId::STR_STATS_DOW_SUN};
  const int dowChartH = 42;
  uint16_t dow[READING_DAY_OF_WEEK_COUNT];
  for (size_t i = 0; i < READING_DAY_OF_WEEK_COUNT; ++i) dow[i] = globalStats_.dayOfWeekSeconds[i];
  drawBarChart(x + 12, y + 32, contentWidth - 24, dowChartH, dow, READING_DAY_OF_WEEK_COUNT, -1);

  const int dowSlotW = (contentWidth - 24) / READING_DAY_OF_WEEK_COUNT;
  for (size_t i = 0; i < READING_DAY_OF_WEEK_COUNT; ++i) {
    const char* label = I18N.get(dowIds[i]);
    const int labelW = renderer.getTextAdvanceX(UI_10_FONT_ID, label, EpdFontFamily::REGULAR);
    const int slotCenter = x + 12 + dowSlotW * i + dowSlotW / 2;
    renderer.drawText(UI_10_FONT_ID, slotCenter - labelW / 2, y + 32 + dowChartH + 4, label);
  }

  y += dowCardH + 8;

  // --- 3. Time of Day Distribution Card ---
  const int todCardH = 96;
  renderer.drawRoundedRect(x, y, contentWidth, todCardH, 1, 6, true);
  renderer.drawText(UI_10_FONT_ID, x + 12, y + 10, tr(STR_STATS_BY_TIME_OF_DAY), true, EpdFontFamily::BOLD);

  static const StrId todIds[READING_TIME_BUCKET_COUNT] = {StrId::STR_STATS_MORNING, StrId::STR_STATS_AFTERNOON,
                                                         StrId::STR_STATS_EVENING, StrId::STR_STATS_NIGHT};
  const int todChartH = 34;
  uint16_t tod[READING_TIME_BUCKET_COUNT];
  for (size_t i = 0; i < READING_TIME_BUCKET_COUNT; ++i) tod[i] = globalStats_.timeOfDaySeconds[i];
  drawBarChart(x + 12, y + 32, contentWidth - 24, todChartH, tod, READING_TIME_BUCKET_COUNT, -1);

  const int todSlotW = (contentWidth - 24) / READING_TIME_BUCKET_COUNT;
  for (size_t i = 0; i < READING_TIME_BUCKET_COUNT; ++i) {
    const char* label = I18N.get(todIds[i]);
    const int labelW = renderer.getTextAdvanceX(UI_10_FONT_ID, label, EpdFontFamily::REGULAR);
    const int slotCenter = x + 12 + todSlotW * i + todSlotW / 2;
    renderer.drawText(UI_10_FONT_ID, slotCenter - labelW / 2, y + 32 + todChartH + 4, label);
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

  const char* confirmLabel;
  switch (page_) {
    case Page::Book:
      confirmLabel = bookStats_.isFinished ? tr(STR_STATS_MARK_UNFINISHED) : tr(STR_STATS_MARK_FINISHED);
      break;
    default:
      confirmLabel = tr(STR_STATS_EXPORT);
      break;
  }
  const auto labels = mappedInput.mapLabels(tr(STR_BACK), confirmLabel, tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer();
}
