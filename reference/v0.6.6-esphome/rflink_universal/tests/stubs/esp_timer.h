#pragma once
#include <cstdint>
// Host-test shim only; 1.23 seconds of elapsed time.
inline int64_t esp_timer_get_time() { return 1230000LL; }
