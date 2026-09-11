#include "SettingsActivity.h"

#include <BoardConfig.h>
#include <GfxRenderer.h>
#include <HalDisplay.h>
#include <Logging.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <optional>

#include "ButtonRemapActivity.h"
#include "ClearCacheActivity.h"
#include "ClockOffsetActivity.h"
#include "ClockSyncActivity.h"
#include "CrossPointSettings.h"
#include "DictionaryDownloadActivity.h"
#include "../games/GamesActivity.h"
#include "PowerStatsActivity.h"
#include "FontDownloadActivity.h"
#include "KOReaderSettingsActivity.h"
#include "LanguageSelectActivity.h"
#include "MappedInputManager.h"
#include "OpdsServerListActivity.h"
#include "OtaUpdateActivity.h"
#include "ReadingStatsActivity.h"
#include "SdCardFontSystem.h"
#include "SdFirmwareUpdateActivity.h"
#include "SettingsList.h"
#include "StatusBarSettingsActivity.h"
#include "SystemInformationActivity.h"
#include "TextSettingsActivity.h"
#include "activities/network/WifiSelectionActivity.h"
#include "activities/util/IntervalSelectionActivity.h"
#include "components/UITheme.h"
#include "components/UIThemeTokens.h"
#include "components/UiAppHelpers.h"
#include "fontIds.h"

namespace fui = freeink::ui;

const StrId SettingsActivity::categoryNames[categoryCount] = {StrId::STR_CAT_DISPLAY, StrId::STR_CAT_READER,
                                                              StrId::STR_CAT_CONTROLS, StrId::STR_CAT_SYSTEM};

SettingsActivity::SettingsActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : UiTabListActivity("Settings", renderer, mappedInput) {}

const char* SettingsActivity::getFolderLabel(SettingAction action) const {
  const bool isRu = (I18N.getLanguage() == Language::RU);
  switch (action) {
    case SettingAction::DisplaySleepScreen:
      return isRu ? "Экран сна и таймеры" : "Sleep Screen & Timers";
    case SettingAction::ReaderFontLayout:
      return isRu ? "Шрифты и текст" : "Fonts & Typography";
    case SettingAction::ReaderPageLayout:
      return isRu ? "Разметка страницы" : "Page Layout & Margins";
    case SettingAction::ReaderFootnotes:
      return isRu ? "Сноски и цитаты" : "Footnotes & Citations";
    case SettingAction::ControlsPowerButton:
      return isRu ? "Кнопка питания" : "Power Button";
    case SettingAction::ControlsSideGestures:
      return isRu ? "Боковые кнопки и жесты" : "Side Buttons & Gestures";
    case SettingAction::SystemNetwork:
      return isRu ? "Сеть и сервисы" : "Network & Services";
    case SettingAction::SystemFilesCache:
      return isRu ? "Память и файлы" : "Storage & Cache";
    case SettingAction::SystemUpdateLanguage:
      return isRu ? "Обновление и язык" : "Updates & Language";
    default:
      return "";
  }
}

void SettingsActivity::setCurrentSettingsForCategory() {
  if (activeSubmenu != SettingAction::None) {
    switch (activeSubmenu) {
      case SettingAction::DisplaySleepScreen:
        currentSettings = &displaySleepSettings;
        break;
      case SettingAction::ReaderFontLayout:
        currentSettings = &readerFontSettings;
        break;
      case SettingAction::ReaderPageLayout:
        currentSettings = &readerPageLayoutSettings;
        break;
      case SettingAction::ReaderFootnotes:
        currentSettings = &readerFootnotesSettings;
        break;
      case SettingAction::ControlsPowerButton:
        currentSettings = &controlsPowerSettings;
        break;
      case SettingAction::ControlsSideGestures:
        currentSettings = &controlsSideButtonSettings;
        break;
      case SettingAction::SystemNetwork:
        currentSettings = &systemNetworkSettings;
        break;
      case SettingAction::SystemFilesCache:
        currentSettings = &systemFilesCacheSettings;
        break;
      case SettingAction::SystemUpdateLanguage:
        currentSettings = &systemUpdateLanguageSettings;
        break;
      default:
        currentSettings = &displaySettings;
        break;
    }
  } else {
    switch (selectedCategoryIndex) {
      case 0:
        currentSettings = &displaySettings;
        break;
      case 1:
        currentSettings = &readerSettings;
        break;
      case 2:
        currentSettings = &controlsSettings;
        break;
      case 3:
        currentSettings = &systemSettings;
        break;
      default:
        currentSettings = &displaySettings;
        break;
    }
  }
  settingsCount = currentSettings ? static_cast<int>(currentSettings->size()) : 0;
}

