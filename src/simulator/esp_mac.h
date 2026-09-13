#pragma once

#include <stdint.h>
#include <esp_err.h>

typedef enum {
  ESP_MAC_WIFI_STA,
  ESP_MAC_WIFI_SOFTAP,
  ESP_MAC_BT,
  ESP_MAC_ETH,
  ESP_MAC_IEEE802154,
} esp_mac_type_t;

inline esp_err_t esp_read_mac(uint8_t* mac, esp_mac_type_t type) {
  if (mac) {
    mac[0] = 0x00;
    mac[1] = 0x11;
    mac[2] = 0x22;
    mac[3] = 0x33;
    mac[4] = 0x44;
    mac[5] = 0x55;
  }
  return ESP_OK;
}

inline esp_err_t esp_efuse_mac_get_default(uint8_t* mac) {
  if (mac) {
    mac[0] = 0xAA;
    mac[1] = 0xBB;
    mac[2] = 0xCC;
    mac[3] = 0xDD;
    mac[4] = 0xEE;
    mac[5] = 0xFF;
  }
  return ESP_OK;
}
