#include "SideDrawerActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>

#include "MappedInputManager.h"
#include "RecentBooksStore.h"
#include "activities/home/FileBrowserActivity.h"
#include "activities/reader/EpubReaderActivity.h"
#include "activities/settings/SettingsActivity.h"
#include "components/UITheme.h"
#include "components/icons/listIcons.h"
#include "fontIds.h"

namespace {
struct DrawerItem {
  const char* label;
  const freeink::Icon* icon;
};

const DrawerItem ITEMS[6] = {
    {"Библиотека", &icon_library_24},
    {"Сейчас читаю", &icon_book_24},
    {"Оглавление", &icon_file_text_24},
    {"Закладки и цитаты", &icon_bookmark_24},
    {"Словарь", &icon_book_24},
    {"Настройки", &icon_wifi_24},  // will render Settings contour
};
}  // namespace

SideDrawerActivity::SideDrawerActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : Activity("SideDrawer", renderer, mappedInput) {}

void SideDrawerActivity::onEnter() {
  Activity::onEnter();
  requestUpdate();
}

void SideDrawerActivity::close() {
  finish();
}

void SideDrawerActivity::activateItem(const int index) {
  close();
  switch (index) {
    case 0:  // Library
      activityManager.pushActivity(std::make_unique<FileBrowserActivity>(renderer, mappedInput));
      break;
    case 1: {  // Now Reading
      const auto books = RECENT_BOOKS.getBooks();
      if (!books.empty()) {
        activityManager.pushActivity(
            std::make_unique<EpubReaderActivity>(renderer, mappedInput, books.front().path, false));
      } else {
        activityManager.pushActivity(std::make_unique<FileBrowserActivity>(renderer, mappedInput));
      }
      break;
    }
    case 2:  // TOC
      break;
    case 3:  // Bookmarks
      break;
    case 4:  // Dictionary
      break;
    case 5:  // Settings
      activityManager.pushActivity(std::make_unique<SettingsActivity>(renderer, mappedInput));
      break;
    default:
      break;
  }
}

void SideDrawerActivity::loop() {
  const bool isLeftHanded = (SETTINGS.handedness == CrossPointSettings::HANDEDNESS_LEFT);
  const int drawerWidth = 320;
  const int drawerX = isLeftHanded ? 0 : (480 - drawerWidth);

  int tx = 0;
  int ty = 0;
  if (mappedInput.wasScreenTapped(tx, ty)) {
    // Check backdrop tap
    if (isLeftHanded && tx >= drawerWidth) {
      close();
      return;
    }
    if (!isLeftHanded && tx < drawerX) {
      close();
      return;
    }

    // Check item taps (indices 0..5)
    for (int i = 0; i < ITEM_COUNT; i++) {
      const int itemY = 100 + i * 70;
      const int itemX = drawerX + 16;
      const int itemW = drawerWidth - 32;
      const int itemH = 56;
      if (tx >= itemX && tx <= itemX + itemW && ty >= itemY && ty <= itemY + itemH) {
        selectedIndex = i;
        activateItem(i);
        return;
      }
    }
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Back) || mappedInput.wasBackGesture()) {
    close();
    return;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    activateItem(selectedIndex);
    return;
  }

  buttonNavigator.onPressAndContinuous({MappedInputManager::Button::Up}, [this] {
    selectedIndex = (selectedIndex + ITEM_COUNT - 1) % ITEM_COUNT;
    requestUpdate();
  });
  buttonNavigator.onPressAndContinuous({MappedInputManager::Button::Down}, [this] {
    selectedIndex = (selectedIndex + 1) % ITEM_COUNT;
    requestUpdate();
  });
}

void SideDrawerActivity::render(RenderLock&&) {
  const bool isLeftHanded = (SETTINGS.handedness == CrossPointSettings::HANDEDNESS_LEFT);
  const int drawerWidth = 320;
  const int drawerX = isLeftHanded ? 0 : (480 - drawerWidth);

  // Fill backdrop with light stipple or white
  renderer.fillRect(drawerX, 0, drawerWidth, 800, false);
  if (isLeftHanded) {
    renderer.drawLine(drawerWidth, 0, drawerWidth, 800);
  } else {
    renderer.drawLine(drawerX, 0, drawerX, 800);
  }

  // Header
  renderer.drawText(UI_12_FONT_ID, drawerX + 24, 45, "BookPoint", true);

  // Items
  for (int i = 0; i < ITEM_COUNT; i++) {
    const int itemY = 100 + i * 70;
    const int itemX = drawerX + 16;
    const int itemW = drawerWidth - 32;
    const int itemH = 56;
    const bool isSelected = (i == selectedIndex);

    if (isSelected) {
      renderer.fillRect(itemX, itemY, itemW, itemH, true);
      renderer.drawText(UI_12_FONT_ID, itemX + 24, itemY + 20, ITEMS[i].label, false);
    } else {
      renderer.drawRect(itemX, itemY, itemW, itemH, true);
      renderer.drawText(UI_12_FONT_ID, itemX + 24, itemY + 20, ITEMS[i].label, true);
    }
  }

  renderer.displayBuffer();
}
