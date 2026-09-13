#include "SdlHalDisplay.h"
#include <iostream>
#include <fstream>
#include <algorithm>

HalDisplay display;

HalDisplay::HalDisplay() {
  frameBuffer = new uint8_t[BUFFER_SIZE];
  prevBuffer  = new uint8_t[BUFFER_SIZE];
  lsbBuffer   = new uint8_t[BUFFER_SIZE];
  msbBuffer   = new uint8_t[BUFFER_SIZE];
  texturePixels = new uint32_t[LOGICAL_WIDTH * LOGICAL_HEIGHT];

  std::memset(frameBuffer, 0xFF, BUFFER_SIZE);
  std::memset(prevBuffer,  0xFF, BUFFER_SIZE);
  std::memset(lsbBuffer,   0x00, BUFFER_SIZE);
  std::memset(msbBuffer,   0x00, BUFFER_SIZE);
  for (int i = 0; i < LOGICAL_WIDTH * LOGICAL_HEIGHT; ++i) {
    texturePixels[i] = COLOR_PAPER_WHITE;
  }
}

HalDisplay::~HalDisplay() {
  if (texture) SDL_DestroyTexture(texture);
  if (renderer) SDL_DestroyRenderer(renderer);
  if (window) SDL_DestroyWindow(window);
  SDL_Quit();

  delete[] frameBuffer;
  delete[] prevBuffer;
  delete[] lsbBuffer;
  delete[] msbBuffer;
  delete[] texturePixels;
}

void HalDisplay::begin(bool seamless) {
  mainThreadId = std::this_thread::get_id();

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    std::cerr << "[SdlHalDisplay] SDL_Init failed: " << SDL_GetError() << std::endl;
    return;
  }

  window = SDL_CreateWindow(
      "BookPoint 2.0.0 — Native E-Ink Simulator (480x800)",
      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      LOGICAL_WIDTH, LOGICAL_HEIGHT,
      SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
  );

  if (!window) {
    std::cerr << "[SdlHalDisplay] SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
    return;
  }

  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!renderer) {
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
  }

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0"); // Nearest neighbor for crisp 1bpp
  SDL_RenderSetLogicalSize(renderer, LOGICAL_WIDTH, LOGICAL_HEIGHT);

  texture = SDL_CreateTexture(
      renderer,
      SDL_PIXELFORMAT_RGBA8888,
      SDL_TEXTUREACCESS_STREAMING,
      LOGICAL_WIDTH, LOGICAL_HEIGHT
  );

  if (!seamless) {
    clearScreen(0xFF);
    renderToTexture(false);
    present();
  }
}

void HalDisplay::clearScreen(uint8_t color) const {
  std::memset(frameBuffer, color, BUFFER_SIZE);
}

uint8_t* HalDisplay::lendFrameBufferStorage(uint32_t* sizeOut) {
  if (sizeOut) *sizeOut = BUFFER_SIZE;
  return frameBuffer;
}

void HalDisplay::returnFrameBufferStorage() {
  std::memset(frameBuffer, 0xFF, BUFFER_SIZE);
}

void HalDisplay::renderToTexture(bool invert) {
  for (int outY = 0; outY < LOGICAL_HEIGHT; ++outY) {
    for (int outX = 0; outX < LOGICAL_WIDTH; ++outX) {
      const int inX = outY;
      const int inY = DISPLAY_HEIGHT - 1 - outX;
      const int byteIdx = inY * DISPLAY_WIDTH_BYTES + (inX / 8);
      const int bitPos = 7 - (inX % 8);
      bool isWhite = (frameBuffer[byteIdx] >> bitPos) & 1;

      if (isInverted_ ^ invert) isWhite = !isWhite;

      texturePixels[outY * LOGICAL_WIDTH + outX] = isWhite ? COLOR_PAPER_WHITE : COLOR_INK_BLACK;
    }
  }
}

void HalDisplay::renderGrayscaleToTexture() {
  for (int outY = 0; outY < LOGICAL_HEIGHT; ++outY) {
    for (int outX = 0; outX < LOGICAL_WIDTH; ++outX) {
      const int inX = outY;
      const int inY = DISPLAY_HEIGHT - 1 - outX;
      const int byteIdx = inY * DISPLAY_WIDTH_BYTES + (inX / 8);
      const int bitPos = 7 - (inX % 8);

      const bool bw  = (frameBuffer[byteIdx] >> bitPos) & 1;
      const bool lsb = (lsbBuffer[byteIdx]   >> bitPos) & 1;
      const bool msb = (msbBuffer[byteIdx]   >> bitPos) & 1;

      uint32_t color;
      if (!msb && !lsb) {
        color = bw ? COLOR_PAPER_WHITE : COLOR_INK_BLACK;
      } else if (msb && lsb) {
        color = COLOR_DARK_GRAY;
      } else if (msb && !lsb) {
        color = COLOR_LIGHT_GRAY;
      } else {
        color = COLOR_DARK_GRAY;
      }

      texturePixels[outY * LOGICAL_WIDTH + outX] = color;
    }
  }
}

