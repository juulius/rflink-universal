#pragma once

// RFLink Universal - host-testable, bounded-memory RX log storage.
// Supplied arena must be caller-owned (prefer heap_caps_malloc(..., MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT)).
// Not thread-safe: call from a single logging task or protect accesses with an external mutex.

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace rflink_logging {

constexpr uint16_t kNone = 0xffff;
constexpr size_t kMaxLogEvents = 4096;
constexpr size_t kDefaultArenaBytes = 1536 * 1024;
constexpr size_t kMaxEventJsonBytes = 32 * 1024; // Oversize event rejected whole, never truncated.
constexpr size_t kMaxPageJsonBytes = 96 * 1024; // Limit temporary export/read memory.

enum class Kind : uint8_t { Signal = 0, Noise = 1 };

enum class AddResult : uint8_t { Saved, Disabled, InvalidRadio, TooLarge, NoSpace };

struct Limits {
  std::array<uint16_t, 3> signals{{250, 250, 250}};
  uint16_t noise = 2000;          // ONE shared noise limit across all radios.
  uint16_t noise_floor = 50;      // Soft floor per radio, if available; yield to memory pressure.
};

struct Entry {
  uint64_t sequence = 0;
  uint64_t epoch_ms = 0;         // 0 when no valid wall clock is available.
  uint64_t uptime_ms = 0;
  uint8_t radio = 0;             // 0=433.42, 1=433.92, 2=868.30
  Kind kind = Kind::Signal;
  std::string json;              // Complete event JSON, including RAW and diagnostics.
  uint32_t json_bytes = 0;      // Original size even if payload is excluded from a metadata-only read.
};

struct Filter {
  int radio = -1;                // -1 = all radios
  int kind = -1;                 // -1 = signal + noise, 0 = signals, 1 = noise
};

struct Counters {
  std::array<uint32_t, 3> signals{{0, 0, 0}};
  std::array<uint32_t, 3> noise_by_radio{{0, 0, 0}};
  uint32_t noise_total = 0;
  uint32_t total = 0;
  uint32_t free_blocks = 0;
  uint32_t total_blocks = 0;
  uint32_t bytes_used_payload = 0;
  uint32_t allocated_arena_bytes = 0;
  std::array<uint64_t, 4> evicted_limit{{0, 0, 0, 0}};
  std::array<uint64_t, 4> evicted_memory{{0, 0, 0, 0}};
  std::array<uint64_t, 4> rejected{{0, 0, 0, 0}};
};

class Store {
 public:
  Store(void* arena, size_t arena_bytes);
  bool ready() const { return ready_; }
  bool set_limits(const Limits& settings);  // Effective immediately, no restart required.
  const Limits& limits() const { return limits_; }

  AddResult add(uint8_t radio, Kind kind, uint64_t epoch_ms,
                uint64_t uptime_ms, const uint8_t* json, size_t json_bytes);
  AddResult add(uint8_t radio, Kind kind, uint64_t epoch_ms,
                uint64_t uptime_ms, const std::string& json) {
    return add(radio, kind, epoch_ms, uptime_ms,
               reinterpret_cast<const uint8_t*>(json.data()), json.size());
  }

  // Live pagination: select events strictly AFTER the given sequence (ascending).
  // Call with external locking. Large exports should page to avoid long RX stalls.
  std::vector<Entry> read_after(uint64_t sequence, const Filter& filter,
                                size_t limit, bool include_json = true) const;
  // Newest first: entries strictly BEFORE given sequence, or UINT64_MAX for latest.
  std::vector<Entry> read_before(uint64_t sequence, const Filter& filter,
                                 size_t limit, bool include_json = true) const;
  // Copies one complete event into caller-owned memory (normally a temporary PSRAM buffer).
  // The caller must protect this operation with the same lock as add()/evict().
  bool copy_json(uint64_t sequence, char* dst, size_t capacity, size_t& written) const;
  Counters counters() const;
  uint64_t latest_sequence() const { return next_sequence_ - 1; }

 private:
  static constexpr size_t kBlockPayload = 254;
  struct Block { uint16_t next; uint8_t data[kBlockPayload]; };
  struct Slot {
    uint64_t sequence = 0;
    uint64_t epoch_ms = 0;
    uint64_t uptime_ms = 0;
    uint32_t data_len = 0;
    uint16_t first_block = kNone;
    uint16_t next = kNone;
    uint16_t prev = kNone;
    uint16_t free_next = kNone;
    uint8_t radio = 0;
    uint8_t kind = 0;
  };
  struct Queue { uint16_t head = kNone; uint16_t tail = kNone; uint16_t count = 0; };
  enum class Why : uint8_t { Limit, Memory };

  bool ready_ = false;
  Limits limits_{};
  Slot* slots_ = nullptr;
  Block* blocks_ = nullptr;
  uint16_t slot_free_ = kNone;
  uint16_t block_free_ = kNone;
  uint16_t free_blocks_ = 0;
  uint16_t block_count_ = 0;
  uint16_t free_slots_ = 0;
  uint64_t next_sequence_ = 1;
  std::array<Queue, 4> queues_{};
  std::array<uint16_t, 3> noise_by_radio_{{0,0,0}};
  std::array<uint64_t, 4> evicted_limit_{{0,0,0,0}};
  std::array<uint64_t, 4> evicted_memory_{{0,0,0,0}};
  std::array<uint64_t, 4> rejected_{{0,0,0,0}};
  uint32_t payload_bytes_ = 0;
  uint32_t arena_bytes_ = 0;

  uint16_t noise_candidate() const;
  uint16_t signal_candidate() const;
  void evict(uint16_t slot, Why why);
  uint16_t take_slot();
  uint16_t take_block();
  void release_blocks(uint16_t head);
  void link(uint16_t slot, int queue);
  bool matches(const Slot& slot, const Filter& filter) const;
  std::string read_json(const Slot& slot) const;
};

}  // namespace rflink_logging
