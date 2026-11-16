#pragma once
// Native receive-only ports of RFLink v0.6.6 Acurite609TXC and FineOffsetWH2.
// The signed-pulse timing and payload checks mirror the archived ESPHome
// decoder packs. No ESPHome dependency, radio configuration, TX or NVS access.
// Recognition is gated by full frame timing, checksum/CRC and sensor bounds.
// IDs and values must NEVER be written to unauthenticated logs.
#include <array>
#include <cstddef>
#include <cstdint>

namespace rflink { namespace weather433 {

enum class Protocol : uint8_t { none=0, acurite_609txc=1, fineoffset_wh2=2, ambiguous=3 };

struct Measurement {
 Protocol protocol=Protocol::none;
 uint16_t sensor_id=0;
 int16_t temperature_tenths_c=0;
 uint8_t humidity_percent=0;
 bool battery_low=false;
 bool integrity_verified=false;
};

// Inclusive percentage tolerance, exactly as in the v0.6.6 pulse_tools helper.
inline bool near_width(int32_t signed_us,uint32_t target,uint8_t tolerance_pct){
 const int64_t n=static_cast<int64_t>(signed_us);
 const uint64_t magnitude=static_cast<uint64_t>(n<0?-n:n);
 const uint64_t min=uint64_t(target)*(100u-tolerance_pct)/100u;
 const uint64_t max=uint64_t(target)*(100u+tolerance_pct)/100u;
 return magnitude>=min && magnitude<=max;
}

inline uint8_t crc8_wh2(const uint8_t* data,size_t count) {
 uint8_t state=0;
 for(size_t i=0;i<count;++i){
  state^=data[i];
  for(unsigned b=0;b<8;++b)
   state=(state&0x80u)?static_cast<uint8_t>((state<<1u)^0x31u):
                       static_cast<uint8_t>(state<<1u);
 }
 return state;
}

// Read a bounded mark/space PWM frame; no heap allocations or bit strings.
// The frame may start after a noisy prefix, as in the v0.6.6 decoder.
inline bool read_pwm(const int32_t* raw,size_t count,size_t offset,
                     size_t bits,uint32_t short_mark,uint32_t long_mark,
                     uint8_t mark_tolerance,bool long_is_one,
                     std::array<uint8_t,6>& bytes){
 if(!raw || offset>count || bits>(count-offset)/2 || bits>48)return false;
 bytes.fill(0);
 for(size_t bit=0;bit<bits;++bit){
  const int32_t mark=raw[offset+2*bit],gap=raw[offset+2*bit+1];
  if(mark<=0 || gap>=0 || !near_width(gap,1000u,40u))return false;
  bool is_long=false;
  if(near_width(mark,long_mark,mark_tolerance))is_long=true;
  else if(!near_width(mark,short_mark,mark_tolerance))return false;
  const bool one=(is_long==long_is_one);
  if(one)bytes[bit/8]|=static_cast<uint8_t>(0x80u>>(bit%8));
 }
 return true;
}

inline bool check_acurite(const std::array<uint8_t,6>& b,Measurement& out) {
 const unsigned sum=unsigned(b[0])+b[1]+b[2]+b[3];
 const uint16_t raw12=uint16_t((uint16_t(b[1]&0x0fu)<<8u)|b[2]);
 const int16_t temp=static_cast<int16_t>(
      raw12&0x800u ? int(raw12)-4096 : int(raw12));
 if(!b[0] || !sum || static_cast<uint8_t>(sum)!=b[4] ||
    b[3]>100 || temp< -400 || temp>700)return false;
 out.protocol=Protocol::acurite_609txc;
 out.sensor_id=b[0];
 out.temperature_tenths_c=temp;
 out.humidity_percent=b[3];
 out.battery_low=(b[1]&0x80u)!=0;
 out.integrity_verified=true;
 return true;
}
inline bool check_wh2(const std::array<uint8_t,6>& b,Measurement& out) {
 // Byte 0 is 0xFF preamble, next five are WH2 data+CRC.
 if(b[0]!=0xffu || (b[1]>>4u)!=4u || !b[1] ||
    b[4]>100 || crc8_wh2(&b[1],4)!=b[5])return false;
 const unsigned magnitude=(unsigned(b[2]&0x0fu)<<8u)|b[3];
 const int temp=(magnitude&0x800u)?-int(magnitude&0x7ffu):int(magnitude);
 if(temp< -400 || temp>700)return false;
 out.protocol=Protocol::fineoffset_wh2;
 out.sensor_id=uint16_t(((b[1]&0x0fu)<<4u)|(b[2]>>4u));
 out.temperature_tenths_c=static_cast<int16_t>(temp);
 out.humidity_percent=b[4];
 out.integrity_verified=true;
 return true;
}
inline Measurement decode(const int32_t* samples,size_t count,uint32_t frequency_hz){
 Measurement none{};
 // Only the 433.92 MHz RAW radio. Never interpret Somfy 433.42 or FIFO 868.
 if(!samples || frequency_hz!=433920000u || count<80 || count>1024)return none;
 Measurement match{};
 bool found=false;
 for(size_t start=0;start<count && start<300;++start){
  if(start+80>count)break;
  for(unsigned polarity=0;polarity<2;++polarity){
   const bool long_is_one=polarity==0;
   std::array<uint8_t,6> packet{};
   Measurement candidate{};
   // The Acurite 609TXC uses 40 marks around 500/1000 us.
   if(read_pwm(samples,count,start,40,500u,1000u,32u,
               long_is_one,packet) && check_acurite(packet,candidate)){
    if(found && match.protocol!=candidate.protocol){
     none.protocol=Protocol::ambiguous;return none;
    }
    match=candidate;found=true;
   }
   // Fine Offset WH2 uses 48 marks around 500/1500 us and 0xFF preamble.
   if(start+96<=count &&
      read_pwm(samples,count,start,48,500u,1500u,35u,
               long_is_one,packet) && check_wh2(packet,candidate)){
    if(found && match.protocol!=candidate.protocol){
     none.protocol=Protocol::ambiguous;return none;
    }
    match=candidate;found=true;
   }
  }
  // After a validated CRC/checksum match, do not keep scanning thousands of
  // edges; no passive background RF can authorize a device or transmission.
  if(found)return match;
 }
 return none;
}

}} // namespace rflink::weather433
