#pragma once
// Hardware compatibility guard for native bench OTA. This is NOT a signature
// check: unauthenticated binaries can forge metadata. Keep OTA LAN-only.
#include <cstddef>
#include <cstdint>

namespace rflink {
constexpr char kNativeBoardTag[]="-n16r8-";
constexpr char kNativeProjectName[]="rflink_universal";

// app_desc fields are fixed-size arrays. Never assume they terminate with NUL.
inline bool fixed_equals(const char* input, size_t cap, const char* literal) {
  if (!input || !literal) return false;
  size_t i=0;
  while (i<cap && literal[i] && input[i]==literal[i]) ++i;
  return i<cap && literal[i]=='\0' && input[i]=='\0';
}
inline bool fixed_contains(const char* input,size_t cap,const char* token){
  if(!input || !token || !token[0])return false;
  size_t token_len=0;
  while(token[token_len])++token_len;
  for(size_t i=0;i+token_len<=cap && input[i];++i){
    size_t j=0;
    while(j<token_len && input[i+j]==token[j])++j;
    if(j==token_len)return true;
  }
  return false;
}
inline bool native_ota_identity_accepted(const char* project, size_t project_size,
                                         const char* version,size_t version_size){
  return fixed_equals(project,project_size,kNativeProjectName) &&
         fixed_contains(version,version_size,kNativeBoardTag);
}

// Native OTA must not silently switch GPIO assignments or SPI settings.
// This 8-hex build identity is not cryptographically authenticated; it is a
// compatibility guard only. For board-profile changes, use a controlled USB test.
inline bool same_board_profile_for_ota(const char* running,size_t run_cap,
                                       const char* candidate,size_t candidate_cap){
  auto find_tag=[](const char* value,size_t cap,char out[9])->bool{
    if(!value)return false;
    for(size_t i=0;i+10<=cap && value[i];++i){
      if(value[i]!='-' || value[i+1]!='b')continue;
      bool valid=true;
      for(size_t j=0;j<8;++j){
        const char c=value[i+2+j];
        if(!((c>='0'&&c<='9')||(c>='a'&&c<='f'))){valid=false;break;}
        out[j]=c;
      }
      if(valid && (i+10==cap || value[i+10]=='\0' || value[i+10]=='-')){
        out[8]='\0';return true;
      }
    }
    return false;
  };
  char a[9]{},b[9]{};
  return find_tag(running,run_cap,a) && find_tag(candidate,candidate_cap,b) &&
         fixed_equals(a,sizeof(a),b);
}

// Do not convert a sub-tick interval with pdMS_TO_TICKS directly:
// ESP-IDF's usual 100 Hz scheduler has 10 ms ticks, so 5 ms truncates
// to ZERO and a priority-5 radio worker can starve the CPU0 idle task.
// Round UP to at least one real scheduler tick, on any configured tick rate.
constexpr uint32_t kRxPollMilliseconds=5;
constexpr uint32_t kRxRecoveryMilliseconds=5000;
constexpr uint32_t rx_poll_delay_ticks(uint32_t ticks_per_second){
  return ticks_per_second == 0 ? 1u :
    static_cast<uint32_t>((uint64_t(kRxPollMilliseconds)*ticks_per_second+999u)/1000u);
}
// Keep RX recovery on wall clock rather than "1000 loop passes": 1000
// loops no longer means five seconds when a 5 ms delay becomes one tick.
constexpr bool rx_recovery_due(uint32_t now_ms,uint32_t previous_ms){
  return uint32_t(now_ms-previous_ms)>=kRxRecoveryMilliseconds;
}

struct BootHealth {
  bool spi_initialised=false;
  bool web_started=false;
  bool rx_task_started=false;
  uint32_t rx_cycles_during_window=0;
  unsigned active_receivers=0;
  unsigned responsive_radios=0;
};
inline bool passes_boot_health(const BootHealth& h){
  // This checks running infrastructure, not the correctness of RF traffic.
  return h.spi_initialised && h.web_started && h.rx_task_started &&
         h.rx_cycles_during_window>=100 && h.active_receivers>=1 &&
         h.responsive_radios>=1;
}
} // namespace rflink
