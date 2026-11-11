#pragma once
#include "crash_trace_collector.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <limits>
#include "rflink_log_store.h"
#if defined(USE_ESP32) && defined(ESP_PLATFORM)
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#endif

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/remote_base/remote_base.h"
#include "esphome/components/remote_receiver/remote_receiver.h"
#include "esphome/components/remote_transmitter/remote_transmitter.h"
#include "esphome/components/cc1101/cc1101.h"

#include "decoder.h"
#include "developer_session.h"
#include "device_store.h"
#include "device_persistence.h"
#include "tx_engine.h"
#include "system_trace.h"
#include "serial_tx_adapter.h"
#include "user_library.h"
#include "itho_codec.h"
#include "status_led.h"
#include "radio_boot_guard.h"
#include "radio_mode_manager.h"
#include "radio_transport_policy.h"
#if defined(RFLINK_DECODER_EXPECTED_COUNT) && RFLINK_DECODER_EXPECTED_COUNT > 0
#include "../../../rflink_decoders/radio_transport_catalog_generated.h"
#else
#include "radio_transport_catalog_generated.h" // host-test fallback
#endif
#include "ramses_spi_fifo.h"
#ifdef USE_ESP32
#include "ramses_spi_esp_backend.h"
#include "ramses_uart_esp_backend.h"
#include "cc1101_gdo_uart_tx.h"
#endif
#ifdef USE_ESP32
#include "esphome/core/preferences.h"
#endif

#ifdef USE_RFLINK_UNIVERSAL_WEB
#include "rflink_web.h"
#endif

namespace rflink_ramses_ii { struct Frame; }
namespace esphome {
namespace cc1101 { class CC1101Component; }
namespace rflink_universal {

class RFLinkUniversal;

class ChannelListener : public remote_base::RemoteReceiverListener {
 public:
  ChannelListener() = default;
  ChannelListener(RFLinkUniversal *parent, uint8_t channel) : parent_(parent), channel_(channel) {}
  bool on_receive(remote_base::RemoteReceiveData data) override;
 private:
  RFLinkUniversal *parent_{nullptr};
  uint8_t channel_{0};
};

class RFLinkUniversal : public Component {
 public:
  struct Channel {
    FrameKind kind{FrameKind::RAW};
    remote_receiver::RemoteReceiverComponent *receiver{nullptr};
    uint32_t frequency_hz{0};
    std::string name;
    uint32_t frames{0};
    uint32_t noise_frames{0};
    uint32_t last_seen_ms{0};
    bool checked{false};
    bool available{false};
    bool rx_ready{false};
    bool tx_ready{false};
    bool has_rssi{false};
    float last_rssi_dbm{0.0f};
    bool has_lqi{false};
    uint8_t last_lqi{0};
    bool has_frequency_offset{false};
    float last_frequency_offset_hz{0.0f};
    // Human-readable board wiring copied from YAML. Diagnostic only; never drives GPIO.
    std::string cs_pin;
    std::string gdo0_pin;
    std::string gdo2_pin;
    cc1101::CC1101Component *cc1101{nullptr};
    bool self_test_done{false};
    bool self_test_spi_ok{false};
    uint8_t partnum{0xFF};
    uint8_t version{0xFF};
    // WATCHDOG1: digital SPI health telemetry. This detects failed/invalid
    // register reads; it cannot measure analog ringing, overshoot or reflections.
    uint32_t spi_checks{0};
    uint32_t spi_failures{0};
    uint8_t spi_consecutive_failures{0};
    uint32_t spi_recovery_attempts{0};
    uint32_t spi_recovery_successes{0};
    uint32_t spi_last_recovery_ms{0};
    bool spi_recovery_requested{false};
    bool spi_recovery_pending{false};
  };

  struct ActivityRecord {
    uint32_t timestamp_ms{0};
    std::string channel;
    uint8_t channel_index{0};
    std::string direction{"RX"};
    std::string protocol;
    std::string summary;
    std::string detail;
    std::string export_detail;
    bool noise_suspect{false};
    std::string noise_reason;
  };

