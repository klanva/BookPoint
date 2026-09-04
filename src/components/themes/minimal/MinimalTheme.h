#pragma once

#include <cstdint>

#include "components/themes/lyra/LyraTheme.h"

namespace MinimalMetrics {
constexpr int coverWidthForHeight(const int coverHeight) {
  return static_cast<int>((static_cast<int64_t>(coverHeight) * 3 + 2) / 5);
}

constexpr ThemeMetrics makeValues() {
  ThemeMetrics v = LyraMetrics::values;
  v.homeTopPadding = 50;
  // The cover image itself, plus ~60px of progress block below it inside the tile.
  v.homeCoverHeight = 340;
  v.homeCoverTileHeight = 400;
  v.homeRecentBooksCount = 1;
  v.homeContinueReadingInMenu = false;
  v.homeMenuTopOffset = 8;
  return v;
}

constexpr ThemeMetrics values = makeValues();
constexpr int homeCoverWidth = coverWidthForHeight(values.homeCoverHeight);
}  // namespace MinimalMetrics

struct GlobalReadingStats;

class MinimalTheme : public LyraTheme {
 public:
  void drawRecentBookCover(GfxRenderer& renderer, Rect rect, const std::vector<RecentBook>& recentBooks,
                           int selectorIndex, bool& coverRendered, bool& coverBufferStored, bool& bufferRestored,
                           std::function<bool()> storeCoverBuffer, const BookReadingStats* stats = nullptr,
                           float progressPercent = -1.0f) const override;

  // Sleep-screen counterparts. drawSleepScreen paints a centered cover with a
  // progress block (white on black); drawStatsSleepScreen adds a small reading
  // streak / total-time footer. Called directly by SleepActivity.
  void drawSleepScreen(const GfxRenderer& renderer, const RecentBook& book, const BookReadingStats* stats = nullptr,
                       float progressPercent = -1.0f) const;
  void drawStatsSleepScreen(const GfxRenderer& renderer, const RecentBook& book, const BookReadingStats* stats,
                            const GlobalReadingStats* globalStats, float progressPercent = -1.0f) const;
};
