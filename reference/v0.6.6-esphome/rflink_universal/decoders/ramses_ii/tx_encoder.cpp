#include "esphome/components/rflink_universal/tx_engine.h"
#include "esphome/components/rflink_universal/tx_policy.h"
#include "tx_codec.h"
namespace esphome { namespace rflink_universal { namespace tx_engine {
class RamsesIIEncoder final : public Encoder {
 public:
  const char *protocol() const override { return "RamsesII"; }
  const char *encoder_id() const override { return "ramses_ii"; }
  static bool read_state(const DeviceRecord &d,uint32_t &controller,uint32_t &fan) {
    const auto *c=tx::state(d,"ramses_controller"),*f=tx::state(d,"ramses_fan"),*p=tx::state(d,"ramses_paired");
    return d.profile_id=="orcon_mvs15r_15rf" && c && f && p && *p=="1" &&
      tx::unsigned_decimal(*c,0xFFFFFFU,controller) && controller &&
      rflink_ramses_ii::deviceClass(controller)==37 &&
      tx::unsigned_decimal(*f,0xFFFFFFU,fan) && fan && controller!=fan;
  }
  bool prepare(const DeviceRecord &d,const std::string &action,Plan &out) const override {
    if(d.protocol!="RamsesII" || !d.transmit_supported)return false;
    uint32_t controller=0,fan=0;
    if(!read_state(d,controller,fan))return false;
    uint8_t cmd=255;
    if(action=="away")cmd=0;
    else if(action=="low")cmd=1;
    else if(action=="medium")cmd=2;
    else if(action=="high")cmd=3;
    else if(action=="auto")cmd=4;
    else if(action=="timer_15")cmd=5;
    else if(action=="timer_30")cmd=6;
    else if(action=="timer_60")cmd=7;
    else return false;
    out.kind=FrameKind::PACKET;out.transport=TxTransport::GDO_SERIAL;
    out.radio=2;out.repeats=1;out.repeat_gap_ms=0;
    out.identity=controller;out.args={{uint8_t(fan>>16),uint8_t(fan>>8),uint8_t(fan),cmd}};
    return true;
  }
  bool capture(const DeviceRecord &d,std::vector<FieldValue> &fields) const override {
    uint32_t controller=0,fan=0;
    if(!read_state(d,controller,fan))return false;
    fields={{"tx_lock_radio","2",""}};return true;
  }
  bool encode_serial(std::vector<uint8_t> &wire,const Plan &plan) const override {
    if(plan.kind!=FrameKind::PACKET || plan.radio!=2 || plan.args[3]>7 ||
        plan.identity>0xFFFFFFU || !plan.identity)return false;
    const uint32_t fan=uint32_t(plan.args[0])<<16 | uint32_t(plan.args[1])<<8 | uint32_t(plan.args[2]);
    rflink_ramses_ii::Frame frame;
    if(!rflink_ramses_ii::tx::buildCommand({uint32_t(plan.identity),fan},
         static_cast<rflink_ramses_ii::tx::Action>(plan.args[3]),frame) ||
         !rflink_ramses_ii::tx::encodeWire(frame,wire))return false;
    wire.insert(wire.begin(),5,0xFF);return true;
  }
};
RFLINK_REGISTER_TX_ENCODER(RamsesIIEncoder);
}}} // namespaces
