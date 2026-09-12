#pragma once

#include <Arduino.h>
#include <BatteryMonitor.h>
#include <InputManager.h>
#include <Logging.h>
#include <freertos/semphr.h>

#include <cassert>

#include "HalGPIO.h"

class HalPowerManager;
extern HalPowerManager powerManager;  // Singleton

class HalPowerManager {
  int normalFreq = 0;  // MHz
  bool isLowPower = false;

  mutable int _batteryCachedPercent = 0;         // Last read battery percentage (0-100)
  mutable unsigned long _batteryLastPollMs = 0;  // Timestamp of last battery read in milliseconds

  enum LockMode { None, NormalSpeed };
  LockMode currentLockMode = None;
  SemaphoreHandle_t modeMutex = nullptr;  // Protect access to currentLockMode

 public:
#if BOARD_HAS_PSRAM
  static constexpr int LOW_POWER_FREQ = 80;  // MHz
#else
  static constexpr int LOW_POWER_FREQ = 10;  // MHz
#endif
  static constexpr unsigned long IDLE_POWER_SAVING_MS = 3000;  // ms
  static constexpr unsigned long BATTERY_POLL_MS = 1500;       // ms
  // Idle-light-sleep thresholds (witchhunt race-to-sleep): after this much
  // inactivity the idle loop stops busy-delaying and sleeps in slices.
  static constexpr unsigned long IDLE_LIGHT_SLEEP_MS = 1000;  // ms
  static constexpr unsigned long LIGHT_SLEEP_SLICE_MS = 50;   // one slice

  void begin();

  // Control CPU frequency for power saving
  void setPowerSaving(bool enabled);

  // Sleep the CPU for one slice between input polls (light sleep, RAM and
  // peripherals retained). X4 Pro only: requires digital buttons as true GPIO
  // wake sources and known rail hold levels. The caller must not hold a Lock,
  // have Wi-Fi up or USB connected — tryLightSleepSlice re-checks those (plus
  // mid-debounce input) itself and declines (returns false) if any is active.
  // A lit frontlight is fine on builds with FREEINK_FRONTLIGHT_LS: the KEEP_ALIVE
  // channels keep the PWM running through light sleep. millis() stays
  // wall-clock correct; the FreeRTOS tick is stepped forward so blocked tasks
  // come due on time.
  bool tryLightSleepSlice(const HalGPIO& gpio);

  // Setup wake up GPIO and enter deep sleep
  // Should be called inside main loop() to handle the currentLockMode
  void startDeepSleep(HalGPIO& gpio) const;

  // Get battery percentage (range 0-100)
  uint16_t getBatteryPercentage() const;

  // Get battery voltage in millivolts
  uint16_t getBatteryVoltageMv() const;

  // RAII helper class to manage power saving locks
  // Usage: create an instance of Lock in a scope to disable power saving, for example when running a task that needs
  // full performance. When the Lock instance is destroyed (goes out of scope), power saving will be re-enabled.
  class Lock {
    friend class HalPowerManager;
    bool valid = false;

   public:
    explicit Lock();
    ~Lock();

    // Non-copyable and non-movable
    Lock(const Lock&) = delete;
    Lock& operator=(const Lock&) = delete;
    Lock(Lock&&) = delete;
    Lock& operator=(Lock&&) = delete;
  };
};
