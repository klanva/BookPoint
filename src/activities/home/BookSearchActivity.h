#pragma once

#include <I18n.h>

#include <string>
#include <vector>

#include "activities/UiListActivity.h"

struct BookSearchResult {
  std::string path;
  std::string filename;
  std::string title;
};

class BookSearchActivity final : public UiListActivity {
 public:
  explicit BookSearchActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);
  void onEnter() override;
  void onExit() override;

 private:
  int listCount() const override { return static_cast<int>(results.size()); }
  void buildScreen(UiScreen& screen) override;
  void activateIndex(int index) override;
  bool handleButtons() override;
  bool handleCustomInput() override;
  const char* headerTitle() const override;
  void drawFooter() override;

  void promptKeyboard();
  void performSearch(const std::string& query);
  void scanDirectory(const std::string& dirPath, const std::string& queryLower, int depth);

  std::string currentQuery;
  bool keyboardLaunched = false;
  bool searchDone = false;
  mutable std::string headerTitleBuffer;
  std::vector<BookSearchResult> results;
  std::vector<freeink::ui::ListItem> rowItems;
  void rebuildRowItems();
};