  RFLinkUniversal();
  void setup() override;
  void loop() override; // RAMSES4: bounded SPI FIFO lab lease, disabled by default
  // Start the RFLink HTTP listener only AFTER ESPHome has initialized WiFi/LwIP.
  // Component's default DATA (600) runs before WIFI (250), and calling
  // WebServerBase::init() that early aborts in lwip sys_mutex_lock().
  // Match ESPHome's own web_server setup priority (WIFI - 1).
  float get_setup_priority() const override { return setup_priority::WIFI - 1.0f; }
  void dump_config() override;
  StatusLed &status_led() { return led_; }
  const StatusLed &status_led() const { return led_; }
  void led_wifi(bool online) { led_.wifi(online); }
  void led_ota(bool active) { led_.ota(active); }
  void led_pair_start() { led_.pair_start(millis()); }
  void led_pair_end() { led_.pair_end(millis()); }
  void led_tx() { led_.tx(millis()); }
  void set_led_settings(uint8_t brightness, bool quiet);
  void led_start_learning(uint8_t channel, bool baseline, const std::string &label);
  void led_finish_baseline();
  void led_stop_learning();

  void set_raw_channel(uint8_t index, remote_receiver::RemoteReceiverComponent *receiver,
                       uint32_t frequency_hz, const std::string &name);
  void set_packet_channel(uint8_t index, uint32_t frequency_hz, const std::string &name);
  void set_radio_available(uint8_t index, bool available, bool rx_ready, bool tx_ready);
  void set_radio_diagnostic_pins(uint8_t index, const std::string &cs_pin,
                                 const std::string &gdo0_pin, const std::string &gdo2_pin);
  void set_radio_cc1101(uint8_t index, cc1101::CC1101Component *radio);
  void run_radio_self_test(uint8_t index);
  void run_radio_watchdog(uint8_t index, bool driver_failed = false);
  bool take_radio_recovery_request(uint8_t index);
  bool radio_watchdog_healthy(uint8_t index) const;
  void record_tx_command(uint8_t index, const std::string &summary);
  // Physical radio TX diagnostics. Always logged for requested/denied/started/
  // reported-complete states; firmware/receiver success are NOT interchangeable.
  void record_radio_tx_event(uint8_t index, const std::string &protocol,
                            const std::string &summary, const std::string &uid,
                            const std::string &detail, const char *level = "info");
  void set_dedupe_window(uint32_t ms) { dedupe_window_ms_ = ms; }
  void set_candidate_threshold(uint16_t count) { devices_.set_candidate_threshold(count); }

  bool receive_raw(uint8_t channel, remote_base::RemoteReceiveData &data);
  void receive_packet(uint8_t channel, const std::vector<uint8_t> &packet,
                      float rssi_dbm, uint8_t lqi, float frequency_offset_hz);

  DeviceRecord *confirm_candidate(const std::string &identity, const std::string &name = "",
                                  const std::vector<std::string> *approved_actions = nullptr,
                                  const std::vector<std::string> *approved_fields = nullptr,
                                  const std::string &selected_profile = "");
  bool link_candidate(const std::string &identity, const std::string &uid);
  bool dismiss_candidate(const std::string &identity);
  bool rename_device(const std::string &uid, const std::string &name);
  bool remove_device(const std::string &uid);
  bool set_home_assistant_enabled(const std::string &uid, bool enabled);
  // Explicit user approval/removal. RF receives cannot mutate approved topology.
  bool change_capability(const std::string &uid, const std::string &kind,
                         const std::string &key, bool approve);
  bool set_transmit_enabled(const std::string &uid, bool enabled);
  void set_tx_controls_enabled(bool enabled) { tx_feature_enabled_=enabled; }
  bool tx_eligible(const std::string &uid) const;
  bool tx_action_ready(const std::string &uid,const std::string &action) const;
  bool native_transmit(const std::string &uid, const std::string &action, std::string &error);
  // Explicit bench-only non-control RAMSES self-announcement; no binding/actuation.
  bool ramses_tx_probe(std::string &error);
  // Web/API task must never execute GDO/SPI TX directly: enqueue a bounded
  // one-shot operation for the ESPHome loop. `true` means QUEUED, not SENT.
  bool enqueue_ramses_operation(const std::string &uid, bool auto_test, std::string &error);
  bool ramses_pair_start(const std::string &uid,std::string &error);
  bool ramses_pair_verify(const std::string &uid,std::string &error);
  bool ramses_pair_test(const std::string &uid,std::string &error);
  bool ramses_pair_test_sent(const std::string &uid) const {return uid==ramses_pair_uid_ && ramses_pair_test_sent_;}
  std::string ramses_pair_stage(const std::string &uid) const;
  bool ramses_pair_lab_ready() const {return ramses_tx_probe_enabled() && tx_feature_enabled_ &&
      (ramses_radio_lab_enabled_ || ramses_spi_lab_enabled_);}

