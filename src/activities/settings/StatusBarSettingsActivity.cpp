#include "StatusBarSettingsActivity.h"

#include <GfxRenderer.h>
#include <HalClock.h>
#include <I18n.h>

#include <cstring>
#include <memory>

#include "ClockOffsetActivity.h"
#include "ClockSyncActivity.h"
#include "CrossPointSettings.h"
#include "MappedInputManager.h"
#include "components/UITheme.h"
#include "fontIds.h"

namespace fui = freeink::ui;

namespace {

constexpr int PROGRESS_BAR_ITEMS = 3;
const StrId progressBarNames[PROGRESS_BAR_ITEMS] = {StrId::STR_BOOK, StrId::STR_CHAPTER, StrId::STR_HIDE};

constexpr int PROGRESS_BAR_THICKNESS_ITEMS = 3;
const StrId progressBarThicknessNames[PROGRESS_BAR_THICKNESS_ITEMS] = {
    StrId::STR_PROGRESS_BAR_THIN, StrId::STR_PROGRESS_BAR_MEDIUM, StrId::STR_PROGRESS_BAR_THICK};

constexpr int TITLE_ITEMS = 3;
const StrId titleNames[TITLE_ITEMS] = {StrId::STR_BOOK, StrId::STR_CHAPTER, StrId::STR_HIDE};

constexpr int PAGE_COUNT_ITEMS = 3;
const StrId pageCountNames[PAGE_COUNT_ITEMS] = {StrId::STR_HIDE, StrId::STR_CHAPTER, StrId::STR_BOOK};

constexpr int XTC_STATUS_BAR_ITEMS = 3;
const StrId xtcStatusBarNames[XTC_STATUS_BAR_ITEMS] = {StrId::STR_HIDE, StrId::STR_BOTTOM, StrId::STR_TOP};

constexpr int STATUS_BAR_CLOCK_ITEMS = CrossPointSettings::STATUS_BAR_CLOCK_MODE_COUNT;
const StrId statusBarClockNames[STATUS_BAR_CLOCK_ITEMS] = {StrId::STR_HIDE, StrId::STR_DIR_RIGHT,
                                                           StrId::STR_DIR_LEFT, StrId::STR_CLOCK_CENTER};

constexpr int STATUS_BAR_POSITION_ITEMS = CrossPointSettings::STATUS_BAR_POSITION_COUNT;
const StrId statusBarPositionNames[STATUS_BAR_POSITION_ITEMS] = {StrId::STR_BOTTOM, StrId::STR_TOP};

constexpr int CLOCK_FORMAT_ITEMS = 2;
const StrId clockFormatNames[CLOCK_FORMAT_ITEMS] = {StrId::STR_CLOCK_FORMAT_24H, StrId::STR_CLOCK_FORMAT_12H};

const int verticalPreviewTextPadding = 40;

std::string formatUtcOffset(uint8_t biasedQ) {
  if (biasedQ > 104) biasedQ = 48;
  int totalMinutes = (static_cast<int>(biasedQ) - 48) * 15;
  bool neg = totalMinutes < 0;
  int absMinutes = neg ? -totalMinutes : totalMinutes;
  int hours = absMinutes / 60;
  int mins = absMinutes % 60;
  char buf[16];
  snprintf(buf, sizeof(buf), "UTC%c%d:%02d", neg ? '-' : '+', hours, mins);
  return buf;
}

}  // namespace

StatusBarSettingsActivity::StatusBarSettingsActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : UiListActivity("StatusBarSettings", renderer, mappedInput) {}

