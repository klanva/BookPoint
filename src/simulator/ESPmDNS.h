#pragma once

#include <cstdint>
#include <string>

class MDNSResponder {
 public:
  bool begin(const char* hostName) { return true; }
  void end() {}
  void update() {}
  bool addService(const char* service, const char* proto, uint16_t port) { return true; }
  bool addServiceTxt(const char* name, const char* proto, const char* key, const char* value) { return true; }
};

inline MDNSResponder MDNS;
