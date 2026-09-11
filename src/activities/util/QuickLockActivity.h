#pragma once

#include "activities/Activity.h"

class QuickLockActivity final : public Activity {
  bool wasLightOn = false;
  unsigned long lockedAt = 0;
  unsigned long lastClickAt = 0;
  bool unlockHintVisible = false;
  unsigned long hintShownAt = 0;

  void unlock();

 public:
  explicit QuickLockActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);
  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;
  bool handleHomeGesture() override;
  bool handleForcedRefresh() override { return false; }
};
