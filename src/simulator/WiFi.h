#pragma once

#include <string>
#include <cstdint>
#include "WString.h"

typedef uint8_t wl_status_t;

#define WL_IDLE_STATUS 0
#define WL_NO_SSID_AVAIL 1
#define WL_SCAN_COMPLETED 2
#define WL_CONNECTED 3
#define WL_CONNECT_FAILED 4
#define WL_CONNECTION_LOST 5
#define WL_DISCONNECTED 6

class IPAddress {
 public:
  uint8_t bytes[4]{0, 0, 0, 0};
  IPAddress() = default;
  IPAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    bytes[0] = a; bytes[1] = b; bytes[2] = c; bytes[3] = d;
  }
  bool operator==(const IPAddress& other) const {
    return bytes[0] == other.bytes[0] && bytes[1] == other.bytes[1] &&
           bytes[2] == other.bytes[2] && bytes[3] == other.bytes[3];
  }
  bool operator!=(const IPAddress& other) const {
    return !(*this == other);
  }
  std::string toString() const {
    return std::to_string(bytes[0]) + "." + std::to_string(bytes[1]) + "." +
           std::to_string(bytes[2]) + "." + std::to_string(bytes[3]);
  }
};

enum wifi_mode_t {
  WIFI_MODE_NULL = 0,
  WIFI_STA = 1,
  WIFI_AP = 2,
  WIFI_AP_STA = 3,
  WIFI_MODE_STA = 1,
  WIFI_MODE_AP = 2,
  WIFI_MODE_APSTA = 3,
  WIFI_OFF = 0
};
typedef wifi_mode_t WiFiMode;

enum wifi_auth_mode_t {
  WIFI_AUTH_OPEN = 0,
  WIFI_AUTH_WEP,
  WIFI_AUTH_WPA_PSK,
  WIFI_AUTH_WPA2_PSK,
  WIFI_AUTH_WPA_WPA2_PSK,
  WIFI_AUTH_WPA2_ENTERPRISE,
  WIFI_AUTH_WPA3_PSK,
  WIFI_AUTH_WPA2_WPA3_PSK,
  WIFI_AUTH_WAPI_PSK,
  WIFI_AUTH_MAX
};

enum wifi_scan_method_t {
  WIFI_FAST_SCAN = 0,
  WIFI_ALL_CHANNEL_SCAN = 1
};

enum wifi_sort_method_t {
  WIFI_CONNECT_AP_BY_SIGNAL = 0,
  WIFI_CONNECT_AP_BY_SECURITY = 1
};

class WiFiClass {
 private:
  wifi_mode_t currentMode = WIFI_MODE_NULL;

 public:
  wifi_mode_t getMode() const { return currentMode; }
  void mode(wifi_mode_t m) { currentMode = m; }
  void disconnect(bool wifioff = false, bool eraseap = false) {}
  bool softAPdisconnect(bool wifioff = false) { return true; }
  bool isConnected() const { return false; }
  wl_status_t status() const { return WL_DISCONNECTED; }
  IPAddress localIP() const { return IPAddress(0, 0, 0, 0); }
  IPAddress softAPIP() const { return IPAddress(192, 168, 4, 1); }
  int RSSI() const { return -60; }
  int32_t RSSI(uint8_t index) const { return -60; }
  String SSID() const { return String(""); }
  String SSID(uint8_t index) const { return String(""); }
  wifi_auth_mode_t encryptionType(uint8_t index) const { return WIFI_AUTH_WPA2_PSK; }
  uint8_t* BSSID(uint8_t* bssid) {
    if (bssid) {
      bssid[0] = 0x00; bssid[1] = 0x11; bssid[2] = 0x22;
      bssid[3] = 0x33; bssid[4] = 0x44; bssid[5] = 0x55;
    }
    return bssid;
  }
  int32_t channel() const { return 1; }
  String macAddress() const { return String("00:11:22:33:44:55"); }

  int16_t scanNetworks(bool async = false, bool show_hidden = false, bool passive = false, uint32_t max_ms_per_chan = 300) {
    return 0;
  }
  int16_t scanComplete() { return 0; }
  void scanDelete() {}

  void persistent(bool) {}
  void setScanMethod(wifi_scan_method_t) {}
  void setSortMethod(wifi_sort_method_t) {}
  bool setHostname(const char*) { return true; }

  wl_status_t begin(const char* ssid, const char* passphrase = nullptr, int32_t channel = 0,
                    const uint8_t* bssid = nullptr, bool connect = true) {
    return WL_DISCONNECTED;
  }
  bool softAP(const char* ssid, const char* passphrase = nullptr, int channel = 1,
              int ssid_hidden = 0, int max_connection = 4) {
    return true;
  }
};

inline WiFiClass WiFi;
