#include "ramses_spi_fifo.h"
#include <algorithm>

namespace esphome { namespace rflink_universal { namespace ramses_spi {
namespace {
// CC1101 register map / command strobes per SWRS061I.
constexpr uint8_t IOCFG2=0x00, IOCFG0=0x02, FIFOTHR=0x03;
constexpr uint8_t SYNC1=0x04, SYNC0=0x05, PKTLEN=0x06;
constexpr uint8_t PKTCTRL1=0x07, PKTCTRL0=0x08, MDMCFG4=0x10, MDMCFG3=0x11;
constexpr uint8_t DEVIATN=0x15;
constexpr uint8_t MDMCFG2=0x12, MCSM1=0x17;
constexpr uint8_t MARCSTATE=0x35, TXBYTES=0x3A, RXBYTES=0x3B;
constexpr uint8_t SIDLE=0x36, SRX=0x34, STX=0x35, SFRX=0x3A, SFTX=0x3B;
constexpr uint8_t MARC_IDLE=0x01, MARC_RX=0x0D;
constexpr uint32_t RX_TIMEOUT_MS=85, TX_TIMEOUT_MS=200;
constexpr size_t MAX_RX_BYTES=224, MAX_TX_BYTES=230;

bool uart_header_bits(std::vector<uint8_t> &out, size_t &bit_len) {
  static const uint8_t sync[]={0xFF,0x00,0x33,0x55,0x53};
  out.assign((50+7)/8, 0);
  bit_len=0;
  for(uint8_t b:sync) for(unsigned j=0;j<10;++j) {
    const unsigned bit=j==0?0:j==9?1:((b>>(j-1))&1);
    if(bit) out[bit_len/8]|=uint8_t(0x80U>>(bit_len%8));
    ++bit_len;
  }
  return bit_len==50;
}
inline uint8_t at_bit(const uint8_t *p, size_t bit) {return (p[bit/8]>>(7U-(bit%8)))&1U;}
inline void set_bit(uint8_t *p, size_t bit, uint8_t v) {if(v) p[bit/8]|=uint8_t(0x80U>>(bit%8));}
} // namespace

bool FifoTransport::parse_fifo_capture(const uint8_t* payload, size_t len, rflink_ramses_ii::Frame& frame) {
  if(!payload || len==0 || len>MAX_RX_BYTES) return false;
  std::vector<uint8_t> header;
  size_t head_bits=0;
  uart_header_bits(header,head_bits);
  const size_t size_bits = SYNC_END_BIT+len*8;
  if(size_bits>256*8) return false;
  std::vector<uint8_t> raw((size_bits+7)/8,0);
  for(size_t i=0;i<SYNC_END_BIT;++i) set_bit(raw.data(),i,at_bit(header.data(),i));
  for(size_t i=0;i<len*8;++i) set_bit(raw.data(),i+SYNC_END_BIT,at_bit(payload,i));
  return rflink_ramses_ii::decodePacked(raw.data(),raw.size(),frame);
}

bool FifoTransport::make_fifo_payload(const rflink_ramses_ii::Frame &frame, std::vector<uint8_t> &result) {
  std::vector<uint8_t> packed;
  size_t bit_count=0;
  if(!rflink_ramses_ii::tx::encodeOnAir(frame,0,packed,bit_count) || bit_count<=SYNC_END_BIT) return false;
  // The FIFO starts immediately after hardware has emitted 0xB32A.
  size_t remaining=bit_count-SYNC_END_BIT;
  if((remaining+7)/8>MAX_TX_BYTES) return false;
  std::vector<uint8_t> data((remaining+7)/8,0x00);
  for(size_t i=0;i<remaining;++i) set_bit(data.data(),i,at_bit(packed.data(),SYNC_END_BIT+i));
  // Pad with UART stop/high (1) bits; no mid-air length byte is emitted.
  for(size_t i=remaining;i<data.size()*8;++i) set_bit(data.data(),i,1);
  result.swap(data);
  return true;
}

bool FifoTransport::fail() {state_=State::ERROR;++stats_.io_errors;return false;}
bool FifoTransport::config(bool tx) {
  // These registers are overwritten ONLY while the ESPHome CC1101 handler is
  // paused by a RadioModeManager lease. No driver's internal state is changed.
  uint8_t original_mc=0;
  if(!bus_.read(MCSM1,original_mc)) return fail();
  if(!bus_.strobe(SIDLE)) return fail();
  if(!bus_.strobe(SFRX) || !bus_.strobe(SFTX)) return fail();
  // I/O signals disconnected from FIFO interrupts: SPI polling only.
  if(!bus_.write(IOCFG0,0x2E)||!bus_.write(IOCFG2,0x2E)||
     !bus_.write(SYNC1,0xB3)||!bus_.write(SYNC0,0x2A)||
     !bus_.write(PKTLEN,0xFF)||!bus_.write(FIFOTHR,0x07)||
     !bus_.write(PKTCTRL1,0x00)||
     // RX infinite length, TX variable length (first byte is length).
     !bus_.write(PKTCTRL0,tx?0x01:0x02)||
     // 26 MHz crystal: 38.383 kbaud, 325 kHz RX BW, 50.78 kHz deviation.
     // Do not inherit an accidental data rate or deviation from the Itho profile.
     // 2-FSK; Manchester remains software-framed, not enabled in the CC1101.
     // In no-sync RX lab mode, receive without a hardware B32A match.
     // TX is never allowed for this experimental unframed mode.
     !bus_.write(MDMCFG2,uint8_t((!tx && unsynced_capture_)?0x00:0x02))||
     !bus_.write(MDMCFG4,0x5A)||
     !bus_.write(MDMCFG3,0x83)||
     !bus_.write(DEVIATN,0x40)||
     // Return to RX after packet detection, TX to idle after transmit.
     !bus_.write(MCSM1,uint8_t((original_mc&0xF0)|0x0C))) return fail();
  uint8_t verify=0;
  if(!bus_.read(SYNC1,verify)||verify!=0xB3||
     !bus_.read(SYNC0,verify)||verify!=0x2A||
     !bus_.read(PKTCTRL0,verify)||verify!=(tx?0x01:0x02)||
     !bus_.read(MDMCFG4,verify)||verify!=0x5A||
     !bus_.read(MDMCFG3,verify)||verify!=0x83||
     !bus_.read(DEVIATN,verify)||verify!=0x40) return fail();
  return true;
}

bool FifoTransport::start_rx(uint32_t now) {
  if(state_!=State::OFF) return false;
  if(!config(false)) return false;
  if(!bus_.strobe(SRX)) return fail();
  rx_len_=0;rx_started_=now;rx_last_activity_ms_=now;rx_last_poll_ms_=now;singleton_seen_=false;state_=State::RX;return true;
}

bool FifoTransport::flush_rx() {
  if(!bus_.strobe(SIDLE)||!bus_.strobe(SFRX)||!bus_.strobe(SRX)) return fail();
  rx_len_=0;return true;
}

// TI SWRS061I section 20: do not drain a streaming RX FIFO to zero
// while RF is still receiving; the last byte can be returned twice.
// Read the RXBYTES status twice to reject transient counter transitions.
bool FifoTransport::read_rx_count(uint8_t &count) {
  uint8_t a=0, b=0;
  if(!bus_.read(RXBYTES,a) || !bus_.read(RXBYTES,b)) return fail();
  if((a|b)&0x80U) { count=0x80U; return true; }
  // Using the smaller sample is conservative when the FIFO is filling.
  count=std::min<uint8_t>(uint8_t(a&0x7FU),uint8_t(b&0x7FU));
  return true;
}

bool FifoTransport::poll_rx(uint32_t now, rflink_ramses_ii::Frame &frame) {
  if(state_!=State::RX) return false;
  ++stats_.rx_polls;
  const uint32_t gap=uint32_t(now-rx_last_poll_ms_);
  stats_.rx_max_poll_gap_ms=std::max(stats_.rx_max_poll_gap_ms, uint16_t(std::min<uint32_t>(gap,65535)));
  if(gap>8U) ++stats_.rx_slow_polls;
  rx_last_poll_ms_=now;
  uint8_t count=0;
  if(!read_rx_count(count)) return false;
  if(count&0x80U) {++stats_.rx_overflows;flush_rx();rx_started_=now;rx_last_activity_ms_=now;return false;}
  stats_.rx_fifo_peak=std::max(stats_.rx_fifo_peak,count);
  if(count>64 || rx_len_+count>MAX_RX_BYTES) {
    ++stats_.rx_overflows;flush_rx();rx_started_=now;rx_last_activity_ms_=now;return false;
  }
  // During a continuous stream we cannot know the real packet end until
  // software checks the checksum. Keep exactly one byte in the HW FIFO.
  // This also works at a full 64-byte FIFO by draining the first 63.
  if(count==1 && rx_len_==0 && !singleton_seen_) {
    singleton_first_seen_ms_=now;
    singleton_seen_=true;
  } else if(count!=1) singleton_seen_=false;
  // A final trailing byte can only be read safely once the FIFO count is
  // stationary longer than a full over-the-air byte time (~0.21 ms). Our
  // 3 ms gap is conservative, and does not assert RAMSES packet validity.
  const bool quiet_tail = count==1 &&
    (rx_len_>0 ? uint32_t(now-rx_last_activity_ms_)>=3U
               : (singleton_seen_ && uint32_t(now-singleton_first_seen_ms_)>=3U));
  const size_t to_read=count>1?size_t(count-1):(quiet_tail?1U:0U);
  if(to_read) {
    singleton_seen_=false;
    if(!bus_.read_fifo(rx_.data()+rx_len_,to_read)) return fail();
    rx_len_+=to_read;
    rx_last_activity_ms_=now;
    stats_.rx_bytes+=uint32_t(to_read);
    if((unsynced_capture_ ? rflink_ramses_ii::decodePacked(rx_.data(),rx_len_,frame)
                          : parse_fifo_capture(rx_.data(),rx_len_,frame))) {
      ++stats_.rx_frames;
      if(!flush_rx())return false;
      rx_started_=now;rx_last_activity_ms_=now;
      return true;
    }
  }
  if(rx_len_ && uint32_t(now-rx_started_)>=RX_TIMEOUT_MS) {
    ++stats_.rx_timeouts; ++stats_.rx_invalid;
    if(!flush_rx()) return false;
    rx_started_=now;rx_last_activity_ms_=now;
  }
  if(!rx_len_) rx_started_=now;
  return false;
}

bool FifoTransport::fill_tx() {
  uint8_t available=0;
  if(!bus_.read(TXBYTES,available)) return fail();
  if(available&0x80) {++stats_.tx_underflows;return fail();}
  available &=0x7F;
  if(available>64) return fail();
  stats_.tx_fifo_peak=std::max(stats_.tx_fifo_peak,available);
  const size_t free_bytes=64-available;
  const size_t n=std::min<size_t>(free_bytes,std::min<size_t>(tx_.size()-tx_pos_,48));
  if(n && !bus_.write_fifo(tx_.data()+tx_pos_,n)) return fail();
  tx_pos_+=n;
  stats_.tx_bytes+=uint32_t(n);
  return true;
}

bool FifoTransport::start_tx(const rflink_ramses_ii::Frame &frame,uint32_t now) {
  if(state_!=State::RX || unsynced_capture_) return false; // No unframed TX.
  std::vector<uint8_t> payload;
  if(!make_fifo_payload(frame,payload) || payload.size()>MAX_TX_BYTES) return false;
  if(!bus_.strobe(SIDLE)||!bus_.strobe(SFRX)||!bus_.strobe(SFTX)) return fail();
  if(!config(true)) return false;
  tx_.swap(payload);tx_pos_=0;tx_started_=now;tx_done_=false;
  const uint8_t len=static_cast<uint8_t>(tx_.size());
  if(!bus_.write_fifo(&len,1)) return fail();
  if(!fill_tx() || !bus_.strobe(STX)) return fail();
  state_=State::TX;
  return true;
}

bool FifoTransport::poll_tx(uint32_t now) {
  if(state_!=State::TX) return false;
  if(uint32_t(now-tx_started_)>TX_TIMEOUT_MS) return fail();
  uint8_t marc=0;
  if(!bus_.read(MARCSTATE,marc)) return fail();
  marc &=0x1F;
  if(marc==0x01) { // IDLE after packet TX; only success after all bytes were enqueued.
    if(tx_pos_!=tx_.size()) {++stats_.tx_underflows;return fail();}
    ++stats_.tx_frames;tx_done_=true;
    if(!config(false))return false;
    if(!bus_.strobe(SRX))return fail();
    rx_len_=0;rx_started_=now;rx_last_activity_ms_=now;rx_last_poll_ms_=now;singleton_seen_=false;state_=State::RX;return true;
  }
  if(!fill_tx()) return false;
  return false;
}

bool FifoTransport::stop() {
  if(state_==State::OFF) return true;
  bool ok=bus_.strobe(SIDLE)&&bus_.strobe(SFRX)&&bus_.strobe(SFTX);
  tx_.clear();rx_len_=0;state_=ok?State::OFF:State::ERROR;
  return ok;
}

}}}
