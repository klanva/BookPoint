#include "LyraCarouselTheme.h"

#include <Bitmap.h>
#include <GfxRenderer.h>
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

namespace {
constexpr int kCoverCornerRadius = 6;
constexpr int kCenterW = 160;
constexpr int kCenterH = 220;
constexpr int kSideW = 104;
constexpr int kSideH = 143;
constexpr int kMainMenuIconSize = 32;

void drawMissingCover(GfxRenderer& renderer, const Rect& rect, const RecentBook& book, bool isCenter) {
  renderer.fillRoundedRect(rect.x, rect.y, rect.width, rect.height, kCoverCornerRadius, Color::White);
  renderer.drawRoundedRect(rect.x, rect.y, rect.width, rect.height, 1, kCoverCornerRadius, true);

  const int iconSize = isCenter ? 32 : 24;
  if (rect.height > iconSize + 30) {
    renderer.drawIcon(CoverIcon, rect.x + (rect.width - iconSize) / 2, rect.y + 12, iconSize);
  }

  if (isCenter) {
    const char* title = book.title.empty() ? book.path.c_str() : book.title.c_str();
    auto titleLines = renderer.wrappedText(UI_10_FONT_ID, title, rect.width - 16, 4, EpdFontFamily::BOLD);
    const int lineH = renderer.getLineHeight(UI_10_FONT_ID);
    int textY = rect.y + 16 + iconSize;
    for (const auto& line : titleLines) {
      const int w = renderer.getTextWidth(UI_10_FONT_ID, line.c_str(), EpdFontFamily::BOLD);
      renderer.drawText(UI_10_FONT_ID, rect.x + (rect.width - w) / 2, textY, line.c_str(), true, EpdFontFamily::BOLD);
      textY += lineH;
    }
  }
}

void drawCoverArt(GfxRenderer& renderer, const Rect& rect, const RecentBook& book, bool isCenter) {
  bool hasCover = false;
  std::string coverBmpPath;
  if (!book.coverBmpPath.empty()) {
    coverBmpPath = UITheme::getCoverThumbPath(book.coverBmpPath, isCenter ? kCenterH : kSideH);
    if (!Storage.exists(coverBmpPath.c_str())) {
      coverBmpPath = book.coverBmpPath;
    }
  }

  if (!coverBmpPath.empty() && Storage.exists(coverBmpPath.c_str())) {
    HalFile file;
    if (Storage.openFileForRead("HOME", coverBmpPath, file)) {
      Bitmap bitmap(file);
      if (bitmap.parseHeaders() == BmpReaderError::Ok) {
        const float wScale = static_cast<float>(rect.width) / static_cast<float>(bitmap.getWidth());
        const float hScale = static_cast<float>(rect.height) / static_cast<float>(bitmap.getHeight());
        const float scale = std::min(wScale, hScale);
        const int drawnW = std::clamp(static_cast<int>(std::ceil(bitmap.getWidth() * scale)), 1, rect.width);
        const int drawnH = std::clamp(static_cast<int>(std::ceil(bitmap.getHeight() * scale)), 1, rect.height);
        const int offX = rect.x + (rect.width - drawnW) / 2;
        const int offY = rect.y + (rect.height - drawnH) / 2;

        renderer.fillRoundedRect(rect.x, rect.y, rect.width, rect.height, kCoverCornerRadius, Color::White);
        renderer.drawBitmap(bitmap, offX, offY, drawnW, drawnH);
        renderer.maskRoundedRectOutsideCorners(offX, offY, drawnW, drawnH, kCoverCornerRadius, Color::White);
        hasCover = true;
      }
      file.close();
    }
  }

  if (!hasCover) {
    drawMissingCover(renderer, rect, book, isCenter);
  }

  if (isCenter) {
    // Dark focus border with zoom effect
    renderer.drawRoundedRect(rect.x, rect.y, rect.width, rect.height, 2, kCoverCornerRadius, true);
    renderer.drawRoundedRect(rect.x + 1, rect.y + 1, rect.width - 2, rect.height - 2, 1, kCoverCornerRadius - 1, true);
  } else {
    // Dimmed / dithered preview for depth-of-field effect
    renderer.fillRectDither(rect.x, rect.y, rect.width, rect.height, Color::LightGray);
    renderer.drawRoundedRect(rect.x, rect.y, rect.width, rect.height, 1, kCoverCornerRadius, true);
  }
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

void LyraCarouselTheme::drawRecentBookCover(GfxRenderer& renderer, Rect rect,
                                            const std::vector<RecentBook>& recentBooks, int selectorIndex,
                                            bool& coverRendered, bool& coverBufferStored, bool& bufferRestored,
                                            std::function<bool()> storeCoverBuffer, const BookReadingStats* stats,
                                            float progressPercent) const {
  if (recentBooks.empty()) {
    drawEmptyRecents(renderer, rect);
    coverRendered = false;
    coverBufferStored = false;
    return;
  }

  const int totalBooks = static_cast<int>(recentBooks.size());
  const int curIdx = std::clamp(selectorIndex, 0, totalBooks - 1);
  const RecentBook& centerBook = recentBooks[curIdx];
  const RecentBook* leftBook = (curIdx > 0) ? &recentBooks[curIdx - 1] : nullptr;
  const RecentBook* rightBook = (curIdx + 1 < totalBooks) ? &recentBooks[curIdx + 1] : nullptr;

  if (!coverRendered) {
    const int centerX = (rect.width - kCenterW) / 2;
    const int centerY = rect.y + 6;
    const int sideY = centerY + (kCenterH - kSideH) / 2;

    // Draw Left neighbor preview if available
    if (leftBook != nullptr) {
      const Rect leftRect{centerX - kSideW - 14, sideY, kSideW, kSideH};
      drawCoverArt(renderer, leftRect, *leftBook, false);
    }

    // Draw Right neighbor preview if available
    if (rightBook != nullptr) {
      const Rect rightRect{centerX + kCenterW + 14, sideY, kSideW, kSideH};
      drawCoverArt(renderer, rightRect, *rightBook, false);
    }

    // Draw Center focused book cover
    const Rect centerRect{centerX, centerY, kCenterW, kCenterH};
    drawCoverArt(renderer, centerRect, centerBook, true);

    // --- Bottom Plaque ---
    const int plaqueY = centerY + kCenterH + 6;

    // Title (bold, centered, truncated if long)
    const char* title = centerBook.title.empty() ? centerBook.path.c_str() : centerBook.title.c_str();
    const std::string truncTitle = renderer.truncatedText(UI_10_FONT_ID, title, rect.width - 40, EpdFontFamily::BOLD);
    const int titleW = renderer.getTextWidth(UI_10_FONT_ID, truncTitle.c_str(), EpdFontFamily::BOLD);
    renderer.drawText(UI_10_FONT_ID, (rect.width - titleW) / 2, plaqueY, truncTitle.c_str(), true, EpdFontFamily::BOLD);

    // Author (centered, small font)
    if (!centerBook.author.empty()) {
      const std::string truncAuthor = renderer.truncatedText(SMALL_FONT_ID, centerBook.author.c_str(), rect.width - 40);
      const int authorW = renderer.getTextWidth(SMALL_FONT_ID, truncAuthor.c_str());
      renderer.drawText(SMALL_FONT_ID, (rect.width - authorW) / 2, plaqueY + 16, truncAuthor.c_str(), true);
    }

    // Progress bar + % label
    const int barW = 160;
    const int barH = 5;
    const int barX = (rect.width - barW) / 2;
    const int barY = plaqueY + 32;
    renderer.drawRoundedRect(barX, barY, barW, barH, 1, 2, true);

    int pct = (progressPercent >= 0.0f) ? static_cast<int>(progressPercent + 0.5f) : 0;
    if (pct > 100) pct = 100;
    if (pct > 0) {
      const int fillW = std::clamp((barW - 2) * pct / 100, 1, barW - 2);
      renderer.fillRect(barX + 1, barY + 1, fillW, barH - 2);
    }

    char pctBuf[32];
    if (pct > 0) {
      snprintf(pctBuf, sizeof(pctBuf), "%d%%", pct);
    } else if (stats != nullptr && stats->totalPagesTurned > 0) {
      snprintf(pctBuf, sizeof(pctBuf), "%u p.", static_cast<unsigned>(stats->totalPagesTurned));
    } else {
      snprintf(pctBuf, sizeof(pctBuf), "0%%");
    }
    renderer.drawText(SMALL_FONT_ID, barX + barW + 8, barY - 4, pctBuf, true);

    // Carousel pagination dots
    const int dotCount = std::min(10, totalBooks);
    if (dotCount > 1) {
      constexpr int dotSpacing = 12;
      const int dotsW = (dotCount - 1) * dotSpacing;
      const int dotsStartX = (rect.width - dotsW) / 2;
      const int dotsY = plaqueY + 46;

      for (int i = 0; i < dotCount; ++i) {
        const int dx = dotsStartX + i * dotSpacing;
        if (i == curIdx) {
          renderer.fillRoundedRect(dx - 3, dotsY - 3, 6, 6, 3, Color::Black);
        } else {
          renderer.drawRoundedRect(dx - 3, dotsY - 3, 6, 6, 1, 3, true);
        }
      }
    }

    coverBufferStored = storeCoverBuffer();
    coverRendered = coverBufferStored;
  }
}

void LyraCarouselTheme::drawButtonMenu(GfxRenderer& renderer, Rect rect, int buttonCount, int selectedIndex,
                                      const std::function<std::string(int index)>& buttonLabel,
                                      const std::function<UIIcon(int index)>& rowIcon) const {
  const auto& metrics = LyraCarouselMetrics::values;
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

    // Check if this row is the search item
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