void StatusBarSettingsActivity::onEnter() {
  UiListActivity::onEnter();

  if (SETTINGS.statusBarChapterPageCount >= PAGE_COUNT_ITEMS) {
    SETTINGS.statusBarChapterPageCount = CrossPointSettings::PAGE_COUNT_CHAPTER;
  }

  if (SETTINGS.statusBarProgressBar >= PROGRESS_BAR_ITEMS) {
    SETTINGS.statusBarProgressBar = CrossPointSettings::STATUS_BAR_PROGRESS_BAR::HIDE_PROGRESS;
  }

  if (SETTINGS.statusBarProgressBarThickness >= PROGRESS_BAR_THICKNESS_ITEMS) {
    SETTINGS.statusBarProgressBarThickness = CrossPointSettings::STATUS_BAR_PROGRESS_BAR_THICKNESS::PROGRESS_BAR_NORMAL;
  }

  if (SETTINGS.statusBarTitle >= TITLE_ITEMS) {
    SETTINGS.statusBarTitle = CrossPointSettings::STATUS_BAR_TITLE::HIDE_TITLE;
  }

  if (SETTINGS.xtcStatusBarMode >= XTC_STATUS_BAR_ITEMS) {
    SETTINGS.xtcStatusBarMode = CrossPointSettings::XTC_STATUS_BAR_MODE::XTC_STATUS_BAR_HIDE;
  }

  if (SETTINGS.clockUtcOffsetQ > 104) {
    SETTINGS.clockUtcOffsetQ = 48;
  }

  if (SETTINGS.clockFormat >= CLOCK_FORMAT_ITEMS) {
    SETTINGS.clockFormat = 0;
  }

  if (SETTINGS.statusBarClock >= STATUS_BAR_CLOCK_ITEMS) {
    SETTINGS.statusBarClock = CrossPointSettings::STATUS_BAR_CLOCK_MODE::STATUS_BAR_CLOCK_HIDE;
  }
  updateVisibleItems();
}


void StatusBarSettingsActivity::updateVisibleItems() {
  const bool isRu = (I18N.getLanguage() == Language::RU);
  visibleItemCount = 0;

  auto addHeader = [&](const char* title) {
    rowItems_[visibleItemCount].label = title;
    rowItems_[visibleItemCount].isHeader = true;
    rowItems_[visibleItemCount].actionValue = -1;
    visibleItemCount++;
  };

  auto addItem = [&](const char* title, int actionValue) {
    rowItems_[visibleItemCount].label = title;
    rowItems_[visibleItemCount].isHeader = false;
    rowItems_[visibleItemCount].actionValue = actionValue;
    visibleItemCount++;
  };

  addHeader(isRu ? "Отображение" : "Display");
  addItem(I18N.get(StrId::STR_STATUS_BAR_HIDDEN), 0);
  addItem(I18N.get(StrId::STR_STATUS_BAR_POSITION), 1);
  addItem(I18N.get(StrId::STR_XTC_STATUS_BAR), 2);

  addHeader(isRu ? "Элементы" : "Elements");
  addItem(I18N.get(StrId::STR_CHAPTER_PAGE_COUNT), 10);
  addItem(I18N.get(StrId::STR_BOOK_PROGRESS_PERCENTAGE), 11);
  addItem(I18N.get(StrId::STR_STATS_TIME_LEFT), 12);
  addItem(I18N.get(StrId::STR_PROGRESS_BAR), 13);
  addItem(I18N.get(StrId::STR_PROGRESS_BAR_THICKNESS), 14);
  addItem(I18N.get(StrId::STR_TITLE), 15);
  addItem(I18N.get(StrId::STR_BATTERY), 16);

  addHeader(I18N.get(StrId::STR_CLOCK));
  addItem(I18N.get(StrId::STR_CLOCK), 20);
  addItem(I18N.get(StrId::STR_CLOCK_FORMAT), 21);
    addItem(I18N.get(StrId::STR_CLOCK_UTC_OFFSET), 22);
    addItem(I18N.get(StrId::STR_CLOCK_SYNC_NOW), 23);
}

bool StatusBarSettingsActivity::handleButtons() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    finish();
    return true;
  }
  return UiListActivity::handleButtons();
}
void StatusBarSettingsActivity::drawChrome() {}

void StatusBarSettingsActivity::activateIndex(const int index) {
  if (optionPopup.isActive()) return;
  nav.selected = index;
  app.clearTapFlash();
  handleSelection();
  requestUpdate();
}


