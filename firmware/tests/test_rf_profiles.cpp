#include <cassert>
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
  assert(rf_profile(RfProfileId::ook_43392_pulses).rx_transport==RxTransport::gdo_rmt);
  return 0;
}
