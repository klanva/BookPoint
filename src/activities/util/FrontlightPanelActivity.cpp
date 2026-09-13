#include "FrontlightPanelActivity.h"

#include <FreeInkUIIcon.h>
#include <GfxRenderer.h>
#include <HalFrontlight.h>
#include <HalGPIO.h>
#include <HalPowerManager.h>
#include <I18n.h>
#include <WiFi.h>

#include <cstdio>

#include "CrossPointSettings.h"
#include "MappedInputManager.h"
#include "activities/ActivityManager.h"
#include "components/UITheme.h"
#include "components/UIThemeTokens.h"
#include "components/icons/customListIcons.h"
#include "components/icons/listIcons.h"
#include "fontIds.h"

namespace fui = freeink::ui;

namespace {
constexpr fui::ActionId ACTION_BRIGHTNESS = 1;
constexpr fui::ActionId ACTION_WARMTH = 2;
constexpr fui::ActionId ACTION_TOGGLE = 3;
constexpr fui::ActionId ACTION_BRIGHTNESS_STEP = 4;
constexpr fui::ActionId ACTION_WARMTH_STEP = 5;
constexpr fui::ActionId ACTION_QUICK_LOCK = 6;
constexpr int BUTTON_BRIGHTNESS_STEP = 5;
constexpr int FINE_STEP = 1;

uint8_t percentFromPermille(const int16_t permille) {
  int value = (static_cast<int>(permille) * 100 + 500) / 1000;
  if (value < 0) value = 0;
  if (value > 100) value = 100;
  return static_cast<uint8_t>(value);
}
}  // namespace

FrontlightPanelActivity::FrontlightPanelActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : Activity("FrontlightPanel", renderer, mappedInput), UiAppHost(renderer) {}

void FrontlightPanelActivity::onEnter() {
  Activity::onEnter();

  brightness = Frontlight.brightness();
  warmth = Frontlight.warmth();
  lightOn = Frontlight.isOn();
  lightOnChanged = false;

  resetUi();
  app.on(ACTION_BRIGHTNESS, &FrontlightPanelActivity::onBrightnessEvent, this);
  app.on(ACTION_WARMTH, &FrontlightPanelActivity::onWarmthEvent, this);
  app.on(ACTION_TOGGLE, &FrontlightPanelActivity::onToggleEvent, this);
  app.on(ACTION_BRIGHTNESS_STEP, &FrontlightPanelActivity::onBrightnessStepEvent, this);
  app.on(ACTION_WARMTH_STEP, &FrontlightPanelActivity::onWarmthStepEvent, this);
  app.on(ACTION_QUICK_LOCK, &FrontlightPanelActivity::onQuickLockEvent, this);
  app.setScreen(&FrontlightPanelActivity::panelScreen, this);
  requestUpdate();
}

void FrontlightPanelActivity::onExit() {
  // brightness/warmth are always restored unconditionally on boot (see
  // main.cpp), so they never diverge from SETTINGS at onEnter() — comparing
  // against SETTINGS here only fires on a genuine user change. lightOn has
  // no such guarantee (see lightOnChanged's declaration), so it's gated on
  // the user actually having touched it this session instead.
  const bool changed = SETTINGS.frontlightBrightness != brightness || SETTINGS.frontlightWarmth != warmth ||
                       (lightOnChanged && SETTINGS.frontlightOn != (lightOn ? 1 : 0));
  if (changed) {
    SETTINGS.frontlightBrightness = brightness;
    SETTINGS.frontlightWarmth = warmth;
    if (lightOnChanged) SETTINGS.frontlightOn = lightOn ? 1 : 0;
    SETTINGS.saveToFile();
  }
  Activity::onExit();
}

void FrontlightPanelActivity::onBrightnessEvent(const fui::ActionEvent& event, void* user) {
  auto* self = static_cast<FrontlightPanelActivity*>(user);
  if (event.dragPermille < 0) return;
  self->brightness = percentFromPermille(event.dragPermille);
  Frontlight.setBrightness(self->brightness);
  if (!self->lightOn) {
    self->lightOn = true;
    self->lightOnChanged = true;
    Frontlight.setOn(true);
  }
}

void FrontlightPanelActivity::onWarmthEvent(const fui::ActionEvent& event, void* user) {
  auto* self = static_cast<FrontlightPanelActivity*>(user);
  if (event.dragPermille < 0) return;
  self->warmth = percentFromPermille(event.dragPermille);
  Frontlight.setWarmth(self->warmth);
}

void FrontlightPanelActivity::onToggleEvent(const fui::ActionEvent&, void* user) {
  static_cast<FrontlightPanelActivity*>(user)->toggleLight();
}

void FrontlightPanelActivity::onBrightnessStepEvent(const fui::ActionEvent& event, void* user) {
  static_cast<FrontlightPanelActivity*>(user)->adjustBrightness(event.value * FINE_STEP);
}

