#include <SDL2/SDL.h>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstdlib>

#include <builtinFonts/all.h>
#include <FontCacheManager.h>
#include <FontDecompressor.h>
#include <GfxRenderer.h>
#include <HalClock.h>
#include <HalDisplay.h>
#include <HalFrontlight.h>
#include <HalGPIO.h>
#include <HalPowerManager.h>
#include <HalStorage.h>
#include <HalSystem.h>
#include <HalTiltSensor.h>
#include <I18n.h>
#include <Logging.h>

#include "CrossPointSettings.h"
#include "CrossPointState.h"
#include "MappedInputManager.h"
#include "RecentBooksStore.h"
#include "SdCardFontSystem.h"
#include "activities/Activity.h"
#include "activities/ActivityManager.h"
#include "components/UITheme.h"
#include "fontIds.h"

// Global firmware singletons
GfxRenderer renderer(display);
MappedInputManager mappedInputManager(gpio, renderer);
ActivityManager activityManager(renderer, mappedInputManager);
FontDecompressor fontDecompressor;
SdCardFontSystem sdFontSystem;
FontCacheManager fontCacheManager(renderer.getFontMap(), renderer.getSdCardFonts());

// Fonts
EpdFont notoserif14RegularFont(&notoserif_14_regular);
EpdFont notoserif14BoldFont(&notoserif_14_bold);
EpdFont notoserif14ItalicFont(&notoserif_14_italic);
EpdFont notoserif14BoldItalicFont(&notoserif_14_bolditalic);
EpdFontFamily notoserif14FontFamily(&notoserif14RegularFont, &notoserif14BoldFont, &notoserif14ItalicFont,
                                    &notoserif14BoldItalicFont);

EpdFont notoserif12RegularFont(&notoserif_12_regular);
EpdFont notoserif12BoldFont(&notoserif_12_bold);
EpdFont notoserif12ItalicFont(&notoserif_12_italic);
EpdFont notoserif12BoldItalicFont(&notoserif_12_bolditalic);
EpdFontFamily notoserif12FontFamily(&notoserif12RegularFont, &notoserif12BoldFont, &notoserif12ItalicFont,
                                    &notoserif12BoldItalicFont);

EpdFont notoserif16RegularFont(&notoserif_16_regular);
EpdFont notoserif16BoldFont(&notoserif_16_bold);
EpdFont notoserif16ItalicFont(&notoserif_16_italic);
EpdFont notoserif16BoldItalicFont(&notoserif_16_bolditalic);
EpdFontFamily notoserif16FontFamily(&notoserif16RegularFont, &notoserif16BoldFont, &notoserif16ItalicFont,
                                    &notoserif16BoldItalicFont);

EpdFont notoserif18RegularFont(&notoserif_18_regular);
EpdFont notoserif18BoldFont(&notoserif_18_bold);
EpdFont notoserif18ItalicFont(&notoserif_18_italic);
EpdFont notoserif18BoldItalicFont(&notoserif_18_bolditalic);
EpdFontFamily notoserif18FontFamily(&notoserif18RegularFont, &notoserif18BoldFont, &notoserif18ItalicFont,
                                    &notoserif18BoldItalicFont);

EpdFont notosans12RegularFont(&notosans_12_regular);
EpdFont notosans12BoldFont(&notosans_12_bold);
EpdFont notosans12ItalicFont(&notosans_12_italic);
EpdFont notosans12BoldItalicFont(&notosans_12_bolditalic);
EpdFontFamily notosans12FontFamily(&notosans12RegularFont, &notosans12BoldFont, &notosans12ItalicFont,
                                   &notosans12BoldItalicFont);

EpdFont notosans14RegularFont(&notosans_14_regular);
EpdFont notosans14BoldFont(&notosans_14_bold);
EpdFont notosans14ItalicFont(&notosans_14_italic);
EpdFont notosans14BoldItalicFont(&notosans_14_bolditalic);
EpdFontFamily notosans14FontFamily(&notosans14RegularFont, &notosans14BoldFont, &notosans14ItalicFont,
                                   &notosans14BoldItalicFont);

EpdFont notosans16RegularFont(&notosans_16_regular);
EpdFont notosans16BoldFont(&notosans_16_bold);
EpdFont motion16BoldFont(&notosans_16_bold);
EpdFont notosans16ItalicFont(&notosans_16_italic);
EpdFont notosans16BoldItalicFont(&notosans_16_bolditalic);
EpdFontFamily notosans16FontFamily(&notosans16RegularFont, &notosans16BoldFont, &notosans16ItalicFont,
                                   &notosans16BoldItalicFont);

EpdFont notosans18RegularFont(&notosans_18_regular);
EpdFont notosans18BoldFont(&notosans_18_bold);
EpdFont notosans18ItalicFont(&notosans_18_italic);
EpdFont notosans18BoldItalicFont(&notosans_18_bolditalic);
EpdFontFamily notosans18FontFamily(&notosans18RegularFont, &notosans18BoldFont, &notosans18ItalicFont,
                                   &notosans18BoldItalicFont);

