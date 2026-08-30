#include "ReadingTracker.h"

#include <Arduino.h>

#include "ReadingStatsTypes.h"

void ReadingTracker::begin() {
  began_ = true;
  pageShownAtMs_ = millis();
  sessionSeconds_ = 0;
  sessionPaceSampleSeconds_ = 0;
  sessionPaceSampleCount_ = 0;
  paceWarmupPending_ = true;
  hasSessionStart_ = getCurrentLocalReadingStatsDateTime(sessionStart_);
}

void ReadingTracker::markPageShown() {
  pageShownAtMs_ = millis();
}

uint32_t ReadingTracker::closePage() {
  if (pageShownAtMs_ == 0) return 0;

  const uint32_t dwellSeconds = (millis() - pageShownAtMs_) / 1000;
  pageShownAtMs_ = 0;

  if (dwellSeconds >= IDLE_THRESHOLD_SECONDS) {
    return 0;
  }
  sessionSeconds_ = (sessionSeconds_ > UINT32_MAX - dwellSeconds) ? UINT32_MAX : sessionSeconds_ + dwellSeconds;
  return dwellSeconds;
}

void ReadingTracker::addForwardPaceSample(const uint32_t seconds) {
  if (paceWarmupPending_) {
    paceWarmupPending_ = false;
    return;
  }
  if (seconds < MIN_PACE_SAMPLE_SECONDS || seconds >= IDLE_THRESHOLD_SECONDS) return;

  sessionPaceSampleSeconds_ =
      (sessionPaceSampleSeconds_ > UINT32_MAX - seconds) ? UINT32_MAX : sessionPaceSampleSeconds_ + seconds;
  if (sessionPaceSampleCount_ < UINT16_MAX) sessionPaceSampleCount_++;
}

bool ReadingTracker::sessionPaceAverage(uint16_t& outAvgSeconds) const {
  outAvgSeconds = 0;
  if (sessionPaceSampleCount_ < 3 || sessionPaceSampleSeconds_ == 0) return false;
  const uint32_t avg =
      (sessionPaceSampleSeconds_ + sessionPaceSampleCount_ / 2u) / sessionPaceSampleCount_;
  outAvgSeconds = static_cast<uint16_t>(avg > UINT16_MAX ? UINT16_MAX : avg);
  return outAvgSeconds > 0;
}

ReadingTracker::Commit ReadingTracker::endSession() {
  Commit commit;
  if (began_) {
    closePage();
  }
  commit.seconds = sessionSeconds_;
  commit.countSession = commit.seconds >= MIN_SESSION_SECONDS_FOR_COUNT;
  commit.countTime = commit.seconds >= MIN_SESSION_SECONDS_FOR_TIME;
  if (hasSessionStart_) {
    commit.start = sessionStart_;
    commit.hasStart = true;
    // Attribute the session to the bucket of its start time; long sessions
    // span buckets in recordReadingSpan().
  }
  sessionSeconds_ = 0;
  began_ = false;
  pageShownAtMs_ = 0;
  return commit;
}
