#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "device_store.h"

namespace esphome {
namespace rflink_universal {

// A bounded, versioned snapshot stored in TWO ESPHome NVS preference slots.
// No radio capture history or volatile millisecond timestamps are persisted.
class DevicePersistence {
 public:
  static constexpr size_t CAPACITY = 8192;
  static constexpr size_t MAX_DEVICES = 24;
  static constexpr uint32_t MAGIC = 0x52464456u;  // RFDV
  static constexpr uint16_t VERSION = 4;
  struct Blob {
    uint32_t magic{MAGIC};
    uint16_t version{VERSION};
    uint16_t used{0};
    uint32_t generation{0};
    uint32_t checksum{0};
    uint8_t bytes[CAPACITY]{};
  };

  // Never silently truncate records. Reject a snapshot that does not fit.
  static bool encode(const DeviceStore &store, uint32_t generation, Blob &blob);
  static bool decode(const Blob &blob, DeviceStore &store);
  static bool valid(const Blob &blob);
};

}  // namespace rflink_universal
}  // namespace esphome