  void set_ramses_tx_probe_lab(bool enabled) { ramses_tx_probe_lab_ = enabled; }
  bool ramses_tx_probe_enabled() const { return ramses_tx_probe_lab_ && gdo_serial_tx_lab_; }
  bool gdo_serial_lab_enabled() const { return gdo_serial_tx_lab_; }
  bool serial_tx_registered() const { return serial_tx_adapter_ != nullptr; }
  bool serial_tx_ready() const { return serial_tx_adapter_ && serial_tx_adapter_->ready(2,"fsk_86830_ramses_gdo_serial_lab"); }
  bool serial_tx_fault() const {
#ifdef USE_ESP32
    return gdo_serial_tx_hw_ && gdo_serial_tx_hw_->sticky_fault();
#else
    return false;
#endif
  }
  int radio_mode_868_state() const { return static_cast<int>(radio_mode_868_.state()); }
  void system_trace(const char *level,const char *subsystem,const std::string &uid,const std::string &message) {
    system_trace_.push(millis(),level,subsystem,uid.c_str(),message.c_str());
  }
  const SystemTrace &trace() const {return system_trace_;}
  SystemTrace &trace_mutable() {return system_trace_;}

  bool somfy_pair(const std::string &uid, std::string &error);
  bool somfy_status(const std::string &uid, uint32_t &address, uint16_t &next_rolling_code, bool &paired) const;
  bool restore_somfy_state(const std::string &uid, uint32_t address, uint16_t next_rolling_code, bool paired, std::string &error);
  bool itho_pair(const std::string &uid, bool compatibility, std::string &error);
  bool itho_unpair(const std::string &uid, std::string &error);
  bool itho_status(const std::string &uid, uint32_t &address, uint32_t &next_sequence,
                   uint8_t &device_type, bool &paired) const;
  bool restore_itho_state(const std::string &uid, uint32_t address, uint32_t next_sequence,
                          uint8_t device_type, bool paired, std::string &error);
  bool tx_controls_enabled() const { return tx_feature_enabled_; }
  bool transmit_request(const std::string &uid, const std::string &action, const std::string &session);
  void set_nexa_transmitter(remote_transmitter::RemoteTransmitterComponent *ptr) { raw_transmitters_[1] = ptr; } // Legacy TX1 YAML support
  void set_serial_tx_adapter(SerialTxAdapter *ptr) { serial_tx_adapter_ = ptr; }
  void set_gdo_serial_tx_lab(bool enable) { gdo_serial_tx_lab_ = enable; }
  void set_gdo_serial_tx_gpio(int pin) { gdo_serial_tx_gpio_ = pin; }
  void set_raw_transmitter(uint8_t radio, remote_transmitter::RemoteTransmitterComponent *ptr) { if (radio < 2) raw_transmitters_[radio] = ptr; }
#ifdef USE_ESP32
  void set_packet_transmitter(cc1101::CC1101Component *ptr) { packet_transmitter_ = ptr; }
#endif
  // RAMSES4: bench RX is opt-in; there is deliberately NO live TX or pairing API.
  void set_ramses_spi_lab(bool enabled) { ramses_spi_lab_enabled_ = enabled; }
  void set_ramses_spi_unsynced(bool enabled) { ramses_spi_unsynced_ = enabled; }
  void set_ramses_radio_lab(bool enabled) { ramses_radio_lab_enabled_ = enabled; }
  void set_decoder_auto_scan_lab(bool enabled) { decoder_auto_scan_lab_enabled_ = enabled; }
  void set_ramses_uart_gdo2_pin(int gpio) { ramses_uart_gdo2_pin_ = gpio; }
  bool request_decoder_rx_window(const std::string &protocol, uint32_t duration_ms);
  bool request_ramses_spi_lab_rx(uint32_t duration_ms); // backwards-compatible lab action
  const UserLibrary &user_library() const { return user_library_; }
  const UserLibrary &firmware_library() const { return firmware_library_; }
  uint32_t web_write_token() const { return web_write_token_; }
  const UserDecoder *effective_user_decoder(const std::string &id) const;
  bool library_ready() const { return library_ready_ && library_ok_; }
  bool save_user_decoder(UserDecoder decoder, std::string &error);
  bool enable_user_decoder(const std::string &id, bool enabled, std::string &error);
  bool remove_user_decoder(const std::string &id, std::string &error);
  bool save_user_profile(UserProfile profile, std::string &error);
  bool remove_user_profile(const std::string &id, std::string &error);
  bool restore_user_state(const std::string &key, uint32_t value, std::string &error);
  bool set_user_profile(const std::string &uid, const std::string &profile_id, std::string &error);
  bool approve_user_transmit(const std::string &uid, bool enabled, std::string &error);
  bool user_transmit(const std::string &uid, const std::string &action, std::string &error);
  bool preview_user_transmit(const std::string &id, const std::string &action,
                             user_vm::TxFrame &frame, std::string &error,
                             const std::string &uid = "") const;
  bool mqtt_connected() const;
  bool mqtt_configured() const;
  // Runtime MQTT broker settings are available ONLY when the optional ESPHome MQTT component was compiled.
  // Never return password via HTTP or diagnostics.
  bool mqtt_runtime_supported() const;
  bool mqtt_runtime_enabled() const;
  bool mqtt_runtime_has_password() const;
  std::string mqtt_runtime_host() const;
  std::string mqtt_runtime_user() const;
  uint16_t mqtt_runtime_port() const;
  bool mqtt_runtime_save(const std::string &host, uint16_t port, const std::string &user,
                         const std::string &password, bool enabled);

