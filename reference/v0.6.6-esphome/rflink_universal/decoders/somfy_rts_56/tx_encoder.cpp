#include "esphome/components/rflink_universal/tx_engine.h"
#include "esphome/components/rflink_universal/tx_policy.h"
#include "esphome/components/rflink_universal/somfy_codec.h"

namespace esphome { namespace rflink_universal { namespace tx_engine {

class SomfyRTS56Encoder final : public Encoder {
 public:
  const char *protocol() const override { return "SomfyRTS56"; }
  const char *encoder_id() const override { return "somfy_rts_56"; }

  bool prepare(const DeviceRecord &d, const std::string &action, Plan &out) const override {
    if (d.protocol != "SomfyRTS56" || !d.transmit_supported) return false;
    somfy::Command command;
    if (!somfy::command_from_name(action, command)) return false;

    const auto *address_text = tx::state(d, "somfy_address");
    const auto *counter_text = tx::state(d, "somfy_rolling_code");
    const auto *paired_text = tx::state(d, "somfy_paired");
    uint32_t address = 0, counter = 0;
    if (!address_text || !counter_text || !paired_text ||
        !tx::unsigned_decimal(*address_text, 0xFFFFFFu, address) || address == 0 ||
        !tx::unsigned_decimal(*counter_text, 0xFFFFu, counter) || counter == 0 || counter >= 0xFFFFu ||
        (*paired_text != "0" && *paired_text != "1")) return false;
    // PROG is the one command allowed before RFLink assumes pairing succeeded.
    if (action != "prog" && *paired_text != "1") return false;

    out.radio = 0;
    out.repeats = 1;  // encode() contains the Somfy first frame + repeat frames itself.
    out.identity = address;
    out.args = {{static_cast<uint8_t>(command), static_cast<uint8_t>(counter >> 8u),
                 static_cast<uint8_t>(counter & 0xFFu), 0xA7}};
    out.state_updates = {{"somfy_rolling_code", std::to_string(counter + 1u), ""}};
    if (action == "prog" && *paired_text == "0")
      out.state_updates.push_back({"somfy_paired", "1", ""});
    return true;
  }

  bool capture(const DeviceRecord &d, std::vector<FieldValue> &fields) const override {
    // The virtual Somfy identity lives in tx_state and does not depend on the
    // received handset. Keep only the physical radio selection as an opt-in lock.
    if (d.protocol != "SomfyRTS56" || !tx::state(d, "somfy_address") ||
        !tx::state(d, "somfy_rolling_code") || !tx::state(d, "somfy_paired")) return false;
    fields = {{"tx_lock_radio", "0", ""}};
    return true;
  }

  bool encode_raw(remote_base::RemoteTransmitData *out, const Plan &plan) const override {
    somfy::Frame56 frame;
    frame.key = plan.args[3];
    frame.command = static_cast<somfy::Command>(plan.args[0]);
    frame.rolling_code = static_cast<uint16_t>((uint16_t(plan.args[1]) << 8u) | plan.args[2]);
    frame.address = static_cast<uint32_t>(plan.identity & 0xFFFFFFu);
    for (const auto pulse : somfy::encode_raw_56(frame, 3)) {
      if (pulse > 0) out->mark(static_cast<uint32_t>(pulse));
      else out->space(static_cast<uint32_t>(-pulse));
    }
    return true;
  }
};

RFLINK_REGISTER_TX_ENCODER(SomfyRTS56Encoder);

}}}  // namespace esphome::rflink_universal::tx_engine