void FrontlightPanelActivity::onWarmthStepEvent(const fui::ActionEvent& event, void* user) {
  static_cast<FrontlightPanelActivity*>(user)->adjustWarmth(event.value * FINE_STEP);
}

void FrontlightPanelActivity::onQuickLockEvent(const fui::ActionEvent&, void* user) {
  auto* self = static_cast<FrontlightPanelActivity*>(user);
  self->close();
  activityManager.goToQuickLock();
}

void FrontlightPanelActivity::adjustBrightness(const int delta) {
  int next = static_cast<int>(brightness) + delta;
  if (next < 0) next = 0;
  if (next > 100) next = 100;
  if (next == brightness) return;
  brightness = static_cast<uint8_t>(next);
  Frontlight.setBrightness(brightness);
  if (!lightOn) {
    lightOn = true;
    lightOnChanged = true;
    Frontlight.setOn(true);
  }
  requestUpdate();
}

void FrontlightPanelActivity::adjustWarmth(const int delta) {
  int next = static_cast<int>(warmth) + delta;
  if (next < 0) next = 0;
  if (next > 100) next = 100;
  if (next == warmth) return;
  warmth = static_cast<uint8_t>(next);
  Frontlight.setWarmth(warmth);
  requestUpdate();
}

void FrontlightPanelActivity::toggleLight() {
  lightOn = !lightOn;
  lightOnChanged = true;
  Frontlight.setOn(lightOn);
  requestUpdate();
}

void FrontlightPanelActivity::close() { finish(); }

bool FrontlightPanelActivity::handleHomeGesture() {
  close();
  return true;
}

void FrontlightPanelActivity::loop() {
  if (mappedInput.wasSwipe() == MappedInputManager::SwipeDir::Up) {
    close();
    return;
  }

  int tapX = 0, tapY = 0;
  if (mappedInput.wasScreenTapped(tapX, tapY)) {
    if (tapY >= 360) {
      close();
      return;
    }
    // Check 4 control pills:
    // WIFI: (20, 155, 210, 46)
    if (tapX >= 20 && tapX <= 230 && tapY >= 155 && tapY <= 201) {
      if (WiFi.status() == WL_CONNECTED || WiFi.getMode() != WIFI_OFF) {
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
      } else {
        WiFi.mode(WIFI_STA);
      }
      requestUpdate();
      return;
    }
    // DARK_MODE: (250, 155, 210, 46)
    if (tapX >= 250 && tapX <= 460 && tapY >= 155 && tapY <= 201) {
      SETTINGS.screenInverted = (SETTINGS.screenInverted ? 0 : 1);
      SETTINGS.saveToFile();
      requestUpdate();
      return;
    }
    // ROTATION_LOCK: (20, 215, 210, 46)
    if (tapX >= 20 && tapX <= 230 && tapY >= 215 && tapY <= 261) {
      SETTINGS.orientation = (SETTINGS.orientation == CrossPointSettings::PORTRAIT ? CrossPointSettings::LANDSCAPE_CW : CrossPointSettings::PORTRAIT);
      SETTINGS.saveToFile();
      requestUpdate();
      return;
    }
    // SLEEP: (250, 215, 210, 46)
    if (tapX >= 250 && tapX <= 460 && tapY >= 215 && tapY <= 261) {
      close();
      activityManager.goToQuickLock();
      return;
    }

    // Discrete step slider buttons:
    // Brightness: minus (20, 60, 40, 32), plus (420, 60, 40, 32)
    if (tapX >= 20 && tapX <= 60 && tapY >= 60 && tapY <= 92) {
      adjustBrightness(-10);
      return;
    }
    if (tapX >= 420 && tapX <= 460 && tapY >= 60 && tapY <= 92) {
      adjustBrightness(10);
      return;
    }
    // CCT / Warmth: minus (20, 105, 40, 32), plus (420, 105, 40, 32)
    if (tapX >= 20 && tapX <= 60 && tapY >= 105 && tapY <= 137) {
      adjustWarmth(-10);
      return;
    }
    if (tapX >= 420 && tapX <= 460 && tapY >= 105 && tapY <= 137) {
      adjustWarmth(10);
      return;
    }
  }

  const auto touch = routeTouch(mappedInput, false, /*routeHeld=*/true);
  if (touch.routed) {
    if (app.invalidated()) requestUpdate();
    if (touch) {
      if (touch.event.dragPermille >= 0) draggingSlider = true;
      return;
    }
    if (touch.snap.touchReleased && !draggingSlider && touch.snap.touchY >= panelBottom) {
      close();
      return;
    }
  }
  if (draggingSlider) {
    if (!touch.snap.touchHeld) draggingSlider = false;
    return;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    close();
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    toggleLight();
    return;
  }

  buttonNavigator.onPressAndContinuous({MappedInputManager::Button::Left},
                                       [this] { adjustBrightness(-BUTTON_BRIGHTNESS_STEP); });
  buttonNavigator.onPressAndContinuous({MappedInputManager::Button::Right},
                                       [this] { adjustBrightness(BUTTON_BRIGHTNESS_STEP); });
}

