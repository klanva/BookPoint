#pragma once
#include <string>
#include <functional>
#include <cstdint>

namespace freeink {
class SecureHttpClient {
public:
  using DataCallback = std::function<bool(const uint8_t* data, size_t len)>;
  using AbortCallback = std::function<bool()>;
  using ProgressCallback = std::function<bool(size_t downloaded, size_t total)>;

  SecureHttpClient() = default;
  void setInsecure() {}
  void setReuse(bool) {}
  void setTimeout(uint32_t) {}
  bool begin(const std::string&) { return false; }
  void addHeader(const std::string&, const std::string&) {}
  int GET() { return -1; }
  int POST(const std::string&) { return -1; }
  int PUT(const std::string&) { return -1; }
  int sendRequest(const char* type, const std::string& payload = "") { (void)type; (void)payload; return -1; }
  int sendRequest(const std::string& type, const std::string& payload = "") { (void)type; (void)payload; return -1; }
  const std::string& getString() const { static std::string empty; return empty; }
  void end() {}
};
} // namespace freeink

using freeink::SecureHttpClient;
