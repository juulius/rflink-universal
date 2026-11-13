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
#include "rflink_time.hpp"
#include "rflink_status_led.hpp"
#include "rflink_ota_slot_guard.hpp"
#include "rflink_log_store.hpp"
#include "rflink_log_details.hpp"
#include "rflink_native_ui_assets.hpp"
#include "cJSON.h"
#include <cstring>
#include <ctime>
#include <cstdio>
#include <cstdarg>
#include <cctype>
#include <cstdlib>
#include <string>
#include <array>
#include <algorithm>
#include "esp_log.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
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
static rflink::DetailStore* detail_provider=nullptr;
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
// access key. The captured ring omits lines that may contain secrets.
// Earlier ROM/panic output and raw core dumps remain USB-only.
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
// Numeric endpoint codes only: do NOT retain URI queries, headers, passwords,
// uploaded file names, private IP addresses or request bodies in log records.
static uint16_t web_endpoint_id(const char* path){
  if(!path)return 0;
  constexpr const char* routes[]={
    "/api/status", "/api/diagnostics/crashes", "/api/diagnostics/console",
    "/api/logs", "/api/logs.csv", "/api/logs/config", "/api/logs/ui",
    "/api/led", "/api/events", "/api/events.csv", "/api/mqtt/status",
    "/api/mqtt/config", "/api/ota", "/setup", "/api/radio/capture"
  };
  for(size_t i=0;i<sizeof(routes)/sizeof(routes[0]);++i){
    const size_t size=strlen(routes[i]);
    if(strncmp(path,routes[i],size)==0 &&
       (path[size]=='\0' || path[size]=='?'))return uint16_t(i+1);
  }
  return 0; // Unknown API route, never capture raw URL.
}
static void record_web_api(httpd_req_t* req,rflink::LogCode code,int status){
  if(!log_provider||!req)return;
  const auto endpoint=web_endpoint_id(req->uri);
  const auto seq=log_provider->push(rflink::LogStream::web,code,
    static_cast<uint32_t>(esp_timer_get_time()/1000),
    rflink::kAnyRadio,status>=400?rflink::LogSeverity::error:rflink::LogSeverity::info,
    endpoint,status,0,req->method==HTTP_POST?1:0);
  if(detail_provider && seq){
    // Deliberately do NOT log request URI, query string, body, header or key.
    char description[160];
    const int n=snprintf(description,sizeof(description),
      "HTTP %s; allowlisted endpoint ID=%u; status=%d; event code=%u. "
      "Request contents and credentials are not retained.",
      req->method==HTTP_POST?"POST":"GET",unsigned(endpoint),status,unsigned(code));
    if(n>0)detail_provider->save_text(5,seq,description,
      static_cast<size_t>(n)<sizeof(description)?
        static_cast<size_t>(n):sizeof(description)-1);
  }
}
// httpd_err_code_t is an enum used by ESP-IDF's send_err API, not an
// HTTP status integer. Writing its raw value made 404 look like "6 / info".
static int http_status_for_error(httpd_err_code_t code){
  switch(code){
    case HTTPD_400_BAD_REQUEST:return 400;
    case HTTPD_403_FORBIDDEN:return 403;
    case HTTPD_404_NOT_FOUND:return 404;
    case HTTPD_500_INTERNAL_SERVER_ERROR:return 500;
    default:return 500;
  }
}
static esp_err_t send_api_error(httpd_req_t* req,httpd_err_code_t code,const char* message){
  record_web_api(req,rflink::LogCode::web_api_failed,http_status_for_error(code));
  return httpd_resp_send_err(req,code,message);
}
static esp_err_t reply(httpd_req_t *req,const char* type,const char* body){
  httpd_resp_set_type(req,type);
  httpd_resp_set_hdr(req,"Cache-Control","no-store");
  httpd_resp_set_hdr(req,"X-Content-Type-Options","nosniff");
  const auto result=httpd_resp_sendstr(req,body);
  if(req && req->method==HTTP_POST && web_endpoint_id(req->uri)!=7){
    record_web_api(req,result==ESP_OK?rflink::LogCode::web_api_success:
      rflink::LogCode::web_api_failed,result==ESP_OK?200:500);
  }
  return result;
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
static rflink::OtaSlotDescriptor ota_slot_descriptor(const esp_partition_t* p) {
  if(!p)return {};
  return {true,static_cast<uint8_t>(p->subtype),
          static_cast<uint32_t>(p->address),static_cast<uint32_t>(p->size)};
}
struct NativeOtaLayout {
  rflink::OtaSlotPlan plan{};
  const esp_partition_t* running=nullptr;
  const esp_partition_t* target=nullptr;
  unsigned count=0;
};
static NativeOtaLayout inspect_ota_layout() {
  NativeOtaLayout layout{};
  layout.running=esp_ota_get_running_partition();
  const auto* zero=esp_partition_find_first(
    ESP_PARTITION_TYPE_APP,ESP_PARTITION_SUBTYPE_APP_OTA_0,nullptr);
  const auto* one=esp_partition_find_first(
    ESP_PARTITION_TYPE_APP,ESP_PARTITION_SUBTYPE_APP_OTA_1,nullptr);
  layout.count=unsigned(zero!=nullptr)+unsigned(one!=nullptr);
  layout.plan=rflink::choose_native_ota_slot(
    ota_slot_descriptor(layout.running),ota_slot_descriptor(zero),
    ota_slot_descriptor(one));
  if(layout.plan.safe) layout.target=layout.plan.target_index==0?zero:one;
  return layout;
}
// Share the same exact status snapshot between the API and CSV export.
static std::string status_snapshot_json(){
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
  // The regular /api/status route and full CSV export both run on httpd.
  // A 4 KiB local printf buffer consumed half of its former 8 KiB stack.
  // Keep a larger bounded formatter in PSRAM, never in HTTP task stack.
  constexpr size_t kStatusJsonCapacity=8192;
  char* body=static_cast<char*>(heap_caps_calloc(1,kStatusJsonCapacity,
                                     MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
  if(!body)body=static_cast<char*>(heap_caps_calloc(1,kStatusJsonCapacity,
                                     MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT));
  if(!body){
    ESP_LOGE(TAG,"Status snapshot allocation failed");
    return "{\"error\":\"status_snapshot_memory_unavailable\"}";
  }
  const auto httpd_stack_min_free=uxTaskGetStackHighWaterMark(nullptr);
  const auto led=rflink::status_led_snapshot();
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
  const auto ota_layout=inspect_ota_layout();
  const auto* running=ota_layout.running;
  esp_ota_img_states_t ota_state{};
  const bool ota_state_known=running &&
    esp_ota_get_state_partition(running,&ota_state)==ESP_OK;
  const int written=snprintf(body,kStatusJsonCapacity,
    "{\"firmware\":\"%s\",\"device_id\":\"%s\",\"ipv4\":\"%s\",\"firmware_display\":\"%s\",\"release_channel\":\"%s\",\"release_build\":%u,\"wifi_connected\":%s,\"mqtt_connected\":%s,\"spi_bus_ready\":%s,\"radio_initialized\":%s,\"startup_complete\":%s,"
    "\"uptime_s\":%llu,\"free_heap_bytes\":%lu,\"internal_total_bytes\":%lu,\"internal_free_bytes\":%lu,\"internal_min_free_bytes\":%lu,\"internal_largest_block_bytes\":%lu,\"psram_total_bytes\":%lu,\"psram_free_bytes\":%lu,\"psram_min_free_bytes\":%lu,\"psram_largest_block_bytes\":%lu,\"rx_worker_cycles\":%lu,\"ota_slot\":%lu,\"ota_state\":%d,\"led_gpio\":%d,\"led_enabled\":%s,\"led_brightness\":%u,\"led_driver_ready\":%s,\"led_mode\":\"%s\","
    "\"ota_slot_count\":%u,\"ota_update_safe\":%s,\"ota_target_slot\":%d,\"ota_running_address\":%lu,\"ota_target_address\":%lu,\"ota_layout_issue\":\"%s\","
    "\"cc1101_present\":[%s,%s,%s],\"cc1101_spi_healthy\":[%s,%s,%s],\"cc1101_partnum\":[%u,%u,%u],\"cc1101_version\":[%u,%u,%u],\"cc1101_boot_readback_verified\":[%s,%s,%s],\"spi_ready_timeouts\":[%lu,%lu,%lu],\"spi_transfer_errors\":[%lu,%lu,%lu],\"radio_operation\":[%u,%u,%u],\"tx_lab_stage\":%u,\"tx_lab_driver_present\":true,\"rx_implemented\":false,\"rx868_packet_diagnostic\":true,\"rx868_listening\":%s,\"rx868_packets\":%lu,\"rx868_fifo_overflows\":%lu,\"rx868_spi_errors\":%lu,\"rx868_recoveries\":%lu,\"rx868_idle_rearms\":%lu,\"rx868_last_marcstate\":%u,\"rx868_last_length\":%u,\"rx868_crc_enabled\":false,\"tx_implemented\":false,"
    "\"rx43342_listening\":%s,\"rx43342_frames\":%lu,\"rx43342_full_frames\":%lu,\"rx43342_filtered_frames\":%lu,\"rx43342_forwarded_frames\":%lu,\"rx43342_short_frames\":%lu,\"rx43342_queue_drops\":%lu,\"rx43342_last_full_symbols\":%lu,\"rx43342_capture_capacity\":%lu,\"rx43342_last_symbols\":%lu,\"rx43342_last_forwarded_pulses\":%lu,\"rx43342_last_outcome\":%u,\"rx43342_rmt_error_stage\":%u,\"rx43342_rmt_error_code\":%d,\"rx43342_rmt_errors\":%lu,\"rx43342_start_error_stage\":%u,\"rx43342_start_error_code\":%d,\"rx43342_start_error_reg\":%d,\"rx43342_start_expected\":%d,\"rx43342_start_observed\":%d,\"rx43342_start_failures\":%lu,\"rx43392_listening\":%s,\"rx43392_frames\":%lu,\"rx43392_full_frames\":%lu,\"rx43392_filtered_frames\":%lu,\"rx43392_forwarded_frames\":%lu,\"rx43392_short_frames\":%lu,\"rx43392_queue_drops\":%lu,\"rx43392_last_full_symbols\":%lu,\"rx43392_capture_capacity\":%lu,\"rx43392_last_symbols\":%lu,\"rx43392_last_forwarded_pulses\":%lu,\"rx43392_last_outcome\":%u,\"rx43392_rmt_error_stage\":%u,\"rx43392_rmt_error_code\":%d,\"rx43392_rmt_errors\":%lu,\"rx43392_start_error_stage\":%u,\"rx43392_start_error_code\":%d,\"rx43392_start_error_reg\":%d,\"rx43392_start_expected\":%d,\"rx43392_start_observed\":%d,\"rx43392_start_failures\":%lu,\"decode_itho\":%lu,\"decode_ramses\":%lu,\"decode_unknown\":%lu,\"decode_unknown_prefix\":%lu,\"decode_unknown_payload\":%lu,\"decode_overlap\":%lu,\"decode_pulses_43342\":%lu,\"decode_pulses_43392\":%lu,\"last_ramses_command\":%u,\"user_rx_matches\":%lu,\"user_rx_ambiguous\":%lu,\"user_decoders_enabled\":%u,\"user_library_storage_state\":%u,\"ota_busy\":%s,\"board_id\":\"%s\",\"board_hash\":\"%s\",\"spi_clock_hz\":%lu,\"spi_pins\":[%d,%d,%d],\"radio_pins\":[[%d,%d,%d],[%d,%d,%d],[%d,%d,%d]],\"httpd_stack_min_free_bytes\":%lu}",
    app->version,physical_id,ipv4,friendly_version.c_str(),firmware_channel(raw_version),firmware_build_number(raw_version),sta_connected?"true":"false",rflink::mqtt_is_connected()?"true":"false",(radio_ready&&spi_bus_ready)?"true":"false",radio_ready?"true":"false",startup_complete_published.load(std::memory_order_acquire)?"true":"false",
    static_cast<unsigned long long>(esp_timer_get_time()/1000000),
    static_cast<unsigned long>(esp_get_free_heap_size()),
    static_cast<unsigned long>(heap_caps_get_total_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)),
    static_cast<unsigned long>(heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)),
    static_cast<unsigned long>(heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)),
    static_cast<unsigned long>(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)),
    static_cast<unsigned long>(heap_caps_get_total_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)),
    static_cast<unsigned long>(heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)),
    static_cast<unsigned long>(heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)),
    static_cast<unsigned long>(heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)),
    static_cast<unsigned long>(radio_ready && worker_cycles_provider ? worker_cycles_provider->load(std::memory_order_relaxed) : 0),
    running ? static_cast<unsigned long>(running->subtype) : 0ul,
    ota_state_known ? static_cast<int>(ota_state) : -1,
    rflink::kStatusLedGpio,led.settings.enabled?"true":"false",
    unsigned(led.settings.brightness),led.driver_ready?"true":"false",
    rflink::led_mode_name(led.mode),
    ota_layout.count,ota_layout.plan.safe?"true":"false",
    ota_layout.target?int(ota_layout.target->subtype):-1,
    static_cast<unsigned long>(running?running->address:0),
    static_cast<unsigned long>(ota_layout.target?ota_layout.target->address:0),
    ota_layout.plan.reason,
    (radio_ready&&(manager_provider?m0.present:detected_radio[0]))?"true":"false",
    (radio_ready&&(manager_provider?m1.present:detected_radio[1]))?"true":"false",
    (radio_ready&&(manager_provider?m2.present:detected_radio[2]))?"true":"false",
    m0.spi_healthy?"true":"false",m1.spi_healthy?"true":"false",m2.spi_healthy?"true":"false",
    unsigned(s0.last_partnum),unsigned(s1.last_partnum),unsigned(s2.last_partnum),
    unsigned(s0.last_version),unsigned(s1.last_version),unsigned(s2.last_version),
    s0.boot_readback_verified?"true":"false",
    s1.boot_readback_verified?"true":"false",
    s2.boot_readback_verified?"true":"false",
    (unsigned long)s0.ready_timeouts,(unsigned long)s1.ready_timeouts,(unsigned long)s2.ready_timeouts,
    (unsigned long)s0.transfer_errors,(unsigned long)s1.transfer_errors,(unsigned long)s2.transfer_errors,
    unsigned(m0.operation),unsigned(m1.operation),unsigned(m2.operation),unsigned(tx_stage),
    rx.listening?"true":"false", (unsigned long)rx.packets, (unsigned long)rx.fifo_overflows, (unsigned long)rx.spi_errors,
    (unsigned long)rx.recoveries, (unsigned long)rx.idle_rearms,
    unsigned(rx.last_marcstate),unsigned(rx.last_length),
    a.listening?"true":"false",(unsigned long)a.captures,
    (unsigned long)a.full_frames,(unsigned long)a.filtered_frames,
    (unsigned long)a.forwarded_frames,(unsigned long)a.short_frames,
    (unsigned long)a.queue_drops,(unsigned long)a.last_full_symbols,
    (unsigned long)a.capture_capacity_symbols,(unsigned long)a.last_symbols,
    (unsigned long)a.last_forwarded_pulses,unsigned(a.last_capture_outcome),
    unsigned(a.last_driver_stage),int(a.last_driver_error),(unsigned long)a.driver_errors,
    unsigned(da.stage),int(da.esp_error),int(da.register_address),
    int(da.expected),int(da.observed),(unsigned long)da.failures,
    b.listening?"true":"false",(unsigned long)b.captures,
    (unsigned long)b.full_frames,(unsigned long)b.filtered_frames,
    (unsigned long)b.forwarded_frames,(unsigned long)b.short_frames,
    (unsigned long)b.queue_drops,(unsigned long)b.last_full_symbols,
    (unsigned long)b.capture_capacity_symbols,(unsigned long)b.last_symbols,
    (unsigned long)b.last_forwarded_pulses,unsigned(b.last_capture_outcome),
    unsigned(b.last_driver_stage),int(b.last_driver_error),(unsigned long)b.driver_errors,
    unsigned(db.stage),int(db.esp_error),int(db.register_address),
    int(db.expected),int(db.observed),(unsigned long)db.failures,
    (unsigned long)dc.itho_messages,(unsigned long)dc.ramses_messages,(unsigned long)dc.unknown_packet_inputs,
    (unsigned long)dc.unknown_prefix_inputs,(unsigned long)dc.unknown_decode_inputs,
    (unsigned long)dc.overlapping_decodes,
    (unsigned long)dc.pulse_inputs_43342,(unsigned long)dc.pulse_inputs_43392,
    unsigned(dc.last_ramses_command),
    (unsigned long)dc.user_rx_matches,(unsigned long)dc.user_rx_ambiguous,
    unsigned(dc.user_decoders_enabled),unsigned(dc.user_library_storage_state),
    ota_busy?"true":"false", rflink::kBoardId, rflink::kBoardHash,
    static_cast<unsigned long>(rflink::kSpiPins.clock_hz),
    rflink::kSpiPins.sclk,rflink::kSpiPins.mosi,rflink::kSpiPins.miso,
    rflink::kRadioPins[0].cs,rflink::kRadioPins[0].gdo0,rflink::kRadioPins[0].gdo2,
    rflink::kRadioPins[1].cs,rflink::kRadioPins[1].gdo0,rflink::kRadioPins[1].gdo2,
    rflink::kRadioPins[2].cs,rflink::kRadioPins[2].gdo0,rflink::kRadioPins[2].gdo2,
    static_cast<unsigned long>(httpd_stack_min_free));
  if(written<0 || static_cast<size_t>(written)>=kStatusJsonCapacity){
    heap_caps_free(body);
    ESP_LOGE(TAG,"Status snapshot JSON overflow/truncation (written=%d cap=%u)",
             written,unsigned(kStatusJsonCapacity));
    return "{\"error\":\"status_snapshot_format_failed\"}";
  }
  std::string snapshot(body,static_cast<size_t>(written));
  heap_caps_free(body);
  // The source is UTC SNTP, never the browser clock or a guessed uptime epoch.
  // An unsynchronized board explicitly reports null, not a plausible fake date.
  if(!snapshot.empty() && snapshot.back()=='}'){
    snapshot.pop_back();
    const auto now=rflink::time_now_epoch_ms();
    const auto started=rflink::time_boot_started_epoch_ms(rflink::time_boot_id());
    snapshot+=",\"boot_id\":"+std::to_string(rflink::time_boot_id());
    snapshot+=",\"clock_synced\":";
    snapshot+=rflink::time_synchronized()?"true":"false";
    snapshot+=",\"clock_source\":\"sntp_utc\"";
    snapshot+=",\"now_utc_ms\":"+(now?std::to_string(now):"null");
    snapshot+=",\"boot_started_utc_ms\":"+(started?std::to_string(started):"null");
    // RMT buffers are NOT protocol frames. Keep the original fields for
    // Builder compatibility and add explicit, unambiguous stage counters.
    // No change to the ISR, radio register profile or FIFO RX.
    const auto append_rmt_stages=[&snapshot](const char* channel,
                                            const rflink::PulseStats& stats){
      const std::string prefix=std::string(",\"rx")+channel;
      const uint32_t short_only=stats.short_frames>=stats.full_frames
        ?stats.short_frames-stats.full_frames:0;
      snapshot+=prefix+"_rmt_non_saturated\":"+std::to_string(
        stats.captures>=stats.full_frames?stats.captures-stats.full_frames:0);
      snapshot+=prefix+"_rmt_saturated\":"+std::to_string(stats.full_frames);
      snapshot+=prefix+"_raw_short_only\":"+std::to_string(short_only);
      snapshot+=prefix+"_last_raw_pulses\":"+std::to_string(stats.last_raw_pulses);
      snapshot+=prefix+"_last_filtered_pulses\":"+std::to_string(stats.last_filtered_pulses);
    };
    append_rmt_stages("43342",a);
    append_rmt_stages("43392",b);
    snapshot+='}';
  }
  return snapshot;
}
static esp_err_t status_get(httpd_req_t* req){
  const auto body=status_snapshot_json();
  return reply(req,"application/json",body.c_str());
}
static const char* event_kind_name(rflink::EventKind kind){
  switch(kind){
    case rflink::EventKind::raw_pulses:return "raw_pulses";
    case rflink::EventKind::unknown_packet:return "unknown_packet";
    case rflink::EventKind::itho:return "itho";
    case rflink::EventKind::ramses:return "ramses";
    case rflink::EventKind::user_match:return "user_rx_match";
    case rflink::EventKind::user_ambiguous:return "user_rx_ambiguous";
  }
  return "unknown";
}
static esp_err_t console_get(httpd_req_t* req) {
  // Read-only sanitized application log, available without the OTA/write key.
  // Raw USB/ROM text and core dump bytes are never exposed by this route.
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
  if(!valid)return send_api_error(req,HTTPD_500_INTERNAL_SERVER_ERROR,"Console rolling buffer changed; retry");
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
    case rflink::LogCode::user_rx_match: return "User decoder RX match";
    case rflink::LogCode::user_rx_ambiguous: return "Ambiguous user RX match";
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
    case rflink::LogCode::rx_capture_summary: return "433 RX capture summary";
    case rflink::LogCode::radio_probe: return "CC1101 identity probe (count=part/version)";
    case rflink::LogCode::radio_health_lost: return "CC1101 stopped responding";
    case rflink::LogCode::web_page_open: return "Web page opened";
    case rflink::LogCode::web_ui_action: return "Web user action";
    case rflink::LogCode::web_api_failed: return "API request failed";
    case rflink::LogCode::web_api_success: return "API write completed";
    case rflink::LogCode::web_client_error: return "Browser API error";
    case rflink::LogCode::rx_started: return "Receiver listening";
    case rflink::LogCode::rx_stopped: return "Receiver stopped";
    case rflink::LogCode::rx_driver_error: return "Receiver driver error";
    case rflink::LogCode::rmt_raw_capture: return "433 raw RMT capture (pre-filter)";
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
  if(httpd_query_key_value(query,"view",value,sizeof(value))==ESP_OK)
    q.radio_activity=strcmp(value,"radio")==0;
  return q;
}
static bool log_u64_param(httpd_req_t* req,const char* key,uint64_t* out){
  if(!out)return false;
  char query[200]{},raw[24]{};
  if(httpd_req_get_url_query_str(req,query,sizeof(query))!=ESP_OK ||
     httpd_query_key_value(query,key,raw,sizeof(raw))!=ESP_OK)return false;
  if(!raw[0])return false;
  uint64_t v=0;
  for(size_t i=0;raw[i];++i) {
    if(raw[i]<'0'||raw[i]>'9')return false;
    const uint64_t digit=uint64_t(raw[i]-'0');
    if(v>(UINT64_MAX-digit)/10)return false;
    v=v*10+digit;
  }
  *out=v;return true;
}
// Read-only and volatile. The compact Radio log links to full raw samples
// using the decoder event sequence, not the global LogStore sequence.
// Data can contain RF IDs/rolling counters; use only on a trusted LAN and
// never persist samples to flash or include them in the generic CSV export.
static esp_err_t raw_capture_get(httpd_req_t* req) {
  uint64_t radio=UINT64_MAX,sequence=0;
  if(!log_u64_param(req,"radio",&radio) ||
     !log_u64_param(req,"seq",&sequence) ||
     radio>=3 || sequence==0 || sequence>UINT32_MAX)
    return send_api_error(req,HTTPD_400_BAD_REQUEST,"Expected radio 0..2 and valid RX event seq");
  uint64_t pre_filter=0;
  const bool requested_pre_filter=log_u64_param(req,"pre",&pre_filter) && pre_filter==1;
  if(detail_provider && detail_provider->ready()){
    // Extended RF history replaces the old eight-per-radio snapshot ring.
    // Exactly one private arena owns this event; never scan across radios.
    auto* detail=static_cast<rflink::DetailSnapshot*>(
      heap_caps_calloc(1,sizeof(rflink::DetailSnapshot),
                       MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
    if(!detail)return send_api_error(req,HTTPD_500_INTERNAL_SERVER_ERROR,
                                     "No PSRAM for detail snapshot");
    const uint64_t key=(uint64_t(1)<<63)|
                       (requested_pre_filter?(uint64_t(1)<<62):0)|sequence;
    const bool found=requested_pre_filter
      ? detail_provider->copy(static_cast<uint8_t>(radio),
                              static_cast<uint8_t>(radio),key,detail)
      : (detail_provider->copy(3,static_cast<uint8_t>(radio),key,detail) ||
         detail_provider->copy(static_cast<uint8_t>(radio),
                               static_cast<uint8_t>(radio),key,detail));
    if(!found){
      heap_caps_free(detail);
      return reply(req,"application/json",
                   "{\"available\":false,\"reason\":\"detail_overwritten_or_unavailable\"}");
    }
    rflink::RfDetailMeta meta{};
    if(detail->size<sizeof(meta) ||
       (detail->format!=rflink::DetailFormat::pulses &&
        detail->format!=rflink::DetailFormat::packet)){
      heap_caps_free(detail);
      return send_api_error(req,HTTPD_500_INTERNAL_SERVER_ERROR,"Invalid RF detail");
    }
    std::memcpy(&meta,detail->bytes.data(),sizeof(meta));
    const size_t payload=detail->size-sizeof(meta);
    if(payload!=size_t(meta.count)*(detail->format==rflink::DetailFormat::pulses?4:1)){
      heap_caps_free(detail);
      return send_api_error(req,HTTPD_500_INTERNAL_SERVER_ERROR,"RF detail size mismatch");
    }
    std::string result;
    result.reserve(detail->format==rflink::DetailFormat::pulses?13000:512);
    char head[256];
    snprintf(head,sizeof(head),
      "{\"available\":true,\"radio\":%u,\"seq\":%lu,\"uptime_ms\":%lu,"
      "\"frequency_hz\":%lu,\"count\":%u,\"hash\":%lu,\"format\":\"%s\",",
      unsigned(radio),static_cast<unsigned long>(sequence),
      static_cast<unsigned long>(meta.uptime_ms),
      static_cast<unsigned long>(meta.frequency_hz),unsigned(meta.count),
      static_cast<unsigned long>(meta.hash),
      detail->format==rflink::DetailFormat::pulses?"signed_pulses_us":"fifo_bytes_hex");
    result=head;
    if(detail->format==rflink::DetailFormat::pulses){
      result+="\"pulses_us\":[";
      for(size_t i=0;i<meta.count;++i){
        int32_t value=0;
        std::memcpy(&value,detail->bytes.data()+sizeof(meta)+i*4,4);
        if(i)result+=",";
        result+=std::to_string(value);
      }
      result+="]}";
    }else{
      static constexpr char hex[]="0123456789ABCDEF";
      result+="\"hex\":\"";
      for(size_t i=0;i<meta.count;++i){
        if(i)result+=' ';
        const uint8_t b=detail->bytes[sizeof(meta)+i];
        result+=hex[b>>4];result+=hex[b&15];
      }
      result+="\"";
      if(meta.has_quality){
        result+=",\"rssi_dbm\":"+std::to_string(meta.rssi_dbm_x2/2.0);
        result+=",\"lqi\":"+std::to_string(unsigned(meta.lqi));
        result+=",\"frequency_offset_hz\":"+
                  std::to_string(meta.frequency_offset_hz);
      }
      result+="}";
    }
    heap_caps_free(detail);
    return reply(req,"application/json",result.c_str());
  }
  if(requested_pre_filter)
    return reply(req,"application/json",
                 "{\"available\":false,\"reason\":\"pre_filter_capture_unavailable\"}");
  if(!radio_providers_published.load(std::memory_order_acquire) || !decoder_stats_provider ||
     !decoder_stats_provider->raw_capture_enabled())
    return reply(req,"application/json","{\"available\":false,\"reason\":\"capture_unavailable\"}");
  // Do not create ~4KB of captured waveform data on the 8KB HTTP task stack.
  auto* frame=static_cast<rflink::RawCaptureEntry*>(
    heap_caps_calloc(1,sizeof(rflink::RawCaptureEntry),MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
  if(!frame)return send_api_error(req,HTTPD_500_INTERNAL_SERVER_ERROR,"No memory for capture snapshot");
  const bool found=decoder_stats_provider->raw_capture(
    static_cast<uint8_t>(radio),static_cast<uint32_t>(sequence),frame);
  if(!found){
    heap_caps_free(frame);
    return reply(req,"application/json","{\"available\":false,\"reason\":\"overwritten_or_not_retained\"}");
  }
  std::string result;
  result.reserve(frame->kind==rflink::RawCaptureKind::pulses?8192:512);
  char meta[256];
  snprintf(meta,sizeof(meta),
    "{\"available\":true,\"radio\":%u,\"seq\":%lu,\"uptime_ms\":%lu,"
    "\"frequency_hz\":%lu,\"count\":%u,\"hash\":%lu,\"format\":\"%s\",",
    unsigned(frame->radio),static_cast<unsigned long>(frame->sequence),
    static_cast<unsigned long>(frame->uptime_ms),
    static_cast<unsigned long>(frame->frequency_hz),
    unsigned(frame->count),static_cast<unsigned long>(frame->hash),
    frame->kind==rflink::RawCaptureKind::pulses?"signed_pulses_us":"fifo_bytes_hex");
  result=meta;
  if(frame->kind==rflink::RawCaptureKind::pulses) {
    result+="\"pulses_us\":[";
    for(size_t i=0;i<frame->count;++i){
      if(i)result+=",";
      result+=std::to_string(frame->pulses_us[i]);
    }
    result+="]}";
  } else {
    static constexpr char hex[]="0123456789ABCDEF";
    result+="\"hex\":\"";
    for(size_t i=0;i<frame->count;++i){
      if(i)result+=' ';
      result+=hex[frame->packet[i]>>4];
      result+=hex[frame->packet[i]&0x0f];
    }
    result+="\"";
    // Only report registers actually read. LQI is an estimate, NOT CRC OK.
    if(frame->quality.has_quality){
      result+=",\"rssi_dbm\":"+std::to_string(frame->quality.rssi_dbm_x2/2.0);
      result+=",\"lqi\":"+std::to_string(unsigned(frame->quality.lqi));
      result+=",\"frequency_offset_hz\":"+
        std::to_string(frame->quality.frequency_offset_hz);
    }
    result+="}";
  }
  heap_caps_free(frame);
  return reply(req,"application/json",result.c_str());
}
// Extra diagnostics are available only by explicit expansion. Never add
// private RF contents, passwords or request bodies to the standard log CSV.
static esp_err_t log_detail_get(httpd_req_t* req){
  uint64_t bucket=UINT64_MAX,radio=UINT64_MAX,sequence=0;
  if(!log_u64_param(req,"bucket",&bucket) ||
     !log_u64_param(req,"radio",&radio) ||
     !log_u64_param(req,"seq",&sequence) ||
     bucket>=6 || sequence==0 ||
     (bucket<4 ? radio>=3 : radio!=255))
    return send_api_error(req,HTTPD_400_BAD_REQUEST,"Invalid detail address");
  if(!detail_provider || !detail_provider->ready())
    return reply(req,"text/plain; charset=utf-8","Detailed log storage unavailable.");
  auto* snapshot=static_cast<rflink::DetailSnapshot*>(
    heap_caps_calloc(1,sizeof(rflink::DetailSnapshot),
                     MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
  if(!snapshot)return send_api_error(req,HTTPD_500_INTERNAL_SERVER_ERROR,
                                      "No PSRAM for detail snapshot");
  const bool found=detail_provider->copy(static_cast<uint8_t>(bucket),
                                         static_cast<uint8_t>(radio),
                                         sequence,snapshot);
  if(!found || snapshot->format!=rflink::DetailFormat::text){
    heap_caps_free(snapshot);
    return reply(req,"text/plain; charset=utf-8",
                 "No additional details retained for this event (or overwritten).");
  }
  std::string result(reinterpret_cast<const char*>(snapshot->bytes.data()),
                     snapshot->size);
  heap_caps_free(snapshot);
  return reply(req,"text/plain; charset=utf-8",result.c_str());
}
static esp_err_t logs_get(httpd_req_t* req){
  constexpr size_t kPageSize=64;
  // One extra row determines whether another older page exists.
  rflink::LogRecord records[kPageSize+1]{};
  const auto snapshot_now=log_provider?log_provider->latest_sequence():0;
  uint64_t before=0,snapshot=snapshot_now;
  (void)log_u64_param(req,"before",&before);
  uint64_t requested=0;
  if(log_u64_param(req,"snapshot",&requested) && requested)
    snapshot=std::min(requested,snapshot_now);
  const size_t found=log_provider?
    log_provider->page_before(records,kPageSize+1,before,snapshot,log_query(req)):0;
  const bool has_more=found>kPageSize;
  const size_t first=has_more?1:0;
  const size_t count=found-first;
  std::string json="{\"events\":[";
  json.reserve(18000);
  char row[680];
  for(size_t i=first;i<found;++i){
    const auto& e=records[i];
    const char* stream=e.stream==0?"signal":e.stream==1?"noise":e.stream==2?"system":"web";
    const auto utc=rflink::time_event_epoch_ms(e.uptime_ms);
    const std::string stamp=utc?std::to_string(utc):"null";
    snprintf(row,sizeof(row),
      "%s{\"sequence\":%llu,\"uptime_ms\":%lu,\"stream\":\"%s\",\"radio\":%u,"
      "\"kind\":\"%s\",\"code\":%u,\"severity\":%u,\"count\":%u,"
      "\"value\":%ld,\"correlation_id\":%lu,\"flags\":%u,"
      "\"profile\":%u,\"tx_transport\":%u,\"tx_stage\":%u,\"tx_repeats\":%u,"
      "\"tx_rx_previous\":%s,\"tx_rx_restored\":%s,"
      "\"tx_driver_attempted\":%s,\"tx_driver_accepted\":%s,"
      "\"tx_hardware_restored\":%s,\"timestamp_utc_ms\":%s,\"boot_id\":%lu}",
      i>first?",":"",static_cast<unsigned long long>(e.sequence),
      static_cast<unsigned long>(e.uptime_ms),stream,unsigned(e.radio),
      log_code_name(e.code),unsigned(e.code),unsigned(e.severity),
      unsigned(e.count),static_cast<long>(e.value),
      static_cast<unsigned long>(e.correlation_id),unsigned(e.flags),
      unsigned(e.profile),unsigned(e.transport),unsigned(e.tx_stage),
      unsigned(e.repeats),(e.flags&1)?"true":"false",(e.flags&2)?"true":"false",
      (e.flags&4)?"true":"false",(e.flags&8)?"true":"false",
      (e.flags&16)?"true":"false",stamp.c_str(),
      static_cast<unsigned long>(rflink::time_boot_id()));
    json+=row;
  }
  char footer[160]{};
  snprintf(footer,sizeof(footer),
    "],\"snapshot\":%llu,\"cursor\":%llu,\"has_more\":%s,\"page_size\":%u}",
    static_cast<unsigned long long>(snapshot),
    static_cast<unsigned long long>(count?records[first].sequence:0),
    has_more?"true":"false",unsigned(kPageSize));
  json+=footer;
  return reply(req,"application/json",json.c_str());
}
// Complete RFC4180 export from the six existing LogStore rings, with their
// already-retained DetailStore samples and live diagnostic snapshots.
// RF ISR and decoder threads are not changed; every HTTP write is lock-free.
static std::string log_config_snapshot_json();
using CsvDiagnosticRow=std::array<std::string,26>;
// Column order 0-23 remains backward-compatible with existing CSV consumers.
static std::string iso_utc_ms(uint64_t epoch){
  if(!epoch)return "";
  const time_t seconds=static_cast<time_t>(epoch/1000ULL);
  struct tm utc{};
  if(!gmtime_r(&seconds,&utc))return "";
  char text[32]{};
  if(!std::strftime(text,sizeof(text),"%Y-%m-%dT%H:%M:%S",&utc))return "";
  char fraction[16]{};
  snprintf(fraction,sizeof(fraction),".%03uZ",
    static_cast<unsigned>(epoch%1000ULL));
  return std::string(text)+fraction;
}
static constexpr char kDiagnosticCsvHeader[]=
  "record_type,sequence,uptime_ms,stream,radio,kind,code,severity,count,"
  "value,correlation_id,flags,profile,tx_transport,tx_stage,"
  "driver_attempted,driver_accepted,hardware_restored,rx_restored,repeats,"
  "detail_state,detail_type,detail_json,snapshot_json,event_utc,boot_id\n";
static std::string csv_escape_field(const std::string& value){
  std::string escaped;
  escaped.reserve(value.size()+2);
  escaped.push_back('"');
  // CSV must not execute formulas if opened in a spreadsheet.
  if(!value.empty() && (value[0]=='=' || value[0]=='+' ||
      value[0]=='@' || (value[0]=='-' && (value.size()<2 ||
      !std::isdigit(static_cast<unsigned char>(value[1]))))))
    escaped.push_back('\'');
  for(char c:value){
    if(c=='"')escaped.push_back('"');
    escaped.push_back(c);
  }
  escaped.push_back('"');
  return escaped;
}
static esp_err_t emit_diagnostic_csv_row(httpd_req_t* req,
                                         const CsvDiagnosticRow& fields){
  std::string line;
  size_t estimate=0;
  for(const auto& f:fields)estimate+=f.size()+3;
  line.reserve(estimate+1);
  for(size_t i=0;i<fields.size();++i){
    if(i)line.push_back(',');
    line+=csv_escape_field(fields[i]);
  }
  line.push_back('\n');
  return httpd_resp_send_chunk(req,line.data(),line.size());
}
static bool export_copy_detail(const rflink::LogRecord& e,
                               rflink::DetailSnapshot* snapshot,
                               std::string* state,std::string* format,
                               std::string* output){
  if(!snapshot || !state || !format || !output)return false;
  const bool rx=(e.code>=1 && e.code<=6) || e.code==46;
  if(!detail_provider || !detail_provider->ready()){
    *state="detail_store_unavailable";return false;
  }
  const uint8_t radio=e.radio;
  bool found=false;
  if(rx){
    if(radio>=3){*state="invalid_radio";return false;}
    const bool pre=e.code==46;
    const uint64_t index=pre?e.sequence:uint64_t(e.correlation_id);
    if(!index){*state="no_capture_link";return false;}
    const uint64_t key=(uint64_t(1)<<63)|
      (pre?(uint64_t(1)<<62):0)|index;
    found=pre
      ? detail_provider->copy(radio,radio,key,snapshot)
      : (detail_provider->copy(3,radio,key,snapshot) ||
         detail_provider->copy(radio,radio,key,snapshot));
  }else{
    const uint8_t bucket=e.stream==0?radio:e.stream==1?3:e.stream==2?4:5;
    if(bucket>=6 || (bucket<4 && radio>=3)){
      *state="no_text_detail_recorded";return false;
    }
    found=detail_provider->copy(bucket,bucket<4?radio:255,e.sequence,snapshot);
  }
  if(!found){
    *state=rx?"not_retained_or_overwritten":"no_detail_or_overwritten";return false;
  }
  if(snapshot->format==rflink::DetailFormat::text){
    *format="text";*state="retained";
    std::string text(reinterpret_cast<const char*>(snapshot->bytes.data()),
                     snapshot->size);
    // Avoid depending on extra cJSON helpers in the native host stubs.
    // Escape arbitrary retained diagnostic bytes as an exact JSON string.
    *output="\"";
    static constexpr char hex[]="0123456789ABCDEF";
    for(unsigned char c:text){
      if(c=='"' || c=='\\'){output->push_back('\\');output->push_back(char(c));}
      else if(c=='\n')*output+="\\n";
      else if(c=='\r')*output+="\\r";
      else if(c=='\t')*output+="\\t";
      else if(c<0x20){
        *output+="\\u00";
        output->push_back(hex[c>>4]);output->push_back(hex[c&15]);
      }else output->push_back(char(c));
    }
    output->push_back('"');
    return true;
  }
  if(snapshot->size<sizeof(rflink::RfDetailMeta) ||
     (snapshot->format!=rflink::DetailFormat::pulses &&
      snapshot->format!=rflink::DetailFormat::packet)){
    *state="invalid_detail_format";return false;
  }
  rflink::RfDetailMeta meta{};
  std::memcpy(&meta,snapshot->bytes.data(),sizeof(meta));
  const bool pulses=snapshot->format==rflink::DetailFormat::pulses;
  const size_t expected=sizeof(meta)+size_t(meta.count)*(pulses?4:1);
  if(expected!=snapshot->size || (pulses && meta.count>1024) ||
     (!pulses && meta.count>63)){
    *state="invalid_detail_length";return false;
  }
  *format=pulses?"signed_pulses_us":"fifo_bytes_hex";
  *state="retained";
  std::string json;
  json.reserve(pulses?12000:800);
  json+=std::string("{\"capture_phase\":\"")+(e.code==46?"pre_filter_rmt":"decoder_input")+
        "\",\"frequency_hz\":"+std::to_string(meta.frequency_hz)+
        ",\"uptime_ms\":"+std::to_string(meta.uptime_ms)+
        ",\"timestamp_utc_ms\":"+([&](){ const auto t=rflink::time_event_epoch_ms(meta.uptime_ms);return t?std::to_string(t):std::string("null"); }())+
        ",\"hash\":"+std::to_string(meta.hash)+
        ",\"count\":"+std::to_string(meta.count)+
        ",\"format\":\""+*format+"\"";
  if(pulses){
    json+=",\"pulses_us\":[";
    for(size_t i=0;i<meta.count;++i){
      int32_t duration=0;
      std::memcpy(&duration,snapshot->bytes.data()+sizeof(meta)+i*4,4);
      if(i)json+=",";
      json+=std::to_string(duration);
    }
    json+="]";
  }else{
    static constexpr char hex[]="0123456789ABCDEF";
    json+=",\"hex\":\"";
    for(size_t i=0;i<meta.count;++i){
      if(i)json+=' ';
      const uint8_t value=snapshot->bytes[sizeof(meta)+i];
      json+=hex[value>>4];json+=hex[value&15];
    }
    json+="\"";
    if(meta.has_quality){
      json+=",\"rssi_dbm\":"+std::to_string(meta.rssi_dbm_x2/2.0);
      json+=",\"lqi\":"+std::to_string(unsigned(meta.lqi));
      json+=",\"frequency_offset_hz\":"+std::to_string(meta.frequency_offset_hz);
    }else json+=",\"quality_state\":\"not_measured\"";
  }
  json+="}";
  *output=std::move(json);
  return true;
}
static esp_err_t emit_diagnostic_csv_context(httpd_req_t* req,
                                              const char* name,
                                              const std::string& json){
  CsvDiagnosticRow fields{};
  fields[0]="context";fields[5]=name;fields[23]=json;
  return emit_diagnostic_csv_row(req,fields);
}
static esp_err_t logs_csv_get(httpd_req_t* req){
  // An event-count estimate is available before streaming, unlike the final
  // byte length. Retention may change during export, so it is not exact.
  const auto usage=log_provider?log_provider->stats():rflink::LogStats{};
  uint32_t estimated_events=0;
  for(const auto used:usage.used)estimated_events+=used;
  char estimated_text[16]{};
  snprintf(estimated_text,sizeof(estimated_text),"%lu",
           static_cast<unsigned long>(estimated_events));
  httpd_resp_set_hdr(req,"X-RFLink-Export-Estimated-Events",estimated_text);
  httpd_resp_set_type(req,"text/csv; charset=utf-8");
  httpd_resp_set_hdr(req,"Cache-Control","no-store");
  httpd_resp_set_hdr(req,"X-Content-Type-Options","nosniff");
  httpd_resp_set_hdr(req,"Content-Disposition",
                     "attachment; filename=RFLink_Diagnostics_Full.csv");
  if(httpd_resp_send_chunk(req,kDiagnosticCsvHeader,
                         sizeof(kDiagnosticCsvHeader)-1)!=ESP_OK)return ESP_FAIL;
  const uint64_t snapshot_end=log_provider?log_provider->latest_sequence():0;
  if(emit_diagnostic_csv_context(req,"export_schema",
      "{\"name\":\"rflink-diagnostics-csv-v2\","
      "\"scope\":\"all_six_buffers_unfiltered\","
      "\"retention\":\"volatile_snapshot_non_atomic\","
      "\"rf_data\":\"retained_detailed_samples_only\","
      "\"secrets\":\"excluded\"}")!=ESP_OK)return ESP_FAIL;
  if(emit_diagnostic_csv_context(req,"device_status",
                                 status_snapshot_json())!=ESP_OK)return ESP_FAIL;
  if(emit_diagnostic_csv_context(req,"log_buffer_usage",
                                 log_config_snapshot_json())!=ESP_OK)return ESP_FAIL;
  if(emit_diagnostic_csv_context(req,"sanitized_crash_history",
                                 rflink::crash_report_json())!=ESP_OK)return ESP_FAIL;
  const auto boot_started=rflink::time_boot_started_epoch_ms(rflink::time_boot_id());
  if(emit_diagnostic_csv_context(req,"time_anchor",
      std::string("{\"boot_id\":")+std::to_string(rflink::time_boot_id())+
      ",\"source\":\"sntp_utc\",\"synced\":"+
      (rflink::time_synchronized()?"true":"false")+
      ",\"boot_started_utc_ms\":"+
      (boot_started?std::to_string(boot_started):"null")+"}")!=ESP_OK)return ESP_FAIL;
  if(emit_diagnostic_csv_context(req,"export_cutoff",
      std::string("{\"last_sequence\":")+std::to_string(snapshot_end)+
      ",\"uptime_ms\":"+std::to_string(
        static_cast<unsigned long long>(esp_timer_get_time()/1000))+"}")!=ESP_OK)
    return ESP_FAIL;
  if(!log_provider)return httpd_resp_send_chunk(req,nullptr,0);
  auto* detail=static_cast<rflink::DetailSnapshot*>(
    heap_caps_calloc(1,sizeof(rflink::DetailSnapshot),
                     MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
  if(!detail && detail_provider && detail_provider->ready()){
    (void)emit_diagnostic_csv_context(req,"export_warning",
        "{\"error\":\"detail_snapshot_allocation_failed\","
        "\"rf_detail_export\":\"unavailable\"}");
  }
  rflink::LogQuery all{};
  all.include_noise=true; // Ignore tabs, radio, noise and current page.
  uint64_t after=0;
  rflink::LogRecord records[16]{};
  size_t exported=0;
  bool lost=false;
  while(after<snapshot_end){
    const size_t count=log_provider->page_after(records,16,after,snapshot_end,all);
    if(!count)break;
    for(size_t i=0;i<count;++i){
      const auto& e=records[i];
      if(e.sequence>after+1 && after>0)lost=true;
      CsvDiagnosticRow row{};
      row[0]="event";
      row[1]=std::to_string(e.sequence);
      row[2]=std::to_string(e.uptime_ms);
      row[3]=e.stream==0?"signal":e.stream==1?"noise":e.stream==2?"system":"web";
      row[4]=e.radio==rflink::kAnyRadio?"":std::to_string(e.radio);
      row[5]=log_code_name(e.code);
      row[6]=std::to_string(e.code);
      row[7]=std::to_string(e.severity);
      row[8]=std::to_string(e.count);
      row[9]=std::to_string(e.value);
      row[10]=std::to_string(e.correlation_id);
      row[11]=std::to_string(e.flags);
      row[12]=std::to_string(e.profile);
      row[13]=std::to_string(e.transport);
      row[14]=std::to_string(e.tx_stage);
      row[15]=(e.flags&4)?"true":"false";
      row[16]=(e.flags&8)?"true":"false";
      row[17]=(e.flags&16)?"true":"false";
      row[18]=(e.flags&2)?"true":"false";
      row[19]=std::to_string(e.repeats);
      row[24]=iso_utc_ms(rflink::time_event_epoch_ms(e.uptime_ms));
      row[25]=std::to_string(rflink::time_boot_id());
      if(detail){
        row[20]="not_recorded";
        (void)export_copy_detail(e,detail,&row[20],&row[21],&row[22]);
      }else row[20]="detail_snapshot_unavailable";
      if(emit_diagnostic_csv_row(req,row)!=ESP_OK){
        if(detail)heap_caps_free(detail);
        return ESP_FAIL;
      }
      ++exported;
    }
    after=records[count-1].sequence;
  }
  if(detail)heap_caps_free(detail);
  if(emit_diagnostic_csv_context(req,"export_summary",
      std::string("{\"exported_events\":")+std::to_string(exported)+
      ",\"snapshot_end\":"+std::to_string(snapshot_end)+
      ",\"sequence_gaps_detected\":"+(lost?"true":"false")+
      ",\"note\":\"ring_evictions_can_occur_during_stream\"}")!=ESP_OK)
    return ESP_FAIL;
  return httpd_resp_send_chunk(req,nullptr,0);
}
// The native ESP-IDF HTTP server dispatches normal routes on one task.
// Streaming a full CSV there can block status, logs and the UI for seconds.
// Run one bounded asynchronous request on a separate worker instead.
static std::atomic<bool> csv_export_busy{false};
static void csv_export_task(void* arg){
  auto* req=static_cast<httpd_req_t*>(arg);
  const esp_err_t result=logs_csv_get(req);
  if(result!=ESP_OK)
    ESP_LOGW(TAG,"CSV download disconnected or failed: %s",esp_err_to_name(result));
  // Mandatory on success and failure; frees the copied request and gives the
  // socket back to ESP-IDF. Otherwise new connections can become exhausted.
  const esp_err_t done=httpd_req_async_handler_complete(req);
  if(done!=ESP_OK)
    ESP_LOGW(TAG,"CSV async cleanup failed: %s",esp_err_to_name(done));
  csv_export_busy.store(false,std::memory_order_release);
  vTaskDelete(nullptr);
}
static esp_err_t logs_csv_async_get(httpd_req_t* req){
  bool expected=false;
  if(!csv_export_busy.compare_exchange_strong(expected,true,
       std::memory_order_acq_rel)){
    httpd_resp_set_status(req,"503 Service Unavailable");
    httpd_resp_set_hdr(req,"Retry-After","3");
    return reply(req,"application/json",
                 "{\"error\":\"Another diagnostics export is already running\"}");
  }
  httpd_req_t* copy=nullptr;
  const esp_err_t started=httpd_req_async_handler_begin(req,&copy);
  if(started!=ESP_OK){
    csv_export_busy.store(false,std::memory_order_release);
    return send_api_error(req,HTTPD_500_INTERNAL_SERVER_ERROR,
                          "Unable to prepare diagnostics export");
  }
  // The existing HTTP stack is 16 KiB. Keep the CSV worker separate and
  // bounded (one concurrent download), with a comparable stack budget.
  if(xTaskCreate(csv_export_task,"csv_export",16384,copy,3,nullptr)!=pdPASS){
    (void)httpd_req_async_handler_complete(copy);
    csv_export_busy.store(false,std::memory_order_release);
    return ESP_FAIL;
  }
  return ESP_OK;
}
static std::string log_config_snapshot_json(){
  const auto stats=log_provider?log_provider->stats():rflink::LogStats{};
  char body[1250];
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
  // Detail allocations are separate from the original six metadata rings.
  const auto details=detail_provider?detail_provider->stats():rflink::DetailStats{};
  const size_t used=std::strlen(body);
  if(used && body[used-1]=='}'){
    body[used-1]='\0';
    snprintf(body+used-1,sizeof(body)-(used-1),
      ",\"details\":{\"ready\":%s,\"object_bytes\":%lu,"
      "\"budget_bytes\":%lu,"
      "\"capacity\":[%lu,%lu,%lu,%lu,%lu,%lu],"
      "\"used\":[%lu,%lu,%lu,%lu,%lu,%lu],"
      "\"evicted\":[%llu,%llu,%llu,%llu,%llu,%llu],"
      "\"rejected\":[%llu,%llu,%llu,%llu,%llu,%llu]}}",
      details.ready?"true":"false",
      static_cast<unsigned long>(details.object_bytes),
      static_cast<unsigned long>(rflink::DetailStore::kMaxBudgetBytes),
      static_cast<unsigned long>(details.capacity[0]),
      static_cast<unsigned long>(details.capacity[1]),
      static_cast<unsigned long>(details.capacity[2]),
      static_cast<unsigned long>(details.capacity[3]),
      static_cast<unsigned long>(details.capacity[4]),
      static_cast<unsigned long>(details.capacity[5]),
      static_cast<unsigned long>(details.used[0]),
      static_cast<unsigned long>(details.used[1]),
      static_cast<unsigned long>(details.used[2]),
      static_cast<unsigned long>(details.used[3]),
      static_cast<unsigned long>(details.used[4]),
      static_cast<unsigned long>(details.used[5]),
      static_cast<unsigned long long>(details.evicted[0]),
      static_cast<unsigned long long>(details.evicted[1]),
      static_cast<unsigned long long>(details.evicted[2]),
      static_cast<unsigned long long>(details.evicted[3]),
      static_cast<unsigned long long>(details.evicted[4]),
      static_cast<unsigned long long>(details.evicted[5]),
      static_cast<unsigned long long>(details.rejected[0]),
      static_cast<unsigned long long>(details.rejected[1]),
      static_cast<unsigned long long>(details.rejected[2]),
      static_cast<unsigned long long>(details.rejected[3]),
      static_cast<unsigned long long>(details.rejected[4]),
      static_cast<unsigned long long>(details.rejected[5]));
  }
  return std::string(body);
}
static esp_err_t log_config_get(httpd_req_t* req){
  const auto body=log_config_snapshot_json();
  return reply(req,"application/json",body.c_str());
}
static bool parsed_log_limit(const cJSON* obj,const char* key,unsigned maximum,uint16_t* out){
  const auto* item=cJSON_GetObjectItemCaseSensitive(obj,key);
  if(!cJSON_IsNumber(item)||item->valuedouble<0 ||
     item->valuedouble>maximum || item->valueint!=item->valuedouble)return false;
  *out=static_cast<uint16_t>(item->valueint);
  return true;
}
static bool browser_json_write(httpd_req_t* req) {
  // A regular HTML form cannot send this media type, and cross-origin
  // JavaScript cannot set it without a CORS preflight (not allowed by
  // the native server). Keep bounded, low-risk local UI settings keyless.
  char type[48]{};
  if(httpd_req_get_hdr_value_str(req,"Content-Type",type,sizeof(type))!=ESP_OK)
    return false;
  constexpr char expected[]="application/json";
  const size_t n=sizeof(expected)-1;
  return strncmp(type,expected,n)==0 && (type[n]=='\0' || type[n]==';');
}
// Frontend-only audit. No arbitrary strings or request bodies enter the ring.
static esp_err_t web_ui_event_post(httpd_req_t* req){
  if(!log_provider)return send_api_error(req,HTTPD_500_INTERNAL_SERVER_ERROR,"Log store unavailable");
  if(!browser_json_write(req))
    return send_api_error(req,HTTPD_403_FORBIDDEN,"Same-origin JSON required");
  if(req->content_len<=0 || req->content_len>192)
    return send_api_error(req,HTTPD_400_BAD_REQUEST,"Invalid UI audit length");
  char body[193]{};
  int received=0,attempts=0;
  while(received<req->content_len){
    const int n=httpd_req_recv(req,body+received,req->content_len-received);
    if(n==HTTPD_SOCK_ERR_TIMEOUT && ++attempts<=3)continue;
    if(n<=0)return send_api_error(req,HTTPD_400_BAD_REQUEST,"UI audit body incomplete");
    received+=n;
  }
  cJSON* json=cJSON_ParseWithLength(body,static_cast<size_t>(received));
  const auto* action=json?cJSON_GetObjectItemCaseSensitive(json,"action"):nullptr;
  const auto* page=json?cJSON_GetObjectItemCaseSensitive(json,"page"):nullptr;
  const auto* value=json?cJSON_GetObjectItemCaseSensitive(json,"value"):nullptr;
  const auto* endpoint=json?cJSON_GetObjectItemCaseSensitive(json,"endpoint"):nullptr;
  constexpr const char* actions[]={
    "page_open","log_source","log_radio","log_noise","log_expand",
    "log_collapse","log_export","log_older","log_newer","log_latest",
    "console_connect","console_disconnect","console_clear",
    "diagnostics_refresh","settings_submit","led_submit",
    "ota_confirm","ota_result","api_client_error","radio_inspect"
  };
  constexpr const char* pages[]={
    "status","devices","logs","learn","diagnostics","settings"
  };
  uint16_t action_id=0,page_id=0;
  if(cJSON_IsString(action)&&action->valuestring)
    for(size_t i=0;i<sizeof(actions)/sizeof(actions[0]);++i)
      if(strcmp(actions[i],action->valuestring)==0){action_id=uint16_t(i+1);break;}
  if(cJSON_IsString(page)&&page->valuestring)
    for(size_t i=0;i<sizeof(pages)/sizeof(pages[0]);++i)
      if(strcmp(pages[i],page->valuestring)==0){page_id=uint16_t(i+1);break;}
  const bool valid_value=!value || (cJSON_IsNumber(value) &&
    value->valuedouble>=0 && value->valuedouble<=999 &&
    value->valueint==value->valuedouble);
  const bool valid_endpoint=!endpoint || (cJSON_IsNumber(endpoint) &&
    endpoint->valuedouble>=0 && endpoint->valuedouble<=14 &&
    endpoint->valueint==endpoint->valuedouble);
  const uint8_t endpoint_id=(endpoint&&cJSON_IsNumber(endpoint))?
    static_cast<uint8_t>(endpoint->valueint):0;
  const int status=(value&&cJSON_IsNumber(value))?value->valueint:0;
  if(json)cJSON_Delete(json);
  if(!action_id||!page_id||!valid_value||!valid_endpoint)
    return send_api_error(req,HTTPD_400_BAD_REQUEST,"Unsupported UI audit event");
  log_provider->push(rflink::LogStream::web,
    action_id==1?rflink::LogCode::web_page_open:
      action_id==19?rflink::LogCode::web_client_error:rflink::LogCode::web_ui_action,
    static_cast<uint32_t>(esp_timer_get_time()/1000),
    rflink::kAnyRadio,action_id==19?rflink::LogSeverity::warning:rflink::LogSeverity::info,
    action_id,status,0,0,page_id,endpoint_id);
  return reply(req,"application/json","{\"ok\":true}");
}
static esp_err_t log_config_post(httpd_req_t* req){
  if(!log_provider)return send_api_error(req,HTTPD_500_INTERNAL_SERVER_ERROR,"Log store unavailable");
  if(!browser_json_write(req))
    return send_api_error(req,HTTPD_403_FORBIDDEN,"Same-origin JSON required");
  if(ota_busy.load())return send_api_error(req,HTTPD_400_BAD_REQUEST,"OTA in progress");
  if(req->content_len<=0||req->content_len>320)
    return send_api_error(req,HTTPD_400_BAD_REQUEST,"Invalid settings body");
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
  if(!valid)return send_api_error(req,HTTPD_400_BAD_REQUEST,"Log limits exceed 1.5 MB memory cap or invalid");
  if(!log_provider->configure(limits))
    return send_api_error(req,HTTPD_500_INTERNAL_SERVER_ERROR,"Not enough PSRAM for log limits");
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

static esp_err_t led_config_get(httpd_req_t* req){
  const auto led=rflink::status_led_snapshot();
  char body[280]{};
  std::snprintf(body,sizeof(body),
    "{\"enabled\":%s,\"brightness\":%u,\"gpio\":%d,"
    "\"mode\":\"%s\",\"driver_ready\":%s,"
    "\"frames_sent\":%lu,\"errors\":%lu,\"last_error\":%ld}",
    led.settings.enabled?"true":"false",unsigned(led.settings.brightness),
    rflink::kStatusLedGpio,rflink::led_mode_name(led.mode),
    led.driver_ready?"true":"false",
    static_cast<unsigned long>(led.frames_sent),
    static_cast<unsigned long>(led.errors),
    static_cast<long>(led.last_error));
  return reply(req,"application/json",body);
}
static esp_err_t led_config_post(httpd_req_t* req){
  if(!browser_json_write(req))
    return send_api_error(req,HTTPD_403_FORBIDDEN,"Same-origin JSON required");
  if(ota_busy.load())
    return send_api_error(req,HTTPD_400_BAD_REQUEST,"OTA in progress");
  if(req->content_len<=0 || req->content_len>128)
    return send_api_error(req,HTTPD_400_BAD_REQUEST,"Invalid LED settings size");
  std::string body(static_cast<size_t>(req->content_len),'\0');
  size_t offset=0;int timeouts=0;
  while(offset<body.size()){
    const int count=httpd_req_recv(req,&body[offset],body.size()-offset);
    if(count==HTTPD_SOCK_ERR_TIMEOUT && ++timeouts<=5)continue;
    if(count<=0)return ESP_FAIL;
    offset+=static_cast<size_t>(count);timeouts=0;
  }
  cJSON* parsed=cJSON_ParseWithLength(body.data(),body.size());
  const cJSON* on=parsed?cJSON_GetObjectItemCaseSensitive(parsed,"enabled"):nullptr;
  const cJSON* bright=parsed?cJSON_GetObjectItemCaseSensitive(parsed,"brightness"):nullptr;
  const bool valid=cJSON_IsBool(on) && cJSON_IsNumber(bright) &&
    bright->valuedouble>=1 && bright->valuedouble<=100 &&
    bright->valueint==bright->valuedouble;
  rflink::LedSettings candidate{};
  if(valid){
    candidate.enabled=cJSON_IsFalse(on)?0:1;
    candidate.brightness=static_cast<uint8_t>(bright->valueint);
  }
  if(parsed)cJSON_Delete(parsed);
  if(!valid)
    return send_api_error(req,HTTPD_400_BAD_REQUEST,
                               "LED enabled must be bool and brightness 1-100");
  if(!rflink::status_led_save_settings(candidate))
    return send_api_error(req,HTTPD_500_INTERNAL_SERVER_ERROR,
                               "LED settings not persisted; unchanged");
  return reply(req,"application/json","{\"saved\":true}");
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
  if(!key_ok(req))return send_api_error(req,HTTPD_403_FORBIDDEN,"RFLink management key required");
  if(ota_busy.load())return send_api_error(req,HTTPD_400_BAD_REQUEST,"OTA in progress");
  if(req->content_len<=0 || req->content_len>1200)return send_api_error(req,HTTPD_400_BAD_REQUEST,"Invalid JSON size");
  std::string json(static_cast<size_t>(req->content_len),'\0');
  size_t offset=0;int failures=0;
  while(offset<json.size()){
    const int n=httpd_req_recv(req,&json[offset],json.size()-offset);
    if(n==HTTPD_SOCK_ERR_TIMEOUT && ++failures<=5)continue;
    if(n<=0)return ESP_FAIL;
    offset+=static_cast<size_t>(n);failures=0;
  }
  cJSON* obj=cJSON_ParseWithLength(json.data(),json.size());
  if(!obj)return send_api_error(req,HTTPD_400_BAD_REQUEST,"Malformed JSON");
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
  if(!ok)return send_api_error(req,HTTPD_400_BAD_REQUEST,"Unsupported or invalid MQTT settings (TLS requires a future verified-certificate build)");
  if(!rflink::mqtt_save_settings(candidate))return send_api_error(req,HTTPD_500_INTERNAL_SERVER_ERROR,"NVS write failed");
  httpd_resp_set_hdr(req,"Cache-Control","no-store");
  httpd_resp_set_type(req,"application/json");
  httpd_resp_sendstr(req,"{\"ok\":true,\"restart_required\":true}");
  record_web_api(req,rflink::LogCode::web_api_success,200);
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
// Keep the WPA2 provisioning screen for new boards, without any legacy
// radio polling or duplicate Advanced diagnostics.
static esp_err_t provisioning_index_get(httpd_req_t* req){
  constexpr const char* page=R"RFHTML(<!doctype html><html lang="nl">
<head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>RFLink · Wi-Fi installatie</title>
<style>body{font:15px/1.6 system-ui;max-width:540px;margin:35px auto;padding:20px;background:#101d28;color:#e9f2f5}
section{background:#14242f;border:1px solid #34515d;border-radius:14px;padding:22px}
h1{margin:0 0 8px}p{color:#a5b8c3}label{display:block;margin:15px 0 4px}
input{display:block;box-sizing:border-box;width:100%;padding:11px;background:#0b1c27;color:#fff;border:1px solid #3b5764;border-radius:8px}
button{background:#35b8ba;border:0;border-radius:9px;padding:12px 20px;margin-top:18px;font-weight:700}
</style></head><body><section><h1>RFLink Universal</h1>
<p>Initial setup: connect this board to your Wi-Fi network. The temporary setup access point is disabled after saving and restarting.</p>
<form method="post" action="/setup">
<label>Wi-Fi SSID<input name="ssid" maxlength="32" required></label>
<label>Wi-Fi password<input name="pass" type="password" maxlength="63"></label>
<label>OTA administrator password (12–64 characters)<input name="admin" type="password" minlength="12" maxlength="64" required></label>
<button type="submit">Save and restart</button></form></section></body></html>)RFHTML";
  return reply(req,"text/html; charset=utf-8",page);
}
static esp_err_t native_redirect_get(httpd_req_t* req){
  // Redirect stale bookmarks rather than starting old /rflink API polling.
  httpd_resp_set_status(req,"302 Found");
  httpd_resp_set_hdr(req,"Location","/");
  return reply(req,"text/plain; charset=utf-8","RFLink has moved to /");
}
static esp_err_t setup_post(httpd_req_t* req){
  if(!provisioning)return send_api_error(req,HTTPD_403_FORBIDDEN,"Already configured");
  if(req->content_len<=0||req->content_len>MAX_FORM)return send_api_error(req,HTTPD_400_BAD_REQUEST,"Bad form size");
  std::string body(req->content_len,'\0');size_t off=0;
  while(off<body.size()){int n=httpd_req_recv(req,&body[off],body.size()-off);if(n<=0)return ESP_FAIL;off+=n;}
  auto ssid=field(body,"ssid"),password=field(body,"pass"),admin=field(body,"admin");
  if(ssid.empty()||ssid.size()>32||password.size()>63||admin.size()<12||admin.size()>64)
    return send_api_error(req,HTTPD_400_BAD_REQUEST,"Invalid SSID/password/admin length");
  nvs_handle_t h;
  if(nvs_open("rf_net",NVS_READWRITE,&h)!=ESP_OK)return send_api_error(req,HTTPD_500_INTERNAL_SERVER_ERROR,"NVS unavailable");
  auto e=nvs_set_str(h,"ssid",ssid.c_str());
  if(e==ESP_OK)e=nvs_set_str(h,"pass",password.c_str());
  if(e==ESP_OK)e=nvs_set_str(h,"update_key",admin.c_str());
  if(e==ESP_OK)e=nvs_commit(h);
  nvs_close(h);
  if(e!=ESP_OK)return send_api_error(req,HTTPD_500_INTERNAL_SERVER_ERROR,"NVS write error");
  httpd_resp_sendstr(req,"Saved. Device will restart. Open its IP on your WiFi network.");
  xTaskCreate([](void*){vTaskDelay(pdMS_TO_TICKS(1000));esp_restart();},"setup_restart",3072,nullptr,4,nullptr);
  return ESP_OK;
}
static esp_err_t ota_post(httpd_req_t* req){
  if(!key_ok(req))return send_api_error(req,HTTPD_403_FORBIDDEN,"Update password required");

  const auto ota_layout=inspect_ota_layout();
  // Select the other physical slot by its known flash address. With only
  // one live OTA partition the round-robin helper may return the running app.
  // Never overwrite the executing firmware or erase its NVS to fix this.
  if(!ota_layout.plan.safe || !ota_layout.target) {
    ESP_LOGE(TAG,"OTA preflight rejected: %s count=%u running=0x%lx",
             ota_layout.plan.reason,ota_layout.count,
             static_cast<unsigned long>(ota_layout.running?
                                        ota_layout.running->address:0));
    return send_api_error(req,HTTPD_400_BAD_REQUEST,
      "OTA blocked: no safe inactive partition. Check actual flash partition "
      "table; full USB recovery may be required. Running firmware unchanged.");
  }
  const esp_partition_t* next=ota_layout.target;
  ESP_LOGI(TAG,"OTA partition preflight: running=%s @0x%lx target=%s @0x%lx count=%u",
           ota_layout.running->label,
           static_cast<unsigned long>(ota_layout.running->address),
           next->label,static_cast<unsigned long>(next->address),ota_layout.count);
  if(req->content_len<256 || static_cast<size_t>(req->content_len)>next->size ||
     static_cast<size_t>(req->content_len)>MAX_IMAGE)
    return send_api_error(req,HTTPD_400_BAD_REQUEST,"Invalid image length for OTA slot");
  if(ota_busy.exchange(true))return send_api_error(req,HTTPD_400_BAD_REQUEST,"Update in progress");
  rflink::status_led_set_ota_active(true);
  esp_ota_handle_t h=0;
  esp_err_t e=esp_ota_begin(next,req->content_len,&h);
  if(e!=ESP_OK) {
    ota_busy=false;
    rflink::status_led_set_ota_active(false);
    rflink::status_led_signal_error();
    ESP_LOGE(TAG,"OTA begin failed: %s running=0x%lx target=0x%lx",
             esp_err_to_name(e),static_cast<unsigned long>(ota_layout.running->address),
             static_cast<unsigned long>(next->address));
    return send_api_error(req,HTTPD_500_INTERNAL_SERVER_ERROR,
                                esp_err_to_name(e));
  }
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
    rflink::status_led_set_ota_active(false);
    rflink::status_led_signal_error();
    ESP_LOGE(TAG,"OTA failure: %s",esp_err_to_name(e));
    return send_api_error(req,HTTPD_500_INTERNAL_SERVER_ERROR,esp_err_to_name(e));
  }
  record_web_api(req,rflink::LogCode::web_api_success,200);
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
    rflink::status_led_set_wifi_connected(false);
    ESP_LOGW(TAG,"Wi-Fi disconnected: reason=%u; reconnecting",reason);
    if(log_provider)log_provider->push(rflink::LogStream::system,
      rflink::LogCode::wifi_disconnected,now,rflink::kAnyRadio,rflink::LogSeverity::warning,
      0,static_cast<int32_t>(reason));
    esp_wifi_connect();
  }else if(base==IP_EVENT && id==IP_EVENT_STA_GOT_IP){
    const auto* got=static_cast<const ip_event_got_ip_t*>(data);
    sta_connected=true;
    rflink::status_led_set_wifi_connected(true);
    rflink::time_service_start(); // Nonblocking; later SNTP callbacks update UTC

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
  if(provisioning)return provisioning_index_get(req);
  return reply(req,"text/html; charset=utf-8",
               rflink::kNativeUiHtml);
}
// Log migration mistakes with bounded URL/referrer metadata. Not a
// legacy API compatibility layer and never expose query-string secrets.
static esp_err_t native_unknown_uri(httpd_req_t* req,httpd_err_code_t error){
  if(error==HTTPD_404_NOT_FOUND && req &&
     strncmp(req->uri,"/rflink/",8)==0){
    char path[96]{},referer[96]{},agent[64]{};
    snprintf(path,sizeof(path),"%.95s",req->uri);
    path[strcspn(path,"?#")]=0;
    if(httpd_req_get_hdr_value_str(req,"Referer",referer,sizeof(referer))==ESP_OK)
      referer[strcspn(referer,"?#")]=0;
    if(httpd_req_get_hdr_value_str(req,"User-Agent",agent,sizeof(agent))!=ESP_OK)
      agent[0]=0;
    ESP_LOGW(TAG,"Legacy API 404: path=%s referer=%s agent=%s",
             path,referer[0]?referer:"(none)",agent[0]?agent:"(none)");
    return send_api_error(req,HTTPD_404_NOT_FOUND,
              "Legacy RFLink API unavailable: native UI is / and API is /api/");
  }
  return send_api_error(req,HTTPD_404_NOT_FOUND,"Not Found");
}
static bool start_http(){
  httpd_config_t cfg=HTTPD_DEFAULT_CONFIG();
  // The httpd server also performs diagnostic export and OTA. Keep headroom
  // for ESP-IDF HTTP internals and callback frames, even under heavy RX.
  cfg.stack_size=16384;cfg.max_uri_handlers=24;cfg.recv_wait_timeout=10;
  esp_err_t result=httpd_start(&server,&cfg);
  if(result!=ESP_OK){ESP_LOGE(TAG,"HTTP server failed: %s",esp_err_to_name(result));return false;}
  httpd_uri_t root={};root.uri="/";root.method=HTTP_GET;root.handler=native_index_get;
  httpd_uri_t advanced{};advanced.uri="/advanced";advanced.method=HTTP_GET;advanced.handler=native_redirect_get;
  httpd_uri_t legacy{};legacy.uri="/rflink";legacy.method=HTTP_GET;legacy.handler=native_redirect_get;
  httpd_uri_t legacy_slash{};legacy_slash.uri="/rflink/";legacy_slash.method=HTTP_GET;legacy_slash.handler=native_redirect_get;
  httpd_uri_t styles{};styles.uri="/app.css";styles.method=HTTP_GET;styles.handler=native_styles_get;
  httpd_uri_t script{};script.uri="/app.js";script.method=HTTP_GET;script.handler=native_script_get;
  httpd_uri_t provision={};provision.uri="/setup";provision.method=HTTP_POST;provision.handler=setup_post;
  httpd_uri_t ota={};ota.uri="/api/ota";ota.method=HTTP_POST;ota.handler=ota_post;
  httpd_uri_t status{};status.uri="/api/status";status.method=HTTP_GET;status.handler=status_get;
  httpd_uri_t crashes{};crashes.uri="/api/diagnostics/crashes";crashes.method=HTTP_GET;crashes.handler=crashes_get;
  httpd_uri_t console{};console.uri="/api/diagnostics/console";console.method=HTTP_GET;console.handler=console_get;
  httpd_uri_t raw_capture{};raw_capture.uri="/api/radio/capture";raw_capture.method=HTTP_GET;raw_capture.handler=raw_capture_get;
  httpd_uri_t logs{};logs.uri="/api/logs";logs.method=HTTP_GET;logs.handler=logs_get;
  httpd_uri_t log_detail{};log_detail.uri="/api/logs/detail";log_detail.method=HTTP_GET;log_detail.handler=log_detail_get;
  httpd_uri_t logs_csv{};logs_csv.uri="/api/logs.csv";logs_csv.method=HTTP_GET;logs_csv.handler=logs_csv_async_get;
  httpd_uri_t web_ui{};web_ui.uri="/api/logs/ui";web_ui.method=HTTP_POST;web_ui.handler=web_ui_event_post;
  httpd_uri_t log_conf_get{};log_conf_get.uri="/api/logs/config";log_conf_get.method=HTTP_GET;log_conf_get.handler=log_config_get;
  httpd_uri_t log_conf_post{};log_conf_post.uri="/api/logs/config";log_conf_post.method=HTTP_POST;log_conf_post.handler=log_config_post;
  httpd_uri_t led_get{};led_get.uri="/api/led";led_get.method=HTTP_GET;led_get.handler=led_config_get;
  httpd_uri_t led_post{};led_post.uri="/api/led";led_post.method=HTTP_POST;led_post.handler=led_config_post;
  httpd_uri_t events{};events.uri="/api/events";events.method=HTTP_GET;events.handler=events_get;
  httpd_uri_t events_csv{};events_csv.uri="/api/events.csv";events_csv.method=HTTP_GET;events_csv.handler=events_csv_get;
  httpd_uri_t mqtt_get{};mqtt_get.uri="/api/mqtt/status";mqtt_get.method=HTTP_GET;mqtt_get.handler=mqtt_status_get;
  httpd_uri_t mqtt_post{};mqtt_post.uri="/api/mqtt/config";mqtt_post.method=HTTP_POST;mqtt_post.handler=mqtt_config_post;
  if(httpd_register_uri_handler(server,&root)!=ESP_OK ||
     httpd_register_uri_handler(server,&advanced)!=ESP_OK ||
     httpd_register_uri_handler(server,&legacy)!=ESP_OK ||
     httpd_register_uri_handler(server,&legacy_slash)!=ESP_OK ||
     httpd_register_uri_handler(server,&styles)!=ESP_OK ||
     httpd_register_uri_handler(server,&script)!=ESP_OK ||
     httpd_register_uri_handler(server,&provision)!=ESP_OK ||
     httpd_register_uri_handler(server,&ota)!=ESP_OK ||
     httpd_register_uri_handler(server,&status)!=ESP_OK ||
     httpd_register_uri_handler(server,&crashes)!=ESP_OK ||
     httpd_register_uri_handler(server,&console)!=ESP_OK ||
     httpd_register_uri_handler(server,&raw_capture)!=ESP_OK ||
     httpd_register_uri_handler(server,&logs)!=ESP_OK ||
     httpd_register_uri_handler(server,&log_detail)!=ESP_OK ||
     httpd_register_uri_handler(server,&logs_csv)!=ESP_OK ||
     httpd_register_uri_handler(server,&web_ui)!=ESP_OK ||
     httpd_register_uri_handler(server,&log_conf_get)!=ESP_OK ||
     httpd_register_uri_handler(server,&log_conf_post)!=ESP_OK ||
     httpd_register_uri_handler(server,&led_get)!=ESP_OK ||
     httpd_register_uri_handler(server,&led_post)!=ESP_OK ||
     httpd_register_uri_handler(server,&events)!=ESP_OK ||
     httpd_register_uri_handler(server,&events_csv)!=ESP_OK ||
     httpd_register_uri_handler(server,&mqtt_get)!=ESP_OK ||
     httpd_register_uri_handler(server,&mqtt_post)!=ESP_OK ||
     httpd_register_err_handler(server,HTTPD_404_NOT_FOUND,&native_unknown_uri)!=ESP_OK){
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
void rflink_set_detail_store_provider(rflink::DetailStore* details) { detail_provider=details; }
