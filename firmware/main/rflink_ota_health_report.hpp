#pragma once
// Fixed-size, versioned OTA health evidence that survives ESP-IDF rollback.
// Does NOT contain credentials, RF packets, serial text or arbitrary pointers.
// Kept in the existing RF diagnostics NVS namespace, NOT the OTA app partition.
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace rflink {
constexpr uint32_t kOtaHealthMagic=0x52464f54;  // "RFOT"
constexpr uint32_t kOtaHealthVersion=1;
enum class OtaHealthOutcome : uint8_t {
  checking=1, failed=2, valid=3, validation_error=4
};
struct OtaHealthReport {
  uint32_t magic=kOtaHealthMagic;
  uint32_t schema=kOtaHealthVersion;
  uint32_t boot_sequence=0;
  uint8_t outcome=0;
  uint8_t spi_ok=0;
  uint8_t web_ok=0;
  uint8_t worker_ok=0;
  uint32_t cycles=0;
  uint32_t active_receivers=0;
  uint32_t responsive_radios=0;
  char firmware[32]={};
  uint32_t checksum=0;
};
inline uint32_t ota_health_checksum(const OtaHealthReport& report) {
  // FNV-1a for detecting torn/corrupt NVS values; not a security signature.
  auto* bytes=reinterpret_cast<const uint8_t*>(&report);
  uint32_t hash=2166136261u;
  for(size_t i=0;i<offsetof(OtaHealthReport,checksum);++i)
    hash=(hash^bytes[i])*16777619u;
  return hash;
}
inline OtaHealthReport ota_health_make(uint32_t boot, const char* version,
                                       OtaHealthOutcome outcome, bool spi,
                                       bool web, bool worker, uint32_t cycles,
                                       unsigned active, unsigned responsive) {
  OtaHealthReport report{};
  report.boot_sequence=boot;
  report.outcome=static_cast<uint8_t>(outcome);
  report.spi_ok=spi?1:0;
  report.web_ok=web?1:0;
  report.worker_ok=worker?1:0;
  report.cycles=cycles;
  report.active_receivers=active;
  report.responsive_radios=responsive;
  if(version) {
    for(size_t i=0;i<sizeof(report.firmware)-1 && version[i];++i) {
      const unsigned char ch=static_cast<unsigned char>(version[i]);
      report.firmware[i]=
        ((ch>='0'&&ch<='9')||(ch>='a'&&ch<='z')||
         (ch>='A'&&ch<='Z')||ch=='-'||ch=='_'||ch=='.'||ch=='+')
          ?static_cast<char>(ch):'_';
    }
  }
  report.checksum=ota_health_checksum(report);
  return report;
}
inline bool ota_health_valid(const OtaHealthReport& report) {
  if(report.magic!=kOtaHealthMagic || report.schema!=kOtaHealthVersion ||
     report.boot_sequence==0 || report.outcome<1 || report.outcome>4 ||
     report.spi_ok>1 || report.web_ok>1 || report.worker_ok>1 ||
     report.firmware[31]!='\0')return false;
  bool terminated=false;
  for(char ch: report.firmware) {
    if(!ch) {terminated=true;break;}
    if(!((ch>='0'&&ch<='9')||(ch>='a'&&ch<='z')||
         (ch>='A'&&ch<='Z')||ch=='-'||ch=='_'||ch=='.'||ch=='+'))return false;
  }
  return terminated && report.checksum==ota_health_checksum(report);
}
inline const char* ota_health_name(OtaHealthOutcome outcome) {
  switch(outcome) {
    case OtaHealthOutcome::checking:return "checking";
    case OtaHealthOutcome::failed:return "failed";
    case OtaHealthOutcome::valid:return "valid";
    case OtaHealthOutcome::validation_error:return "validation_error";
    default:return "unknown";
  }
}
} // namespace rflink
