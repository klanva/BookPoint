#pragma once

#include "activities/Activity.h"

// Classic 2048 on e-ink. 4x4 grid, d-pad moves, Confirm restarts after a game
// over. Rendering is text-based (2 px-styled cells drawn with rect + number),
// so a move costs one partial refresh.
class Games2048Activity final : public Activity {
  uint16_t board[4][4] = {};
  uint32_t score = 0;
  uint32_t best = 0;
  bool gameOver = false;
  bool wonShown = false;
  bool moved = false;  // set by move handlers when any tile moved

  bool slideLeft();
  void rotateRight();
  bool moveUp();
  bool spawnTile();
  bool canMove() const;
  void drawCell(int row, int col, int x, int y, int size) const;

 public:
  explicit Games2048Activity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("2048", renderer, mappedInput) {}

  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;
};