void StatusBarSettingsActivity::handleSelection() {
  if (nav.selected >= visibleItemCount) return;
  int action = rowItems_[nav.selected].actionValue;
  if (action == -1) return;

  switch (action) {
    case 0:
      SETTINGS.statusBarHidden = !SETTINGS.statusBarHidden;
      break;
    case 1:
      SETTINGS.statusBarPosition = (SETTINGS.statusBarPosition + 1) % 2;
      break;
    case 2:
      SETTINGS.xtcStatusBarMode = (SETTINGS.xtcStatusBarMode + 1) % 3;
      break;
    case 10:
      optionPopup.show(StrId::STR_CHAPTER_PAGE_COUNT, pageCountNames, PAGE_COUNT_ITEMS,
                       SETTINGS.statusBarChapterPageCount, [this](int idx) {
                         SETTINGS.statusBarChapterPageCount = idx;
                         SETTINGS.saveToFile();
                       });
      return;
    case 11:
      SETTINGS.statusBarBookProgressPercentage = (SETTINGS.statusBarBookProgressPercentage + 1) % 2;
      break;
    case 12:
      SETTINGS.statusBarTimeLeft = (SETTINGS.statusBarTimeLeft + 1) % 2;
      break;
    case 13:
      optionPopup.show(StrId::STR_PROGRESS_BAR, progressBarNames, PROGRESS_BAR_ITEMS, SETTINGS.statusBarProgressBar,
                       [this](int idx) {
                         SETTINGS.statusBarProgressBar = idx;
                         SETTINGS.saveToFile();
                       });
      return;
    case 14:
      optionPopup.show(StrId::STR_PROGRESS_BAR_THICKNESS, progressBarThicknessNames, PROGRESS_BAR_THICKNESS_ITEMS,
                       SETTINGS.statusBarProgressBarThickness, [this](int idx) {
                         SETTINGS.statusBarProgressBarThickness = idx;
                         SETTINGS.saveToFile();
                       });
      return;
    case 15:
      optionPopup.show(StrId::STR_TITLE, titleNames, TITLE_ITEMS, SETTINGS.statusBarTitle, [this](int idx) {
        SETTINGS.statusBarTitle = idx;
        SETTINGS.saveToFile();
      });
      return;
    case 16:
      if (!SETTINGS.statusBarBattery) {
        SETTINGS.statusBarBattery = 1;
        SETTINGS.batteryStyle = CrossPointSettings::BATTERY_STYLE_ICON_AND_PERCENT;
      } else if (SETTINGS.batteryStyle == CrossPointSettings::BATTERY_STYLE_ICON_AND_PERCENT) {
        SETTINGS.batteryStyle = CrossPointSettings::BATTERY_STYLE_PERCENT_ONLY;
      } else if (SETTINGS.batteryStyle == CrossPointSettings::BATTERY_STYLE_PERCENT_ONLY) {
        SETTINGS.batteryStyle = CrossPointSettings::BATTERY_STYLE_ICON_ONLY;
      } else {
        SETTINGS.statusBarBattery = 0;
      }
      break;
    case 20:
      SETTINGS.statusBarClock = (SETTINGS.statusBarClock + 1) % STATUS_BAR_CLOCK_ITEMS;
      break;
    case 21:
      SETTINGS.clockFormat = (SETTINGS.clockFormat + 1) % CLOCK_FORMAT_ITEMS;
      break;
    case 22:
      startActivityForResult(std::make_unique<ClockOffsetActivity>(renderer, mappedInput), nullptr);
      return;
    case 23:
      startActivityForResult(std::make_unique<ClockSyncActivity>(renderer, mappedInput), nullptr);
      return;
  }
  SETTINGS.saveToFile();
}

