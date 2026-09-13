#pragma once

#include <cstdint>
#include "IPAddress.h"

enum class DNSReplyCode {
  NoError = 0,
  FormErr = 1,
  ServFail = 2,
  NXDomain = 3,
  NotImp = 4,
  Refused = 5
};

class DNSServer {
 public:
  void setErrorReplyCode(DNSReplyCode code) {}
  bool start(uint16_t port, const char* domainName, const IPAddress& resolvedIP) { return true; }
  void processNextRequest() {}
  void stop() {}
};
