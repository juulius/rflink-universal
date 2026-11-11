#include "esphome/components/rflink_universal/tx_engine.h"
#include "esphome/components/rflink_universal/tx_policy.h"
#include "esphome/components/rflink_universal/itho_codec.h"

namespace esphome { namespace rflink_universal { namespace tx_engine {

namespace {
bool parse_state(const DeviceRecord &d, uint32_t &address, uint32_t &sequence,
                 uint8_t &device_type, bool &paired) {
  const auto *address_text = tx::state(d, "itho_address");
  const auto *sequence_text = tx::state(d, "itho_sequence");
  const auto *device_type_text = tx::state(d, "itho_device_type");
  const auto *paired_text = tx::state(d, "itho_paired");
  uint32_t parsed_type = 0;
  if (!address_text || !sequence_text || !device_type_text || !paired_text ||
      !tx::unsigned_decimal(*address_text, 0xFFFFFFu, address) || address == 0 ||
      !tx::unsigned_decimal(*sequence_text, 0xFFFFFFFEu, sequence) || sequence == 0 || sequence >= 0xFFFFFFFEu ||
      !tx::unsigned_decimal(*device_type_text, 0xFFu, parsed_type) ||
      (*paired_text != "0" && *paired_text != "1")) return false;
  device_type = static_cast<uint8_t>(parsed_type);
  paired = *paired_text == "1";
  return true;
}
}  // namespace

class IthoRFTEncoder final : public Encoder {
 public:
  const char *protocol() const override { return "IthoRFT"; }
  const char *encoder_id() const override { return "itho_rft"; }

  bool prepare(const DeviceRecord &d, const std::string &action, Plan &out) const override {
    if (d.protocol != "IthoRFT" || !d.transmit_supported) return false;
    uint32_t address = 0, sequence = 0;
    uint8_t device_type = 0;
    bool paired = false;
    if (!parse_state(d, address, sequence, device_type, paired) || !paired) return false;

    uint8_t command = 0;
    uint8_t timer = 0;
    if (action == "standby") command = static_cast<uint8_t>(itho::SpeedCommand::STANDBY);
    else if (action == "low") command = static_cast<uint8_t>(itho::SpeedCommand::LOW);
    else if (action == "medium") command = static_cast<uint8_t>(itho::SpeedCommand::MEDIUM);
    else if (action == "high") command = static_cast<uint8_t>(itho::SpeedCommand::HIGH);
    else if (action == "timer_10") timer = 10;
    else if (action == "timer_20") timer = 20;
    else if (action == "timer_30") timer = 30;
    else return false;

    out.kind = FrameKind::PACKET;
    out.radio = 2;
    out.repeats = 3;
    out.repeat_gap_ms = 40;
    out.identity = address;
    // args: command (0 means timer), timer minutes, low 8 bits of logical sequence, device type.
    out.args = {{command, timer, static_cast<uint8_t>(sequence & 0xFFu), device_type}};
    out.state_updates = {{"itho_sequence", std::to_string(sequence + 1u), ""}};
    return true;
  }

  bool capture(const DeviceRecord &d, std::vector<FieldValue> &fields) const override {
    uint32_t address = 0, sequence = 0;
    uint8_t device_type = 0;
    bool paired = false;
    if (d.protocol != "IthoRFT" || !parse_state(d, address, sequence, device_type, paired) || !paired)
      return false;
    fields = {{"tx_lock_radio", "2", ""}};
    return true;
  }

  bool encode_packet(std::vector<uint8_t> &packet, const Plan &plan) const override {
    if (plan.kind != FrameKind::PACKET || plan.radio != 2 || plan.identity == 0 || plan.identity > 0xFFFFFFu)
      return false;
    const std::array<uint8_t, 3> address{{
        static_cast<uint8_t>((plan.identity >> 16u) & 0xFFu),
        static_cast<uint8_t>((plan.identity >> 8u) & 0xFFu),
        static_cast<uint8_t>(plan.identity & 0xFFu)}};
    std::vector<uint8_t> payload;
    if (plan.args[0] != 0) {
      if (plan.args[0] < static_cast<uint8_t>(itho::SpeedCommand::STANDBY) ||
          plan.args[0] > static_cast<uint8_t>(itho::SpeedCommand::HIGH)) return false;
      payload = itho::build_speed_payload(plan.args[3], address, plan.args[2],
                                           static_cast<itho::SpeedCommand>(plan.args[0]));
    } else {
      if (plan.args[1] != 10 && plan.args[1] != 20 && plan.args[1] != 30) return false;
      payload = itho::build_timer_payload(plan.args[3], address, plan.args[2], plan.args[1]);
    }
    packet = itho::encode_payload(payload, 0xAC);
    return packet.size() == 63;
  }
};

RFLINK_REGISTER_TX_ENCODER(IthoRFTEncoder);

}}}  // namespace esphome::rflink_universal::tx_engine
