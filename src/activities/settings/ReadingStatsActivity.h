#pragma once

#include <string>

#include "../../stats/BookReadingStats.h"
#include "../../stats/GlobalReadingStats.h"

#include "../Activity.h"

// Reading statistics screen. Three pages cycled with Up/Down (or Left/Right):
//   1. This book   — per-book numbers + mark-as-finished toggle on Confirm
//   2. Device      — lifetime totals + streaks, Confirm exports the CSV
//   3. Activity    — 14-day bar chart + weekday and time-of-day distribution
// Page 1 only appears when the activity is opened with a book context (from
// the reader menu); from Settings it starts on the Device page.
class ReadingStatsActivity final : public Activity {
 public:
  explicit ReadingStatsActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                                const std::string& bookPath = "", const std::string& bookTitle = "",
                                const std::string& bookAuthor = "", float bookProgressPercent = -1.0f,
                                uint32_t estimatedSecondsLeft = 0);

  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  enum class Page : uint8_t { Book, Device, Activity, PAGE_COUNT };

  bool hasBookPage() const { return !bookPath_.empty(); }
  Page firstPage() const { return hasBookPage() ? Page::Book : Page::Device; }
  void cyclePage(int delta);
  void handleConfirm();
  void exportCsv();
  void renderBookPage(int yTop, int contentWidth);
  void renderDevicePage(int yTop, int contentWidth);
  void renderActivityPage(int yTop, int contentWidth);
  void drawBarChart(int x, int y, int width, int height, const uint16_t* values, int count,
                    int highlightIndex) const;
  void drawStatRow(int y, int contentWidth, const char* label, const std::string& value) const;
  static std::string formatDay(int daysAgo, uint32_t anchorDayIndex);

  std::string bookPath_;
  std::string bookTitle_;
  std::string bookAuthor_;
  float bookProgressPercent_;
  uint32_t estimatedSecondsLeft_;

  BookReadingStats bookStats_;
  GlobalReadingStats globalStats_;

  Page page_ = Page::Device;
  bool hasData_ = false;
  uint32_t todayDayIndex_ = 0;
  bool messageVisible_ = false;
  unsigned long messageShownAt_ = 0;
  std::string message_;

  bool hasAnyBookData() const;
};