  void sync_home_assistant();
  void invalidate_tx_session() { ha_tx_session_.clear(); }
  void set_ha_node(const std::string &name) { ha_node_ = name; }
  bool unlink_identity(const std::string &uid, const std::string &identity);
  bool replace_identity(const std::string &uid, const std::string &previous_identity,
                        const std::string &candidate_identity);
  bool storage_ok() const { return storage_ok_; }
  // Language preference is device-wide and stored independently of GPIO/device NVS.
  const char *language() const;
  bool set_language(const std::string &language);
  bool storage_ready() const { return storage_ready_; }
  size_t decoder_count() const { return decoder_registry_size_(); }

  DeviceStore &devices() { return devices_; }
  DeveloperSession &developer() { return developer_; }
  uint32_t total_frames() const { return total_frames_; }
  uint32_t decoded_frames() const { return decoded_frames_; }
  uint32_t unknown_frames() const { return unknown_frames_; }
  uint32_t noise_frames() const { return noise_frames_; }
  // BOOTDIAG1: counters are per boot; flash history is updated once per boot.
  struct BootEvent { uint32_t boot_number; uint8_t reset_reason; uint8_t reserved[3]; };
  struct BootHistory {
    uint32_t marker;
    uint32_t boots;
    uint8_t count;
    uint8_t reserved[3];
    BootEvent events[8];  // newest first
  };
  uint32_t boot_session_id() const { return boot_session_id_; }
  uint32_t boot_count() const { return boot_history_.boots; }
  uint32_t reset_reason_code() const { return reset_reason_; }
  static const char *reset_reason_label(uint32_t reason);
  const BootHistory &boot_history() const { return boot_history_; }
  bool boot_history_persisted() const { return boot_history_persisted_; }
  const CrashTraceHistory &crash_trace_history() const { return crash_trace_history_; }
  bool crash_trace_persisted() const { return crash_trace_persisted_; }


  // Exposes existing per-channel listeners to ESPHome code generation (2026.10+).
  // Do not allocate additional listeners: these are initialized in our constructor.
  ChannelListener *get_raw_channel_listener(uint8_t index) {
    return index < MAX_CHANNELS ? &listeners_[index] : nullptr;
  }
  uint8_t channel_count() const { return channel_count_; }
  const Channel &channel(uint8_t index) const { return channels_[index]; }
  const std::vector<ActivityRecord> &activity() const { return activity_; }
  // RX history: three independent signal queues + ONE common noise queue.
  bool rx_log_ready() const { return rx_log_ != nullptr && rx_log_->ready(); }
  rflink_logging::Counters rx_log_counters() const;
  rflink_logging::Limits rx_log_limits() const;
  std::vector<rflink_logging::Entry> rx_log_read(uint64_t sequence, const rflink_logging::Filter &filter,
                                                size_t limit, bool ascending, bool include_json = true) const;
  bool rx_log_copy_json(uint64_t sequence, char *dst, size_t capacity, size_t &written) const;
  bool rx_log_set_limits(const rflink_logging::Limits &limits);
  uint32_t rx_log_busy_drops() const { return rx_log_busy_drops_; }


 protected:
  friend class ChannelListener;
  static constexpr uint8_t MAX_CHANNELS = 3;
  static constexpr uint8_t MIN_DECODER_CONFIDENCE = 60;


