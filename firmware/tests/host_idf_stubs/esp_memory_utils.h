#pragma once
// Host syntax audit only: real ESP-IDF checks internal memory address ranges.
inline bool esp_ptr_internal(const void*) { return true; }