void SettingsActivity::openSubmenu(SettingAction action) {
  parentSelectedIndex = activeNav().selected;
  activeSubmenu = action;
  setCurrentSettingsForCategory();
  activeNav().top = 0;
  activeNav().selected = 1;
  rebuildRowItems();
  requestUpdate();
}

void SettingsActivity::closeSubmenu() {
  activeSubmenu = SettingAction::None;
  setCurrentSettingsForCategory();
  activeNav().top = 0;
  activeNav().selected = parentSelectedIndex > 0 ? parentSelectedIndex : 1;
  rebuildRowItems();
  requestUpdate();
}

void SettingsActivity::rebuildSettingsLists() {
  displaySettings.clear();
  readerSettings.clear();
  controlsSettings.clear();
  systemSettings.clear();

  displaySleepSettings.clear();
  readerFontSettings.clear();
  readerPageLayoutSettings.clear();
  readerFootnotesSettings.clear();
  controlsPowerSettings.clear();
  controlsSideButtonSettings.clear();
  systemNetworkSettings.clear();
  systemFilesCacheSettings.clear();
  systemUpdateLanguageSettings.clear();

  // Pick up any fonts uploaded/deleted over the web server
  sdFontSystem.refreshIfDirty();

  // Rescan /dictionaries on every rebuild
  std::vector<DictionaryEntry> dictionaries;
  DictionaryRegistry::discover(dictionaries);

  const auto allSettings = getSettingsList(&sdFontSystem.registry(), &dictionaries);

  auto findSettingByPtr = [&](uint8_t CrossPointSettings::* ptr) -> std::optional<SettingInfo> {
    for (const auto& s : allSettings) {
      if (s.valuePtr == ptr) return s;
    }
    return std::nullopt;
  };

  auto findSettingByKey = [&](const char* key) -> std::optional<SettingInfo> {
    for (const auto& s : allSettings) {
      if (s.key && strcmp(s.key, key) == 0) return s;
    }
    return std::nullopt;
  };

  auto findSettingByNameId = [&](StrId id) -> std::optional<SettingInfo> {
    for (const auto& s : allSettings) {
      if (s.nameId == id) return s;
    }
    return std::nullopt;
  };

  // --- Category 0: Display ---
  // Sleep Submenu
  if (auto s = findSettingByPtr(&CrossPointSettings::sleepScreen)) displaySleepSettings.push_back(*s);
  if (auto s = findSettingByPtr(&CrossPointSettings::sleepScreenCoverMode)) displaySleepSettings.push_back(*s);
  if (auto s = findSettingByPtr(&CrossPointSettings::sleepScreenCoverFilter)) displaySleepSettings.push_back(*s);
  if (auto s = findSettingByPtr(&CrossPointSettings::quickResumeSleepScreen)) displaySleepSettings.push_back(*s);
  if (auto s = findSettingByPtr(&CrossPointSettings::sleepTimeoutMinutes)) displaySleepSettings.push_back(*s);

  // Root Display
  displaySettings.push_back(SettingInfo::Submenu(SettingAction::DisplaySleepScreen));
  if (auto s = findSettingByPtr(&CrossPointSettings::refreshFrequency)) displaySettings.push_back(*s);
  if (auto s = findSettingByPtr(&CrossPointSettings::uiTheme)) displaySettings.push_back(*s);
  if (auto s = findSettingByPtr(&CrossPointSettings::hideBatteryPercentage)) displaySettings.push_back(*s);
  if (!BoardConfig::isX4Pro()) {
    if (auto s = findSettingByPtr(&CrossPointSettings::fadingFix)) displaySettings.push_back(*s);
  }
#if FREEINK_CAP_FRONTLIGHT
  if (auto s = findSettingByPtr(&CrossPointSettings::frontlightRestoreOnWake)) displaySettings.push_back(*s);
#endif

  // --- Category 1: Reader ---
  // Font Submenu
  readerFontSettings.push_back(SettingInfo::Action(StrId::STR_TEXT_SETTINGS, SettingAction::TextSettings));
  readerFontSettings.push_back(SettingInfo::Action(StrId::STR_MANAGE_FONTS, SettingAction::DownloadFonts));
  if (auto s = findSettingByKey("fontFamily")) readerFontSettings.push_back(*s);
  if (auto s = findSettingByKey("fontSize")) readerFontSettings.push_back(*s);
  if (auto s = findSettingByPtr(&CrossPointSettings::lineSpacing)) readerFontSettings.push_back(*s);
  if (auto s = findSettingByPtr(&CrossPointSettings::textAntiAliasing)) readerFontSettings.push_back(*s);

  // Page Layout Submenu
  if (auto s = findSettingByPtr(&CrossPointSettings::screenMargin)) readerPageLayoutSettings.push_back(*s);
  if (auto s = findSettingByPtr(&CrossPointSettings::paragraphAlignment)) readerPageLayoutSettings.push_back(*s);
  if (auto s = findSettingByPtr(&CrossPointSettings::hyphenationEnabled)) readerPageLayoutSettings.push_back(*s);
  if (auto s = findSettingByPtr(&CrossPointSettings::extraParagraphSpacing)) readerPageLayoutSettings.push_back(*s);
  if (auto s = findSettingByPtr(&CrossPointSettings::embeddedStyle)) readerPageLayoutSettings.push_back(*s);
  if (auto s = findSettingByPtr(&CrossPointSettings::orientation)) readerPageLayoutSettings.push_back(*s);

  // Footnotes Submenu
  if (auto s = findSettingByPtr(&CrossPointSettings::footnoteDisplay)) readerFootnotesSettings.push_back(*s);
  if (auto s = findSettingByPtr(&CrossPointSettings::bracketFootnotes)) readerFootnotesSettings.push_back(*s);
  if (auto s = findSettingByNameId(StrId::STR_DICTIONARY)) readerFootnotesSettings.push_back(*s);
  readerFootnotesSettings.push_back(SettingInfo::Action(StrId::STR_DICT_DOWNLOAD, SettingAction::DictionaryDownload));

  // Root Reader
  readerSettings.push_back(SettingInfo::Submenu(SettingAction::ReaderFontLayout));
  readerSettings.push_back(SettingInfo::Submenu(SettingAction::ReaderPageLayout));
  readerSettings.push_back(SettingInfo::Submenu(SettingAction::ReaderFootnotes));
  readerSettings.push_back(SettingInfo::Action(StrId::STR_CUSTOMISE_STATUS_BAR, SettingAction::CustomiseStatusBar));
  readerSettings.push_back(SettingInfo::Action(StrId::STR_READING_STATS, SettingAction::ReadingStats));
  if (auto s = findSettingByPtr(&CrossPointSettings::screenInverted)) readerSettings.push_back(*s);
  if (auto s = findSettingByPtr(&CrossPointSettings::imageRendering)) readerSettings.push_back(*s);
  if (auto s = findSettingByPtr(&CrossPointSettings::focusReadingEnabled)) readerSettings.push_back(*s);
  if (auto s = findSettingByPtr(&CrossPointSettings::trackReadingStats)) readerSettings.push_back(*s);

  // --- Category 2: Controls ---
  // Power Button Submenu
  if (auto s = findSettingByPtr(&CrossPointSettings::shortPwrBtn)) controlsPowerSettings.push_back(*s);
  if (SETTINGS.shortPwrBtn == CrossPointSettings::SHORT_PWRBTN::FOOTNOTES) {
    if (auto s = findSettingByPtr(&CrossPointSettings::pwrBtnFootnoteBack)) controlsPowerSettings.push_back(*s);
  }

  // Side Gestures Submenu
  if (auto s = findSettingByPtr(&CrossPointSettings::sideButtonLayout)) controlsSideButtonSettings.push_back(*s);
  if (auto s = findSettingByPtr(&CrossPointSettings::touchReaderControls)) controlsSideButtonSettings.push_back(*s);
  if (auto s = findSettingByPtr(&CrossPointSettings::tapForReaderMenu)) controlsSideButtonSettings.push_back(*s);
  if (auto s = findSettingByPtr(&CrossPointSettings::frontButtonFollowOrientation)) {
    controlsSideButtonSettings.push_back(*s);
  }
  if (auto s = findSettingByPtr(&CrossPointSettings::longPressButtonBehavior)) {
    controlsSideButtonSettings.push_back(*s);
  }
  if (auto s = findSettingByPtr(&CrossPointSettings::longPressMenuFunction)) {
    controlsSideButtonSettings.push_back(*s);
  }
  if (auto s = findSettingByPtr(&CrossPointSettings::backShortToFileBrowser)) {
    controlsSideButtonSettings.push_back(*s);
  }

  // Root Controls
  if (!BoardConfig::hasTouch()) {
    controlsSettings.push_back(SettingInfo::Action(StrId::STR_REMAP_FRONT_BUTTONS, SettingAction::RemapFrontButtons));
  }
  controlsSettings.push_back(SettingInfo::Submenu(SettingAction::ControlsPowerButton));
  controlsSettings.push_back(SettingInfo::Submenu(SettingAction::ControlsSideGestures));

  // --- Category 3: System ---
  // Network Submenu
  systemNetworkSettings.push_back(SettingInfo::Action(StrId::STR_WIFI_NETWORKS, SettingAction::Network));
  if (auto s = findSettingByPtr(&CrossPointSettings::wifiAutoOffMinutes)) systemNetworkSettings.push_back(*s);
  systemNetworkSettings.push_back(SettingInfo::Action(StrId::STR_OPDS_SERVERS, SettingAction::OPDSBrowser));
  systemNetworkSettings.push_back(SettingInfo::Action(StrId::STR_KOREADER_SYNC, SettingAction::KOReaderSync));

  // Files & Cache Submenu
  if (auto s = findSettingByPtr(&CrossPointSettings::showHiddenFiles)) systemFilesCacheSettings.push_back(*s);
  if (auto s = findSettingByPtr(&CrossPointSettings::fileSortMode)) systemFilesCacheSettings.push_back(*s);
  if (auto s = findSettingByPtr(&CrossPointSettings::fileSortDirection)) systemFilesCacheSettings.push_back(*s);
  if (auto s = findSettingByPtr(&CrossPointSettings::removeReadBooksFromRecents)) {
    systemFilesCacheSettings.push_back(*s);
  }
  if (auto s = findSettingByPtr(&CrossPointSettings::moveFinishedToReadFolder)) {
    systemFilesCacheSettings.push_back(*s);
  }
  systemFilesCacheSettings.push_back(SettingInfo::Action(StrId::STR_CLEAR_READING_CACHE, SettingAction::ClearCache));

  // Updates & Language Submenu
  systemUpdateLanguageSettings.push_back(SettingInfo::Action(StrId::STR_LANGUAGE, SettingAction::Language));
  systemUpdateLanguageSettings.push_back(SettingInfo::Action(StrId::STR_CHECK_UPDATES, SettingAction::CheckForUpdates));
  systemUpdateLanguageSettings.push_back(
      SettingInfo::Action(StrId::STR_SD_FIRMWARE_UPDATE, SettingAction::SdFirmwareUpdate));

  // Root System: TOP ITEM (#1) IS SYSTEM DIAGNOSTICS
  systemSettings.push_back(SettingInfo::Action(StrId::STR_SYSTEM_INFO, SettingAction::SystemInfo));
  systemSettings.push_back(SettingInfo::Submenu(SettingAction::SystemNetwork));
  systemSettings.push_back(SettingInfo::Submenu(SettingAction::SystemFilesCache));
  systemSettings.push_back(SettingInfo::Submenu(SettingAction::SystemUpdateLanguage));
  if (auto s = findSettingByPtr(&CrossPointSettings::clockFormat)) systemSettings.push_back(*s);
  systemSettings.push_back(SettingInfo::Action(StrId::STR_CLOCK_UTC_OFFSET, SettingAction::ClockUtcOffset));
  systemSettings.push_back(SettingInfo::Action(StrId::STR_CLOCK_SYNC, SettingAction::ClockSync));
  if (auto s = findSettingByPtr(&CrossPointSettings::batteryLogEnabled)) systemSettings.push_back(*s);

  setCurrentSettingsForCategory();
  rebuildRowItems();
}

