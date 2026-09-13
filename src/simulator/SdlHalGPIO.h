#pragma once

#include <SDL2/SDL.h>
#include <cstdint>
#include <algorithm>
#include <cmath>

class HalGPIO {
 public:
  static constexpr uint8_t BTN_BACK    = 0;
  static constexpr uint8_t BTN_CONFIRM = 1;
  static constexpr uint8_t BTN_LEFT    = 2;
  static constexpr uint8_t BTN_RIGHT   = 3;
  static constexpr uint8_t BTN_UP      = 4;
  static constexpr uint8_t BTN_DOWN    = 5;
  static constexpr uint8_t BTN_POWER   = 6;

  enum class WakeupReason { PowerButton, AfterFlash, AfterUSBPower, Other };

  HalGPIO();
  void begin() {}

  // Buttons
  bool isPressed(uint8_t btn) const { return (currentState & (1 << btn)) != 0; }
  bool wasPressed(uint8_t btn) const { return (pressedEvents & (1 << btn)) != 0; }
  bool wasReleased(uint8_t btn) const { return (releasedEvents & (1 << btn)) != 0; }
  bool wasAnyPressed() const { return pressedEvents != 0; }
  bool wasAnyReleased() const { return releasedEvents != 0; }
  bool isDebouncePending() const { return false; }
  unsigned long getHeldTime() const { return heldDurationMs; }
  unsigned long getPowerButtonHeldTime() const { return isPressed(BTN_POWER) ? heldDurationMs : 0; }

  // Capabilities
  bool hasTouch() const { return true; }
  bool hasHomeKey() const { return true; }
  bool wasHomeKeyTapped() const { return homeKeyTapEvent; }
  bool wasHomeKeyLongPressed() const { return homeKeyLongEvent; }

  // Touch Coordinates
  bool wasTouchTap(float& nx, float& ny) const;
  bool wasTouchDown(float& nx, float& ny) const;
  bool wasTouchReleased() const { return touchReleasedEvent; }
  bool isTouchTapCandidate(float& nx, float& ny, unsigned long& heldMs) const;
  bool isTouchHeldAt(float& nx, float& ny) const;
  bool wasTouchLongPress(float& nx, float& ny) const;
  void suppressTouchContact() { touchSuppressed = true; }
  unsigned long lastTouchHeldMs() const { return lastTouchDurationMs; }
  bool wasSwipe(float& nxStart, float& nyStart, float& nxEnd, float& nyEnd) const;
  bool wasTouchActivity() const;

  // Board compatibility
  void setSharedConfirmPowerShortPressEmitsPower(bool) {}
  bool verifyPowerButtonWakeup(uint16_t, bool) { return true; }
  bool isUsbConnected() const { return true; }
  bool wasUsbStateChanged() const { return false; }
  WakeupReason getWakeupReason() const { return WakeupReason::PowerButton; }
  bool hasEdgeSideButtons() const { return true; }
  bool deviceIsX3() const { return false; }
  bool deviceIsX4() const { return true; }
  bool isXteinkDevice() const { return true; }
  bool coldBootImpliesPowerButton() const { return true; }

  // Event Processing
  void handleSdlEvent(const SDL_Event& e);
  void update();

  // Programmatic scripting helpers
  void injectTap(int x, int y);
  void injectSwipe(int x1, int y1, int x2, int y2, int durationMs = 200);
  void injectButton(uint8_t btn, bool press);

 private:
  uint8_t currentState = 0;
  uint8_t pressedEvents = 0;
  uint8_t releasedEvents = 0;
  uint8_t rawPressed = 0;
  uint8_t rawReleased = 0;
  unsigned long heldDurationMs = 0;

  // Mouse / Touch tracking
  bool isMouseDown = false;
  int touchDownX = 0, touchDownY = 0;
  int currentMouseX = 0, currentMouseY = 0;
  int touchUpX = 0, touchUpY = 0;
  uint32_t mouseDownTime = 0;
  unsigned long lastTouchDurationMs = 0;
  bool touchMovedBeyondSlop = false;
  bool touchLongPressFired = false;
  bool touchSuppressed = false;

  bool touchDownEvent = false, rawTouchDown = false;
  bool touchTapEvent = false, rawTouchTap = false;
  bool touchSwipeEvent = false, rawTouchSwipe = false;
  bool touchReleasedEvent = false, rawTouchReleased = false;
  bool touchLongPressEvent = false, rawTouchLongPress = false;

  // Home key tracking
  bool isHomeKeyDown = false;
  uint32_t homeKeyDownTime = 0;
  bool homeKeyLongFired = false;
  bool homeKeyTapEvent = false, rawHomeKeyTap = false;
  bool homeKeyLongEvent = false, rawHomeKeyLong = false;

  static void mapLogicalToNormalized(int lx, int ly, float& nx, float& ny);
  void onMouseDown(int x, int y);
  void onMouseMove(int x, int y);
  void onMouseUp(int x, int y);
  void handleKeyDown(SDL_Keycode sym);
  void handleKeyUp(SDL_Keycode sym);
};

extern HalGPIO gpio;
using SdlHalGPIO = HalGPIO;
