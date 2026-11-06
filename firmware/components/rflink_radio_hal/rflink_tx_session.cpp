#include "rflink_tx_session.hpp"
#include "esp_log.h"
namespace rflink {
Cc1101TxSession::Cc1101TxSession(Cc1101SpiBus& bus,RadioId radio):bus_(bus),radio_(radio){}
bool Cc1101TxSession::snapshot(uint8_t addr) {
  if(addr>=old_.size())return false;
  if(saved_[addr])return true;
  uint8_t v=0;
  if(bus_.read_register(radio_,addr,&v)!=ESP_OK)return false;
  old_[addr]=v;
  saved_[addr]=true;
  return true;
}
bool Cc1101TxSession::set_checked(uint8_t addr,uint8_t value) {
  if(!snapshot(addr))return false;
  if(bus_.write_register(radio_,addr,value)!=ESP_OK)return false;
  uint8_t got=0xff;
  if(bus_.read_register(radio_,addr,&got)!=ESP_OK)return false;
  const uint8_t mask=addr==0x15?0xf7:0xff; // DEVIATN reserved bit
  return (got&mask)==(value&mask);
}
bool Cc1101TxSession::prepare(const RfProfile& p,bool asynchronous,uint8_t length){
  if(!profile_has_registers(p) || !p.pa_table || p.pa_table_len==0 ||
      p.pa_table_len>8 || !profile_compatible_with_radio(p,radio_))return false;
  touched_=true; // subsequent restore required even after a partial snapshot
  if(bus_.strobe(radio_,0x36)!=ESP_OK || bus_.strobe(radio_,0x3b)!=ESP_OK)return false;
  if(bus_.read_pa_table(radio_,old_pa_.data(),old_pa_.size())!=ESP_OK)return false;
  pa_saved_=true;
  const uint32_t freq=cc1101_frequency_word(p.frequency_hz);
  const RegisterSetting frequency[]={{0x0d,uint8_t(freq>>16)},{0x0e,uint8_t(freq>>8)},{0x0f,uint8_t(freq)}};
  for(const auto& reg:frequency)if(!set_checked(reg.address,reg.value))return false;
  for(size_t i=0;i<p.register_count;i++)
    if(!set_checked(p.modem_registers[i].address,p.modem_registers[i].value))return false;
  // Keep all eight entries deterministic, including the unused slots.
  std::array<uint8_t,8> pa{};
  for(size_t i=0;i<p.pa_table_len;i++)pa[i]=p.pa_table[i];
  if(bus_.write_pa_table(radio_,pa.data(),pa.size())!=ESP_OK)return false;
  std::array<uint8_t,8> verify{};
  if(bus_.read_pa_table(radio_,verify.data(),verify.size())!=ESP_OK || pa!=verify)return false;
  if(asynchronous) {
    // Async TX data always enters CC1101 on GDO0. RMT drives that pin.
    if(!set_checked(0x00,0x2e) || !set_checked(0x08,0x30) ||
       !set_checked(0x22,0x11))return false; // OOK: PATABLE[0] off, PATABLE[1] on
  } else {
    // Fixed-length FIFO mode, no append status or variable length byte.
    if(!set_checked(0x08,0x00) || !set_checked(0x06,length) ||
       !set_checked(0x22,0x10))return false; // FSK: PA_POWER=0, PATABLE[0]
    if(!set_checked(0x00,0x06))return false; // packet sync/end indicator on GDO0
  }
  return true;
}
bool Cc1101TxSession::restore(){
  if(!touched_)return true;
  bool ok=bus_.strobe(radio_,0x36)==ESP_OK;
  for(size_t i=0;i<saved_.size();i++)if(saved_[i]) {
    if(bus_.write_register(radio_,static_cast<uint8_t>(i),old_[i])!=ESP_OK)ok=false;
    uint8_t readback=0xff;
    if(bus_.read_register(radio_,static_cast<uint8_t>(i),&readback)!=ESP_OK ||
       (readback&(i==0x15?0xf7:0xff))!=(old_[i]&(i==0x15?0xf7:0xff)))ok=false;
  }
  if(pa_saved_) {
    if(bus_.write_pa_table(radio_,old_pa_.data(),old_pa_.size())!=ESP_OK)ok=false;
    std::array<uint8_t,8> readback{};
    if(bus_.read_pa_table(radio_,readback.data(),readback.size())!=ESP_OK ||
       readback!=old_pa_)ok=false;
  }
  if(!ok)ESP_LOGE("rflink.txsession","radio%u register/PA restore failed",unsigned(radio_));
  touched_=false;
  return ok;
}
} // namespace rflink