void SettingsActivity::onEnter() {
  UiTabListActivity::onEnter();

  selectedCategoryIndex = 0;
  activeSubmenu = SettingAction::None;
  preserveQuickResumeTimeoutOn =
      SETTINGS.quickResumeSleepScreen == CrossPointSettings::QUICK_RESUME_SLEEP_SCREEN::QUICK_RESUME_AFTER_TIMEOUT;
  quickResumeTimeoutAutoEnabled = false;
  syncQuickResumeTimeoutForSleepScreen(/*sleepScreenChanged=*/true, /*quickResumeTimeoutChanged=*/false);

  rebuildSettingsLists();
}

void SettingsActivity::selectCategory(const int categoryIndex) {
  selectedCategoryIndex = categoryIndex;
  activeSubmenu = SettingAction::None;
  setCurrentSettingsForCategory();
  activeNav().top = 0;
  rebuildRowItems();
}

void SettingsActivity::rebuildRowItems() {
  if (!currentSettings) return;
  const auto& settings = *currentSettings;
  rowValues_.assign(settings.size(), std::string());
  rowItems_.clear();
  rowItems_.reserve(settings.size());
  for (size_t i = 0; i < settings.size(); i++) {
    fui::ListItem item;
    if (settings[i].type == SettingType::SUBMENU) {
      item.label = settings[i].customLabel ? settings[i].customLabel : getFolderLabel(settings[i].action);
    } else if (settings[i].customLabel != nullptr) {
      item.label = settings[i].customLabel;
    } else {
      item.label = I18N.get(settings[i].nameId);
    }
    item.actionValue = static_cast<int16_t>(i);
    rowItems_.push_back(item);
  }
}

