#include "rflink_mdns.hpp"
#include "mdns.h"
#include "esp_mac.h"
#include "esp_log.h"
#include <cstdio>

namespace {
constexpr char TAG[]="rflink.discovery";
char service_hostname[20]{};
char service_device_id[16]{};

bool prepare_identity() {
  if(service_hostname[0] && service_device_id[0]) return true;
  uint8_t mac[6]{};
  if(esp_read_mac(mac,ESP_MAC_WIFI_STA)!=ESP_OK) return false;
  snprintf(service_hostname,sizeof(service_hostname),
    "rflink-%02x%02x%02x%02x%02x%02x",
    mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
  snprintf(service_device_id,sizeof(service_device_id),
    "rf-%02x%02x%02x%02x%02x%02x",
    mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
  return true;
}
}

void rflink_set_dhcp_hostname(esp_netif_t* station) {
  if(!station || !prepare_identity()) return;
  const esp_err_t err=esp_netif_set_hostname(station,service_hostname);
  if(err!=ESP_OK) ESP_LOGW(TAG,"DHCP hostname unavailable: %s",esp_err_to_name(err));
}

bool rflink_start_service_discovery() {
  if(!prepare_identity()) return false;
  esp_err_t err=mdns_init();
  if(err!=ESP_OK){
    ESP_LOGW(TAG,"mDNS init unavailable: %s",esp_err_to_name(err));
    return false;
  }
  err=mdns_hostname_set(service_hostname);
  if(err!=ESP_OK){
    ESP_LOGW(TAG,"mDNS hostname unavailable: %s",esp_err_to_name(err));
    return false;
  }
  err=mdns_instance_name_set(service_hostname);
  if(err!=ESP_OK){
    ESP_LOGW(TAG,"mDNS instance unavailable: %s",esp_err_to_name(err));
    return false;
  }
  // Only public identity/HTTP discovery metadata. TXT is not trusted by
  // Builder for pairing; /api/status must confirm identity and board profile.
  const mdns_txt_item_t txt[]={
    {"id",service_device_id},
    {"path","/api/status"},
    {"schema","rflink-native-v07"},
  };
  err=mdns_service_add(nullptr,"_rflink","_tcp",80,txt,3);
  if(err!=ESP_OK){
    ESP_LOGW(TAG,"RFLink mDNS service unavailable: %s",esp_err_to_name(err));
    return false;
  }
  ESP_LOGI(TAG,"RFLink DNS-SD: %s.local _rflink._tcp port 80",service_hostname);
  return true;
}
