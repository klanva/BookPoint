#pragma once

#include <cstddef>
#include <cstdlib>

#ifndef MALLOC_CAP_8BIT
#define MALLOC_CAP_8BIT 0
#endif
#ifndef MALLOC_CAP_SPIRAM
#define MALLOC_CAP_SPIRAM 0
#endif
#ifndef MALLOC_CAP_INTERNAL
#define MALLOC_CAP_INTERNAL 0
#endif

inline size_t heap_caps_get_total_size(int) { return 16 * 1024 * 1024; }
inline size_t heap_caps_get_free_size(int) { return 16 * 1024 * 1024; }
inline size_t heap_caps_get_largest_free_block(int) { return 16 * 1024 * 1024; }
inline void* heap_caps_malloc(size_t size, int) { return malloc(size); }
inline void heap_caps_free(void* ptr) { free(ptr); }