void HalDisplay::displayBuffer(RefreshMode mode, bool turnOffScreen) {
  (void)turnOffScreen;
  isGrayscale_ = false;

  if (mode == FULL_REFRESH && simulateEinkFlash && renderer && (std::this_thread::get_id() == mainThreadId)) {
    // E-ink flash simulation on full refresh
    renderToTexture(true); // invert
    present();
    SDL_Delay(50);

    SDL_SetRenderDrawColor(renderer, 245, 243, 238, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
    SDL_Delay(35);
  }

  renderToTexture(false);
  std::memcpy(prevBuffer, frameBuffer, BUFFER_SIZE);
  isDirty_ = true;

  if (std::this_thread::get_id() == mainThreadId) {
    present();
  }
}

void HalDisplay::displayBufferAsync(RefreshMode mode) {
  displayBuffer(mode, false);
}

void HalDisplay::refreshDisplay(RefreshMode mode, bool turnOffScreen) {
  displayBuffer(mode, turnOffScreen);
}

void HalDisplay::copyGrayscaleBuffers(const uint8_t* lsb, const uint8_t* msb) {
  if (lsb) std::memcpy(lsbBuffer, lsb, BUFFER_SIZE);
  if (msb) std::memcpy(msbBuffer, msb, BUFFER_SIZE);
}

void HalDisplay::copyGrayscaleLsbBuffers(const uint8_t* lsb) {
  if (lsb) std::memcpy(lsbBuffer, lsb, BUFFER_SIZE);
}

void HalDisplay::copyGrayscaleMsbBuffers(const uint8_t* msb) {
  if (msb) std::memcpy(msbBuffer, msb, BUFFER_SIZE);
}

void HalDisplay::writeGrayscalePlaneStrip(bool lsbPlane, const uint8_t* rows, uint16_t yStart, uint16_t numRows) {
  uint8_t* dst = lsbPlane ? lsbBuffer : msbBuffer;
  std::memcpy(dst + (yStart * DISPLAY_WIDTH_BYTES), rows, numRows * DISPLAY_WIDTH_BYTES);
}

void HalDisplay::cleanupGrayscaleBuffers(const uint8_t* bwBuffer) {
  if (bwBuffer) std::memcpy(frameBuffer, bwBuffer, BUFFER_SIZE);
}

void HalDisplay::displayGrayscaleBase(RefreshMode fallback, bool turnOffScreen) {
  displayBuffer(fallback, turnOffScreen);
}

void HalDisplay::displayGrayBuffer(bool turnOffScreen) {
  (void)turnOffScreen;
  isGrayscale_ = true;
  renderGrayscaleToTexture();
  std::memcpy(prevBuffer, frameBuffer, BUFFER_SIZE);
  isDirty_ = true;

  if (std::this_thread::get_id() == mainThreadId) {
    present();
  }
}

void HalDisplay::present() {
  std::lock_guard<std::mutex> lock(renderMutex);
  if (!renderer || !texture) return;

  SDL_UpdateTexture(texture, nullptr, texturePixels, LOGICAL_WIDTH * sizeof(uint32_t));
  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, texture, nullptr, nullptr);
  SDL_RenderPresent(renderer);
  isDirty_ = false;
}

bool HalDisplay::saveScreenshotBMP(const char* filepath) {
  std::lock_guard<std::mutex> lock(renderMutex);
  SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(
      0, LOGICAL_WIDTH, LOGICAL_HEIGHT, 32, SDL_PIXELFORMAT_RGBA8888
  );
  if (!surface) return false;

  std::memcpy(surface->pixels, texturePixels, LOGICAL_WIDTH * LOGICAL_HEIGHT * sizeof(uint32_t));
  int res = SDL_SaveBMP(surface, filepath);
  SDL_FreeSurface(surface);
  return res == 0;
}

void HalDisplay::drawImage(const uint8_t* imageData, uint16_t x, uint16_t y, uint16_t w, uint16_t h, bool fromProgmem) {
  (void)imageData; (void)x; (void)y; (void)w; (void)h; (void)fromProgmem;
}

void HalDisplay::drawImageTransparent(const uint8_t* imageData, uint16_t x, uint16_t y, uint16_t w, uint16_t h, bool fromProgmem) {
  (void)imageData; (void)x; (void)y; (void)w; (void)h; (void)fromProgmem;
}

