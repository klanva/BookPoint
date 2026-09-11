#pragma once

#include "activities/Activity.h"

// Battery autonomy screen: charge level, time on the current battery cycle,
// pages shown since the last charge, days since the cycle began. Zero wakeups
// of its own - all counters live in PowerStats and are accrued lazily.
class PowerStatsActivity final : public Activity {
 public:
  explicit PowerStatsActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("PowerStats", renderer, mappedInput) {}

  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;
};
