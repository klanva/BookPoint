#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <sstream>
#include <iomanip>

class MD5Builder {
 public:
  MD5Builder() { begin(); }

  void begin() {
    state[0] = 0x67452301;
    state[1] = 0xefcdab89;
    state[2] = 0x98badcfe;
    state[3] = 0x10325476;
    count[0] = count[1] = 0;
    std::memset(buffer, 0, sizeof(buffer));
  }

  void add(const uint8_t* data, size_t len) {
    uint32_t i, index, partLen;
    index = static_cast<uint32_t>((count[0] >> 3) & 0x3F);
    if ((count[0] += static_cast<uint32_t>(len << 3)) < static_cast<uint32_t>(len << 3)) {
      count[1]++;
    }
    count[1] += static_cast<uint32_t>(len >> 29);
    partLen = 64 - index;
    if (len >= partLen) {
      std::memcpy(&buffer[index], data, partLen);
      transform(state, buffer);
      for (i = partLen; i + 63 < len; i += 64) {
        transform(state, &data[i]);
      }
      index = 0;
    } else {
      i = 0;
    }
    std::memcpy(&buffer[index], &data[i], len - i);
  }

  void add(const char* str) {
    if (str) {
      add(reinterpret_cast<const uint8_t*>(str), std::strlen(str));
    }
  }

  void calculate() {
    uint8_t bits[8];
    encode(bits, count, 8);
    uint32_t index = static_cast<uint32_t>((count[0] >> 3) & 0x3F);
    uint32_t padLen = (index < 56) ? (56 - index) : (120 - index);
    static const uint8_t PADDING[64] = { 0x80 };
    add(PADDING, padLen);
    add(bits, 8);
    encode(digest, state, 16);
  }

  void getBytes(uint8_t* out) const {
    std::memcpy(out, digest, 16);
  }

  std::string toString() const {
    std::ostringstream ss;
    for (int i = 0; i < 16; ++i) {
      ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(digest[i]);
    }
    return ss.str();
  }

 private:
  uint32_t state[4];
  uint32_t count[2];
  uint8_t buffer[64];
  uint8_t digest[16];

  static void encode(uint8_t* output, const uint32_t* input, size_t len) {
    for (size_t i = 0, j = 0; j < len; i++, j += 4) {
      output[j] = static_cast<uint8_t>(input[i] & 0xFF);
      output[j + 1] = static_cast<uint8_t>((input[i] >> 8) & 0xFF);
      output[j + 2] = static_cast<uint8_t>((input[i] >> 16) & 0xFF);
      output[j + 3] = static_cast<uint8_t>((input[i] >> 24) & 0xFF);
    }
  }

  static void decode(uint32_t* output, const uint8_t* input, size_t len) {
    for (size_t i = 0, j = 0; j < len; i++, j += 4) {
      output[i] = static_cast<uint32_t>(input[j]) |
                  (static_cast<uint32_t>(input[j + 1]) << 8) |
                  (static_cast<uint32_t>(input[j + 2]) << 16) |
                  (static_cast<uint32_t>(input[j + 3]) << 24);
    }
  }

  static inline uint32_t md5_F(uint32_t x, uint32_t y, uint32_t z) { return (x & y) | (~x & z); }
  static inline uint32_t md5_G(uint32_t x, uint32_t y, uint32_t z) { return (x & z) | (y & ~z); }
  static inline uint32_t md5_H(uint32_t x, uint32_t y, uint32_t z) { return x ^ y ^ z; }
  static inline uint32_t md5_I(uint32_t x, uint32_t y, uint32_t z) { return y ^ (x | ~z); }
  static inline uint32_t ROTATE_LEFT(uint32_t x, uint32_t n) { return (x << n) | (x >> (32 - n)); }

  static inline void FF(uint32_t& a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac) {
    a = ROTATE_LEFT(a + md5_F(b, c, d) + x + ac, s) + b;
  }
  static inline void GG(uint32_t& a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac) {
    a = ROTATE_LEFT(a + md5_G(b, c, d) + x + ac, s) + b;
  }
  static inline void HH(uint32_t& a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac) {
    a = ROTATE_LEFT(a + md5_H(b, c, d) + x + ac, s) + b;
  }
  static inline void II(uint32_t& a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac) {
    a = ROTATE_LEFT(a + md5_I(b, c, d) + x + ac, s) + b;
  }

