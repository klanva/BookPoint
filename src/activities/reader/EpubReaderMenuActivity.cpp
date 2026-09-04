#include "EpubReaderMenuActivity.h"

#include <GfxRenderer.h>
#include <HalFrontlight.h>
#include <I18n.h>

#include "CrossPointSettings.h"
#include "KOReaderCredentialStore.h"
#include "MappedInputManager.h"
#include "ReaderUtils.h"
#include "components/UITheme.h"

namespace fui = freeink::ui;

EpubReaderMenuActivity::EpubReaderMenuActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                                               const std::string& title, const int currentPage, const int totalPages,
                                               const int bookProgressPercent, const uint8_t currentOrientation,
                                               const bool hasFootnotes, const bool hasBookmarks)
    : UiListActivity("EpubReaderMenu", renderer, mappedInput),
      menuItems(buildMenuItems(hasFootnotes, hasBookmarks)),
      title(title),
      pendingOrientation(currentOrientation),
      currentPage(currentPage),
      totalPages(totalPages),
      bookProgressPercent(bookProgressPercent) {
  buildMenuRowItems();
}

// Populates menuRowItems's labels/actionValue from menuItems. Called once
// here since menuItems (and thus which rows exist) never changes after
// construction; buildScreen() only touches the two rows with a live value.
void EpubReaderMenuActivity::buildMenuRowItems() {
  for (size_t i = 0; i < menuItems.size() && i < MAX_MENU_ITEMS; i++) {
    fui::ListItem item;
    item.label = I18N.get(menuItems[i].labelId);
    item.actionValue = static_cast<int16_t>(i);
    if (menuItems[i].action == MenuAction::NIGHT_MODE) {
      item.toggle = true;
      item.toggleChecked = (SETTINGS.screenInverted != 0);
    } else if (menuItems[i].action == MenuAction::FRONTLIGHT) {
      item.toggle = true;
      item.toggleChecked = Frontlight.isOn();
    }
    menuRowItems[i] = item;
  }
}

std::vector<EpubReaderMenuActivity::MenuItem> EpubReaderMenuActivity::buildMenuItems(bool hasFootnotes,
                                                                                     bool hasBookmarks) {
  std::vector<MenuItem> items;
  items.reserve(MAX_MENU_ITEMS);
  items.push_back({MenuAction::SELECT_CHAPTER, StrId::STR_SELECT_CHAPTER});
  if (hasBookmarks) {
    items.push_back({MenuAction::BOOKMARKS, StrId::STR_BOOKMARKS});
  }
  items.push_back({MenuAction::CREATE_CLIPPING, StrId::STR_CREATE_CLIPPING});
  items.push_back({MenuAction::VIEW_CLIPPINGS, StrId::STR_CLIPPINGS});
  items.push_back({MenuAction::STATISTICS, StrId::STR_READING_STATS});
  items.push_back({MenuAction::TEXT_SETTINGS, StrId::STR_TEXT_SETTINGS});
  items.push_back({MenuAction::GO_TO_PERCENT, StrId::STR_GO_TO_PERCENT});
  items.push_back({MenuAction::DICTIONARY, StrId::STR_LOOKUP});
  items.push_back({MenuAction::NIGHT_MODE, StrId::STR_NIGHT_MODE});
  if (Frontlight.present()) {
    items.push_back({MenuAction::FRONTLIGHT, StrId::STR_FRONTLIGHT});
  }
  items.push_back({MenuAction::ROTATE_SCREEN, StrId::STR_ORIENTATION});
  items.push_back({MenuAction::AUTO_PAGE_TURN, StrId::STR_AUTO_TURN_PAGES_PER_MIN});
  if (hasFootnotes) {
    items.push_back({MenuAction::FOOTNOTES, StrId::STR_FOOTNOTES});
  }
  if (KOREADER_STORE.hasCredentials()) {
    items.push_back({MenuAction::SYNC, StrId::STR_SYNC_PROGRESS});
  }
  items.push_back({MenuAction::SYSTEM_SETTINGS, StrId::STR_SETTINGS_TITLE});
  items.push_back({MenuAction::GO_HOME, StrId::STR_GO_HOME_BUTTON});
  return items;
}

void EpubReaderMenuActivity::closeCancelled() {
  ActivityResult result;
  result.isCancelled = true;
  result.data = MenuResult{-1, pendingOrientation, selectedPageTurnOption};
  setResult(std::move(result));
  finish();
}

bool EpubReaderMenuActivity::handleHomeGesture() {
  closeCancelled();
  return true;
}

