#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "decoder.h"

namespace esphome {
namespace rflink_universal {

struct DeveloperCandidate {
  uint32_t hash{0};
  uint16_t observations{0};
  uint32_t first_seen_ms{0};
  uint32_t last_seen_ms{0};
  uint16_t pulse_count{0};
  FrameKind frame_kind{FrameKind::RAW};
  float best_rssi_dbm{-127.0f};
  bool has_rssi{false};
  bool was_in_baseline{false};
  std::string protocol;
  std::string identity_key;
  std::string action;
  uint8_t decoder_confidence{0};
};

struct CapturedFrame {
  uint8_t channel{0};
  FrameKind kind{FrameKind::RAW};
  std::string label;
  uint32_t hash{0};
  std::vector<int32_t> raw;
  std::vector<uint8_t> packet;
};

class DeveloperSession {
 public:
  void reset();
  void start_baseline(uint8_t channel);
  void start_action(uint8_t channel, const std::string &label);
  void finish_baseline();
  void observe(uint32_t hash, const RFFrame &frame, uint16_t pulse_count);
  void annotate(uint32_t hash, const DecodeResult &result, uint8_t decoder_confidence);

  bool baseline_mode() const { return baseline_mode_; }
  uint8_t channel() const { return channel_; }
  const std::string &label() const { return label_; }
  const std::vector<DeveloperCandidate> &candidates() const { return candidates_; }
  const std::vector<CapturedFrame> &captures() const { return captures_; }
  const DeveloperCandidate *best_new_candidate() const;
  std::string generate_raw_decoder_snippet(const DeveloperCandidate &candidate) const;
  std::string generate_packet_decoder_snippet(const std::vector<uint8_t> &packet) const;
  std::string generate_packet_decoder_snippet(const DeveloperCandidate &candidate) const;

 private:
  bool baseline_contains_(uint32_t hash) const;
  uint8_t channel_{0};
  bool baseline_mode_{false};
  bool capturing_{false};
  std::string label_;
  std::vector<uint32_t> baseline_hashes_;
  std::vector<DeveloperCandidate> candidates_;
  std::vector<CapturedFrame> captures_;
};

}  // namespace rflink_universal
}  // namespace esphome