void SettingsActivity::onTabAction(const int index) {
  if (optionPopup.isActive()) return;
  if (activeSubmenu != SettingAction::None) {
    closeSubmenu();
    return;
  }
  selectCategory(index);
  activeNav().selected = 0;
  app.clearTapFlash();
}

void SettingsActivity::activateIndex(const int index) {
  if (optionPopup.isActive()) return;
  (void)index;
  app.clearTapFlash();
  toggleCurrentSetting();
}

void SettingsActivity::onExit() {
  Activity::onExit();
  UITheme::getInstance().reload();
}

void SettingsActivity::applyUiSettingChange(uint8_t CrossPointSettings::* valuePtr) {
  if (valuePtr != &CrossPointSettings::uiTheme) {
    return;
  }
  UITheme::getInstance().reload();
  resetUi();
}

bool SettingsActivity::handleCustomInput() {
  return optionPopup.handleInput(mappedInput, [this] { requestUpdate(); });
}

void SettingsActivity::stepTab(const int direction) {
  if (activeSubmenu != SettingAction::None) {
    closeSubmenu();
    return;
  }
  const bool onTabBar = ringPos() == 0;
  selectedCategoryIndex = direction > 0 ? ButtonNavigator::nextIndex(selectedCategoryIndex, categoryCount)
                                        : ButtonNavigator::previousIndex(selectedCategoryIndex, categoryCount);
  selectCategory(selectedCategoryIndex);
  activeNav().selected = onTabBar ? 0 : 1;
  requestUpdate();
}

