#pragma once

#include <cstdint>
#include <cstdio>
#include <string>

namespace esphome {
namespace rflink_universal {

inline std::string hex_id(const char *prefix, uint64_t value, unsigned digits) {
  char buffer[64];
  std::snprintf(buffer, sizeof(buffer), "%s:%0*llX", prefix, static_cast<int>(digits),
                static_cast<unsigned long long>(value));
  return buffer;
}

inline std::string number_string(double value, unsigned decimals = 1) {
  char fmt[12];
  char buffer[32];
  std::snprintf(fmt, sizeof(fmt), "%%.%uf", decimals);
  std::snprintf(buffer, sizeof(buffer), fmt, value);
  return buffer;
}

}  // namespace rflink_universal
}  // namespace esphome
