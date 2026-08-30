#include "ReaderFontSizes.h"

#include <iterator>

std::vector<uint8_t> readerFontPointSizes(const SdCardFontRegistry* registry, const char* sdFamilyName,
                                          const bool builtinSans) {
  if (registry && sdFamilyName && sdFamilyName[0] != '\0') {
    if (const auto* family = registry->findFamily(sdFamilyName)) {
      auto sizes = family->availableSizes();
      if (!sizes.empty()) return sizes;
    }
  }
  const uint8_t* builtin = builtinReaderPointSizes(builtinSans);
  const size_t count = builtinReaderPointSizeCount(builtinSans);
  return {builtin, builtin + count};
}

uint8_t snapToNearestPointSize(const uint8_t* sizes, const size_t count, const uint8_t pt) {
  if (!sizes || count == 0) return pt;

  uint8_t best = sizes[0];
  uint8_t bestDelta = best > pt ? best - pt : pt - best;
  for (size_t i = 1; i < count; i++) {
    const uint8_t delta = sizes[i] > pt ? sizes[i] - pt : pt - sizes[i];
    // Strictly-less keeps the smaller size on a tie, since `sizes` is ascending.
    if (delta < bestDelta) {
      best = sizes[i];
      bestDelta = delta;
    }
  }
  return best;
}
