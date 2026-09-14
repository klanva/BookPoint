#pragma once

#include <stdint.h>

// Precomputed gamma 1.8 expansion table for E-Ink 4-level displays:
// Lifts shadow details and midtones so portraits, engravings, and dark
// areas do not crush into flat black on reflective e-paper.
inline const uint8_t EINK_GAMMA_TABLE[256] = {
      0,  12,  17,  22,  25,  29,  32,  35,  37,  40,  42,  44,  47,  49,  51,  53,
     55,  57,  58,  60,  62,  64,  65,  67,  69,  70,  72,  73,  75,  76,  78,  79,
     80,  82,  83,  85,  86,  87,  89,  90,  91,  92,  94,  95,  96,  97,  98, 100,
    101, 102, 103, 104, 105, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117,
    118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133,
    134, 135, 136, 137, 138, 139, 139, 140, 141, 142, 143, 144, 145, 146, 146, 147,
    148, 149, 150, 151, 152, 152, 153, 154, 155, 156, 157, 157, 158, 159, 160, 161,
    161, 162, 163, 164, 165, 165, 166, 167, 168, 169, 169, 170, 171, 172, 172, 173,
    174, 175, 175, 176, 177, 178, 178, 179, 180, 181, 181, 182, 183, 183, 184, 185,
    186, 186, 187, 188, 188, 189, 190, 191, 191, 192, 193, 193, 194, 195, 195, 196,
    197, 198, 198, 199, 200, 200, 201, 202, 202, 203, 204, 204, 205, 206, 206, 207,
    208, 208, 209, 209, 210, 211, 211, 212, 213, 213, 214, 215, 215, 216, 217, 217,
    218, 218, 219, 220, 220, 221, 222, 222, 223, 223, 224, 225, 225, 226, 226, 227,
    228, 228, 229, 230, 230, 231, 231, 232, 233, 233, 234, 234, 235, 236, 236, 237,
    237, 238, 238, 239, 240, 240, 241, 241, 242, 243, 243, 244, 244, 245, 245, 246,
    247, 247, 248, 248, 249, 249, 250, 251, 251, 252, 252, 253, 253, 254, 254, 255,
};

// 8x8 Bayer matrix for 64-level smooth ordered dithering:
// Eliminates coarse 4x4 checkerboard artifacts while maintaining zero RAM state.
inline const uint8_t bayer8x8[8][8] = {
    { 0, 32,  8, 40,  2, 34, 10, 42},
    {48, 16, 56, 24, 50, 18, 58, 26},
    {12, 44,  4, 36, 14, 46,  6, 38},
    {60, 28, 52, 20, 62, 30, 54, 22},
    { 3, 35, 11, 43,  1, 33,  9, 41},
    {51, 19, 59, 27, 49, 17, 57, 25},
    {15, 47,  7, 39, 13, 45,  5, 37},
    {63, 31, 55, 23, 61, 29, 53, 21}
};

// Legacy 4x4 fallback definition (for API compatibility)
inline const uint8_t bayer4x4[4][4] = {
    {0, 8, 2, 10},
    {12, 4, 14, 6},
    {3, 11, 1, 9},
    {15, 7, 13, 5},
};

// Apply 64-level 8x8 Bayer dithering with gamma expansion and clean-background bleaching.
// Quantizes to 4 levels (0 = Black, 1 = Dark Gray, 2 = Light Gray, 3 = White).
// Stateless and branch-predictable.
inline uint8_t applyBayerDither4Level(uint8_t gray, int x, int y) {
  // 1. Clean White Bleaching: illustrations on near-white paper backgrounds (>= 242)
  // are forced to pure display white (Level 3) to prevent noisy gray speckles.
  if (gray >= 242) return 3;

  // 2. Clean Black Bleaching: crisp line-art / text stays dense black.
  if (gray <= 10) return 0;

  // 3. Perceptual Gamma 1.8 expansion: lifts shadows so details don't crush.
  uint8_t mappedGray = EINK_GAMMA_TABLE[gray];

  // 4. 8x8 Bayer dithering (64 levels of fine modulation).
  int bayer = bayer8x8[y & 7][x & 7];
  // bayer in [0..63]. Center around 31.5, scale to +/- 42
  int dither = ((bayer - 31) * 85) >> 6;  // (bayer - 31) * 85 / 64

  int adjusted = static_cast<int>(mappedGray) + dither;
  if (adjusted < 0) adjusted = 0;
  if (adjusted > 255) adjusted = 255;

  // 5. Symmetric 4-level quantization thresholds around [0, 85, 170, 255]
  if (adjusted < 43) return 0;
  if (adjusted < 128) return 1;
  if (adjusted < 213) return 2;
  return 3;
}
