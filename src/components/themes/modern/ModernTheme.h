#pragma once

#include "components/themes/BaseTheme.h"

struct GlobalReadingStats;

namespace ModernMetrics {
constexpr ThemeMetrics makeValues() {
  ThemeMetrics v = BaseMetrics::values;
  v.topPadding = 0;
  v.homeTopPadding = 50;
  v.homeCoverHeight = 219;
  v.homeCoverTileHeight = 490;
  v.homeRecentBooksCount = 4;
  v.homeContinueReadingInMenu = false;
  v.homeMenuTopOffset = 180;
  v.menuRowHeight = 80;
  v.menuSpacing = 0;
  v.contentSidePadding = 14;
  v.buttonHintsHeight = 0;
  return v;
}
constexpr ThemeMetrics values = makeValues();
}  // namespace ModernMetrics

class ModernTheme : public BaseTheme {
 public:
  void drawHeader(const GfxRenderer& renderer, Rect rect, const char* title,
                  const char* subtitle = nullptr) const override;

  void drawRecentBookCover(GfxRenderer& renderer, Rect rect, const std::vector<RecentBook>& recentBooks,
                           int selectorIndex, bool& coverRendered, bool& coverBufferStored, bool& bufferRestored,
                           std::function<bool()> storeCoverBuffer, const BookReadingStats* stats = nullptr,
                           float progressPercent = -1.0f) const override;

  void drawButtonMenu(GfxRenderer& renderer, Rect rect, int buttonCount, int selectedIndex,
                      const std::function<std::string(int index)>& buttonLabel,
                      const std::function<UIIcon(int index)>& rowIcon) const override;

  int getMenuRowHeight(const GfxRenderer& renderer) const override { return ModernMetrics::values.menuRowHeight; }

 private:
  void drawHeroCard(const GfxRenderer& renderer, Rect cardRect, const RecentBook* book, bool isSelected,
                    const BookReadingStats* stats, float progressPercent) const;
  void drawRecentShelf(const GfxRenderer& renderer, Rect shelfRect, const std::vector<RecentBook>& recentBooks,
                       int selectorIndex) const;
  void drawMiniBookCard(const GfxRenderer& renderer, Rect miniRect, const RecentBook& book, bool isSelected) const;
};