bool SettingsActivity::handleButtons() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    if (ringPos() == 0) {
      if (activeSubmenu != SettingAction::None) {
        closeSubmenu();
      } else {
        stepTab(1);
      }
    } else {
      toggleCurrentSetting();
      requestUpdate();
    }
    return true;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    if (activeSubmenu != SettingAction::None) {
      closeSubmenu();
      return true;
    }
    if (ringPos() > 0) {
      activeNav().selected = 0;
      requestUpdate();
    } else {
      SETTINGS.saveToFile();
      onGoHome();
    }
    return true;
  }

  return false;
}

void SettingsActivity::toggleCurrentSetting() {
  int selectedSetting = ringPos() - 1;
  if (selectedSetting < 0 || selectedSetting >= settingsCount) {
    return;
  }

  const auto& setting = (*currentSettings)[selectedSetting];

  if (setting.type == SettingType::SUBMENU) {
    openSubmenu(setting.action);
    return;
  }

  const bool sleepScreenChanged = setting.valuePtr == &CrossPointSettings::sleepScreen;
  const bool quickResumeTimeoutChanged = setting.valuePtr == &CrossPointSettings::quickResumeSleepScreen;

  if (setting.nameId == StrId::STR_TIME_TO_SLEEP) {
    openSleepTimeoutPicker();
    return;
  }

  if (setting.type == SettingType::TOGGLE && setting.valuePtr != nullptr) {
    const bool currentValue = SETTINGS.*(setting.valuePtr);
    SETTINGS.*(setting.valuePtr) = !currentValue;
  } else if (setting.type == SettingType::ENUM && setting.valuePtr != nullptr) {
    const uint8_t currentValue = SETTINGS.*(setting.valuePtr);
    if (setting.enumValues.size() > 2) {
      const auto valuePtr = setting.valuePtr;
      optionPopup.show(setting.nameId, setting.enumValues.data(), static_cast<int>(setting.enumValues.size()),
                       currentValue, [this, valuePtr, sleepScreenChanged, quickResumeTimeoutChanged](int idx) {
                         SETTINGS.*valuePtr = idx;
                         syncQuickResumeTimeoutForSleepScreen(sleepScreenChanged, quickResumeTimeoutChanged);
                         SETTINGS.saveToFile();
                         rebuildSettingsLists();
                         applyUiSettingChange(valuePtr);
                       });
      requestUpdate();
      return;
    }
    SETTINGS.*(setting.valuePtr) = (currentValue + 1) % static_cast<uint8_t>(setting.enumValues.size());
  } else if (setting.type == SettingType::ENUM && setting.valueGetter && setting.valueSetter) {
    const uint8_t totalValues = setting.enumStringValues.empty()
                                    ? static_cast<uint8_t>(setting.enumValues.size())
                                    : static_cast<uint8_t>(setting.enumStringValues.size());
    const uint8_t cur = setting.valueGetter();
    if (totalValues > 2) {
      const auto valueSetter = setting.valueSetter;
      auto onSelect = [this, valueSetter, sleepScreenChanged, quickResumeTimeoutChanged](int idx) {
        valueSetter(idx);
        syncQuickResumeTimeoutForSleepScreen(sleepScreenChanged, quickResumeTimeoutChanged);
        SETTINGS.saveToFile();
        rebuildSettingsLists();
      };
      if (!setting.enumStringValues.empty()) {
        optionPopup.show(setting.nameId, setting.enumStringValues, cur, std::move(onSelect));
      } else {
        optionPopup.show(setting.nameId, setting.enumValues.data(), static_cast<int>(setting.enumValues.size()), cur,
                          std::move(onSelect));
      }
      requestUpdate();
      return;
    }
    setting.valueSetter((cur + 1) % totalValues);
  } else if (setting.type == SettingType::VALUE && setting.valuePtr != nullptr) {
    const int8_t currentValue = SETTINGS.*(setting.valuePtr);
    if (currentValue + setting.valueRange.step > setting.valueRange.max) {
      SETTINGS.*(setting.valuePtr) = setting.valueRange.min;
    } else {
      SETTINGS.*(setting.valuePtr) = currentValue + setting.valueRange.step;
    }
  } else if (setting.type == SettingType::ACTION) {
    auto resultHandler = [this](const ActivityResult&) {
      SETTINGS.saveToFile();
      rebuildSettingsLists();
      requestUpdate();
    };

    switch (setting.action) {
      case SettingAction::RemapFrontButtons:
        startActivityForResult(std::make_unique<ButtonRemapActivity>(renderer, mappedInput), resultHandler);
        break;
      case SettingAction::CustomiseStatusBar:
        startActivityForResult(std::make_unique<StatusBarSettingsActivity>(renderer, mappedInput), resultHandler);
        break;
      case SettingAction::KOReaderSync:
        startActivityForResult(std::make_unique<KOReaderSettingsActivity>(renderer, mappedInput), resultHandler);
        break;
      case SettingAction::OPDSBrowser:
        startActivityForResult(std::make_unique<OpdsServerListActivity>(renderer, mappedInput), resultHandler);
        break;
      case SettingAction::Network:
        startActivityForResult(std::make_unique<WifiSelectionActivity>(renderer, mappedInput, false), resultHandler);
        break;
      case SettingAction::ClearCache:
        startActivityForResult(std::make_unique<ClearCacheActivity>(renderer, mappedInput), resultHandler);
        break;
      case SettingAction::ReadingStats:
        startActivityForResult(std::make_unique<ReadingStatsActivity>(renderer, mappedInput), resultHandler);
        break;
      case SettingAction::ClockUtcOffset:
        startActivityForResult(std::make_unique<ClockOffsetActivity>(renderer, mappedInput), resultHandler);
        break;
      case SettingAction::ClockSync:
        startActivityForResult(std::make_unique<ClockSyncActivity>(renderer, mappedInput), resultHandler);
        break;
      case SettingAction::DictionaryDownload:
        startActivityForResult(std::make_unique<DictionaryDownloadActivity>(renderer, mappedInput), resultHandler);
        break;
      case SettingAction::Games:
        startActivityForResult(std::make_unique<GamesActivity>(renderer, mappedInput), resultHandler);
        break;
      case SettingAction::Autonomy:
        startActivityForResult(std::make_unique<PowerStatsActivity>(renderer, mappedInput), resultHandler);
        break;
      case SettingAction::CheckForUpdates:
        startActivityForResult(std::make_unique<OtaUpdateActivity>(renderer, mappedInput), resultHandler);
        break;
      case SettingAction::SdFirmwareUpdate:
        startActivityForResult(std::make_unique<SdFirmwareUpdateActivity>(renderer, mappedInput), resultHandler);
        break;
      case SettingAction::DownloadFonts:
        startActivityForResult(std::make_unique<FontDownloadActivity>(renderer, mappedInput), resultHandler);
        break;
      case SettingAction::TextSettings:
        startActivityForResult(std::make_unique<TextSettingsActivity>(renderer, mappedInput, &sdFontSystem.registry(),
                                                                      TextSettingsActivity::Tab::Family),
                               resultHandler);
        break;
      case SettingAction::Language:
        startActivityForResult(std::make_unique<LanguageSelectActivity>(renderer, mappedInput), resultHandler);
        break;
      case SettingAction::SystemInfo:
        startActivityForResult(std::make_unique<SystemInformationActivity>(renderer, mappedInput), resultHandler);
        break;
      default:
        break;
    }
    return;
  } else {
    return;
  }

  syncQuickResumeTimeoutForSleepScreen(sleepScreenChanged, quickResumeTimeoutChanged);
  SETTINGS.saveToFile();
  rebuildSettingsLists();
  applyUiSettingChange(setting.valuePtr);
  activeNav().selected = std::min(ringPos(), settingsCount);
}

