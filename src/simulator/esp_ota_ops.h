#pragma once

#include "esp_partition.h"

#ifdef __cplusplus
extern "C" {
#endif

inline const esp_partition_t* esp_ota_get_running_partition(void) {
  static esp_partition_t runningPart = {0, 0x10, 0x10000, 1024 * 1024 * 4, "app0", false};
  return &runningPart;
}

#ifdef __cplusplus
}
#endif
