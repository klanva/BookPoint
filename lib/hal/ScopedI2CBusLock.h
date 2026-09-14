#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#ifdef __cplusplus
extern "C" {
#endif

SemaphoreHandle_t getSharedI2cBusMutex();

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/**
 * @brief Thread-safe recursive mutex lock for the shared I2C bus on Xteink X4 Pro.
 * 
 * Synchronizes hardware access across:
 * - Goodix GT911 touch controller (I2C addr 0x5D)
 * - Belling BM8563 RTC (I2C addr 0x51)
 * - CellWise CW2017 fuel gauge (I2C addr 0x63)
 */
class ScopedI2CBusLock {
  bool _locked = false;

 public:
  explicit ScopedI2CBusLock(TickType_t timeoutTicks = portMAX_DELAY) {
    SemaphoreHandle_t m = getSharedI2cBusMutex();
    if (m != nullptr) {
      if (xSemaphoreTakeRecursive(m, timeoutTicks) == pdTRUE) {
        _locked = true;
      }
    }
  }

  ~ScopedI2CBusLock() {
    unlock();
  }

  ScopedI2CBusLock(const ScopedI2CBusLock&) = delete;
  ScopedI2CBusLock& operator=(const ScopedI2CBusLock&) = delete;

  ScopedI2CBusLock(ScopedI2CBusLock&& other) noexcept : _locked(other._locked) {
    other._locked = false;
  }

  ScopedI2CBusLock& operator=(ScopedI2CBusLock&& other) noexcept {
    if (this != &other) {
      unlock();
      _locked = other._locked;
      other._locked = false;
    }
    return *this;
  }

  bool isLocked() const { return _locked; }

  void unlock() {
    if (_locked) {
      SemaphoreHandle_t m = getSharedI2cBusMutex();
      if (m != nullptr) {
        xSemaphoreGiveRecursive(m);
      }
      _locked = false;
    }
  }
};
#endif
