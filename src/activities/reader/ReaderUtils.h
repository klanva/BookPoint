#pragma once

#include <CrossPointSettings.h>
#include <GfxRenderer.h>
#include <HalGPIO.h>
#include <HalTiltSensor.h>
#include <Logging.h>
#include <components/bars/tap-zones.h>

#include "MappedInputManager.h"
#include "activities/ActivityManager.h"

namespace ReaderUtils {

constexpr unsigned long GO_HOME_MS = 1000;
constexpr unsigned long GO_BACK_OR_HOME_MS = GO_HOME_MS;
constexpr unsigned long SKIP_HOLD_MS = 700;
constexpr unsigned long BOOKMARK_HOLD_MS = 400;
constexpr unsigned long BOOKMARK_MESSAGE_DURATION_MS = 2500;

enum ReaderTouchAction : freeink::ui::ActionId {
  READER_TOUCH_PREV = 1,
  READER_TOUCH_NEXT = 3,
};

inline void applyOrientation(GfxRenderer& renderer, const uint8_t orientation) {
  switch (orientation) {
    case CrossPointSettings::ORIENTATION::PORTRAIT:
      renderer.setOrientation(GfxRenderer::Orientation::Portrait);
      break;
    case CrossPointSettings::ORIENTATION::LANDSCAPE_CW:
      renderer.setOrientation(GfxRenderer::Orientation::LandscapeClockwise);
      break;
    case CrossPointSettings::ORIENTATION::INVERTED:
      renderer.setOrientation(GfxRenderer::Orientation::PortraitInverted);
      break;
    case CrossPointSettings::ORIENTATION::LANDSCAPE_CCW:
      renderer.setOrientation(GfxRenderer::Orientation::LandscapeCounterClockwise);
      break;
    default:
      break;
  }
}

struct PageTurnResult {
  bool prev;
  bool next;
  bool fromTilt;
};

inline PageTurnResult detectPageTurn(const MappedInputManager& input) {
  const bool usePress = SETTINGS.longPressButtonBehavior == SETTINGS.OFF;
  const bool tiltNext = SETTINGS.tiltPageTurn && halTiltSensor.wasTiltedForward();
  const bool tiltPrev = SETTINGS.tiltPageTurn && halTiltSensor.wasTiltedBack();
  const bool swapFront = input.isNavDirectionSwapped();
  const auto prevButton = swapFront ? MappedInputManager::Button::Right : MappedInputManager::Button::Left;
  const auto nextButton = swapFront ? MappedInputManager::Button::Left : MappedInputManager::Button::Right;
  const bool prev =
      tiltPrev ||
      (usePress ? (input.wasPressed(MappedInputManager::Button::PageBack) || input.wasPressed(prevButton))
                : (input.wasReleased(MappedInputManager::Button::PageBack) || input.wasReleased(prevButton)));
  const bool powerTurn = SETTINGS.shortPwrBtn == CrossPointSettings::SHORT_PWRBTN::PAGE_TURN &&
                         input.wasReleased(MappedInputManager::Button::Power);
  const bool next = tiltNext || (usePress ? (input.wasPressed(MappedInputManager::Button::PageForward) || powerTurn ||
                                             input.wasPressed(nextButton))
                                          : (input.wasReleased(MappedInputManager::Button::PageForward) || powerTurn ||
                                             input.wasReleased(nextButton)));
  return {prev, next, tiltPrev || tiltNext};
}

struct TouchPageTurn {
  bool prev;
  bool next;
  unsigned long heldMs;
};

inline TouchPageTurn detectTouchPageTurn(GfxRenderer& renderer, const MappedInputManager& input) {
  TouchPageTurn result{false, false, 0};
  if (!SETTINGS.touchReaderControls || !input.hasTouch()) {
    return result;
  }

  if (SETTINGS.touchReaderControls == CrossPointSettings::TOUCH_READER_SWIPE) {
    // Horizontal swipes turn pages; taps remain free for the centered reader-menu
    // zone. A slow swipe never becomes a long-press chapter skip.
    const auto dir = input.wasSwipe();
    if (dir == MappedInputManager::SwipeDir::Left) {
      result.next = true;
    } else if (dir == MappedInputManager::SwipeDir::Right) {
      result.prev = true;
    }
    return result;
  }

  // Also accept swipe gestures in tap modes
  const auto swipeDir = input.wasSwipe();
  if (swipeDir == MappedInputManager::SwipeDir::Left) {
    result.next = true;
    return result;
  } else if (swipeDir == MappedInputManager::SwipeDir::Right) {
    result.prev = true;
    return result;
  }

  int x = 0;
  int y = 0;
  if (!input.wasScreenTapped(x, y)) {
    return result;
  }

  const int width = renderer.getScreenWidth();
  const int height = renderer.getScreenHeight();

  // Configurable center menu zone (30%..40% width, default 35%)
  const int menuPct = std::clamp(static_cast<int>(SETTINGS.touchMenuZoneWidthPercent), 30, 40);
  const int sidePct = (100 - menuPct) / 2;
  const int menu_x_start = width * sidePct / 100;
  const int menu_x_end = width * (sidePct + menuPct) / 100;
  const int menu_y_start = height * 35 / 100;
  const int menu_y_end = height * 65 / 100;

  // Center menu exclusion zone: tapping here opens the menu/overlay, not page turns
  if (SETTINGS.tapForReaderMenu && (x >= menu_x_start && x < menu_x_end) && (y >= menu_y_start && y < menu_y_end)) {
    return result;
  }

  const bool isLeftHanded = (SETTINGS.handedness == CrossPointSettings::HANDEDNESS_LEFT);
  const bool inverted = (SETTINGS.touchReaderControls == CrossPointSettings::TOUCH_READER_INVERTED_TAP);
  bool forward = false;

  if (SETTINGS.readerTouchZoneLayout == CrossPointSettings::TOUCH_LAYOUT_3_ZONE) {
    // 3-Zone Layout: Left column (0..menu_x_start), Center column (menu_x_start..menu_x_end), Right column (menu_x_end..width)
    if (x >= menu_x_start && x < menu_x_end) {
      if (SETTINGS.tapForReaderMenu) return result;
    }
    if (!isLeftHanded) {
      forward = (x >= menu_x_end);
    } else {
      forward = (x < menu_x_start);
    }
  } else if (SETTINGS.readerTouchZoneLayout == CrossPointSettings::TOUCH_LAYOUT_9_ZONE) {
    // 9-Zone Layout (3x3 Grid):
    // Columns: Left (< menu_x_start), Center (menu_x_start..menu_x_end), Right (>= menu_x_end)
    // Rows: Top (< menu_y_start), Center (menu_y_start..menu_y_end), Bottom (>= menu_y_end)
    const bool inCenterCol = (x >= menu_x_start && x < menu_x_end);
    if (inCenterCol && SETTINGS.tapForReaderMenu) {
      return result;
    }
    if (!isLeftHanded) {
      forward = (x >= menu_x_end);
    } else {
      forward = (x < menu_x_start);
    }
  } else {
    // TOUCH_LAYOUT_2_ZONE_HANDED (75/25 handed touch zones - legacy compatibility)
    if (!isLeftHanded) {
      // Right-handed: 0..25% is PREV, 25..100% is NEXT
      forward = (x >= width / 4);
    } else {
      // Left-handed: 0..75% is NEXT, 75..100% is PREV
      forward = (x < width * 3 / 4);
    }
  }

  if (inverted) {
    forward = !forward;
  }

  result.next = forward;
  result.prev = !forward;
  result.heldMs = gpio.lastTouchHeldMs();
  return result;
}

// Tap in the center menu zone of the screen (configurable 30..40% width, middle 30% height):
// invokes reader overlay / menu.
inline bool isTouchMenuTap(const GfxRenderer& renderer, const MappedInputManager& input) {
  if (!input.hasTouch()) return false;
  if (!SETTINGS.tapForReaderMenu) return false;
  int x = 0;
  int y = 0;
  if (!input.wasScreenTapped(x, y)) return false;
  const int width = renderer.getScreenWidth();
  const int height = renderer.getScreenHeight();
  const int menuPct = std::clamp(static_cast<int>(SETTINGS.touchMenuZoneWidthPercent), 30, 40);
  const int sidePct = (100 - menuPct) / 2;
  const int menu_x_start = width * sidePct / 100;
  const int menu_x_end = width * (sidePct + menuPct) / 100;
  const int menu_y_start = height * 35 / 100;
  const int menu_y_end = height * 65 / 100;
  return (x >= menu_x_start && x < menu_x_end) && (y >= menu_y_start && y < menu_y_end);
}

// Reader menu opens on the menu edge-swipe or a center-third tap. On home-key
// boards a long press of the capacitive key runs the user-selected long-press
// function instead (SETTINGS.longPressMenuFunction), not the menu.
// With touch reader controls Off the reading surface ignores touch entirely,
// menu included, so a stray brush of the screen can't open it; the menu stays
// reachable via the Confirm button.
inline bool isTouchMenuGesture(const GfxRenderer& renderer, const MappedInputManager& input) {
  if (!SETTINGS.touchReaderControls) return false;
  return (input.hasTouch() && input.wasMenuGesture()) || isTouchMenuTap(renderer, input);
}

// One helper, blocking or deferred: the async form starts the refresh and
// returns so the caller can overlap CPU work with the panel's refresh time.
// Async callers must not touch the framebuffer until
// renderer.waitRefreshComplete() and must rebuild the differential baseline
// before the next page turn (the tiled grayscale cleanup does).
inline void displayWithRefreshCycle(const GfxRenderer& renderer, int& pagesUntilFullRefresh, bool async = false) {
  const auto mode = (pagesUntilFullRefresh <= 1) ? HalDisplay::HALF_REFRESH : HalDisplay::FAST_REFRESH;
  if (async) {
    renderer.displayBufferAsync(mode);
  } else {
    renderer.displayBuffer(mode);
  }
  if (pagesUntilFullRefresh <= 1) {
    pagesUntilFullRefresh = SETTINGS.getRefreshFrequency();
  } else {
    pagesUntilFullRefresh--;
  }
}

// Display the B/W base of a page whose grayscale pass follows. Panels that
// combine the base (Paper Mono) defer the activation so base + gray planes go
// out as one waveform — displaying the base separately makes the gray pass
// re-drive the whole text body (a visible flash). Other panels display
// normally. Same refresh-cadence bookkeeping as displayWithRefreshCycle.
inline void displayBaseWithRefreshCycle(const GfxRenderer& renderer, int& pagesUntilFullRefresh) {
  if (!renderer.combinesGrayscaleBase()) {
    displayWithRefreshCycle(renderer, pagesUntilFullRefresh);
    return;
  }
  const auto mode = (pagesUntilFullRefresh <= 1) ? HalDisplay::HALF_REFRESH : HalDisplay::FAST_REFRESH;
  renderer.displayGrayscaleBase(mode);
  if (pagesUntilFullRefresh <= 1) {
    pagesUntilFullRefresh = SETTINGS.getRefreshFrequency();
  } else {
    pagesUntilFullRefresh--;
  }
}

// Grayscale anti-aliasing pass. Renders content twice (LSB + MSB) to build
// the grayscale buffer. Only the content callback is re-rendered — status bars
// and other overlays should be drawn before calling this.
// Kept as a template to avoid std::function overhead; instantiated once per reader type.
template <typename RenderFn>
void renderAntiAliased(GfxRenderer& renderer, RenderFn&& renderFn) {
  if (!renderer.storeBwBuffer()) {
    LOG_ERR("READER", "Failed to store BW buffer for anti-aliasing");
    // A combined-base panel may still hold a deferred B/W activation; flush it
    // so the page reaches the panel even without its grays.
    if (renderer.combinesGrayscaleBase()) renderer.cleanupGrayscaleWithFrameBuffer();
    return;
  }

  renderer.clearScreen(0x00);
  renderer.setRenderMode(GfxRenderer::GRAYSCALE_LSB);
  renderFn();
  renderer.copyGrayscaleLsbBuffers();

  renderer.clearScreen(0x00);
  renderer.setRenderMode(GfxRenderer::GRAYSCALE_MSB);
  renderFn();
  renderer.copyGrayscaleMsbBuffers();

  renderer.displayGrayBuffer();
  renderer.setRenderMode(GfxRenderer::BW);

  renderer.restoreBwBuffer();
}

struct BackNavCallback {
  void* ctx;
  void (*fn)(void*);
};

// Returns true if the back button was consumed (caller should return).
// Long press (>= GO_BACK_OR_HOME_MS):
// - default: go to file browser
// - with backShortToFileBrowser: go home
// Short press (< GO_BACK_OR_HOME_MS):
// - default: go home
// - with backShortToFileBrowser: go to file browser.
inline bool handleBackNavigation(const MappedInputManager& mappedInput, ActivityManager& activityManager,
                                 const char* filePath, BackNavCallback goHome) {
  // The reading surface deliberately has no left-edge swipe-to-exit path: in
  // swipe page-turn mode a right swipe must page back instead. Home remains
  // available through the board's dedicated Home gesture/key. Back swipes stay
  // available in menus and other activities; only this reader-surface handler
  // ignores them. Physical Back buttons are unaffected: isPressed() is
  // button-only, and this guard skips just the gesture's own release frame.
  if (mappedInput.wasBackGesture()) {
    return false;
  }

  if (!mappedInput.wasReleased(MappedInputManager::Button::Back)) return false;

  const bool longPress = mappedInput.getHeldTime() >= GO_BACK_OR_HOME_MS;
  if (longPress != SETTINGS.backShortToFileBrowser) {
    activityManager.goToFileBrowser(filePath);
  } else {
    goHome.fn(goHome.ctx);
  }
  return true;
}

}  // namespace ReaderUtils
