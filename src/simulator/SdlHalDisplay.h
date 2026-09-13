#pragma once

#include <SDL2/SDL.h>
#include <cstdint>
#include <cstring>
#include <atomic>
#include <mutex>
#include <thread>

class HalDisplay {
 public:
  enum RefreshMode {
    FULL_REFRESH = 0,
    HALF_REFRESH = 1,
    FAST_REFRESH = 2
  };

  static constexpr uint16_t DISPLAY_WIDTH = 800;
  static constexpr uint16_t DISPLAY_HEIGHT = 480;
  static constexpr uint16_t DISPLAY_WIDTH_BYTES = DISPLAY_WIDTH / 8; // 100
  static constexpr uint32_t BUFFER_SIZE = DISPLAY_WIDTH_BYTES * DISPLAY_HEIGHT; // 48000

  // Logical portrait dimensions
  static constexpr int LOGICAL_WIDTH = 480;
  static constexpr int LOGICAL_HEIGHT = 800;

  // E-Ink Palette (RGBA8888)
  static constexpr uint32_t COLOR_PAPER_WHITE = 0xF5F3EEFF;
  static constexpr uint32_t COLOR_INK_BLACK   = 0x181A1CFF;
  static constexpr uint32_t COLOR_LIGHT_GRAY  = 0xBEBBB4FF;
  static constexpr uint32_t COLOR_DARK_GRAY   = 0x605E59FF;

  HalDisplay();
  ~HalDisplay();

  void begin(bool seamless = false);
  void clearScreen(uint8_t color = 0xFF) const;
  uint8_t* getFrameBuffer() const { return frameBuffer; }
  uint8_t* lendFrameBufferStorage(uint32_t* sizeOut);
  void returnFrameBufferStorage();

  void drawImage(const uint8_t* imageData, uint16_t x, uint16_t y, uint16_t w, uint16_t h, bool fromProgmem = true);
  void drawImageTransparent(const uint8_t* imageData, uint16_t x, uint16_t y, uint16_t w, uint16_t h, bool fromProgmem = true);

  // Display Updates
  void displayBuffer(RefreshMode mode = FAST_REFRESH, bool turnOffScreen = false);
  void displayBufferAsync(RefreshMode mode = FAST_REFRESH);
  void waitRefreshComplete() {}
  bool supportsAsyncRefresh() const { return true; }
  void refreshDisplay(RefreshMode mode = FAST_REFRESH, bool turnOffScreen = false);

  // Grayscale Support
  void copyGrayscaleBuffers(const uint8_t* lsb, const uint8_t* msb);
  void copyGrayscaleLsbBuffers(const uint8_t* lsb);
  void copyGrayscaleMsbBuffers(const uint8_t* msb);
  void writeGrayscalePlaneStrip(bool lsbPlane, const uint8_t* rows, uint16_t yStart, uint16_t numRows);
  void cleanupGrayscaleBuffers(const uint8_t* bwBuffer);
  void displayGrayscaleBase(RefreshMode fallback = HALF_REFRESH, bool turnOffScreen = false);
  void displayGrayBuffer(bool turnOffScreen = false);

  bool supportsStripGrayscale() const { return true; }
  bool combinesGrayscaleBase() const { return false; }
  void preconditionGrayscale() {}
  void preconditionGrayscale(uint16_t, uint16_t, uint16_t, uint16_t) {}

  // Geometry
  uint16_t getDisplayWidth() const { return DISPLAY_WIDTH; }
  uint16_t getDisplayHeight() const { return DISPLAY_HEIGHT; }
  uint16_t getDisplayWidthBytes() const { return DISPLAY_WIDTH_BYTES; }
  uint32_t getBufferSize() const { return BUFFER_SIZE; }

  // Polarity & Power
  void setInverted(bool inverted) { isInverted_ = inverted; }
  bool toggleInverted() { isInverted_ = !isInverted_; return isInverted_; }
  bool isInverted() const { return isInverted_; }
  void deepSleep() {}

  // Simulator Utilities
  bool isDirty() const { return isDirty_; }
  void present();
  bool saveScreenshotBMP(const char* filepath);
  void setSimulateEinkFlash(bool enable) { simulateEinkFlash = enable; }
  SDL_Window* getWindow() const { return window; }
  SDL_Renderer* getRenderer() const { return renderer; }

 private:
  uint8_t* frameBuffer = nullptr;
  uint8_t* prevBuffer = nullptr;
  uint8_t* lsbBuffer = nullptr;
  uint8_t* msbBuffer = nullptr;
  uint32_t* texturePixels = nullptr;

  SDL_Window* window = nullptr;
  SDL_Renderer* renderer = nullptr;
  SDL_Texture* texture = nullptr;
  std::thread::id mainThreadId;
  mutable std::mutex renderMutex;

  bool isInverted_ = false;
  bool simulateEinkFlash = true;
  std::atomic<bool> isDirty_{false};
  std::atomic<bool> isGrayscale_{false};

  void renderToTexture(bool invert);
  void renderGrayscaleToTexture();
};

extern HalDisplay display;
using SdlHalDisplay = HalDisplay;
