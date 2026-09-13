#pragma once

#include <cstdint>
#include <cstddef>

using TickType_t = uint32_t;
using BaseType_t = int;
using UBaseType_t = unsigned int;

struct SimTaskImpl;
using TaskHandle_t = SimTaskImpl*;

#ifndef pdTRUE
#define pdTRUE 1
#endif

#ifndef pdFALSE
#define pdFALSE 0
#endif

#ifndef pdPASS
#define pdPASS 1
#endif

#ifndef pdFAIL
#define pdFAIL 0
#endif

#ifndef portMAX_DELAY
#define portMAX_DELAY 0xFFFFFFFFUL
#endif

#ifndef pdMS_TO_TICKS
#define pdMS_TO_TICKS(ms) (static_cast<TickType_t>(ms))
#endif

using portMUX_TYPE = int;
#define portMUX_INITIALIZER_UNLOCKED 0
#define taskENTER_CRITICAL(mux) do {} while (0)
#define taskEXIT_CRITICAL(mux) do {} while (0)
#define taskENTER_CRITICAL_ISR(mux) do {} while (0)
#define taskEXIT_CRITICAL_ISR(mux) do {} while (0)
