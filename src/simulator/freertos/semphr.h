#pragma once

#include "FreeRTOS.h"
#include <mutex>
#include <atomic>

struct SimSemaphoreImpl {
  std::recursive_mutex mtx;
  std::atomic<bool> isLocked{false};
};

using SemaphoreHandle_t = SimSemaphoreImpl*;

inline SemaphoreHandle_t xSemaphoreCreateMutex() {
  return new SimSemaphoreImpl();
}

inline SemaphoreHandle_t xSemaphoreCreateBinary() {
  return new SimSemaphoreImpl();
}

inline void vSemaphoreDelete(SemaphoreHandle_t sem) {
  delete sem;
}

inline BaseType_t xSemaphoreTake(SemaphoreHandle_t sem, TickType_t xTicksToWait) {
  if (!sem) return pdFAIL;
  if (xTicksToWait == 0) {
    bool ok = sem->mtx.try_lock();
    if (ok) sem->isLocked = true;
    return ok ? pdTRUE : pdFAIL;
  }
  sem->mtx.lock();
  sem->isLocked = true;
  return pdTRUE;
}

inline BaseType_t xSemaphoreGive(SemaphoreHandle_t sem) {
  if (!sem) return pdFAIL;
  sem->isLocked = false;
  sem->mtx.unlock();
  return pdTRUE;
}

inline BaseType_t xQueuePeek(SemaphoreHandle_t sem, void* pvBuffer, TickType_t xTicksToWait) {
  if (!sem) return pdFAIL;
  return sem->isLocked ? pdFALSE : pdTRUE;
}

inline TaskHandle_t xSemaphoreGetMutexHolder(SemaphoreHandle_t) {
  return nullptr;
}
