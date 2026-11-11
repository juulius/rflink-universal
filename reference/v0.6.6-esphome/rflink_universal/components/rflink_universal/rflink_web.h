#pragma once

#ifdef USE_RFLINK_UNIVERSAL_WEB

#include <string>

#include "esphome/components/web_server_base/web_server_base.h"

namespace esphome {
namespace rflink_universal {

class RFLinkUniversal;

class RFLinkWebHandler : public AsyncWebHandler {
 public:
  explicit RFLinkWebHandler(RFLinkUniversal *parent) : parent_(parent) {}
  bool canHandle(AsyncWebServerRequest *request) const override;
  void handleRequest(AsyncWebServerRequest *request) override;

 protected:
  std::string url_(AsyncWebServerRequest *request) const;
  void send_json_(AsyncWebServerRequest *request, const std::string &json) const;
  void send_error_(AsyncWebServerRequest *request, int code, const std::string &message) const;
  std::string status_json_() const;
  std::string activity_export_json_() const;
#if defined(USE_ESP32) && defined(ESP_PLATFORM)
  void send_rx_log_chunked_(AsyncWebServerRequest *request) const;
#else
  std::string rx_log_json_(AsyncWebServerRequest *request) const;  // Host fallback only.
#endif
  void handle_rx_log_config_(AsyncWebServerRequest *request);

  std::string system_trace_json_() const;
  std::string devices_json_() const;
  std::string developer_json_() const;
  std::string decoders_json_() const;
  std::string library_json_() const;
  std::string user_library_json_() const;
  void handle_user_library_(AsyncWebServerRequest *request);
  void handle_user_tx_(AsyncWebServerRequest *request);
  void handle_candidate_(AsyncWebServerRequest *request);
  void handle_device_(AsyncWebServerRequest *request);
  void handle_developer_(AsyncWebServerRequest *request);
  void handle_led_settings_(AsyncWebServerRequest *request);
  void handle_mqtt_settings_(AsyncWebServerRequest *request);

  RFLinkUniversal *parent_{nullptr};
};

}  // namespace rflink_universal
}  // namespace esphome

#endif
