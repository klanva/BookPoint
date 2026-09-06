#pragma once
#include <string>

#include "activities/UiListActivity.h"
#include "components/OptionPopup.h"

// Reader status bar configuration activity
class StatusBarSettingsActivity final : public UiListActivity {
 public:
  explicit StatusBarSettingsActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);

  // Capacity must hold all status bar rows and section headers.
  static constexpr int MAX_STATUS_BAR_ITEMS = 32;

  void onEnter() override;
  void render(RenderLock&&) override;

 private:
  OptionPopup optionPopup;

  int visibleItemCount = 0;

  int listCount() const override { return std::min(visibleItemCount, MAX_STATUS_BAR_ITEMS); }
  bool isSelectable(int index) const override {
    if (index < 0 || index >= visibleItemCount || index >= MAX_STATUS_BAR_ITEMS) return false;
    return !rowItems_[index].isHeader;
  }
  void buildScreen(UiScreen& screen) override;
  void activateIndex(int index) override;
  bool handleCustomInput() override;
  const char* headerTitle() const override;

  std::string rowValueText(int index);
  void handleSelection();
  void updateVisibleItems();

  std::string rowValues_[MAX_STATUS_BAR_ITEMS];
  freeink::ui::ListItem rowItems_[MAX_STATUS_BAR_ITEMS]{};
};