void SettingsActivity::syncQuickResumeTimeoutForSleepScreen(bool sleepScreenChanged, bool quickResumeTimeoutChanged) {
  if (quickResumeTimeoutChanged) {
    preserveQuickResumeTimeoutOn =
        SETTINGS.quickResumeSleepScreen == CrossPointSettings::QUICK_RESUME_SLEEP_SCREEN::QUICK_RESUME_AFTER_TIMEOUT;
    quickResumeTimeoutAutoEnabled = false;
  }

  if (SETTINGS.sleepScreen == CrossPointSettings::SLEEP_SCREEN_MODE::QUICK_RESUME) {
    if (SETTINGS.quickResumeSleepScreen != CrossPointSettings::QUICK_RESUME_SLEEP_SCREEN::QUICK_RESUME_AFTER_TIMEOUT) {
      SETTINGS.quickResumeSleepScreen = CrossPointSettings::QUICK_RESUME_SLEEP_SCREEN::QUICK_RESUME_AFTER_TIMEOUT;
      quickResumeTimeoutAutoEnabled = !preserveQuickResumeTimeoutOn;
    } else if (sleepScreenChanged && !preserveQuickResumeTimeoutOn) {
      quickResumeTimeoutAutoEnabled = true;
    }
    return;
  }

  if (sleepScreenChanged && quickResumeTimeoutAutoEnabled && !preserveQuickResumeTimeoutOn) {
    SETTINGS.quickResumeSleepScreen = CrossPointSettings::QUICK_RESUME_SLEEP_SCREEN::QUICK_RESUME_NEVER;
    quickResumeTimeoutAutoEnabled = false;
  }
}

