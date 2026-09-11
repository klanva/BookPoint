#include "PowerStatsActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>
#include <time.h>

#include <cstdio>

#include "../../util/PowerStats.h"
#include "components/UITheme.h"
#include "fontIds.h"
#include "MappedInputManager.h"

namespace {
void formatHm(uint32_t seconds, char* buf, size_t len) {
  const uint32_t totalMinutes = seconds / 60;
  const uint32_t days = totalMinutes / (24 * 60);
  const uint32_t hours = (totalMinutes / 60) % 24;
  const uint32_t minutes = totalMinutes % 60;
  if (days > 0) {
    snprintf(buf, len, "%ud %uh", static_cast<unsigned>(days), static_cast<unsigned>(hours));
  } else if (hours > 0) {
    snprintf(buf, len, "%uh %02um", static_cast<unsigned>(hours), static_cast<unsigned>(minutes));
  } else {
    snprintf(buf, len, "%um", static_cast<unsigned>(minutes));
  }
}

void drawStatBlock(class GfxRenderer& renderer, int x, int y, int colWidth, const char* label,
                   const char* value) {
  renderer.drawText(UI_10_FONT_ID, x, y, label);
  const int vw = renderer.getTextAdvanceX(UI_12_FONT_ID, value, EpdFontFamily::BOLD);
  renderer.drawText(UI_12_FONT_ID, x, y + 16, value, true, EpdFontFamily::BOLD);
  (void)colWidth;
}
}  // namespace

void PowerStatsActivity::onEnter() {
  Activity::onEnter();
  PowerStats::update();
  requestUpdate();
}

void PowerStatsActivity::loop() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    finish();
    return;
  }
}

void PowerStatsActivity::render(RenderLock&&) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int pageWidth = renderer.getScreenWidth();
  const int contentWidth = pageWidth - 40;

  renderer.clearScreen();
  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, tr(STR_AUTONOMY));

  const auto snap = PowerStats::snapshot();
  int y = metrics.topPadding + metrics.headerHeight + 16;

  // Charge level: big percent + bar.
  char buf[64];
  snprintf(buf, sizeof(buf), "%u%%", static_cast<unsigned>(snap.batteryPct));
  const int pctW = renderer.getTextAdvanceX(UI_12_FONT_ID, buf, EpdFontFamily::BOLD);
  renderer.drawText(UI_12_FONT_ID, contentWidth - pctW, y, buf, true, EpdFontFamily::BOLD);
  const int barY = y + 4;
  const int barW = contentWidth - pctW - 12;
  renderer.drawRect(0, barY, barW, 10, true);
  if (snap.batteryPct > 0) {
    const int fill = barW * snap.batteryPct / 100;
    if (fill > 0) renderer.fillRect(0, barY, fill, 10, true);
  }
  y = barY + 10 + 18;

  if (snap.charging) {
    renderer.drawCenteredText(UI_10_FONT_ID, y, tr(STR_AUTONOMY_CHARGING), true, EpdFontFamily::BOLD);
  }
  y += 22;

  renderer.drawLine(0, y, contentWidth, y, true);
  y += 12;

  const int col2 = contentWidth / 2;

  // Time on battery.
  formatHm(snap.secondsOnBattery, buf, sizeof(buf));
  drawStatBlock(renderer, 0, y, col2, tr(STR_AUTONOMY_ON_BATTERY), buf);

  // Pages since charge.
  snprintf(buf, sizeof(buf), "%u", static_cast<unsigned>(snap.pagesOnBattery));
  drawStatBlock(renderer, col2, y, col2, tr(STR_AUTONOMY_PAGES), buf);
  y += 52;

  // Days since charge started.
  snprintf(buf, sizeof(buf), "%u", static_cast<unsigned>(snap.cycleDays));
  drawStatBlock(renderer, 0, y, col2, tr(STR_AUTONOMY_DAYS), buf);

  // Last charge end date (from epoch).
  if (snap.lastChargeEndEpoch > 1600000000) {
    const time_t te = static_cast<time_t>(snap.lastChargeEndEpoch);
    struct tm tmUtc;
    gmtime_r(&te, &tmUtc);
    snprintf(buf, sizeof(buf), "%02u.%02u", static_cast<unsigned>(tmUtc.tm_mday),
             static_cast<unsigned>(tmUtc.tm_mon + 1));
    drawStatBlock(renderer, col2, y, col2, tr(STR_AUTONOMY_LAST_CHARGE), buf);
  }
  y += 52;

  renderer.drawLine(0, y - 6, contentWidth, y - 6, true);
  renderer.drawText(UI_10_FONT_ID, 0, y + 2, tr(STR_AUTONOMY_HINT));

  const auto labels = mappedInput.mapLabels(tr(STR_BACK), "", "", "");
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer();
}
