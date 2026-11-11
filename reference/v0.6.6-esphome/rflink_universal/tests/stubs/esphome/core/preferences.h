#pragma once
#include <cstdint>
namespace esphome {
class ESPPreferenceObject {
 public:
  template<typename T> bool load(T *) const { return false; }
  template<typename T> bool save(const T *) const { return true; }
};
class ESPPreferences {
 public:
  template<typename T> ESPPreferenceObject make_preference(uint32_t) { return {}; }
  bool sync() { return true; }
};
extern ESPPreferences *global_preferences;
}