  bool save_devices_();
  bool save_user_library_();
  bool reserve_tx_state_(const std::string &uid, const tx_engine::Plan &plan, std::string &error);
  TxTransport native_route_for_(const tx_engine::Plan &plan) const;
  bool native_plan_available_(const tx_engine::Plan &plan) const;
  bool dispatch_native_plan_(const std::string &uid, const std::string &action,
                             const tx_engine::Plan &plan, const tx_engine::Encoder &encoder,
                             const std::string &origin, std::string &error);
  bool ensure_somfy_remote_(const std::string &uid, std::string &error);
  bool ensure_itho_remote_(const std::string &uid, std::string &error);
  bool ramses_send_management_(const rflink_ramses_ii::Frame &message,const std::string &uid,const char *stage,std::string &error);
  void ramses_pair_receive_ack_(const rflink_ramses_ii::Frame &frame,uint32_t now);
  void ramses_pair_poll_(uint32_t now);

  bool itho_management_transmit_(const std::string &uid, bool pair, bool compatibility,
                                 std::string &error);
  bool validate_user_decoder_activation_(const UserDecoder &candidate, const UserLibrary &staged, std::string &error) const;
  bool ha_discovery_(const DeviceRecord &device, bool remove, bool force = false);
  void ha_command_(const std::string &topic, const std::string &payload);
  bool ha_remove_capability_(const DeviceRecord &device, const std::string &kind, const std::string &key);
  void ha_received_(const DeviceRecord &device, const DecodeResult &result);

  static size_t decoder_registry_size_();
  uint32_t hash_raw_(const std::vector<int32_t> &raw) const;
  uint32_t hash_packet_(const std::vector<uint8_t> &packet) const;
  bool is_repeat_(uint8_t channel, uint32_t hash, uint32_t now);
  void process_frame_(const RFFrame &frame, uint32_t hash, uint16_t item_count);
  bool classify_noise_(const RFFrame &frame, uint8_t decoder_confidence, uint16_t item_count, std::string &reason) const;
  std::string full_frame_detail_(const RFFrame &frame, uint32_t hash, uint16_t item_count) const;
  void record_activity_(uint8_t channel, const std::string &protocol, const std::string &summary, uint32_t timestamp_ms,
                        const std::string &direction = "RX", const std::string &detail = "",
                        const std::string &export_detail = "", bool noise_suspect = false,
                        const std::string &noise_reason = "", const RFFrame *frame = nullptr,
                        const std::string &identity = "", uint8_t confidence = 0,
                        const std::string &level = "info");

  std::array<Channel, MAX_CHANNELS> channels_{};
  std::array<ChannelListener, MAX_CHANNELS> listeners_{};
  uint8_t channel_count_{0};

  uint32_t dedupe_window_ms_{80};
  uint32_t last_hash_{0};
  uint32_t last_hash_ms_{0};
  uint8_t last_hash_channel_{255};
  uint16_t last_repeat_count_{0};

  uint32_t total_frames_{0};
  uint32_t decoded_frames_{0};
  uint32_t unknown_frames_{0};
  uint32_t noise_frames_{0};
  DeviceStore devices_{};
  UserLibrary user_library_{};
  UserLibrary firmware_library_{};
  uint32_t web_write_token_{0};
  std::string ha_node_{"rflink-universal"};
  std::map<std::string, std::string> ha_discovery_cache_{};
  std::array<remote_transmitter::RemoteTransmitterComponent *, 2> raw_transmitters_{{nullptr,nullptr}};
#ifdef USE_ESP32
  cc1101::CC1101Component *packet_transmitter_{nullptr};
#endif
  SerialTxAdapter *serial_tx_adapter_{nullptr}; // no default driver; GDO TX denied until verified
  bool gdo_serial_tx_lab_{false};
  bool ramses_tx_probe_lab_{false};
  uint32_t last_ramses_probe_ms_{0};
  enum class RamsesPairPhase : uint8_t { IDLE, WAIT_ACK, CONFIRM_PENDING, AWAIT_USER, FAILED };
  // Single-slot mailbox (lock-free state machine). 0=free, 1=writing,
  // 2=ready, 3=executing. Release/acquire guarantees UID bytes are complete
  // before the loop consumes them. No heap allocation in the HTTP path.
  std::atomic<uint8_t> ramses_op_state_{0};
  std::array<char, 41> ramses_op_uid_{};
  uint8_t ramses_op_kind_{0}; // 1=1FC9 offer, 2=one-shot AUTO test
  void process_ramses_operation_();
  RamsesPairPhase ramses_pair_phase_{RamsesPairPhase::IDLE};
  std::string ramses_pair_uid_{};
  uint32_t ramses_pair_controller_{0},ramses_pair_fan_{0},ramses_pair_started_ms_{0};
  uint32_t ramses_pair_next_rx_ms_{0};
  bool ramses_pair_test_sent_{false};
  uint32_t ramses_pair_last_test_ms_{0};

