#pragma once

#include <vector>

#include "activities/Activity.h"

// Launcher for the built-in games: 2048, Minesweeper, Chess. A simple list;
// Confirm opens the selected game, Back returns to Settings.
class GamesActivity final : public Activity {
  int selected_ = 0;

  static constexpr int GAME_COUNT = 3;
  static const char* gameName(int index);

 public:
  explicit GamesActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("Games", renderer, mappedInput) {}

  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;
};
