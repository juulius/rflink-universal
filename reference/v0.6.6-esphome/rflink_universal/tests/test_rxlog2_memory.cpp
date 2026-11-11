#include "rflink_log_store.h"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

using namespace rflink_logging;
int main() {
  std::vector<uint8_t> arena(kDefaultArenaBytes);
  Store store(arena.data(), arena.size());
  assert(store.ready());
  Limits lim{};
  assert(store.set_limits(lim));
  const std::string special = R"({"summary":"quote \" ok","detail":"abc\\def","full_detail":"RX"})";
  assert(store.add(0, Kind::Signal, 0, 11, special)==AddResult::Saved);
  const std::string maximal = "{\"full_detail\":\"" + std::string(30000, 'A') + "\"}";
  assert(maximal.size() < kMaxEventJsonBytes);
  assert(store.add(1, Kind::Signal, 0, 12, maximal)==AddResult::Saved);
  for (uint32_t i=0;i<3500;i++) {
    std::string noise = "{\"noise_suspect\":true,\"raw\":\"" + std::to_string(i) + std::string(190, 'Z') + "\"}";
    assert(store.add(i%3, Kind::Noise, 0, i+100, noise)==AddResult::Saved);
  }
  const auto counts = store.counters();
  assert(counts.signals[0]==1 && counts.signals[1]==1);
  assert(counts.noise_total<=2000);
  assert(counts.bytes_used_payload<kDefaultArenaBytes);
  assert(counts.evicted_limit[3]+counts.evicted_memory[3]>0);
  const Filter only_signal{-1,0};
  auto meta = store.read_before(UINT64_MAX, only_signal, 20, false);
  assert(meta.size()==2);
  assert(meta[0].json.empty() && meta[1].json.empty());
  assert(meta[0].json_bytes==maximal.size());
  std::vector<char> scratch(kMaxEventJsonBytes);
  size_t written=0;
  assert(store.copy_json(meta[0].sequence, scratch.data(), scratch.size(), written));
  assert(written==maximal.size());
  assert(memcmp(scratch.data(),maximal.data(),written)==0);
  assert(store.copy_json(meta[1].sequence, scratch.data(), scratch.size(), written));
  assert(written==special.size());
  assert(memcmp(scratch.data(),special.data(),written)==0);
  const auto noise_page=store.read_before(UINT64_MAX,Filter{-1,1},21,false);
  assert(noise_page.size()==21);
  assert(noise_page[0].sequence>noise_page[20].sequence);
  const auto next=store.read_before(noise_page[19].sequence,Filter{-1,1},21,false);
  assert(next.size()==21 && next.front().sequence<noise_page[19].sequence);
  assert(!store.copy_json(UINT64_MAX, scratch.data(), scratch.size(), written));
  std::cout << "RXLOG2: 30KB RAW payload, metadata paging, exact PSRAM scratch copy, noise eviction priority: PASS\n";
}