  SystemTrace system_trace_{};
  int gdo_serial_tx_gpio_{-1};
#ifdef USE_ESP32
  std::unique_ptr<CC1101GdoUartTx> gdo_serial_tx_hw_{};
#endif
  RadioModeManager radio_mode_868_;
  RadioModeManager::State ramses_lab_last_radio_state_{RadioModeManager::State::ITHO};
  bool ramses_spi_lab_enabled_{false};
  bool ramses_radio_lab_enabled_{false};
  bool decoder_auto_scan_lab_enabled_{false};
  RadioScanScheduler decoder_scan_scheduler_{};
  bool ramses_uart_runtime_failed_{false}; // fallback only after verified Itho restore
  int ramses_uart_gdo2_pin_{-1};
  RxTransport ramses_active_transport_{RxTransport::NONE};
  bool ramses_spi_unsynced_{false};
#ifdef USE_ESP32
  std::unique_ptr<ramses_spi::CC1101SpiBackend> ramses_spi_backend_{};
  std::unique_ptr<CC1101UartBackend> ramses_uart_backend_{};
#endif
  std::string ha_tx_session_{};
  bool ha_tx_subscribed_{false};
  uint32_t tx_last_ms_{0};
  bool tx_sent_{false};
  bool tx_feature_enabled_{false};

  DeveloperSession developer_{};
  StatusLed led_{};
#ifdef USE_ESP32
  struct LedPreferences { uint8_t brightness; uint8_t quiet; uint16_t marker; };
  struct LanguagePreferences { uint8_t code; uint8_t reserved; uint16_t marker; };
  // 352 bytes in NVS, saved only on explicit user action; not encrypted by default ESPHome NVS.
  struct MqttPrefs {
    uint32_t marker{0};
    uint16_t port{1883};
    uint8_t enabled{0};
    uint8_t reserved{0};
    char host[128]{};
    char user[64]{};
    char password[128]{};
  };
  MqttPrefs mqtt_prefs_{};
  bool mqtt_prefs_loaded_{false};
  ESPPreferenceObject mqtt_settings_pref_{};

  ESPPreferenceObject boot_history_pref_{};
  ESPPreferenceObject crash_trace_pref_{};
  ESPPreferenceObject language_pref_{};
  ESPPreferenceObject led_pref_{};
  ESPPreferenceObject device_pref_a_{};
  ESPPreferenceObject device_pref_b_{};
  ESPPreferenceObject library_pref_a_{};
  ESPPreferenceObject library_pref_b_{};
  uint32_t device_generation_{0};
  bool active_device_bank_b_{false};
  uint32_t library_generation_{0};
  bool active_library_bank_b_{false};
#endif
  BootHistory boot_history_{};
  uint32_t boot_session_id_{0};
  uint32_t reset_reason_{0};
  bool boot_history_persisted_{false};
  CrashTraceHistory crash_trace_history_{};
  CrashTraceCollector crash_trace_collector_{};
  bool crash_trace_persisted_{false};
  uint8_t language_code_{0};  // 0=en, 1=nl, 2=fr, 3=de.
  bool storage_ready_{false};
  bool storage_ok_{false};
  bool library_ready_{false};
  bool library_ok_{false};
  std::vector<ActivityRecord> activity_{};  // legacy 32 recent entries, preserves TX/status consumers.
  std::unique_ptr<rflink_logging::Store> rx_log_{};
  void *rx_log_arena_{nullptr};  // lifetime: device/firmware, allocated exclusively from PSRAM.
  uint32_t rx_log_busy_drops_{0};
#if defined(USE_ESP32) && defined(ESP_PLATFORM)
  SemaphoreHandle_t rx_log_mutex_{nullptr};
#endif
#ifdef USE_RFLINK_UNIVERSAL_WEB
  RFLinkWebHandler *web_handler_{nullptr};
#endif
};

}  // namespace rflink_universal
}  // namespace esphome
