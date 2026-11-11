#include "rflink_log_store.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <new>

namespace rflink_logging {

Store::Store(void* arena, size_t arena_bytes) {
  if (!arena || arena_bytes > std::numeric_limits<uint32_t>::max()) return;
  // Align slot array inside the supplied, single fixed-size arena.
  uintptr_t ptr = reinterpret_cast<uintptr_t>(arena);
  uintptr_t aligned = (ptr + alignof(Slot) - 1) & ~(uintptr_t(alignof(Slot)) - 1);
  size_t pad = aligned - ptr;
  const size_t slot_bytes = sizeof(Slot) * kMaxLogEvents;
  if (arena_bytes <= pad + slot_bytes + sizeof(Block) * 2) return;
  size_t count = (arena_bytes - pad - slot_bytes) / sizeof(Block);
  if (count > kNone) count = kNone;
  if (count < 2) return;
  arena_bytes_ = static_cast<uint32_t>(arena_bytes);
  slots_ = reinterpret_cast<Slot*>(aligned);
  blocks_ = reinterpret_cast<Block*>(aligned + slot_bytes);
  block_count_ = static_cast<uint16_t>(count);
  free_slots_ = kMaxLogEvents;
  free_blocks_ = block_count_;
  for (size_t i = 0; i < kMaxLogEvents; ++i) {
    new (&slots_[i]) Slot();
    slots_[i].free_next = (i + 1 < kMaxLogEvents) ? static_cast<uint16_t>(i + 1) : kNone;
  }
  slot_free_ = 0;
  for (size_t i = 0; i < block_count_; ++i) {
    new (&blocks_[i]) Block();
    blocks_[i].next = (i + 1 < block_count_) ? static_cast<uint16_t>(i + 1) : kNone;
  }
  block_free_ = 0;
  ready_ = true;
}

uint16_t Store::take_slot() {
  if (slot_free_ == kNone) return kNone;
  uint16_t id = slot_free_;
  slot_free_ = slots_[id].free_next;
  --free_slots_;
  slots_[id] = Slot{};
  return id;
}

uint16_t Store::take_block() {
  if (block_free_ == kNone) return kNone;
  uint16_t id = block_free_;
  block_free_ = blocks_[id].next;
  blocks_[id].next = kNone;
  --free_blocks_;
  return id;
}

void Store::release_blocks(uint16_t head) {
  while (head != kNone) {
    uint16_t id = head;
    head = blocks_[id].next;
    blocks_[id].next = block_free_;
    block_free_ = id;
    ++free_blocks_;
  }
}

void Store::link(uint16_t id, int queue_id) {
  Queue& queue = queues_[queue_id];
  Slot& slot = slots_[id];
  slot.prev = queue.tail;
  slot.next = kNone;
  if (queue.tail == kNone) queue.head = id;
  else slots_[queue.tail].next = id;
  queue.tail = id;
  ++queue.count;
  if (queue_id == 3) ++noise_by_radio_[slot.radio];
}

uint16_t Store::noise_candidate() const {
  const Queue& queue = queues_[3];
  if (queue.head == kNone) return kNone;
  // Soft protection: prefer evicting noise from a radio with > floor samples.
  for (uint16_t id = queue.head; id != kNone; id = slots_[id].next) {
    if (noise_by_radio_[slots_[id].radio] > limits_.noise_floor) return id;
  }
  return queue.head;  // All at floor: soft protection yields to the hard budget.
}

uint16_t Store::signal_candidate() const {
  uint16_t oldest = kNone;
  for (int i = 0; i < 3; ++i) {
    uint16_t head = queues_[i].head;
    if (head != kNone && (oldest == kNone || slots_[head].sequence < slots_[oldest].sequence))
      oldest = head;
  }
  return oldest;
}

void Store::evict(uint16_t id, Why why) {
  if (id == kNone) return;
  Slot& slot = slots_[id];
  int qid = slot.kind == static_cast<uint8_t>(Kind::Noise) ? 3 : slot.radio;
  Queue& q = queues_[qid];
  if (slot.prev == kNone) q.head = slot.next;
  else slots_[slot.prev].next = slot.next;
  if (slot.next == kNone) q.tail = slot.prev;
  else slots_[slot.next].prev = slot.prev;
  --q.count;
  if (qid == 3) --noise_by_radio_[slot.radio];
  release_blocks(slot.first_block);
  payload_bytes_ -= slot.data_len;
  if (why == Why::Limit) ++evicted_limit_[qid];
  else ++evicted_memory_[qid];
  slot = Slot{};
  slot.free_next = slot_free_;
  slot_free_ = id;
  ++free_slots_;
}

bool Store::set_limits(const Limits& settings) {
  if (!ready_ || settings.noise > kMaxLogEvents || settings.noise_floor > 2000) return false;
  size_t potential = settings.noise;
  for (uint16_t n : settings.signals) {
    if (n > kMaxLogEvents) return false;
    potential += n;
  }
  if (potential > kMaxLogEvents) return false;
  limits_ = settings;
  for (int i = 0; i < 3; ++i)
    while (queues_[i].count > limits_.signals[i]) evict(queues_[i].head, Why::Limit);
  while (queues_[3].count > limits_.noise) evict(noise_candidate(), Why::Limit);
  return true;
}

AddResult Store::add(uint8_t radio, Kind kind, uint64_t epoch_ms,
                     uint64_t uptime_ms, const uint8_t* json, size_t len) {
  if (!ready_ || radio >= 3 || (kind != Kind::Noise && kind != Kind::Signal))
    return AddResult::InvalidRadio;
  const int qid = kind == Kind::Noise ? 3 : radio;
  const uint16_t cap = kind == Kind::Noise ? limits_.noise : limits_.signals[radio];
  if (cap == 0) { ++rejected_[qid]; return AddResult::Disabled; }
  if (!json || len == 0 || len > kMaxEventJsonBytes) {
    ++rejected_[qid]; return AddResult::TooLarge;
  }
  const size_t needed = (len + kBlockPayload - 1) / kBlockPayload;
  if (needed > block_count_) { ++rejected_[qid]; return AddResult::TooLarge; }

  while (queues_[qid].count >= cap) {
    evict(kind == Kind::Noise ? noise_candidate() : queues_[qid].head, Why::Limit);
  }
  while (free_slots_ == 0 || free_blocks_ < needed) {
    uint16_t id = noise_candidate();
    if (id == kNone && kind == Kind::Signal) id = signal_candidate();
    if (id == kNone) { ++rejected_[qid]; return AddResult::NoSpace; }
    evict(id, Why::Memory);
  }
  uint16_t sid = take_slot();
  if (sid == kNone) { ++rejected_[qid]; return AddResult::NoSpace; }
  Slot& s = slots_[sid];
  s.radio = radio;
  s.kind = static_cast<uint8_t>(kind);
  s.epoch_ms = epoch_ms;
  s.uptime_ms = uptime_ms;
  s.data_len = static_cast<uint32_t>(len);
  s.sequence = next_sequence_++;
  uint16_t prev = kNone;
  size_t pos = 0;
  for (size_t i = 0; i < needed; ++i) {
    uint16_t b = take_block(); // Capacity was checked before taking slot.
    if (i == 0) s.first_block = b;
    else blocks_[prev].next = b;
    size_t n = std::min(kBlockPayload, len - pos);
    std::memcpy(blocks_[b].data, json + pos, n);
    pos += n;
    prev = b;
  }
  payload_bytes_ += len;
  link(sid, qid);
  return AddResult::Saved;
}

bool Store::matches(const Slot& s, const Filter& f) const {
  return (f.radio < 0 || s.radio == f.radio) && (f.kind < 0 || s.kind == f.kind);
}

std::string Store::read_json(const Slot& slot) const {
  std::string out;
  out.resize(slot.data_len);
  size_t pos = 0;
  for (uint16_t i = slot.first_block; i != kNone; i = blocks_[i].next) {
    size_t n = std::min(kBlockPayload, out.size() - pos);
    std::memcpy(&out[pos], blocks_[i].data, n);
    pos += n;
  }
  return out;
}

std::vector<Entry> Store::read_after(uint64_t sequence, const Filter& filter,
                                     size_t limit, bool include_json) const {
  std::vector<Entry> output;
  if (!ready_ || limit == 0 || filter.radio > 2 || filter.kind > 1) return output;
  // Safety cap: keep each page small so RX is not blocked by a full export.
  limit = std::min<size_t>(limit, 100);
  uint16_t ids[4] = {queues_[0].head, queues_[1].head,
                     queues_[2].head, queues_[3].head};
  while (output.size() < limit) {
    int selected = -1;
    for (int q = 0; q < 4; ++q) {
      while (ids[q] != kNone && slots_[ids[q]].sequence <= sequence)
        ids[q] = slots_[ids[q]].next;
      if (ids[q] != kNone && (selected < 0 ||
          slots_[ids[q]].sequence < slots_[ids[selected]].sequence)) selected = q;
    }
    if (selected < 0) break;
    const Slot& s = slots_[ids[selected]];
    ids[selected] = s.next;
    if (!matches(s, filter)) continue;
    size_t total_bytes = 0;
    for (const auto& prior : output) total_bytes += prior.json.size();
    if (!output.empty() && total_bytes + s.data_len > kMaxPageJsonBytes) break;
    Entry e;
    e.sequence = s.sequence;
    e.epoch_ms = s.epoch_ms;
    e.uptime_ms = s.uptime_ms;
    e.radio = s.radio;
    e.kind = static_cast<Kind>(s.kind);
    e.json_bytes = s.data_len;
    if (include_json) e.json = read_json(s);
    output.push_back(std::move(e));
  }
  return output;
}


std::vector<Entry> Store::read_before(uint64_t sequence, const Filter& filter,
                                      size_t limit, bool include_json) const {
  std::vector<Entry> output;
  if (!ready_ || limit == 0 || filter.radio > 2 || filter.kind > 1) return output;
  limit = std::min<size_t>(limit, 100);
  uint16_t ids[4] = {queues_[0].tail, queues_[1].tail,
                     queues_[2].tail, queues_[3].tail};
  while (output.size() < limit) {
    int selected = -1;
    for (int q = 0; q < 4; ++q) {
      while (ids[q] != kNone && slots_[ids[q]].sequence >= sequence)
        ids[q] = slots_[ids[q]].prev;
      if (ids[q] != kNone && (selected < 0 ||
          slots_[ids[q]].sequence > slots_[ids[selected]].sequence)) selected = q;
    }
    if (selected < 0) break;
    const Slot& s = slots_[ids[selected]];
    ids[selected] = s.prev;
    if (!matches(s, filter)) continue;
    size_t total_bytes = 0;
    for (const auto& prior : output) total_bytes += prior.json.size();
    if (!output.empty() && total_bytes + s.data_len > kMaxPageJsonBytes) break;
    Entry e;
    e.sequence = s.sequence;
    e.epoch_ms = s.epoch_ms;
    e.uptime_ms = s.uptime_ms;
    e.radio = s.radio;
    e.kind = static_cast<Kind>(s.kind);
    e.json_bytes = s.data_len;
    if (include_json) e.json = read_json(s);
    output.push_back(std::move(e));
  }
  return output;
}

bool Store::copy_json(uint64_t sequence, char* dst, size_t capacity, size_t& written) const {
  written = 0;
  if (!ready_ || !dst || sequence == 0) return false;
  for (int q = 0; q < 4; ++q) {
    // Start at tail: recent pages resolve quickly; bounded by kMaxLogEvents.
    for (uint16_t id = queues_[q].tail; id != kNone; id = slots_[id].prev) {
      const Slot& s = slots_[id];
      if (s.sequence < sequence) break;
      if (s.sequence != sequence) continue;
      if (s.data_len > capacity) return false;
      for (uint16_t block = s.first_block; block != kNone; block = blocks_[block].next) {
        const size_t n = std::min(kBlockPayload, static_cast<size_t>(s.data_len) - written);
        std::memcpy(dst + written, blocks_[block].data, n);
        written += n;
      }
      return written == s.data_len;
    }
  }
  return false;
}

Counters Store::counters() const {
  Counters c;
  if (!ready_) return c;
  for (int i = 0; i < 3; ++i) c.signals[i] = queues_[i].count;
  for (int i = 0; i < 3; ++i) c.noise_by_radio[i] = noise_by_radio_[i];
  c.noise_total = queues_[3].count;
  c.total = c.noise_total + c.signals[0] + c.signals[1] + c.signals[2];
  c.free_blocks = free_blocks_;
  c.total_blocks = block_count_;
  c.bytes_used_payload = payload_bytes_;
  c.allocated_arena_bytes = arena_bytes_;
  c.evicted_limit = evicted_limit_;
  c.evicted_memory = evicted_memory_;
  c.rejected = rejected_;
  return c;
}

}  // namespace rflink_logging
