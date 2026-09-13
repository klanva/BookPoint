#pragma once

#include "esp_err.h"

inline esp_err_t esp_task_wdt_status(void*) { return -1; }
inline esp_err_t esp_task_wdt_reset() { return ESP_OK; }
