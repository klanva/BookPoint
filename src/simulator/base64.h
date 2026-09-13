#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include "WString.h"

namespace base64 {

inline String encode(const uint8_t* data, size_t length) {
  static const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  String out;
  if (!data || length == 0) return out;
  out.reserve(((length + 2) / 3) * 4);
  for (size_t i = 0; i < length; i += 3) {
    uint32_t val = (static_cast<uint32_t>(data[i])) << 16;
    if (i + 1 < length) val |= (static_cast<uint32_t>(data[i + 1])) << 8;
    if (i + 2 < length) val |= (static_cast<uint32_t>(data[i + 2]));

    out.push_back(table[(val >> 18) & 0x3F]);
    out.push_back(table[(val >> 12) & 0x3F]);
    out.push_back((i + 1 < length) ? table[(val >> 6) & 0x3F] : '=');
    out.push_back((i + 2 < length) ? table[val & 0x3F] : '=');
  }
  return out;
}

inline String encode(const String& str) {
  return encode(reinterpret_cast<const uint8_t*>(str.c_str()), str.length());
}

}  // namespace base64
