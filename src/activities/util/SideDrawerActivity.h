#pragma once

#include "CrossPointSettings.h"
#include "CrossPointState.h"
#include "activities/Activity.h"
#include "activities/ActivityManager.h"
#include "util/ButtonNavigator.h"

class SideDrawerActivity final : public Activity {
  ButtonNavigator buttonNavigator;
  int selectedIndex = 0;
  static constexpr int ITEM_COUNT = 6;

  void activateItem(int index);
  void close();

 public:
  explicit SideDrawerActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);
  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;
};
