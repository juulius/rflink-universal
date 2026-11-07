#pragma once
// Fixed-size, versioned boot history. Pure C++ so overflow/ordering can be
// tested without an ESP32 or mock flash. Never store Wi-Fi keys or RF captures.
#include <cstdint>

namespace rflink {
constexpr uint32_t kBootHistoryMagic = 0x52464248; // "RFBH"
constexpr uint32_t kBootHistoryVersion = 1;
constexpr unsigned kBootHistoryCapacity = 12;

struct BootEntry {
  uint32_t sequence = 0;
  uint32_t reset_reason = 0;
  uint32_t crash_pc = 0;
  uint8_t core_dump = 0;
  char task[16] = {};
};

struct BootHistory {
  uint32_t magic = kBootHistoryMagic;
  uint32_t version = kBootHistoryVersion;
  uint32_t next_sequence = 1;
  uint32_t count = 0;
  BootEntry entries[kBootHistoryCapacity] = {};
};

inline bool boot_history_valid(const BootHistory& history) {
  return history.magic == kBootHistoryMagic &&
         history.version == kBootHistoryVersion &&
         history.next_sequence != 0 &&
         history.count <= kBootHistoryCapacity;
}

inline void boot_history_append(BootHistory& history, BootEntry entry) {
  if (!boot_history_valid(history)) history = BootHistory{};
  if (history.next_sequence == UINT32_MAX) history = BootHistory{};
  entry.sequence = history.next_sequence++;
  history.entries[(entry.sequence - 1) % kBootHistoryCapacity] = entry;
  if (history.count < kBootHistoryCapacity) ++history.count;
}

inline BootEntry boot_history_recent(const BootHistory& history, unsigned index) {
  if (!boot_history_valid(history) || index >= history.count) return {};
  const uint32_t sequence = history.next_sequence - 1 - index;
  return history.entries[(sequence - 1) % kBootHistoryCapacity];
}
} // namespace rflink