EpdFont smallFont(&notosans_8_regular);
EpdFontFamily smallFontFamily(&smallFont);

EpdFont ui10RegularFont(&ubuntu_10_regular);
EpdFont ui10BoldFont(&ubuntu_10_bold);
EpdFontFamily ui10FontFamily(&ui10RegularFont, &ui10BoldFont);

EpdFont ui12RegularFont(&ubuntu_12_regular);
EpdFont ui12BoldFont(&ubuntu_12_bold);
EpdFontFamily ui12FontFamily(&ui12RegularFont, &ui12BoldFont);

namespace {

struct TimedAction {
  uint32_t triggerMs = 0;
  std::string command;
  bool executed = false;
};

struct TimedScreenshot {
  uint32_t triggerMs = 0;
  std::string filepath;
  bool executed = false;
};

std::vector<TimedAction> g_scriptActions;
std::vector<TimedScreenshot> g_screenshots;
uint32_t g_timeoutMs = 0;

void parseInputScript(const std::string& scriptStr) {
  // Format: "1000:TAP:240,400;2500:SWIPE:400,10,400,300;3500:HOME;4000:QUIT"
  std::stringstream ss(scriptStr);
  std::string item;
  while (std::getline(ss, item, ';')) {
    if (item.empty()) continue;
    auto colonPos = item.find(':');
    if (colonPos != std::string::npos) {
      uint32_t ms = static_cast<uint32_t>(std::strtoul(item.substr(0, colonPos).c_str(), nullptr, 10));
      std::string cmd = item.substr(colonPos + 1);
      g_scriptActions.push_back({ms, cmd, false});
      std::cout << "[Simulator] Script action registered: at " << ms << "ms -> " << cmd << std::endl;
    }
  }
}

void parseScreenshots(const std::string& screenStr) {
  // Format: "1500:artifacts/screen1.bmp;3000:artifacts/screen2.bmp"
  std::stringstream ss(screenStr);
  std::string item;
  while (std::getline(ss, item, ';')) {
    if (item.empty()) continue;
    auto colonPos = item.find(':');
    if (colonPos != std::string::npos) {
      uint32_t ms = static_cast<uint32_t>(std::strtoul(item.substr(0, colonPos).c_str(), nullptr, 10));
      std::string path = item.substr(colonPos + 1);
      g_screenshots.push_back({ms, path, false});
      std::cout << "[Simulator] Scheduled screenshot: at " << ms << "ms -> " << path << std::endl;
    }
  }
}

void executeCommand(const std::string& cmd, bool& running) {
  std::cout << "[Simulator] Executing script command: " << cmd << std::endl;
  if (cmd == "QUIT") {
    running = false;
    return;
  }
  if (cmd == "HOME") {
    activityManager.goHome();
    return;
  }
  if (cmd == "BACK") {
    gpio.injectButton(HalGPIO::BTN_BACK, true);
    gpio.injectButton(HalGPIO::BTN_BACK, false);
    return;
  }
  if (cmd == "ENTER" || cmd == "CONFIRM") {
    gpio.injectButton(HalGPIO::BTN_CONFIRM, true);
    gpio.injectButton(HalGPIO::BTN_CONFIRM, false);
    return;
  }
  if (cmd == "UP") {
    gpio.injectButton(HalGPIO::BTN_UP, true);
    gpio.injectButton(HalGPIO::BTN_UP, false);
    return;
  }
  if (cmd == "DOWN") {
    gpio.injectButton(HalGPIO::BTN_DOWN, true);
    gpio.injectButton(HalGPIO::BTN_DOWN, false);
    return;
  }
  if (cmd == "LEFT") {
    gpio.injectButton(HalGPIO::BTN_LEFT, true);
    gpio.injectButton(HalGPIO::BTN_LEFT, false);
    return;
  }
  if (cmd == "RIGHT") {
    gpio.injectButton(HalGPIO::BTN_RIGHT, true);
    gpio.injectButton(HalGPIO::BTN_RIGHT, false);
    return;
  }
  if (cmd.rfind("TAP:", 0) == 0) {
    // TAP:x,y
    std::string coords = cmd.substr(4);
    auto comma = coords.find(',');
    if (comma != std::string::npos) {
      int x = std::atoi(coords.substr(0, comma).c_str());
      int y = std::atoi(coords.substr(comma + 1).c_str());
      gpio.injectTap(x, y);
    }
    return;
  }
  if (cmd.rfind("SWIPE:", 0) == 0) {
    // SWIPE:x1,y1,x2,y2[,ms]
    std::stringstream cs(cmd.substr(6));
    std::string p1, p2, p3, p4, p5;
    std::getline(cs, p1, ',');
    std::getline(cs, p2, ',');
    std::getline(cs, p3, ',');
    std::getline(cs, p4, ',');
    std::getline(cs, p5, ',');
    int x1 = std::atoi(p1.c_str());
    int y1 = std::atoi(p2.c_str());
    int x2 = std::atoi(p3.c_str());
    int y2 = std::atoi(p4.c_str());
    int dur = p5.empty() ? 200 : std::atoi(p5.c_str());
    gpio.injectSwipe(x1, y1, x2, y2, dur);
    return;
  }
}

void setupDisplayAndFonts() {
  display.begin(false);
  renderer.begin();
  activityManager.begin();

  if (!fontDecompressor.init()) {
    std::cerr << "[Simulator] Font decompressor init failed" << std::endl;
  }
  fontCacheManager.setFontDecompressor(&fontDecompressor);
  renderer.setFontCacheManager(&fontCacheManager);

  renderer.insertFont(NOTOSERIF_14_FONT_ID, notoserif14FontFamily);
  renderer.insertFont(NOTOSERIF_12_FONT_ID, notoserif12FontFamily);
  renderer.insertFont(NOTOSERIF_16_FONT_ID, notoserif16FontFamily);
  renderer.insertFont(NOTOSERIF_18_FONT_ID, notoserif18FontFamily);

  renderer.insertFont(NOTOSANS_12_FONT_ID, notosans12FontFamily);
  renderer.insertFont(NOTOSANS_14_FONT_ID, notosans14FontFamily);
  renderer.insertFont(NOTOSANS_16_FONT_ID, notosans16FontFamily);
  renderer.insertFont(NOTOSANS_18_FONT_ID, notosans18FontFamily);

  renderer.insertFont(UI_10_FONT_ID, ui10FontFamily);
  renderer.insertFont(UI_12_FONT_ID, ui12FontFamily);
  renderer.insertFont(SMALL_FONT_ID, smallFontFamily);

  sdFontSystem.begin(renderer);
  std::cout << "[Simulator] Display, Fonts and ActivityManager initialized." << std::endl;
}

}  // namespace

