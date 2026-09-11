#include "PowerStats.h"

#include <HalGPIO.h>
#include <HalPowerManager.h>
#include <time.h>

namespace {

// RAM-resident state; persisted through CrossPointState fields at sleep.
uint32_t secondsOnBattery = 0;
uint32_t pagesOnBattery = 0;
uint32_t chargeStartEpoch = 0;   // start of the current on-battery cycle
uint32_t lastChargeEndEpoch = 0; // end of the previous charge
uint32_t lastAccrualEpoch = 0;   // lazy accrual marker (RAM only)
uint8_t lastPct = 0;
uint8_t cyclePeakPct = 0;        // highest level seen in the current cycle
bool onCable = false;
bool cableSeenThisCycle = false;
bool bootCompared = false;

time_t nowEpoch() { return time(nullptr); }

void accrueOnBattery(const time_t now) {
  if (lastAccrualEpoch == 0) {
    lastAccrualEpoch = static_cast<uint32_t>(now);
    return;
  }
  if (now > static_cast<time_t>(lastAccrualEpoch)) {
    const uint32_t delta = static_cast<uint32_t>(now) - lastAccrualEpoch;
    // Cap the fold: a wrong clock jump must not add years of reading time.
    if (delta < 7u * 24u * 3600u) {
      secondsOnBattery = (secondsOnBattery > UINT32_MAX - delta) ? UINT32_MAX : secondsOnBattery + delta;
    }
  }
  lastAccrualEpoch = static_cast<uint32_t>(now);
}

void beginBatteryCycle(const time_t now) {
  chargeStartEpoch = static_cast<uint32_t>(now);
  secondsOnBattery = 0;
  pagesOnBattery = 0;
  cyclePeakPct = lastPct;
  cableSeenThisCycle = false;
  lastAccrualEpoch = static_cast<uint32_t>(now);
}

}  // namespace

void PowerStats::update() {
  const bool usbNow = gpio.isUsbConnected();
  const uint16_t pct = powerManager.getBatteryPercentage();
  const bool pctValid = pct > 0 && pct <= 100;
  if (pctValid) lastPct = static_cast<uint8_t>(pct);
  const time_t now = nowEpoch();

  // One-time comparison against the level persisted before sleep: a higher
  // level at boot means the device charged while it was off.
  if (!bootCompared) {
    bootCompared = true;
    if (chargeStartEpoch == 0 && lastPct > 0) beginBatteryCycle(now);
    if (cyclePeakPct != 0 && lastPct > cyclePeakPct) {
      // Charged (at least partially) while off/asleep: new cycle from now.
      beginBatteryCycle(now);
      lastChargeEndEpoch = chargeStartEpoch;
    }
  }

  if (usbNow) {
    onCable = true;
    cableSeenThisCycle = true;
    if (pctValid && pct > cyclePeakPct) cyclePeakPct = pct;
    lastAccrualEpoch = static_cast<uint32_t>(now);  // freeze battery-time accrual
    return;
  }

  if (onCable) {
    // Cable just left after charging: close the charge, start a new cycle.
    onCable = false;
    lastChargeEndEpoch = static_cast<uint32_t>(now);
    beginBatteryCycle(now);
    return;
  }

  // Inferred charge without an observed cable event (charged while off).
  if (cyclePeakPct != 0 && lastPct > cyclePeakPct) {
    lastChargeEndEpoch = static_cast<uint32_t>(now);
    beginBatteryCycle(now);
    return;
  }

  accrueOnBattery(now);
}

void PowerStats::onPageShown() {
  if (pagesOnBattery < UINT16_MAX) pagesOnBattery++;
}

PowerStats::Snapshot PowerStats::snapshot() {
  update();
  Snapshot snap;
  snap.secondsOnBattery = secondsOnBattery;
  snap.pagesOnBattery = pagesOnBattery;
  snap.chargeStartEpoch = chargeStartEpoch;
  snap.lastChargeEndEpoch = lastChargeEndEpoch;
  snap.batteryPct = lastPct;
  snap.charging = onCable;
  if (chargeStartEpoch != 0 && nowEpoch() > static_cast<time_t>(chargeStartEpoch)) {
    snap.cycleDays = static_cast<uint8_t>((nowEpoch() - static_cast<time_t>(chargeStartEpoch)) / 86400);
  }
  return snap;
}

void PowerStats::loadPersisted(const uint32_t seconds, const uint32_t pages, const uint32_t chargeStart,
                               const uint32_t lastEnd, const uint8_t pct) {
  secondsOnBattery = seconds;
  pagesOnBattery = pages;
  chargeStartEpoch = chargeStart;
  lastChargeEndEpoch = lastEnd;
  lastPct = pct;
  cyclePeakPct = pct;
  lastAccrualEpoch = static_cast<uint32_t>(time(nullptr));
}

uint32_t PowerStats::persistedSeconds() { return secondsOnBattery; }
uint32_t PowerStats::persistedPages() { return pagesOnBattery; }
uint32_t PowerStats::persistedChargeStart() { return chargeStartEpoch; }
uint32_t PowerStats::persistedLastChargeEnd() { return lastChargeEndEpoch; }
uint8_t PowerStats::persistedPct() { return lastPct; }
