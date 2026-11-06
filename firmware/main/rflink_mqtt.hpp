#pragma once
#include <cstdint>
#include <cstddef>
#include <string>

namespace rflink {
struct MqttSettings {
  uint32_t schema = 1;
  char host[96]{};
  char username[65]{};
  char password[129]{};
  uint16_t port = 1883;
  uint8_t tls = 0;
  uint8_t enabled = 1;
};
// Pure validation reused by host tests. Reject malformed hosts and TLS until
// broker certificate verification has been fully implemented.
bool mqtt_valid_host(const char* host);
bool mqtt_valid_settings(const MqttSettings& s);
std::string mqtt_hub_id(const uint8_t mac[6]);

// Starts only when a provisioned WiFi STA receives an IP and MQTT is configured.
void mqtt_service_init();
bool mqtt_load_settings(MqttSettings* out);
bool mqtt_save_settings(const MqttSettings& settings);
bool mqtt_is_connected();
} // namespace rflink
