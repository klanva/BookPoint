#pragma once

#include <Arduino.h>

// Global "last Wi-Fi activity" timestamp. Touched by the web server request
// pump and every HTTP(S) download/fetch (OPDS, KOReader sync, OTA, file
// transfers). Network activities watch the idle time and shut the radio down
// when nothing happens for SETTINGS.wifiAutoOffMinutes — the radio is by far
// the biggest battery consumer, so an idle connection must not outlive its
// usefulness.
namespace WifiActivity {

inline unsigned long lastActivityMs = 0;

inline void touch() { lastActivityMs = millis(); }

inline bool idleExceeded(const unsigned long limitSeconds) {
  return lastActivityMs != 0 && limitSeconds != 0 && (millis() - lastActivityMs) > limitSeconds * 1000UL;
}

}  // namespace WifiActivity
