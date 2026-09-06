#pragma once

#include "components/themes/BaseTheme.h"

struct GlobalReadingStats;

// Dashboard: a real cover (correct aspect ratio, rounded corners) on the left
// and a column of right-aligned reading-stat rows on the right, for the home
// screen and (via drawDashboardSleepScreen) the sleep screen. Ported from
// CrossInk's DashboardTheme and sized to fit BookPoint's generic
// list-menu-below-the-cover home screen flow.
namespace DashboardMetrics {
constexpr ThemeMetrics makeValues() {
  ThemeMetrics v = BaseMetrics::values;
  // Kept at/under Classic's homeTopPadding+homeCoverTileHeight (40+370=410) so
  // the menu and button hints below keep the same room they have in Classic.
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
  void drawRecentBookCover(GfxRenderer& renderer, Rect rect, const std::vector<RecentBook>& recentBooks,
                           int selectorIndex, bool& coverRendered, bool& coverBufferStored, bool& bufferRestored,
                           std::function<bool()> storeCoverBuffer, const BookReadingStats* stats = nullptr,
                           float progressPercent = -1.0f) const override;

  // Sleep-screen counterpart of drawRecentBookCover: same cover+stats layout.
  // Called directly by SleepActivity (not part of the BaseTheme interface).
  void drawDashboardSleepScreen(const GfxRenderer& renderer, const RecentBook& book, const BookReadingStats* stats,
                                float progressPercent) const;

 private:
  void drawDashboardRow(const GfxRenderer& renderer, Rect rect, const RecentBook* book, bool hasBook,
                        const BookReadingStats* stats, float progressPercent) const;
  void drawCoverPanel(const GfxRenderer& renderer, Rect rect, const RecentBook* book, bool hasBook) const;
  void drawStatsColumn(const GfxRenderer& renderer, Rect rect, const BookReadingStats* stats,
                       float progressPercent) const;
};
