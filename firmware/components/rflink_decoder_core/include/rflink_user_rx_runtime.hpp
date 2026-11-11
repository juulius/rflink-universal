#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include "rflink_user_library.hpp"

namespace rflink {

// Returns a decoder *recipe* match, not a saved physical-device identity,
// authentication, TX authorization, pairing or proof of on-air origin.
enum class UserRxOutcome : uint8_t { none=0, match=1, ambiguous=2 };

class UserRxRuntime {
 public:
  // Install ONCE at boot, before RX worker and HTTP provider publication.
  // Only tested, enabled RX programs are copied. This class is not a
  // concurrent mutable registry and has no NVS/ESPHome/CC1101 dependency.
  bool install(const UserLibrary& library, std::string& error);
  size_t enabled_count() const { return rules_.size(); }
  // Run only in the regular RX worker. Bounded copy of frame data is
  // made if and only if the channel/kind has an enabled matching recipe.
  // The result is only populated on one unambiguous recipe acceptance.
  UserRxOutcome match_raw(uint8_t radio, uint32_t frequency_hz,
                          const int32_t* data, size_t count,
                          DecodeResult& result) const;
  UserRxOutcome match_packet(uint8_t radio,uint32_t frequency_hz,
                             const uint8_t* data,size_t count,
                             DecodeResult& result) const;
 private:
  struct Rule {
    uint8_t radio=255;
    FrameKind kind=FrameKind::RAW;
    std::string protocol;
    user_vm::Program rx;
  };
  UserRxOutcome evaluate(const RFFrame& frame,DecodeResult& result) const;
  std::vector<Rule> rules_;
};

} // namespace rflink
