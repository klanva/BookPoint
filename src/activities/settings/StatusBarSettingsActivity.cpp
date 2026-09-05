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

  activeFolder = Folder::None;
  updateVisibleItems();
}

void StatusBarSettingsActivity::updateVisibleItems() {
  const bool isRu = (I18N.getLanguage() == Language::RU);

  switch (activeFolder) {
    case Folder::None: {
      visibleItemCount = 3;
      rowItems_[0].label = isRu ? "Отображение" : "Display";
      rowItems_[0].actionValue = 0;
      rowItems_[1].label = isRu ? "Элементы" : "Elements";
      rowItems_[1].actionValue = 1;
      rowItems_[2].label = I18N.get(StrId::STR_CLOCK);
      rowItems_[2].actionValue = 2;
      break;
    }
    case Folder::Display: {
      visibleItemCount = 3;
      rowItems_[0].label = I18N.get(StrId::STR_STATUS_BAR_HIDDEN);
      rowItems_[0].actionValue = 0;
      rowItems_[1].label = I18N.get(StrId::STR_STATUS_BAR_POSITION);
      rowItems_[1].actionValue = 1;
      rowItems_[2].label = I18N.get(StrId::STR_XTC_STATUS_BAR);
      rowItems_[2].actionValue = 2;
      break;
    }
    case Folder::Elements: {
      visibleItemCount = 7;
      rowItems_[0].label = isRu ? "Номера страниц" : I18N.get(StrId::STR_CHAPTER_PAGE_COUNT);
      rowItems_[0].actionValue = 0;
      rowItems_[1].label = I18N.get(StrId::STR_BOOK_PROGRESS_PERCENTAGE);
      rowItems_[1].actionValue = 1;
      rowItems_[2].label = I18N.get(StrId::STR_STATS_TIME_LEFT);
      rowItems_[2].actionValue = 2;
      rowItems_[3].label = I18N.get(StrId::STR_PROGRESS_BAR);
      rowItems_[3].actionValue = 3;
      rowItems_[4].label = I18N.get(StrId::STR_PROGRESS_BAR_THICKNESS);
      rowItems_[4].actionValue = 4;
      rowItems_[5].label = I18N.get(StrId::STR_TITLE);
      rowItems_[5].actionValue = 5;
      rowItems_[6].label = I18N.get(StrId::STR_BATTERY);
      rowItems_[6].actionValue = 6;
      break;
    }
    case Folder::Clock: {
      visibleItemCount = 4;
      rowItems_[0].label = isRu ? "Часы в строке" : I18N.get(StrId::STR_CLOCK);
      rowItems_[0].actionValue = 0;
      rowItems_[1].label = I18N.get(StrId::STR_CLOCK_FORMAT);
      rowItems_[1].actionValue = 1;
      rowItems_[2].label = I18N.get(StrId::STR_CLOCK_UTC_OFFSET);
      rowItems_[2].actionValue = 2;
      rowItems_[3].label = I18N.get(StrId::STR_CLOCK_SYNC_NOW);
      rowItems_[3].actionValue = 3;
      break;
    }
  }
}

bool StatusBarSettingsActivity::handleCustomInput() {
  return optionPopup.handleInput(mappedInput, [this] { requestUpdate(); });
}

