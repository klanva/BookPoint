#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ESP_PARTITION_TYPE_APP 0x00
#define ESP_PARTITION_TYPE_DATA 0x01
#define ESP_PARTITION_SUBTYPE_DATA_SPIFFS 0x82

typedef struct {
  uint8_t type;
  uint8_t subtype;
  uint32_t address;
  uint32_t size;
  char label[17];
  bool encrypted;
} esp_partition_t;

inline const esp_partition_t* esp_partition_find_first(uint8_t type, uint8_t subtype, const char* label) {
  static esp_partition_t dummyPart = {1, 0x82, 0, 1024 * 1024 * 4, "spiffs", false};
  return &dummyPart;
}

#ifdef __cplusplus
}
#endif
