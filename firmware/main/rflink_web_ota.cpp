#include "rflink_packet_rx.hpp"
#include "rflink_cc1101_spi.hpp"
#include "rflink_rmt_rx.hpp"
#include "rflink_ook_rx.hpp"
#include "rflink_native_decoder.hpp"
#include "rflink_radio_manager.hpp"
#include "rflink_web_ota.hpp"
#include "rflink_native_safety.hpp"
#include "rflink_mqtt.hpp"
#include "rflink_crash_report.hpp"
#include "rflink_log_store.hpp"
#include "rflink_native_ui_assets.hpp"
#include "cJSON.h"
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <cctype>
#include <cstdlib>
#include <string>
#include <algorithm>
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_app_desc.h"
#include "esp_timer.h"
#include "nvs.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {
constexpr char TAG[]="rflink_web_ota";
constexpr size_t MAX_IMAGE=4*1024*1024; // per-partition upper bound further checked dynamically
constexpr int MAX_FORM=512;
static bool provisioning=false;
static std::atomic<bool> sta_connected{false};
static std::atomic<bool> ota_busy{false};
static bool spi_bus_ready=false;
static const rflink::Cc1101SpiBus* spi_provider=nullptr;
static bool detected_radio[3]={false,false,false};
static rflink::PacketRx868* rx_stats_provider=nullptr;
static rflink::OokPulseReceiver* ook_stats_a=nullptr;
static rflink::OokPulseReceiver* ook_stats_b=nullptr;
static rflink::NativeDecoder* decoder_stats_provider=nullptr;
static rflink::LogStore* log_provider=nullptr;
static rflink::RadioManager* manager_provider=nullptr;
static const std::atomic<uint32_t>* worker_cycles_provider=nullptr;
// HTTP/diagnostics may run before radio setup. A release/acquire handshake
// publishes all radio providers and their initial snapshot together.
static std::atomic<bool> radio_providers_published{false};
static std::atomic<bool> startup_complete_published{false};
static char update_key[65]={0};
static httpd_handle_t server=nullptr;
static esp_netif_t* station=nullptr;
static esp_netif_t* access_point=nullptr;

// Rolling, nonpersistent copy of ESP-IDF application console output. No RF
// payloads are added; access to these raw-ish lines requires the device's OTA
// management key. Previous boot's ROM/panic output cannot be replayed via LAN.
static constexpr size_t CONSOLE_CAPACITY=16384;
static constexpr size_t CONSOLE_PAGE=8192;
static char console_ring[CONSOLE_CAPACITY]{};
static uint64_t console_written=0;
static uint32_t console_boot_id=0;
static portMUX_TYPE console_lock=portMUX_INITIALIZER_UNLOCKED;
static vprintf_like_t previous_log_writer=nullptr;
static bool console_capture_installed=false;
static bool unsafe_console_text(const char* line) {
  // Never copy provisioning secrets into the LAN-visible ring. The original
  // serial terminal still receives its output unchanged.
  char lower[384]{};
  for(size_t i=0;line[i] && i<sizeof(lower)-1;i++)
    lower[i]=static_cast<char>(tolower(static_cast<unsigned char>(line[i])));
  return strstr(lower,"password") || strstr(lower,"secret") ||
         strstr(lower,"token=") || strstr(lower,"key=") ||
         strstr(lower,"authorization") || strstr(lower,"credential");
}
static int console_vprintf(const char* format,va_list args) {
  char text[384]{};
  va_list copy;
  va_copy(copy,args);
  const int length=vsnprintf(text,sizeof(text),format,copy);
  va_end(copy);
  if(length>0 && !unsafe_console_text(text)) {
    const size_t count=std::min(static_cast<size_t>(length),sizeof(text)-1);
    portENTER_CRITICAL(&console_lock);
    for(size_t i=0;i<count;i++)
      console_ring[(console_written++)%CONSOLE_CAPACITY]=text[i];
    portEXIT_CRITICAL(&console_lock);
  }
  // Keep the actual USB logging behavior exactly as before.
  return previous_log_writer?previous_log_writer(format,args):vprintf(format,args);
}

