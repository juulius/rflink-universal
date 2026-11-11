#include <cassert>
#include <initializer_list>
#include "rflink_rf_profiles.hpp"
using namespace rflink;
int main(){
  const auto& p=rf_profile(RfProfileId::itho_868_packet);
  assert(profile_compatible_with_radio(p,RadioId::rf86830));
  assert(!profile_compatible_with_radio(p,RadioId::rf43392));
  assert(p.packet_length==63 && p.rx_transport==RxTransport::packet_fifo);
  bool len=false, c1=false, c0=false;
  for(size_t i=0;i<p.register_count;i++){
    const auto& r=p.modem_registers[i];
    if(r.address==0x06){assert(r.value==63);len=true;}
    if(r.address==0x07){assert(r.value==0);c1=true;}
    if(r.address==0x08){assert(r.value==0);c0=true;}
  }
  assert(len && c1 && c0);
  assert(cc1101_frequency_word(868299900)>0);
  // Both 433 MHz channels share ESPHome-equivalent receive modem values.
  // Leave the native TX owner/other radio profiles entirely unchanged.
  for(const auto id:{RfProfileId::ook_43342_pulses,RfProfileId::ook_43392_pulses}){
    const auto& ook=rf_profile(id);
    assert(ook.rx_transport==RxTransport::gdo_rmt);
    auto get=[&ook](uint8_t reg)->int{
      for(size_t j=0;j<ook.register_count;++j)
        if(ook.modem_registers[j].address==reg)return ook.modem_registers[j].value;
      return -1;
    };
    assert(get(0x00)==0x0d && get(0x02)==0x2e); // asynchronous GDO2, GDO0 RX-safe
    assert(get(0x0b)==0x06);                      // 152.3 kHz IF on 26 MHz XTAL
    assert(get(0x10)==0x87 && get(0x11)==0x93);  // ~4996 baud, 203 kHz filter
    assert(get(0x1b)==0xc7 && get(0x1c)==0x00 && get(0x1d)==0xb1);
    assert(get(0x08)==0x30 && get(0x12)==0x30);  // preserve native async policy
    assert(ook.tx_transport==TxTransport::gdo_rmt);
  }
  return 0;
}