void SettingsActivity::openSleepTimeoutPicker() {
  startActivityForResult(
      std::make_unique<IntervalSelectionActivity>(
          renderer, mappedInput, "SleepTimeoutInterval", StrId::STR_TIME_TO_SLEEP, SETTINGS.sleepTimeoutMinutes,
          CrossPointSettings::MIN_SLEEP_TIMEOUT_MINUTES, CrossPointSettings::MAX_SLEEP_TIMEOUT_MINUTES, 1, 5,
          StrId::STR_SLEEP_TIMER_VALUE_FORMAT, false, StrId::STR_SLEEP_NEVER),
      [this](const ActivityResult& result) {
        if (!result.isCancelled) {
          SETTINGS.sleepTimeoutMinutes = static_cast<uint8_t>(std::get<IntervalResult>(result.data).value);
          SETTINGS.saveToFile();
        }
        requestUpdate();
      });
}

std::string SettingsActivity::settingValueText(const SettingInfo& setting) {
  if (setting.type == SettingType::SUBMENU) {
    return "›";
  }
  if (setting.type == SettingType::ACTION) {
    return "›";
  }
  if (setting.type == SettingType::TOGGLE && setting.valuePtr != nullptr) {
    return SETTINGS.*(setting.valuePtr) ? tr(STR_STATE_ON) : tr(STR_STATE_OFF);
  }
  if (setting.type == SettingType::ENUM && setting.valuePtr != nullptr) {
    const uint8_t value = SETTINGS.*(setting.valuePtr);
    if (value >= setting.enumValues.size()) return "";
    return I18N.get(setting.enumValues[value]);
  }
  if (setting.type == SettingType::ENUM && setting.valueGetter) {
    const uint8_t value = setting.valueGetter();
    if (!setting.enumStringValues.empty() && value < setting.enumStringValues.size()) {
      return setting.enumStringValues[value];
    }
    if (value < setting.enumValues.size()) {
      return I18N.get(setting.enumValues[value]);
    }
    return "";
  }
  if (setting.type == SettingType::VALUE && setting.valuePtr != nullptr) {
    if (setting.nameId == StrId::STR_TIME_TO_SLEEP) {
      if (SETTINGS.sleepTimeoutMinutes >= CrossPointSettings::SLEEP_TIMEOUT_NEVER_MINUTES) {
        return tr(STR_SLEEP_NEVER);
      }
      char valueBuffer[32];
      snprintf(valueBuffer, sizeof(valueBuffer), tr(STR_SLEEP_TIMER_VALUE_FORMAT),
               static_cast<unsigned int>(SETTINGS.*(setting.valuePtr)));
      return valueBuffer;
    }
    return std::to_string(SETTINGS.*(setting.valuePtr));
  }
  return "";
}

