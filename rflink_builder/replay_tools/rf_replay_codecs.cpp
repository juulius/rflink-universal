// Offline RFLink 868 MHz replay adapter. Deliberately links the SAME
// RAMSES-II and Itho source files as native firmware, without radio, SPI or TX.
// Each stdin line must be exactly one 63-byte FIFO packet in 126 hex digits.
// Outputs one small JSON verdict per line; no device addresses or raw secrets.
#include "rflink_ramses_codec.hpp"
#include "rflink_itho_codec.hpp"
#include <array>
#include <cctype>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {
constexpr size_t kPacketBytes=63;
constexpr uint8_t kPrefix[]={0x00,0xB3,0x2A,0xAB,0x2A};

int nibble(char c) {
  if(c>='0' && c<='9')return c-'0';
  if(c>='A' && c<='F')return c-'A'+10;
  if(c>='a' && c<='f')return c-'a'+10;
  return -1;
}

bool parse(const std::string& line,std::array<uint8_t,kPacketBytes>& bytes) {
  if(line.size()!=kPacketBytes*2)return false;
  for(size_t i=0;i<kPacketBytes;++i){
    const int hi=nibble(line[i*2]),lo=nibble(line[i*2+1]);
    if(hi<0 || lo<0)return false;
    bytes[i]=static_cast<uint8_t>(hi*16+lo);
  }
  return true;
}
void emit(const std::array<uint8_t,kPacketBytes>& packet) {
  for(size_t i=0;i<sizeof(kPrefix);++i){
    if(packet[i]!=kPrefix[i]){
      std::cout << R"({"protocol":null,"reason":"legacy_preamble_absent","hardware_crc_verified":false})" << '\n';
      return;
    }
  }
  rflink_ramses_ii::Frame ramses;
  const bool has_ramses=rflink_ramses_ii::decodePacked(packet.data(),packet.size(),ramses);
  rflink::itho::Decoded itho;
  const bool has_itho=rflink::itho::decode_packet(
    std::vector<uint8_t>(packet.begin(),packet.end()),itho) && itho.valid;

  // Preserve the live decoder's precedence: RAMSES-II wins if both pass.
  if(has_ramses){
    uint8_t mode=0;
    const bool has_mode=rflink_ramses_ii::extractMode(ramses,mode);
    std::cout << R"({"protocol":"ramses_ii","reason":"built_in_decoder_valid","hardware_crc_verified":false,"command":)"
              << ramses.command
              << R"(,"mode":)";
    if(has_mode)std::cout << unsigned(mode);
    else std::cout << "null";
    std::cout << R"(,"payload_length":)" << unsigned(ramses.payload_length)
              << R"(,"decoder_overlap":)" << (has_itho?"true":"false") << "}\n";
    return;
  }
  if(has_itho){
    std::cout << R"({"protocol":"itho_rft","reason":"built_in_decoder_valid","hardware_crc_verified":false,"itho_type":)"
              << unsigned(itho.type)
              << R"(,"speed":)" << unsigned(itho.speed)
              << R"(,"decoder_overlap":false})" << '\n';
    return;
  }
  // Missing user-installed recipes cannot be inferred from built-in codecs.
  std::cout << R"({"protocol":null,"reason":"built_in_decoder_rejected","hardware_crc_verified":false})" << '\n';
}
}  // namespace

int main() {
  std::ios::sync_with_stdio(false);
  std::string line;
  std::array<uint8_t,kPacketBytes> packet{};
  while(std::getline(std::cin,line)){
    if(!line.empty() && line.back()=='\r')line.pop_back();
    if(!parse(line,packet)){
      std::cout << R"({"protocol":null,"reason":"invalid_fifo_hex_or_length","hardware_crc_verified":false})" << '\n';
      continue;
    }
    emit(packet);
  }
  return std::cin.bad()?1:0;
}
