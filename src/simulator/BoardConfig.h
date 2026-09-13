#pragma once

#include <cstdint>

namespace BoardConfig {

enum class Board {
  XteinkX3,
  XteinkX3Uc8279,
  XteinkX4,
  XteinkX4Pro,
  Sticky,
  WsEpaper397
};

enum class DisplayController {
  SSD1677,
  UC8253,
  UC8279,
  UC8179,
  ED2208,
  LgfxEpd,
  IT8951
};

struct ViewableInsets {
  int top = 0;
  int right = 0;
  int bottom = 0;
  int left = 0;
};
using Insets = ViewableInsets;

struct ActiveProfile {
  const char* name = "BookPoint X4 Pro Simulator";
  uint16_t displayWidth = 800;
  uint16_t displayHeight = 480;
  uint32_t displaySpiHz = 10000000;
  Board board = Board::XteinkX4Pro;
  DisplayController displayController = DisplayController::SSD1677;
  ViewableInsets viewableInsets{0, 0, 0, 0};
};

inline ActiveProfile ACTIVE;

constexpr uint32_t MAX_FRAMEBUFFER_BYTES = 48000;

inline bool isX4Pro() { return true; }
inline bool hasTouch() { return true; }
inline bool hasHomeKey() { return true; }
inline bool isX4Classic() { return false; }
inline void selectDevice(Board) {}
inline void holdPowerRails() {}

}  // namespace BoardConfig
