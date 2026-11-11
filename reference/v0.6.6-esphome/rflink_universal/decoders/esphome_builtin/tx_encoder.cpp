// Protocol-specific encoders live WITH their decoder pack; no protocol switch
// is required in RFLink core when adding another reviewed encoder.
#include "esphome/components/rflink_universal/tx_engine.h"
#include "esphome/components/rflink_universal/tx_policy.h"
#include "esphome/components/remote_base/nexa_protocol.h"
#include "esphome/components/remote_base/byronsx_protocol.h"
#include "esphome/components/remote_base/dooya_protocol.h"
#include "esphome/components/remote_base/rc_switch_protocol.h"
#include "esphome/components/rflink_universal/format_utils.h"

namespace esphome { namespace rflink_universal { namespace tx_engine {
class NexaEncoder final : public Encoder {
 public:
  const char *protocol() const override { return "Nexa"; }
  const char *encoder_id() const override { return "nexa"; }
  bool prepare(const DeviceRecord &d, const std::string &action, Plan &out) const override {
    tx::NexaCommand code;
    if (!tx::nexa(d,action,code)) return false;
    out.radio=1;out.repeats=3;out.identity=code.device;
    out.args={{code.channel,code.group,code.level,static_cast<uint8_t>(code.state)}};
    return true;
  }
  bool capture(const DeviceRecord &d, std::vector<FieldValue> &fields) const override {
    tx::NexaCommand code;
    if (!tx::nexa(d,"on",code) && !tx::nexa(d,"off",code)) return false;
    fields={{"tx_lock_radio","1",""},{"tx_lock_channel",std::to_string(code.channel),""},
            {"tx_lock_group",code.group?"true":"false",""},
            {"tx_lock_level",std::to_string(code.level),""}};
    return true;
  }
  bool encode_raw(remote_base::RemoteTransmitData *out, const Plan &plan) const override {
    remote_base::NexaData code{};
    code.device=static_cast<uint32_t>(plan.identity);code.channel=plan.args[0];
    code.group=plan.args[1];code.level=plan.args[2];code.state=plan.args[3]!=0;
    remote_base::NexaProtocol().encode(out,code);
    return true;
  }
};
RFLINK_REGISTER_TX_ENCODER(NexaEncoder);

// Byron SX: the received address and four-bit chime command are a fixed code.
// Only an individually approved received command may be transmitted; 0x10 is
// the wildcard used by ESPHome matching and is NEVER a valid TX command.
class ByronEncoder final : public Encoder {
 public:
  const char *protocol() const override { return "ByronSX"; }
  const char *encoder_id() const override { return "byron_sx"; }
  bool prepare(const DeviceRecord &d, const std::string &action, Plan &out) const override {
    if (d.protocol!="ByronSX" || d.identity_confidence!=IdentityConfidence::HIGH ||
        d.rf_identities.size()!=1 || !d.transmit_supported ||
        action.compare(0,8,"command_")!=0) return false;
    const auto *radio=tx::field(d,d.transmit_enabled?"tx_lock_radio":"tx_radio");
    if (!radio || *radio!="1") return false;
    const auto &identity=d.rf_identities.front();
    if (identity.size()!=10 || identity.compare(0,8,"byronsx:")!=0) return false;
    uint8_t address=0;
    for(size_t i=8;i<10;i++) {
      const char digit=identity[i];
      const int value=digit>='0'&&digit<='9'?digit-'0':digit>='A'&&digit<='F'?digit-'A'+10:-1;
      if (value<0) return false;
      address=static_cast<uint8_t>(address*16+value);
    }
    const auto text=action.substr(8);
    uint8_t command=0;
    if (text.empty() || text.size()>2 || (text.size()>1 && text.front()=='0')) return false;
    for(char ch:text) {
      if (ch<'0'||ch>'9') return false;
      command=static_cast<uint8_t>(command*10+(ch-'0'));
    }
    if(command>15 || std::find(d.actions.begin(),d.actions.end(),action)==d.actions.end()) return false;
    out.radio=1;out.repeats=3;out.identity=address;out.args={{command,0,0,0}};
    return true;
  }
  bool capture(const DeviceRecord &d, std::vector<FieldValue> &fields) const override {
    if (!eligible_action(d)) return false;
    fields={{"tx_lock_radio","1",""}};
    return true;
  }
  bool encode_raw(remote_base::RemoteTransmitData *out, const Plan &plan) const override {
    remote_base::ByronSXData data{};
    data.address=static_cast<uint8_t>(plan.identity);
    data.command=plan.args[0];
    remote_base::ByronSXProtocol().encode(out,data);
    return true;
  }
 private:
  bool eligible_action(const DeviceRecord &d) const {
    for(const auto &action:d.actions) {
      Plan p;
      if(this->prepare(d,action,p)) return true;
    }
    return false;
  }
};
RFLINK_REGISTER_TX_ENCODER(ByronEncoder);
// Dooya: transmit ONLY a button whose exact check nibble was observed and
// individually approved. Freeze the channel and each approved check on opt-in;
// subsequent RF traffic must never redirect a Home Assistant command.
class DooyaEncoder final : public Encoder {
 public:
  const char *protocol() const override { return "Dooya"; }
  const char *encoder_id() const override { return "dooya"; }
  bool prepare(const DeviceRecord &d, const std::string &action, Plan &out) const override {
    if (d.protocol!="Dooya" || !d.transmit_supported ||
        d.identity_confidence!=IdentityConfidence::HIGH || d.rf_identities.size()!=1 ||
        action.compare(0,7,"button_")!=0 ||
        std::find(d.actions.begin(),d.actions.end(),action)==d.actions.end()) return false;
    const std::string suffix=action.substr(7);
    uint8_t button=0;
    if (!tx::decimal(suffix,15,button) || suffix!=std::to_string(button)) return false;
    const std::string &identity=d.rf_identities.front();
    if (identity.size()!=12 || identity.compare(0,6,"dooya:")!=0) return false;
    uint32_t remote_id=0;
    for (size_t n=6;n<identity.size();n++) {
      const char c=identity[n];
      const int nibble=c>='0'&&c<='9'?c-'0':c>='A'&&c<='F'?c-'A'+10:-1;
      if (nibble<0) return false;
      remote_id=(remote_id<<4)|static_cast<unsigned>(nibble);
    }
    const bool locked=d.transmit_enabled;
    const auto *radio=tx::field(d,locked?"tx_lock_radio":"tx_radio");
    const auto *channel=tx::field(d,locked?"tx_lock_channel":"channel");
    const std::string check_key=(locked?"tx_lock_check_button_":"check_button_")+suffix;
    const auto *check=tx::field(d,check_key.c_str());
    uint8_t ch=0,ck=0;
    if (!radio || *radio!="1" || !channel || !tx::decimal(*channel,255,ch) ||
        !check || !tx::decimal(*check,15,ck)) return false;
    out.radio=1;out.repeats=3;out.identity=remote_id;
    out.args={{ch,button,ck,0}};
    return true;
  }
  bool capture(const DeviceRecord &d, std::vector<FieldValue> &fields) const override {
    if (d.actions.empty()) return false;
    Plan p;
    for (const auto &action:d.actions) {
      if (!prepare(d,action,p)) return false; // all published actions need their OWN approved checks
    }
    const auto *channel=tx::field(d,"channel");
    fields={{"tx_lock_radio","1",""},{"tx_lock_channel",*channel,""}};
    for (const auto &action:d.actions) {
      const auto *check=tx::field(d,("check_"+action).c_str());
      if (!check) return false;
      fields.push_back({"tx_lock_check_"+action,*check,""});
    }
    return true;
  }
  bool encode_raw(remote_base::RemoteTransmitData *out,const Plan &plan) const override {
    remote_base::DooyaData data{};
    data.id=static_cast<uint32_t>(plan.identity);data.channel=plan.args[0];
    data.button=plan.args[1];data.check=plan.args[2];
    remote_base::DooyaProtocol().encode(out,data);
    return true;
  }
};
RFLINK_REGISTER_TX_ENCODER(DooyaEncoder);

// RC-Switch: no attempt to infer paired on/off receivers or to replay a raw
// recording. Only a confirmed fixed telegram (code+protocol+bit count) may TX.
class RCSwitchEncoder final : public Encoder {
 public:
  const char *protocol() const override { return "RCSwitch"; }
  const char *encoder_id() const override { return "rc_switch"; }
  bool prepare(const DeviceRecord &d, const std::string &action, Plan &out) const override {
    if (d.protocol!="RCSwitch" || !d.transmit_supported || action!="code" ||
        d.rf_identities.size()!=1 || d.actions.size()!=1 || d.actions.front()!=action) return false;
    const bool locked=d.transmit_enabled;
    const auto *radio=tx::field(d,locked?"tx_lock_radio":"tx_radio");
    const auto *proto=tx::field(d,locked?"tx_lock_protocol":"protocol");
    const auto *bits=tx::field(d,locked?"tx_lock_bit_length":"bit_length");
    const auto *code=tx::field(d,locked?"tx_lock_code":"code");
    uint8_t p=0,n=0;
    if (!radio || *radio!="1" || !proto || !tx::decimal(*proto,8,p) || p<1 ||
        !bits || !tx::decimal(*bits,64,n) || n<8 || !code || code->empty() || code->size()>20) return false;
    uint64_t value=0;
    for (char c:*code) {
      if(c<'0'||c>'9') return false;
      const unsigned digit=static_cast<unsigned>(c-'0');
      if (value > (UINT64_MAX-digit)/10) return false;
      value=value*10+digit;
    }
    if (std::to_string(value)!=*code ||
        d.rf_identities.front()!=hex_id("rcswitch",value,1) ||
        (n<64 && value >= (uint64_t{1} << n))) return false;
    out.radio=1;out.repeats=3;out.identity=value;out.args={{p,n,0,0}};
    return true;
  }
  bool capture(const DeviceRecord &d,std::vector<FieldValue> &fields) const override {
    Plan p;
    if (!prepare(d,"code",p)) return false;
    fields={{"tx_lock_radio","1",""},
            {"tx_lock_protocol",std::to_string(p.args[0]),""},
            {"tx_lock_bit_length",std::to_string(p.args[1]),""},
            {"tx_lock_code",std::to_string(p.identity),""}};
    return true;
  }
  bool encode_raw(remote_base::RemoteTransmitData *out,const Plan &plan) const override {
    remote_base::RC_SWITCH_PROTOCOLS[plan.args[0]].transmit(out,plan.identity,plan.args[1]);
    return true;
  }
};
RFLINK_REGISTER_TX_ENCODER(RCSwitchEncoder);

}}} // namespaces