// Prototype: HTTP local only. Never expose port 80 through NAT / reverse proxy without TLS.
static esp_err_t reply(httpd_req_t *req,const char* type,const char* body){
  httpd_resp_set_type(req,type);
  httpd_resp_set_hdr(req,"Cache-Control","no-store");
  httpd_resp_set_hdr(req,"X-Content-Type-Options","nosniff");
  return httpd_resp_sendstr(req,body);
}
static bool read_str(nvs_handle_t h,const char* key,char* buf,size_t cap){
  size_t l=cap; return nvs_get_str(h,key,buf,&l)==ESP_OK;
}
static bool read_credentials(char* ssid,size_t sn,char* pass,size_t pn){
  nvs_handle_t h;
  if(nvs_open("rf_net",NVS_READONLY,&h)!=ESP_OK)return false;
  bool ok=read_str(h,"ssid",ssid,sn) && read_str(h,"pass",pass,pn) && read_str(h,"update_key",update_key,sizeof(update_key));
  nvs_close(h); return ok;
}
static std::string random_hex(size_t nbytes){
  static const char* hex="0123456789abcdef";std::string s; s.reserve(nbytes*2);
  for(size_t i=0;i<nbytes;i++){uint8_t v=esp_random()&0xff;s+=hex[v>>4];s+=hex[v&15];}
  return s;
}
static int from_hex(char c){if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;return -1;}
static std::string url_decode(const std::string& in){
  std::string s;for(size_t i=0;i<in.size();i++){
    if(in[i]=='+' )s+=' ';
    else if(in[i]=='%'&&i+2<in.size()&&from_hex(in[i+1])>=0&&from_hex(in[i+2])>=0){s+=(char)((from_hex(in[i+1])<<4)|from_hex(in[i+2]));i+=2;}
    else s+=in[i];
  }return s;
}
static std::string field(const std::string& form,const char* name){
  std::string k=std::string(name)+"=";size_t p=0;
  while(p<form.size()){
    size_t e=form.find('&',p);if(e==std::string::npos)e=form.size();
    if(form.compare(p,k.size(),k)==0)return url_decode(form.substr(p+k.size(),e-p-k.size()));
    p=e+1;
  }return "";
}
static bool key_ok(httpd_req_t* req){
  if(!update_key[0] || provisioning)return false;
  char input[96]={0};
  if(httpd_req_get_hdr_value_str(req,"X-RFLink-Update-Key",input,sizeof(input))!=ESP_OK)return false;
  // Fixed-size comparison; not a replacement for encrypted transport.
  size_t al=strlen(input),bl=strlen(update_key);unsigned diff=(unsigned)(al^bl);
  for(size_t i=0;i<64;i++){unsigned char a=i<al?(unsigned char)input[i]:0;unsigned char b=i<bl?(unsigned char)update_key[i]:0;diff|=(a^b);}
  return diff==0;
}
// User-facing label follows the ESP-IDF project version, which is tied to a
// pinned GitHub release tag. The raw board hash stays in technical details.
static const char* firmware_channel(const std::string& raw){
  if(raw.find("-exp.")!=std::string::npos)return "experimental";
  if(raw.find("-rc.")!=std::string::npos)return "pre-release";
  if(raw.find("-s20-")!=std::string::npos)return "development";
  return "stable";
}
static unsigned firmware_build_number(const std::string& raw){
  auto p=raw.find("-exp.");
  size_t prefix=5;
  if(p==std::string::npos){p=raw.find("-rc.");prefix=4;}
  if(p==std::string::npos)return 0;
  const auto end=raw.find('-',p+prefix);
  const std::string digits=raw.substr(p+prefix,end==std::string::npos?std::string::npos:end-(p+prefix));
  if(digits.empty() || digits.find_first_not_of("0123456789")!=std::string::npos)return 0;
  return static_cast<unsigned>(strtoul(digits.c_str(),nullptr,10));
}
static std::string firmware_display_version(const char* current){
  const std::string raw=current?current:"";
  auto board=raw.find("-n16r8");
  auto core=raw.substr(0,board);
  const auto exp=core.find("-exp.");
  const auto rc=core.find("-rc.");
  if(exp!=std::string::npos)return "v"+core.substr(0,exp)+" · Experimental · build "+core.substr(exp+5);
  if(rc!=std::string::npos)return "v"+core.substr(0,rc)+" · Pre-release · RC "+core.substr(rc+4);
  if(raw.find("-s20-")!=std::string::npos)return "v0.7.0 · Development";
  return "v"+core+" · Stable";
}
static esp_err_t status_get(httpd_req_t* req){
  const esp_app_desc_t* app=esp_app_get_description();
  const std::string raw_version=app->version;
  const std::string friendly_version=firmware_display_version(app->version);
  const bool radio_ready=radio_providers_published.load(std::memory_order_acquire);
  auto rx=radio_ready && rx_stats_provider ? rx_stats_provider->stats() : rflink::PacketRxStats{};
  auto a=radio_ready && ook_stats_a ? ook_stats_a->stats() : rflink::PulseStats{};
  auto b=radio_ready && ook_stats_b ? ook_stats_b->stats() : rflink::PulseStats{};
  // Include failures originating in CC1101 SPI/configuration BEFORE RMT.
  const auto da=radio_ready && ook_stats_a ? ook_stats_a->start_diagnostics() : rflink::OokStartDiagnostics{};
  const auto db=radio_ready && ook_stats_b ? ook_stats_b->start_diagnostics() : rflink::OokStartDiagnostics{};
  auto dc=radio_ready && decoder_stats_provider ? decoder_stats_provider->snapshot() : rflink::DecodeCounters{};
  const auto m0=(radio_ready && manager_provider)?manager_provider->status(rflink::RadioId::rf43342):rflink::ManagedRadio{};
  const auto m1=(radio_ready && manager_provider)?manager_provider->status(rflink::RadioId::rf43392):rflink::ManagedRadio{};
  const auto m2=(radio_ready && manager_provider)?manager_provider->status(rflink::RadioId::rf86830):rflink::ManagedRadio{};
  const auto tx_stage=(radio_ready && manager_provider)?manager_provider->last_tx_stage(rflink::RadioId::rf86830):rflink::TxFailureStage::unsupported;
  const auto s0=(radio_ready && spi_provider)?spi_provider->health(rflink::RadioId::rf43342):rflink::SpiHealthCounters{};
  const auto s1=(radio_ready && spi_provider)?spi_provider->health(rflink::RadioId::rf43392):rflink::SpiHealthCounters{};
  const auto s2=(radio_ready && spi_provider)?spi_provider->health(rflink::RadioId::rf86830):rflink::SpiHealthCounters{};
  char body[3584];
  // Advertise a stable physical identity, separate from a Builder config ID.
  // This enables explicit pairing with a saved configuration after first USB flash.
  uint8_t hardware_mac[6]{};
  char physical_id[16]{};
  if(esp_read_mac(hardware_mac,ESP_MAC_WIFI_STA)==ESP_OK){
    snprintf(physical_id,sizeof(physical_id),"rf-%02x%02x%02x%02x%02x%02x",
      hardware_mac[0],hardware_mac[1],hardware_mac[2],
      hardware_mac[3],hardware_mac[4],hardware_mac[5]);
  }
  char ipv4[16]{};
  if(station){
    esp_netif_ip_info_t address{};
    if(esp_netif_get_ip_info(station,&address)==ESP_OK && address.ip.addr)
      snprintf(ipv4,sizeof(ipv4),IPSTR,IP2STR(&address.ip));
  }
  const auto* running=esp_ota_get_running_partition();
  esp_ota_img_states_t ota_state{};
  const bool ota_state_known=running &&
    esp_ota_get_state_partition(running,&ota_state)==ESP_OK;
  snprintf(body,sizeof(body),
    "{\"firmware\":\"%s\",\"device_id\":\"%s\",\"ipv4\":\"%s\",\"firmware_display\":\"%s\",\"release_channel\":\"%s\",\"release_build\":%u,\"wifi_connected\":%s,\"mqtt_connected\":%s,\"spi_bus_ready\":%s,\"radio_initialized\":%s,\"startup_complete\":%s,"
    "\"uptime_s\":%llu,\"free_heap_bytes\":%lu,\"rx_worker_cycles\":%lu,\"ota_slot\":%lu,\"ota_state\":%d,"
    "\"cc1101_present\":[%s,%s,%s],\"spi_ready_timeouts\":[%lu,%lu,%lu],\"spi_transfer_errors\":[%lu,%lu,%lu],\"radio_operation\":[%u,%u,%u],\"tx_lab_stage\":%u,\"tx_lab_driver_present\":true,\"rx_implemented\":false,\"rx868_packet_diagnostic\":true,\"rx868_listening\":%s,\"rx868_packets\":%lu,\"rx868_fifo_overflows\":%lu,\"rx868_spi_errors\":%lu,\"rx868_recoveries\":%lu,\"rx868_idle_rearms\":%lu,\"tx_implemented\":false,"
    "\"rx43342_listening\":%s,\"rx43342_frames\":%lu,\"rx43342_rmt_error_stage\":%u,\"rx43342_rmt_error_code\":%d,\"rx43342_rmt_errors\":%lu,\"rx43342_start_error_stage\":%u,\"rx43342_start_error_code\":%d,\"rx43342_start_error_reg\":%d,\"rx43342_start_expected\":%d,\"rx43342_start_observed\":%d,\"rx43342_start_failures\":%lu,\"rx43392_listening\":%s,\"rx43392_frames\":%lu,\"rx43392_rmt_error_stage\":%u,\"rx43392_rmt_error_code\":%d,\"rx43392_rmt_errors\":%lu,\"rx43392_start_error_stage\":%u,\"rx43392_start_error_code\":%d,\"rx43392_start_error_reg\":%d,\"rx43392_start_expected\":%d,\"rx43392_start_observed\":%d,\"rx43392_start_failures\":%lu,\"decode_itho\":%lu,\"decode_ramses\":%lu,\"decode_unknown\":%lu,\"decode_overlap\":%lu,\"decode_pulses_43342\":%lu,\"decode_pulses_43392\":%lu,\"last_ramses_command\":%u,\"ota_busy\":%s,\"board_id\":\"%s\",\"board_hash\":\"%s\",\"spi_clock_hz\":%lu,\"spi_pins\":[%d,%d,%d],\"radio_pins\":[[%d,%d,%d],[%d,%d,%d],[%d,%d,%d]]}",
    app->version,physical_id,ipv4,friendly_version.c_str(),firmware_channel(raw_version),firmware_build_number(raw_version),sta_connected?"true":"false",rflink::mqtt_is_connected()?"true":"false",(radio_ready&&spi_bus_ready)?"true":"false",radio_ready?"true":"false",startup_complete_published.load(std::memory_order_acquire)?"true":"false",
    static_cast<unsigned long long>(esp_timer_get_time()/1000000),
    static_cast<unsigned long>(esp_get_free_heap_size()),
    static_cast<unsigned long>(radio_ready && worker_cycles_provider ? worker_cycles_provider->load(std::memory_order_relaxed) : 0),
    running ? static_cast<unsigned long>(running->subtype) : 0ul,
    ota_state_known ? static_cast<int>(ota_state) : -1,
    (radio_ready&&(manager_provider?m0.present:detected_radio[0]))?"true":"false",
    (radio_ready&&(manager_provider?m1.present:detected_radio[1]))?"true":"false",
    (radio_ready&&(manager_provider?m2.present:detected_radio[2]))?"true":"false",
    (unsigned long)s0.ready_timeouts,(unsigned long)s1.ready_timeouts,(unsigned long)s2.ready_timeouts,
    (unsigned long)s0.transfer_errors,(unsigned long)s1.transfer_errors,(unsigned long)s2.transfer_errors,
    unsigned(m0.operation),unsigned(m1.operation),unsigned(m2.operation),unsigned(tx_stage),
    rx.listening?"true":"false", (unsigned long)rx.packets, (unsigned long)rx.fifo_overflows, (unsigned long)rx.spi_errors,
    (unsigned long)rx.recoveries, (unsigned long)rx.idle_rearms,
    a.listening?"true":"false",(unsigned long)a.captures,
    unsigned(a.last_driver_stage),int(a.last_driver_error),(unsigned long)a.driver_errors,
    unsigned(da.stage),int(da.esp_error),int(da.register_address),
    int(da.expected),int(da.observed),(unsigned long)da.failures,
    b.listening?"true":"false",(unsigned long)b.captures,
    unsigned(b.last_driver_stage),int(b.last_driver_error),(unsigned long)b.driver_errors,
    unsigned(db.stage),int(db.esp_error),int(db.register_address),
    int(db.expected),int(db.observed),(unsigned long)db.failures,
    (unsigned long)dc.itho_messages,(unsigned long)dc.ramses_messages,(unsigned long)dc.unknown_packet_inputs,(unsigned long)dc.overlapping_decodes,
    (unsigned long)dc.pulse_inputs_43342,(unsigned long)dc.pulse_inputs_43392,
    unsigned(dc.last_ramses_command),
    ota_busy?"true":"false", rflink::kBoardId, rflink::kBoardHash,
    static_cast<unsigned long>(rflink::kSpiPins.clock_hz),
    rflink::kSpiPins.sclk,rflink::kSpiPins.mosi,rflink::kSpiPins.miso,
    rflink::kRadioPins[0].cs,rflink::kRadioPins[0].gdo0,rflink::kRadioPins[0].gdo2,
    rflink::kRadioPins[1].cs,rflink::kRadioPins[1].gdo0,rflink::kRadioPins[1].gdo2,
    rflink::kRadioPins[2].cs,rflink::kRadioPins[2].gdo0,rflink::kRadioPins[2].gdo2);
  return reply(req,"application/json",body);
}
static const char* event_kind_name(rflink::EventKind kind){
  switch(kind){
    case rflink::EventKind::raw_pulses:return "raw_pulses";
    case rflink::EventKind::unknown_packet:return "unknown_packet";
    case rflink::EventKind::itho:return "itho";
    case rflink::EventKind::ramses:return "ramses";
  }
  return "unknown";
}
static esp_err_t console_get(httpd_req_t* req) {
  if(!key_ok(req))
    return httpd_resp_send_err(req,HTTPD_403_FORBIDDEN,"Device management key required");
  char query[80]={},value[32]={};
  uint64_t after=0;
  if(httpd_req_get_url_query_str(req,query,sizeof(query))==ESP_OK &&
     httpd_query_key_value(query,"since",value,sizeof(value))==ESP_OK) {
    if(value[0] && strspn(value,"0123456789")==strlen(value))
      after=strtoull(value,nullptr,10);
  }
  // Snapshot atomically, release spinlock before JSON serialization and HTTP.
  uint64_t end=0,begin=0;
  portENTER_CRITICAL(&console_lock);
  end=console_written;
  const uint64_t oldest=end>CONSOLE_CAPACITY?end-CONSOLE_CAPACITY:0;
  begin=after>end?oldest:std::max(after,oldest);
  if(end-begin>CONSOLE_PAGE)begin=end-CONSOLE_PAGE;
  portEXIT_CRITICAL(&console_lock);
  const bool truncated=after<=end && begin>after;
  const size_t size=static_cast<size_t>(end-begin);
  std::string fragment(size,'\0');
  portENTER_CRITICAL(&console_lock);
  const uint64_t stable_end=console_written;
  // HTTP may race a busy logger. If the requested bytes wrapped during
  // allocation, abandon this page rather than returning corrupted text.
  const bool valid=stable_end-begin<=CONSOLE_CAPACITY;
  if(valid) {
    for(size_t i=0;i<size;i++)
      fragment[i]=console_ring[(begin+i)%CONSOLE_CAPACITY];
  }
  portEXIT_CRITICAL(&console_lock);
  if(!valid)return httpd_resp_send_err(req,HTTPD_500_INTERNAL_SERVER_ERROR,"Console rolling buffer changed; retry");
  cJSON* json=cJSON_CreateObject();
  if(!json)return ESP_ERR_NO_MEM;
  cJSON_AddStringToObject(json,"text",fragment.c_str());
  cJSON_AddNumberToObject(json,"cursor",static_cast<double>(begin+size));
  cJSON_AddNumberToObject(json,"boot_id",console_boot_id);
  cJSON_AddBoolToObject(json,"truncated",truncated);
  cJSON_AddNumberToObject(json,"capacity",CONSOLE_CAPACITY);
  char* output=cJSON_PrintUnformatted(json);
  cJSON_Delete(json);
  if(!output)return ESP_ERR_NO_MEM;
  const auto err=reply(req,"application/json",output);
  cJSON_free(output);
  return err;
}
static esp_err_t crashes_get(httpd_req_t* req){
  // Only reset causes and a sanitized PC/task summary are exposed here.
  // Raw core dumps can contain Wi-Fi keys and task stack secrets and must
  // never be downloadable from an unauthenticated local HTTP route.
  const std::string json=rflink::crash_report_json();
  return reply(req,"application/json",json.c_str());
}
static const char* log_code_name(uint16_t code){
  switch(static_cast<rflink::LogCode>(code)){
    case rflink::LogCode::raw_pulses: return "Raw pulses";
    case rflink::LogCode::unknown_packet: return "Unknown packet";
    case rflink::LogCode::itho: return "Itho";
    case rflink::LogCode::ramses: return "RAMSES-II";
    case rflink::LogCode::boot: return "RFLink started";
    case rflink::LogCode::radio_missing: return "Radio not responding";
    case rflink::LogCode::rx_recovered: return "Receiver restored";
    case rflink::LogCode::rx_restart_failure: return "Receiver restore failed";
    case rflink::LogCode::wifi_connected: return "Wi-Fi connected";
    case rflink::LogCode::wifi_disconnected: return "Wi-Fi disconnected";
    case rflink::LogCode::setup_started: return "Setup started";
    case rflink::LogCode::settings_saved: return "Log settings saved";
    case rflink::LogCode::settings_rejected: return "Log settings rejected";
    case rflink::LogCode::ota_attempt: return "OTA requested";
    case rflink::LogCode::ota_rejected: return "OTA rejected";
    case rflink::LogCode::tx_rejected: return "TX rejected before driver";
    case rflink::LogCode::tx_started: return "TX operation started";
    case rflink::LogCode::tx_driver_complete: return "TX driver accepted (RF unverified)";
    case rflink::LogCode::tx_failed: return "TX driver failed";
    case rflink::LogCode::tx_restore_failed: return "TX/RX restoration failed";
  }
  return "Other event";
}
static rflink::LogQuery log_query(httpd_req_t* req){
  rflink::LogQuery q{};
  char query[160]={};
  if(httpd_req_get_url_query_str(req,query,sizeof(query))!=ESP_OK)return q;
  char value[24]={};
  if(httpd_query_key_value(query,"stream",value,sizeof(value))==ESP_OK){
    if(strcmp(value,"signal")==0)q.stream=0;
    else if(strcmp(value,"noise")==0){q.stream=1;q.include_noise=true;}
    else if(strcmp(value,"system")==0)q.stream=2;
    else if(strcmp(value,"web")==0)q.stream=3;
  }
  if(httpd_query_key_value(query,"radio",value,sizeof(value))==ESP_OK){
    if(strcmp(value,"0")==0)q.radio=0;
    else if(strcmp(value,"1")==0)q.radio=1;
    else if(strcmp(value,"2")==0)q.radio=2;
  }
  if(httpd_query_key_value(query,"noise",value,sizeof(value))==ESP_OK){
    q.include_noise=strcmp(value,"1")==0;
  }
  return q;
}
static esp_err_t logs_get(httpd_req_t* req){
  rflink::LogRecord records[48]{};
  const size_t count=log_provider?
    log_provider->recent(records,48,log_query(req)):0;
  std::string json="{\"events\":[";
  char row[560];
  for(size_t i=0;i<count;++i){
    const auto& e=records[i];
    const char* stream=e.stream==0?"signal":e.stream==1?"noise":e.stream==2?"system":"web";
    snprintf(row,sizeof(row),
      "%s{\"sequence\":%llu,\"uptime_ms\":%lu,\"stream\":\"%s\",\"radio\":%u,"
      "\"kind\":\"%s\",\"code\":%u,\"severity\":%u,\"count\":%u,"
      "\"value\":%ld,\"correlation_id\":%lu,\"flags\":%u,"
      "\"profile\":%u,\"tx_transport\":%u,\"tx_stage\":%u,\"tx_repeats\":%u,"
      "\"tx_rx_previous\":%s,\"tx_rx_restored\":%s,"
      "\"tx_driver_attempted\":%s,\"tx_driver_accepted\":%s,"
      "\"tx_hardware_restored\":%s}",
      i?",":"",static_cast<unsigned long long>(e.sequence),
      static_cast<unsigned long>(e.uptime_ms),stream,unsigned(e.radio),
      log_code_name(e.code),unsigned(e.code),unsigned(e.severity),
      unsigned(e.count),static_cast<long>(e.value),
      static_cast<unsigned long>(e.correlation_id),unsigned(e.flags),
      unsigned(e.profile),unsigned(e.transport),unsigned(e.tx_stage),
      unsigned(e.repeats),(e.flags&1)?"true":"false",(e.flags&2)?"true":"false",
      (e.flags&4)?"true":"false",(e.flags&8)?"true":"false",
      (e.flags&16)?"true":"false");
    json+=row;
  }
  json+="]}";
  return reply(req,"application/json",json.c_str());
}
// Stream a bounded page at a time. Do not copy the entire volatile PSRAM
// log into the HTTP task stack or hold the RF receive lock across network I/O.
static esp_err_t logs_csv_get(httpd_req_t* req){
  httpd_resp_set_type(req,"text/csv; charset=utf-8");
  httpd_resp_set_hdr(req,"Cache-Control","no-store");
  httpd_resp_set_hdr(req,"X-Content-Type-Options","nosniff");
  httpd_resp_set_hdr(req,"Content-Disposition","attachment; filename=RFLink_Logbook.csv");
  constexpr char header[]="sequence,uptime_ms,stream,radio,kind,code,severity,count,value,correlation_id,flags,profile,tx_transport,tx_stage,driver_attempted,driver_accepted,hardware_restored,rx_restored,repeats\n";
  if(httpd_resp_send_chunk(req,header,strlen(header))!=ESP_OK)return ESP_FAIL;
  if(log_provider){
    const auto query=log_query(req);
    const uint64_t snapshot_end=log_provider->latest_sequence();
    uint64_t after=0;
    rflink::LogRecord records[24]{};
    char row[320];
    while(after<snapshot_end){
      const size_t n=log_provider->page_after(records,24,after,snapshot_end,query);
      if(!n)break;
      for(size_t i=0;i<n;++i){
        const auto& e=records[i];
        const char* stream=e.stream==0?"signal":e.stream==1?"noise":e.stream==2?"system":"web";
        const int length=snprintf(row,sizeof(row),
          "%llu,%lu,%s,%u,%s,%u,%u,%u,%ld,%lu,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",
          static_cast<unsigned long long>(e.sequence),
          static_cast<unsigned long>(e.uptime_ms),stream,unsigned(e.radio),
          log_code_name(e.code),unsigned(e.code),unsigned(e.severity),unsigned(e.count),
          static_cast<long>(e.value),static_cast<unsigned long>(e.correlation_id),
          unsigned(e.flags),unsigned(e.profile),unsigned(e.transport),unsigned(e.tx_stage),
          (e.flags&4)?1u:0u,(e.flags&8)?1u:0u,(e.flags&16)?1u:0u,(e.flags&2)?1u:0u,unsigned(e.repeats));
        if(length<=0 || static_cast<size_t>(length)>=sizeof(row) ||
           httpd_resp_send_chunk(req,row,static_cast<size_t>(length))!=ESP_OK)return ESP_FAIL;
      }
      after=records[n-1].sequence;
    }
  }
  return httpd_resp_send_chunk(req,nullptr,0);
}
static esp_err_t log_config_get(httpd_req_t* req){
  const auto stats=log_provider?log_provider->stats():rflink::LogStats{};
  char body[420];
  snprintf(body,sizeof(body),
    "{\"ready\":%s,\"signal\":[%u,%u,%u],\"noise\":%u,\"allocated_bytes\":%lu,"
    "\"budget_bytes\":%lu,\"used\":[%lu,%lu,%lu,%lu,%lu,%lu],"
    "\"overwritten\":[%llu,%llu,%llu,%llu,%llu,%llu]}",
    stats.ready?"true":"false",unsigned(stats.limits.signal[0]),unsigned(stats.limits.signal[1]),
    unsigned(stats.limits.signal[2]),unsigned(stats.limits.noise),
    static_cast<unsigned long>(stats.allocated_bytes),
    static_cast<unsigned long>(rflink::kLogBudgetBytes),
    static_cast<unsigned long>(stats.used[0]),static_cast<unsigned long>(stats.used[1]),
    static_cast<unsigned long>(stats.used[2]),static_cast<unsigned long>(stats.used[3]),
    static_cast<unsigned long>(stats.used[4]),static_cast<unsigned long>(stats.used[5]),
    static_cast<unsigned long long>(stats.overwritten[0]),
    static_cast<unsigned long long>(stats.overwritten[1]),
    static_cast<unsigned long long>(stats.overwritten[2]),
    static_cast<unsigned long long>(stats.overwritten[3]),
    static_cast<unsigned long long>(stats.overwritten[4]),
    static_cast<unsigned long long>(stats.overwritten[5]));
  return reply(req,"application/json",body);
}
static bool parsed_log_limit(const cJSON* obj,const char* key,unsigned maximum,uint16_t* out){
  const auto* item=cJSON_GetObjectItemCaseSensitive(obj,key);
  if(!cJSON_IsNumber(item)||item->valuedouble<0 ||
     item->valuedouble>maximum || item->valueint!=item->valuedouble)return false;
  *out=static_cast<uint16_t>(item->valueint);
  return true;
}
static esp_err_t log_config_post(httpd_req_t* req){
  if(!log_provider)return httpd_resp_send_err(req,HTTPD_500_INTERNAL_SERVER_ERROR,"Log store unavailable");
  if(!key_ok(req)){
    log_provider->push(rflink::LogStream::web,rflink::LogCode::settings_rejected,
      static_cast<uint32_t>(esp_timer_get_time()/1000),rflink::kAnyRadio,
      rflink::LogSeverity::warning);
    return httpd_resp_send_err(req,HTTPD_403_FORBIDDEN,"Management key required");
  }
  if(ota_busy.load())return httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"OTA in progress");
  if(req->content_len<=0||req->content_len>320)
    return httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"Invalid settings body");
  std::string body(static_cast<size_t>(req->content_len),'\0');
  size_t offset=0;int timeouts=0;
  while(offset<body.size()){
    int n=httpd_req_recv(req,&body[offset],body.size()-offset);
    if(n==HTTPD_SOCK_ERR_TIMEOUT && ++timeouts<=5)continue;
    if(n<=0)return ESP_FAIL;
    offset+=static_cast<size_t>(n);
  }
  cJSON* parsed=cJSON_ParseWithLength(body.data(),body.size());
  rflink::LogLimits limits{};
  bool valid=parsed &&
    parsed_log_limit(parsed,"signal_0",4000,&limits.signal[0]) &&
    parsed_log_limit(parsed,"signal_1",4000,&limits.signal[1]) &&
    parsed_log_limit(parsed,"signal_2",4000,&limits.signal[2]) &&
    parsed_log_limit(parsed,"noise",10000,&limits.noise) &&
    rflink::LogStore::valid_limits(limits);
  if(parsed)cJSON_Delete(parsed);
  if(!valid)return httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"Log limits exceed 1.5 MB memory cap or invalid");
  if(!log_provider->configure(limits))
    return httpd_resp_send_err(req,HTTPD_500_INTERNAL_SERVER_ERROR,"Not enough PSRAM for log limits");
  rflink::SavedLogLimits saved{};saved.limits=limits;
  nvs_handle_t handle{};
  bool persisted=false;
  if(nvs_open("rf_logs",NVS_READWRITE,&handle)==ESP_OK){
    persisted=nvs_set_blob(handle,"limits",&saved,sizeof(saved))==ESP_OK &&
              nvs_commit(handle)==ESP_OK;
    nvs_close(handle);
  }
  log_provider->push(rflink::LogStream::web,
    persisted?rflink::LogCode::settings_saved:rflink::LogCode::settings_rejected,
    static_cast<uint32_t>(esp_timer_get_time()/1000),rflink::kAnyRadio,
    persisted?rflink::LogSeverity::info:rflink::LogSeverity::warning);
  return reply(req,"application/json",persisted?
    "{\"applied\":true,\"saved\":true}":
    "{\"applied\":true,\"saved\":false,\"warning\":\"Settings only active until reboot\"}");
}
static esp_err_t events_get(httpd_req_t* req){
  rflink::RxRecentEvent events[rflink::kRecentEventCapacity]{};
  const size_t count=radio_providers_published.load(std::memory_order_acquire) && decoder_stats_provider ?
    decoder_stats_provider->recent_events(events,rflink::kRecentEventCapacity):0;
  std::string body="{\"events\":[";
  char line[192];
  for(size_t i=0;i<count;i++){
    const auto& ev=events[i];
    snprintf(line,sizeof(line),
      "%s{\"seq\":%lu,\"uptime_ms\":%lu,\"radio\":%u,\"kind\":\"%s\",\"count\":%u,\"command\":%u,\"overlap\":%s}",
      i?",":"",static_cast<unsigned long>(ev.sequence),static_cast<unsigned long>(ev.uptime_ms),
      unsigned(ev.radio),event_kind_name(ev.kind),unsigned(ev.count),unsigned(ev.command),
      ev.overlapping?"true":"false");
    body+=line;
  }
  body+="]}";
  return reply(req,"application/json",body.c_str());
}
static esp_err_t events_csv_get(httpd_req_t* req){
  // Only benign, numeric and enumerated metadata is exported, never RF raw
  // data, device addresses or rolling-code secrets.
  rflink::RxRecentEvent events[rflink::kRecentEventCapacity]{};
  const size_t count=radio_providers_published.load(std::memory_order_acquire) && decoder_stats_provider ?
    decoder_stats_provider->recent_events(events,rflink::kRecentEventCapacity):0;
  std::string csv="sequence,uptime_ms,radio,kind,count,command,overlap\n";
  char line[128];
  for(size_t i=0;i<count;i++){
    const auto& ev=events[i];
    snprintf(line,sizeof(line),"%lu,%lu,%u,%s,%u,%u,%u\n",
      static_cast<unsigned long>(ev.sequence),static_cast<unsigned long>(ev.uptime_ms),
      unsigned(ev.radio),event_kind_name(ev.kind),unsigned(ev.count),
      unsigned(ev.command),ev.overlapping?1:0);
    csv+=line;
  }
  httpd_resp_set_hdr(req,"Content-Disposition","attachment; filename=RFLink_rx_events.csv");
  return reply(req,"text/csv; charset=utf-8",csv.c_str());
}
// The management API uses the same installation key as OTA. Requests are
// authenticated but transmitted over plaintext HTTP: lab/trusted LAN only.
// Never include passwords in responses, diagnostics, or logs.
static esp_err_t mqtt_status_get(httpd_req_t* req){
  rflink::MqttSettings settings{};
  const bool configured=rflink::mqtt_load_settings(&settings);
  char body[400];
  snprintf(body,sizeof(body),
    "{\"configured\":%s,\"connected\":%s,\"tls_supported\":false,"
    "\"password_saved\":%s,\"host\":\"%s\",\"port\":%u}",
    configured?"true":"false",rflink::mqtt_is_connected()?"true":"false",
    configured && settings.password[0]?"true":"false",configured?settings.host:"",
    configured?unsigned(settings.port):0);
  return reply(req,"application/json",body);
}
static esp_err_t mqtt_config_post(httpd_req_t* req){
  if(!key_ok(req))return httpd_resp_send_err(req,HTTPD_403_FORBIDDEN,"RFLink management key required");
  if(ota_busy.load())return httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"OTA in progress");
  if(req->content_len<=0 || req->content_len>1200)return httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"Invalid JSON size");
  std::string json(static_cast<size_t>(req->content_len),'\0');
  size_t offset=0;int failures=0;
  while(offset<json.size()){
    const int n=httpd_req_recv(req,&json[offset],json.size()-offset);
    if(n==HTTPD_SOCK_ERR_TIMEOUT && ++failures<=5)continue;
    if(n<=0)return ESP_FAIL;
    offset+=static_cast<size_t>(n);failures=0;
  }
  cJSON* obj=cJSON_ParseWithLength(json.data(),json.size());
  if(!obj)return httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"Malformed JSON");
  const auto* host=cJSON_GetObjectItemCaseSensitive(obj,"host");
  const auto* username=cJSON_GetObjectItemCaseSensitive(obj,"username");
  const auto* password=cJSON_GetObjectItemCaseSensitive(obj,"password");
  const auto* port=cJSON_GetObjectItemCaseSensitive(obj,"port");
  const auto* tls=cJSON_GetObjectItemCaseSensitive(obj,"tls");
  bool ok=cJSON_IsString(host)&&cJSON_IsString(username)&&cJSON_IsString(password)&&
          cJSON_IsNumber(port)&&port->valuedouble>=1&&port->valuedouble<=65535&&
          port->valuedouble==static_cast<int>(port->valuedouble)&&cJSON_IsBool(tls)&&cJSON_IsFalse(tls);
  rflink::MqttSettings candidate{};
  if(ok){
    const size_t hn=strlen(host->valuestring),un=strlen(username->valuestring),pn=strlen(password->valuestring);
    ok=hn<sizeof(candidate.host)&&un<sizeof(candidate.username)&&pn<sizeof(candidate.password);
    if(ok){
      memcpy(candidate.host,host->valuestring,hn);
      memcpy(candidate.username,username->valuestring,un);
      memcpy(candidate.password,password->valuestring,pn);
      candidate.port=static_cast<uint16_t>(port->valueint);
      ok=rflink::mqtt_valid_settings(candidate);
    }
  }
  cJSON_Delete(obj);
  if(!ok)return httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"Unsupported or invalid MQTT settings (TLS requires a future verified-certificate build)");
  if(!rflink::mqtt_save_settings(candidate))return httpd_resp_send_err(req,HTTPD_500_INTERNAL_SERVER_ERROR,"NVS write failed");
  httpd_resp_set_hdr(req,"Cache-Control","no-store");
  httpd_resp_set_type(req,"application/json");
  httpd_resp_sendstr(req,"{\"ok\":true,\"restart_required\":true}");
  xTaskCreate([](void*){vTaskDelay(pdMS_TO_TICKS(1500));esp_restart();},"mqtt_restart",3072,nullptr,4,nullptr);
  return ESP_OK;
}
// Keep the known-working provisioning form and technical recovery view.
static esp_err_t native_index_get(httpd_req_t* req);
static esp_err_t native_styles_get(httpd_req_t* req){
  return reply(req,"text/css; charset=utf-8",
               rflink::kNativeUiCss);
}
static esp_err_t native_script_get(httpd_req_t* req){
  return reply(req,"application/javascript; charset=utf-8",
               rflink::kNativeUiJs);
}
static esp_err_t index_get(httpd_req_t* req){
  const esp_app_desc_t* app=esp_app_get_description();
  std::string page=R"HTML(<!doctype html><html lang="nl"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>RFLink Radioactive — test</title><style>body{font:15px system-ui;max-width:850px;margin:22px auto;padding:16px;background:#101820;color:#eaf2f4;line-height:1.45}section{border:1px solid #334351;background:#15212d;border-radius:12px;padding:16px;margin:12px 0}h1{margin:0 0 6px;font-size:24px}h2{font-size:17px;margin:0 0 12px}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(190px,1fr));gap:10px}.rf-card{border-radius:10px;border:1px solid #384755;padding:12px;background:#0e1b25}.rf-card strong,.rf-card span{display:block}.rf-card span{font-size:13px;color:#bdc8ce;margin-top:4px}.system-mini{display:grid;grid-template-columns:repeat(auto-fit,minmax(220px,1fr));gap:10px;align-items:start}.system-mini .rf-card{min-height:0}.system-mini .rf-card span{overflow-wrap:anywhere}input,button{font:inherit;padding:9px;max-width:95%;margin:6px 0;border-radius:7px}button{background:#57c7ae;color:#092020;border:0;cursor:pointer}button:disabled{opacity:.5}label{display:block}small,.muted{color:#bdc8ce}.warn{color:#ffb06d}details summary{cursor:pointer;padding:5px 0}pre{white-space:pre-wrap;font:13px ui-monospace,monospace;color:#d5e4e8}.log-toolbar,.log-sizes{display:grid;grid-template-columns:repeat(auto-fit,minmax(145px,1fr));gap:8px;align-items:end}.log-toolbar label,.log-sizes label{font-size:13px;color:#bdc8ce}.log-toolbar select,.log-sizes input{display:block;background:#0e1b25;color:#eaf2f4;border:1px solid #384755;border-radius:7px;padding:8px;width:100%;box-sizing:border-box}.log-toolbar input[type=checkbox]{width:auto}#log-lines{max-height:265px;min-height:85px;overflow:auto;background:#0e1b25;padding:10px;border-radius:8px}progress{width:100%}a{color:#81dac6}a:focus-visible,button:focus-visible{outline:2px solid #81dac6;outline-offset:3px}.status{font-size:13px;color:#a8dbcb}</style><h1>RFLink Universal</h1><p class="warn">Radioactive v0.7 · experimentele testfirmware, nog niet hardware-gevalideerd.</p>)HTML";
  page+="<section><strong>Firmware:</strong> "+firmware_display_version(app->version)+
    "<p>WiFi: "+(sta_connected?"verbonden":"niet verbonden")+
    "</p><details><summary>Technische buildgegevens</summary><code>"+std::string(app->version)+"</code></details></section>";
  if(provisioning){
    page+=R"HTML(<section><h2>WiFi instellen</h2><p>Alleen uitvoeren op je eigen RFLink-installatie. Dit tijdelijke accesspoint wordt daarna uitgeschakeld.</p><form method="post" action="/setup"><label>WiFi SSID <input name="ssid" maxlength="32" required></label><label>WiFi wachtwoord <input name="pass" type="password" maxlength="63"></label><label>Uniek OTA beheerderswachtwoord (min. 12 tekens) <input name="admin" type="password" minlength="12" maxlength="64" required></label><button>Opslaan en herstarten</button></form></section>)HTML";
  }else{
    page+=R"HTML(<section><h2>Firmware bijwerken via browser</h2><p>Upload uitsluitend een <code>firmware.bin</code> die voor deze specifieke testboardpartities is gebouwd.</p><label>OTA beheerderswachtwoord <input id="key" type="password" autocomplete="off"></label><label>ESP-IDF app .bin <input id="fw" type="file" accept=".bin,application/octet-stream"></label><button onclick="upload()">Installeren</button><progress id="p" value="0" max="100"></progress><p id="msg"></p></section><script>async function upload(){const f=document.getElementById('fw').files[0],k=document.getElementById('key').value,m=document.getElementById('msg'),p=document.getElementById('p');if(!f||!k){m.textContent='Kies bestand en vul wachtwoord in';return;}if(!confirm('Deze testfirmware installeren en herstarten?'))return;const x=new XMLHttpRequest();x.open('POST','/api/ota');x.setRequestHeader('X-RFLink-Update-Key',k);x.setRequestHeader('Content-Type','application/octet-stream');x.upload.onprogress=e=>{if(e.lengthComputable)p.value=Math.round(e.loaded/e.total*100)};x.onload=()=>{m.textContent=x.responseText};x.onerror=()=>{m.textContent='Verbinding verbroken; controleer apparaatstatus'};m.textContent='Upload gestart';x.send(f);}</script>)HTML";
  }
  page+=R"HTML(<section><h2>Home Assistant / MQTT</h2><p class="muted" id="mqttinfo">Verbinding ophalen…</p></section><script>fetch("/api/mqtt/status",{cache:"no-store"}).then(r=>r.json()).then(s=>{document.getElementById("mqttinfo").textContent=!s.configured?"Nog geen broker ingesteld":(s.connected?"Verbonden met ":"Niet verbonden met ")+s.host+":"+s.port+" · MQTT Discovery (RFLink hub)"}).catch(()=>{document.getElementById("mqttinfo").textContent="MQTT-status niet bereikbaar"})</script>)HTML";
  // Local read-only diagnostics. Does not expose raw RF identities or packet bytes.
  page+=R"HTML(<section><h2>Radio-overzicht</h2><div class="grid">
    <div class="rf-card"><strong>433,42 MHz</strong><span id="rx0">Status ophalen…</span></div>
    <div class="rf-card"><strong>433,92 MHz</strong><span id="rx1">Status ophalen…</span></div>
    <div class="rf-card"><strong>868,30 MHz</strong><span id="rx2">Status ophalen…</span></div>
  </div><p class="status" id="rxstatus">Ontvangst wordt gecontroleerd…</p><p class="muted" id="decoderstatus">Decoderstatus ophalen…</p><button type="button" onclick="rflinkRefresh()">Vernieuwen</button></section>
  <section><h2>Crashes and restarts</h2><p class="muted">Stored on the device by default, even when Home Assistant is offline.</p>
  <div class="system-mini"><div class="rf-card"><strong>Last restart</strong><span id="last-reset">Loading…</span></div>
  <div class="rf-card"><strong>Stored crash report</strong><span id="crash-state">Loading…</span></div>
  <div class="rf-card"><strong>Crash firmware</strong><span id="crash-build-state">Loading…</span></div>
   <div class="rf-card"><strong>Last boot checkpoint</strong><span id="boot-checkpoint">Loading…</span></div></div>
  <details><summary>Boot history and build identification</summary>
    <pre id="crash-build-details">Loading…</pre>
    <pre id="crash-history">Loading…</pre>
    <p><a href="/api/diagnostics/crashes" download="RFLink_crash_report.json">Save sanitized crash report (JSON)</a></p>
  </details></section>
  <section><h2>Logbooks</h2><p class="muted">RX and TX · System · Web · newest 48. A TX driver success is not proof of an on-air signal.</p>
  <div class="log-toolbar">
    <label>Source<select id="log-stream"><option value="all">All</option><option value="signal">Radio signals</option><option value="noise">Radio noise</option><option value="system">System</option><option value="web">Web</option></select></label>
    <label>Radio<select id="log-radio"><option value="all">All radios</option><option value="0">433.42 MHz</option><option value="1">433.92 MHz</option><option value="2">868.30 MHz</option></select></label>
    <label><input id="log-noise" type="checkbox"> Include noise</label>
  </div>
  <pre id="log-lines">Loading recent events…</pre><p class="muted" id="log-summary">Values are sanitized metadata; received data and private IDs are not exposed here.</p>
  <p class="muted"><a id="log-export" href="/api/logs.csv?stream=all&radio=all&noise=0" download="RFLink_Logbook.csv">Export filtered logbook (CSV)</a> · <a href="/api/events.csv" download="RFLink_rx_events.csv">Legacy 32 RX events</a></p><p class="muted">Live rings can overwrite old events during export. No stored RF payloads or secrets are included.</p></section>
  <section><h2>Log retention</h2><p class="muted">Compact memory-limited settings. Save with your RFLink management key.</p>
  <div class="log-sizes">
    <label>433.42 signals<input id="log-size-0" type="number" min="0" max="4000" value="250"></label>
    <label>433.92 signals<input id="log-size-1" type="number" min="0" max="4000" value="250"></label>
    <label>868.30 signals<input id="log-size-2" type="number" min="0" max="4000" value="250"></label>
    <label>Shared noise<input id="log-size-noise" type="number" min="0" max="10000" value="2000"></label>
  </div>
  <div class="log-toolbar">
    <label>Management key<input id="log-key" type="password" autocomplete="off" placeholder="Required to save"></label>
    <button type="button" onclick="saveLogLimits()">Save log sizes</button>
  </div>
  <p class="muted" id="log-feedback" role="status">Memory usage loading…</p></section>
  <section><details><summary><strong>Hardware en SPI</strong> <small>· geavanceerd · alleen lezen</small></summary>
    <p class="muted">De GPIO-aansluitingen en de SPI-snelheid staan vast in deze firmwarebuild. Wijzig <code>boards/rflink_s3_n16r8.json</code> en compileer opnieuw om ze te veranderen. Niet live instelbaar.</p>
    <pre id="boardinfo">Hardwareprofiel ophalen…</pre>
  </details></section>
  <script>
  async function refreshLogbooks(){
    try {
      const stream=document.getElementById('log-stream').value;
      const radio=document.getElementById('log-radio').value;
      const noise=document.getElementById('log-noise').checked?'1':'0';
      const query='?stream='+encodeURIComponent(stream)+'&radio='+encodeURIComponent(radio)+'&noise='+noise;
      document.getElementById('log-export').href='/api/logs.csv'+query;
      const response=await fetch('/api/logs'+query,{cache:'no-store'});
      if(!response.ok)throw Error('HTTP '+response.status);
      const rows=(await response.json()).events||[];
      const radios=['433.42','433.92','868.30'];
      const readable=rows.slice().reverse().map(e=>{
        const seconds=(e.uptime_ms/1000).toFixed(3)+'s';
        const source=e.radio<3?radios[e.radio]+' MHz':e.stream;
        const isTx=e.code>=30 && e.code<=34;
        const stageNames=['none','unsupported transport','snapshot','modem setup','GPIO UART','enter TX',
          'UART write','UART drain','FIFO write','FIFO wait','RMT setup','RMT send','RMT wait','restore','pause RX'];
        const errors=['none','invalid radio','radio missing','busy','wrong profile','profile missing',
          'unsupported transport','driver failure','invalid payload','TX failed','restore failed'];
        const transportNames=['UART','RMT','FIFO'];
        const reason=isTx && e.value?' · '+(errors[e.value]||'error '+e.value):'';
        const ipValue=e.value>>>0;
        const connection=e.code===14
          ?' · IPv4 '+[ipValue&255,(ipValue>>>8)&255,(ipValue>>>16)&255,(ipValue>>>24)&255].join('.')
          :e.code===15?' · Wi-Fi disconnect reason '+e.value:'';
        const status=isTx
          ?' · '+(transportNames[e.tx_transport]||'transport unknown')+
           ' · driver '+(e.tx_driver_attempted?
             (e.tx_driver_accepted?'accepted (RF not verified)':'failed'):'not reached')+
           ' · RX '+(e.code===30?'unchanged':e.code===31?'transition pending':
             e.tx_rx_previous?(e.tx_rx_restored?'restored':'NOT restored'):'no previous RX')+
           (e.tx_stage?' · phase '+(stageNames[e.tx_stage]||e.tx_stage):'')+reason
          :connection;
        const corr=e.correlation_id?' · TX #'+e.correlation_id:'';
        const warning=e.severity===2?'ERROR · ':e.severity===1?'WARN · ':'';
        return '#'+e.sequence+' · '+seconds+' · '+source+' · '+warning+e.kind+
          (e.count?' · '+e.count+(isTx?' units':' items'):'')+
          (isTx && e.tx_repeats?' · '+e.tx_repeats+' repeats':'')+status+corr;
      });
      document.getElementById('log-lines').textContent=readable.length?readable.join('\n'):'No matching events yet.';
    }catch(e){document.getElementById('log-lines').textContent='Logbook unavailable: '+e.message;}
  }
  async function loadLogLimits(){
    try {
      const response=await fetch('/api/logs/config',{cache:'no-store'});
      if(!response.ok)throw Error('HTTP '+response.status);
      const c=await response.json();
      for(let i=0;i<3;i++)document.getElementById('log-size-'+i).value=c.signal[i];
      document.getElementById('log-size-noise').value=c.noise;
      const overwritten=(c.overwritten||[]).reduce((a,v)=>a+v,0);
      document.getElementById('log-feedback').textContent=(c.allocated_bytes/1024).toFixed(1)+' KB allocated · memory cap '+(c.budget_bytes/1048576).toFixed(1)+' MB · overwritten '+overwritten+(c.ready?'':' · log storage unavailable');
    }catch(e){document.getElementById('log-feedback').textContent='Log settings unavailable: '+e.message;}
  }
  async function saveLogLimits(){
    const feedback=document.getElementById('log-feedback');
    const key=document.getElementById('log-key').value;
    if(!key){feedback.textContent='Enter your management key to save log sizes.';return;}
    const fields=['log-size-0','log-size-1','log-size-2','log-size-noise'].map(id=>Number(document.getElementById(id).value));
    if(fields.some(n=>!Number.isInteger(n)||n<0)){feedback.textContent='Enter non-negative whole numbers.';return;}
    feedback.textContent='Saving…';
    try {
      const response=await fetch('/api/logs/config',{method:'POST',headers:{
        'Content-Type':'application/json','X-RFLink-Update-Key':key},body:JSON.stringify({
        signal_0:fields[0],signal_1:fields[1],signal_2:fields[2],noise:fields[3]})});
      if(!response.ok)throw Error((await response.text()).slice(0,120));
      const result=await response.json();
      await loadLogLimits();
      feedback.textContent=result.saved?'Saved. The new limits are active and will survive a restart.':
        'Applied for this session, but could not be stored in flash.';
      if(result.saved)document.getElementById('log-key').value='';
      refreshLogbooks();
    }catch(e){feedback.textContent='Could not save: '+e.message;}
  }
  for(const id of ['log-stream','log-radio','log-noise']){
    document.getElementById(id).addEventListener('change',refreshLogbooks);
  }
  async function refreshCrashes(){
    try{
      const response=await fetch('/api/diagnostics/crashes',{cache:'no-store'});
      if(!response.ok)throw Error('HTTP '+response.status);
      const data=await response.json(),crash=data.core_dump||{},running=data.running_firmware||{};
      document.getElementById('last-reset').textContent=data.last_reset||'Unknown';
      const integrity=crash.integrity||'unknown';
      document.getElementById('crash-state').textContent=integrity==='valid'
        ?crash.summary_available?'Saved · '+crash.task+' · PC '+crash.pc:'Saved · summary unavailable'
        :integrity==='not_found'?'No crash report stored':'Crash data invalid or unreadable';
      const comparison=crash.build_comparison||'unknown';
      const buildLabel=comparison==='same_full_sha'?'Full ELF hash matches current firmware':
        comparison==='matching_prefix'?'ELF hash prefix matches current firmware':
        comparison==='different'?'Different firmware build (possibly before OTA)':
        'Unknown firmware build';
      document.getElementById('crash-build-state').textContent=
        integrity==='valid'?buildLabel:integrity==='not_found'?'No saved crash':'Build cannot be identified';
      document.getElementById('crash-build-details').textContent=
        'Current version: '+(running.version||'Unknown')+
        '\nCurrent ELF hash: '+(running.elf_sha256||'Unavailable')+
        '\nSaved crash ELF hash: '+(crash.elf_sha256||'Unavailable')+
        '\nComparison: '+buildLabel+
        '\nNote: a saved dump may predate the last normal restart. A matching prefix is not a full-hash proof.';
      const history=data.history||[];
      const phaseList=Array.isArray(data.boot_phases)?data.boot_phases:[];
      const phaseMap=new Map(phaseList.map(e=>[e.boot,e]));
      const current=history.length?phaseMap.get(history[0].boot):null;
      document.getElementById('boot-checkpoint').textContent=current
        ?current.stage.replaceAll('_',' ')+(current.startup_complete?' · startup completed':' · last reached')
        :'No saved startup checkpoint';
      document.getElementById('crash-history').textContent=
        (data.history_saved?'':'Warning: reset history was not saved in flash\n')+
        (data.phase_history_saved?'':'Warning: boot checkpoints were not saved in flash\n')+
        (history.length?history.map(e=>{
          const phase=phaseMap.get(e.boot);
          return '#'+e.boot+' · '+e.reason+
            (e.has_core_dump?' · '+e.task+' PC '+e.pc:'')+
            (phase?' · checkpoint: '+phase.stage.replaceAll('_',' ')+
              (phase.startup_complete?' (startup completed)':' (last reached)'):'');
        }).join('\n'):'No restart history available')+
        '\nA checkpoint shows the last reached stage, not a proven crash cause.';
    }catch(e){
      document.getElementById('last-reset').textContent='Diagnostics unavailable';
      document.getElementById('crash-state').textContent='Unavailable';
    }
  }
  async function rflinkRefresh(){
    const status=document.getElementById('rxstatus');
    try{
      const response=await fetch('/api/status',{cache:'no-store'});if(!response.ok)throw Error('HTTP '+response.status);
      const s=await response.json(),freq=['433,42','433,92','868,30'];
      const totals=[s.rx43342_frames,s.rx43392_frames,s.rx868_packets];
      const listening=[s.rx43342_listening,s.rx43392_listening,s.rx868_listening];
      const rmtStages=['none','queue','channel allocation','GDO2 pulldown','callbacks','enable','receive arm','receive rearm'];
      const rmtFailures=[
        {stage:s.rx43342_rmt_error_stage||0,code:s.rx43342_rmt_error_code||0},
        {stage:s.rx43392_rmt_error_stage||0,code:s.rx43392_rmt_error_code||0}
      ];
      const startStages=['none','mutex','profile','CC1101 absent','SIDLE',
        'frequency SPI write','modem SPI write','modem SPI read','modem readback mismatch',
        'RMT receiver start','CC1101 RX state entry'];
      const startFailures=[
        {stage:s.rx43342_start_error_stage||0,code:s.rx43342_start_error_code,
          reg:s.rx43342_start_error_reg,expected:s.rx43342_start_expected,got:s.rx43342_start_observed},
        {stage:s.rx43392_start_error_stage||0,code:s.rx43392_start_error_code,
          reg:s.rx43392_start_error_reg,expected:s.rx43392_start_expected,got:s.rx43392_start_observed}
      ];
      s.cc1101_present.forEach((v,i)=>{
        const el=document.getElementById('rx'+i);
        el.textContent=s.radio_initialized===false?'Radio initialization pending':
          (v?(listening[i]?'Ontvanger actief':'Hardware aanwezig, RX niet actief'):'Geen CC1101 gedetecteerd')+' · '+totals[i]+' ruwe '+(i===2?'pakketten':'frames')+' · SPI time-outs '+s.spi_ready_timeouts[i]+
          (i<2 && startFailures[i].stage?
            ' · RX start fout: '+(startStages[startFailures[i].stage]||'stage '+startFailures[i].stage)+
            ' (ESP-IDF '+startFailures[i].code+
            (startFailures[i].reg>=0?' · CC1101 0x'+startFailures[i].reg.toString(16).padStart(2,'0'):'')+
            (startFailures[i].expected>=0&&startFailures[i].got>=0?
              ' verwacht 0x'+startFailures[i].expected.toString(16)+' gelezen 0x'+startFailures[i].got.toString(16):'')+')':'')+
          (i<2 && rmtFailures[i].stage?
            ' · RMT fout: '+(rmtStages[rmtFailures[i].stage]||'stage '+rmtFailures[i].stage)+
            ' (ESP-IDF '+rmtFailures[i].code+')':'');
      });
      status.textContent=s.radio_initialized===false?
        'Diagnostics online · radio initialization in progress · '+s.firmware:
        (s.wifi_connected?'WiFi verbonden':'WiFi niet verbonden')+' · RX-worker '+s.rx_worker_cycles+' cycli · '+s.firmware;
      document.getElementById('decoderstatus').textContent=s.radio_initialized===false?
        'Decoder initialization pending':
        'Herkend: Itho '+s.decode_itho+' · RAMSES-II '+s.decode_ramses+' · dubbele herkenningen '+s.decode_overlap+' (testdecoder)';
      const er=await fetch('/api/events',{cache:'no-store'});
      if(er.ok){
        const ev=(await er.json()).events||[];
        const names=['433,42','433,92','868,30'];
        document.getElementById('events').textContent=ev.length ? ev.slice().reverse().map(v=>
          '#'+v.seq+' · '+(v.uptime_ms/1000).toFixed(1)+'s · '+(names[v.radio]||'?')+' MHz · '+v.kind+' · '+v.count+(v.command?' · cmd 0x'+v.command.toString(16):'')+(v.overlap?' · overlap':'')).join('\n'):'Nog geen signalen ontvangen';
      }
      const p=s.spi_pins,radio=s.radio_pins;
      document.getElementById('boardinfo').textContent='Board: '+s.board_id+' ('+s.board_hash+')\nSPI: SCLK GPIO'+p[0]+' · MOSI GPIO'+p[1]+' · MISO GPIO'+p[2]+' · '+(s.spi_clock_hz/1000000).toFixed(2)+' MHz\n'+radio.map((r,i)=>freq[i]+' MHz: CS GPIO'+r[0]+' · GDO0 GPIO'+r[1]+' · GDO2 GPIO'+r[2]).join('\n');
    }catch(e){status.textContent='Status niet bereikbaar';}
  }
  rflinkRefresh();refreshCrashes();refreshLogbooks();loadLogLimits();
  setInterval(rflinkRefresh,5000);setInterval(refreshLogbooks,5000);
  setInterval(refreshCrashes,5000);
  </script>)HTML";
  page+="<p><small>Itho/RAMSES decoder van 868 pakketten in test. MQTT Discovery meldt alleen de RFLink-hub; RF-apparaten worden nog niet gepubliceerd. UART-TX heeft alleen een interne testinterface; er is geen web-TX-knop. HTTP is niet veilig op onbeheerde netwerken.</small></p></html>";
  return reply(req,"text/html; charset=utf-8",page.c_str());
}
static esp_err_t setup_post(httpd_req_t* req){
  if(!provisioning)return httpd_resp_send_err(req,HTTPD_403_FORBIDDEN,"Already configured");
  if(req->content_len<=0||req->content_len>MAX_FORM)return httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"Bad form size");
  std::string body(req->content_len,'\0');size_t off=0;
  while(off<body.size()){int n=httpd_req_recv(req,&body[off],body.size()-off);if(n<=0)return ESP_FAIL;off+=n;}
  auto ssid=field(body,"ssid"),password=field(body,"pass"),admin=field(body,"admin");
  if(ssid.empty()||ssid.size()>32||password.size()>63||admin.size()<12||admin.size()>64)
    return httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"Invalid SSID/password/admin length");
  nvs_handle_t h;
  if(nvs_open("rf_net",NVS_READWRITE,&h)!=ESP_OK)return httpd_resp_send_err(req,HTTPD_500_INTERNAL_SERVER_ERROR,"NVS unavailable");
  auto e=nvs_set_str(h,"ssid",ssid.c_str());
  if(e==ESP_OK)e=nvs_set_str(h,"pass",password.c_str());
  if(e==ESP_OK)e=nvs_set_str(h,"update_key",admin.c_str());
  if(e==ESP_OK)e=nvs_commit(h);
  nvs_close(h);
  if(e!=ESP_OK)return httpd_resp_send_err(req,HTTPD_500_INTERNAL_SERVER_ERROR,"NVS write error");
  httpd_resp_sendstr(req,"Saved. Device will restart. Open its IP on your WiFi network.");
  xTaskCreate([](void*){vTaskDelay(pdMS_TO_TICKS(1000));esp_restart();},"setup_restart",3072,nullptr,4,nullptr);
  return ESP_OK;
}
static esp_err_t ota_post(httpd_req_t* req){
  if(!key_ok(req))return httpd_resp_send_err(req,HTTPD_403_FORBIDDEN,"Update password required");
  const esp_partition_t* next=esp_ota_get_next_update_partition(nullptr);
  if(!next)return httpd_resp_send_err(req,HTTPD_500_INTERNAL_SERVER_ERROR,"No OTA partition");
  if(req->content_len<256 || static_cast<size_t>(req->content_len)>next->size ||
     static_cast<size_t>(req->content_len)>MAX_IMAGE)
    return httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"Invalid image length for OTA slot");
  if(ota_busy.exchange(true))return httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"Update in progress");
  esp_ota_handle_t h=0;
  esp_err_t e=esp_ota_begin(next,req->content_len,&h);
  size_t received=0;uint8_t buffer[2048];int timeout_count=0;
  while(e==ESP_OK && received<(size_t)req->content_len){
    int n=httpd_req_recv(req,(char*)buffer,std::min(sizeof(buffer),(size_t)req->content_len-received));
    if(n==HTTPD_SOCK_ERR_TIMEOUT){if(++timeout_count<10)continue;e=ESP_ERR_TIMEOUT;break;}
    if(n<=0){e=ESP_FAIL;break;}timeout_count=0;
    if(received==0 && buffer[0]!=0xE9){e=ESP_ERR_INVALID_ARG;break;}
    e=esp_ota_write(h,buffer,n);
    received+=n;
  }
  if(e==ESP_OK)e=esp_ota_end(h);
  else if(h)esp_ota_abort(h);
  if(e==ESP_OK){
    // Reject a valid ESP32 app from a different project before activating it.
    esp_app_desc_t candidate{};
    const esp_app_desc_t* running=esp_app_get_description();
    e=esp_ota_get_partition_description(next,&candidate);
    if(e==ESP_OK && !rflink::native_ota_identity_accepted(
         candidate.project_name,sizeof(candidate.project_name),
         candidate.version,sizeof(candidate.version))){
      ESP_LOGE(TAG,"OTA candidate rejected: project/board identity mismatch");
      e=ESP_ERR_INVALID_ARG;
    }
    // Cross-check metadata of the running firmware, too. This deliberately
    // prevents OTA from an incompatible development build to the native N16R8
    // track. It does not establish cryptographic image authenticity.
    if(e==ESP_OK && !rflink::native_ota_identity_accepted(
         running->project_name,sizeof(running->project_name),
         running->version,sizeof(running->version))) {
      ESP_LOGE(TAG,"Running image does not belong to the N16R8 native track");
      e=ESP_ERR_INVALID_STATE;
    }
  }
  if(e==ESP_OK){
    esp_app_desc_t candidate{};
    e=esp_ota_get_partition_description(next,&candidate);
    if(e==ESP_OK && !rflink::same_board_profile_for_ota(
        esp_app_get_description()->version,sizeof(esp_app_get_description()->version),
        candidate.version,sizeof(candidate.version))){
      ESP_LOGE(TAG,"OTA refused: board pinout or SPI speed differs from running image");
      e=ESP_ERR_INVALID_ARG;
    }
  }
  if(e==ESP_OK){
    // esp_ota_set_boot_partition validates the app image. Hardware revision must be
    // validated separately before this becomes an official migration route.
    e=esp_ota_set_boot_partition(next);
  }
  // Once a new boot slot has been selected, refuse another upload until
  // the scheduled reboot. A second upload could otherwise clobber the slot.
  if(e!=ESP_OK){
    ota_busy=false;
    ESP_LOGE(TAG,"OTA failure: %s",esp_err_to_name(e));
    return httpd_resp_send_err(req,HTTPD_500_INTERNAL_SERVER_ERROR,esp_err_to_name(e));
  }
  httpd_resp_sendstr(req,"Update geschreven. Opnieuw opstarten...");
  xTaskCreate([](void*){vTaskDelay(pdMS_TO_TICKS(1500));esp_restart();},"ota_restart",3072,nullptr,4,nullptr);
  return ESP_OK;
}
// Wi-Fi diagnostics omit credentials and keep event records bounded.
static void event_handler(void*,esp_event_base_t base,int32_t id,void* data){
  const uint32_t now=static_cast<uint32_t>(esp_timer_get_time()/1000);
  if(base==WIFI_EVENT && id==WIFI_EVENT_STA_START){
    ESP_LOGI(TAG,"Wi-Fi STA started; connecting (waiting for authentication and DHCP)");
    esp_wifi_connect();
  }else if(base==WIFI_EVENT && id==WIFI_EVENT_STA_CONNECTED){
    ESP_LOGI(TAG,"Wi-Fi associated with access point; waiting for DHCP IPv4");
  }else if(base==WIFI_EVENT && id==WIFI_EVENT_STA_DISCONNECTED){
    const auto* disconnected=static_cast<const wifi_event_sta_disconnected_t*>(data);
    const unsigned reason=disconnected?static_cast<unsigned>(disconnected->reason):0u;
    sta_connected=false;
    ESP_LOGW(TAG,"Wi-Fi disconnected: reason=%u; reconnecting",reason);
    if(log_provider)log_provider->push(rflink::LogStream::system,
      rflink::LogCode::wifi_disconnected,now,rflink::kAnyRadio,rflink::LogSeverity::warning,
      0,static_cast<int32_t>(reason));
    esp_wifi_connect();
  }else if(base==IP_EVENT && id==IP_EVENT_STA_GOT_IP){
    const auto* got=static_cast<const ip_event_got_ip_t*>(data);
    sta_connected=true;
    if(got){
      const auto& ip=got->ip_info;
      ESP_LOGI(TAG,"Wi-Fi READY: IP=" IPSTR " subnet=" IPSTR " gateway=" IPSTR
        " | Web http://" IPSTR "/ | API http://" IPSTR "/api/status",
        IP2STR(&ip.ip),IP2STR(&ip.netmask),IP2STR(&ip.gw),IP2STR(&ip.ip),IP2STR(&ip.ip));
      if(log_provider)log_provider->push(rflink::LogStream::system,
        rflink::LogCode::wifi_connected,now,rflink::kAnyRadio,rflink::LogSeverity::info,
        0,static_cast<int32_t>(ip.ip.addr));
    }else{
      ESP_LOGI(TAG,"Wi-Fi connected and IPv4 assigned; use /api/status to check address");
    }
  }
}
static esp_err_t native_index_get(httpd_req_t* req){
  // A fresh board must still expose the original Wi-Fi + OTA-key provisioning
  // form until the device has credentials. No false empty setup page.
  if(provisioning)return index_get(req);
  return reply(req,"text/html; charset=utf-8",
               rflink::kNativeUiHtml);
}
static bool start_http(){
  httpd_config_t cfg=HTTPD_DEFAULT_CONFIG();cfg.stack_size=8192;cfg.max_uri_handlers=17;cfg.recv_wait_timeout=10;
  esp_err_t result=httpd_start(&server,&cfg);
  if(result!=ESP_OK){ESP_LOGE(TAG,"HTTP server failed: %s",esp_err_to_name(result));return false;}
  httpd_uri_t root={};root.uri="/";root.method=HTTP_GET;root.handler=native_index_get;
  httpd_uri_t advanced{};advanced.uri="/advanced";advanced.method=HTTP_GET;advanced.handler=index_get;
  httpd_uri_t styles{};styles.uri="/app.css";styles.method=HTTP_GET;styles.handler=native_styles_get;
  httpd_uri_t script{};script.uri="/app.js";script.method=HTTP_GET;script.handler=native_script_get;
  httpd_uri_t provision={};provision.uri="/setup";provision.method=HTTP_POST;provision.handler=setup_post;
  httpd_uri_t ota={};ota.uri="/api/ota";ota.method=HTTP_POST;ota.handler=ota_post;
  httpd_uri_t status{};status.uri="/api/status";status.method=HTTP_GET;status.handler=status_get;
  httpd_uri_t crashes{};crashes.uri="/api/diagnostics/crashes";crashes.method=HTTP_GET;crashes.handler=crashes_get;
  httpd_uri_t console{};console.uri="/api/diagnostics/console";console.method=HTTP_GET;console.handler=console_get;
  httpd_uri_t logs{};logs.uri="/api/logs";logs.method=HTTP_GET;logs.handler=logs_get;
  httpd_uri_t logs_csv{};logs_csv.uri="/api/logs.csv";logs_csv.method=HTTP_GET;logs_csv.handler=logs_csv_get;
  httpd_uri_t log_conf_get{};log_conf_get.uri="/api/logs/config";log_conf_get.method=HTTP_GET;log_conf_get.handler=log_config_get;
  httpd_uri_t log_conf_post{};log_conf_post.uri="/api/logs/config";log_conf_post.method=HTTP_POST;log_conf_post.handler=log_config_post;
  httpd_uri_t events{};events.uri="/api/events";events.method=HTTP_GET;events.handler=events_get;
  httpd_uri_t events_csv{};events_csv.uri="/api/events.csv";events_csv.method=HTTP_GET;events_csv.handler=events_csv_get;
  httpd_uri_t mqtt_get{};mqtt_get.uri="/api/mqtt/status";mqtt_get.method=HTTP_GET;mqtt_get.handler=mqtt_status_get;
  httpd_uri_t mqtt_post{};mqtt_post.uri="/api/mqtt/config";mqtt_post.method=HTTP_POST;mqtt_post.handler=mqtt_config_post;
  if(httpd_register_uri_handler(server,&root)!=ESP_OK ||
     httpd_register_uri_handler(server,&advanced)!=ESP_OK ||
     httpd_register_uri_handler(server,&styles)!=ESP_OK ||
     httpd_register_uri_handler(server,&script)!=ESP_OK ||
     httpd_register_uri_handler(server,&provision)!=ESP_OK ||
     httpd_register_uri_handler(server,&ota)!=ESP_OK ||
     httpd_register_uri_handler(server,&status)!=ESP_OK ||
     httpd_register_uri_handler(server,&crashes)!=ESP_OK ||
     httpd_register_uri_handler(server,&console)!=ESP_OK ||
     httpd_register_uri_handler(server,&logs)!=ESP_OK ||
     httpd_register_uri_handler(server,&logs_csv)!=ESP_OK ||
     httpd_register_uri_handler(server,&log_conf_get)!=ESP_OK ||
     httpd_register_uri_handler(server,&log_conf_post)!=ESP_OK ||
     httpd_register_uri_handler(server,&events)!=ESP_OK ||
     httpd_register_uri_handler(server,&events_csv)!=ESP_OK ||
     httpd_register_uri_handler(server,&mqtt_get)!=ESP_OK ||
     httpd_register_uri_handler(server,&mqtt_post)!=ESP_OK){
    httpd_stop(server);server=nullptr;return false;
  }
  return true;
}
} // namespace
void rflink_install_console_capture(){
  if(!console_capture_installed){
    // Set once, in app_main before any project startup actions.
    console_boot_id=esp_random();
    previous_log_writer=esp_log_set_vprintf(console_vprintf);
    console_capture_installed=true;
  }
}
void rflink_set_manager_provider(rflink::RadioManager* manager) { manager_provider=manager; }
void rflink_publish_radio_providers(){
  radio_providers_published.store(true,std::memory_order_release);
}
void rflink_publish_startup_complete(){
  startup_complete_published.store(true,std::memory_order_release);
}
void rflink_set_radio_snapshot(bool spi_ok,const bool present[3]){
  spi_bus_ready=spi_ok;
  for(int i=0;i<3;i++)detected_radio[i]=present[i];
}
bool rflink_start_web_ota(){
  char ssid[33]={0},pass[64]={0};
  provisioning=!read_credentials(ssid,sizeof(ssid),pass,sizeof(pass));
  ESP_ERROR_CHECK(esp_netif_init());ESP_ERROR_CHECK(esp_event_loop_create_default());
  wifi_init_config_t init=WIFI_INIT_CONFIG_DEFAULT();
  if(provisioning){
    access_point=esp_netif_create_default_wifi_ap();
    ESP_ERROR_CHECK(esp_wifi_init(&init));
    wifi_config_t w={};strcpy((char*)w.ap.ssid,"RFLink-Setup");w.ap.ssid_len=12;
    // Generated per-boot password, visible on serial console; no open AP.
    auto ap_password=random_hex(8);strncpy((char*)w.ap.password,ap_password.c_str(),sizeof(w.ap.password)-1);
    w.ap.authmode=WIFI_AUTH_WPA2_PSK;w.ap.max_connection=1;
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP,&w));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGW(TAG,"SETUP AP SSID=RFLink-Setup password=%s; open http://192.168.4.1",ap_password.c_str());
  }else{
    station=esp_netif_create_default_wifi_sta();
    ESP_ERROR_CHECK(esp_wifi_init(&init));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT,ESP_EVENT_ANY_ID,&event_handler,nullptr));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT,IP_EVENT_STA_GOT_IP,&event_handler,nullptr));
    wifi_config_t w={};strncpy((char*)w.sta.ssid,ssid,sizeof(w.sta.ssid));strncpy((char*)w.sta.password,pass,sizeof(w.sta.password));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA,&w));
    ESP_ERROR_CHECK(esp_wifi_start());
  }
  const bool web_ready=start_http();
  ESP_LOGI(TAG,"RFLink HTTP API: %s (port 80, endpoint /api/status)",web_ready?"ready":"NOT AVAILABLE");
  return web_ready;
}

void rflink_set_rx_stats_provider(rflink::PacketRx868* receiver) { rx_stats_provider=receiver; }

void rflink_set_ook_stats_providers(rflink::OokPulseReceiver* a, rflink::OokPulseReceiver* b){ook_stats_a=a;ook_stats_b=b;}

void rflink_set_decoder_stats_provider(rflink::NativeDecoder* decoder){decoder_stats_provider=decoder;}

void rflink_set_rx_worker_cycles_provider(const std::atomic<uint32_t>* counter){worker_cycles_provider=counter;}

void rflink_set_spi_stats_provider(const rflink::Cc1101SpiBus* bus){spi_provider=bus;}

void rflink_set_log_store_provider(rflink::LogStore* logs) { log_provider=logs; }
