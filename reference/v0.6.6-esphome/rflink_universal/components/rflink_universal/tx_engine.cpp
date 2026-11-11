#include "tx_engine.h"
#include "capability_catalog.h"
#include "profile_catalog.h"
#include "radio_v2_catalog.h"
#include <algorithm>
#include <cstring>
#if defined(__has_include)
#  if __has_include("esphome/core/defines.h")
#    include "esphome/core/defines.h"
#  endif
#endif
#if defined(RFLINK_TX_ENCODER_EXPECTED_COUNT) && RFLINK_TX_ENCODER_EXPECTED_COUNT > 0
#  include "../../../rflink_decoders/tx_encoder_anchors_generated.h"
#  define RFLINK_HAS_TX_ENCODER_ANCHORS 1
#endif
namespace esphome { namespace rflink_universal { namespace tx_engine {
static std::vector<Encoder *> &all() { static std::vector<Encoder *> encoders; return encoders; }
void register_once(Encoder *p) {
  if (p && std::find(all().begin(),all().end(),p)==all().end()) all().push_back(p);
}
void ensure_linked() {
#ifdef RFLINK_HAS_TX_ENCODER_ANCHORS
  rflink_generated_force_link_tx_encoders();
#endif
}
// A chosen, built-in, source-backed profile can authorize *known* commands for
// the two virtual-remote encoders that generate independent complete messages.
// These still require pairing, current radio state, TX opt-in in the core and
// the encoder's own strict protocol validation. Other encoders (e.g. Dooya)
// require observing each individual button/check nibble and remain unchanged.
bool reviewed_profile_action(const DeviceRecord &d,const std::string &action) {
  if (d.protocol!="SomfyRTS56" && d.protocol!="IthoRFT" && d.protocol!="RamsesII") return false;
  const auto *p=profiles::find(d.profile_id.c_str());
  if(!p || d.protocol!=p->protocol) return false;
  if(d.protocol=="SomfyRTS56" && action=="prog") return true; // explicit pairing endpoint only
  const std::string allowed="|"+std::string(p->controls)+"|";
  return allowed.find("|"+action+"|")!=std::string::npos;
}
const Encoder *resolve(const std::string &protocol) {
  const auto *cap=capabilities::find(protocol.c_str());
  if (!cap || std::strcmp(cap->encoder,"none")==0 || std::strcmp(cap->tx_status,"disabled")==0) return nullptr;
  for (const auto *encoder:all()) if (encoder && protocol==encoder->protocol() &&
      std::strcmp(cap->encoder,encoder->encoder_id())==0) return encoder;
  return nullptr;
}
bool prepare(const DeviceRecord &d, const std::string &action, Plan &plan, const Encoder *&encoder) {
  encoder=resolve(d.protocol);
  if (!encoder || !d.transmit_supported || !capabilities::tx_allowed(d.protocol.c_str(),action.c_str()) ||
      (std::find(d.actions.begin(),d.actions.end(),action)==d.actions.end() &&
       !reviewed_profile_action(d,action))) return false;
  Plan candidate;
  if (!encoder->prepare(d,action,candidate) || candidate.radio>=3 || candidate.repeats==0 ||
      candidate.repeats>10 || candidate.repeat_gap_ms>1000) return false;
  // The manifest binds the encoder to an approved physical channel and TX
  // adapter; the radio hardware decides availability independently. A new
  // decoder does not require a protocol-specific branch in the core.
  const auto *cap=capabilities::find(d.protocol.c_str());
  const uint32_t approved_frequency = cap && std::strcmp(cap->tx_radio,"433.42")==0 ? 433420000U :
      cap && std::strcmp(cap->tx_radio,"433.92")==0 ? 433920000U :
      cap && std::strcmp(cap->tx_radio,"868.30")==0 ? 868299900U : 0U;
  if (approved_frequency == 0) return false;
  const auto *tx_spec = radio_v2_catalog::tx_for(d.protocol.c_str());
  if (!tx_spec || !tx_spec->enabled || tx_spec->channel != candidate.radio ||
      (!tx_transport_kind_compatible(tx_spec->preferred,candidate.kind) &&
       !tx_transport_kind_compatible(tx_spec->fallback,candidate.kind))) return false;
  candidate.transport=tx_transport_kind_compatible(tx_spec->preferred,candidate.kind) ?
      tx_spec->preferred : tx_spec->fallback;
  candidate.protocol=d.protocol;
  candidate.frequency_hz=approved_frequency;
  plan=candidate;
  return true;
}
bool eligible(const DeviceRecord &d) {
  for (const auto &action:d.actions) {
    Plan plan; const Encoder *encoder=nullptr;
    if (prepare(d,action,plan,encoder)) return true;
  }
  const auto *p=profiles::find(d.profile_id.c_str());
  if (p && d.protocol==p->protocol) {
    std::string controls=p->controls;
    size_t start=0;
    while(start<controls.size()) {
      const size_t end=controls.find('|',start);
      const auto command=controls.substr(start,end==std::string::npos ? end : end-start);
      Plan plan; const Encoder *encoder=nullptr;
      if(prepare(d,command,plan,encoder))return true;
      if(end==std::string::npos)break;
      start=end+1;
    }
  }
  return false;
}
}}}
