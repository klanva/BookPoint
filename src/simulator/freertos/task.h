#pragma once

#include "FreeRTOS.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

enum eNotifyAction {
  eNoAction,
  eSetBits,
  eIncrement,
  eSetValueWithOverwrite,
  eSetValueWithoutOverwrite
};

struct SimTaskImpl {
  void (*code)(void*) = nullptr;
  void* param = nullptr;
  std::thread thr;
  std::mutex mtx;
  std::condition_variable cv;
  uint32_t notifyValue = 0;
};

using TaskHandle_t = SimTaskImpl*;
using TaskFunction_t = void (*)(void*);

inline thread_local SimTaskImpl* s_currentSimTask = nullptr;

inline TaskHandle_t xTaskGetCurrentTaskHandle() {
  if (!s_currentSimTask) {
    static SimTaskImpl mainSimTask;
    s_currentSimTask = &mainSimTask;
  }
  return s_currentSimTask;
}

inline BaseType_t xTaskCreatePinnedToCore(
    TaskFunction_t pxTaskCode,
    const char* const pcName,
    const uint32_t usStackDepth,
    void* const pvParameters,
    UBaseType_t uxPriority,
    TaskHandle_t* const pxCreatedTask,
    const BaseType_t xCoreID) {
  auto* task = new SimTaskImpl();
  task->code = pxTaskCode;
  task->param = pvParameters;
  task->thr = std::thread([task]() {
    s_currentSimTask = task;
    task->code(task->param);
  });
  task->thr.detach();
  if (pxCreatedTask) {
    *pxCreatedTask = task;
  }
  return pdPASS;
}

inline uint32_t ulTaskNotifyTake(BaseType_t xClearCountOnExit, TickType_t xTicksToWait) {
  SimTaskImpl* task = xTaskGetCurrentTaskHandle();
  if (!task) return 0;
  std::unique_lock<std::mutex> lock(task->mtx);
  if (xTicksToWait == portMAX_DELAY) {
    task->cv.wait(lock, [task]() { return task->notifyValue > 0; });
  } else {
    task->cv.wait_for(lock, std::chrono::milliseconds(xTicksToWait), [task]() { return task->notifyValue > 0; });
  }
  uint32_t val = task->notifyValue;
  if (xClearCountOnExit) {
    task->notifyValue = 0;
  } else if (val > 0) {
    task->notifyValue--;
  }
  return val;
}

inline BaseType_t xTaskNotify(TaskHandle_t xTaskToNotify, uint32_t ulValue, eNotifyAction eAction) {
  if (!xTaskToNotify) return pdFAIL;
  {
    std::lock_guard<std::mutex> lock(xTaskToNotify->mtx);
    if (eAction == eIncrement) {
      xTaskToNotify->notifyValue++;
    } else if (eAction == eSetValueWithOverwrite) {
      xTaskToNotify->notifyValue = ulValue;
    }
  }
  xTaskToNotify->cv.notify_one();
  return pdPASS;
}

inline void vTaskDelay(const TickType_t xTicksToDelay) {
  std::this_thread::sleep_for(std::chrono::milliseconds(xTicksToDelay));
}