int main(int argc, char* argv[]) {
  std::cout << "========================================================" << std::endl;
  std::cout << " BookPoint 2.0.0 — Native SDL2 Firmware Simulator" << std::endl;
  std::cout << " Platform: Windows x86_64 (MinGW-W64 GCC 13.2.0)" << std::endl;
  std::cout << "========================================================" << std::endl;

  // Check command-line args and environment variables
  const char* envScript = std::getenv("CROSSPOINT_SIM_INPUT_SCRIPT");
  if (envScript) parseInputScript(envScript);

  const char* envScreens = std::getenv("CROSSPOINT_SIM_SCREENSHOTS");
  if (envScreens) parseScreenshots(envScreens);

  const char* envTimeout = std::getenv("CROSSPOINT_SIM_TIMEOUT_MS");
  if (envTimeout) g_timeoutMs = static_cast<uint32_t>(std::strtoul(envTimeout, nullptr, 10));

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--script" && i + 1 < argc) {
      parseInputScript(argv[++i]);
    } else if (arg == "--screenshots" && i + 1 < argc) {
      parseScreenshots(argv[++i]);
    } else if (arg == "--timeout" && i + 1 < argc) {
      g_timeoutMs = static_cast<uint32_t>(std::strtoul(argv[++i], nullptr, 10));
    }
  }

  // 1. Mount virtual SD Card filesystem
  Storage.begin();
  std::cout << "[Simulator] Virtual SD filesystem mounted at ./fs_/" << std::endl;

  // 2. Load settings, state and recent books
  SETTINGS.loadFromFile();
  I18N.setLanguage(static_cast<Language>(SETTINGS.language));
  APP_STATE.loadFromFile();
  RECENT_BOOKS.loadFromFile();
  UITheme::getInstance().reload();

  // 3. Initialize display, renderer and fonts
  setupDisplayAndFonts();

  // 4. Navigate to Home
  activityManager.goHome();

  uint32_t startMs = SDL_GetTicks();
  bool running = true;

  while (running) {
    uint32_t elapsed = SDL_GetTicks() - startMs;

    if (g_timeoutMs > 0 && elapsed >= g_timeoutMs) {
      std::cout << "[Simulator] Timeout reached (" << g_timeoutMs << " ms). Exiting..." << std::endl;
      break;
    }

    // Process window events
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        running = false;
        break;
      }
      gpio.handleSdlEvent(event);
    }
    if (!running) break;

    // Process timed script actions
    for (auto& act : g_scriptActions) {
      if (!act.executed && elapsed >= act.triggerMs) {
        act.executed = true;
        executeCommand(act.command, running);
        if (!running) break;
      }
    }
    if (!running) break;

    // Tick Input and ActivityManager
    gpio.update();
    activityManager.loop();

    // Render present if dirty
    if (display.isDirty()) {
      display.present();
    }

    // Process scheduled screenshots
    for (auto& s : g_screenshots) {
      if (!s.executed && elapsed >= s.triggerMs) {
        s.executed = true;
        std::cout << "[Simulator] Capturing screenshot: " << s.filepath << std::endl;
        display.saveScreenshotBMP(s.filepath.c_str());
      }
    }

    SDL_Delay(10);
  }

  std::cout << "[Simulator] Shutting down cleanly." << std::endl;
  std::_Exit(0);
}

