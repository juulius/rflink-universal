#include "../components/rflink_universal/ramses_spi_fifo.h"
#include "ramses_ii_fixtures.h"
#include <cassert>
#include <cstdio>
#include <deque>
#include <vector>
using namespace esphome::rflink_universal::ramses_spi;
using namespace rflink_ramses_ii;

struct MockBus: Bus {
  uint8_t regs[64]{};
  std::deque<uint8_t> rx;
  std::deque<uint8_t> tx;
  std::vector<uint8_t> sent;
  unsigned writes=0;
  unsigned strobes=0;
  bool io_ok=true;
  bool simulate_tx=false;
  bool underflow=false;
  bool bad_readback=false;
  uint8_t marc=1;
  bool read(uint8_t reg,uint8_t& value) override {
    if(!io_ok)return false;
    if(reg==0x3B) value=uint8_t(rx.size())|(rx.size()>64?0x80:0);
    else if(reg==0x3A) value=uint8_t(tx.size())|(underflow?0x80:0);
    else if(reg==0x35) value=marc;
    else value=regs[reg];
    if(bad_readback && reg==0x11) value^=1;
    return true;
  }
  bool write(uint8_t reg,uint8_t v) override {if(!io_ok)return false;regs[reg]=v;++writes;return true;}
  bool strobe(uint8_t cmd) override {
    if(!io_ok)return false;
    ++strobes;
    if(cmd==0x36)marc=1;
    if(cmd==0x34)marc=0x0d;
    if(cmd==0x35) {marc=0x13;simulate_tx=true;}
    if(cmd==0x3A)rx.clear();
    if(cmd==0x3B)tx.clear();
    return true;
  }
  bool read_fifo(uint8_t* out,size_t count)override {
    if(!io_ok||count>rx.size())return false;
    while(count--){*out++=rx.front();rx.pop_front();}
    return true;
  }
  bool write_fifo(const uint8_t* in,size_t n)override {
    if(!io_ok||n+tx.size()>64)return false;
    for(size_t i=0;i<n;++i)tx.push_back(in[i]);
    return true;
  }
  void tick_tx(unsigned n=10) {
    if(!simulate_tx)return;
    while(n--&&!tx.empty()) {sent.push_back(tx.front());tx.pop_front();}
    if(tx.empty() && sent.size() && (sent.size()==size_t(sent[0]+1)))marc=1;
  }
};