void EpubReaderMenuActivity::activateIndex(const int index) {
  if (optionPopup.isActive()) return;
  // The activated row leaves this screen (popup or finish); a lingering flash
  // would gray an unrelated element on the next render.
  app.clearTapFlash();
  nav.selected = index;

  const auto selectedAction = menuItems[index].action;
  if (selectedAction == MenuAction::ROTATE_SCREEN) {
    optionPopup.show(StrId::STR_ORIENTATION, orientationLabels.data(), static_cast<int>(orientationLabels.size()),
                     pendingOrientation, [this](int idx) {
                       pendingOrientation = idx;
                       // Rotate the menu immediately. Only the renderer turns;
                       // SETTINGS.orientation stays unchanged so the reader's
                       // result handler still detects the change and reflows.
                       ReaderUtils::applyOrientation(renderer, pendingOrientation);
                       app.setDevice(uiTarget.deviceContext());  // hit rects follow the new frame
                       requestUpdate(true);
                     });
    requestUpdate();
    return;
  }

  if (selectedAction == MenuAction::AUTO_PAGE_TURN) {
    optionPopup.show(I18N.get(StrId::STR_AUTO_TURN_PAGES_PER_MIN), pageTurnLabels.data(),
                     static_cast<int>(pageTurnLabels.size()), selectedPageTurnOption, [this](int idx) {
                       selectedPageTurnOption = idx;
                       requestUpdate();
                     });
    requestUpdate();
    return;
  }

  if (selectedAction == MenuAction::NIGHT_MODE) {
    SETTINGS.screenInverted = SETTINGS.screenInverted == 0 ? 1 : 0;
    SETTINGS.saveToFile();
    menuRowItems[index].toggleChecked = (SETTINGS.screenInverted != 0);
    requestUpdate();
    return;
  }

  if (selectedAction == MenuAction::FRONTLIGHT) {
    const bool lightOn = !Frontlight.isOn();
    Frontlight.setOn(lightOn);
    SETTINGS.frontlightOn = lightOn ? 1 : 0;
    SETTINGS.saveToFile();
    menuRowItems[index].toggleChecked = lightOn;
    requestUpdate();
    return;
  }

  setResult(MenuResult{static_cast<int>(selectedAction), pendingOrientation, selectedPageTurnOption});
  finish();
}

bool EpubReaderMenuActivity::handleCustomInput() {
  return optionPopup.handleInput(mappedInput, [this] { requestUpdate(); });
}

bool EpubReaderMenuActivity::handleButtons() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    closeCancelled();
    return true;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    activateIndex(nav.selected);
    return true;
  }

  return false;
}

