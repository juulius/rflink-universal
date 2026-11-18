// Private, host-only protocol analysis of bounded 868 synchronous bit runs.
// This executable does not access hardware, Wi-Fi, Home Assistant or RFLink.
// Input (stdin): segment_index bit_count HEX_MSB, one continuous run per line.
// Output (stdout): segment TAB protocol TAB bit_offset TAB span_bits TAB proof TAB detail.
// No raw RF values, addresses, payloads or secrets are printed.
// For safety, Itho matches are STRUCTURAL only: its legacy codec does not
// currently verify a complete protocol checksum.
#include "rflink_itho_codec.hpp"
#include "rflink_ramses_codec.hpp"
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace {
constexpr size_t kMaxBits = 256u * 2048u;
constexpr size_t kIthoBits = 63u * 8u;
constexpr size_t kRamsesLimitBits = 2048u;
constexpr unsigned kMaxSegments = 256;
constexpr char kIthoHeader[] = "\x00\xb3\x2a\xab\x2a";

unsigned bit(const std::vector<uint8_t>& data, size_t i, bool invert) {
  return unsigned((data[i / 8] >> (7 - (i % 8))) & 1u) ^ unsigned(invert);
}

bool u8(const std::vector<uint8_t>& data,size_t bitcount,size_t at,bool invert,uint8_t& value) {
  if (at > bitcount || bitcount-at < 10) return false;
  if (bit(data,at,invert) != 0 || bit(data,at+9,invert) != 1) return false;
  value=0;
  for (unsigned i=0;i<8;++i)
    value|=static_cast<uint8_t>(bit(data,at+1+i,invert)<<i);
  return true;
}

std::vector<uint8_t> extract(const std::vector<uint8_t>& data, size_t start,
                             size_t count, bool invert) {
  std::vector<uint8_t> out((count+7)/8,0);
  for (size_t k=0;k<count;++k)
    if (bit(data,start+k,invert)) out[k>>3]|=uint8_t(0x80u>>(k&7u));
  return out;
}

void match(unsigned segment,const char* protocol,size_t offset,size_t count,
           const char* proof,const std::string& summary,bool inverted) {
  std::cout<<segment<<'\t'<<protocol<<'\t'<<offset<<'\t'<<count<<'\t'
           <<proof<<'\t'<<summary<<'\t'<<(inverted?"inverted":"normal")<<'\n';
}

void scan_itho(unsigned segment,const std::vector<uint8_t>& bits,
               size_t bitcount,bool inverted) {
  if(bitcount<kIthoBits)return;
  for(size_t at=0;at<=bitcount-kIthoBits;++at) {
    // First, require the exact 40-bit packet header at an arbitrary bit offset.
    bool header=true;
    for(unsigned i=0;i<5 && header;++i) {
      const uint8_t expected=static_cast<uint8_t>(kIthoHeader[i]);
      for(unsigned j=0;j<8;++j)
        if(bit(bits,at+i*8+j,inverted)!=((expected>>(7-j))&1u)) {
          header=false;break;
        }
    }
    if(!header)continue;
    auto packet=extract(bits,at,kIthoBits,inverted);
    rflink::itho::Decoded out{};
    if(!rflink::itho::decode_packet(packet,out))continue;
    std::ostringstream ss;
    switch(out.type) {
      case rflink::itho::MessageType::SPEED:
        ss<<"speed="<<unsigned(out.speed);break;
      case rflink::itho::MessageType::TIMER:
        ss<<"timer_minutes="<<unsigned(out.timer_minutes);break;
      case rflink::itho::MessageType::UNIT_SPEED:
        ss<<"fan_percent="<<unsigned(out.fan_percent);break;
      case rflink::itho::MessageType::HUMIDITY:
        ss<<"humidity_percent="<<unsigned(out.humidity_percent);break;
      default: ss<<"type=unknown";break;
    }
    match(segment,"itho",at,kIthoBits,"structure_only",ss.str(),inverted);
    at+=kIthoBits-1; // Do not report overlapping interpretations.
  }
}

void scan_ramses(unsigned segment,const std::vector<uint8_t>& bits,
                 size_t bitcount,bool inverted) {
  if(bitcount<160)return;
  for(size_t at=0;at+40<=bitcount;++at) {
    uint8_t a=0,b=0,c=0;
    if(!u8(bits,bitcount,at,inverted,a) || a!=0x33 ||
       !u8(bits,bitcount,at+10,inverted,b) || b!=0x55 ||
       !u8(bits,bitcount,at+20,inverted,c) || c!=0x53)continue;
    // The terminator must be UART-framed, and all intervening bytes 8N1.
    // The existing native decoder is the only checksum authority.
    size_t end=0;
    for(size_t pos=at+30;pos+10<=bitcount && pos+10-at<=kRamsesLimitBits;pos+=10) {
      uint8_t v=0;
      if(!u8(bits,bitcount,pos,inverted,v))break;
      if(v==0x35){end=pos+10;break;}
    }
    if(!end)continue;
    const size_t count=end-at;
    auto packed=extract(bits,at,count,inverted);
    rflink_ramses_ii::Frame frame;
    if(!rflink_ramses_ii::decodePacked(packed.data(),packed.size(),frame))continue;
    std::ostringstream ss;
    ss<<"command=0x"<<std::uppercase<<std::hex<<std::setw(4)
      <<std::setfill('0')<<frame.command;
    match(segment,"ramses_ii",at,count,"checksum_valid",ss.str(),inverted);
    at=end-1; // Exclude nested duplicate scans of this validated transmission.
  }
}

int nibble(char c) {
  if(c>='0'&&c<='9')return c-'0';
  if(c>='a'&&c<='f')return c-'a'+10;
  if(c>='A'&&c<='F')return c-'A'+10;
  return -1;
}

int run(bool try_inverted) {
  std::string line;
  unsigned entries=0;
  size_t total=0;
  while(std::getline(std::cin,line)){
    if(line.empty())continue;
    if(++entries>kMaxSegments) {
      std::cerr<<"Too many segments\n";return 2;
    }
    std::istringstream stream(line);
    unsigned segment=0;
    size_t count=0;
    std::string hex,extra;
    if(!(stream>>segment>>count>>hex) || (stream>>extra) ||
       segment>=kMaxSegments || count==0 || count>kMaxBits ||
       hex.size()!=((count+7)/8)*2 || total>kMaxBits-count){
      std::cerr<<"Invalid segment metadata\n";return 2;
    }
    std::vector<uint8_t> bits(hex.size()/2);
    for(size_t i=0;i<bits.size();++i) {
      int hi=nibble(hex[2*i]),lo=nibble(hex[2*i+1]);
      if(hi<0||lo<0){std::cerr<<"Invalid hex\n";return 2;}
      bits[i]=static_cast<uint8_t>((hi<<4)|lo);
    }
    // The Python CSV inspector validates zero-padded bits before invoking this
    // helper, but also reject them here if called directly.
    const unsigned unused=unsigned((8-count%8)%8);
    if(unused && (bits.back()&((1u<<unused)-1u))){
      std::cerr<<"Nonzero unused bits\n";return 2;
    }
    total+=count;
    scan_itho(segment,bits,count,false);
    scan_ramses(segment,bits,count,false);
    if(try_inverted) {
      scan_itho(segment,bits,count,true);
      scan_ramses(segment,bits,count,true);
    }
  }
  return 0;
}
} // namespace

int main(int argc,char** argv) {
  if(argc>2 || (argc==2 && std::string(argv[1])!="--try-inverted")) {
    std::cerr<<"Usage: sync_replay_native [--try-inverted]\n";return 2;
  }
  return run(argc==2);
}