int main(){
  const uint32_t remote=(37u<<18)|123456u;
  const uint32_t fan=(29u<<18)|204810u;
  Frame f, out;
  assert(tx::buildCommand({remote,fan},tx::Action::AUTO,f));
  std::vector<uint8_t> spi;
  assert(FifoTransport::make_fifo_payload(f,spi));
  assert(spi.size()>10 && FifoTransport::parse_fifo_capture(spi.data(),spi.size(),out));
  assert(out.command==0x22F1 && out.payload[1]==4);
  std::vector<uint8_t> corrupt=spi;
  corrupt[corrupt.size()-7]^=0x07;
  assert(!FifoTransport::parse_fifo_capture(corrupt.data(),corrupt.size(),out));

  MockBus hw;
  hw.regs[0x12]=0x12; hw.regs[0x10]=0x6A;hw.regs[0x17]=0x30;
  FifoTransport engine(hw);
  assert(engine.start_rx(100) && engine.state()==FifoTransport::State::RX);
  assert(hw.regs[4]==0xB3 && hw.regs[5]==0x2A && hw.regs[8]==0x02);
  // Streaming without a fixed 63-byte assumption: max 40 bytes per poll.
  size_t pos=0;unsigned reads=0;
  while(pos<spi.size()) {
    const size_t n=std::min<size_t>(10,spi.size()-pos);
    for(size_t i=0;i<n;++i)hw.rx.push_back(spi[pos+i]);
    pos+=n;
    const bool got=engine.poll_rx(100+(++reads)*2,out);
    if(got)break;
  }
  // HW-FIFO single-byte tail remains buffered until a short quiet interval.
  if(engine.stats().rx_frames==0) assert(engine.poll_rx(100+(reads)*2+5,out));
  assert(out.command==0x22F1 && out.payload[1]==4);
  assert(engine.stats().rx_frames==1);
  assert(engine.stats().rx_bytes==spi.size());
  assert(engine.stats().rx_polls>0);
  assert(hw.regs[0x10]==0x5A && hw.regs[0x11]==0x83 && hw.regs[0x15]==0x40);
  hw.rx.push_back(0x88); assert(!engine.poll_rx(150,out));
  assert(!engine.poll_rx(240,out) && engine.stats().rx_timeouts==1);
  for(int i=0;i<65;++i)hw.rx.push_back(0xFF);
  assert(!engine.poll_rx(250,out) && engine.stats().rx_overflows==1);
  assert(hw.rx.empty());
  // Long binding telegrams use TX FIFO refills; CRC and encoded byte length verified on host.
  Frame offer;assert(tx::buildBindOffer(remote,offer));
  std::vector<uint8_t> offer_spi;
  assert(FifoTransport::make_fifo_payload(offer,offer_spi) && offer_spi.size()>64);
  assert(engine.start_tx(offer,300));
  assert(engine.state()==FifoTransport::State::TX);
  unsigned polls=0;
  for(;polls<100 && engine.state()==FifoTransport::State::TX;++polls) {
    hw.tick_tx(16);
    engine.poll_tx(301+polls);
  }
  assert(engine.stats().tx_frames==1);
  assert(engine.state()==FifoTransport::State::RX);
  assert(hw.sent.size()==offer_spi.size()+1 && hw.sent[0]==offer_spi.size());
  assert(std::equal(offer_spi.begin(),offer_spi.end(),hw.sent.begin()+1));
  assert(engine.stop() && engine.state()==FifoTransport::State::OFF);
  // Alternate unframed SPI lab mode must accept an arbitrary byte/bit phase,
  // using software UART sync search and checksum instead of B32A hardware sync.
  MockBus lab_hw;
  FifoTransport unframed(lab_hw, true);
  assert(unframed.start_rx(1000));
  assert(lab_hw.regs[0x12]==0x00 && unframed.unsynced_capture());
  assert(!unframed.start_tx(f,1000));  // Never transmit in unframed lab mode.
  std::vector<uint8_t> air;
  size_t airbits=0;
  assert(tx::encodeOnAir(f,5,air,airbits));
  for(size_t off=0;off<air.size();off+=12) {
    const size_t len=std::min<size_t>(12,air.size()-off);
    for(size_t j=0;j<len;++j)lab_hw.rx.push_back(air[off+j]);
    unframed.poll_rx(uint32_t(1000+(off/12+1)*2),out);
  }
  if(unframed.stats().rx_frames==0) unframed.poll_rx(1050,out);
  assert(unframed.stats().rx_frames==1);
  assert(out.command==0x22F1 && out.payload[1]==4);
  assert(unframed.stop());
  // Wrong CC1101 modem readback must abort before radio can accept packets.
  MockBus mismatched;
  mismatched.bad_readback=true;
  FifoTransport rejected(mismatched);
  assert(!rejected.start_rx(800) && rejected.state()==FifoTransport::State::ERROR);
  assert(rejected.stats().io_errors==1);
  // TX FIFO underflow means never claim the command was transmitted.
  MockBus underrun_bus;
  FifoTransport underrun(underrun_bus);
  assert(underrun.start_rx(900));
  assert(underrun.start_tx(offer,901));
  underrun_bus.underflow=true;
  assert(!underrun.poll_tx(902));
  assert(underrun.stats().tx_underflows==1 && underrun.stats().tx_frames==0);
  assert(underrun.state()==FifoTransport::State::ERROR);
  assert(underrun.stop());
  // Fail-closed bus errors.
  hw.io_ok=false;
  assert(!engine.start_rx(900) && engine.state()==FifoTransport::State::ERROR);
  assert(engine.stats().io_errors>0);
  std::puts("RAMSES5 SPI lab: synced + unsynced RX, FIFO guard, explicit 38.4k modem, long TX host simulation: PASS");
}