void SettingsActivity::buildScreen(UiScreen& screen) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  screen.setContentMargin(fui::Insets{static_cast<int16_t>(metrics.topPadding + metrics.headerHeight), 0,
                                      static_cast<int16_t>(metrics.buttonHintsHeight), 0});

  if (activeSubmenu == SettingAction::None) {
    buildTabBar(screen);
  } else {
    // Breadcrumb header pill in place of tab bar: "‹ Назад"
    fui::TabItem breadcrumbTab[1];
    std::string crumbText = std::string("‹ ") + tr(STR_BACK);
    breadcrumbTab[0].label = crumbText.c_str();
    breadcrumbTab[0].value = 0;
    breadcrumbTab[0].selected = true;

    fui::TabBarProps barProps;
    barProps.tabs = breadcrumbTab;
    barProps.count = 1;
    barProps.action = ACTION_TAB;
    barProps.inputMask = fui::InputTouch;
    barProps.text = screen.theme().smallText;
    barProps.layout = fui::TabBarLayout::ContentWidth;
    barProps.leadingInset = static_cast<int16_t>(metrics.contentSidePadding);
    barProps.tabInset = ringPos() == 0 ? fui::Insets{2, 0, 4, 0} : fui::Insets{2, 0, 0, 0};
    barProps.contentInset = fui::Insets{2, 10, 2, 10};
    barProps.divider = true;

    const bool tabsFocused = ringPos() == 0;
    fui::StyleSet tabStyles;
    tabStyles.explicitlySet = true;
    tabStyles.normal.foreground = fui::Paint::solid(fui::Color::Black);
    if (tabsFocused) {
      tabStyles.selected.background = fui::Paint::solid(fui::Color::Black);
      tabStyles.selected.foreground = fui::Paint::solid(fui::Color::White);
      tabStyles.selected.radius = screen.theme().listRowRadius;
    } else {
      tabStyles.selected.background = fui::Paint::dither(fui::Color::LightGray);
      tabStyles.selected.foreground = fui::Paint::solid(fui::Color::Black);
    }
    tabStyles.focused = tabStyles.selected;
    tabStyles.active = tabStyles.selected;
    barProps.tabStyles = tabStyles;

    const int16_t tabLineHeight = screen.target().lineHeight(barProps.text.font);
    const int16_t tabBand =
        static_cast<int16_t>(metrics.tabBarHeight > tabLineHeight + 10 ? metrics.tabBarHeight : tabLineHeight + 10);
    const fui::Rect tabRect = screen.takeTop(tabBand);
    fui::tabBar(screen.frame(), tabRect, barProps);
    screen.spacer(static_cast<int16_t>(metrics.verticalSpacing));
  }

  const auto& settings = *currentSettings;
  for (size_t i = 0; i < settings.size(); i++) {
    rowValues_[i] = settingValueText(settings[i]);
    rowItems_[i].value = rowValues_[i].empty() ? nullptr : rowValues_[i].c_str();
  }

  fui::ListProps props;
  props.items = rowItems_.data();
  props.count = static_cast<uint16_t>(rowItems_.size());
  props.action = ACTION_ROW;
  props.inputMask = fui::InputTouch;
  props.valueInset = 8;
  props.labelText = screen.theme().smallText;
  props.labelText.maxLines = 2;
  syncTabListViewport(screen, props);
  screen.list(props);
}

void SettingsActivity::render(RenderLock&&) {
  if (optionPopup.processRender(renderer, mappedInput)) return;

  renderer.clearScreen();

  const auto pageWidth = renderer.getScreenWidth();
  const auto& metrics = UITheme::getInstance().getMetrics();

  const char* titleText =
      (activeSubmenu != SettingAction::None) ? getFolderLabel(activeSubmenu) : tr(STR_SETTINGS_TITLE);
  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, titleText,
                 CROSSPOINT_VERSION);

  renderUi();

  const int ring = ringPos();
  const char* confirmLabel = nullptr;
  if (activeSubmenu != SettingAction::None) {
    if (ring == 0) {
      confirmLabel = tr(STR_BACK);
    } else if (ring > 0 && ring <= settingsCount) {
      const auto& curSetting = (*currentSettings)[ring - 1];
      if (curSetting.type == SettingType::SUBMENU || curSetting.type == SettingType::ACTION) {
        confirmLabel = tr(STR_SELECT);
      } else if (curSetting.nameId == StrId::STR_TIME_TO_SLEEP) {
        confirmLabel = tr(STR_SELECT);
      } else {
        confirmLabel = tr(STR_TOGGLE);
      }
    } else {
      confirmLabel = tr(STR_TOGGLE);
    }
  } else {
    if (ring == 0) {
      confirmLabel = I18N.get(categoryNames[(selectedCategoryIndex + 1) % categoryCount]);
    } else if (ring > 0 && ring <= settingsCount) {
      const auto& curSetting = (*currentSettings)[ring - 1];
      if (curSetting.type == SettingType::SUBMENU || curSetting.type == SettingType::ACTION) {
        confirmLabel = tr(STR_SELECT);
      } else if (curSetting.nameId == StrId::STR_TIME_TO_SLEEP) {
        confirmLabel = tr(STR_SELECT);
      } else {
        confirmLabel = tr(STR_TOGGLE);
      }
    } else {
      confirmLabel = tr(STR_TOGGLE);
    }
  }

  const auto labels = mappedInput.mapLabels(tr(STR_BACK), confirmLabel, tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer();
}
