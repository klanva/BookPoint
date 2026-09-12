#pragma once

#include "components/themes/BaseTheme.h"

struct GlobalReadingStats;

namespace DashboardMetrics {
constexpr ThemeMetrics makeValues() {
  ThemeMetrics v = BaseMetrics::values;
  v.homeTopPadding = 40;
  v.homeCoverHeight = 260;
  v.homeCoverTileHeight = 280;
  v.homeRecentBooksCount = 1;
  v.homeContinueReadingInMenu = false;
  v.homeMenuTopOffset = 16;
  return v;
}
constexpr ThemeMetrics values = makeValues();
}  // namespace DashboardMetrics

class DashboardTheme : public BaseTheme {
 public:
  void drawHeader(const GfxRenderer& renderer, Rect rect, const char* title,
                  const char* subtitle = nullptr) const override;

  void drawRecentBookCover(GfxRenderer& renderer, Rect rect, const std::vector<RecentBook>& recentBooks,
                           int selectorIndex, bool& coverRendered, bool& coverBufferStored, bool& bufferRestored,
                           std::function<bool()> storeCoverBuffer, const BookReadingStats* stats = nullptr,
                           float progressPercent = -1.0f) const override;

  void drawDashboardSleepScreen(const GfxRenderer& renderer, const RecentBook& book, const BookReadingStats* stats,
                                float progressPercent) const;

  void drawButtonMenu(GfxRenderer& renderer, Rect rect, int buttonCount, int selectedIndex,
                      const std::function<std::string(int index)>& buttonLabel,
                      const std::function<UIIcon(int index)>& rowIcon) const override;

 private:
  void drawDashboardRow(const GfxRenderer& renderer, Rect rect, const RecentBook* book, bool hasBook,
                        const BookReadingStats* stats, float progressPercent) const;
  void drawCoverPanel(const GfxRenderer& renderer, Rect rect, const RecentBook* book, bool hasBook) const;
  void drawStatsColumn(const GfxRenderer& renderer, Rect rect, const RecentBook* book,
                       const BookReadingStats* stats, float progressPercent) const;
};
