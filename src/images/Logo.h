#pragma once

#include <cstdint>
#include "images/Logo120.h"
#include "images/Logo200.h"
#include "images/Logo240.h"

struct LogoAsset {
  const uint8_t* data;
  int size;
};

/**
 * Returns the best fitting logo asset based on available width and height.
 */
inline LogoAsset getAdaptiveLogo(int maxW, int maxH) {
  if (maxW >= 240 && maxH >= 240) {
    return {Logo240, 240};
  } else if (maxW >= 200 && maxH >= 200) {
    return {Logo200, 200};
  } else {
    return {Logo120, 120};
  }
}
