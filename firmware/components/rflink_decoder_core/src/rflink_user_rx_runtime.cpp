#include "rflink_user_rx_runtime.hpp"
#include <algorithm>
#include <set>
#include <utility>

namespace rflink {

bool UserRxRuntime::install(const UserLibrary& library,std::string& error) {
  error.clear();
  if(library.decoders().size()>UserLibrary::MAX_DECODERS) {
    error="User decoder library exceeds the safe limit";return false;
  }
  std::vector<Rule> proposed;
  std::set<std::string> seen;
  for(const auto& decoder:library.decoders()) {
    if(!decoder.enabled)continue;
    if(!user_vm::identifier(decoder.id) ||
       decoder.id.rfind("user_",0)!=0 ||
       !seen.insert(decoder.id).second ||
       decoder.radio>2 ||
       ((decoder.radio==2)!=(decoder.kind==FrameKind::PACKET)) ||
       (decoder.kind!=FrameKind::RAW && decoder.kind!=FrameKind::PACKET)) {
      error="Invalid active user decoder metadata";return false;
    }
    Rule next{};
    next.radio=decoder.radio;
    next.kind=decoder.kind;
    next.protocol=decoder.id;
    if(!user_vm::compile(decoder.rx_source,user_vm::Mode::RX,next.rx,error))
      return false;
    // An enabled decoder must reproduce its labelled captures BEFORE it is
    // eligible to classify any live RF signal. No on-air TX ever runs here.
    UserDecoder checked=decoder;
    checked.rx=next.rx;
    if(!library.replay(checked,error))return false;
    proposed.push_back(std::move(next));
  }
  rules_=std::move(proposed);
  return true;
}

UserRxOutcome UserRxRuntime::evaluate(const RFFrame& frame,DecodeResult& result) const {
  result=DecodeResult{};
  unsigned matches=0;
  DecodeResult only{};
  for(const auto& rule:rules_) {
    if(rule.radio!=frame.channel || rule.kind!=frame.kind)continue;
    DecodeResult candidate;
    std::string why;
    if(!user_vm::receive(rule.rx,rule.protocol,frame,candidate,why) ||
       !candidate.valid)continue;
    if(++matches==1)only=std::move(candidate);
    if(matches>1)return UserRxOutcome::ambiguous;
  }
  if(matches==1) {
    // No user-program TX path or profile/pairing capability.
    only.transmit_supported=false;
    result=std::move(only);
    return UserRxOutcome::match;
  }
  return UserRxOutcome::none;
}

UserRxOutcome UserRxRuntime::match_raw(uint8_t radio,uint32_t frequency_hz,
                                      const int32_t* data,size_t count,
                                      DecodeResult& result) const {
  result=DecodeResult{};
  if(!data || count<2 || count>512 ||
     !((radio==0 && frequency_hz==433420000u) ||
       (radio==1 && frequency_hz==433920000u)))
    return UserRxOutcome::none;
  if(!std::any_of(rules_.begin(),rules_.end(),
       [radio](const Rule& rule){return rule.radio==radio && rule.kind==FrameKind::RAW;}))
    return UserRxOutcome::none;
  std::vector<int32_t> samples(data,data+count);
  RFFrame frame{};
  frame.kind=FrameKind::RAW;
  frame.channel=radio;
  frame.frequency_hz=frequency_hz;
  frame.raw=&samples;
  return evaluate(frame,result);
}

UserRxOutcome UserRxRuntime::match_packet(uint8_t radio,uint32_t frequency_hz,
                                         const uint8_t* data,size_t count,
                                         DecodeResult& result) const {
  result=DecodeResult{};
  if(!data || radio!=2 || frequency_hz<868200000u ||
     frequency_hz>868400000u || count!=63)
    return UserRxOutcome::none;
  if(!std::any_of(rules_.begin(),rules_.end(),
       [](const Rule& rule){return rule.radio==2 && rule.kind==FrameKind::PACKET;}))
    return UserRxOutcome::none;
  std::vector<uint8_t> bytes(data,data+count);
  RFFrame frame{};
  frame.kind=FrameKind::PACKET;
  frame.channel=radio;
  frame.frequency_hz=frequency_hz;
  frame.packet=&bytes;
  return evaluate(frame,result);
}

} // namespace rflink
