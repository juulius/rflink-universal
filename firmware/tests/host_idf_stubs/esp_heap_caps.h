#pragma once
// Stub used only by host compiler; ESP-IDF supplies the actual PSRAM APIs.
#include <cstddef>
constexpr unsigned MALLOC_CAP_SPIRAM=1, MALLOC_CAP_8BIT=2;
extern "C" void* heap_caps_calloc(size_t count,size_t size,unsigned caps);
extern "C" void heap_caps_free(void*);
