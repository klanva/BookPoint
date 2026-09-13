#include "FrontlightPanelActivity.h"

#include <FreeInkUIIcon.h>
#include <GfxRenderer.h>
#include <HalFrontlight.h>
#include <HalGPIO.h>
#include <HalPowerManager.h>
#include <I18n.h>
#include <WiFi.h>

#include <cmath>
#include <cstdio>
#include <ctime>

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
  SETTINGS.frontlightBrightness = brightness;
  SETTINGS.saveToFile();
  requestUpdate();
}

void FrontlightPanelActivity::adjustWarmth(const int delta) {
  int next = static_cast<int>(warmth) + delta;
  if (next < 0) next = 0;
  if (next > 100) next = 100;
  if (next == warmth) return;
  warmth = static_cast<uint8_t>(next);
  Frontlight.setWarmth(warmth);
  SETTINGS.frontlightWarmth = warmth;
  SETTINGS.saveToFile();
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
    const int pageWidth = renderer.getScreenWidth();
    const int offsetX = (pageWidth > 480) ? (pageWidth - 448) / 2 : 0;

    // Check 4 control pills:
    // WIFI: (offsetX + 20, 155, 210, 46)
    if (tapX >= offsetX + 20 && tapX <= offsetX + 230 && tapY >= 155 && tapY <= 201) {
      if (WiFi.status() == WL_CONNECTED || WiFi.getMode() != WIFI_OFF) {
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
      } else {
        WiFi.mode(WIFI_STA);
      }
      requestUpdate();
      return;
    }
    // DARK_MODE: (offsetX + 250, 155, 210, 46)
    if (tapX >= offsetX + 250 && tapX <= offsetX + 460 && tapY >= 155 && tapY <= 201) {
      SETTINGS.screenInverted = (SETTINGS.screenInverted ? 0 : 1);
      SETTINGS.saveToFile();
      requestUpdate();
      return;
    }
    // ROTATION_LOCK: (offsetX + 20, 215, 210, 46)
    if (tapX >= offsetX + 20 && tapX <= offsetX + 230 && tapY >= 215 && tapY <= 261) {
      SETTINGS.orientation = (SETTINGS.orientation == CrossPointSettings::PORTRAIT ? CrossPointSettings::LANDSCAPE_CW : CrossPointSettings::PORTRAIT);
      SETTINGS.saveToFile();
      requestUpdate();
      return;
    }
    // SLEEP: (offsetX + 250, 215, 210, 46)
    if (tapX >= offsetX + 250 && tapX <= offsetX + 460 && tapY >= 215 && tapY <= 261) {
      close();
      activityManager.goToQuickLock();
      return;
    }

    // Discrete step slider buttons (>= 44px) & track seek:
    // Brightness: minus (offsetX + 16, 54, 44, 44), plus (offsetX + 420, 54, 44, 44)
    if (tapX >= offsetX + 16 && tapX <= offsetX + 60 && tapY >= 54 && tapY <= 98) {
      adjustBrightness(-10);
      return;
    }
    if (tapX >= offsetX + 420 && tapX <= offsetX + 464 && tapY >= 54 && tapY <= 98) {
      adjustBrightness(10);
      return;
    }
    // Brightness track direct touch seek (offsetX + 68, 54, 344, 44)
    if (tapX >= offsetX + 68 && tapX <= offsetX + 412 && tapY >= 54 && tapY <= 98) {
      float frac = static_cast<float>(tapX - (offsetX + 68)) / 344.0f;
      int val = static_cast<int>(std::round(frac * 100.0f));
      if (val < 0) val = 0;
      if (val > 100) val = 100;
      brightness = static_cast<uint8_t>(val);
      Frontlight.setBrightness(brightness);
      if (!lightOn) {
        lightOn = true;
        lightOnChanged = true;
        Frontlight.setOn(true);
      }
      SETTINGS.frontlightBrightness = brightness;
      SETTINGS.saveToFile();
      requestUpdate();
      return;
    }

    // CCT / Warmth: minus (offsetX + 16, 104, 44, 44), plus (offsetX + 420, 104, 44, 44)
    if (tapX >= offsetX + 16 && tapX <= offsetX + 60 && tapY >= 104 && tapY <= 148) {
      adjustWarmth(-10);
      return;
    }
    if (tapX >= offsetX + 420 && tapX <= offsetX + 464 && tapY >= 104 && tapY <= 148) {
      adjustWarmth(10);
      return;
    }
    // CCT track direct touch seek (offsetX + 68, 104, 344, 44)
    if (tapX >= offsetX + 68 && tapX <= offsetX + 412 && tapY >= 104 && tapY <= 148) {
      float frac = static_cast<float>(tapX - (offsetX + 68)) / 344.0f;
      int val = static_cast<int>(std::round(frac * 100.0f));
      if (val < 0) val = 0;
      if (val > 100) val = 100;
      warmth = static_cast<uint8_t>(val);
      Frontlight.setWarmth(warmth);
      SETTINGS.frontlightWarmth = warmth;
      SETTINGS.saveToFile();
      requestUpdate();
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

  const int offsetX = (pageWidth > 480) ? (pageWidth - 448) / 2 : 0;

  renderer.drawText(UI_12_FONT_ID, offsetX + 20, 38, "Центр управления", true);

  // Brightness row at Y: 54..98
  renderer.drawText(SMALL_FONT_ID, offsetX + 68, 56, "Холодный", true);
  renderer.drawRect(offsetX + 16, 54, 44, 44, true);
  renderer.drawText(UI_12_FONT_ID, offsetX + 33, 67, "-", true);
  renderer.drawRect(offsetX + 68, 72, 344, 8, true);
  const int bFill = (static_cast<int>(brightness) * 344) / 100;
  renderer.fillRect(offsetX + 68, 72, bFill, 8, true);
  renderer.drawRect(offsetX + 420, 54, 44, 44, true);
  renderer.drawText(UI_12_FONT_ID, offsetX + 437, 67, "+", true);

  // CCT row at Y: 104..148
  renderer.drawText(SMALL_FONT_ID, offsetX + 68, 106, "Теплый", true);
  renderer.drawRect(offsetX + 16, 104, 44, 44, true);
  renderer.drawText(UI_12_FONT_ID, offsetX + 33, 117, "-", true);
  renderer.drawRect(offsetX + 68, 122, 344, 8, true);
  const int cctFill = (static_cast<int>(warmth) * 344) / 100;
  renderer.fillRect(offsetX + 68, 122, cctFill, 8, true);
  renderer.drawRect(offsetX + 420, 104, 44, 44, true);
  renderer.drawText(UI_12_FONT_ID, offsetX + 437, 117, "+", true);

  // 4 Control Pills
  // WIFI: (offsetX + 20, 155, 210, 46)
  const bool wifiOn = (WiFi.status() == WL_CONNECTED || WiFi.getMode() != WIFI_OFF);
  if (wifiOn) {
    renderer.fillRect(offsetX + 20, 155, 210, 46, true);
    renderer.drawText(SMALL_FONT_ID, offsetX + 45, 170, "Wi-Fi (Вкл)", false);
  } else {
    renderer.drawRect(offsetX + 20, 155, 210, 46, true);
    renderer.drawText(SMALL_FONT_ID, offsetX + 45, 170, "Wi-Fi (Выкл)", true);
  }

  // DARK_MODE: (offsetX + 250, 155, 210, 46)
  const bool darkOn = (SETTINGS.screenInverted != 0);
  if (darkOn) {
    renderer.fillRect(offsetX + 250, 155, 210, 46, true);
    renderer.drawText(SMALL_FONT_ID, offsetX + 275, 170, "Ночной режим (Вкл)", false);
  } else {
    renderer.drawRect(offsetX + 250, 155, 210, 46, true);
    renderer.drawText(SMALL_FONT_ID, offsetX + 275, 170, "Ночной режим (Выкл)", true);
  }

  // ROTATION_LOCK: (offsetX + 20, 215, 210, 46)
  const bool rotOn = (SETTINGS.orientation != CrossPointSettings::PORTRAIT);
  if (rotOn) {
    renderer.fillRect(offsetX + 20, 215, 210, 46, true);
    renderer.drawText(SMALL_FONT_ID, offsetX + 45, 230, "Автоповорот (Вкл)", false);
  } else {
    renderer.drawRect(offsetX + 20, 215, 210, 46, true);
    renderer.drawText(SMALL_FONT_ID, offsetX + 45, 230, "Автоповорот (Выкл)", true);
  }

  // SLEEP: (offsetX + 250, 215, 210, 46)
  renderer.drawRect(offsetX + 250, 215, 210, 46, true);
  renderer.drawText(SMALL_FONT_ID, offsetX + 275, 230, "Режим сна", true);

  // Battery stats & Clock/Date at Y: 290 and 320
  char statsBuf[64];
  char uptimeBuf[48];
  const int percent = powerManager.getBatteryPercentage();
  const bool isCharging = gpio.isUsbConnected();
  const uint32_t uptime = millis() / 1000;
  const uint32_t hours = uptime / 3600;
  const uint32_t mins = (uptime % 3600) / 60;
  time_t rawTime = time(nullptr);
  struct tm* t = localtime(&rawTime);
  char timeStr[16] = "12:00";
  char dateStr[24] = "01.01.2026";
  if (t) {
    strftime(timeStr, sizeof(timeStr), "%H:%M", t);
    strftime(dateStr, sizeof(dateStr), "%d.%m.%Y", t);
  }
  if (isCharging) {
    snprintf(statsBuf, sizeof(statsBuf), "Батарея: %d%% (Зарядка) • %s • %s", percent, timeStr, dateStr);
  } else {
    snprintf(statsBuf, sizeof(statsBuf), "Батарея: %d%% • %s • %s", percent, timeStr, dateStr);
  }
  snprintf(uptimeBuf, sizeof(uptimeBuf), "Время работы: %uч %uм", (unsigned)hours, (unsigned)mins);
  renderer.drawText(SMALL_FONT_ID, offsetX + 20, 290, statsBuf, true);
  renderer.drawText(SMALL_FONT_ID, offsetX + 20, 320, uptimeBuf, true);

  renderer.drawLine(0, panelBottom - 1, pageWidth, panelBottom - 1);
  renderer.displayBuffer();
}
