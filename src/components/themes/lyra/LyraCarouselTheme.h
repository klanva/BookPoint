#pragma once

#include "components/themes/lyra/LyraTheme.h"

class GfxRenderer;

namespace LyraCarouselMetrics {
constexpr ThemeMetrics makeValues() {
  ThemeMetrics v = LyraMetrics::values;
  v.homeTopPadding = 36;
  v.homeCoverHeight = 220;
  v.homeCoverTileHeight = 310;
  v.homeRecentBooksCount = 10;
  v.homeContinueReadingInMenu = false;
  v.homeMenuTopOffset = 10;
  v.menuRowHeight = 44;
  v.menuSpacing = 4;
  return v;
}
constexpr ThemeMetrics values = makeValues();
}  // namespace LyraCarouselMetrics

class LyraCarouselTheme : public LyraTheme {
 public:
  void drawRecentBookCover(GfxRenderer& renderer, Rect rect, const std::vector<RecentBook>& recentBooks,
                           int selectorIndex, bool& coverRendered, bool& coverBufferStored, bool& bufferRestored,
                           std::function<bool()> storeCoverBuffer, const BookReadingStats* stats = nullptr,
                           float progressPercent = -1.0f) const override;

  void drawButtonMenu(GfxRenderer& renderer, Rect rect, int buttonCount, int selectedIndex,
                      const std::function<std::string(int index)>& buttonLabel,
                      const std::function<UIIcon(int index)>& rowIcon) const override;
};
