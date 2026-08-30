#include "ReadingStatsActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>
#include <Logging.h>

#include <cstdio>
#include <vector>

#include "stats/StatsStore.h"
#include "stats/ReadingStatsTypes.h"
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

void ReadingStatsActivity::cyclePage(const int delta) {
  const int count = static_cast<int>(Page::PAGE_COUNT);
  int page = static_cast<int>(page_);
  // Page 0 (book) does not exist without a book context.
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

void ReadingStatsActivity::drawStatRow(const int y, const int contentWidth, const char* label,
                                       const std::string& value) const {
  renderer.drawText(UI_12_FONT_ID, 0, y, label);
  // Right-align the value.
  const int valueWidth = renderer.getTextAdvanceX(UI_12_FONT_ID, value.c_str(), EpdFontFamily::REGULAR);
  renderer.drawText(UI_12_FONT_ID, contentWidth - valueWidth, y, value.c_str());
}

void ReadingStatsActivity::drawBarChart(const int x, const int y, const int width, const int height,
                                        const uint16_t* values, const int count, const int highlightIndex) const {
  uint16_t maxValue = 1;
  for (int i = 0; i < count; ++i) {
    if (values[i] > maxValue) maxValue = values[i];
  }

  const int slotWidth = width / (count > 0 ? count : 1);
  const int barWidth = slotWidth > 6 ? slotWidth - 4 : slotWidth - 1;
  const int baseline = y + height;
  for (int i = 0; i < count; ++i) {
    const int barX = x + i * slotWidth + (slotWidth - barWidth) / 2;
    int barHeight = static_cast<int>((static_cast<uint32_t>(values[i]) * height) / maxValue);
    if (values[i] > 0 && barHeight < 2) barHeight = 2;
    if (barHeight > 0) {
      renderer.fillRect(barX, baseline - barHeight, barWidth, barHeight, true);
    } else {
      // Empty day: a 2px floor tick so the day is visibly present.
      renderer.fillRect(barX, baseline - 2, barWidth, 2, false);
    }
    if (i == highlightIndex) {
      renderer.drawRect(barX - 1, baseline - height - 1, barWidth + 2, height + 1, true);
    }
  }
  renderer.drawLine(x, baseline, x + width, baseline, true);
}

std::string ReadingStatsActivity::formatDay(const int daysAgo, const uint32_t anchorDayIndex) {
  ReadingStatsDate date;
  if (anchorDayIndex == 0 || !readingStatsDateFromDayIndex(anchorDayIndex - daysAgo, date)) return "-";
  char buf[16];
  snprintf(buf, sizeof(buf), "%02u.%02u", date.day, date.month);
  return buf;
}

void ReadingStatsActivity::renderBookPage(const int yTop, const int contentWidth) {
  int y = yTop;

  std::string title = bookStats_.bookTitle.empty() ? bookTitle_ : bookStats_.bookTitle;
  if (title.empty()) title = tr(STR_UNNAMED);
  renderer.drawText(UI_12_FONT_ID, 0, y, renderer.truncatedText(UI_12_FONT_ID, title.c_str(), contentWidth).c_str(),
                    true, EpdFontFamily::BOLD);
  y += 26;

  char buf[96];
  BookReadingStats::formatDuration(bookStats_.totalReadingSeconds, buf, sizeof(buf));
  drawStatRow(y, contentWidth, tr(STR_STATS_TOTAL_TIME), buf);
  y += 30;

  snprintf(buf, sizeof(buf), "%u", bookStats_.sessionCount);
  drawStatRow(y, contentWidth, tr(STR_STATS_SESSIONS), buf);
  y += 30;

  snprintf(buf, sizeof(buf), "%lu", static_cast<unsigned long>(bookStats_.totalPagesTurned));
  drawStatRow(y, contentWidth, tr(STR_STATS_PAGES_TURNED), buf);
  y += 30;

  if (bookStats_.avgSecondsPerForwardPage > 0) {
    snprintf(buf, sizeof(buf), "%u %s", bookStats_.avgSecondsPerForwardPage, tr(STR_STATS_UNIT_SEC_PER_PAGE));
    drawStatRow(y, contentWidth, tr(STR_STATS_PACE), buf);
    y += 30;
  }

  if (estimatedSecondsLeft_ > 0 && bookProgressPercent_ >= 0.0f && bookProgressPercent_ < 100.0f) {
    BookReadingStats::formatDuration(estimatedSecondsLeft_, buf, sizeof(buf));
    drawStatRow(y, contentWidth, tr(STR_STATS_TIME_LEFT), buf);
    y += 30;
  }

  const auto formatDate = [](const ReadingStatsDate& d) {
    if (!d.isValid()) return std::string("-");
    char dateBuf[16];
    snprintf(dateBuf, sizeof(dateBuf), "%02u.%02u.%04u", d.day, d.month, d.year);
    return std::string(dateBuf);
  };
  if (bookStats_.startDate.isValid()) {
    drawStatRow(y, contentWidth, tr(STR_STATS_STARTED), formatDate(bookStats_.startDate));
    y += 30;
  }
  if (bookStats_.isFinished) {
    drawStatRow(y, contentWidth, tr(STR_STATS_FINISHED),
                bookStats_.finishedDate.isValid() ? formatDate(bookStats_.finishedDate) : std::string("-"));
    y += 30;
  } else if (bookStats_.lastReadEpoch > 0) {
    // Show when the book was last opened (local date, derived from the epoch).
    const time_t epoch = static_cast<time_t>(bookStats_.lastReadEpoch);
    struct tm tmUtc;
    gmtime_r(&epoch, &tmUtc);
    snprintf(buf, sizeof(buf), "%02u.%02u.%04u", tmUtc.tm_mday, tmUtc.tm_mon + 1, tmUtc.tm_year + 1900);
    drawStatRow(y, contentWidth, tr(STR_STATS_LAST_READ), buf);
    y += 30;
  }

  // Toggle hint.
  renderer.drawText(UI_10_FONT_ID, 0, y + 8,
                    bookStats_.isFinished ? tr(STR_STATS_MARK_UNFINISHED_HINT) : tr(STR_STATS_MARK_FINISHED_HINT));
}

void ReadingStatsActivity::renderDevicePage(const int yTop, const int contentWidth) {
  // The header already carries the screen title; start with the rows.
  int y = yTop + 10;

  char buf[96];
  BookReadingStats::formatDuration(globalStats_.totalReadingSeconds, buf, sizeof(buf));
  drawStatRow(y, contentWidth, tr(STR_STATS_TOTAL_TIME), buf);
  y += 30;

  snprintf(buf, sizeof(buf), "%lu", static_cast<unsigned long>(globalStats_.totalSessions));
  drawStatRow(y, contentWidth, tr(STR_STATS_SESSIONS), buf);
  y += 30;

  snprintf(buf, sizeof(buf), "%lu", static_cast<unsigned long>(globalStats_.totalPagesTurned));
  drawStatRow(y, contentWidth, tr(STR_STATS_PAGES_TURNED), buf);
  y += 30;

  snprintf(buf, sizeof(buf), "%lu", static_cast<unsigned long>(globalStats_.completedBooks));
  drawStatRow(y, contentWidth, tr(STR_STATS_BOOKS_FINISHED), buf);
  y += 30;

  snprintf(buf, sizeof(buf), "%u", globalStats_.currentReadingStreak(todayDayIndex_));
  drawStatRow(y, contentWidth, tr(STR_STATS_CURRENT_STREAK), buf);
  y += 30;

  snprintf(buf, sizeof(buf), "%u", globalStats_.displayLongestReadingStreak());
  drawStatRow(y, contentWidth, tr(STR_STATS_LONGEST_STREAK), buf);
  y += 30;

  renderer.drawText(UI_10_FONT_ID, 0, y + 8, tr(STR_STATS_EXPORT_HINT));
}

void ReadingStatsActivity::renderActivityPage(const int yTop, const int contentWidth) {
  int y = yTop;

  renderer.drawText(UI_12_FONT_ID, 0, y, tr(STR_STATS_ACTIVITY), true, EpdFontFamily::BOLD);
  y += 30;

  // --- 14-day chart ---
  renderer.drawText(UI_10_FONT_ID, 0, y, tr(STR_STATS_LAST_14_DAYS));
  y += 20;
  const int chartHeight = 80;
  const int chartWidth = contentWidth;
  uint16_t days[14];
  for (int i = 0; i < 14; ++i) {
    const int32_t secs = globalStats_.secondsForDaysAgo(13 - i);
    days[i] = secs > 0 ? static_cast<uint16_t>(secs) : 0;
  }
  drawBarChart(0, y, chartWidth, chartHeight, days, 14, 13);
  // First/last day labels under the chart.
  renderer.drawText(UI_10_FONT_ID, 0, y + chartHeight + 6, formatDay(13, globalStats_.anchorDayIndex).c_str());
  const std::string lastLabel = formatDay(0, globalStats_.anchorDayIndex);
  renderer.drawText(UI_10_FONT_ID, contentWidth - renderer.getTextAdvanceX(UI_10_FONT_ID, lastLabel.c_str(), EpdFontFamily::REGULAR),
                    y + chartHeight + 6, lastLabel.c_str());
  y += chartHeight + 36;

  // --- weekday distribution (Mon..Sun) ---
  renderer.drawText(UI_10_FONT_ID, 0, y, tr(STR_STATS_BY_WEEKDAY));
  y += 20;
  static const StrId dowIds[READING_DAY_OF_WEEK_COUNT] = {
      StrId::STR_STATS_DOW_MON, StrId::STR_STATS_DOW_TUE, StrId::STR_STATS_DOW_WED, StrId::STR_STATS_DOW_THU,
      StrId::STR_STATS_DOW_FRI, StrId::STR_STATS_DOW_SAT, StrId::STR_STATS_DOW_SUN};
  const int dowChartHeight = 46;
  uint16_t dow[READING_DAY_OF_WEEK_COUNT];
  for (size_t i = 0; i < READING_DAY_OF_WEEK_COUNT; ++i) dow[i] = globalStats_.dayOfWeekSeconds[i];
  drawBarChart(0, y, contentWidth, dowChartHeight, dow, READING_DAY_OF_WEEK_COUNT, -1);
  for (size_t i = 0; i < READING_DAY_OF_WEEK_COUNT; ++i) {
    const int slotWidth = contentWidth / READING_DAY_OF_WEEK_COUNT;
    const char* label = I18N.get(dowIds[i]);
    const int labelWidth = renderer.getTextAdvanceX(UI_10_FONT_ID, label, EpdFontFamily::REGULAR);
    const int slotCenter = (contentWidth / READING_DAY_OF_WEEK_COUNT) * i + slotWidth / 2;
    renderer.drawText(UI_10_FONT_ID, slotCenter - labelWidth / 2, y + dowChartHeight + 4, label);
  }
  y += dowChartHeight + 32;

  // --- time-of-day distribution ---
  renderer.drawText(UI_10_FONT_ID, 0, y, tr(STR_STATS_BY_TIME_OF_DAY));
  y += 20;
  static const StrId todIds[READING_TIME_BUCKET_COUNT] = {StrId::STR_STATS_MORNING, StrId::STR_STATS_AFTERNOON,
                                                         StrId::STR_STATS_EVENING, StrId::STR_STATS_NIGHT};
  const int todChartHeight = 38;
  uint16_t tod[READING_TIME_BUCKET_COUNT];
  for (size_t i = 0; i < READING_TIME_BUCKET_COUNT; ++i) tod[i] = globalStats_.timeOfDaySeconds[i];
  drawBarChart(0, y, contentWidth, todChartHeight, tod, READING_TIME_BUCKET_COUNT, -1);
  for (size_t i = 0; i < READING_TIME_BUCKET_COUNT; ++i) {
    const int slotWidth = contentWidth / READING_TIME_BUCKET_COUNT;
    const char* label = I18N.get(todIds[i]);
    const int labelWidth = renderer.getTextAdvanceX(UI_10_FONT_ID, label, EpdFontFamily::REGULAR);
    const int slotCenter = slotWidth * i + slotWidth / 2;
    renderer.drawText(UI_10_FONT_ID, slotCenter - labelWidth / 2, y + todChartHeight + 4, label);
  }
}

void ReadingStatsActivity::render(RenderLock&&) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();
  const int contentWidth = pageWidth - 2 * 20;

  renderer.clearScreen();
  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, tr(STR_READING_STATS));

  int y = metrics.topPadding + metrics.headerHeight + 14;

  const bool noData = (!hasBookPage() || !hasAnyBookData()) && globalStats_.totalReadingSeconds == 0 &&
                      globalStats_.totalSessions == 0;
  if (noData) {
    renderer.drawCenteredText(UI_12_FONT_ID, pageHeight / 2, tr(STR_STATS_NO_DATA), true);
  } else {
    switch (page_) {
      case Page::Book:
        renderBookPage(y, contentWidth);
        break;
      case Page::Device:
        renderDevicePage(y, contentWidth);
        break;
      case Page::Activity:
        renderActivityPage(y, contentWidth);
        break;
      default:
        break;
    }
  }

  // Page dots just above the button hints.
  const int first = hasBookPage() ? 0 : 1;
  for (int i = first; i < static_cast<int>(Page::PAGE_COUNT); ++i) {
    const int dotX = pageWidth - 14 - (static_cast<int>(Page::PAGE_COUNT) - i) * 12;
    const int dotY = pageHeight - 88;
    if (static_cast<int>(page_) == i) {
      renderer.fillRect(dotX, dotY, 6, 6, true);
    } else {
      renderer.drawRect(dotX, dotY, 6, 6, true);
    }
  }

  if (messageVisible_) {
    GUI.drawPopup(renderer, renderer.truncatedText(UI_12_FONT_ID, message_.c_str(), pageWidth - 40).c_str());
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
