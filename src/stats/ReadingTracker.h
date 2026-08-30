#pragma once

#include <cstdint>

#include "ReadingStatsTypes.h"

// Tracks one reading session: how long the user actively looked at pages,
// their forward-page pace, and when the session started.
//
// The reader calls markPageShown() whenever a fresh page lands on screen and
// closePage() when the reader leaves it (page turn, menu, footnote jump,
// exit). Dwell longer than the idle threshold is treated as "walked away /
// fell asleep" and contributes nothing — which is also what keeps a sleep
// gap from inflating the numbers.
class ReadingTracker {
 public:
  static constexpr uint16_t IDLE_THRESHOLD_SECONDS = 5 * 60;
  static constexpr uint16_t MIN_PACE_SAMPLE_SECONDS = 2;
  static constexpr uint32_t MIN_SESSION_SECONDS_FOR_COUNT = 60;
  static constexpr uint32_t MIN_SESSION_SECONDS_FOR_TIME = 10;

  void begin();

  // Start/stop the dwell clock for the currently visible page.
  void markPageShown();
  // Returns the validated dwell seconds (0 when rejected as idle).
  uint32_t closePage();

  // Fold a forward-page dwell into the session pace. The first sample of a
  // session is discarded: it typically includes the time spent getting into
  // the book, not reading pace.
  void addForwardPaceSample(uint32_t seconds);

  uint32_t sessionSeconds() const { return sessionSeconds_; }

  bool sessionPaceAverage(uint16_t& outAvgSeconds) const;

  struct Commit {
    bool countSession = false;  // session >= 60 s
    bool countTime = false;     // session >= 10 s
    uint32_t seconds = 0;
    ReadingStatsDateTime start;
    bool hasStart = false;
  };

  // Closes the session and produces the values to fold into the stores.
  Commit endSession();

 private:
  unsigned long pageShownAtMs_ = 0;
  uint32_t sessionSeconds_ = 0;
  uint32_t sessionPaceSampleSeconds_ = 0;
  uint16_t sessionPaceSampleCount_ = 0;
  bool paceWarmupPending_ = true;
  bool began_ = false;
  ReadingStatsDateTime sessionStart_;
  bool hasSessionStart_ = false;
};
