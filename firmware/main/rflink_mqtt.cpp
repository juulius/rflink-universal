#include "rflink_mqtt.hpp"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_mac.h"
#include "esp_timer.h"
#include "mqtt_client.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <atomic>
#include <cstdio>
#include <cstring>
#include <string>

namespace rflink {
namespace {
constexpr const char* TAG="rflink_mqtt";
constexpr const char* NS="rf_mqtt";
constexpr const char* KEY="cfg";
MqttSettings active{};
std::string hub_id, availability_topic, uptime_topic, discovery_topic, broker_uri;
esp_mqtt_client_handle_t client=nullptr;
std::atomic<bool> connected{false};
bool client_started=false;
bool initialized=false;

void publish_discovery() {
  if(!client || !connected.load()) return;
  char body[1152];
  // All strings in this JSON come from our stable generated MAC identity.
  // User-supplied secrets, hostnames and device names are never serialized.
  const int n=snprintf(body,sizeof(body),
    "{\"device\":{\"identifiers\":[\"%s\"],\"name\":\"RFLink Universal\","
    "\"manufacturer\":\"RFLink Universal\",\"model\":\"ESP32-S3 N16R8\",\"sw_version\":\"0.7.0 Radioactive\"},"
    "\"origin\":{\"name\":\"RFLink Universal\",\"sw_version\":\"0.7.0\"},"
    "\"components\":{\"uptime\":{\"platform\":\"sensor\",\"name\":\"Uptime\","
    "\"unique_id\":\"%s_uptime\",\"state_topic\":\"%s\","
    "\"unit_of_measurement\":\"s\",\"icon\":\"mdi:timer-outline\"}},"
    "\"availability_topic\":\"%s\",\"payload_available\":\"online\",\"payload_not_available\":\"offline\"}",
    hub_id.c_str(),hub_id.c_str(),uptime_topic.c_str(),availability_topic.c_str());
  if(n<0 || n>=static_cast<int>(sizeof(body))) return;
  esp_mqtt_client_publish(client,discovery_topic.c_str(),body,0,1,1);
  esp_mqtt_client_publish(client,availability_topic.c_str(),"online",0,1,1);
  char uptime[32];snprintf(uptime,sizeof(uptime),"%llu",static_cast<unsigned long long>(esp_timer_get_time()/1000000));
  esp_mqtt_client_publish(client,uptime_topic.c_str(),uptime,0,0,0);
}
void mqtt_events(void*,esp_event_base_t,int32_t event_id,void* data) {
  auto* ev=static_cast<esp_mqtt_event_handle_t>(data);
  switch(event_id){
    case MQTT_EVENT_CONNECTED:
      connected=true;
      ESP_LOGI(TAG,"MQTT broker connected");
      esp_mqtt_client_subscribe(client,"homeassistant/status",0);
      publish_discovery();
      break;
    case MQTT_EVENT_DISCONNECTED:
      connected=false;
      ESP_LOGW(TAG,"MQTT disconnected");
      break;
    case MQTT_EVENT_DATA:
      if(ev && ev->topic_len==20 && ev->data_len==6 && ev->current_data_offset==0 &&
         memcmp(ev->topic,"homeassistant/status",20)==0 && memcmp(ev->data,"online",6)==0) publish_discovery();
      break;
    default: break;
  }
}
void uptime_task(void*) {
  while(true){
    vTaskDelay(pdMS_TO_TICKS(60000));
    if(connected.load() && client){
      char uptime[32];snprintf(uptime,sizeof(uptime),"%llu",static_cast<unsigned long long>(esp_timer_get_time()/1000000));
      esp_mqtt_client_publish(client,uptime_topic.c_str(),uptime,0,0,0);
    }
  }
}
void start_client() {
  if(client_started || !mqtt_valid_settings(active)) return;
  uint8_t mac[6]{};
  if(esp_read_mac(mac,ESP_MAC_WIFI_STA)!=ESP_OK) return;
  hub_id=mqtt_hub_id(mac);
  availability_topic=hub_id+"/availability";
  uptime_topic=hub_id+"/uptime";
  discovery_topic="homeassistant/device/"+hub_id+"/config";
  broker_uri="mqtt://"+std::string(active.host)+":"+std::to_string(active.port);
  esp_mqtt_client_config_t cfg{};
  cfg.broker.address.uri=broker_uri.c_str();
  cfg.credentials.client_id=hub_id.c_str();
  cfg.credentials.username=active.username[0]?active.username:nullptr;
  cfg.credentials.authentication.password=active.password[0]?active.password:nullptr;
  cfg.session.last_will.topic=availability_topic.c_str();
  cfg.session.last_will.msg="offline";
  cfg.session.last_will.qos=1;
  cfg.session.last_will.retain=1;
  client=esp_mqtt_client_init(&cfg);
  if(!client){ESP_LOGE(TAG,"MQTT init failed");return;}
  if(esp_mqtt_client_register_event(client,MQTT_EVENT_ANY,&mqtt_events,nullptr)!=ESP_OK ||
     esp_mqtt_client_start(client)!=ESP_OK){
    ESP_LOGE(TAG,"MQTT client start failed");
    esp_mqtt_client_destroy(client);client=nullptr;return;
  }
  client_started=true;
  xTaskCreate(uptime_task,"rf_mqtt_uptime",3072,nullptr,3,nullptr);
}
void ip_events(void*,esp_event_base_t,int32_t event_id,void*) {
  if(event_id==IP_EVENT_STA_GOT_IP)start_client();
}
} // namespace

bool mqtt_load_settings(MqttSettings* out) {
  if(!out)return false;
  nvs_handle_t h;
  if(nvs_open(NS,NVS_READONLY,&h)!=ESP_OK)return false;
  MqttSettings copy{};size_t size=sizeof(copy);
  esp_err_t e=nvs_get_blob(h,KEY,&copy,&size);
  nvs_close(h);
  if(e!=ESP_OK || size!=sizeof(copy) || !mqtt_valid_settings(copy))return false;
  *out=copy;
  return true;
}
bool mqtt_save_settings(const MqttSettings& s) {
  if(!mqtt_valid_settings(s))return false;
  nvs_handle_t h;
  if(nvs_open(NS,NVS_READWRITE,&h)!=ESP_OK)return false;
  esp_err_t e=nvs_set_blob(h,KEY,&s,sizeof(s));
  if(e==ESP_OK)e=nvs_commit(h);
  nvs_close(h);
  return e==ESP_OK;
}
bool mqtt_is_connected(){return connected.load();}
void mqtt_service_init() {
  if(initialized)return;
  initialized=true;
  if(!mqtt_load_settings(&active)){ESP_LOGI(TAG,"MQTT not configured; no client started");return;}
  esp_event_handler_register(IP_EVENT,IP_EVENT_STA_GOT_IP,&ip_events,nullptr);
  // Handle cases where the station got an IP before registering the callback.
  auto* sta=esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
  if(sta){esp_netif_ip_info_t ip{};if(esp_netif_get_ip_info(sta,&ip)==ESP_OK && ip.ip.addr!=0)start_client();}
}
} // namespace rflink
