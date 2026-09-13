#include "SdlHalGPIO.h"

HalGPIO gpio;

HalGPIO::HalGPIO() = default;

void HalGPIO::mapLogicalToNormalized(int lx, int ly, float& nx, float& ny) {
  lx = std::clamp(lx, 0, 479);
  ly = std::clamp(ly, 0, 799);
  // Physical panel: width=800, height=480
  // Portrait rotation: phyX = ly, phyY = 479 - lx
  nx = static_cast<float>(ly) / 800.0f;
  ny = static_cast<float>(479 - lx) / 480.0f;
}

bool HalGPIO::wasTouchTap(float& nx, float& ny) const {
  if (!touchTapEvent) return false;
  mapLogicalToNormalized(touchDownX, touchDownY, nx, ny);
  return true;
}

bool HalGPIO::wasTouchDown(float& nx, float& ny) const {
  if (!touchDownEvent) return false;
  mapLogicalToNormalized(touchDownX, touchDownY, nx, ny);
  return true;
}

bool HalGPIO::isTouchTapCandidate(float& nx, float& ny, unsigned long& heldMs) const {
  if (!isMouseDown || touchMovedBeyondSlop || touchSuppressed) return false;
  mapLogicalToNormalized(touchDownX, touchDownY, nx, ny);
  heldMs = SDL_GetTicks() - mouseDownTime;
  return true;
}

bool HalGPIO::isTouchHeldAt(float& nx, float& ny) const {
  if (!isMouseDown || touchSuppressed) return false;
  mapLogicalToNormalized(currentMouseX, currentMouseY, nx, ny);
  return true;
}

bool HalGPIO::wasTouchLongPress(float& nx, float& ny) const {
  if (!touchLongPressEvent) return false;
  mapLogicalToNormalized(touchDownX, touchDownY, nx, ny);
  return true;
}

bool HalGPIO::wasSwipe(float& nxStart, float& nyStart, float& nxEnd, float& nyEnd) const {
  if (!touchSwipeEvent) return false;
  mapLogicalToNormalized(touchDownX, touchDownY, nxStart, nyStart);
  mapLogicalToNormalized(touchUpX, touchUpY, nxEnd, nyEnd);
  return true;
}

bool HalGPIO::wasTouchActivity() const {
  return touchDownEvent || touchReleasedEvent || homeKeyTapEvent || homeKeyLongEvent;
}

void HalGPIO::onMouseDown(int x, int y) {
  isMouseDown = true;
  touchDownX = x;
  touchDownY = y;
  currentMouseX = x;
  currentMouseY = y;
  mouseDownTime = SDL_GetTicks();
  touchMovedBeyondSlop = false;
  touchLongPressFired = false;
  rawTouchDown = true;
}

void HalGPIO::onMouseMove(int x, int y) {
  currentMouseX = x;
  currentMouseY = y;
  if (isMouseDown) {
    if (std::abs(x - touchDownX) > 28 || std::abs(y - touchDownY) > 28) {
      touchMovedBeyondSlop = true;
    }
  }
}

void HalGPIO::onMouseUp(int x, int y) {
  if (!isMouseDown) return;
  isMouseDown = false;
  touchUpX = x;
  touchUpY = y;
  const uint32_t duration = SDL_GetTicks() - mouseDownTime;
  lastTouchDurationMs = duration;
  rawTouchReleased = true;

  if (touchSuppressed || touchLongPressFired) return;

  const int dx = x - touchDownX;
  const int dy = y - touchDownY;
  const int maxDelta = std::max(std::abs(dx), std::abs(dy));

  if (maxDelta >= 60 && duration <= 700) {
    rawTouchSwipe = true;
  } else if (maxDelta < 60) {
    rawTouchTap = true;
  }
}

void HalGPIO::handleKeyDown(SDL_Keycode sym) {
  int btn = -1;
  switch (sym) {
    case SDLK_ESCAPE: btn = BTN_BACK; break;
    case SDLK_RETURN:
    case SDLK_SPACE:  btn = BTN_CONFIRM; break;
    case SDLK_LEFT:   btn = BTN_LEFT; break;
    case SDLK_RIGHT:  btn = BTN_RIGHT; break;
    case SDLK_UP:
    case SDLK_PAGEUP: btn = BTN_UP; break;
    case SDLK_DOWN:
    case SDLK_PAGEDOWN: btn = BTN_DOWN; break;
    case SDLK_p:      btn = BTN_POWER; break;
    case SDLK_h:
      isHomeKeyDown = true;
      homeKeyDownTime = SDL_GetTicks();
      homeKeyLongFired = false;
      return;
  }
  if (btn >= 0) {
    currentState |= (1 << btn);
    rawPressed |= (1 << btn);
  }
}

