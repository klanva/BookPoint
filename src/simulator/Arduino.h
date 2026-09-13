#pragma once

#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <cassert>
#include <chrono>
#include <thread>
#include <string>
#include <algorithm>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define PROGMEM
#define PSTR(s) (s)
#define F(s) (s)
#define RTC_NOINIT_ATTR
#define RTC_DATA_ATTR
#define IRAM_ATTR

#ifndef memcpy_P
#define memcpy_P memcpy
#endif
#ifndef strlen_P
#define strlen_P strlen
#endif
#ifndef strcpy_P
#define strcpy_P strcpy
#endif
#ifndef strncpy_P
#define strncpy_P strncpy
#endif
#ifndef strcmp_P
#define strcmp_P strcmp
#endif
#ifndef strncmp_P
#define strncmp_P strncmp
#endif

#define LOW 0
#define HIGH 1
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2

#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) (bitvalue ? bitSet(value, bit) : bitClear(value, bit))

inline unsigned long millis() {
  using namespace std::chrono;
  static const auto start = steady_clock::now();
  return static_cast<unsigned long>(duration_cast<milliseconds>(steady_clock::now() - start).count());
}

inline unsigned long micros() {
  using namespace std::chrono;
  static const auto start = steady_clock::now();
  return static_cast<unsigned long>(duration_cast<microseconds>(steady_clock::now() - start).count());
}

inline void delay(unsigned long ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

inline void yield() {
  std::this_thread::yield();
}

inline void randomSeed(unsigned long seed) {
  srand(static_cast<unsigned int>(seed));
}

inline long random(long max) {
  if (max <= 0) return 0;
  return rand() % max;
}

inline long random(long min, long max) {
  if (min >= max) return min;
  return min + (rand() % (max - min));
}

#include "Esp.h"

class Print {
 public:
  virtual ~Print() = default;
  virtual size_t write(uint8_t c) = 0;
  virtual size_t write(const uint8_t *buffer, size_t size) {
    size_t n = 0;
    while (size--) {
      if (write(*buffer++)) n++;
      else break;
    }
    return n;
  }
  size_t print(const char* s) {
    if (!s) return 0;
    return write(reinterpret_cast<const uint8_t*>(s), strlen(s));
  }
  size_t print(const std::string& s) {
    return write(reinterpret_cast<const uint8_t*>(s.data()), s.size());
  }
  size_t print(char c) { return write(static_cast<uint8_t>(c)); }
  size_t print(int n) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", n);
    return print(buf);
  }
  size_t print(unsigned int n) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%u", n);
    return print(buf);
  }
  size_t print(long n) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%ld", n);
    return print(buf);
  }
  size_t print(unsigned long n) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%lu", n);
    return print(buf);
  }
  size_t println(const char* s = "") {
    size_t n = print(s);
    n += print("\n");
    return n;
  }
  size_t println(const std::string& s) {
    size_t n = print(s);
    n += print("\n");
    return n;
  }
  size_t println(int n) {
    size_t res = print(n);
    res += print("\n");
    return res;
  }
  size_t println(unsigned long n) {
    size_t res = print(n);
    res += print("\n");
    return res;
  }
  size_t printf(const char* format, ...) {
    char buf[1024];
    va_list args;
    va_start(args, format);
    int len = vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    if (len > 0) {
      return write(reinterpret_cast<const uint8_t*>(buf), len);
    }
    return 0;
  }
  virtual void flush() {}
};

class HardwareSerial : public Print {
 public:
  void begin(unsigned long) {}
  operator bool() const { return true; }
  void flush() override { fflush(stdout); }
  size_t write(uint8_t c) override {
    putchar(c);
    return 1;
  }
  size_t write(const uint8_t *buffer, size_t size) override {
    return fwrite(buffer, 1, size, stdout);
  }
  int available() { return 0; }
  int read() { return -1; }
};

inline HardwareSerial Serial;
using HWCDC = HardwareSerial;

class String : public std::string {
 public:
  String() : std::string() {}
  String(const char* s) : std::string(s ? s : "") {}
  explicit String(const std::string& s) : std::string(s) {}
  explicit String(std::string&& s) : std::string(std::move(s)) {}
  explicit String(char c) : std::string(1, c) {}
  explicit String(int n) : std::string(std::to_string(n)) {}
  explicit String(unsigned int n) : std::string(std::to_string(n)) {}
  explicit String(long n) : std::string(std::to_string(n)) {}
  explicit String(unsigned long n) : std::string(std::to_string(n)) {}

  size_t write(uint8_t c) {
    push_back(static_cast<char>(c));
    return 1;
  }
  size_t write(const uint8_t* buffer, size_t size) {
    if (buffer && size > 0) {
      append(reinterpret_cast<const char*>(buffer), size);
      return size;
    }
    return 0;
  }

  int toInt() const {
    try {
      return std::stoi(*this);
    } catch (...) {
      return 0;
    }
  }

  bool isEmpty() const { return empty(); }
  int indexOf(char c, size_t fromIndex = 0) const {
    auto pos = find(c, fromIndex);
    return (pos == std::string::npos) ? -1 : static_cast<int>(pos);
  }
  int indexOf(const String& str, size_t fromIndex = 0) const {
    auto pos = find(str, fromIndex);
    return (pos == std::string::npos) ? -1 : static_cast<int>(pos);
  }
  int lastIndexOf(char c) const {
    auto pos = rfind(c);
    return (pos == std::string::npos) ? -1 : static_cast<int>(pos);
  }
  String substring(size_t from, size_t to = std::string::npos) const {
    if (to == std::string::npos || to > size()) {
      return String(substr(from));
    }
    return String(substr(from, to - from));
  }
  bool startsWith(const String& prefix) const {
    return rfind(prefix, 0) == 0;
  }
  bool endsWith(const String& suffix) const {
    if (suffix.size() > size()) return false;
    return compare(size() - suffix.size(), suffix.size(), suffix) == 0;
  }
  void toLowerCase() {
    std::transform(begin(), end(), begin(), [](unsigned char c) { return std::tolower(c); });
  }
  void toUpperCase() {
    std::transform(begin(), end(), begin(), [](unsigned char c) { return std::toupper(c); });
  }
  void trim() {
    auto isNotSpace = [](unsigned char ch) { return !std::isspace(ch); };
    auto start = std::find_if(begin(), end(), isNotSpace);
    auto end_it = std::find_if(rbegin(), rend(), isNotSpace).base();
    if (start < end_it) {
      assign(start, end_it);
    } else {
      clear();
    }
  }

  friend String operator+(const char* lhs, const String& rhs) {
    String res(lhs);
    res += rhs;
    return res;
  }
  friend String operator+(const String& lhs, const char* rhs) {
    String res(lhs);
    res += rhs;
    return res;
  }
  friend String operator+(const String& lhs, const String& rhs) {
    String res(lhs);
    res += rhs;
    return res;
  }
  friend String operator+(const String& lhs, char rhs) {
    String res(lhs);
    res += rhs;
    return res;
  }
  friend String operator+(char lhs, const String& rhs) {
    String res;
    res += lhs;
    res += rhs;
    return res;
  }
  friend String operator+(const String& lhs, int rhs) {
    String res(lhs);
    res += std::to_string(rhs);
    return res;
  }
  friend String operator+(const String& lhs, unsigned int rhs) {
    String res(lhs);
    res += std::to_string(rhs);
    return res;
  }
};

#include "Stream.h"

