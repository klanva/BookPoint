#include "BookSearchActivity.h"

#include <FsHelpers.h>
#include <GfxRenderer.h>
#include <HalStorage.h>
#include <I18n.h>
#include <Utf8.h>

#include <algorithm>
#include <cstring>
#include <memory>

#include "MappedInputManager.h"
#include "activities/ActivityManager.h"
#include "activities/util/KeyboardEntryActivity.h"
#include "components/UITheme.h"
#include "components/UiAppHelpers.h"
#include "fontIds.h"

namespace fui = freeink::ui;

namespace {
std::string toLowerUtf8(const std::string& str) {
  std::string result;
  result.reserve(str.size());
  const unsigned char* p = reinterpret_cast<const unsigned char*>(str.c_str());
  while (*p) {
    uint32_t cp = utf8NextCodepoint(&p);
    if (cp == REPLACEMENT_GLYPH) continue;
    if (cp >= 'A' && cp <= 'Z') {
      cp += ('a' - 'A');
    } else if (cp >= 0x0410 && cp <= 0x042F) {
      cp += 0x20;
    } else if (cp == 0x0401) {
      cp = 0x0451;
    }
    utf8AppendCodepoint(cp, result);
  }
  return result;
}
}  // namespace

BookSearchActivity::BookSearchActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : UiListActivity("BookSearch", renderer, mappedInput, /*wantsTouchLongPress=*/false) {}

void BookSearchActivity::onEnter() {
  UiListActivity::onEnter();
  if (!keyboardLaunched) {
    promptKeyboard();
  }
}

void BookSearchActivity::onExit() {
  Activity::onExit();
  rowItems.clear();
  results.clear();
}

const char* BookSearchActivity::headerTitle() const {
  if (currentQuery.empty()) {
    return tr(STR_SEARCH_BOOKS);
  }
  headerTitleBuffer = tr(STR_SEARCH) + std::string(": ") + currentQuery;
  return headerTitleBuffer.c_str();
}

void BookSearchActivity::promptKeyboard() {
  keyboardLaunched = true;
  auto keyboard =
      std::make_unique<KeyboardEntryActivity>(renderer, mappedInput, tr(STR_SEARCH_BOOKS), currentQuery);
  startActivityForResult(std::move(keyboard), [this](const ActivityResult& result) {
    if (!result.isCancelled) {
      currentQuery = std::get<KeyboardResult>(result.data).text;
      performSearch(currentQuery);
    } else {
      if (!searchDone || results.empty()) {
        finish();
      } else {
        requestUpdate();
      }
    }
  });
}

void BookSearchActivity::scanDirectory(const std::string& dirPath, const std::string& queryLower, int depth) {
  if (depth > 4 || results.size() >= 100) return;

  auto dir = Storage.open(dirPath.c_str());
  if (!dir || !dir.isDirectory()) return;

  dir.rewindDirectory();
  char nameBuf[256];

  for (auto file = dir.openNextFile(); file; file = dir.openNextFile()) {
    if (results.size() >= 100) {
      file.close();
      break;
    }
    file.getName(nameBuf, sizeof(nameBuf));
    if (nameBuf[0] == '.' || strcmp(nameBuf, "System Volume Information") == 0) {
      file.close();
      continue;
    }

    if (file.isDirectory()) {
      std::string subDir = dirPath;
      if (subDir.empty() || subDir.back() != '/') subDir += '/';
      subDir += nameBuf;
      file.close();
      scanDirectory(subDir, queryLower, depth + 1);
    } else {
      std::string filename(nameBuf);
      std::string_view sv(filename);
      bool isBook = FsHelpers::hasEpubExtension(sv) || FsHelpers::hasXtcExtension(sv) ||
                    FsHelpers::hasTxtExtension(sv) || FsHelpers::hasMarkdownExtension(sv) ||
                    FsHelpers::checkFileExtension(sv, ".fb2");
      if (isBook) {
        std::string filenameLower = toLowerUtf8(filename);
        if (queryLower.empty() || filenameLower.find(queryLower) != std::string::npos) {
          BookSearchResult res;
          std::string fullPath = dirPath;
          if (fullPath.empty() || fullPath.back() != '/') fullPath += '/';
          fullPath += filename;
          res.path = std::move(fullPath);
          res.filename = filename;
          const auto dot = filename.find_last_of('.');
          res.title = (dot == std::string::npos) ? filename : filename.substr(0, dot);
          results.push_back(std::move(res));
        }
      }
      file.close();
    }
  }
  dir.close();
}

void BookSearchActivity::performSearch(const std::string& query) {
  results.clear();
  rowItems.clear();
  searchDone = true;

  const std::string queryLower = toLowerUtf8(query);
  scanDirectory("/", queryLower, 0);

  std::sort(results.begin(), results.end(), [](const BookSearchResult& a, const BookSearchResult& b) {
    return FsHelpers::naturalLess(a.title, b.title);
  });

  rebuildRowItems();
  nav.selected = 0;
  nav.follow(listCount());
  requestUpdate(true);
}

void BookSearchActivity::rebuildRowItems() {
  rowItems.clear();
  rowItems.reserve(results.size());
  for (size_t i = 0; i < results.size(); ++i) {
    fui::ListItem item;
    item.label = results[i].title.c_str();
    item.subtitle = results[i].path.c_str();
    item.icon = listIconFor(UITheme::getFileIcon(results[i].path), 32);
    item.actionValue = static_cast<int16_t>(i);
    rowItems.push_back(item);
  }
}

void BookSearchActivity::activateIndex(const int index) {
  if (index < 0 || index >= listCount()) return;
  app.clearTapFlash();
  activityManager.goToReader(results[index].path);
}

bool BookSearchActivity::handleCustomInput() {
  // Tap in header triggers re-search
  int tx = 0, ty = 0;
  if (mappedInput.wasScreenTapped(tx, ty)) {
    const auto& metrics = UITheme::getInstance().getMetrics();
    if (ty < metrics.topPadding + metrics.headerHeight) {
      promptKeyboard();
      return true;
    }
  }
  return false;
}

bool BookSearchActivity::handleButtons() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    finish();
    return true;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    if (results.empty()) {
      promptKeyboard();
      return true;
    }
    if (nav.selected < listCount()) {
      activateIndex(nav.selected);
      return true;
    }
  }
  return false;
}

void BookSearchActivity::buildScreen(UiScreen& screen) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  screen.setContentMargin(fui::Insets{static_cast<int16_t>(metrics.topPadding + metrics.headerHeight), 0,
                                      static_cast<int16_t>(metrics.buttonHintsHeight), 0});
  screen.spacer(static_cast<int16_t>(metrics.verticalSpacing));

  if (!searchDone) {
    screen.centeredText(tr(STR_LOADING), screen.theme().bodyText);
    return;
  }

  if (results.empty()) {
    screen.centeredText(tr(STR_NO_FILES_FOUND), screen.theme().bodyText);
    return;
  }

  fui::ListProps props;
  props.items = rowItems.data();
  props.count = static_cast<uint16_t>(rowItems.size());
  props.action = ACTION_ROW;
  props.inputMask = fui::InputTouch;
  fui::TextStyle label = screen.theme().smallText;
  label.bold = true;
  props.labelText = label;
  syncListViewport(screen, props, /*hasSubtitle=*/true);
  screen.list(props);
}

void BookSearchActivity::drawFooter() {
  const bool empty = results.empty();
  const auto labels = mappedInput.mapLabels(tr(STR_BACK), empty ? tr(STR_SEARCH) : tr(STR_OPEN),
                                            empty ? "" : tr(STR_DIR_UP), empty ? "" : tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
}
