#include "BootActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>

#include "fontIds.h"
#include "images/Logo.h"

void BootActivity::onEnter() {
  Activity::onEnter();

  const auto pageWidth = renderer.getScreenWidth();
  const auto pageHeight = renderer.getScreenHeight();

  const auto logo = (pageWidth >= 480 && pageHeight >= 700) ? LogoAsset{Logo240, 240}
                  : (pageWidth >= 400 && pageHeight >= 400) ? LogoAsset{Logo200, 200}
                                                            : LogoAsset{Logo120, 120};

  const int yOffset = (logo.size >= 240) ? 55 : (logo.size >= 200 ? 35 : 20);
  const int logoX = (pageWidth - logo.size) / 2;
  const int logoY = (pageHeight - logo.size) / 2 - yOffset;

  renderer.clearScreen();
  renderer.drawImage(logo.data, logoX, logoY, logo.size, logo.size);

  const int textTitleY = logoY + logo.size + ((logo.size >= 200) ? 35 : 20);
  const int textSubY = textTitleY + ((logo.size >= 200) ? 28 : 22);

  renderer.drawCenteredText(UI_10_FONT_ID, textTitleY, tr(STR_CROSSPOINT), true, EpdFontFamily::BOLD);
  renderer.drawCenteredText(SMALL_FONT_ID, textSubY, tr(STR_BOOTING));
  renderer.drawCenteredText(SMALL_FONT_ID, pageHeight - 30, CROSSPOINT_VERSION);
  renderer.displayBuffer();
}

