#pragma once
// Small, versioned flash journal of the last reached startup checkpoint.
// Records only stage enums and boot sequence numbers, not serial text, RF
// captures or credentials. No ESP-IDF dependencies: host-testable.
#include <cstdint>

namespace rflink {
constexpr uint32_t kBootPhaseMagic = 0x52464250; // "RFBP"
constexpr uint32_t kBootPhaseVersion = 1;
constexpr unsigned kBootPhaseCapacity = 12;

enum class BootPhase : uint8_t {
  unknown = 0,
  nvs_ready = 1,
  probing_radios = 2,
  starting_receivers = 3,
  starting_network = 4,
  verifying_ota = 5,
  app_ready = 6,
};

inline const char* boot_phase_name(BootPhase phase) {
  switch (phase) {
    case BootPhase::nvs_ready: return "nvs_ready";
    case BootPhase::probing_radios: return "probing_radios";
    case BootPhase::starting_receivers: return "starting_receivers";
    case BootPhase::starting_network: return "starting_network";
    case BootPhase::verifying_ota: return "verifying_ota";
    case BootPhase::app_ready: return "app_ready";
    default: return "unknown";
  }
}

struct BootPhaseEntry {
  uint32_t sequence = 0;
  uint8_t stage = 0;
  uint8_t startup_complete = 0;
  uint8_t reserved[2] = {};
};

struct BootPhaseHistory {
  uint32_t magic = kBootPhaseMagic;
  uint32_t version = kBootPhaseVersion;
  uint32_t last_sequence = 0;
  uint32_t count = 0;
  BootPhaseEntry entries[kBootPhaseCapacity] = {};
};

inline bool boot_phase_history_valid(const BootPhaseHistory& history) {
  return history.magic == kBootPhaseMagic &&
         history.version == kBootPhaseVersion &&
         history.count <= kBootPhaseCapacity &&
         ((history.count == 0 && history.last_sequence == 0) ||
          (history.count > 0 && history.last_sequence >= history.count));
}

// The existing reset-history sequence is authoritative. A migrated, corrupt,
// wrapped or discontinuous sequence starts a fresh phase journal, without
// modifying the previous reset-history blob.
inline bool boot_phase_begin(BootPhaseHistory& history, uint32_t sequence) {
  if (!sequence) return false;
  if (!boot_phase_history_valid(history) ||
      (history.count &&
       (history.last_sequence == UINT32_MAX ||
        sequence != history.last_sequence + 1))) {
    history = BootPhaseHistory{};
  }
  history.last_sequence = sequence;
  auto& entry = history.entries[(sequence - 1) % kBootPhaseCapacity];
  entry = BootPhaseEntry{};
  entry.sequence = sequence;
  entry.stage = static_cast<uint8_t>(BootPhase::nvs_ready);
  if (history.count < kBootPhaseCapacity) ++history.count;
  return true;
}

inline bool boot_phase_set(BootPhaseHistory& history, uint32_t sequence,
                           BootPhase stage, bool startup_complete = false) {
  if (!boot_phase_history_valid(history) ||
      sequence != history.last_sequence || !sequence ||
      stage == BootPhase::unknown || stage > BootPhase::app_ready ||
      (startup_complete && stage != BootPhase::app_ready)) return false;
  auto& entry = history.entries[(sequence - 1) % kBootPhaseCapacity];
  if (entry.sequence != sequence ||
      static_cast<uint8_t>(stage) < entry.stage ||
      (entry.startup_complete != 0)) return false;
  if (entry.stage == static_cast<uint8_t>(stage) &&
      entry.startup_complete == static_cast<uint8_t>(startup_complete)) return false;
  entry.stage = static_cast<uint8_t>(stage);
  entry.startup_complete = startup_complete ? 1 : 0;
  return true;
}

inline const BootPhaseEntry* boot_phase_find(const BootPhaseHistory& history,
                                             uint32_t sequence) {
  if (!boot_phase_history_valid(history) || sequence == 0) return nullptr;
  const auto& entry = history.entries[(sequence - 1) % kBootPhaseCapacity];
  if (entry.sequence != sequence ||
      entry.stage < static_cast<uint8_t>(BootPhase::nvs_ready) ||
      entry.stage > static_cast<uint8_t>(BootPhase::app_ready)) return nullptr;
  return &entry;
}
} // namespace rflink
