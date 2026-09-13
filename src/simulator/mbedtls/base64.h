#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL -0x002A
#define MBEDTLS_ERR_BASE64_INVALID_CHARACTER -0x002C

static inline int mbedtls_base64_decode(unsigned char *dst, size_t dlen, size_t *olen,
                                        const unsigned char *src, size_t slen) {
  if (!olen) return MBEDTLS_ERR_BASE64_INVALID_CHARACTER;
  if (!src || slen == 0) {
    *olen = 0;
    return 0;
  }

  // Count valid base64 chars and padding
  size_t valid = 0;
  size_t pad = 0;
  for (size_t i = 0; i < slen; i++) {
    unsigned char c = src[i];
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '+' || c == '/') {
      valid++;
    } else if (c == '=') {
      pad++;
    }
  }

  size_t total = valid + pad;
  if (total == 0 || (total % 4) != 0) {
    *olen = 0;
    return MBEDTLS_ERR_BASE64_INVALID_CHARACTER;
  }

  size_t out_len = (total / 4) * 3 - pad;
  *olen = out_len;

  if (!dst || dlen < out_len) {
    return MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL;
  }

  size_t di = 0;
  uint32_t buf = 0;
  int bits = 0;

  for (size_t i = 0; i < slen; i++) {
    unsigned char c = src[i];
    int val = -1;
    if (c >= 'A' && c <= 'Z') val = c - 'A';
    else if (c >= 'a' && c <= 'z') val = c - 'a' + 26;
    else if (c >= '0' && c <= '9') val = c - '0' + 52;
    else if (c == '+') val = 62;
    else if (c == '/') val = 63;
    else if (c == '=') break;
    else continue;

    buf = (buf << 6) | val;
    bits += 6;
    if (bits >= 8) {
      bits -= 8;
      if (di < dlen) {
        dst[di++] = (buf >> bits) & 0xFF;
      }
    }
  }

  *olen = di;
  return 0;
}

#ifdef __cplusplus
}
#endif