std::string StatusBarSettingsActivity::rowValueText(const int index) {
  if (index >= visibleItemCount) return "";
  int action = rowItems_[index].actionValue;
  if (action == -1) return "";

  switch (action) {
    case 0: return SETTINGS.statusBarHidden ? tr(STR_HIDE) : tr(STR_SHOW);
    case 1: return I18N.get(statusBarPositionNames[SETTINGS.statusBarPosition]);
    case 2: return I18N.get(xtcStatusBarNames[SETTINGS.xtcStatusBarMode]);
    case 10: return I18N.get(pageCountNames[SETTINGS.statusBarChapterPageCount]);
    case 11: return SETTINGS.statusBarBookProgressPercentage ? tr(STR_SHOW) : tr(STR_HIDE);
    case 12: return SETTINGS.statusBarTimeLeft ? tr(STR_SHOW) : tr(STR_HIDE);
    case 13: return I18N.get(progressBarNames[SETTINGS.statusBarProgressBar]);
    case 14: return I18N.get(progressBarThicknessNames[SETTINGS.statusBarProgressBarThickness]);
    case 15: return I18N.get(titleNames[SETTINGS.statusBarTitle]);
    case 16:
      if (!SETTINGS.statusBarBattery) return tr(STR_HIDE);
      if (SETTINGS.batteryStyle == CrossPointSettings::BATTERY_STYLE_PERCENT_ONLY) {
        return I18N.getLanguage() == Language::RU ? "Только %" : "Only %";
      }
      if (SETTINGS.batteryStyle == CrossPointSettings::BATTERY_STYLE_ICON_ONLY) {
        return I18N.getLanguage() == Language::RU ? "Только иконка" : "Only icon";
      }
      return I18N.getLanguage() == Language::RU ? "Иконка и %" : "Icon & %";
    case 20: return I18N.get(statusBarClockNames[SETTINGS.statusBarClock]);
    case 21: {
      const uint8_t fmt = SETTINGS.clockFormat < CLOCK_FORMAT_ITEMS ? SETTINGS.clockFormat : 0;
      return std::string(I18N.get(clockFormatNames[fmt]));
    }
    case 22: return formatUtcOffset(SETTINGS.clockUtcOffsetQ);
    case 23: return SETTINGS.clockHasBeenSynced ? tr(STR_CLOCK_SYNCED) : tr(STR_NOT_SET);
  }
  return "";
}
void StatusBarSettingsActivity::buildScreen(UiScreen& screen) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int statusBarHeight = UITheme::getInstance().getStatusBarHeight();
  const auto previewFooter =
      static_cast<int16_t>(statusBarHeight + verticalPreviewTextPadding + metrics.verticalSpacing);
  screen.setContentMargin(fui::Insets{static_cast<int16_t>(metrics.topPadding + metrics.headerHeight), 0,
                                      static_cast<int16_t>(metrics.buttonHintsHeight + previewFooter), 0});
  screen.spacer(static_cast<int16_t>(metrics.verticalSpacing));

  updateVisibleItems();

  for (int i = 0; i < visibleItemCount; i++) {
    rowValues_[i] = rowValueText(i);
    rowItems_[i].value = rowValues_[i].empty() ? nullptr : rowValues_[i].c_str();
  }

  fui::ListProps props;
  props.items = rowItems_;
  props.count = static_cast<uint16_t>(visibleItemCount);
  props.action = ACTION_ROW;
  props.inputMask = fui::InputTouch;
  props.valueInset = 8;
  props.labelText = screen.theme().smallText;
  props.labelText.maxLines = 2;
  syncListViewport(screen, props);
  screen.list(props);
}

void StatusBarSettingsActivity::render(RenderLock&&) {
  if (optionPopup.processRender(renderer, mappedInput)) return;

  renderer.clearScreen();

  auto metrics = UITheme::getInstance().getMetrics();
  const auto pageWidth = renderer.getScreenWidth();

  const char* title = tr(STR_CUSTOMISE_STATUS_BAR);

  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, title);

  renderUi();

  const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_TOGGLE), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  std::string previewTitle;
  if (SETTINGS.statusBarTitle == CrossPointSettings::STATUS_BAR_TITLE::BOOK_TITLE) {
    previewTitle = tr(STR_EXAMPLE_BOOK);
  } else if (SETTINGS.statusBarTitle == CrossPointSettings::STATUS_BAR_TITLE::CHAPTER_TITLE) {
    previewTitle = tr(STR_EXAMPLE_CHAPTER);
  }

  GUI.drawStatusBar(renderer, 75, 8, 32, previewTitle, metrics.buttonHintsHeight, 0, false, false, false, true, 4500);

  renderer.drawCenteredText(UI_10_FONT_ID,
                            renderer.getScreenHeight() - UITheme::getInstance().getStatusBarHeight() -
                                metrics.buttonHintsHeight - verticalPreviewTextPadding,
                            tr(STR_PREVIEW));

  renderer.displayBuffer();
}

bool StatusBarSettingsActivity::handleCustomInput() {
  return optionPopup.handleInput(mappedInput, [this] { requestUpdate(); });
}
