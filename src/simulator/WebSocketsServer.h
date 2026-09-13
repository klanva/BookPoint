#pragma once

#include <cstdint>
#include <cstddef>
#include <functional>

enum WStype_t {
  WStype_DISCONNECTED,
  WStype_CONNECTED,
  WStype_TEXT,
  WStype_BIN,
  WStype_FRAGMENT_TEXT_START,
  WStype_FRAGMENT_BIN_START,
  WStype_FRAGMENT,
  WStype_FRAGMENT_FIN
};

class WebSocketsServer {
 public:
  explicit WebSocketsServer(uint16_t) {}
  void begin() {}
  void close() {}
  void loop() {}
  void onEvent(std::function<void(uint8_t, WStype_t, uint8_t*, size_t)>) {}
  void broadcastTXT(const char*) {}
  void disconnect() {}
};