void EpubReaderMenuActivity::buildScreen(UiScreen& screen) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const Rect safe = UITheme::getInstance().getScreenSafeArea(renderer, true, false);
  // Content: the safe area minus the header band GUI.drawHeader paints.
  screen.setContentMargin(fui::Insets{static_cast<int16_t>(safe.y + metrics.topPadding + metrics.headerHeight),
                                      static_cast<int16_t>(renderer.getScreenWidth() - (safe.x + safe.width)),
                                      static_cast<int16_t>(renderer.getScreenHeight() - (safe.y + safe.height)),
                                      static_cast<int16_t>(safe.x)});

  // Hero Reading Status Card at the top
  const int16_t heroH = 68;
  const fui::Rect hero = screen.takeTop(heroH);

  // Outer frame for hero card (crisp 1px border, 8px rounded corners)
  screen.target().stroke(hero, fui::Paint::solid(fui::Color::Black), 1, 8, fui::CornersAll);

  // Book title (bold, clean)
  fui::TextStyle titleStyle = screen.theme().bodyText;
  titleStyle.bold = true;
  fui::Rect titleRect{static_cast<int16_t>(hero.x + 12), static_cast<int16_t>(hero.y + 8),
                      static_cast<int16_t>(hero.width - 24), 20};
  screen.target().text(titleRect, title.c_str(), titleStyle);

  // Progress bar inside hero card
  const int16_t pctBadgeW = 44;
  const int16_t barW = static_cast<int16_t>(hero.width - 24 - pctBadgeW - 8);
  const int16_t barH = 8;
  const int16_t barY = static_cast<int16_t>(hero.y + 32);
  fui::Rect barRect{static_cast<int16_t>(hero.x + 12), barY, barW, barH};
  screen.target().stroke(barRect, fui::Paint::solid(fui::Color::Black), 1, 4, fui::CornersAll);
  if (bookProgressPercent > 0) {
    const int16_t fillW = static_cast<int16_t>((barW * std::min(bookProgressPercent, 100)) / 100);
    if (fillW > 0) {
      fui::Rect fillRect{barRect.x, barRect.y, fillW, barH};
      screen.target().fill(fillRect, fui::Paint::solid(fui::Color::Black), 4, fui::CornersAll);
    }
  }

  // Progress percent text
  fui::TextStyle pctStyle = screen.theme().bodyText;
  pctStyle.bold = true;
  char pctBuf[16];
  snprintf(pctBuf, sizeof(pctBuf), "%d%%", bookProgressPercent);
  fui::Rect pctRect{static_cast<int16_t>(hero.x + hero.width - 12 - pctBadgeW), static_cast<int16_t>(barY - 5),
                    pctBadgeW, 18};
  screen.target().text(pctRect, pctBuf, pctStyle);

  // Chapter and page details
  std::string progressLine;
  if (totalPages > 0) {
    progressLine = std::string(tr(STR_CHAPTER_PREFIX)) + std::to_string(currentPage) + "/" +
                   std::to_string(totalPages) + std::string(tr(STR_PAGES_SEPARATOR));
  }
  progressLine += std::string(tr(STR_BOOK_PREFIX)) + std::to_string(bookProgressPercent) + "%";
  fui::TextStyle infoStyle = screen.theme().smallText;
  fui::Rect infoRect{static_cast<int16_t>(hero.x + 12), static_cast<int16_t>(hero.y + 46),
                     static_cast<int16_t>(hero.width - 24), 16};
  screen.target().text(infoRect, progressLine.c_str(), infoStyle);

  screen.spacer(static_cast<int16_t>(metrics.verticalSpacing));

  // menuRowItems's labels/actionValue were set once in the constructor (see
  // buildMenuRowItems()); only rows with live values need refreshing here.
  for (size_t i = 0; i < menuItems.size(); i++) {
    const auto action = menuItems[i].action;
    if (action == MenuAction::ROTATE_SCREEN) {
      menuRowItems[i].value = I18N.get(orientationLabels[pendingOrientation]);
    } else if (action == MenuAction::AUTO_PAGE_TURN) {
      menuRowItems[i].value = pageTurnLabels[selectedPageTurnOption];
    } else if (action == MenuAction::NIGHT_MODE) {
      menuRowItems[i].toggleChecked = (SETTINGS.screenInverted != 0);
    } else if (action == MenuAction::FRONTLIGHT) {
      menuRowItems[i].toggleChecked = Frontlight.isOn();
    }
  }

  fui::ListProps props;
  props.items = menuRowItems;
  props.count = static_cast<uint16_t>(menuItems.size());
  props.action = ACTION_ROW;
  props.inputMask = fui::InputTouch;  // physical buttons stay in loop()
  props.rowHeight = 44;               // spacious card block
  props.rowGap = 6;                  // distinct gap between cards
  props.rowRadius = 6;               // rounded corners
  props.sidePadding = 14;
  props.valueInset = 10;              // air between the value and the row edge

  fui::TextStyle labelStyle = screen.theme().bodyText;
  labelStyle.bold = true;
  props.labelText = labelStyle;

  fui::TextStyle valStyle = screen.theme().smallText;
  valStyle.bold = true;
  props.valueText = valStyle;

  // Modern Card Block Styles with rounded borders
  props.rowStyles.explicitlySet = true;
  props.rowStyles.normal.border = fui::Paint::solid(fui::Color::Black);
  props.rowStyles.normal.borderWidth = 1;
  props.rowStyles.normal.radius = 6;
  props.rowStyles.normal.background = fui::Paint::solid(fui::Color::White);
  props.rowStyles.normal.foreground = fui::Paint::solid(fui::Color::Black);

  props.rowStyles.selected.border = fui::Paint::solid(fui::Color::Black);
  props.rowStyles.selected.borderWidth = 1;
  props.rowStyles.selected.radius = 6;
  props.rowStyles.selected.background = fui::Paint::solid(fui::Color::Black);
  props.rowStyles.selected.foreground = fui::Paint::solid(fui::Color::White);

  syncListViewport(screen, props);
  screen.list(props);
}

void EpubReaderMenuActivity::drawChrome() {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const Rect screen = UITheme::getInstance().getScreenSafeArea(renderer, true, false);

  // Header via GUI.drawHeader (already FreeInkUI-themed) for the battery
  // indicator; the rest of the screen renders through the app.
  GUI.drawHeader(renderer, Rect{screen.x, screen.y + metrics.topPadding, screen.width, metrics.headerHeight},
                 title.c_str());
}

void EpubReaderMenuActivity::render(RenderLock&&) {
  if (optionPopup.processRender(renderer, mappedInput)) return;

  renderer.clearScreen();
  drawChrome();

  renderUi();

  drawFooter();
  renderer.displayBuffer();
}
