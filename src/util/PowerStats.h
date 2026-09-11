#pragma once

#include <cstdint>

// Battery autonomy tracker, ported in spirit from YACP PowerHistory.
//
// ZERO periodic wakeups: time-on-battery is accrued lazily — any reader of the
// stats first folds (now - lastAccrualEpoch) into the counters and moves the
// marker forward. The battery percentage is sampled exactly once per deep
// sleep entry (reuse of the existing state save), never on a timer.
//
// Charge cycle: starts when charging is observed (USB present) or inferred
// (percentage grew >= 2 without a cable event); ends when the cable is gone
// and the level drops. Counters (time, pages, days) refer to the current or
// last completed cycle.
namespace PowerStats {

struct Snapshot {
  uint32_t secondsOnBattery = 0;   // active time of the current/last cycle
  uint32_t pagesOnBattery = 0;     // pages shown since last charge
  uint32_t chargeStartEpoch = 0;   // when the current cycle began (0 = unknown)
  uint32_t lastChargeEndEpoch = 0; // end of the previous cycle
  uint8_t batteryPct = 0;          // last sampled level
  uint8_t cycleDays = 0;           // whole days since the cycle began
  bool charging = false;
};

// Fold lazy time and refresh charge state. Call before persisting state
// (sleep path) and before opening the autonomy screen.
void update();

// RAM-only page counter; called from the reader's page-render path.
void onPageShown();

// Current values (after update()).
Snapshot snapshot();

// Persistence hooks: APP_STATE JSON fields (epoch + counters) survive sleep
// and reboot; RAM accrual resumes from the stored marker at boot.
void loadPersisted(uint32_t secondsOnBattery, uint32_t pagesOnBattery, uint32_t chargeStartEpoch,
                   uint32_t lastChargeEndEpoch, uint8_t batteryPct);
uint32_t persistedSeconds();
uint32_t persistedPages();
uint32_t persistedChargeStart();
uint32_t persistedLastChargeEnd();
uint8_t persistedPct();

}  // namespace PowerStats
