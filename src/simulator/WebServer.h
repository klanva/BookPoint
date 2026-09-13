#pragma once

#include <string>
#include <functional>

class WebServer {
 public:
  explicit WebServer(uint16_t) {}
  void begin() {}
  void stop() {}
  void handleClient() {}
  void on(const char*, std::function<void()>) {}
  void onNotFound(std::function<void()>) {}
  void send(int, const char*, const std::string&) {}
  bool hasArg(const char*) const { return false; }
  std::string arg(const char*) const { return ""; }
};