void HalGPIO::handleKeyUp(SDL_Keycode sym) {
  int btn = -1;
  switch (sym) {
    case SDLK_ESCAPE: btn = BTN_BACK; break;
    case SDLK_RETURN:
    case SDLK_SPACE:  btn = BTN_CONFIRM; break;
    case SDLK_LEFT:   btn = BTN_LEFT; break;
    case SDLK_RIGHT:  btn = BTN_RIGHT; break;
    case SDLK_UP:
    case SDLK_PAGEUP: btn = BTN_UP; break;
    case SDLK_DOWN:
    case SDLK_PAGEDOWN: btn = BTN_DOWN; break;
    case SDLK_p:      btn = BTN_POWER; break;
    case SDLK_h:
      if (isHomeKeyDown) {
        isHomeKeyDown = false;
        if (!homeKeyLongFired) {
          rawHomeKeyTap = true;
        }
      }
      return;
  }
  if (btn >= 0) {
    currentState &= ~(1 << btn);
    rawReleased |= (1 << btn);
  }
}

void HalGPIO::handleSdlEvent(const SDL_Event& e) {
  if (e.type == SDL_KEYDOWN && !e.key.repeat) {
    handleKeyDown(e.key.keysym.sym);
  } else if (e.type == SDL_KEYUP) {
    handleKeyUp(e.key.keysym.sym);
  } else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
    onMouseDown(e.button.x, e.button.y);
  } else if (e.type == SDL_MOUSEMOTION) {
    onMouseMove(e.motion.x, e.motion.y);
  } else if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
    onMouseUp(e.button.x, e.button.y);
  }
}

void HalGPIO::update() {
  pressedEvents = rawPressed;
  releasedEvents = rawReleased;
  rawPressed = 0;
  rawReleased = 0;

  touchDownEvent = rawTouchDown;
  touchTapEvent = rawTouchTap;
  touchSwipeEvent = rawTouchSwipe;
  touchReleasedEvent = rawTouchReleased;
  touchLongPressEvent = rawTouchLongPress;
  homeKeyTapEvent = rawHomeKeyTap;
  homeKeyLongEvent = rawHomeKeyLong;

  rawTouchDown = false;
  rawTouchTap = false;
  rawTouchSwipe = false;
  rawTouchReleased = false;
  rawTouchLongPress = false;
  rawHomeKeyTap = false;
  rawHomeKeyLong = false;

  const uint32_t now = SDL_GetTicks();
  if (isMouseDown && !touchMovedBeyondSlop && !touchLongPressFired && !touchSuppressed) {
    if (now - mouseDownTime >= 500) {
      touchLongPressFired = true;
      touchSuppressed = true;
      rawTouchLongPress = true;
    }
  }

  if (isHomeKeyDown && !homeKeyLongFired && (now - homeKeyDownTime >= 700)) {
    homeKeyLongFired = true;
    rawHomeKeyLong = true;
  }

  if (!isMouseDown) {
    touchSuppressed = false;
    touchLongPressFired = false;
  }
}

void HalGPIO::injectTap(int x, int y) {
  touchDownX = x;
  touchDownY = y;
  touchUpX = x;
  touchUpY = y;
  lastTouchDurationMs = 100;
  rawTouchTap = true;
  rawTouchReleased = true;
}

void HalGPIO::injectSwipe(int x1, int y1, int x2, int y2, int durationMs) {
  touchDownX = x1;
  touchDownY = y1;
  touchUpX = x2;
  touchUpY = y2;
  lastTouchDurationMs = durationMs;
  rawTouchSwipe = true;
  rawTouchReleased = true;
}

void HalGPIO::injectButton(uint8_t btn, bool press) {
  if (press) {
    currentState |= (1 << btn);
    rawPressed |= (1 << btn);
  } else {
    currentState &= ~(1 << btn);
    rawReleased |= (1 << btn);
  }
}
