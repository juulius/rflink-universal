#pragma once
// Stub used only by host compiler; ESP-IDF supplies the actual PSRAM APIs.
#include <cstddef>
constexpr unsigned MALLOC_CAP_SPIRAM=1, MALLOC_CAP_8BIT=2, MALLOC_CAP_INTERNAL=4;
extern "C" void* heap_caps_calloc(size_t count,size_t size,unsigned caps);
extern "C" void heap_caps_free(void*);
extern "C" size_t heap_caps_get_free_size(unsigned caps);
extern "C" bool heap_caps_check_integrity(unsigned caps,bool print_errors);