int FrontlightPanelActivity::computePanelBottom() const {
  return 360;
}

void FrontlightPanelActivity::panelScreen(UiScreen& screen, void* user) {
  static_cast<FrontlightPanelActivity*>(user)->buildPanelScreen(screen);
}

void FrontlightPanelActivity::buildPanelScreen(UiScreen& screen) {
}

void FrontlightPanelActivity::addStepSlider(UiScreen& screen, const fui::Rect& row, const uint8_t value,
                                            const fui::ActionId sliderAction, const fui::ActionId stepAction) {
}

void FrontlightPanelActivity::render(RenderLock&&) {
  panelBottom = 360;
  const int pageWidth = renderer.getScreenWidth();
  renderer.fillRect(0, 0, pageWidth, panelBottom, false);

  renderer.drawText(UI_12_FONT_ID, 20, 38, "Центр управления", true);

  // Brightness row at Y: 60..92
  renderer.drawRect(20, 60, 40, 32, true);
  renderer.drawText(UI_12_FONT_ID, 35, 68, "-", true);
  renderer.drawRect(70, 72, 340, 8, true);
  const int bFill = (static_cast<int>(brightness) * 340) / 100;
  renderer.fillRect(70, 72, bFill, 8, true);
  renderer.drawRect(420, 60, 40, 32, true);
  renderer.drawText(UI_12_FONT_ID, 433, 68, "+", true);

  // CCT row at Y: 105..137
  renderer.drawRect(20, 105, 40, 32, true);
  renderer.drawText(UI_12_FONT_ID, 35, 113, "-", true);
  renderer.drawRect(70, 117, 340, 8, true);
  const int cctFill = (static_cast<int>(warmth) * 340) / 100;
  renderer.fillRect(70, 117, cctFill, 8, true);
  renderer.drawRect(420, 105, 40, 32, true);
  renderer.drawText(UI_12_FONT_ID, 433, 113, "+", true);

  // 4 Control Pills
  // WIFI: (20, 155, 210, 46)
  const bool wifiOn = (WiFi.status() == WL_CONNECTED || WiFi.getMode() != WIFI_OFF);
  if (wifiOn) {
    renderer.fillRect(20, 155, 210, 46, true);
    renderer.drawText(SMALL_FONT_ID, 45, 170, "Wi-Fi (Вкл)", false);
  } else {
    renderer.drawRect(20, 155, 210, 46, true);
    renderer.drawText(SMALL_FONT_ID, 45, 170, "Wi-Fi (Выкл)", true);
  }

  // DARK_MODE: (250, 155, 210, 46)
  const bool darkOn = (SETTINGS.screenInverted != 0);
  if (darkOn) {
    renderer.fillRect(250, 155, 210, 46, true);
    renderer.drawText(SMALL_FONT_ID, 275, 170, "Ночной режим (Вкл)", false);
  } else {
    renderer.drawRect(250, 155, 210, 46, true);
    renderer.drawText(SMALL_FONT_ID, 275, 170, "Ночной режим (Выкл)", true);
  }

  // ROTATION_LOCK: (20, 215, 210, 46)
  const bool rotOn = (SETTINGS.orientation != CrossPointSettings::PORTRAIT);
  if (rotOn) {
    renderer.fillRect(20, 215, 210, 46, true);
    renderer.drawText(SMALL_FONT_ID, 45, 230, "Автоповорот (Вкл)", false);
  } else {
    renderer.drawRect(20, 215, 210, 46, true);
    renderer.drawText(SMALL_FONT_ID, 45, 230, "Автоповорот (Выкл)", true);
  }

  // SLEEP: (250, 215, 210, 46)
  renderer.drawRect(250, 215, 210, 46, true);
  renderer.drawText(SMALL_FONT_ID, 275, 230, "Режим сна", true);

  // Battery stats at (20, 280, 440, 60)
  char statsBuf[64];
  const int percent = powerManager.getBatteryPercentage();
  const bool isCharging = gpio.isUsbConnected();
  const uint32_t uptime = millis() / 1000;
  const uint32_t hours = uptime / 3600;
  const uint32_t mins = (uptime % 3600) / 60;
  if (isCharging) {
    snprintf(statsBuf, sizeof(statsBuf), "Батарея: %d%% (Зарядка) • Работа: %uч %uм", percent, (unsigned)hours, (unsigned)mins);
  } else {
    snprintf(statsBuf, sizeof(statsBuf), "Батарея: %d%% • Работа: %uч %uм", percent, (unsigned)hours, (unsigned)mins);
  }
  renderer.drawText(SMALL_FONT_ID, 20, 305, statsBuf, true);

  renderer.drawLine(0, panelBottom - 1, pageWidth, panelBottom - 1);
  renderer.displayBuffer();
}
