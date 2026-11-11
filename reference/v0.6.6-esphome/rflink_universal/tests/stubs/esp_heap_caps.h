#pragma once
#include <cstddef>
// Host-test shim only.
#define MALLOC_CAP_8BIT 1
inline size_t heap_caps_get_free_size(unsigned) { return 120000; }
inline size_t heap_caps_get_minimum_free_size(unsigned) { return 110000; }
