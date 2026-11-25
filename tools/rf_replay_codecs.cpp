// Offline RFLink 868 MHz replay adapter. Deliberately links the SAME
// RAMSES-II and Itho source files as native firmware, without radio, SPI or TX.
// Each stdin line must be exactly one 63-byte FIFO packet in 126 hex digits.
// Outputs one small JSON verdict per line; no device addresses or raw secrets.
#include "rflink_ramses_codec.hpp"
#include "rflink_itho_codec.hpp"
#include "rflink_sync868_replay.hpp"
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
// Each automatic window is an independent fragment. Never concatenate
// separate rows or treat carrier context as protocol validation.
bool read_window(void* context,uint32_t index,rflink::ClockedBitBlock* out){
  if(index!=0 || !out)return false;
  *out=*static_cast<const rflink::ClockedBitBlock*>(context);return true;
}
void emit_sync(const std::array<uint8_t,kPacketBytes>& packet){
  rflink::ClockedBitBlock block{};
  block.epoch=1;block.bit_count=504;block.window_end=true;
  // Scanner uses timestamps only for validity. These are synthetic
  // positional placeholders, not measurement timestamps or RF timing.
  block.first_sample_us=0;block.last_sample_us=503;
  for(size_t i=0;i<packet.size();++i)block.bytes[i]=packet[i];
  rflink::Sync868ReplayReport report{};
  if(!rflink::replay_sync868_frozen(1,read_window,&block,true,&report)){
    std::cout << R"({"protocol":null,"reason":"sync_window_scan_failed","hardware_crc_verified":false})" << '\n';
    return;
  }
  // Embedded matches may be the same on-air evidence as an Itho structure.
  // Only a checksum-valid non-embedded RAMSES hit is a protocol verdict.
  const bool valid=report.ramses_outside_itho>0 && !report.hits_truncated;
  const char* reason=valid?"sync_ramses_checksum_valid":
    report.itho_structure_only?"sync_itho_structure_only":
    report.ramses_checksum_valid?"sync_ramses_overlap_or_partial":"sync_no_match_in_window";
  std::cout << "{\"protocol\":" << (valid?"\"ramses_ii\"":"null")
    << ",\"reason\":\"" << reason << "\""
    << ",\"hardware_crc_verified\":false,\"sync_scan\":{"
    << "\"itho_structure_only\":" << report.itho_structure_only
    << ",\"ramses_checksum_valid\":" << report.ramses_checksum_valid
    << ",\"ramses_embedded_in_itho\":" << report.ramses_embedded_in_itho
    << ",\"ramses_outside_itho\":" << report.ramses_outside_itho
    << ",\"hits_truncated\":" << (report.hits_truncated?"true":"false") << "}}\n";
}
}  // namespace

int main() {
  std::ios::sync_with_stdio(false);
  std::string line;
  std::array<uint8_t,kPacketBytes> packet{};
  while(std::getline(std::cin,line)){
    if(!line.empty() && line.back()=='\r')line.pop_back();
    const bool sync=line.rfind("SYNC:",0)==0;
    if(!parse(sync?line.substr(5):line,packet)){
      std::cout << R"({"protocol":null,"reason":"invalid_fifo_hex_or_length","hardware_crc_verified":false})" << '\n';
      continue;
    }
    if(sync)emit_sync(packet);else emit(packet);
  }
  return std::cin.bad()?1:0;
}
