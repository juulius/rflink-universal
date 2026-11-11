#pragma once
#include "rflink_log_store.h"

namespace rflink_logging {
// Requires initialized ESP-IDF NVS. Caller must serialize with the RX logger mutex.
// Only log count limits/noise floor are stored. Arena allocation size is a firmware setting.
bool load_limits_from_nvs(Store& store);
bool commit_limits_to_nvs(Store& store, const Limits& limits);
}