bool StatusBarSettingsActivity::handleButtons() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    if (activeFolder != Folder::None) {
      activeFolder = Folder::None;
      nav.top = 0;
      nav.selected = lastFolderIndex_;
      updateVisibleItems();
      requestUpdate();
      return true;
    }
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
  if (activeFolder == Folder::None) {
    lastFolderIndex_ = nav.selected;
    if (nav.selected == 0) {
      activeFolder = Folder::Display;
    } else if (nav.selected == 1) {
      activeFolder = Folder::Elements;
    } else if (nav.selected == 2) {
      activeFolder = Folder::Clock;
    }
    nav.top = 0;
    nav.selected = 0;
    updateVisibleItems();
    return;
  }

  if (activeFolder == Folder::Display) {
    switch (nav.selected) {
      case 0:
        SETTINGS.statusBarHidden = (SETTINGS.statusBarHidden + 1) % 2;
        break;
      case 1:
        optionPopup.show(StrId::STR_STATUS_BAR_POSITION, statusBarPositionNames, STATUS_BAR_POSITION_ITEMS,
                         SETTINGS.statusBarPosition, [this](int idx) {
                           SETTINGS.statusBarPosition = idx;
                           SETTINGS.saveToFile();
                         });
        return;
      case 2:
        optionPopup.show(StrId::STR_XTC_STATUS_BAR, xtcStatusBarNames, XTC_STATUS_BAR_ITEMS, SETTINGS.xtcStatusBarMode,
                         [this](int idx) {
                           SETTINGS.xtcStatusBarMode = idx;
                           SETTINGS.saveToFile();
                         });
        return;
      default:
        return;
    }
    SETTINGS.saveToFile();
    return;
  }

  if (activeFolder == Folder::Elements) {
    switch (nav.selected) {
      case 0:
        optionPopup.show(StrId::STR_CHAPTER_PAGE_COUNT, pageCountNames, PAGE_COUNT_ITEMS,
                         SETTINGS.statusBarChapterPageCount, [this](int idx) {
                           SETTINGS.statusBarChapterPageCount = idx;
                           SETTINGS.saveToFile();
                         });
        return;
      case 1:
        SETTINGS.statusBarBookProgressPercentage = (SETTINGS.statusBarBookProgressPercentage + 1) % 2;
        break;
      case 2:
        SETTINGS.statusBarTimeLeft = (SETTINGS.statusBarTimeLeft + 1) % 2;
        break;
      case 3:
        optionPopup.show(StrId::STR_PROGRESS_BAR, progressBarNames, PROGRESS_BAR_ITEMS, SETTINGS.statusBarProgressBar,
                         [this](int idx) {
                           SETTINGS.statusBarProgressBar = idx;
                           SETTINGS.saveToFile();
                         });
        return;
      case 4:
        optionPopup.show(StrId::STR_PROGRESS_BAR_THICKNESS, progressBarThicknessNames, PROGRESS_BAR_THICKNESS_ITEMS,
                         SETTINGS.statusBarProgressBarThickness, [this](int idx) {
                           SETTINGS.statusBarProgressBarThickness = idx;
                           SETTINGS.saveToFile();
                         });
        return;
      case 5:
        optionPopup.show(StrId::STR_TITLE, titleNames, TITLE_ITEMS, SETTINGS.statusBarTitle, [this](int idx) {
          SETTINGS.statusBarTitle = idx;
          SETTINGS.saveToFile();
        });
        return;
      case 6:
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
      default:
        return;
    }
    SETTINGS.saveToFile();
    return;
  }

  if (activeFolder == Folder::Clock) {
    switch (nav.selected) {
      case 0:
        SETTINGS.statusBarClock = (SETTINGS.statusBarClock + 1) % STATUS_BAR_CLOCK_ITEMS;
        break;
      case 1:
        SETTINGS.clockFormat = (SETTINGS.clockFormat + 1) % CLOCK_FORMAT_ITEMS;
        break;
      case 2:
        startActivityForResult(std::make_unique<ClockOffsetActivity>(renderer, mappedInput), nullptr);
        return;
      case 3:
        startActivityForResult(std::make_unique<ClockSyncActivity>(renderer, mappedInput), nullptr);
        return;
      default:
        return;
    }
    SETTINGS.saveToFile();
  }
}

std::string StatusBarSettingsActivity::rowValueText(const int index) {
  if (activeFolder == Folder::None) {
    return ">";
  }

  if (activeFolder == Folder::Display) {
    switch (index) {
      case 0:
        return SETTINGS.statusBarHidden ? tr(STR_HIDE) : tr(STR_SHOW);
      case 1:
        return I18N.get(statusBarPositionNames[SETTINGS.statusBarPosition]);
      case 2:
        return I18N.get(xtcStatusBarNames[SETTINGS.xtcStatusBarMode]);
      default:
        return "";
    }
  }

  if (activeFolder == Folder::Elements) {
    switch (index) {
      case 0:
        return I18N.get(pageCountNames[SETTINGS.statusBarChapterPageCount]);
      case 1:
        return SETTINGS.statusBarBookProgressPercentage ? tr(STR_SHOW) : tr(STR_HIDE);
      case 2:
        return SETTINGS.statusBarTimeLeft ? tr(STR_SHOW) : tr(STR_HIDE);
      case 3:
        return I18N.get(progressBarNames[SETTINGS.statusBarProgressBar]);
      case 4:
        return I18N.get(progressBarThicknessNames[SETTINGS.statusBarProgressBarThickness]);
      case 5:
        return I18N.get(titleNames[SETTINGS.statusBarTitle]);
      case 6:
        if (!SETTINGS.statusBarBattery) return tr(STR_HIDE);
        if (SETTINGS.batteryStyle == CrossPointSettings::BATTERY_STYLE_PERCENT_ONLY) {
          return I18N.getLanguage() == Language::RU ? "Только %" : "Only %";
        }
        if (SETTINGS.batteryStyle == CrossPointSettings::BATTERY_STYLE_ICON_ONLY) {
          return I18N.getLanguage() == Language::RU ? "Только иконка" : "Only icon";
        }
        return I18N.getLanguage() == Language::RU ? "Иконка и %" : "Icon & %";
      default:
        return "";
    }
  }

  if (activeFolder == Folder::Clock) {
    switch (index) {
      case 0:
        return I18N.get(statusBarClockNames[SETTINGS.statusBarClock]);
      case 1: {
        const uint8_t fmt = SETTINGS.clockFormat < CLOCK_FORMAT_ITEMS ? SETTINGS.clockFormat : 0;
        return std::string(I18N.get(clockFormatNames[fmt]));
      }
      case 2:
        return formatUtcOffset(SETTINGS.clockUtcOffsetQ);
      case 3:
        return SETTINGS.clockHasBeenSynced ? tr(STR_CLOCK_SYNCED) : tr(STR_NOT_SET);
      default:
        return "";
    }
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
  if (activeFolder == Folder::Display) {
    title = (I18N.getLanguage() == Language::RU) ? "Строка: отображение" : "Status Bar: Display";
  } else if (activeFolder == Folder::Elements) {
    title = (I18N.getLanguage() == Language::RU) ? "Строка: элементы" : "Status Bar: Elements";
  } else if (activeFolder == Folder::Clock) {
    title = (I18N.getLanguage() == Language::RU) ? "Строка: часы" : "Status Bar: Clock";
  }

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