  static void transform(uint32_t state[4], const uint8_t block[64]) {
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3], x[16];
    decode(x, block, 64);
    FF(a, b, c, d, x[0], 7, 0xd76aa478);
    FF(d, a, b, c, x[1], 12, 0xe8c7b756);
    FF(c, d, a, b, x[2], 17, 0x242070db);
    FF(b, c, d, a, x[3], 22, 0xc1bdceee);
    FF(a, b, c, d, x[4], 7, 0xf57c0faf);
    FF(d, a, b, c, x[5], 12, 0x4787c62a);
    FF(c, d, a, b, x[6], 17, 0xa8304613);
    FF(b, c, d, a, x[7], 22, 0xfd469501);
    FF(a, b, c, d, x[8], 7, 0x698098d8);
    FF(d, a, b, c, x[9], 12, 0x8b44f7af);
    FF(c, d, a, b, x[10], 17, 0xffff5bb1);
    FF(b, c, d, a, x[11], 22, 0x895cd7be);
    FF(a, b, c, d, x[12], 7, 0x6b901122);
    FF(d, a, b, c, x[13], 12, 0xfd987193);
    FF(c, d, a, b, x[14], 17, 0xa679438e);
    FF(b, c, d, a, x[15], 22, 0x49b40821);
    GG(a, b, c, d, x[1], 5, 0xf61e2562);
    GG(d, a, b, c, x[6], 9, 0xc040b340);
    GG(c, d, a, b, x[11], 14, 0x265e5a51);
    GG(b, c, d, a, x[0], 20, 0xe9b6c7aa);
    GG(a, b, c, d, x[5], 5, 0xd62f105d);
    GG(d, a, b, c, x[10], 9, 0x02441453);
    GG(c, d, a, b, x[15], 14, 0xd8a1e681);
    GG(b, c, d, a, x[4], 20, 0xe7d3fbc8);
    GG(a, b, c, d, x[9], 5, 0x21e1cde6);
    GG(d, a, b, c, x[14], 9, 0xc33707d6);
    GG(c, d, a, b, x[3], 14, 0xf4d50d87);
    GG(b, c, d, a, x[8], 20, 0x455a14ed);
    GG(a, b, c, d, x[13], 5, 0xa9e3e905);
    GG(d, a, b, c, x[2], 9, 0xfcefa3f8);
    GG(c, d, a, b, x[7], 14, 0x676f02d9);
    GG(b, c, d, a, x[12], 20, 0x8d2a4c8a);
    HH(a, b, c, d, x[5], 4, 0xfffa3942);
    HH(d, a, b, c, x[8], 11, 0x8771f681);
    HH(c, d, a, b, x[11], 16, 0x6d9d6122);
    HH(b, c, d, a, x[14], 23, 0xfde5380c);
    HH(a, b, c, d, x[1], 4, 0xa4beea44);
    HH(d, a, b, c, x[4], 11, 0x4bdecfa9);
    HH(c, d, a, b, x[7], 16, 0xf6bb4b60);
    HH(b, c, d, a, x[10], 23, 0xbebfbc70);
    HH(a, b, c, d, x[13], 4, 0x289b7ec6);
    HH(d, a, b, c, x[0], 11, 0xeaa127fa);
    HH(c, d, a, b, x[3], 16, 0xd4ef3085);
    HH(b, c, d, a, x[6], 23, 0x04881d05);
    HH(a, b, c, d, x[9], 4, 0xd9d4d039);
    HH(d, a, b, c, x[12], 11, 0xe6db99e5);
    HH(c, d, a, b, x[15], 16, 0x1fa27cf8);
    HH(b, c, d, a, x[2], 23, 0xc4ac5665);
    II(a, b, c, d, x[0], 6, 0xf4292244);
    II(d, a, b, c, x[7], 10, 0x432aff97);
    II(c, d, a, b, x[14], 15, 0xab9423a7);
    II(b, c, d, a, x[5], 21, 0xfc93a039);
    II(a, b, c, d, x[12], 6, 0x655b59c3);
    II(d, a, b, c, x[3], 10, 0x8f0ccc92);
    II(c, d, a, b, x[10], 15, 0xffeff47d);
    II(b, c, d, a, x[1], 21, 0x85845dd1);
    II(a, b, c, d, x[8], 6, 0x6fa87e4f);
    II(d, a, b, c, x[15], 10, 0xfe2ce6e0);
    II(c, d, a, b, x[6], 15, 0xa3014314);
    II(b, c, d, a, x[13], 21, 0x4e0811a1);
    II(a, b, c, d, x[4], 6, 0xf7537e82);
    II(d, a, b, c, x[11], 10, 0xbd3af235);
    II(c, d, a, b, x[2], 15, 0x2ad7d2bb);
    II(b, c, d, a, x[9], 21, 0xeb86d391);
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
  }
};
