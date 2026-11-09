#pragma once
// Read-only bridge from the working v0.6.6 ESPHome CC1101 868 packet
// callback to the native v0.7 decoder core. No SPI, GPIO, RMT, UART or TX.
#include <cstdint>
#include <vector>

namespace rflink_hybrid {
struct ProbeCounters {
  uint32_t packet_inputs = 0;
  uint32_t itho_messages = 0;
  uint32_t ramses_messages = 0;
  uint32_t unknown_packet_inputs = 0;
  uint32_t rejected_inputs = 0;
  uint32_t overlapping_decodes = 0;
};

// Called AFTER the existing v0.6.6 callback. Does not modify the packet.
void observe_868_packet(const std::vector<uint8_t>& packet);
ProbeCounters counters();
}  // namespace rflink_hybrid
