#include "hybrid_probe.hpp"
#include "rflink_native_decoder.hpp"

namespace rflink_hybrid {
namespace {
// The native decoder is an observer only. ESPHome retains sole ownership of
// packet capture, CC1101 state and both 433 MHz RMT receivers.
rflink::NativeDecoder& decoder() {
  static rflink::NativeDecoder instance;
  return instance;
}
}  // namespace

void observe_868_packet(const std::vector<uint8_t>& packet) {
  const auto* bytes = packet.empty() ? nullptr : packet.data();
  decoder().on_packet(rflink::RadioId::rf86830, 868299900u,
                      bytes, packet.size());
}

ProbeCounters counters() {
  const auto result = decoder().snapshot();
  ProbeCounters counters{};
  counters.packet_inputs = result.packet_inputs;
  counters.itho_messages = result.itho_messages;
  counters.ramses_messages = result.ramses_messages;
  counters.unknown_packet_inputs = result.unknown_packet_inputs;
  counters.rejected_inputs = result.rejected_inputs;
  counters.overlapping_decodes = result.overlapping_decodes;
  return counters;
}
}  // namespace rflink_hybrid
