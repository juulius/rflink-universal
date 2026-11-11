#include "ramses_uart_stream.h"
namespace esphome { namespace rflink_universal {
void RamsesUartStream::reset_buffer_() {count_=0;last_byte_ms_=0;}
void RamsesUartStream::reset() {reset_buffer_();pending_count_=0;}
bool RamsesUartStream::take_pending(rflink_ramses_ii::Frame &out) {
  if(!pending_count_)return false;
  out=pending_[0];
  if(pending_count_==2)pending_[0]=pending_[1];
  --pending_count_;
  return true;
}
bool RamsesUartStream::feed(const uint8_t *data, size_t len, uint32_t now,
                           rflink_ramses_ii::Frame &frame) {
  if (!data || len==0 || len>1024) return false;
  if (count_ && static_cast<uint32_t>(now-last_byte_ms_)>120U) reset_buffer_();
  last_byte_ms_=now;
  bool matched=false;
  for(size_t i=0;i<len;++i) {
    if(count_==CAPACITY) {
      ++overflow_;
      // Keep the most recent sync prefix across buffer rollover.
      const uint8_t x=bytes_[count_-2], y=bytes_[count_-1];
      count_=0;
      if(x==0x33 && y==0x55) {bytes_[count_++]=x;bytes_[count_++]=y;}
    }
    bytes_[count_++]=data[i];
    // The FF 00 preamble is optional; synchronization begins at 33 55 53.
    size_t sync=count_;
    for(size_t j=0;j+2<count_;++j)
      if(bytes_[j]==0x33 && bytes_[j+1]==0x55 && bytes_[j+2]==0x53) {sync=j;break;}
    if(sync==count_) {
      // No frame started; preserve at most the partial sync marker.
      if(count_>2) {
        const uint8_t x=bytes_[count_-2],y=bytes_[count_-1];
        count_=0;
        if(x==0x33 && y==0x55) {bytes_[count_++]=x;bytes_[count_++]=y;}
        else if(y==0x33) bytes_[count_++]=y;
      }
      continue;
    }
    if(sync>0) {
      for(size_t j=sync;j<count_;++j)bytes_[j-sync]=bytes_[j];
      count_-=sync;
    }
    // Decode only after the 0x35 terminator, which cannot occur in a valid
    // Manchester nibble pair. Invalid checksums never escape this layer.
    if(count_>3 && data[i]==0x35) {
      rflink_ramses_ii::Frame decoded;
      if(rflink_ramses_ii::decodeWire(bytes_.data(),count_,decoded)) {
        if(!matched){frame=decoded;matched=true;}
        else if(pending_count_<pending_.size())pending_[pending_count_++]=decoded;
        else ++overflow_;
        reset_buffer_();
        continue;
      }
      ++rejected_;
      reset_buffer_();
    }
  }
  return matched;
}
}}
