#pragma once
#include <string>

#include "activities/UiListActivity.h"
#include "components/OptionPopup.h"

// Reader status bar configuration activity
class StatusBarSettingsActivity final : public UiListActivity {
 public:
  explicit StatusBarSettingsActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);

  // Must equal ITEM_COUNT in the .cpp (static_assert'd there) — the max
  // possible row count (RTC-equipped devices show all of them).
  static constexpr int MAX_STATUS_BAR_ITEMS = 14;

  void onEnter() override;
  void render(RenderLock&&) override;

 private:
  OptionPopup optionPopup;

  enum class Folder : uint8_t {
    None,
    Display,
    Elements,
    Clock
  };
  Folder activeFolder = Folder::None;
  int lastFolderIndex_ = 0;

  int visibleItemCount = 0;

  int listCount() const override { return visibleItemCount; }
  void buildScreen(UiScreen& screen) override;
  void activateIndex(int index) override;
  bool handleCustomInput() override;
  bool handleButtons() override;
  void drawChrome() override;

  std::string rowValueText(int index);
  void handleSelection();
  void updateVisibleItems();

  std::string rowValues_[MAX_STATUS_BAR_ITEMS];
  freeink::ui::ListItem rowItems_[MAX_STATUS_BAR_ITEMS]{};
};
