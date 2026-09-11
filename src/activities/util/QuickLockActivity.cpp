#include "QuickLockActivity.h"

#include <Arduino.h>
#include <HalDisplay.h>
#include <HalFrontlight.h>
#include <HalGPIO.h>
#include <I18n.h>

#include "CrossPointSettings.h"
#include "activities/ActivityManager.h"
#include "components/UITheme.h"
#include "fontIds.h"

QuickLockActivity::QuickLockActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : Activity("QuickLock", renderer, mappedInput) {}

void QuickLockActivity::onEnter() {
  Activity::onEnter();
  wasLightOn = Frontlight.isOn();
  if (wasLightOn) {
    Frontlight.setOn(false);
  }
  lockedAt = millis();
  lastClickAt = 0;
  unlockHintVisible = false;
  requestUpdateAndWait();
}

void QuickLockActivity::onExit() {
  if (wasLightOn) {
    Frontlight.setOn(true);
  }
  Activity::onExit();
}

void QuickLockActivity::unlock() {
  finish();
}

bool QuickLockActivity::handleHomeGesture() {
  unlock();
  return true;
}

void QuickLockActivity::loop() {
  const unsigned long now = millis();

  // Power button double-click to unlock
  if (gpio.wasReleased(HalGPIO::BTN_POWER)) {
    if (lastClickAt != 0 && (now - lastClickAt <= 500)) {
      unlock();
      return;
    }
    lastClickAt = now;
  }

  // Home gesture or Back button to unlock
  if (mappedInput.wasHomeGesture() || mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    unlock();
    return;
  }

  // Screen tap reveals unlock hint
  int touchX = 0;
  int touchY = 0;
  if (mappedInput.wasScreenTapped(touchX, touchY)) {
    unlockHintVisible = true;
    hintShownAt = now;
    requestUpdate();
  }

  if (unlockHintVisible && (now - hintShownAt > 3000)) {
    unlockHintVisible = false;
    requestUpdate();
  }
}

void QuickLockActivity::render(RenderLock&&) {
  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();

  const int bannerHeight = 60;
  const int bannerY = pageHeight - bannerHeight - 20;
  const int bannerWidth = pageWidth - 40;
  const int bannerX = 20;

  renderer.fillRect(bannerX, bannerY, bannerWidth, bannerHeight, false);
  renderer.drawRect(bannerX, bannerY, bannerWidth, bannerHeight, true);
  renderer.drawRect(bannerX + 1, bannerY + 1, bannerWidth - 2, bannerHeight - 2, true);

  const char* msg = tr(STR_LOCKED_MSG);
  const char* hint = tr(STR_UNLOCK_HINT);

  const int msgWidth = renderer.getTextWidth(UI_10_FONT_ID, msg);
  renderer.drawText(UI_10_FONT_ID, bannerX + (bannerWidth - msgWidth) / 2, bannerY + 12, msg);

  const int hintWidth = renderer.getTextWidth(SMALL_FONT_ID, hint);
  renderer.drawText(SMALL_FONT_ID, bannerX + (bannerWidth - hintWidth) / 2, bannerY + 36, hint);

  renderer.displayBuffer();
}
