#pragma once

#ifdef _WIN32
#include <time.h>
#include <string.h>

#ifndef _UINT_DEFINED
#define _UINT_DEFINED
typedef unsigned int uint;
#endif

#ifndef _ULONG_DEFINED
#define _ULONG_DEFINED
typedef unsigned long ulong;
#endif

#ifndef _USHORT_DEFINED
#define _USHORT_DEFINED
typedef unsigned short ushort;
#endif

#ifndef gmtime_r
static inline struct tm* gmtime_r(const time_t* timer, struct tm* result) {
  if (!timer || !result) return NULL;
  struct tm* p = gmtime(timer);
  if (!p) return NULL;
  *result = *p;
  return result;
}
#endif

#ifndef localtime_r
static inline struct tm* localtime_r(const time_t* timer, struct tm* result) {
  if (!timer || !result) return NULL;
  struct tm* p = localtime(timer);
  if (!p) return NULL;
  *result = *p;
  return result;
}
#endif

#endif

#ifdef __cplusplus
#include "Arduino.h"
#endif
