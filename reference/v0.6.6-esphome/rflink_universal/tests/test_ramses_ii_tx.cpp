#include "../decoders/ramses_ii/tx_codec.h"
#include <cassert>
#include <cstdio>
using namespace rflink_ramses_ii;
using namespace rflink_ramses_ii::tx;
static uint32_t makeId(uint8_t cls, uint32_t serial) {return (uint32_t(cls)<<18)|serial;}
static void wireVerify(const Frame& source) {
  std::vector<uint8_t> wire;
  assert(encodeWire(source,wire));
  Frame result;
  assert(decodeWire(wire.data(),wire.size(),result));
  assert(result.type == source.type);
  assert(result.command==source.command && result.payload_length==source.payload_length);
  for (unsigned i=0;i<source.payload_length;i++) assert(result.payload[i]==source.payload[i]);
  std::vector<uint8_t> packed;size_t bits=0;
  assert(encodeOnAir(source,5,packed,bits));
  // UART framing is bit-packed in the format accepted by existing decoder.
  assert(bits%10==0 && packed.size()*8 >= bits);
  Frame got;assert(decodePacked(packed.data(),packed.size(),got));
  assert(got.command==source.command);
  assert(got.payload_length==source.payload_length);
  for (unsigned i=0;i<source.payload_length;i++) assert(got.payload[i]==source.payload[i]);
}
int main() {
  const uint32_t src=makeId(37,123456), fan=makeId(32,82155);
  assert(deviceClass(src)==37 && deviceSerial(src)==123456);
  const Ids ids{src,fan};
  Frame f;
  for (auto action : {Action::AWAY, Action::LOW, Action::MEDIUM, Action::HIGH, Action::AUTO}) {
    assert(buildCommand(ids,action,f));
    assert(f.type==Frame::INFORMATION && f.command==0x22F1 && f.payload_length==3);
    assert(f.payload[0]==0 && f.payload[2]==4);
    assert(f.has_addr[0] && f.has_addr[1] && !f.has_addr[2]);
    wireVerify(f);
  }
  assert(buildCommand(ids,Action::AUTO,f) && f.payload[1]==4);
  for (auto action : {Action::TIMER_15,Action::TIMER_30,Action::TIMER_60}) {
    assert(buildCommand(ids,action,f) && f.command==0x22F3 && f.payload_length==7);
    assert(f.payload[0]==0 && f.payload[1]==2 && f.payload[3]==3);
    assert(f.payload[2]==(action==Action::TIMER_15?15:action==Action::TIMER_30?30:60));
    wireVerify(f);
  }
  // Not paired: prevent stale/wrong controller/fan identity from producing commands.
  assert(!buildCommand({fan,fan},Action::AUTO,f));
  assert(!buildCommand({src,0},Action::AUTO,f));
  assert(!buildCommand({src,src},Action::AUTO,f));
  assert(buildBindOffer(src,f));
  assert(f.command==0x1FC9 && f.payload_length==18 && f.payload[1]==0x22 && f.payload[2]==0xF1);
  assert(f.payload[7]==0x22 && f.payload[8]==0xF3 && f.payload[13]==0x10 && f.payload[14]==0xE0);
  wireVerify(f);
  assert(buildBindConfirm(src,fan,f) && f.payload_length==1 && f.payload[0]==0x21);
  wireVerify(f);
  assert(buildDeviceInfo(src,f) && f.command==0x10E0 && f.payload_length==38);
  assert(f.payload[7]==0x67);
  wireVerify(f);
  // Simulated W -- fan -> controller 1FC9 012 handshake ACK captured in documentation.
  Frame ack;ack.type=Frame::WRITE;ack.command=0x1FC9;
  ack.has_addr[0]=ack.has_addr[1]=true;ack.addr[0]=fan;ack.addr[1]=src;
  ack.payload_length=12;
  const uint8_t data[]={0x21,0x31,0xDA,uint8_t(fan>>16),uint8_t(fan>>8),uint8_t(fan),
                        0x6C,0x10,0xE0,uint8_t(fan>>16),uint8_t(fan>>8),uint8_t(fan)};
  // Real examples use an OEM/domain tag at payload[6] (0x6C); ignore in ACK check.
  for(size_t i=0;i<sizeof(data);i++)ack.payload[i]=data[i];
  assert(isBindAcknowledgement(ack,src,ack.addr[2]));
  wireVerify(ack);
  uint32_t inferred=0;
  assert(isBindAcknowledgement(ack,src,inferred) && inferred==fan);
  PairingSession s;
  assert(!s.begin(fan,100));
  assert(s.begin(src,100));
  assert(s.state()==PairingSession::State::OFFER_READY && s.makeOffer(f));
  s.offerSent(200);
  assert(s.state()==PairingSession::State::WAITING_ACK);
  assert(!s.verify(fan,210));
  Frame invalid=ack;invalid.addr[1]=makeId(37,999);
  assert(!s.receive(invalid,500) && s.fan()==0);
  assert(s.receive(ack,600) && s.fan()==fan);
  assert(s.makeConfirmation(f) && f.command==0x1FC9);
  s.confirmationSent(700);
  assert(s.state()==PairingSession::State::WAITING_VERIFICATION);
  assert(!s.verify(makeId(32,11),800));
  assert(s.verify(fan,900) && s.state()==PairingSession::State::VERIFIED);
  assert(s.begin(src,2000));s.offerSent(2001);s.checkTimeout(182100);
  assert(s.state()==PairingSession::State::EXPIRED && !s.receive(ack,182200));
  std::puts("RAMSES TX codec: 8 commands, bind offer/confirm/10E0, bit encoding/decoding, ACK guard, expiry: PASS");
}
