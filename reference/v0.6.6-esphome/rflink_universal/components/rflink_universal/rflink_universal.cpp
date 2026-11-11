#include "universal_tx_route.h"
#include "rflink_universal.h"
#include <cctype>
#include <cstring>
#include "esphome/core/version.h"

#include <algorithm>
#include <cstdio>
#include <inttypes.h>
#include <cstdlib>
#include <memory>

#include "decoder_registry.h"
#include "decoder_dispatch_v2.h"
#include "radio_profile_plan_v2.h"
#include "radio_v2_catalog.h"
#include "user_radio_v2_adapter.h"
#include "capability_catalog.h"
#include "profile_catalog.h"
#include "profile_suggestions.h"
#include "ha_discovery.h"
#include "tx_policy.h"
#include "tx_engine.h"
#include "format_utils.h"
#include "firmware_user_recipes.h"
#include "firmware_identity.h"
#include "rflink_log_nvs_espidf.h"
// Optional logger bridge: ESPHome owns the .noinit panic record. We only
// observe its public diagnostic output; no private symbols or panic hooks.
#if defined(USE_ESP32) && defined(USE_ESP32_CRASH_HANDLER) && defined(USE_LOG_LISTENERS)
#include "esphome/components/esp32/crash_handler.h"
#include "esphome/components/logger/logger.h"
#endif

#include <new>
#ifdef USE_ESP32
#include <esp_random.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <esp_heap_caps.h>
#if defined(ESP_PLATFORM)
#include <esp_http_server.h>
#endif
#endif
#ifdef USE_ESP32
#include "esphome/components/cc1101/cc1101.h"
#endif
#ifdef USE_MQTT
#include "esphome/components/mqtt/mqtt_client.h"
#endif
#if defined(__has_include)
#  if __has_include("esphome/core/defines.h")
#    include "esphome/core/defines.h"
#  endif
#endif
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#ifdef USE_RFLINK_UNIVERSAL_WEB
#include "esphome/components/web_server_base/web_server_base.h"
#endif

namespace esphome {
namespace rflink_universal {

static const char *const TAG = "rflink_universal";

// Keep reset labels stable for the web UI; the numeric reason is also exported.
const char *RFLinkUniversal::reset_reason_label(uint32_t reason) {
#ifdef USE_ESP32
  switch (static_cast<esp_reset_reason_t>(reason)) {
    case ESP_RST_POWERON: return "power_on";
    case ESP_RST_EXT: return "external_reset";
    case ESP_RST_SW: return "software";
    case ESP_RST_PANIC: return "panic";
    case ESP_RST_INT_WDT: return "interrupt_watchdog";
    case ESP_RST_TASK_WDT: return "task_watchdog";
    case ESP_RST_WDT: return "watchdog";
    case ESP_RST_DEEPSLEEP: return "deep_sleep";
    case ESP_RST_BROWNOUT: return "brownout";
    case ESP_RST_SDIO: return "sdio";
    default: break;
  }
#endif
  (void) reason;
  return "unknown";
}


RFLinkUniversal::RFLinkUniversal() {
  activity_.reserve(32);
  for (uint8_t i = 0; i < MAX_CHANNELS; i++) listeners_[i] = ChannelListener(this, i);
}

bool ChannelListener::on_receive(remote_base::RemoteReceiveData data) {
  return parent_ != nullptr && parent_->receive_raw(channel_, data);
}

void RFLinkUniversal::set_raw_channel(uint8_t index, remote_receiver::RemoteReceiverComponent *receiver,
                                      uint32_t frequency_hz, const std::string &name) {
  if (index >= MAX_CHANNELS) return;
  channels_[index].kind = FrameKind::RAW;
  channels_[index].receiver = receiver;
  channels_[index].frequency_hz = frequency_hz;
  channels_[index].name = name;
  channel_count_ = std::max<uint8_t>(channel_count_, index + 1);
}

void RFLinkUniversal::set_packet_channel(uint8_t index, uint32_t frequency_hz, const std::string &name) {
  if (index >= MAX_CHANNELS) return;
  channels_[index].kind = FrameKind::PACKET;
  channels_[index].receiver = nullptr;
  channels_[index].frequency_hz = frequency_hz;
  channels_[index].name = name;
  channel_count_ = std::max<uint8_t>(channel_count_, index + 1);
}

void RFLinkUniversal::loop() {
  const uint32_t now=millis();
#ifdef USE_ESP32
  // Time-sharing is opt-in and bounded by each decoder's manifest declaration.
  // The scheduler never decides whether a decoder needs SPI or UART: the
  // transport policy does. Itho remains the primary radio owner.
  if(decoder_auto_scan_lab_enabled_ && ramses_radio_lab_enabled_ &&
     ramses_pair_phase_!=RamsesPairPhase::WAIT_ACK && ramses_pair_phase_!=RamsesPairPhase::CONFIRM_PENDING) {
    const auto policy=radio_catalog::policy();
    const auto *due=decoder_scan_scheduler_.due(policy,now,radio_mode_868_.itho_ready(),true);
    if(due && due->radio_index==2 &&
       request_decoder_rx_window(due->protocol,due->scan_window_ms))
      decoder_scan_scheduler_.mark(policy,due->protocol,now);
  }
#endif
  radio_mode_868_.poll(now);
  process_ramses_operation_(); // bounded mailbox, never execute RF from web task
  // TX in process_ramses_operation_() is blocking: refresh the clock before
  // testing a pairing timestamp that may have been created during this loop.
  ramses_pair_poll_(millis());
#ifdef USE_ESP32
  const auto current_radio_state=radio_mode_868_.state();
  const ramses_spi::Stats *diag=nullptr;
  if(ramses_active_transport_==RxTransport::UART_STREAM && ramses_uart_backend_)
    diag=&ramses_uart_backend_->stats();
  else if(ramses_spi_backend_) diag=&ramses_spi_backend_->stats();
  if(current_radio_state!=ramses_lab_last_radio_state_ && diag &&
     (current_radio_state==RadioModeManager::State::ITHO ||
      current_radio_state==RadioModeManager::State::FAULT)) {
    ESP_LOGI(TAG,"RF transport %s: %s, valid=%" PRIu32 " bytes=%" PRIu32
                 " polls=%" PRIu32 " overflow=%" PRIu32 " invalid=%" PRIu32
                 " io_errors=%" PRIu32,
             transport_name(ramses_active_transport_),
             current_radio_state==RadioModeManager::State::ITHO ? "Itho restored" : "RADIO FAULT",
             diag->rx_frames,diag->rx_bytes,diag->rx_polls,diag->rx_overflows,
             diag->rx_invalid,diag->io_errors);
  }
  // Only a verified return to the Itho profile can mark UART unavailable.
  // A no-packets window is NOT a UART failure and never triggers fallback.
  if(current_radio_state==RadioModeManager::State::ITHO &&
     ramses_lab_last_radio_state_!=current_radio_state &&
     ramses_active_transport_==RxTransport::UART_STREAM &&
     radio_mode_868_.last_error()==RadioModeManager::Error::RAMSES_START_FAILED) {
    ramses_uart_runtime_failed_=true;
    ESP_LOGW(TAG,"RAMSES UART init failed and Itho restored; manifest SPI fallback available for next request");
  }
  if(current_radio_state!=ramses_lab_last_radio_state_){
    system_trace(current_radio_state==RadioModeManager::State::FAULT?"error":"info","radio","",
      std::string("868 radio mode changed to ")+std::to_string(static_cast<int>(current_radio_state))+
      " (0=Itho, 1=acquiring, 2=RAMSES RX, 3=restoring, 4=fault), error="+
      std::to_string(static_cast<int>(radio_mode_868_.last_error()))+
      " (0=none, 4=pause, 5=RX start, 6=RX stop, 7=restore, 8=verify)");
  }
  ramses_lab_last_radio_state_=current_radio_state;
  if(diag && radio_mode_868_.ramses_listening()) {
    rflink_ramses_ii::Frame decoded;
    bool recognized=false;
    if(ramses_active_transport_==RxTransport::UART_STREAM && ramses_uart_backend_)
      recognized=ramses_uart_backend_->poll(now,decoded);
    else if(ramses_spi_backend_) recognized=ramses_spi_backend_->poll(now,decoded);
    if(recognized) {
      ramses_pair_receive_ack_(decoded,now);
      std::vector<uint8_t> packet;
      size_t bits=0;
      if(rflink_ramses_ii::tx::encodeOnAir(decoded,5,packet,bits)) {
        RFFrame frame;
        frame.kind=FrameKind::PACKET;
        frame.channel=2;
        frame.frequency_hz=868299900;
        frame.received_ms=now;
        frame.packet=&packet;
        frame.has_rssi=false;
        frame.has_lqi=false;
        process_frame_(frame,hash_packet_(packet),uint16_t(packet.size()));
      }
    }
    if((ramses_active_transport_==RxTransport::UART_STREAM && ramses_uart_backend_ && ramses_uart_backend_->error()) ||
       (ramses_active_transport_!=RxTransport::UART_STREAM && ramses_spi_backend_ && ramses_spi_backend_->error()))
      radio_mode_868_.request_restore();
  }
#endif
}

bool RFLinkUniversal::request_decoder_rx_window(const std::string &protocol,uint32_t duration_ms) {
#ifdef USE_ESP32
  const bool enabled=ramses_radio_lab_enabled_ || ramses_spi_lab_enabled_;
  if(!packet_transmitter_ || packet_transmitter_->is_failed())return false;
  uint32_t supported=0;
  if(ramses_radio_lab_enabled_ && ramses_uart_gdo2_pin_>=0 && !ramses_uart_runtime_failed_)
    supported|=RadioTransportPolicy::bit(RxTransport::UART_STREAM);
  if(enabled) {
    supported|=RadioTransportPolicy::bit(RxTransport::SPI_FIFO_SYNC);
    supported|=RadioTransportPolicy::bit(RxTransport::SPI_FIFO_RAW);
  }
  auto choice=radio_catalog::policy().choose(protocol.c_str(),2,supported,
           radio_mode_868_.busy(),enabled,duration_ms);
  if(choice.decision!=RadioTransportPolicy::Decision::SELECTED)return false;
  // Switch the adapter only while Itho owns the radio and has been verified.
  // On a fault the manager refuses disarm(), so stale or corrupt radio state
  // can never be bypassed by requesting another transport.
  if(ramses_active_transport_!=choice.transport || !radio_mode_868_.hardware_adapter_present()) {
    if(radio_mode_868_.hardware_adapter_present() && !radio_mode_868_.disarm())return false;
    ramses_uart_backend_.reset();ramses_spi_backend_.reset();
    if(choice.transport==RxTransport::UART_STREAM) {
      ramses_uart_backend_.reset(new CC1101UartBackend(*packet_transmitter_,ramses_uart_gdo2_pin_));
      if(!radio_mode_868_.arm(ramses_uart_backend_.get())) {ramses_uart_backend_.reset();return false;}
    } else if(choice.transport==RxTransport::SPI_FIFO_SYNC || choice.transport==RxTransport::SPI_FIFO_RAW) {
      ramses_spi_backend_.reset(new ramses_spi::CC1101SpiBackend(*packet_transmitter_,
                                  choice.transport==RxTransport::SPI_FIFO_RAW ||
                                  (!ramses_radio_lab_enabled_ && ramses_spi_unsynced_)));
      if(!radio_mode_868_.arm(ramses_spi_backend_.get())) {ramses_spi_backend_.reset();return false;}
    } else return false; // packet mode already owned by the normal Itho driver
    ramses_active_transport_=choice.transport;
  }
  const bool granted=radio_mode_868_.request_ramses_rx(millis(),duration_ms);
  system_trace(granted?"info":"warn","radio","",granted?"RAMSES receive lease requested":"RAMSES receive lease rejected (radio busy or unavailable)");
  return granted;
#else
  (void)protocol;(void)duration_ms;return false;
#endif
}

bool RFLinkUniversal::request_ramses_spi_lab_rx(uint32_t duration_ms) {
  return request_decoder_rx_window("RamsesII",duration_ms);
}

void RFLinkUniversal::setup() {
  led_.boot(millis());
  system_trace("info","boot","","RFLink started; system trace records decisions (not RF noise)");
#ifdef USE_ESP32
  // No UART/GPIO/radio ownership until explicitly enabled in YAML; RAMSES
  // manifest and capability gate remain TX disabled regardless of this lab.
  if (gdo_serial_tx_lab_ && packet_transmitter_ && gdo_serial_tx_gpio_ >= 0) {
    const GdoUartPreset modem={"fsk_86830_ramses_gdo_serial_lab", 38400,
                               0x30, 0x5A, 0x83, 0x00, 0x40};
    gdo_serial_tx_hw_.reset(new CC1101GdoUartTx(*packet_transmitter_,
        radio_mode_868_, 2, gdo_serial_tx_gpio_, modem, true));
    gdo_serial_tx_hw_->set_trace_observer([](void *ctx,const char *phase,bool ok){
      auto *owner=static_cast<RFLinkUniversal *>(ctx);
      const char *detail=(!ok && owner->gdo_serial_tx_hw_)
           ?owner->gdo_serial_tx_hw_->last_failure():"";
      const std::string stage=std::string(phase)+(ok?" OK":" FAILED");
      owner->system_trace(ok?"info":"error","radio","",stage);
      // The radio log must show hardware attempts AND failures, not only a
      // transient browser toast. No secret or raw RF bytes are recorded.
      owner->record_radio_tx_event(2,"RamsesII","TX hardware: "+stage,"",
          (!ok && detail && *detail)?detail:"GDO/UART 868.30 MHz physical TX adapter",
          ok?"info":"error");
    },this);
    this->set_serial_tx_adapter(gdo_serial_tx_hw_.get());
    system_trace("warn","radio","","GDO UART lab driver registered; normal protocol TX remains gated");
    ESP_LOGW(TAG, "Experimental CC1101 GDO serial TX adapter registered; protocol TX permissions still apply");
  }
#endif
#ifdef USE_ESP32
  reset_reason_ = static_cast<uint32_t>(esp_reset_reason());
  boot_session_id_ = esp_random();
  if (boot_session_id_ == 0) boot_session_id_ = 1;
  web_write_token_ = esp_random();
  if (web_write_token_ == 0) web_write_token_ = esp_random() | 1u;
#else
  web_write_token_ = 0x52464C4Bu;
#endif
  if (!load_firmware_user_recipes(firmware_library_)) {
    ESP_LOGE(TAG, "Compiled user recipe failed validation; firmware recipe RX/TX disabled");
    firmware_library_ = UserLibrary{};
  }
#ifdef USE_ESP32
  if (global_preferences != nullptr) {
    // Dedicated NVS key, independent of device profiles, radio config and language.
    // Exactly one small flash record is committed per boot, NOT per RX packet.
    boot_history_pref_ = global_preferences->make_preference<BootHistory>(0x52464231u);
    BootHistory stored_boot_history{};
    if (boot_history_pref_.load(&stored_boot_history) && stored_boot_history.marker == 0x52464231u &&
        stored_boot_history.count <= 8 && stored_boot_history.boots >= stored_boot_history.count) {
      boot_history_ = stored_boot_history;
    }
    boot_history_.marker = 0x52464231u;
    if (boot_history_.boots == UINT32_MAX) {
      // Saturate rather than reusing a previous boot number.
      boot_history_.boots = UINT32_MAX;
    } else {
      ++boot_history_.boots;
    }
    for (size_t i = 7; i > 0; --i) boot_history_.events[i] = boot_history_.events[i - 1];
    boot_history_.events[0] = BootEvent{boot_history_.boots, static_cast<uint8_t>(reset_reason_), {0, 0, 0}};
    if (boot_history_.count < 8) ++boot_history_.count;
    if (boot_history_pref_.save(&boot_history_)) {
      boot_history_persisted_ = global_preferences->sync();
    }
    if (!boot_history_persisted_) {
      ESP_LOGW(TAG, "Boot history could not be committed to NVS; counters may reset after power loss");
    }
    // Unique NVS key. Persist only when the user explicitly saves settings.
    led_pref_ = global_preferences->make_preference<LedPreferences>(0x52464C33u);
    LedPreferences stored{20, 0, 0};
    if (led_pref_.load(&stored) && stored.marker == 0x4C45 && stored.brightness >= 1 && stored.brightness <= 100 && stored.quiet <= 1) {
      led_.settings(stored.brightness, stored.quiet != 0);
    }
    // Separate key: upgrading language support never rewrites existing records.
    language_pref_ = global_preferences->make_preference<LanguagePreferences>(0x52464C47u);
    LanguagePreferences language_stored{0, 0, 0};
    if (language_pref_.load(&language_stored) && language_stored.marker == 0x4C47 && language_stored.code < 4)
      language_code_ = language_stored.code;
    // MQTT option is compiled separately; load stored broker profile before MQTT setup (AFTER_WIFI).
    mqtt_settings_pref_ = global_preferences->make_preference<MqttPrefs>(0x52464D34u);
    MqttPrefs saved_mqtt{};
    if (mqtt_settings_pref_.load(&saved_mqtt) && saved_mqtt.marker == 0x52464D34u &&
        saved_mqtt.port > 0 && saved_mqtt.enabled <= 1 &&
        memchr(saved_mqtt.host, 0, sizeof(saved_mqtt.host)) &&
        memchr(saved_mqtt.user, 0, sizeof(saved_mqtt.user)) &&
        memchr(saved_mqtt.password, 0, sizeof(saved_mqtt.password))) {
      mqtt_prefs_ = saved_mqtt;
      mqtt_prefs_loaded_ = true;
#ifdef USE_MQTT
      if (mqtt::global_mqtt_client != nullptr) {
        auto *client = mqtt::global_mqtt_client;
        client->set_broker_address(mqtt_prefs_.host);
        client->set_broker_port(mqtt_prefs_.port);
        client->set_username(mqtt_prefs_.user);
        client->set_password(mqtt_prefs_.password);
        client->set_enable_on_boot(mqtt_prefs_.enabled != 0);
      }
#endif
    }
    device_pref_a_ = global_preferences->make_preference<DevicePersistence::Blob>(0x52464441u);
    device_pref_b_ = global_preferences->make_preference<DevicePersistence::Blob>(0x52464442u);
    auto a = std::make_unique<DevicePersistence::Blob>();
    auto b = std::make_unique<DevicePersistence::Blob>();
    const bool read_a = device_pref_a_.load(a.get());
    const bool read_b = device_pref_b_.load(b.get());
    const bool ok_a = read_a && DevicePersistence::valid(*a);
    const bool ok_b = read_b && DevicePersistence::valid(*b);
    storage_ready_ = true;
    storage_ok_ = true;
    if (ok_a || ok_b) {
      active_device_bank_b_ = ok_b && (!ok_a || b->generation > a->generation);
      const auto &latest = active_device_bank_b_ ? *b : *a;
      if (!DevicePersistence::decode(latest, devices_)) {
        // Try the older, complete snapshot before giving up. Never erase NVS here.
        const auto &older = active_device_bank_b_ ? *a : *b;
        if (!(active_device_bank_b_ ? ok_a : ok_b) || !DevicePersistence::decode(older, devices_)) {
          storage_ok_ = false;
          ESP_LOGE(TAG, "Device storage corrupt; no automatic overwrite of saved records");
        } else {
          active_device_bank_b_ = !active_device_bank_b_;
          device_generation_ = older.generation;
        }
      } else {
        device_generation_ = latest.generation;
      }
      if (storage_ok_) ESP_LOGI(TAG, "Restored %u devices from NVS generation %" PRIu32, static_cast<unsigned>(devices_.devices().size()), device_generation_);
    } else if (read_a || read_b) {
      storage_ok_ = false;
      ESP_LOGE(TAG, "Both stored device snapshots are invalid; blocking changes to prevent data loss");
    } else {
      ESP_LOGI(TAG, "No stored devices found; starting with empty device list");
    }
    library_pref_a_ = global_preferences->make_preference<UserLibraryPersistence::Blob>(0x52464C41u);
    library_pref_b_ = global_preferences->make_preference<UserLibraryPersistence::Blob>(0x52464C42u);
    auto la = std::make_unique<UserLibraryPersistence::Blob>();
    auto lb = std::make_unique<UserLibraryPersistence::Blob>();
    const bool read_la = library_pref_a_.load(la.get());
    const bool read_lb = library_pref_b_.load(lb.get());
    const bool ok_la = read_la && UserLibraryPersistence::valid(*la);
    const bool ok_lb = read_lb && UserLibraryPersistence::valid(*lb);
    library_ready_ = true;
    library_ok_ = true;
    if (ok_la || ok_lb) {
      active_library_bank_b_ = ok_lb && (!ok_la || lb->generation > la->generation);
      const auto &newer = active_library_bank_b_ ? *lb : *la;
      if (!UserLibraryPersistence::decode(newer, user_library_)) {
        const auto &older = active_library_bank_b_ ? *la : *lb;
        if (!(active_library_bank_b_ ? ok_la : ok_lb) || !UserLibraryPersistence::decode(older, user_library_)) {
          library_ok_ = false;
          ESP_LOGE(TAG, "User library corrupt; writes blocked to preserve stored data");
        } else {
          active_library_bank_b_ = !active_library_bank_b_;
          library_generation_ = older.generation;
        }
      } else library_generation_ = newer.generation;
    } else if (read_la || read_lb) {
      library_ok_ = false;
      ESP_LOGE(TAG, "Both user library snapshots invalid; writes blocked");
    }
  }
#endif
#ifndef USE_ESP32
  library_ready_ = true;
  library_ok_ = true;
#endif
  if (boot_history_.count == 0) {
    boot_history_.boots = 1;
    boot_history_.count = 1;
    boot_history_.events[0] = BootEvent{1, static_cast<uint8_t>(reset_reason_), {0, 0, 0}};
  }
  // CRASHTRACE: keep up to four *technical* crash reports in a dedicated NVS
  // record. The existing eight-entry reset history stays unchanged.
#ifdef USE_ESP32
  if (global_preferences) {
    crash_trace_pref_ = global_preferences->make_preference<CrashTraceHistory>(CrashTraceHistory::MARKER);
    CrashTraceHistory saved{};
    if (crash_trace_pref_.load(&saved) && saved.marker == CrashTraceHistory::MARKER &&
        saved.version == 1 && saved.count <= 4) {
      crash_trace_history_ = saved;
      crash_trace_persisted_ = true;
    }
  }
#endif
#if defined(USE_ESP32) && defined(USE_ESP32_CRASH_HANDLER) && defined(USE_LOG_LISTENERS)
  // The Logger and crash recorder are owned by ESPHome. Register the observer
  // before asking the public crash handler to replay its available report.
  // Callback only copies bounded text; NO NVS, JSON or logging from callback.
  if (logger::global_logger && esp32::crash_handler_has_data()) {
    crash_trace_collector_.reset();
    logger::global_logger->add_log_callback(this, [](void *instance, uint8_t, const char *tag,
                                                       const char *message, size_t size) {
      static_cast<RFLinkUniversal *>(instance)->crash_trace_collector_.ingest(tag, message, size);
    });
    esp32::crash_handler_log();
    if (crash_trace_collector_.complete()) {
      CrashTraceRecord new_record = crash_trace_collector_.report();
      // ESPHome explicitly marks old-build addresses as foreign. Such an
      // old record may survive an OTA reboot; do not assign it to this boot.
      if (!new_record.foreign_build) {
        const bool crash_reset = reset_reason_ == ESP_RST_PANIC || reset_reason_ == ESP_RST_INT_WDT ||
                                 reset_reason_ == ESP_RST_TASK_WDT || reset_reason_ == ESP_RST_WDT;
        if (crash_reset) new_record.boot_number = boot_history_.boots;
        std::snprintf(new_record.source_hash, sizeof(new_record.source_hash), "%s", RFLINK_FIRMWARE_BUILD_HASH);
      }
      const bool duplicate = crash_trace_history_.count > 0 &&
          crash_trace_history_.records[0].boot_number == new_record.boot_number &&
          crash_trace_history_.records[0].pc == new_record.pc &&
          crash_trace_history_.records[0].backtrace_count == new_record.backtrace_count &&
          crash_trace_history_.records[0].foreign_build == new_record.foreign_build &&
          std::memcmp(crash_trace_history_.records[0].backtrace, new_record.backtrace,
                      sizeof(new_record.backtrace)) == 0;
      if (!duplicate) {
        for (size_t i = 3; i > 0; --i) crash_trace_history_.records[i] = crash_trace_history_.records[i - 1];
        crash_trace_history_.records[0] = new_record;
        if (crash_trace_history_.count < 4) ++crash_trace_history_.count;
        if (global_preferences && crash_trace_pref_.save(&crash_trace_history_))
          crash_trace_persisted_ = global_preferences->sync();
        else crash_trace_persisted_ = false;
      }
      char report_line[140]{};
      snprintf(report_line, sizeof(report_line), "Crash trace saved: PC=0x%08" PRIX32 " core=%u frames=%u %s",
               new_record.pc, unsigned(new_record.core), unsigned(new_record.backtrace_count),
               new_record.foreign_build ? "other build" : "same build");
      system_trace("error", "boot", "", report_line);
    } else {
      system_trace("warn", "boot", "", "ESPHome crash report exists but raw backtrace capture incomplete");
    }
  }
#endif
  // DIAG7: append one bounded boot marker to the existing system log.
  // This records the PREVIOUS reset reason; it is not an ESP-IDF stack trace.
  // Flash boot history is already persisted separately, once per boot.
  {
    char boot_message[142]{};
    const char *reason = reset_reason_label(reset_reason_);
    snprintf(boot_message, sizeof(boot_message), "Boot #%" PRIu32 " after %s; previous reset reason only", boot_history_.boots, reason);
    const bool crashed = (strcmp(reason, "panic") == 0 || strcmp(reason, "task_watchdog") == 0 ||
                          strcmp(reason, "interrupt_watchdog") == 0 || strcmp(reason, "watchdog") == 0 ||
                          strcmp(reason, "brownout") == 0);
    system_trace(crashed ? "error" : "info", "boot", "", boot_message);
  }
  ESP_LOGI(TAG, "Boot #%" PRIu32 " reset_reason=%s (%" PRIu32 ") NVS=%s",
           boot_history_.boots, reset_reason_label(reset_reason_), reset_reason_,
           boot_history_persisted_ ? "saved" : "unavailable");
  ESP_LOGI(TAG, "Starting RFLink Universal %s [%s]", RFLINK_FIRMWARE_VERSION, RFLINK_FIRMWARE_REVISION);
  for (uint8_t i = 0; i < channel_count_; i++) {
    if (channels_[i].kind == FrameKind::RAW) {
      if (channels_[i].receiver == nullptr) {
        ESP_LOGE(TAG, "Raw channel %u has no remote_receiver", i);
        led_.fault(true);
        continue;
      }
      // ESPHome 2026.10+ registers this listener from to_code(), where the
      // slot is counted. Older ESPHome keeps the runtime registration path.
#if ESPHOME_VERSION_CODE < VERSION_CODE(2026, 10, 0)
      channels_[i].receiver->register_listener(&listeners_[i]);
#endif
    }
    ESP_LOGI(TAG, "Channel %u: %s @ %.4f MHz (%s)", i, channels_[i].name.c_str(),
             channels_[i].frequency_hz / 1000000.0f,
             channels_[i].kind == FrameKind::RAW ? "raw" : "packet");
  }
  // RXLOG1: keep logging optional; do NOT compromise OTA or WiFi if PSRAM is unavailable.
#if defined(USE_ESP32) && defined(ESP_PLATFORM)
  rx_log_mutex_ = xSemaphoreCreateMutex();
  if (rx_log_mutex_) {
    for (size_t arena_bytes : {rflink_logging::kDefaultArenaBytes, size_t(1024 * 1024), size_t(512 * 1024)}) {
      rx_log_arena_ = heap_caps_malloc(arena_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
      if (!rx_log_arena_) continue;
      auto candidate = std::unique_ptr<rflink_logging::Store>(new (std::nothrow) rflink_logging::Store(rx_log_arena_, arena_bytes));
      if (candidate && candidate->ready()) {
        rx_log_ = std::move(candidate);
        rflink_logging::load_limits_from_nvs(*rx_log_); // defaults on first install.
        ESP_LOGI(TAG, "RXLOG1: arena %u bytes PSRAM, three signal queues + shared noise queue",
                 unsigned(arena_bytes));
        break;
      }
      heap_caps_free(rx_log_arena_);
      rx_log_arena_ = nullptr;
    }
  }
  if (!rx_log_) ESP_LOGW(TAG, "RXLOG1: PSRAM unavailable; RX history disabled (radio, web, OTA continue)");
#endif
  ensure_decoder_packs_linked();  // Keep dynamic decoder packs in ESP-IDF static libraries.
  tx_engine::ensure_linked(); // Missing encoder anchor fails the firmware link, never silently enables TX.
  const auto registered_count = decoder_registry().size();
  // Explain compatible capture groups at boot; never switch modem settings
  // automatically merely because two decoders mention the same frequency.
  {
    size_t route_count=0;
    const auto *routes=radio_v2_catalog::specs(route_count);
    const RadioProfilePlanV2 plan(routes,route_count);
    for(uint8_t channel=0; channel<3; ++channel) {
      const auto groups=plan.groups_on(channel);
      ESP_LOGI(TAG,"Radio Engine V2 channel %u: %u configured capture group(s)%s",
               unsigned(channel),unsigned(groups),
               plan.can_listen_without_switch(channel) ? " (compatible)" :
                   " (distinct modes: switching requires separate validation)");
    }
  }

#ifdef RFLINK_DECODER_EXPECTED_COUNT
  const unsigned expected_count = RFLINK_DECODER_EXPECTED_COUNT;
  ESP_LOGI(TAG, "Decoder registration: %u/%u enabled decoders", static_cast<unsigned>(registered_count), expected_count);
  if (registered_count != expected_count) {
    ESP_LOGE(TAG, "Decoder registration mismatch: expected %u, found %u. Check ESPHome build sources and imported files.",
             expected_count, static_cast<unsigned>(registered_count));
    // Preserve web / Wi-Fi / OTA even when the decoder library is incomplete.
  }
#else
  ESP_LOGI(TAG, "%u decoder(s) registered (build expectation unavailable)", static_cast<unsigned>(registered_count));
#endif
#ifdef USE_RFLINK_UNIVERSAL_WEB
  auto *base = web_server_base::global_web_server_base;
  if (base != nullptr) {
    this->web_handler_ = new RFLinkWebHandler(this);  // lifetime = firmware lifetime
    base->add_handler(this->web_handler_);
    // RFLINK_BOOT_ORDER_FIX_20260918: get_setup_priority() guarantees that
    // ESPHome WiFi has initialized the LwIP mutex before this opens a socket.
    base->init();
    ESP_LOGI(TAG, "Lightweight web UI enabled at /rflink");
  } else {
    ESP_LOGW(TAG, "web_server_base unavailable; RFLink web UI disabled");
  }
#endif
}

// RFLINK_OPTIONAL_RADIOS_20260918
// Legacy diagnostic contract: "not detected (RF disabled; network services remain active)"
// Missing radio hardware is a channel fault, not a firmware fault. Keep web,
// Wi-Fi, API and OTA alive and never process input from an absent module.
void RFLinkUniversal::set_radio_available(uint8_t index, bool available, bool rx_ready, bool tx_ready) {
  if (index >= channel_count_) return;
  auto &channel = channels_[index];
  if (!channel.checked || channel.available != available) {
    if (available) {
      ESP_LOGI(TAG, "Radio %s: CC1101 SPI init OK (RX=%s, TX=%s)", channel.name.c_str(),
               rx_ready ? "ready" : "FAILED", tx_ready ? "ready" : "FAILED");
    } else {
      ESP_LOGE(TAG, "Radio %s: not detected (RF disabled; network services remain active); CC1101 init FAILED; check 3.3V/GND, CS=%s and shared SPI wiring.",
               channel.name.c_str(), channel.cs_pin.empty() ? "unknown" : channel.cs_pin.c_str());
    }
  }
  channel.checked = true;
  channel.available = available;
  channel.rx_ready = available && rx_ready;
  channel.tx_ready = available && tx_ready;
  // A missing CC1101 is non-fatal: report an amber hardware warning without
  // disabling Wi-Fi, API, web server or OTA. Only decide after *all* three
  // configured radios have been checked; never report green on an empty board.
  bool all_checked = channel_count_ != 0;
  bool any_unavailable = false;
  for (uint8_t i = 0; i < channel_count_; i++) {
    const auto &radio = channels_[i];
    all_checked = all_checked && radio.checked;
    any_unavailable = any_unavailable || !radio.available || !radio.rx_ready || !radio.tx_ready;
  }
  led_.radio_missing(all_checked && any_unavailable);
}

void RFLinkUniversal::set_radio_diagnostic_pins(uint8_t index, const std::string &cs_pin,
                                                  const std::string &gdo0_pin, const std::string &gdo2_pin) {
  if (index >= MAX_CHANNELS) return;
  channels_[index].cs_pin = cs_pin;
  channels_[index].gdo0_pin = gdo0_pin;
  channels_[index].gdo2_pin = gdo2_pin;
}

void RFLinkUniversal::set_radio_cc1101(uint8_t index, cc1101::CC1101Component *radio) {
  if (index >= MAX_CHANNELS) return;
  channels_[index].cc1101 = radio;
}

void RFLinkUniversal::run_radio_self_test(uint8_t index) {
  // Backwards-compatible one-shot entry point used by older configs/tests.
  this->run_radio_watchdog(index, false);
}

void RFLinkUniversal::run_radio_watchdog(uint8_t index, bool driver_failed) {
  if (index == 2 && radio_mode_868_.busy()) return; // protect exclusive UART/SPI lease
  if (index >= MAX_CHANNELS) return;
  auto &ch = channels_[index];
  if (ch.cc1101 == nullptr) return;

  // Read-only CC1101 status-register probe. PARTNUM=0x30, VERSION=0x31.
  // Status registers require READ + BURST status-register access (0xC0).
  ch.cc1101->enable();
  ch.cc1101->write_byte(0xF0);  // 0x30 | READ(0x80) | BURST(0x40)
  ch.partnum = ch.cc1101->read_byte();
  ch.cc1101->disable();

  ch.cc1101->enable();
  ch.cc1101->write_byte(0xF1);  // 0x31 | READ(0x80) | BURST(0x40)
  ch.version = ch.cc1101->read_byte();
  ch.cc1101->disable();

  ch.self_test_done = true;
  ch.spi_checks++;
  // PARTNUM for a real CC1101 is normally 0x00, therefore VERSION is the
  // useful liveness discriminator. 0xFF usually means floating/no response;
  // 0x00 often indicates a stuck-low or otherwise invalid SPI response.
  const bool spi_ok = ch.version != 0x00 && ch.version != 0xFF;
  ch.self_test_spi_ok = spi_ok;

  if (spi_ok) {
    if (ch.spi_consecutive_failures != 0) {
      system_trace("info", "radio", "", "SPI restored: " + ch.name + " after " +
                   std::to_string(ch.spi_consecutive_failures) + " failed checks");
      ESP_LOGI(TAG, "Radio SPI watchdog %s: response restored after %u consecutive failure(s); PARTNUM=0x%02X VERSION=0x%02X",
               ch.name.c_str(), ch.spi_consecutive_failures, ch.partnum, ch.version);
    }
    ch.spi_consecutive_failures = 0;
    if (ch.spi_recovery_pending && !driver_failed) {
      ch.spi_recovery_pending = false;
      ch.spi_recovery_successes++;
      system_trace("info", "radio", "", "SPI recovery succeeded: " + ch.name);
      ESP_LOGI(TAG, "Radio SPI watchdog %s: automatic recovery succeeded (%u/%u)", ch.name.c_str(),
               static_cast<unsigned>(ch.spi_recovery_successes), static_cast<unsigned>(ch.spi_recovery_attempts));
    }
  } else {
    ch.spi_failures++;
    if (ch.spi_consecutive_failures < 0xFF) ch.spi_consecutive_failures++;
    if (ch.spi_consecutive_failures == 1 || ch.spi_consecutive_failures == 3) {
      system_trace(ch.spi_consecutive_failures == 3 ? "error" : "warn", "radio", "",
                   "SPI invalid response: " + ch.name + " (" +
                   std::to_string(ch.spi_consecutive_failures) + "/3); VERSION=" +
                   std::to_string(ch.version));
      ESP_LOGW(TAG, "Radio SPI watchdog %s: invalid response %u/3; PARTNUM=0x%02X VERSION=0x%02X",
               ch.name.c_str(), ch.spi_consecutive_failures, ch.partnum, ch.version);
    }
  }

  // Recovery policy:
  //  * one/two bad probes are treated as transient and do not reset hardware;
  //  * three consecutive invalid probes request a reinitialisation;
  //  * if ESPHome marked the driver failed but SPI is responding again, request
  //    reinitialisation immediately;
  //  * at most one automatic recovery every 30 seconds per radio.
  const bool needs_recovery = (spi_ok && driver_failed) || (!spi_ok && ch.spi_consecutive_failures >= 3);
  const uint32_t now = millis();
  const bool cooldown_done = ch.spi_last_recovery_ms == 0 ||
                             static_cast<uint32_t>(now - ch.spi_last_recovery_ms) >= 30000U;
  if (needs_recovery && cooldown_done && !ch.spi_recovery_requested) {
    ch.spi_recovery_requested = true;
    ch.spi_recovery_pending = true;
    ch.spi_last_recovery_ms = now;
    ch.spi_recovery_attempts++;
    system_trace("warn", "radio", "", "SPI watchdog requesting CC1101 recovery: " + ch.name);
    ESP_LOGW(TAG, "Radio SPI watchdog %s: requesting automatic CC1101 reinitialisation (attempt %u)",
             ch.name.c_str(), static_cast<unsigned>(ch.spi_recovery_attempts));
  }

  ESP_LOGD(TAG, "Radio SPI watchdog %s: check=%u failures=%u consecutive=%u PARTNUM=0x%02X VERSION=0x%02X SPI=%s",
           ch.name.c_str(), static_cast<unsigned>(ch.spi_checks), static_cast<unsigned>(ch.spi_failures),
           ch.spi_consecutive_failures, ch.partnum, ch.version, spi_ok ? "response" : "invalid");
}

bool RFLinkUniversal::take_radio_recovery_request(uint8_t index) {
  if (index >= MAX_CHANNELS) return false;
  if (index == 2 && radio_mode_868_.busy()) return false; // defer pending watchdog reset
  auto &ch = channels_[index];
  const bool requested = ch.spi_recovery_requested;
  ch.spi_recovery_requested = false;
  return requested;
}

bool RFLinkUniversal::radio_watchdog_healthy(uint8_t index) const {
  if (index >= MAX_CHANNELS) return false;
  if (index == 2 && !radio_mode_868_.itho_ready()) return false; // show leased/faulted as unavailable
  const auto &ch = channels_[index];
  // Allow one or two isolated read errors without taking a working radio
  // offline. Three consecutive invalid reads are considered a real fault.
  return ch.self_test_done && ch.spi_consecutive_failures < 3;
}

// TX entries are RADIO events, not just web toasts or internal system traces.
// Emit them even if a radio is unavailable, so rejection is visible in the
// selected radio's timeline (and in the combined logbook). Do not print keys
// or raw payloads; this log says what the firmware requested/observed, never
// claims that a physical receiver executed a command.
void RFLinkUniversal::record_radio_tx_event(uint8_t index, const std::string &protocol,
                               const std::string &summary, const std::string &uid,
                               const std::string &detail, const char *level) {
  if (index >= channel_count_) return;
  this->record_activity_(index, protocol, summary.substr(0, 180), millis(), "TX",
                         detail.substr(0, 240), "", false, "", nullptr, uid,
                         0, level ? level : "info");
}

// Old native TX call sites continue to record a bounded radio event.
void RFLinkUniversal::record_tx_command(uint8_t index, const std::string &summary) {
  this->record_radio_tx_event(index, "TX-opdracht", summary, "",
      "Firmware accepted the TX request; receiver reaction is unconfirmed", "info");
}

const char *RFLinkUniversal::language() const {
  static const char *const languages[] = {"en", "nl", "fr", "de"};
  return languages[language_code_ < 4 ? language_code_ : 0];
}

bool RFLinkUniversal::set_language(const std::string &language) {
  const char *const languages[] = {"en", "nl", "fr", "de"};
  uint8_t selected = 4;
  for (uint8_t index = 0; index < 4; ++index)
    if (language == languages[index]) { selected = index; break; }
  if (selected == 4) return false;
  if (selected == language_code_) return true;
#ifdef USE_ESP32
  // Do not report a preference as saved if NVS cannot persist it.
  if (global_preferences == nullptr) return false;
  const LanguagePreferences stored{selected, 0, 0x4C47};
  if (!language_pref_.save(&stored) || !global_preferences->sync()) return false;
#endif
  language_code_ = selected;
  return true;
}

void RFLinkUniversal::set_led_settings(uint8_t brightness, bool quiet) {
  led_.settings(brightness, quiet);
#ifdef USE_ESP32
  if (global_preferences != nullptr) {
    LedPreferences stored{led_.brightness(), static_cast<uint8_t>(led_.quiet()), 0x4C45};
    if (led_pref_.save(&stored)) global_preferences->sync();
  }
#endif
}

void RFLinkUniversal::led_start_learning(uint8_t channel, bool baseline, const std::string &label) {
  if (baseline) developer_.start_baseline(channel);
  else developer_.start_action(channel, label);
  led_.learn(true);
}

void RFLinkUniversal::led_finish_baseline() {
  developer_.finish_baseline();
  led_.learn(false);
}

void RFLinkUniversal::led_stop_learning() {
  developer_.reset();
  led_.learn(false);
}

size_t RFLinkUniversal::decoder_registry_size_() { return decoder_registry().size(); }

bool RFLinkUniversal::save_devices_() {
#ifdef USE_ESP32
  if (!storage_ready_ || !storage_ok_ || global_preferences == nullptr || device_generation_ == UINT32_MAX) return false;
  auto blob = std::make_unique<DevicePersistence::Blob>();
  if (!DevicePersistence::encode(devices_, device_generation_ + 1, *blob)) {
    ESP_LOGE(TAG, "Device storage limit exceeded (%u bytes, maximum %u devices)",
             static_cast<unsigned>(DevicePersistence::CAPACITY), static_cast<unsigned>(DevicePersistence::MAX_DEVICES));
    return false;
  }
  auto &target = active_device_bank_b_ ? device_pref_a_ : device_pref_b_;
  if (!target.save(blob.get()) || !global_preferences->sync()) {
    ESP_LOGE(TAG, "Saving device list failed; previous snapshot remains active");
    return false;
  }
  active_device_bank_b_ = !active_device_bank_b_;
  device_generation_++;
#endif
  return true;
}

bool RFLinkUniversal::save_user_library_() {
#ifdef USE_ESP32
  if (!library_ready_ || !library_ok_ || global_preferences == nullptr || library_generation_ == UINT32_MAX) return false;
  auto blob = std::make_unique<UserLibraryPersistence::Blob>();
  if (!UserLibraryPersistence::encode(user_library_, library_generation_ + 1, *blob)) return false;
  auto &target = active_library_bank_b_ ? library_pref_a_ : library_pref_b_;
  if (!target.save(blob.get()) || !global_preferences->sync()) return false;
  active_library_bank_b_ = !active_library_bank_b_;
  ++library_generation_;
#else
  if (!library_ready_ || !library_ok_) return false;
#endif
  return true;
}

const UserDecoder *RFLinkUniversal::effective_user_decoder(const std::string &id) const {
  if (const auto *local = user_library_.decoder(id)) return local;
  return firmware_library_.decoder(id);
}

bool RFLinkUniversal::validate_user_decoder_activation_(const UserDecoder &candidate,
                                                         const UserLibrary &staged,
                                                         std::string &error) const {
  for (const auto &sample : candidate.samples) {
    RFFrame frame;
    frame.kind = sample.kind;
    frame.channel = sample.channel;
    frame.raw = &sample.raw;
    frame.packet = &sample.packet;
    for (auto *builtin : decoder_registry()) if (builtin && builtin->match(frame) >= MIN_DECODER_CONFIDENCE) {
      DecodeResult result;
      if (builtin->decode(frame, result) && result.valid) {
        error = "capture belongs to the built-in decoder " + std::string(builtin->name());
        return false;
      }
    }
    auto overlaps = [&](const UserLibrary &library) {
      for (const auto &other : library.decoders()) {
        if (!other.enabled || other.id == candidate.id || other.radio != sample.channel || other.kind != sample.kind) continue;
        DecodeResult result;
        std::string reason;
        if (user_vm::receive(other.rx, other.id, frame, result, reason)) {
          error = "capture overlaps active user decoder " + other.id;
          return true;
        }
      }
      return false;
    };
    if (overlaps(staged) || overlaps(firmware_library_)) return false;
  }
  return true;
}

bool RFLinkUniversal::save_user_decoder(UserDecoder decoder, std::string &error) {
  if (!this->library_ready()) { error = "user library storage is unavailable"; return false; }
  const std::string decoder_id = decoder.id;
  const auto *local = user_library_.decoder(decoder_id);
  const bool restore_enabled = local && local->enabled;
  const auto *previous = this->effective_user_decoder(decoder_id);
  if (previous && (previous->tx_source != decoder.tx_source || previous->radio != decoder.radio ||
                   previous->kind != decoder.kind || previous->actions != decoder.actions)) {
    for (const auto &device : devices_.devices()) if (device.protocol == decoder.id && device.local_transmit_enabled) {
      error = "revoke device TX approval before changing its send rules"; return false;
    }
  }

  // Transactional edit: an active decoder is disabled only in RAM, the new
  // rules are compiled/replayed, overlap-checked and re-enabled, and only then
  // is one complete NVS snapshot written. Any failure restores the old working
  // library, so editing can never strand a previously active decoder disabled.
  const auto backup = user_library_;
  if (restore_enabled && !user_library_.enable_decoder(decoder.id, false, error)) {
    user_library_ = backup; return false;
  }
  decoder.enabled = false;
  if (!user_library_.upsert_decoder(std::move(decoder), error)) {
    user_library_ = backup; return false;
  }
  if (restore_enabled) {
    const auto *candidate = user_library_.decoder(decoder_id);
    if (!candidate) { user_library_ = backup; error = "staged decoder disappeared"; return false; }
    if (!this->validate_user_decoder_activation_(*candidate, user_library_, error) ||
        !user_library_.enable_decoder(candidate->id, true, error)) {
      user_library_ = backup; return false;
    }
  }
  if (!save_user_library_()) { user_library_ = backup; error = "could not save decoder"; return false; }
  return true;
}

bool RFLinkUniversal::enable_user_decoder(const std::string &id, bool enabled, std::string &error) {
  if (!this->library_ready()) { error = "user library storage is unavailable"; return false; }
  const auto *candidate = user_library_.decoder(id);
  if (enabled && candidate && !this->validate_user_decoder_activation_(*candidate, user_library_, error)) return false;
  if (!enabled) for (const auto &device : devices_.devices())
    if (device.protocol == id && device.local_transmit_enabled) {
      error = "disable TX on attached devices first"; return false;
    }
  const auto backup = user_library_;
  if (!user_library_.enable_decoder(id, enabled, error)) return false;
  if (!save_user_library_()) { user_library_ = backup; error = "could not save decoder state"; return false; }
  return true;
}

bool RFLinkUniversal::remove_user_decoder(const std::string &id, std::string &error) {
  if (!this->library_ready()) { error = "user library storage is unavailable"; return false; }
  for (const auto &device : devices_.devices()) if (device.protocol == id) {
    error = "a saved device still uses this decoder"; return false;
  }
  const auto backup = user_library_;
  if (!user_library_.remove_decoder(id)) { error = "disable decoder and remove dependent profiles first"; return false; }
  if (!save_user_library_()) { user_library_ = backup; error = "could not save deletion"; return false; }
  return true;
}

bool RFLinkUniversal::save_user_profile(UserProfile profile, std::string &error) {
  if (!this->library_ready()) { error = "user library storage is unavailable"; return false; }
  if (const auto *decoder = this->effective_user_decoder(profile.protocol)) {
    for (const auto &action : profile.expected_actions)
      if (std::find(decoder->actions.begin(), decoder->actions.end(), action) == decoder->actions.end()) {
        error = "action is not defined by decoder"; return false;
      }
  } else {
    const auto *builtin = capabilities::find(profile.protocol.c_str());
    if (!builtin) { error = "decoder not found"; return false; }
    for (const auto &action : profile.expected_actions)
      if (!capabilities::contains(builtin->rx_actions, action.c_str())) {
        error = "action is not defined by decoder"; return false;
      }
  }
  const auto backup = user_library_;
  if (!user_library_.upsert_profile(std::move(profile), error)) return false;
  if (!save_user_library_()) { user_library_ = backup; error = "could not save profile"; return false; }
  return true;
}

bool RFLinkUniversal::restore_user_state(const std::string &key, uint32_t value, std::string &error) {
  if (!this->library_ready()) { error = "user library storage is unavailable"; return false; }
  if (key.empty() || key.size() > 128 || value == 0) { error = "invalid protocol state"; return false; }
  const auto existing = user_library_.state().find(key);
  if (existing != user_library_.state().end() && existing->second >= value) return true;
  const auto backup = user_library_;
  if (!user_library_.commit_state({{key, value}})) { error = "protocol state could not be advanced"; return false; }
  if (!save_user_library_()) { user_library_ = backup; error = "could not save protocol state"; return false; }
  return true;
}

bool RFLinkUniversal::remove_user_profile(const std::string &id, std::string &error) {
  if (!this->library_ready()) { error = "user library storage is unavailable"; return false; }
  for (const auto &device : devices_.devices()) if (device.profile_id == id) {
    error = "a saved device still uses this profile"; return false;
  }
  const auto backup = user_library_;
  if (!user_library_.remove_profile(id)) { error = "profile not found"; return false; }
  if (!save_user_library_()) { user_library_ = backup; error = "could not save deletion"; return false; }
  return true;
}

bool RFLinkUniversal::set_user_profile(const std::string &uid, const std::string &profile_id,
                                        std::string &error) {
  if (!storage_ready_ || !storage_ok_ || !this->library_ready()) {
    error = "storage unavailable"; return false;
  }
  const auto *device = devices_.find_by_uid(uid);
  if (!device) { error = "device not found"; return false; }
  if (!profile_id.empty()) {
    // Local/firmware profiles intentionally shadow a built-in profile with the
    // same stable ID. This lets a user customize presentation without mutating
    // the read-only firmware catalog.
    const UserProfile *shadow = nullptr;
    for (const auto &p : user_library_.profiles()) if (p.id == profile_id) { shadow = &p; break; }
    if (!shadow) for (const auto &p : firmware_library_.profiles()) if (p.id == profile_id) { shadow = &p; break; }
    if (shadow) {
      if (shadow->protocol != device->protocol) { error = "profile does not match device protocol"; return false; }
    } else {
      const auto *official = profiles::find(profile_id.c_str());
      if (!official || device->protocol != official->protocol) {
        error = "profile does not match device protocol"; return false;
      }
    }
  }
  if (device->profile_id == profile_id) return true;
  const auto old = devices_;
  devices_.find_by_uid(uid)->profile_id = profile_id;
  if (!save_devices_()) { devices_ = old; error = "could not save device profile"; system_trace("error","device",uid,"Profile change failed: NVS save");return false; }
  system_trace("info","device",uid,std::string("Selected device profile: ")+(profile_id.empty()?"none":profile_id));
  return true;
}

bool RFLinkUniversal::preview_user_transmit(const std::string &id, const std::string &action,
                                             user_vm::TxFrame &frame, std::string &error,
                                             const std::string &uid) const {
  const auto *decoder = this->effective_user_decoder(id);
  if (!decoder || decoder->tx_source.empty() ||
      std::find(decoder->actions.begin(), decoder->actions.end(), action) == decoder->actions.end()) {
    error = "no TX rule for this action"; return false;
  }
  std::map<std::string, uint32_t> state;
  if (!uid.empty()) {
    const auto *device = devices_.find_by_uid(uid);
    if (!device || device->protocol != id) { error = "device and decoder do not match"; return false; }
    const auto prefix = uid + ":" + id + ":";
    for (const auto &entry : user_library_.state())
      if (entry.first.compare(0, prefix.size(), prefix) == 0)
        state[entry.first.substr(prefix.size())] = entry.second;
  }
  return user_vm::transmit(decoder->tx, action, decoder->radio, state, frame, error);
}

bool RFLinkUniversal::approve_user_transmit(const std::string &uid, bool enabled, std::string &error) {
  if (!storage_ready_ || !storage_ok_ || !this->library_ready()) {
    error = "storage unavailable"; return false;
  }
  const auto *found = devices_.find_by_uid(uid);
  const auto *decoder = found ? this->effective_user_decoder(found->protocol) : nullptr;
  if (!found || !decoder || !decoder->enabled || decoder->tx_source.empty()) {
    error = "device has no active user TX decoder"; return false;
  }
  if (found->local_transmit_enabled == enabled) return true;
  const auto old = devices_;
  auto *device = devices_.find_by_uid(uid);
  device->local_transmit_enabled = enabled;
  if (!save_devices_()) { devices_ = old; error = "could not save TX approval"; return false; }
  const auto *updated = devices_.find_by_uid(uid);
  if (updated && updated->home_assistant_enabled && this->mqtt_connected())
    this->ha_discovery_(*updated, false, true);
  return true;
}

bool RFLinkUniversal::user_transmit(const std::string &uid, const std::string &action, std::string &error) {
  system_trace("info","tx",uid,"User decoder TX requested: "+action);
  if (!storage_ready_ || !storage_ok_ || !this->library_ready() || (tx_sent_ && millis() - tx_last_ms_ < 600)) {
    error = "storage unavailable or transmit rate limit active"; return false;
  }
  const auto *device = devices_.find_by_uid(uid);
  const auto *decoder = device ? this->effective_user_decoder(device->protocol) : nullptr;
  if (!device || !device->local_transmit_enabled || !decoder || !decoder->enabled ||
      decoder->tx_source.empty() ||
      std::find(device->actions.begin(), device->actions.end(), action) == device->actions.end() ||
      std::find(decoder->actions.begin(), decoder->actions.end(), action) == decoder->actions.end()) {
    error = "action not approved for this device";
    system_trace("warn","tx",uid,"User decoder TX rejected: action, decoder or opt-in missing");return false;
  }
  const uint8_t radio = decoder->radio;
  // Check arbitration before any NVS counter reservation or other side effect.
  if (radio == 2 && !radio_mode_868_.itho_ready()) {
    error = "868 MHz radio reserved by another protocol"; return false;
  }
  if (radio >= channel_count_ || !channels_[radio].available || !channels_[radio].tx_ready ||
      ((decoder->kind == FrameKind::RAW) != (channels_[radio].kind == FrameKind::RAW))) {
    error = "required radio is unavailable"; return false;
  }
  const auto user_tx_spec=legacy_user_tx_spec(*decoder);
  const TxTransport user_tx_transport=choose_tx_transport(
      user_tx_spec,decoder->kind,
      radio < raw_transmitters_.size() && raw_transmitters_[radio] != nullptr,
#ifdef USE_ESP32
      radio == 2 && packet_transmitter_ != nullptr,
#else
      false,
#endif
      false); // GDO serial must be explicitly implemented and approved.
  if (user_tx_transport==TxTransport::NONE) {
    error = "TX transport unavailable or not approved";
    system_trace("warn","tx",uid,error);return false;
  }
  std::map<std::string, uint32_t> device_state;
  const std::string prefix = uid + ":" + decoder->id + ":";
  for (const auto &entry : user_library_.state())
    if (entry.first.compare(0, prefix.size(), prefix) == 0)
      device_state[entry.first.substr(prefix.size())] = entry.second;
  user_vm::TxFrame frame;
  if (!user_vm::transmit(decoder->tx, action, radio, device_state, frame, error) ||
      (decoder->kind == FrameKind::RAW && frame.raw.empty()) ||
      (decoder->kind == FrameKind::PACKET && frame.packet.empty())) return false;
  if (decoder->kind == FrameKind::PACKET && !radio_mode_868_.itho_ready()) {
    error = "868 MHz radio reserved by another protocol"; return false;
  }
  if (decoder->kind == FrameKind::PACKET && frame.packet.size() != 63) {
    error = "packet TX requires exactly 63 bytes on this board"; return false;
  }
  if (!frame.next_state.empty()) {
    std::map<std::string, uint32_t> updates;
    for (const auto &entry : frame.next_state) updates[prefix + entry.first] = entry.second;
    const auto old = user_library_;
    if (!user_library_.commit_state(updates) || !save_user_library_()) {
      user_library_ = old; error = "could not reserve TX protocol state"; return false;
    }
    // Commit the next counter BEFORE touching the radio: a lost ACK must never
    // cause a rolling counter to be reused after reboot.
  }
  if (user_tx_transport == TxTransport::RMT_RAW) {
    if (radio >= raw_transmitters_.size() || raw_transmitters_[radio] == nullptr) {
      error = "RAW TX driver unavailable"; return false;
    }
    auto call = raw_transmitters_[radio]->transmit();
    auto *data = call.get_data();
    for (int32_t pulse : frame.raw) {
      if (pulse > 0) data->mark(pulse);
      else data->space(-pulse);
    }
    call.set_send_times(frame.repeats);
    call.perform();
  } else if (user_tx_transport == TxTransport::SPI_FIFO_PACKET) {
#ifdef USE_ESP32
    if (packet_transmitter_ == nullptr) { error = "packet TX driver unavailable"; return false; }
    for (uint8_t n = 0; n < frame.repeats; ++n)
      if (packet_transmitter_->transmit_packet(frame.packet) != cc1101::CC1101Error::NONE) {
        error = "packet radio rejected TX"; return false;
      }
#else
    error = "packet TX requires ESP32 hardware"; return false;
#endif
  } else {
    error = "unsupported user TX physical route";return false;
  }
  tx_last_ms_ = millis(); tx_sent_ = true;
  record_tx_command(radio, "User " + decoder->id + " " + uid + " " + action + " (TX requested)");
  return true;
}

DeviceRecord *RFLinkUniversal::confirm_candidate(const std::string &identity, const std::string &name,
                                                  const std::vector<std::string> *approved_actions,
                                                  const std::vector<std::string> *approved_fields,
                                                  const std::string &selected_profile) {
  if (!storage_ok_ && storage_ready_) return nullptr;
  if (!selected_profile.empty()) {
    const auto *candidate = devices_.find_candidate(identity);
    if (!candidate) return nullptr;
    std::string owner;
    for (const auto &p : user_library_.profiles()) if (p.id == selected_profile) { owner=p.protocol; break; }
    if (owner.empty()) for (const auto &p : firmware_library_.profiles()) if (p.id == selected_profile) { owner=p.protocol; break; }
    if (owner.empty()) {
      const auto *official = profiles::find(selected_profile.c_str());
      if (official) owner=official->protocol;
    }
    if (owner.empty() || owner != candidate->protocol) return nullptr;
  }
  const auto old = devices_;
  auto *record = devices_.confirm_candidate(identity, name, approved_actions, approved_fields);
  if (record == nullptr) return nullptr;
  std::string single_profile_id;
  size_t matching_profiles = 0;
  auto local_id_exists = [&](const std::string &id) {
    return std::any_of(user_library_.profiles().begin(), user_library_.profiles().end(),
                       [&](const UserProfile &p) { return p.id == id; }) ||
           std::any_of(firmware_library_.profiles().begin(), firmware_library_.profiles().end(),
                       [&](const UserProfile &p) { return p.id == id; });
  };
  for (const auto &profile : user_library_.profiles()) if (profile.protocol == record->protocol) {
    single_profile_id = profile.id; ++matching_profiles;
  }
  for (const auto &profile : firmware_library_.profiles()) if (profile.protocol == record->protocol) {
    if (std::any_of(user_library_.profiles().begin(), user_library_.profiles().end(),
                    [&](const UserProfile &local) { return local.id == profile.id; })) continue;
    single_profile_id = profile.id; ++matching_profiles;
  }
  size_t official_count = 0;
  const auto *official = profiles::all(official_count);
  for (size_t i = 0; i < official_count; ++i) {
    if (record->protocol != official[i].protocol || local_id_exists(official[i].id)) continue;
    // Shared protocols (e.g. RAMSES-II) must not silently identify the vendor.
    // A source-backed manual_only profile remains user-selectable in Devices.
    if (std::strcmp(official[i].compatibility, "manual_only") == 0) continue;
    single_profile_id = official[i].id; ++matching_profiles;
  }
  if (!selected_profile.empty()) record->profile_id = selected_profile;
  else if (matching_profiles == 1) record->profile_id = single_profile_id;
  const auto uid = record->uid;
  if (!save_devices_()) { devices_ = old; led_.error(millis()); system_trace("error","learn",uid,"New device could not be saved");return nullptr; }
  system_trace("info","learn",uid,std::string("Device learned; protocol ")+record->protocol+
               "; profile "+(record->profile_id.empty()?"not selected":record->profile_id));
  led_.learned(millis());
  return devices_.find_by_uid(uid);
}

bool RFLinkUniversal::dismiss_candidate(const std::string &identity) {
  // A candidate is volatile and not paired or saved; no NVS/HA changes.
  return devices_.dismiss_candidate(identity);
}

bool RFLinkUniversal::link_candidate(const std::string &identity, const std::string &uid) {
  const auto *bound=devices_.find_by_uid(uid);
  if (bound && (bound->transmit_enabled || bound->local_transmit_enabled)) return false; // disable TX and approve new wiring first
  if (!storage_ok_ && storage_ready_) return false;
  const auto old = devices_;
  if (!devices_.link_candidate(identity, uid) || !save_devices_()) {
    devices_ = old;
    led_.error(millis());
    return false;
  }
  system_trace("info","learn",uid,"RF identity linked to existing device and saved");
  led_.learned(millis());
  return true;
}

bool RFLinkUniversal::rename_device(const std::string &uid, const std::string &name) {
  if (!storage_ok_ && storage_ready_) return false;
  const auto old = devices_;
  if (!devices_.rename_device(uid, name) || !save_devices_()) { devices_ = old; return false; }
  auto *updated = devices_.find_by_uid(uid);
  if (updated != nullptr && updated->home_assistant_enabled) this->ha_discovery_(*updated, false, true);
  return true;
}

bool RFLinkUniversal::remove_device(const std::string &uid) {
  if (!storage_ok_ && storage_ready_) return false;
  const auto old = devices_;
  const auto *record = devices_.find_by_uid(uid);
  if (record == nullptr) return false;
  const bool published = record->home_assistant_enabled;
  // Never delete a local record while HA would retain an undiscoverable ghost.
  if (published && !this->ha_discovery_(*record, true)) return false;
  if (!devices_.remove_device(uid) || !save_devices_()) {
    devices_ = old;
    if (published) this->ha_discovery_(*devices_.find_by_uid(uid), false, true);
    return false;
  }
  ha_discovery_cache_.erase(uid);
  return true;
}

TxTransport RFLinkUniversal::native_route_for_(const tx_engine::Plan &plan) const {
  if (plan.radio >= channel_count_ || plan.radio >= 3 ||
      !channels_[plan.radio].available || !channels_[plan.radio].tx_ready ||
      channels_[plan.radio].kind != plan.kind ||
      channels_[plan.radio].frequency_hz != plan.frequency_hz) return TxTransport::NONE;
  if (plan.radio == 2 && !radio_mode_868_.itho_ready()) return TxTransport::NONE;
  TxAdapterStatus available;
  available.rmt = plan.kind == FrameKind::RAW && plan.radio < raw_transmitters_.size() &&
                  raw_transmitters_[plan.radio] != nullptr;
#ifdef USE_ESP32
  available.spi_fifo = plan.kind == FrameKind::PACKET && plan.radio == 2 && packet_transmitter_ != nullptr;
#endif
  // A serial physical backend must prove TX, RX-restoration and pin ownership
  // before this can become true. Manifest preferences alone never enable RF.
  available.gdo_serial = false;
  if (!plan.protocol.empty() && serial_tx_adapter_) {
    const auto *approved = radio_v2_catalog::tx_for(plan.protocol.c_str());
    available.gdo_serial = approved && approved->enabled && approved->channel == plan.radio &&
                           approved->rf_profile &&
                           serial_tx_adapter_->ready(plan.radio, approved->rf_profile);
  }
  if (!plan.protocol.empty()) {
    return resolve_transmit_route(radio_v2_catalog::tx_for(plan.protocol.c_str()),
                                  plan.radio, plan.kind, available);
  }
  // Legacy internal management frames have no decoder plan; they remain
  // restricted to their original physical adapter (no new fallback access).
  return plan.transport == TxTransport::RMT_RAW && available.rmt ? TxTransport::RMT_RAW :
         plan.transport == TxTransport::SPI_FIFO_PACKET && available.spi_fifo ? TxTransport::SPI_FIFO_PACKET :
         TxTransport::NONE;
}

bool RFLinkUniversal::native_plan_available_(const tx_engine::Plan &plan) const {
  return this->native_route_for_(plan) != TxTransport::NONE;
}

bool RFLinkUniversal::tx_action_ready(const std::string &uid,const std::string &action) const {
  if(!tx_feature_enabled_)return false;
  const auto *device=devices_.find_by_uid(uid);
  if(!device || action=="prog" || (device->protocol=="RamsesII" && !ramses_pair_lab_ready()))return false; // explicit experimental gate
  tx_engine::Plan plan; const tx_engine::Encoder *encoder=nullptr;
  return tx_engine::prepare(*device,action,plan,encoder) && encoder && native_plan_available_(plan);
}

bool RFLinkUniversal::tx_eligible(const std::string &uid) const {
  const auto *device=devices_.find_by_uid(uid);
  if(!tx_feature_enabled_||!device)return false;
  if(!device->profile_id.empty()){
    const auto *p=profiles::find(device->profile_id.c_str());
    if(p && device->protocol==p->protocol){
      const std::string controls=p->controls;
      size_t start=0;
      while(start<controls.size()){
        const size_t end=controls.find('|',start);
        if(this->tx_action_ready(uid,controls.substr(start,end==std::string::npos?end:end-start)))return true;
        if(end==std::string::npos)break;
        start=end+1;
      }
    }
  }
  for(const auto &action:device->actions)if(this->tx_action_ready(uid,action))return true;
  return false;
}
bool RFLinkUniversal::dispatch_native_plan_(const std::string &uid, const std::string &action,
                                             const tx_engine::Plan &plan, const tx_engine::Encoder &encoder,
                                             const std::string &origin, std::string &error) {
  system_trace("info","tx",uid,"Dispatch: encoder prepared; selecting approved RF route");
  record_radio_tx_event(plan.radio, plan.protocol, "TX aangevraagd: "+action, uid,
      origin+"; "+std::to_string(plan.frequency_hz)+" Hz; route selection pending");
  if (!this->native_plan_available_(plan)) {
    error = "action or required radio is unavailable";
    record_radio_tx_event(plan.radio, plan.protocol, "TX afgewezen: "+action, uid, error, "error");
    system_trace("warn","tx",uid,error);return false;
  }
  // Route is resolved from the trusted manifest and live adapter state at
  // dispatch time. A fallback never gains permission from a UI/user profile.
  const TxTransport route = this->native_route_for_(plan);
  if (route == TxTransport::NONE) {
    error = "no approved TX adapter available";
    record_radio_tx_event(plan.radio, plan.protocol, "TX afgewezen: "+action, uid, error, "error");
    system_trace("warn","tx",uid,error);return false;
  }
  system_trace("info","tx",uid,std::string("TX route selected: ")+(route==TxTransport::GDO_SERIAL?"GDO_SERIAL":route==TxTransport::SPI_FIFO_PACKET?"SPI_FIFO": "RMT_RAW")+
               " radio="+std::to_string(plan.radio));
  if (!this->reserve_tx_state_(uid, plan, error)) {
    record_radio_tx_event(plan.radio, plan.protocol, "TX afgewezen: "+action, uid, error, "error");
    system_trace("error","tx",uid,"TX state reservation failed: "+error);return false;
  }
  record_radio_tx_event(plan.radio, plan.protocol, "TX-route voorbereid: "+action, uid,
      std::string("Hardware route: ")+(route==TxTransport::GDO_SERIAL?"GDO/UART":route==TxTransport::SPI_FIFO_PACKET?"SPI/FIFO":"RMT/OOK"));
  system_trace("info","tx",uid,"Protocol state checked/reserved; requesting hardware TX");
  if (route == TxTransport::RMT_RAW) {
    auto call = raw_transmitters_[plan.radio]->transmit();
    if (!encoder.encode_raw(call.get_data(), plan)) { error = "encoder rejected RAW transmit plan"; record_radio_tx_event(plan.radio,plan.protocol,"TX mislukt: "+action,uid,error,"error"); return false; }
    call.set_send_times(plan.repeats);
    call.perform();
  } else if (route == TxTransport::SPI_FIFO_PACKET) {
#ifdef USE_ESP32
    std::vector<uint8_t> packet;
    if (!encoder.encode_packet(packet, plan) || packet.size() != 63) {
      error = "encoder rejected packet transmit plan"; record_radio_tx_event(plan.radio,plan.protocol,"TX mislukt: "+action,uid,error,"error"); return false;
    }
    for (uint8_t n = 0; n < plan.repeats; ++n) {
      if (packet_transmitter_->transmit_packet(packet) != cc1101::CC1101Error::NONE) {
        error = "packet radio rejected TX"; record_radio_tx_event(plan.radio,plan.protocol,"TX mislukt: "+action,uid,error,"error"); return false;
      }
      if (plan.repeat_gap_ms && n + 1u < plan.repeats) delay(plan.repeat_gap_ms);
    }
#else
    error = "packet TX requires ESP32 hardware"; record_radio_tx_event(plan.radio,plan.protocol,"TX afgewezen: "+action,uid,error,"error"); return false;
#endif
  } else if (route == TxTransport::GDO_SERIAL) {
    // Generic waveform adapter. No decoder-specific radio code in the engine.
    const auto *approved = radio_v2_catalog::tx_for(plan.protocol.c_str());
    std::vector<uint8_t> wire;
    if (!serial_tx_adapter_ || !approved || !approved->enabled ||
        !encoder.encode_serial(wire, plan) ||
        !serial_wire_sane(wire, plan.repeats, plan.repeat_gap_ms)) {
      error = "serial TX encoder or adapter is unavailable";
      record_radio_tx_event(plan.radio,plan.protocol,"TX mislukt: "+action,uid,error,"error");
      return false;
    }
    system_trace("info","tx",uid,"GDO serial TX: waveform prepared, entering hardware session");
    if (!serial_tx_adapter_->send(plan.radio, approved->rf_profile, wire.data(),
                                  wire.size(), plan.repeats, plan.repeat_gap_ms, error)) {
      system_trace("error","tx",uid,std::string("GDO serial TX failed: ")+error);
      if (error.empty()) error = "serial radio rejected TX or RX restoration";
      record_radio_tx_event(plan.radio,plan.protocol,"TX mislukt: "+action,uid,error,"error");
      return false;
    }
  } else {
    error = "declared TX transport is not verified or available";
    record_radio_tx_event(plan.radio,plan.protocol,"TX afgewezen: "+action,uid,error,"error");
    return false;
  }
  system_trace("info","tx",uid,"Hardware adapter returned success; device reaction NOT confirmed");
  record_radio_tx_event(plan.radio,plan.protocol,"TX door driver afgerond: "+action,uid,
      "Radio driver accepted transmission; receiver reaction is unconfirmed; RX restoration depends on adapter");
  tx_last_ms_ = millis(); tx_sent_ = true; led_.tx(millis());
  // The radio log already contains request, selected adapter and outcome.
  // Do not add a second, misleading TX-request event after completion.
  return true;
}

bool RFLinkUniversal::somfy_status(const std::string &uid, uint32_t &address,
                                        uint16_t &next_rolling_code, bool &paired) const {
  const auto *device = devices_.find_by_uid(uid);
  if (!device || device->protocol != "SomfyRTS56") return false;
  const auto *address_text = tx::state(*device, "somfy_address");
  const auto *counter_text = tx::state(*device, "somfy_rolling_code");
  const auto *paired_text = tx::state(*device, "somfy_paired");
  uint32_t counter = 0;
  if (!address_text || !counter_text || !paired_text ||
      !tx::unsigned_decimal(*address_text, 0xFFFFFFu, address) || address == 0 ||
      !tx::unsigned_decimal(*counter_text, 0xFFFFu, counter) || counter == 0 ||
      (*paired_text != "0" && *paired_text != "1")) return false;
  next_rolling_code = static_cast<uint16_t>(counter);
  paired = *paired_text == "1";
  return true;
}

bool RFLinkUniversal::restore_somfy_state(const std::string &uid, uint32_t address,
                                             uint16_t next_rolling_code, bool paired,
                                             std::string &error) {
  if (!storage_ready_ || !storage_ok_) { error = "device storage unavailable"; return false; }
  if (address == 0 || address > 0xFFFFFFu || next_rolling_code == 0) {
    error = "invalid Somfy backup state"; return false;
  }
  auto *device = devices_.find_by_uid(uid);
  if (!device || device->protocol != "SomfyRTS56" || !device->transmit_supported) {
    error = "device does not support Somfy RTS transmit"; return false;
  }

  uint32_t current_address = 0; uint16_t current_counter = 0; bool current_paired = false;
  const bool current_valid = this->somfy_status(uid, current_address, current_counter, current_paired);
  if (current_valid && current_address != address) {
    error = "backup belongs to a different Somfy virtual remote"; return false;
  }
  // Even a partially stored address is an identity: never silently replace it
  // with another virtual transmitter during restore.
  if (!current_valid) {
    if (const auto *address_text = tx::state(*device, "somfy_address")) {
      uint32_t partial_address = 0;
      if (tx::unsigned_decimal(*address_text, 0xFFFFFFu, partial_address) &&
          partial_address != 0 && partial_address != address) {
        error = "stored Somfy virtual remote differs from backup"; return false;
      }
    }
  }

  const uint16_t restored_counter = current_valid && current_counter > next_rolling_code
                                        ? current_counter : next_rolling_code;
  const bool restored_paired = (current_valid && current_paired) || paired;
  const DeviceStore backup = devices_;
  device = devices_.find_by_uid(uid);
  device->tx_state = {{"somfy_address", std::to_string(address), ""},
                      {"somfy_rolling_code", std::to_string(restored_counter), ""},
                      {"somfy_paired", restored_paired ? "1" : "0", ""}};
  if (!save_devices_()) { devices_ = backup; error = "could not save Somfy backup state"; return false; }
  return true;
}

bool RFLinkUniversal::ensure_somfy_remote_(const std::string &uid, std::string &error) {
  if (!storage_ready_ || !storage_ok_) { error = "device storage unavailable"; return false; }
  auto *device = devices_.find_by_uid(uid);
  if (!device || device->protocol != "SomfyRTS56" || !device->transmit_supported) {
    error = "device does not support Somfy RTS transmit"; return false;
  }
  uint32_t address = 0; uint16_t rolling = 0; bool paired = false;
  if (this->somfy_status(uid, address, rolling, paired)) return true;
  if (!device->tx_state.empty()) {
    error = "stored Somfy transmitter state is incomplete; reset is required"; return false;
  }
  auto used = [&](uint32_t candidate) {
    const std::string identity = hex_id("somfy_rts", candidate, 6);
    for (const auto &other : devices_.devices()) {
      uint32_t existing = 0;
      if (const auto *text = tx::state(other, "somfy_address"))
        if (tx::unsigned_decimal(*text, 0xFFFFFFu, existing) && existing == candidate) return true;
      if (std::find(other.rf_identities.begin(), other.rf_identities.end(), identity) != other.rf_identities.end()) return true;
    }
    return false;
  };
  uint32_t candidate = 0;
  for (uint8_t attempt = 0; attempt < 32 && candidate == 0; ++attempt) {
#ifdef USE_ESP32
    const uint32_t value = esp_random() & 0xFFFFFFu;
#else
    uint32_t value = 2166136261u ^ web_write_token_ ^ attempt;
    for (const char ch : uid) { value ^= static_cast<uint8_t>(ch); value *= 16777619u; }
    value &= 0xFFFFFFu;
#endif
    if (value != 0 && !used(value)) candidate = value;
  }
  if (candidate == 0) { error = "could not allocate a unique Somfy transmitter address"; return false; }
  const DeviceStore backup = devices_;
  device = devices_.find_by_uid(uid);
  device->tx_state = {{"somfy_address", std::to_string(candidate), ""},
                      {"somfy_rolling_code", "1", ""},
                      {"somfy_paired", "0", ""}};
  if (!save_devices_()) { devices_ = backup; error = "could not save Somfy transmitter state"; return false; }
  return true;
}

bool RFLinkUniversal::itho_status(const std::string &uid, uint32_t &address, uint32_t &next_sequence,
                                    uint8_t &device_type, bool &paired) const {
  const auto *device = devices_.find_by_uid(uid);
  if (!device || device->protocol != "IthoRFT") return false;
  const auto *address_text = tx::state(*device, "itho_address");
  const auto *sequence_text = tx::state(*device, "itho_sequence");
  const auto *device_type_text = tx::state(*device, "itho_device_type");
  const auto *paired_text = tx::state(*device, "itho_paired");
  uint32_t parsed_type = 0;
  if (!address_text || !sequence_text || !device_type_text || !paired_text ||
      !tx::unsigned_decimal(*address_text, 0xFFFFFFu, address) || address == 0 ||
      !tx::unsigned_decimal(*sequence_text, 0xFFFFFFFEu, next_sequence) || next_sequence == 0 ||
      !tx::unsigned_decimal(*device_type_text, 0xFFu, parsed_type) ||
      (*paired_text != "0" && *paired_text != "1")) return false;
  device_type = static_cast<uint8_t>(parsed_type);
  paired = *paired_text == "1";
  return true;
}

bool RFLinkUniversal::restore_itho_state(const std::string &uid, uint32_t address,
                                          uint32_t next_sequence, uint8_t device_type,
                                          bool paired, std::string &error) {
  if (!storage_ready_ || !storage_ok_) { error = "device storage unavailable"; return false; }
  if (address == 0 || address > 0xFFFFFFu || next_sequence == 0 || next_sequence > 0xFFFFFFFEu) {
    error = "invalid Itho backup state"; return false;
  }
  auto *device = devices_.find_by_uid(uid);
  if (!device || device->protocol != "IthoRFT" || !device->transmit_supported) {
    error = "device does not support Itho RFT transmit"; return false;
  }

  uint32_t current_address = 0, current_sequence = 0;
  uint8_t current_type = 0;
  bool current_paired = false;
  const bool current_valid = this->itho_status(uid, current_address, current_sequence, current_type, current_paired);
  if (current_valid && current_address != address) {
    error = "backup belongs to a different Itho virtual remote"; return false;
  }
  if (current_valid && current_type != device_type) {
    error = "backup uses a different Itho device type"; return false;
  }
  if (!current_valid) {
    if (const auto *address_text = tx::state(*device, "itho_address")) {
      uint32_t partial_address = 0;
      if (tx::unsigned_decimal(*address_text, 0xFFFFFFu, partial_address) &&
          partial_address != 0 && partial_address != address) {
        error = "stored Itho virtual remote differs from backup"; return false;
      }
    }
  }

  const uint32_t restored_sequence = current_valid && current_sequence > next_sequence
                                         ? current_sequence : next_sequence;
  const bool restored_paired = (current_valid && current_paired) || paired;
  const DeviceStore backup = devices_;
  device = devices_.find_by_uid(uid);
  device->tx_state = {{"itho_address", std::to_string(address), ""},
                      {"itho_sequence", std::to_string(restored_sequence), ""},
                      {"itho_device_type", std::to_string(device_type), ""},
                      {"itho_paired", restored_paired ? "1" : "0", ""}};
  if (!save_devices_()) { devices_ = backup; error = "could not save Itho backup state"; return false; }
  return true;
}

bool RFLinkUniversal::ensure_itho_remote_(const std::string &uid, std::string &error) {
  if (!storage_ready_ || !storage_ok_) { error = "device storage unavailable"; return false; }
  auto *device = devices_.find_by_uid(uid);
  if (!device || device->protocol != "IthoRFT" || !device->transmit_supported) {
    error = "device does not support Itho RFT transmit"; return false;
  }
  uint32_t address = 0, sequence = 0; uint8_t device_type = 0; bool paired = false;
  if (this->itho_status(uid, address, sequence, device_type, paired)) return true;
  if (!device->tx_state.empty()) {
    error = "stored Itho transmitter state is incomplete; reset is required"; return false;
  }
  auto used = [&](uint32_t candidate) {
    const std::string identity = hex_id("itho_rft", candidate, 6);
    for (const auto &other : devices_.devices()) {
      uint32_t existing = 0;
      if (const auto *text = tx::state(other, "itho_address"))
        if (tx::unsigned_decimal(*text, 0xFFFFFFu, existing) && existing == candidate) return true;
      if (std::find(other.rf_identities.begin(), other.rf_identities.end(), identity) != other.rf_identities.end()) return true;
    }
    return false;
  };
  uint32_t candidate = 0;
  for (uint8_t attempt = 0; attempt < 32 && candidate == 0; ++attempt) {
#ifdef USE_ESP32
    const uint32_t value = esp_random() & 0xFFFFFFu;
#else
    uint32_t value = 2166136261u ^ web_write_token_ ^ (uint32_t(attempt) << 16u) ^ 0x4954484Fu;
    for (const char ch : uid) { value ^= static_cast<uint8_t>(ch); value *= 16777619u; }
    value &= 0xFFFFFFu;
#endif
    if (value != 0 && !used(value)) candidate = value;
  }
  if (candidate == 0) { error = "could not allocate a unique Itho transmitter address"; return false; }
  const DeviceStore backup = devices_;
  device = devices_.find_by_uid(uid);
  device->tx_state = {{"itho_address", std::to_string(candidate), ""},
                      {"itho_sequence", "1", ""},
                      {"itho_device_type", "22", ""},  // 0x16: proven legacy RFT virtual-remote type.
                      {"itho_paired", "0", ""}};
  if (!save_devices_()) { devices_ = backup; error = "could not save Itho transmitter state"; return false; }
  return true;
}

bool RFLinkUniversal::itho_management_transmit_(const std::string &uid, bool pair,
                                                 bool compatibility, std::string &error) {
  if (!tx_feature_enabled_) { error = "transmit controls are disabled in firmware configuration"; return false; }
  if (!storage_ready_ || !storage_ok_ || (tx_sent_ && millis() - tx_last_ms_ < 600)) {
    error = "storage unavailable or transmit rate limit active"; return false;
  }
  if (!this->ensure_itho_remote_(uid, error)) return false;
  uint32_t address = 0, sequence = 0; uint8_t device_type = 0; bool paired = false;
  if (!this->itho_status(uid, address, sequence, device_type, paired)) {
    error = "Itho virtual remote state is unavailable"; return false;
  }
  if (sequence >= 0xFFFFFFFEu) { error = "Itho sequence counter is exhausted"; return false; }
  tx_engine::Plan availability;
  availability.protocol = "IthoRFT"; // native pairing uses the same trusted transport contract
  availability.transport = TxTransport::SPI_FIFO_PACKET;
  availability.kind = FrameKind::PACKET;
  availability.radio = 2;
  availability.frequency_hz = 868299900U;
  availability.repeats = pair ? 10 : 30;
  availability.repeat_gap_ms = pair ? 40 : 4;
  if (!this->native_plan_available_(availability)) {
    error = "868.30 MHz packet radio is unavailable"; return false;
  }

  const std::array<uint8_t,3> rid{{static_cast<uint8_t>(address >> 16u),
                                   static_cast<uint8_t>(address >> 8u),
                                   static_cast<uint8_t>(address)}};
  const uint8_t counter = static_cast<uint8_t>(sequence & 0xFFu);
  const auto payload = pair ? itho::build_join_payload(device_type, rid, counter, compatibility)
                            : itho::build_leave_payload(device_type, rid, counter);
  const auto packet = itho::encode_payload(payload, 0xCA);
  if (packet.size() != 63) { error = "Itho encoder produced an invalid packet"; return false; }

  tx_engine::Plan reserve;
  reserve.state_updates = {{"itho_sequence", std::to_string(sequence + 1u), ""}};
  if (!this->reserve_tx_state_(uid, reserve, error)) return false;
#ifdef USE_ESP32
  if (pair) led_.pair_start(millis()); else led_.tx(millis());
  for (uint8_t n = 0; n < availability.repeats; ++n) {
    if (packet_transmitter_->transmit_packet(packet) != cc1101::CC1101Error::NONE) {
      if (pair) led_.pair_end(millis());
      error = "packet radio rejected TX"; return false;
    }
    if (availability.repeat_gap_ms && n + 1u < availability.repeats) delay(availability.repeat_gap_ms);
  }
  if (pair) led_.pair_end(millis());
#else
  error = "packet TX requires ESP32 hardware"; return false;
#endif
  // Pairing acknowledgement is not available over this RF path. Like Somfy,
  // remember that the user requested pairing after the packet driver accepted
  // the complete burst; the UI keeps a retry action available.
  const DeviceStore backup = devices_;
  auto *device = devices_.find_by_uid(uid);
  auto state = std::find_if(device->tx_state.begin(), device->tx_state.end(), [](const FieldValue &f){ return f.key == "itho_paired"; });
  if (state == device->tx_state.end()) { devices_ = backup; error = "Itho pairing state is missing"; return false; }
  state->value = pair ? "1" : "0";
  if (!save_devices_()) { devices_ = backup; error = "could not save Itho pairing state"; return false; }
  tx_last_ms_ = millis(); tx_sent_ = true;
  this->record_tx_command(2, std::string(pair ? (compatibility ? "Itho pair compatibility" : "Itho pair") : "Itho unpair") +
                              " " + uid + " (TX requested)");
  return true;
}

bool RFLinkUniversal::itho_pair(const std::string &uid, bool compatibility, std::string &error) {
  return this->itho_management_transmit_(uid, true, compatibility, error);
}

bool RFLinkUniversal::itho_unpair(const std::string &uid, std::string &error) {
  return this->itho_management_transmit_(uid, false, false, error);
}

bool RFLinkUniversal::reserve_tx_state_(const std::string &uid, const tx_engine::Plan &plan, std::string &error) {
  if (plan.state_updates.empty()) return true;
  if (!storage_ready_ || !storage_ok_) { error = "device storage unavailable"; return false; }
  const DeviceStore backup = devices_;
  auto *device = devices_.find_by_uid(uid);
  if (!device) { error = "device not found"; return false; }
  for (const auto &update : plan.state_updates) {
    if (update.key.empty() || update.key.size() > 64 || update.value.empty() || update.value.size() > 32) {
      devices_ = backup; error = "invalid protocol state update"; return false;
    }
    auto existing = std::find_if(device->tx_state.begin(), device->tx_state.end(),
                                 [&](const FieldValue &f) { return f.key == update.key; });
    if (update.key == "somfy_rolling_code") {
      uint32_t old_value = 0, new_value = 0;
      if (existing == device->tx_state.end() ||
          !tx::unsigned_decimal(existing->value, 0xFFFFu, old_value) ||
          !tx::unsigned_decimal(update.value, 0xFFFFu, new_value) || new_value <= old_value) {
        devices_ = backup; error = "Somfy rolling counter may only move forward"; return false;
      }
    }
    if (update.key == "somfy_paired" && update.value != "0" && update.value != "1") {
      devices_ = backup; error = "invalid Somfy pairing state"; return false;
    }
    if (update.key == "itho_sequence") {
      uint32_t old_value = 0, new_value = 0;
      if (existing == device->tx_state.end() ||
          !tx::unsigned_decimal(existing->value, 0xFFFFFFFEu, old_value) ||
          !tx::unsigned_decimal(update.value, 0xFFFFFFFEu, new_value) || new_value <= old_value) {
        devices_ = backup; error = "Itho sequence may only move forward"; return false;
      }
    }
    if (update.key == "itho_paired" && update.value != "0" && update.value != "1") {
      devices_ = backup; error = "invalid Itho pairing state"; return false;
    }
    if (existing == device->tx_state.end()) {
      if (device->tx_state.size() >= 32) { devices_ = backup; error = "protocol state is full"; return false; }
      device->tx_state.push_back(update);
    } else {
      *existing = update;
    }
  }
  if (!save_devices_()) { devices_ = backup; error = "could not reserve rolling-code state"; return false; }
  return true;
}

bool RFLinkUniversal::native_transmit(const std::string &uid, const std::string &action, std::string &error) {
  system_trace("info","tx",uid,"User requested command: "+action);
  if (!tx_feature_enabled_) { error = "transmit controls are disabled in firmware configuration";system_trace("warn","tx",uid,error);return false; }
  if (!storage_ready_ || !storage_ok_ || (tx_sent_ && millis() - tx_last_ms_ < 600)) {
    error = "storage unavailable or transmit rate limit active";system_trace("warn","tx",uid,error);return false;
  }
  const auto *device = devices_.find_by_uid(uid);
  if (!device) { error = "device not found";system_trace("warn","tx",uid,error);return false; }
  if(device->protocol=="RamsesII" && !ramses_pair_lab_ready()) {error="experimental RAMSES TX requires explicit YAML lab flags";system_trace("warn","tx",uid,error);return false;}
  tx_engine::Plan plan; const tx_engine::Encoder *encoder = nullptr;
  if (!tx_engine::prepare(*device, action, plan, encoder) || !encoder) {
    error = "action or required protocol state is unavailable";
    system_trace("warn","tx",uid,"No approved decoder encoder, valid action, or required protocol state");return false;
  }
  const bool ok=this->dispatch_native_plan_(uid, action, plan, *encoder, "Web", error);
  if(!ok)system_trace("error","tx",uid,"TX failed: "+error);
  return ok;
}

// A targeted physical transport probe, not a pairing action or device command.
// It sends a non-addressed RAMSES 10E0 self-description from an ephemeral class-37
// test identity. Only exposed when both YAML lab flags are deliberately enabled.
bool RFLinkUniversal::ramses_tx_probe(std::string &error) {
  record_radio_tx_event(2,"RamsesII","TX-test aangevraagd: 10E0","",
      "Experimenteel testtelegram, niet voor het bedienen of koppelen");
  system_trace("info","lab","","Manual RAMSES radio self-announcement requested");
  if(!ramses_tx_probe_lab_ || !gdo_serial_tx_lab_){
    error="Enable both explicit GDO UART and RAMSES TX probe YAML lab flags";
    system_trace("warn","lab","",error);return false;
  }
  const uint32_t now=millis();
  if(last_ramses_probe_ms_ && static_cast<uint32_t>(now-last_ramses_probe_ms_)<15000U){
    error="Lab TX cooldown: minimum 15 seconds between test frames";
    system_trace("warn","lab","",error);return false;
  }
  // Set before acquiring: even failed TX requests must not cause rapid retries.
  last_ramses_probe_ms_=now?now:1;
  if(!serial_tx_ready()){
    error="Serial TX adapter not ready: check 868 radio, shared RX lease, UART and GDO0";
    system_trace("warn","lab","",error);return false;
  }
  uint32_t serial=0;
#ifdef USE_ESP32
  serial=esp_random()&0x3FFFFU;
#else
  serial=0x12345U;
#endif
  if(!serial)serial=1;
  const uint32_t remote=(37U<<18)|serial;
  rflink_ramses_ii::Frame message;
  std::vector<uint8_t> wire;
  if(!rflink_ramses_ii::tx::buildDeviceInfo(remote,message) ||
     !rflink_ramses_ii::tx::encodeWire(message,wire)){
    error="RAMSES encoder could not build 10E0 diagnostic self-description";
    system_trace("error","lab","",error);return false;
  }
  // Five UART idle/preamble bytes, same as the existing codec's host fixtures.
  wire.insert(wire.begin(),5,0xFF);
  system_trace("info","lab","","10E0 frame encoded; radio=868.30 MHz; UART=38400 8N1; 1 repeat");
  if(!serial_tx_adapter_->send(2,"fsk_86830_ramses_gdo_serial_lab",
                                wire.data(),wire.size(),1,0,error)) {
    system_trace("error","lab","",std::string("Physical GDO TX failed / RX restore: ")+error);
    return false;
  }
  system_trace("info","lab","","UART driver reported completion AND RX restoration; RF on air NOT confirmed");
  record_radio_tx_event(2,"RamsesII","TX-test door driver afgerond: 10E0","",
      "Driver reported TX completion and RX restoration; no receiver acknowledgement");
  return true;
}

// RAMSES-II pairing is an experimental, device-specific workflow; only the
// codec knows the protocol. CC1101/GDO is accessed through the same trusted
// protocol-independent TX route used by normal decoder commands.
std::string RFLinkUniversal::ramses_pair_stage(const std::string &uid) const {
  const auto *d=devices_.find_by_uid(uid);
  if(!d || d->protocol!="RamsesII")return "unavailable";
  if(ramses_op_state_.load(std::memory_order_acquire)==2 &&
     std::string(ramses_op_uid_.data())==uid)return "queued";
  if(ramses_pair_uid_==uid){
    if(ramses_pair_phase_==RamsesPairPhase::WAIT_ACK)return "waiting_ack";
    if(ramses_pair_phase_==RamsesPairPhase::CONFIRM_PENDING)return "confirming";
    if(ramses_pair_phase_==RamsesPairPhase::AWAIT_USER)return "awaiting_user";
    if(ramses_pair_phase_==RamsesPairPhase::FAILED)return "failed";
  }
  const auto *p=tx::state(*d,"ramses_paired");
  return p && *p=="1" ? "paired" : "not_paired";
}

bool RFLinkUniversal::ramses_send_management_(const rflink_ramses_ii::Frame &frame,
                         const std::string &uid,const char *stage,std::string &error){
  tx_engine::Plan plan;
  plan.protocol="RamsesII";plan.kind=FrameKind::PACKET;plan.radio=2;
  plan.transport=TxTransport::GDO_SERIAL;
  // Management packets bypass tx_engine::prepare() (no learned action exists yet).
  // They MUST still carry the manifest-approved physical frequency, otherwise
  // native_route_for_ rejects the plan even with a healthy idle 868-MHz radio.
  plan.frequency_hz=868299900U;
  record_radio_tx_event(2,"RamsesII",std::string("TX voorbereiden: ")+stage,uid,
      "868.30 MHz; RAMSES-II; GDO/UART; payload details hidden");
  if(native_route_for_(plan)!=TxTransport::GDO_SERIAL || !serial_tx_adapter_){
    error="approved 868.30 MHz GDO serial transmitter is unavailable/busy";
    const char *radio_state=radio_mode_868_.itho_ready()?"itho_rx":
       radio_mode_868_.state()==RadioModeManager::State::FAULT?"fault":"busy";
    record_radio_tx_event(2,"RamsesII",std::string("TX afgewezen: ")+stage,uid,
        std::string("Route unavailable/busy; radio=")+radio_state+
        ", channel_available="+(channels_[2].available?"yes":"no")+
        ", tx_ready="+(channels_[2].tx_ready?"yes":"no")+
        ", serial_ready="+(serial_tx_ready()?"yes":"no"),"error");
    system_trace("error","pair",uid,std::string(stage)+": TX route rejected; radio="+radio_state+
      ", channel_available="+(channels_[2].available?"yes":"no")+
      ", tx_ready="+(channels_[2].tx_ready?"yes":"no")+
      ", serial_adapter="+(serial_tx_adapter_?"registered":"missing")+
      ", serial_ready="+(serial_tx_ready()?"yes":"no")+
      "; "+error);
    return false;
  }
  std::vector<uint8_t> wire;
  if(!rflink_ramses_ii::tx::encodeWire(frame,wire)){
    error="RAMSES encoder rejected management frame";
    record_radio_tx_event(2,"RamsesII",std::string("TX afgewezen: ")+stage,uid,error,"error");return false;
  }
  wire.insert(wire.begin(),5,0xFF);
  if(!serial_wire_sane(wire,1,0)){
    error="RAMSES management frame exceeds safe UART limits";
    record_radio_tx_event(2,"RamsesII",std::string("TX afgewezen: ")+stage,uid,error,"error");return false;
  }
  system_trace("info","pair",uid,std::string(stage)+": serialized RAMSES frame; requesting radio TX");
  record_radio_tx_event(2,"RamsesII",std::string("TX aan driver aangeboden: ")+stage,uid,
      "GDO/UART adapter called; RF emission not confirmed until CC1101 enters TX");
  const auto *approved=radio_v2_catalog::tx_for("RamsesII");
  if(!approved || !serial_tx_adapter_->send(2,approved->rf_profile,wire.data(),wire.size(),1,0,error)){
    record_radio_tx_event(2,"RamsesII",std::string("TX mislukt: ")+stage,uid,
        "Hardware TX or RX restoration error: "+error,"error");
    system_trace("error","pair",uid,std::string(stage)+": serial TX/RX recovery failed: "+error);return false;
  }
  system_trace("info","pair",uid,std::string(stage)+": radio reported TX and RX restoration; NOT device execution");
  record_radio_tx_event(2,"RamsesII",std::string("TX door driver afgerond: ")+stage,uid,
      "Hardware adapter reported completion and RX recovery; device reaction NOT confirmed");
  tx_last_ms_=millis();tx_sent_=true;
  return true;
}

bool RFLinkUniversal::enqueue_ramses_operation(const std::string &uid,
                                                  bool auto_test,std::string &error) {
  if (uid.empty() || uid.size() >= ramses_op_uid_.size() ||
      uid.find('\0') != std::string::npos) {
    error = "invalid RF device ID";
    return false;
  }
  if (!ramses_pair_lab_ready()) {
    error = "enable TX controls and RAMSES GDO/UART lab flags";
    return false;
  }
  uint8_t free = 0;
  if (!ramses_op_state_.compare_exchange_strong(free,1,std::memory_order_acq_rel)) {
    error = "another experimental TX operation is pending";
    return false;
  }
  // The HTTP callback only copies a bounded ID and releases the mailbox.
  // NVS, decoder, profile resolution, UART and CC1101 remain loop-owned.
  std::copy(uid.begin(),uid.end(),ramses_op_uid_.begin());
  ramses_op_uid_[uid.size()] = '\0';
  ramses_op_kind_ = auto_test ? 2 : 1;
  ramses_op_state_.store(2,std::memory_order_release);
  return true;
}

void RFLinkUniversal::process_ramses_operation_() {
  uint8_t ready = 2;
  if (!ramses_op_state_.compare_exchange_strong(ready,3,std::memory_order_acq_rel))
    return;
  const std::string uid(ramses_op_uid_.data());
  const bool test = ramses_op_kind_ == 2;
  std::string error;
#if defined(USE_ESP32) && defined(ESP_PLATFORM)
  // ESP-IDF FreeRTOS reports stack high-water in bytes. Fail closed if the
  // ESPHome loop already has insufficient headroom for a blocking TX session.
  const size_t high_water = uxTaskGetStackHighWaterMark(nullptr);
  if (high_water < 1536U) {
    error = "ESPHome loop stack headroom too low for experimental TX (" +
            std::to_string(high_water) + " bytes)";
  }
  system_trace(error.empty()?"info":"error","tx",uid,
               std::string("Loop task TX dispatch; min free stack=") +
               std::to_string(high_water) + " bytes");
#endif
  const bool ok = error.empty() &&
         (test ? ramses_pair_test(uid,error) : ramses_pair_start(uid,error));
  if (!ok) {
    system_trace("error","pair",uid,
                 std::string(test?"AUTO test":"1FC9 pairing")+
                 " rejected in loop: " + error);
    record_radio_tx_event(2,"RamsesII",
                 test?"TX-test afgewezen: AUTO":"Koppelverzoek mislukt: 1FC9",
                 uid,error,"error");
  }
  ramses_op_state_.store(0,std::memory_order_release);
}

bool RFLinkUniversal::ramses_pair_start(const std::string &uid,std::string &error){
  record_radio_tx_event(2,"RamsesII","Koppelverzoek aangevraagd: 1FC9",uid,
      "Gebruiker start experimentele Orcon-koppeling; veiligheids- en radiocontroles volgen");
  if(!ramses_pair_lab_ready()){error="enable TX controls, GDO UART lab, RAMSES probe lab and RAMSES RX lab in YAML";return false;}
  if(!storage_ready_ || !storage_ok_){error="device storage unavailable";return false;}
  if(ramses_pair_phase_==RamsesPairPhase::WAIT_ACK || ramses_pair_phase_==RamsesPairPhase::CONFIRM_PENDING ||
     ramses_pair_phase_==RamsesPairPhase::AWAIT_USER){
    error="another RAMSES pairing session is already active";return false;
  }
  const auto *d=devices_.find_by_uid(uid);
  if(!d || d->protocol!="RamsesII" || d->profile_id!="orcon_mvs15r_15rf"){
    error="choose the reviewed Orcon MVS-15R / 15RF profile first";return false;
  }
  if(const auto *paired=tx::state(*d,"ramses_paired"))if(*paired=="1"){error="device already paired; no automatic replacement of existing controller";return false;}
  if(!serial_tx_ready()){error="GDO UART adapter unavailable or another RX session owns 868 MHz";return false;}
  uint32_t controller=0;
  const auto *oldcontroller=tx::state(*d,"ramses_controller");
  if(oldcontroller){
    if(!tx::unsigned_decimal(*oldcontroller,0xFFFFFFU,controller) ||
        rflink_ramses_ii::deviceClass(controller)!=37 ||
        !tx::state(*d,"ramses_fan") || !tx::state(*d,"ramses_paired")){
      error="incomplete persistent RAMSES state; refusing to overwrite virtual remote";return false;
    }
  } else {
    // RX remote IDs must NEVER be reused for our independent virtual controller.
    for(unsigned tries=0;tries<48 && !controller;++tries){
#ifdef USE_ESP32
      const uint32_t candidate=(37U<<18)|(esp_random()&0x3FFFFU);
#else
      const uint32_t candidate=(37U<<18)|((web_write_token_+0x1A42u+tries*65521u)&0x3FFFFU);
#endif
      bool used=(candidate==(37U<<18));
      for(const auto &other:devices_.devices()){
        if(const auto *text=tx::state(other,"ramses_controller")){
          uint32_t existing=0;if(tx::unsigned_decimal(*text,0xFFFFFFU,existing)&&existing==candidate)used=true;
        }
        // Don't duplicate the learned source even in another device record.
        for(const auto &rf:other.rf_identities){
          if(rf.compare(0,10,"ramses:37:")==0){
            uint32_t rxserial=0;
            if(tx::unsigned_decimal(rf.substr(10),0x3FFFFU,rxserial) &&
               rxserial==(candidate&0x3FFFFU))used=true;
          }
        }
      }
      if(!used)controller=candidate;
    }
    if(!controller){error="unable to allocate unique controller ID";return false;}
    const DeviceStore backup=devices_;
    auto *mutable_device=devices_.find_by_uid(uid);
    mutable_device->tx_state.push_back({"ramses_controller",std::to_string(controller),""});
    mutable_device->tx_state.push_back({"ramses_fan","0",""});
    mutable_device->tx_state.push_back({"ramses_paired","0",""});
    mutable_device->transmit_supported=true;
    mutable_device->pairing_supported=true;
    if(!save_devices_()){devices_=backup;error="could not persist unique controller before first transmission";return false;}
  }
  // Upgrade legacy flags *before* attempting TX. Never send an offer and then
  // discover we could not persist the state that identifies our controller.
  if(!devices_.find_by_uid(uid)->transmit_supported ||
     !devices_.find_by_uid(uid)->pairing_supported){
    const DeviceStore backup=devices_;
    devices_.find_by_uid(uid)->transmit_supported=true;
    devices_.find_by_uid(uid)->pairing_supported=true;
    if(!save_devices_()){devices_=backup;error="could not upgrade legacy device TX flags";return false;}
  }
  rflink_ramses_ii::Frame offer;
  if(!rflink_ramses_ii::tx::buildBindOffer(controller,offer)){error="invalid RAMSES bind offer";return false;}
  if(!ramses_send_management_(offer,uid,"1FC9 offer",error))return false;
  ramses_pair_uid_=uid;ramses_pair_controller_=controller;ramses_pair_fan_=0;
  ramses_pair_test_sent_=false;ramses_pair_last_test_ms_=0;
  ramses_pair_started_ms_=millis();ramses_pair_next_rx_ms_=0;
  ramses_pair_phase_=RamsesPairPhase::WAIT_ACK;
  system_trace("info","pair",uid,"1FC9 offer transmitted; waiting for ACK addressed to virtual remote");
  if(!request_decoder_rx_window("RamsesII",25000))
    system_trace("warn","pair",uid,"RX lease not started immediately; will retry while pairing session is active");
  return true;
}

void RFLinkUniversal::ramses_pair_receive_ack_(const rflink_ramses_ii::Frame &frame,uint32_t now){
  if(ramses_pair_phase_!=RamsesPairPhase::WAIT_ACK)return;
  uint32_t fan=0;
  if(!rflink_ramses_ii::tx::isBindAcknowledgement(frame,ramses_pair_controller_,fan))return;
  ramses_pair_fan_=fan;ramses_pair_phase_=RamsesPairPhase::CONFIRM_PENDING;
  system_trace("info","pair",ramses_pair_uid_,"Matching 1FC9 ACK received; fan ID verified, waiting for radio restoration");
  this->record_activity_(2,"RamsesII","RX: passend 1FC9-koppelantwoord",millis(),"RX",
                         "Apparaatantwoord komt overeen met de virtuele zender; wacht op bevestiging");
  radio_mode_868_.request_restore();
  (void)now;
}

void RFLinkUniversal::ramses_pair_poll_(uint32_t now){
  if(ramses_pair_phase_!=RamsesPairPhase::WAIT_ACK && ramses_pair_phase_!=RamsesPairPhase::CONFIRM_PENDING)return;
  if(static_cast<int32_t>(now-ramses_pair_started_ms_)>=180000){
    system_trace("warn","pair",ramses_pair_uid_,"Pairing timeout; existing remote and saved RX data retained");
    record_radio_tx_event(2,"RamsesII","Koppeling: 1FC9 antwoord uitgebleven",ramses_pair_uid_,
        "Geen passend ACK binnen wachttijd; huidige koppelingen behouden", "warn");
    radio_mode_868_.request_restore();ramses_pair_phase_=RamsesPairPhase::FAILED;return;
  }
  if(ramses_pair_phase_==RamsesPairPhase::WAIT_ACK && radio_mode_868_.itho_ready() &&
       (!ramses_pair_next_rx_ms_ || uint32_t(now-ramses_pair_next_rx_ms_)<0x80000000U)){
    if(request_decoder_rx_window("RamsesII",25000))ramses_pair_next_rx_ms_=now+26000U;
  }
  if(ramses_pair_phase_!=RamsesPairPhase::CONFIRM_PENDING || !radio_mode_868_.itho_ready())return;
  // Never transmit from the RX callback while the radio is leased to the RX backend.
  const std::string uid=ramses_pair_uid_;
  ramses_pair_phase_=RamsesPairPhase::FAILED; // fail closed on any subsequent error
  std::string error;
  rflink_ramses_ii::Frame confirm,info;
  if(!rflink_ramses_ii::tx::buildBindConfirm(ramses_pair_controller_,ramses_pair_fan_,confirm) ||
     !rflink_ramses_ii::tx::buildDeviceInfo(ramses_pair_controller_,info) ||
     !ramses_send_management_(confirm,uid,"1FC9 confirm",error) ||
     !ramses_send_management_(info,uid,"10E0 description",error)){
    system_trace("error","pair",uid,"Pairing incomplete: "+error);return;
  }
  const DeviceStore backup=devices_;
  auto *d=devices_.find_by_uid(uid);
  if(!d){system_trace("error","pair",uid,"device removed while pairing");return;}
  for(auto &field:d->tx_state)if(field.key=="ramses_fan")field.value=std::to_string(ramses_pair_fan_);
  if(!save_devices_()){devices_=backup;system_trace("error","pair",uid,"failed to persist ACK fan identity");return;}
  ramses_pair_phase_=RamsesPairPhase::AWAIT_USER;
  system_trace("info","pair",uid,"1FC9 ACK and confirmation sent; user must verify ventilation response before enabling controls");
}

bool RFLinkUniversal::ramses_pair_test(const std::string &uid,std::string &error){
  if(!ramses_pair_lab_ready() || ramses_pair_phase_!=RamsesPairPhase::AWAIT_USER ||
      ramses_pair_uid_!=uid || !ramses_pair_fan_){
    error="matching ACK and pairing confirmation required before command test";return false;
  }
  const uint32_t now=millis();
  if(ramses_pair_last_test_ms_ && uint32_t(now-ramses_pair_last_test_ms_)<15000U){
    error="minimum 15 seconds between experimental test commands";return false;
  }
  ramses_pair_last_test_ms_=now?now:1;
  rflink_ramses_ii::Frame test;
  if(!rflink_ramses_ii::tx::buildCommand({ramses_pair_controller_,ramses_pair_fan_},
         rflink_ramses_ii::tx::Action::AUTO,test)){
    error="unable to encode addressed Orcon Auto command";return false;
  }
  if(!ramses_send_management_(test,uid,"experimental AUTO test",error))return false;
  ramses_pair_test_sent_=true;
  system_trace("info","pair",uid,"AUTO test sent to ACK-identified fan; user observation still required");
  return true;
}

bool RFLinkUniversal::ramses_pair_verify(const std::string &uid,std::string &error){
  if(ramses_pair_phase_!=RamsesPairPhase::AWAIT_USER || ramses_pair_uid_!=uid){
    error="no ACK-backed pairing session awaiting device verification";return false;
  }
  if(!ramses_pair_test_sent_){error="send an explicit Auto test and observe fan reaction before verification";return false;}
  if(!ramses_pair_lab_ready() || !serial_tx_ready()){
    error="serial radio unavailable; verify RX restoration before confirming pair";return false;
  }
  const DeviceStore backup=devices_;
  auto *d=devices_.find_by_uid(uid);
  if(!d){error="device no longer available";return false;}
  auto p=std::find_if(d->tx_state.begin(),d->tx_state.end(),[](const FieldValue &f){return f.key=="ramses_paired";});
  if(p==d->tx_state.end()){error="incomplete persistent controller state";return false;}
  p->value="1";
  if(!save_devices_()){devices_=backup;error="could not store verified pairing";return false;}
  ramses_pair_phase_=RamsesPairPhase::IDLE;ramses_pair_uid_.clear();ramses_pair_test_sent_=false;
  system_trace("info","pair",uid,"Owner confirmed pairing; experimental TX buttons unlocked");
  return true;
}

bool RFLinkUniversal::somfy_pair(const std::string &uid, std::string &error) {
  if (!tx_feature_enabled_) { error = "transmit controls are disabled in firmware configuration"; return false; }
  if (!this->ensure_somfy_remote_(uid, error)) return false;
  led_.pair_start(millis());
  const bool ok = this->native_transmit(uid, "prog", error);
  led_.pair_end(millis());
  return ok;
}

bool RFLinkUniversal::set_transmit_enabled(const std::string &uid, bool enabled) {
#ifdef USE_MQTT
  if (!storage_ready_ || !storage_ok_ || !this->mqtt_connected()) return false;
  auto *device=devices_.find_by_uid(uid);
  if (!device || !device->home_assistant_enabled ||
      (enabled && (!this->tx_eligible(uid) || ha_tx_session_.empty()))) return false;
  if (device->transmit_enabled == enabled) return true;
  const DeviceStore backup=devices_;
  const auto *encoder=tx_engine::resolve(device->protocol);
  if (!encoder) return false;
  if (enabled) {
    std::vector<FieldValue> snapshot;
    if (!encoder->capture(*device,snapshot) || snapshot.empty() ||
        device->fields.size()+snapshot.size()>32) return false;
    for (const auto &entry:snapshot) {
      if (entry.key.compare(0,8,"tx_lock_")!=0 || tx::field(*device,entry.key.c_str())) return false;
    }
    device->fields.insert(device->fields.end(),snapshot.begin(),snapshot.end());
    device->transmit_enabled=true;
  }
  // All TX buttons are derived from the encoder, never from a receive-only
  // manifest or a received unapproved action. Leave sensor/event topics alone.
  auto publish_buttons=[this](const DeviceRecord &record, bool active)->bool {
    for (const auto &action:record.actions) {
      if (record.protocol == "SomfyRTS56" && action == "prog") continue;
      tx_engine::Plan plan; const tx_engine::Encoder *impl=nullptr;
      if (!tx_engine::prepare(record,action,plan,impl)) continue;
      if (!mqtt::global_mqtt_client->publish(ha::button_topic(ha_node_,record,action),
          active ? ha::button_discovery(ha_node_,record,action,ha_tx_session_) : std::string(),1,true)) return false;
    }
    return true;
  };
  if (!publish_buttons(*device,enabled)) {
    devices_=backup;
    const auto *old=devices_.find_by_uid(uid);
    if (old) publish_buttons(*old,!enabled); // best effort reversal after partial MQTT publication
    return false;
  }
  if (!enabled) {
    device->transmit_enabled=false;
    auto &fields=device->fields;
    fields.erase(std::remove_if(fields.begin(),fields.end(),[](const FieldValue &x){
      return x.key.compare(0,8,"tx_lock_")==0;
    }),fields.end());
  }
  if (!save_devices_()) {
    devices_=backup;
    const auto *old=devices_.find_by_uid(uid);
    if (old) publish_buttons(*old,!enabled);
    return false;
  }
  ha_discovery_cache_.erase(uid);
  return true;
#else
  (void)uid; (void)enabled;
  return false;
#endif
}

bool RFLinkUniversal::transmit_request(const std::string &uid, const std::string &action,
                                       const std::string &session) {
  if (!tx_feature_enabled_ || !this->mqtt_connected() || ha_tx_session_.empty() || session!=ha_tx_session_ ||
      !storage_ready_ || !storage_ok_ || (tx_sent_ && millis()-tx_last_ms_<600)) return false;
  const DeviceRecord *device=devices_.find_by_uid(uid);
  if (!device || !device->home_assistant_enabled) return false;
  if (device->local_transmit_enabled && this->effective_user_decoder(device->protocol)) {
    std::string error;
    return this->user_transmit(uid, action, error);
  }
  if (!device->transmit_enabled) return false;
  tx_engine::Plan plan; const tx_engine::Encoder *encoder=nullptr;
  if (!tx_engine::prepare(*device,action,plan,encoder) || !encoder) return false;
  std::string error;
  return this->dispatch_native_plan_(uid, action, plan, *encoder, "HA", error);
}

void RFLinkUniversal::ha_command_(const std::string &topic, const std::string &payload) {
  if (ha_tx_session_.empty() || payload!=ha_tx_session_) return; // stale/retained payload
  const std::string prefix="rflink/" + ha::safe_node(ha_node_) + "/";
  if (topic.compare(0,prefix.size(),prefix)!=0 || topic.size()<prefix.size()+7 ||
      topic.compare(topic.size()-4,4,"/set")!=0) return;
  const std::string path=topic.substr(prefix.size(),topic.size()-prefix.size()-4);
  const auto slash=path.find('/');
  if (slash==std::string::npos || path.find('/',slash+1)!=std::string::npos) return;
  const auto uid=path.substr(0,slash), action=path.substr(slash+1);
  const bool accepted=this->transmit_request(uid,action,payload);
  ESP_LOGI(TAG,"HA TX %s uid=%s action=%s",accepted?"queued":"refused",uid.c_str(),action.c_str());
}

bool RFLinkUniversal::mqtt_runtime_supported() const {
#ifdef USE_MQTT
  return mqtt::global_mqtt_client != nullptr;
#else
  return false;
#endif
}

bool RFLinkUniversal::mqtt_runtime_enabled() const {
#ifdef USE_ESP32
  return mqtt_prefs_loaded_ ? mqtt_prefs_.enabled != 0 : this->mqtt_connected();
#else
  return this->mqtt_connected();
#endif
}

bool RFLinkUniversal::mqtt_runtime_has_password() const {
#ifdef USE_ESP32
  return mqtt_prefs_loaded_ && mqtt_prefs_.password[0] != 0;
#else
  return false;
#endif
}
std::string RFLinkUniversal::mqtt_runtime_host() const {
#ifdef USE_ESP32
  return mqtt_prefs_loaded_ ? mqtt_prefs_.host : "";
#else
  return "";
#endif
}
std::string RFLinkUniversal::mqtt_runtime_user() const {
#ifdef USE_ESP32
  return mqtt_prefs_loaded_ ? mqtt_prefs_.user : "";
#else
  return "";
#endif
}
uint16_t RFLinkUniversal::mqtt_runtime_port() const {
#ifdef USE_ESP32
  return mqtt_prefs_loaded_ ? mqtt_prefs_.port : 1883;
#else
  return 1883;
#endif
}
bool RFLinkUniversal::mqtt_runtime_save(const std::string &host, uint16_t port,
                                        const std::string &user, const std::string &password, bool enabled) {
#if defined(USE_ESP32) && defined(USE_MQTT)
  if (!this->mqtt_runtime_supported() || global_preferences == nullptr || port == 0 ||
      host.size() >= sizeof(mqtt_prefs_.host) || user.size() >= sizeof(mqtt_prefs_.user) ||
      password.size() >= sizeof(mqtt_prefs_.password) || (enabled && host.empty())) return false;
  // Host is a DNS name or IP, not a URL. Disallow line breaks/whitespace/control chars.
  for (unsigned char ch : host) if (!(std::isalnum(ch) || ch == '.' || ch == '-' || ch == ':' || ch == '_')) return false;
  for (unsigned char ch : user) if (ch < 32 || ch == 127) return false;
  for (unsigned char ch : password) if (ch < 32 || ch == 127) return false;
  MqttPrefs next{};
  next.marker = 0x52464D34u;
  next.port = port;
  next.enabled = enabled ? 1 : 0;
  std::memcpy(next.host, host.c_str(), host.size());
  std::memcpy(next.user, user.c_str(), user.size());
  if (!password.empty()) std::memcpy(next.password, password.c_str(), password.size());
  else if (mqtt_prefs_loaded_) std::memcpy(next.password, mqtt_prefs_.password, sizeof(next.password));
  if (!mqtt_settings_pref_.save(&next) || !global_preferences->sync()) return false;
  mqtt_prefs_ = next;
  mqtt_prefs_loaded_ = true;
  auto *client = mqtt::global_mqtt_client;
  if (client == nullptr) return false;
  client->disable();
  this->invalidate_tx_session();
  client->set_broker_address(mqtt_prefs_.host);
  client->set_broker_port(mqtt_prefs_.port);
  client->set_username(mqtt_prefs_.user);
  client->set_password(mqtt_prefs_.password);
  client->set_enable_on_boot(enabled);
  if (enabled) client->enable();
  return true;
#else
  (void) host; (void) port; (void) user; (void) password; (void) enabled;
  return false;
#endif
}

bool RFLinkUniversal::mqtt_configured() const {
#ifdef USE_MQTT
  return mqtt::global_mqtt_client != nullptr;
#else
  return false;
#endif
}

bool RFLinkUniversal::mqtt_connected() const {
#ifdef USE_MQTT
  return mqtt::global_mqtt_client != nullptr && mqtt::global_mqtt_client->is_connected();
#else
  return false;
#endif
}

bool RFLinkUniversal::ha_discovery_(const DeviceRecord &device, bool remove, bool force) {
#ifdef USE_MQTT
  if (!this->mqtt_connected()) return false;
  const std::string sig = ha::signature(device);
  auto it = ha_discovery_cache_.find(device.uid);
  if (!remove && !force && it != ha_discovery_cache_.end() && it->second == sig) return true;
  auto *client = mqtt::global_mqtt_client;
  // Only supported, meaningful sensor fields become HA entities. No unverified
  // control/switch/cover entities are created from receive-only RF remotes.
  for (const auto &field : device.fields) {
    const auto *p = ha::profile(field.key);
    if (p == nullptr) continue;
    const std::string topic = ha::discovery_topic(ha_node_, device, field.key);
    const std::string data = remove ? std::string() : ha::field_discovery(ha_node_, device, *p);
    if (!client->publish(topic, data, 1, true)) { ha_discovery_cache_.erase(device.uid); return false; }
  }
  if (!device.actions.empty()) {
    const std::string topic = ha::discovery_topic(ha_node_, device, "action");
    // Also tombstone retained legacy "Knoppen" entities created for weather
    // packet-kind strings by older HA1 firmware. Sensor actions are internal.
    const std::string data = remove ? std::string() : ha::event_discovery(ha_node_, device);
    if (!client->publish(topic, data, 1, true)) { ha_discovery_cache_.erase(device.uid); return false; }
  }
  if (device.transmit_enabled && tx_engine::eligible(device)) {
    for (const auto &action : device.actions) {
      if (device.protocol == "SomfyRTS56" && action == "prog") continue;
      tx_engine::Plan plan; const tx_engine::Encoder *encoder=nullptr;
      if (!tx_engine::prepare(device,action,plan,encoder)) continue;
      const std::string topic = ha::button_topic(ha_node_, device, action);
      const std::string data = (remove || !tx_feature_enabled_) ? std::string() : ha::button_discovery(ha_node_, device, action, ha_tx_session_);
      if (!client->publish(topic, data, 1, true)) { ha_discovery_cache_.erase(device.uid); return false; }
    }
  }
  if (device.protocol.rfind("user_", 0) == 0) {
    const auto *recipe = this->effective_user_decoder(device.protocol);
    for (const auto &action : device.actions) {
      const std::string topic = ha::button_topic(ha_node_, device, action);
      const bool allowed = !remove && tx_feature_enabled_ && device.local_transmit_enabled &&
                           recipe && recipe->enabled && !recipe->tx_source.empty() &&
                           std::find(recipe->actions.begin(), recipe->actions.end(), action) != recipe->actions.end();
      const std::string data = allowed ? ha::button_discovery(ha_node_, device, action, ha_tx_session_) : std::string();
      if (!client->publish(topic, data, 1, true)) { ha_discovery_cache_.erase(device.uid); return false; }
    }
  }
  if (remove) ha_discovery_cache_.erase(device.uid);
  else ha_discovery_cache_[device.uid] = sig;
  return true;
#else
  (void)device; (void)remove; (void)force;
  return false;
#endif
}

bool RFLinkUniversal::ha_remove_capability_(const DeviceRecord &device, const std::string &kind,
                                            const std::string &key) {
#ifdef USE_MQTT
  if (!this->mqtt_connected()) return false;
  // Removing one sensor may never unregister the other entities of a device.
  // The event entity is removed only when its FINAL approved action is deleted.
  if (kind == "action" && device.local_transmit_enabled && device.protocol.rfind("user_", 0) == 0 &&
      !mqtt::global_mqtt_client->publish(ha::button_topic(ha_node_,device,key), "", 1, true)) return false;
  if (kind == "action" && device.transmit_enabled) {
    tx_engine::Plan plan; const tx_engine::Encoder *encoder=nullptr;
    if (tx_engine::prepare(device,key,plan,encoder) &&
        !mqtt::global_mqtt_client->publish(ha::button_topic(ha_node_,device,key), "", 1, true)) return false;
  }
  if (kind == "field" && ha::profile(key) == nullptr) return true;
  if (kind == "action" && device.actions.size() > 1) return true;
  return mqtt::global_mqtt_client->publish(
      ha::discovery_topic(ha_node_, device, kind == "action" ? "action" : key), "", 1, true);
#else
  (void)device; (void)kind; (void)key;
  return false;
#endif
}

bool RFLinkUniversal::change_capability(const std::string &uid, const std::string &kind,
                                        const std::string &key, bool approve) {
  if (!storage_ready_ || !storage_ok_ || key.empty() || key.size() > 255 ||
      (kind != "action" && kind != "field")) return false;
  const DeviceRecord *found = devices_.find_by_uid(uid);
  if (found == nullptr || key.compare(0,8,"tx_lock_")==0) return false;
  if (found->transmit_enabled && kind == "field" &&
      (key == "channel" || key == "group" || key == "level" || key == "tx_radio" ||
       key.compare(0,13,"check_button_")==0 ||
       key == "protocol" || key == "code" || key == "bit_length")) return false;
  const DeviceRecord before = *found;
  // Removing an approved HA entity requires a live broker to publish its
  // retained tombstone. Refuse an offline change instead of creating ghosts.
  if (!approve && before.home_assistant_enabled && !this->mqtt_connected()) return false;
  const auto backup = devices_;
  const bool changed = kind == "action"
      ? (approve ? devices_.approve_action(uid, key) : devices_.remove_action(uid, key))
      : (approve ? devices_.approve_field(uid, key) : devices_.remove_field(uid, key));
  if (!changed) return false;
  if (!approve && before.home_assistant_enabled && !this->ha_remove_capability_(before, kind, key)) {
    devices_ = backup;
    return false;
  }
  if (!save_devices_()) {
    devices_ = backup;
    if (!approve && before.home_assistant_enabled) this->ha_discovery_(before, false, true);
    return false;
  }
  auto *updated = devices_.find_by_uid(uid);
  ha_discovery_cache_.erase(uid); // approved entity types changed, not merely its live value
  if (updated != nullptr && updated->home_assistant_enabled && this->mqtt_connected())
    this->ha_discovery_(*updated, false, true);
  return true;
}

void RFLinkUniversal::ha_received_(const DeviceRecord &device, const DecodeResult &result) {
#ifdef USE_MQTT
  if (!device.home_assistant_enabled || !this->mqtt_connected()) return;
  if (!this->ha_discovery_(device, false)) return;
  auto *client = mqtt::global_mqtt_client;
  unsigned published_count=0;
  bool publication_failed=false;
  for (const auto &field : result.fields) {
    if (ha::profile(field.key) == nullptr) continue;
    // Only user-approved capability keys can leave RFLink. A new decoder field
    // may be seen as a pending proposal, but cannot publish itself to HA.
    const auto approved = std::find_if(device.fields.begin(), device.fields.end(),
                                      [&](const FieldValue &f) { return f.key == field.key; });
    if (approved == device.fields.end()) continue;
    // Sensor values are NEVER retained; old weather measurements must not be
    // replayed as fresh after the broker or Home Assistant restarts.
    if(client->publish(ha::state_topic(ha_node_, device, field.key), field.value, 0, false)) ++published_count;
    else publication_failed=true;
  }
  if (ha::has_events(device) && !result.action.empty() &&
      std::find(device.actions.begin(), device.actions.end(), result.action) != device.actions.end()) {
    const std::string json = "{\"event_type\":" + ha::quoted(result.action) + "}";
    if(client->publish(ha::state_topic(ha_node_, device, "action"), json, 0, false)) ++published_count;
    else publication_failed=true;
  }
  if(published_count || publication_failed)
    system_trace(publication_failed?"error":"info","ha",device.uid,
      std::string("MQTT publication: ")+std::to_string(published_count)+" event/value(s)"+
      (publication_failed?"; publish failed":"; accepted by broker client, HA not verified"));
#else
  (void)device; (void)result;
#endif
}

void RFLinkUniversal::sync_home_assistant() {
#ifdef USE_MQTT
  if (!this->mqtt_connected()) return;
  // New capability token on EVERY reconnect. Retained messages from an
  // earlier session cannot transmit after a reboot or reconnect.
  char session[32];
#ifdef USE_ESP32
  std::snprintf(session, sizeof(session), "%08lX%08lX",
                static_cast<unsigned long>(esp_random()), static_cast<unsigned long>(esp_random()));
#else
  static uint32_t sequence=0;
  std::snprintf(session, sizeof(session), "HOST%08lX%08lX",
                static_cast<unsigned long>(++sequence), static_cast<unsigned long>(millis()));
#endif
  ha_tx_session_ = session;
  if (!ha_tx_subscribed_) {
    mqtt::global_mqtt_client->subscribe("rflink/" + ha::safe_node(ha_node_) + "/+/+/set",
      [this](const std::string &topic, const std::string &payload) { this->ha_command_(topic, payload); });
    ha_tx_subscribed_ = true;
  }
  ha_discovery_cache_.clear(); // force republish after broker reconnect/restart
  for (const auto &device : devices_.devices()) {
    if (device.home_assistant_enabled) this->ha_discovery_(device, false, true);
  }
#endif
}

bool RFLinkUniversal::set_home_assistant_enabled(const std::string &uid, bool enabled) {
  if (!storage_ready_ || !storage_ok_) return false;
  auto *record = devices_.find_by_uid(uid);
  if (record == nullptr) return false;
  if (record->home_assistant_enabled == enabled) return true;
  if (enabled && !ha::has_entities(*record)) return false;
  if (!this->mqtt_connected()) return false;
  const auto old = devices_;
  if (!enabled && !this->ha_discovery_(*record, true)) return false;
  if (enabled && !this->ha_discovery_(*record, false, true)) return false;
  record->home_assistant_enabled = enabled;
  if (!save_devices_()) {
    devices_ = old;
    auto *reverted = devices_.find_by_uid(uid);
    if (enabled) this->ha_discovery_(*reverted, true); // roll back discovery
    else if (reverted && reverted->home_assistant_enabled) this->ha_discovery_(*reverted, false, true);
    return false;
  }
  system_trace("info","ha",uid,enabled?"Home Assistant publication enabled and persisted":"Home Assistant publication disabled and persisted");
  return true;
}

bool RFLinkUniversal::unlink_identity(const std::string &uid, const std::string &identity) {
  const auto *bound=devices_.find_by_uid(uid);
  if (bound && (bound->transmit_enabled || bound->local_transmit_enabled)) return false;
  if (!storage_ready_ || !storage_ok_) return false;
  const auto previous = devices_;
  if (!devices_.unlink_identity(uid, identity) || !save_devices_()) {
    devices_ = previous;
    return false;
  }
  return true;
}

bool RFLinkUniversal::replace_identity(const std::string &uid, const std::string &old_identity,
                                      const std::string &candidate_identity) {
  const auto *bound=devices_.find_by_uid(uid);
  if (bound && (bound->transmit_enabled || bound->local_transmit_enabled)) return false;
  if (!storage_ready_ || !storage_ok_) return false;
  const auto previous = devices_;
  if (!devices_.replace_identity(uid, old_identity, candidate_identity) || !save_devices_()) {
    devices_ = previous;
    return false;
  }
  return true;
}

void RFLinkUniversal::dump_config() {
  ESP_LOGCONFIG(TAG, "RFLink Universal %s [%s]:", RFLINK_FIRMWARE_VERSION, RFLINK_FIRMWARE_REVISION);
  ESP_LOGCONFIG(TAG, "  Channels: %u", channel_count_);
  ESP_LOGCONFIG(TAG, "  Dedupe: %u ms", dedupe_window_ms_);
  ESP_LOGCONFIG(TAG, "  Decoder threshold: %u%%", MIN_DECODER_CONFIDENCE);
}

uint32_t RFLinkUniversal::hash_raw_(const std::vector<int32_t> &raw) const {
  uint32_t hash = 2166136261u;
  for (const auto p : raw) {
    const uint32_t width = static_cast<uint32_t>(std::abs(p));
    const uint32_t bucket = ((width + 10u) / 20u) * 20u;
    const uint32_t token = bucket ^ (p < 0 ? 0x80000000u : 0u);
    hash ^= token;
    hash *= 16777619u;
  }
  return hash;
}

uint32_t RFLinkUniversal::hash_packet_(const std::vector<uint8_t> &packet) const {
  uint32_t hash = 2166136261u;
  for (auto b : packet) { hash ^= b; hash *= 16777619u; }
  return hash;
}

bool RFLinkUniversal::is_repeat_(uint8_t channel, uint32_t hash, uint32_t now) {
  if (channel == last_hash_channel_ && hash == last_hash_ && (now - last_hash_ms_) <= dedupe_window_ms_) {
    last_hash_ms_ = now;
    last_repeat_count_++;
    return true;
  }
  last_hash_channel_ = channel;
  last_hash_ = hash;
  last_hash_ms_ = now;
  last_repeat_count_ = 1;
  return false;
}

void RFLinkUniversal::record_activity_(uint8_t channel, const std::string &protocol, const std::string &summary,
                                        uint32_t timestamp_ms, const std::string &direction, const std::string &detail,
                                        const std::string &export_detail, bool noise_suspect,
                                        const std::string &noise_reason, const RFFrame *frame,
                                        const std::string &identity, uint8_t confidence,
                                        const std::string &level) {
  ActivityRecord rec;
  rec.timestamp_ms = timestamp_ms;
  rec.channel = channel < channel_count_ ? channels_[channel].name : std::to_string(channel);
  rec.channel_index = channel;
  rec.direction = direction;
  rec.detail = detail;
  rec.export_detail = export_detail.empty() ? detail : export_detail;
  rec.noise_suspect = noise_suspect;
  rec.noise_reason = noise_reason;
  rec.protocol = protocol;
  rec.summary = summary;
  // Capture full original export string (not the 16-byte preview) in bounded PSRAM.
  // RX and TX share the existing bounded per-radio signal queues. TX is not
  // RF noise and must survive beyond the short 32-event activity feed. Never
  // allocate a fourth unbounded buffer for TX diagnostic events.
  if ((direction == "RX" || direction == "TX") && channel < 3 && rx_log_ready()) {
    auto json_q = [](const std::string &s) {
      static constexpr char hex[] = "0123456789ABCDEF";
      std::string out; out.reserve(s.size() + 8);
      for (unsigned char c : s) {
        if (c == '"' || c == '\\') { out.push_back('\\'); out.push_back(static_cast<char>(c)); }
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else if (c < 0x20) { out += "\\u00"; out.push_back(hex[c >> 4]); out.push_back(hex[c & 0x0F]); }
        else out.push_back(static_cast<char>(c));
      }
      return out;
    };
    std::string event;
    event.reserve(rec.export_detail.size() + rec.detail.size() + 256);
    event = "{\"channel_index\":" + std::to_string(channel) + ",\"channel\":\"" + json_q(rec.channel) +
      "\",\"direction\":\"" + json_q(direction) + "\",\"protocol\":\"" + json_q(protocol) +
      "\",\"summary\":\"" + json_q(summary) + "\",\"level\":\"" + json_q(level) +
      "\",\"detail\":\"" + json_q(detail) +
      "\",\"full_detail\":\"" + json_q(rec.export_detail) +
      "\",\"noise_suspect\":" + (noise_suspect ? "true" : "false") +
      ",\"noise_reason\":\"" + json_q(noise_reason) + "\",\"timestamp_ms\":" +
      std::to_string(timestamp_ms) + ",\"identity\":\"" + json_q(identity) +
      "\",\"confidence\":" + std::to_string(unsigned(confidence));
    if (frame) {
      event += ",\"frame_kind\":\"" + std::string(frame->kind == FrameKind::RAW ? "RAW" : "PACKET") + "\"";
      event += ",\"length\":" + std::to_string(frame->kind == FrameKind::RAW && frame->raw != nullptr ?
                    frame->raw->size() : (frame->packet != nullptr ? frame->packet->size() : 0));
      event += ",\"rssi_dbm\":" + std::string(frame->has_rssi ? std::to_string(frame->rssi_dbm) : "null");
      event += ",\"lqi\":" + std::string(frame->has_lqi ? std::to_string(unsigned(frame->lqi)) : "null");
      event += ",\"freqest_hz\":" + std::string(frame->kind == FrameKind::PACKET ?
                    std::to_string(frame->frequency_offset_hz) : "null");
    }
    event += "}";
#if defined(USE_ESP32) && defined(ESP_PLATFORM)
    // Never wait for RF noise; give TX diagnostic records a short chance to
    // acquire the logger lock so an important rejection isn't silently lost.
    const TickType_t log_wait = direction == "TX" ? pdMS_TO_TICKS(10) : 0;
    if (rx_log_mutex_ && xSemaphoreTake(rx_log_mutex_, log_wait) == pdTRUE) {
      rx_log_->add(channel, noise_suspect ? rflink_logging::Kind::Noise : rflink_logging::Kind::Signal,
                   0, timestamp_ms, event);
      xSemaphoreGive(rx_log_mutex_);
    } else ++rx_log_busy_drops_;
#else
    rx_log_->add(channel, noise_suspect ? rflink_logging::Kind::Noise : rflink_logging::Kind::Signal,
                 0, timestamp_ms, event);
#endif
  }
  if (rx_log_ready() && direction == "RX") rec.export_detail = rec.detail;
  activity_.insert(activity_.begin(), std::move(rec));
  if (activity_.size() > 32) activity_.resize(32);
}

rflink_logging::Counters RFLinkUniversal::rx_log_counters() const {
  if (!rx_log_ready()) return {};
#if defined(USE_ESP32) && defined(ESP_PLATFORM)
  if (!rx_log_mutex_ || xSemaphoreTake(rx_log_mutex_, pdMS_TO_TICKS(100)) != pdTRUE) return {};
#endif
  const auto value = rx_log_->counters();
#if defined(USE_ESP32) && defined(ESP_PLATFORM)
  xSemaphoreGive(rx_log_mutex_);
#endif
  return value;
}

rflink_logging::Limits RFLinkUniversal::rx_log_limits() const {
  if (!rx_log_ready()) return {};
#if defined(USE_ESP32) && defined(ESP_PLATFORM)
  if (!rx_log_mutex_ || xSemaphoreTake(rx_log_mutex_, pdMS_TO_TICKS(100)) != pdTRUE) return {};
#endif
  const auto value = rx_log_->limits();
#if defined(USE_ESP32) && defined(ESP_PLATFORM)
  xSemaphoreGive(rx_log_mutex_);
#endif
  return value;
}

std::vector<rflink_logging::Entry> RFLinkUniversal::rx_log_read(uint64_t sequence,
                            const rflink_logging::Filter &filter, size_t limit, bool ascending, bool include_json) const {
  if (!rx_log_ready()) return {};
#if defined(USE_ESP32) && defined(ESP_PLATFORM)
  if (!rx_log_mutex_ || xSemaphoreTake(rx_log_mutex_, pdMS_TO_TICKS(100)) != pdTRUE) return {};
#endif
  auto records = ascending ? rx_log_->read_after(sequence, filter, limit, include_json)
                           : rx_log_->read_before(sequence, filter, limit, include_json);
#if defined(USE_ESP32) && defined(ESP_PLATFORM)
  xSemaphoreGive(rx_log_mutex_);
#endif
  return records;
}

bool RFLinkUniversal::rx_log_copy_json(uint64_t sequence, char *dst, size_t capacity, size_t &written) const {
  written = 0;
  if (!rx_log_ready()) return false;
#if defined(USE_ESP32) && defined(ESP_PLATFORM)
  if (!rx_log_mutex_ || xSemaphoreTake(rx_log_mutex_, pdMS_TO_TICKS(100)) != pdTRUE) return false;
#endif
  const bool ok = rx_log_->copy_json(sequence, dst, capacity, written);
#if defined(USE_ESP32) && defined(ESP_PLATFORM)
  xSemaphoreGive(rx_log_mutex_);
#endif
  return ok;
}

bool RFLinkUniversal::rx_log_set_limits(const rflink_logging::Limits &limits) {
  if (!rx_log_ready()) return false;
#if defined(USE_ESP32) && defined(ESP_PLATFORM)
  if (!rx_log_mutex_ || xSemaphoreTake(rx_log_mutex_, pdMS_TO_TICKS(100)) != pdTRUE) return false;
  const bool ok = rflink_logging::commit_limits_to_nvs(*rx_log_, limits);
  xSemaphoreGive(rx_log_mutex_);
  return ok;
#else
  return rx_log_->set_limits(limits);
#endif
}

bool RFLinkUniversal::classify_noise_(const RFFrame &frame, uint8_t decoder_confidence,
                                      uint16_t item_count, std::string &reason) const {
  reason.clear();
  if (decoder_confidence >= MIN_DECODER_CONFIDENCE) return false;
  if (frame.kind == FrameKind::PACKET && frame.packet != nullptr) {
    // The currently selected packet profile is Itho RFT: fixed 63-byte frames
    // with a strict preamble/header validated by the decoder. A full FIFO-sized
    // frame that does not match any decoder is retained for diagnostics/export,
    // but kept out of the normal unknown-device flow to avoid learning RF noise.
    if (frame.channel < channel_count_ && frame.packet->size() == 63 &&
        channels_[frame.channel].name.find("Itho") != std::string::npos) {
      reason = "63-byte packet does not match the active Itho profile";
      return true;
    }
    return false;
  }
  if (frame.kind == FrameKind::RAW && frame.raw != nullptr) {
    if (item_count <= 5) {
      reason = "raw capture has too few transitions";
      return true;
    }
    uint64_t total_us = 0;
    for (const auto pulse : *frame.raw) {
      const int64_t value = pulse;
      total_us += static_cast<uint64_t>(value < 0 ? -value : value);
    }
    if (total_us < 1000) {
      reason = "raw capture is shorter than 1 ms";
      return true;
    }
  }
  return false;
}

std::string RFLinkUniversal::full_frame_detail_(const RFFrame &frame, uint32_t hash, uint16_t item_count) const {
  std::string out;
  if (frame.kind == FrameKind::RAW && frame.raw != nullptr) {
    char info[72];
    std::snprintf(info, sizeof(info), "RAW hash %08" PRIX32 " | %u pulses | ", hash, unsigned(item_count));
    out = info;
    // RXLOG1: preserve every pulse. The log store rejects oversized events as a whole,
    // rather than silently truncating samples needed for decoder development.
    const size_t limit = frame.raw->size();
    for (size_t i = 0; i < limit; ++i) {
      if (i) out += ',';
      out += std::to_string((*frame.raw)[i]);
    }

  } else if (frame.packet != nullptr) {
    static constexpr char hex[] = "0123456789ABCDEF";
    out.reserve(frame.packet->size() * 3 + 96);
    out += "PACKET " + std::to_string(frame.packet->size()) + " bytes | ";
    for (size_t i = 0; i < frame.packet->size(); ++i) {
      if (i) out += ' ';
      const uint8_t byte = (*frame.packet)[i];
      out += hex[byte >> 4];
      out += hex[byte & 15];
    }
  }
  if (frame.has_rssi) {
    char meta[96];
    std::snprintf(meta, sizeof(meta), " | RSSI %.1f dBm", frame.rssi_dbm);
    out += meta;
  }
  if (frame.has_lqi) {
    out += " | LQI " + std::to_string(unsigned(frame.lqi));
  }
  if (frame.kind == FrameKind::PACKET) {
    char offset[64];
    std::snprintf(offset, sizeof(offset), " | FREQEST %.0f Hz", frame.frequency_offset_hz);
    out += offset;
  }
  return out;
}

void RFLinkUniversal::process_frame_(const RFFrame &frame, uint32_t hash, uint16_t item_count) {
  total_frames_++;
  if (frame.channel < channel_count_) {
    channels_[frame.channel].frames++;
    channels_[frame.channel].last_seen_ms = frame.received_ms;
    if (frame.has_rssi) {
      channels_[frame.channel].has_rssi = true;
      channels_[frame.channel].last_rssi_dbm = frame.rssi_dbm;
    }
    if (frame.has_lqi) {
      channels_[frame.channel].has_lqi = true;
      channels_[frame.channel].last_lqi = frame.lqi;
      channels_[frame.channel].has_frequency_offset = true;
      channels_[frame.channel].last_frequency_offset_hz = frame.frequency_offset_hz;
    }
  }

  if (is_repeat_(frame.channel, hash, frame.received_ms)) return;

  // V2 route-aware dispatch: do not discard valid lower-ranked matches just
  // because another decoder has a higher (but invalid) match score.
  auto dispatch = DecoderDispatchV2::run(frame, decoder_registry(),
                                         radio_v2_catalog::index(), MIN_DECODER_CONFIDENCE);
  RFDecoder *best = dispatch.top_candidate;
  uint8_t confidence = dispatch.recognized ? dispatch.confidence : dispatch.top_score;
  // Keep the normal activity view compact. Full packet bytes / raw pulse data
  // are retained only in the bounded 32-event history and exposed on demand
  // through the export endpoint, so status polling stays small.
  std::string detail;
  if (frame.kind == FrameKind::RAW) {
    char info[64];
    std::snprintf(info, sizeof(info), "RAW hash %08" PRIX32 " | %u pulses", hash, unsigned(item_count));
    detail = info;
  } else if (frame.packet != nullptr) {
    static constexpr char hex[] = "0123456789ABCDEF";
    const size_t n = std::min<size_t>(frame.packet->size(), 16);
    detail.reserve(n * 3 + 96);
    for (size_t i = 0; i < n; i++) {
      if (i) detail += ' ';
      const uint8_t byte = (*frame.packet)[i];
      detail += hex[byte >> 4];
      detail += hex[byte & 15];
    }
    if (frame.packet->size() > n) detail += " ...";
    detail += " (" + std::to_string(frame.packet->size()) + " bytes)";
  }
  if (frame.has_rssi) {
    char rssi[40];
    std::snprintf(rssi, sizeof(rssi), " | RSSI %.0f dBm", frame.rssi_dbm);
    detail += rssi;
  }
  if (frame.has_lqi) detail += " | LQI " + std::to_string(unsigned(frame.lqi));
  if (frame.kind == FrameKind::PACKET) {
    char offset[48];
    std::snprintf(offset, sizeof(offset), " | FREQEST %.0f Hz", frame.frequency_offset_hz);
    detail += offset;
  }
  const std::string export_detail = this->full_frame_detail_(frame, hash, item_count);
  DecodeResult result = std::move(dispatch.result);
  bool recognized = dispatch.recognized;
  // Two equally strong, conflicting, fully validated protocols must not
  // silently become one device. Do not fall through to weaker user recipes.
  if (dispatch.ambiguous) {
    unknown_frames_++;
    this->record_activity_(frame.channel, "Ambiguous", "Conflicting validated protocols",
                           frame.received_ms, "RX", detail, export_detail, false,
                           "multiple valid decoder results", &frame, "", confidence);
    return;
  }
  if (!recognized && library_ok_) {
    auto try_library = [&](const UserLibrary &library, bool firmware) {
      for (const auto &custom : library.decoders()) {
      if (!user_decoder_accepts_v2(custom,frame) ||
          (firmware && user_library_.decoder(custom.id))) continue;
      std::string reason;
      if (!user_vm::receive(custom.rx, custom.id, frame, result, reason)) continue;
      result.transmit_supported = !custom.tx_source.empty();
      result.expected_actions = custom.actions;
      confidence = 65;
      return true;
      }
      return false;
    };
    recognized = try_library(user_library_, false) || try_library(firmware_library_, true);
  }
  if (!recognized) {
    std::string noise_reason;
    if (this->classify_noise_(frame, confidence, item_count, noise_reason)) {
      noise_frames_++;
      if (frame.channel < channel_count_) channels_[frame.channel].noise_frames++;
      this->record_activity_(frame.channel, "Noise", "Suspected RF noise", frame.received_ms, "RX", detail,
                             export_detail, true, noise_reason, &frame, "", confidence);
      ESP_LOGD(TAG, "[%s] suppressed suspected noise: %s",
               frame.channel < channel_count_ ? channels_[frame.channel].name.c_str() : "?", noise_reason.c_str());
      return;
    }
    developer_.observe(hash, frame, item_count);
    if (led_.learning() && !developer_.baseline_mode() && frame.channel == developer_.channel())
      led_.capture(frame.received_ms);
    unknown_frames_++;
    this->record_activity_(frame.channel, best && confidence >= MIN_DECODER_CONFIDENCE ? best->name() : "Onbekend",
                           best && confidence >= MIN_DECODER_CONFIDENCE ? "Decoder verwierp frame" : "Nieuw signaal",
                           frame.received_ms, "RX", detail, export_detail, false, "", &frame, "", confidence);
    return;
  }
  developer_.observe(hash, frame, item_count);
  if (led_.learning() && !developer_.baseline_mode() && frame.channel == developer_.channel())
    led_.capture(frame.received_ms);
  decoded_frames_++;
  developer_.annotate(hash, result, confidence);
  std::string summary = result.action;
  if (summary.empty() && !result.fields.empty()) {
    summary = result.fields.front().key + " " + result.fields.front().value;
    if (!result.fields.front().unit.empty()) summary += " " + result.fields.front().unit;
  }
  if (summary.empty()) summary = "Ontvangen";
  this->record_activity_(frame.channel, result.protocol, summary, frame.received_ms, "RX", detail,
                         export_detail, false, "", &frame, result.identity_key, confidence);
  DeviceRecord *known = devices_.observe(result, frame.received_ms, frame.channel);
  if (known != nullptr) {
    this->ha_received_(*known, result);
    system_trace("info","decoder",known->uid,std::string("Decoded ")+result.protocol+
                 " action="+result.action+"; matched saved device");
  } else if (devices_.candidate_ready(result.identity_key)) {
    system_trace("info","learn","",std::string("Learn candidate ready: ")+result.protocol);
  }
  const auto *candidate = devices_.find_candidate(result.identity_key);
  ESP_LOGI(TAG, "[%s] %s identity=%s action=%s confidence=%u%% device=%s",
           channels_[frame.channel].name.c_str(), result.protocol.c_str(), result.identity_key.c_str(),
           result.action.c_str(), confidence,
           known != nullptr ? known->uid.c_str() : (candidate != nullptr && devices_.candidate_ready(result.identity_key) ? "candidate-ready" : "candidate"));
}

bool RFLinkUniversal::receive_raw(uint8_t channel, remote_base::RemoteReceiveData &data) {
  if (channel >= channel_count_ || channels_[channel].kind != FrameKind::RAW ||
      !channels_[channel].rx_ready) return false;
  const auto &raw = data.get_raw_data();
  if (raw.size() < 4) return false;
  RFFrame frame;
  frame.kind = FrameKind::RAW;
  frame.channel = channel;
  frame.frequency_hz = channels_[channel].frequency_hz;
  frame.received_ms = millis();
  frame.raw = &raw;
  // ESPHome's async CC1101 -> remote_receiver path does not expose RSSI here.
  frame.has_rssi = false;
  process_frame_(frame, hash_raw_(raw), static_cast<uint16_t>(std::min<size_t>(raw.size(), 65535)));
  return false;
}

void RFLinkUniversal::receive_packet(uint8_t channel, const std::vector<uint8_t> &packet,
                                     float rssi_dbm, uint8_t lqi, float frequency_offset_hz) {
  if (channel == 2 && !radio_mode_868_.itho_ready()) return; // stale FIFO callback during mode transition

  if (channel >= channel_count_ || channels_[channel].kind != FrameKind::PACKET ||
      !channels_[channel].rx_ready || packet.empty()) return;
  RFFrame frame;
  frame.kind = FrameKind::PACKET;
  frame.channel = channel;
  frame.frequency_hz = channels_[channel].frequency_hz;
  frame.received_ms = millis();
  frame.packet = &packet;
  frame.has_rssi = true;
  frame.rssi_dbm = rssi_dbm;
  frame.has_lqi = true;
  frame.lqi = lqi;
  frame.frequency_offset_hz = frequency_offset_hz;
  process_frame_(frame, hash_packet_(packet), static_cast<uint16_t>(std::min<size_t>(packet.size(), 65535)));
}

}  // namespace rflink_universal
}  // namespace esphome

// RFLINK_WEBHANDLER_LINK_HOTFIX_20260918
// ESPHome native IDF build: keep web handler in the guaranteed core translation unit.
// This prevents an undefined RFLinkWebHandler vtable if separate .cpp source selection changes.
#include "rflink_web.h"

#ifdef USE_RFLINK_UNIVERSAL_WEB

#include <array>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <sstream>
#include <set>

#include "rflink_universal.h"
#include "decoder_registry.h"
#include "web_ui.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace rflink_universal {
namespace {

std::string json_escape(const std::string &in) {
  std::string out;
  out.reserve(in.size() + 8);
  for (const char c : in) {
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '"': out += "\\\""; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if (static_cast<unsigned char>(c) < 0x20) {
          char b[8];
          std::snprintf(b, sizeof(b), "\\u%04x", static_cast<unsigned>(static_cast<unsigned char>(c)));
          out += b;
        } else out += c;
    }
  }
  return out;
}

const char *bool_json(bool value) { return value ? "true" : "false"; }

std::string age_json(uint32_t timestamp_ms, uint32_t now) {
  if (timestamp_ms == 0) return "null";
  return std::to_string(now - timestamp_ms);
}

void append_fields(std::ostringstream &out, const std::vector<FieldValue> &fields) {
  out << '[';
  for (size_t i = 0; i < fields.size(); i++) {
    if (i) out << ',';
    out << "{\"key\":\"" << json_escape(fields[i].key) << "\",\"value\":\""
        << json_escape(fields[i].value) << "\",\"unit\":\"" << json_escape(fields[i].unit) << "\"}";
  }
  out << ']';
}

void append_strings(std::ostringstream &out, const std::vector<std::string> &values) {
  out << '[';
  for (size_t i = 0; i < values.size(); i++) {
    if (i) out << ',';
    out << '"' << json_escape(values[i]) << '"';
  }
  out << ']';
}

void append_token_strings(std::ostringstream &out, const char *tokens) {
  std::vector<std::string> values;
  std::istringstream input(tokens ? tokens : "");
  std::string value;
  while (std::getline(input, value, '|')) if (!value.empty()) values.push_back(value);
  append_strings(out, values);
}

void append_profile_sources(std::ostringstream &out, const char *tokens) {
  out << '[';
  std::istringstream input(tokens ? tokens : "");
  std::string value;
  bool first = true;
  while (std::getline(input, value, '|')) {
    const auto a = value.find("::");
    const auto b = a == std::string::npos ? std::string::npos : value.find("::", a + 2);
    if (a == std::string::npos || b == std::string::npos) continue;
    if (!first) out << ',';
    first = false;
    out << "{\"kind\":\"" << json_escape(value.substr(0, a))
        << "\",\"label\":\"" << json_escape(value.substr(a + 2, b - a - 2))
        << "\",\"url\":\"" << json_escape(value.substr(b + 2)) << "\"}";
  }
  out << ']';
}

bool parse_unsigned_text(const std::string &text, uint32_t maximum, uint32_t &value) {
  if (text.empty() || text.size() > 10) return false;
  char *end = nullptr;
  const unsigned long parsed = std::strtoul(text.c_str(), &end, 10);
  if (end != text.c_str() + text.size() || parsed > maximum) return false;
  value = uint32_t(parsed);
  return true;
}

bool parse_raw_csv(const std::string &text, std::vector<int32_t> &values) {
  values.clear();
  if (text.empty()) return true;
  std::istringstream input(text);
  std::string cell;
  while (std::getline(input, cell, ',')) {
    if (values.size() >= 512 || cell.empty() || cell.size() > 12) return false;
    char *end = nullptr;
    const long parsed = std::strtol(cell.c_str(), &end, 10);
    if (end != cell.c_str() + cell.size() || parsed < -1000000 || parsed > 1000000) return false;
    values.push_back(int32_t(parsed));
  }
  return true;
}

bool parse_byte_csv(const std::string &text, std::vector<uint8_t> &values) {
  values.clear();
  if (text.empty()) return true;
  std::istringstream input(text);
  std::string cell;
  while (std::getline(input, cell, ',')) {
    uint32_t parsed = 0;
    if (values.size() >= 64 || !parse_unsigned_text(cell, 255, parsed)) return false;
    values.push_back(uint8_t(parsed));
  }
  return true;
}

}  // namespace

std::string RFLinkWebHandler::url_(AsyncWebServerRequest *request) const {
#ifdef USE_ESP32
  char buffer[AsyncWebServerRequest::URL_BUF_SIZE];
  auto ref = request->url_to(buffer);
  return std::string(ref.c_str(), ref.size());
#else
  return std::string(request->url().c_str());
#endif
}

bool RFLinkWebHandler::canHandle(AsyncWebServerRequest *request) const {
  if (request->method() != HTTP_GET && request->method() != HTTP_POST) return false;
  const auto url = this->url_(request);
  // Serve the bare IP and both standard browser favicon paths ourselves.
  if (url == "/" || url == "/favicon.ico" || url == "/rflink/favicon.svg")
    return request->method() == HTTP_GET;
  return url == "/rflink" || url == "/rflink/" || url.rfind("/rflink/api/", 0) == 0;
}

void RFLinkWebHandler::send_json_(AsyncWebServerRequest *request, const std::string &json) const {
  auto *response = request->beginResponse(200, "application/json", json);
  response->addHeader("Cache-Control", "no-store");
  request->send(response);
}

void RFLinkWebHandler::send_error_(AsyncWebServerRequest *request, int code, const std::string &message) const {
  const std::string json = std::string("{\"ok\":false,\"error\":\"") + json_escape(message) + "\"}";
  auto *response = request->beginResponse(code, "application/json", json);
  response->addHeader("Cache-Control", "no-store");
  request->send(response);
}

std::string RFLinkWebHandler::status_json_() const {
  const uint32_t now = millis();
  std::ostringstream out;
  const auto &led = parent_->status_led();
  const uint64_t uptime_ms =
#ifdef USE_ESP32
      static_cast<uint64_t>(esp_timer_get_time()) / 1000ULL;
#else
      now;
#endif
  out << "{\"firmware_version\":\"" << RFLINK_FIRMWARE_VERSION
      << "\",\"firmware_name\":\"" << RFLINK_FIRMWARE_NAME
      << "\",\"firmware_build_hash\":\"" << RFLINK_FIRMWARE_BUILD_HASH
      << "\",\"firmware_revision\":\"" << RFLINK_FIRMWARE_REVISION
      << "\",\"firmware_built\":\"" << __DATE__ << " " << __TIME__ << "\","
      << "\"boot_session_id\":" << parent_->boot_session_id()
      << ",\"boot_count\":" << parent_->boot_count()
      << ",\"boot_history_persisted\":" << bool_json(parent_->boot_history_persisted())
      << ",\"reset_reason\":\"" << RFLinkUniversal::reset_reason_label(parent_->reset_reason_code())
      << "\",\"reset_reason_code\":" << parent_->reset_reason_code()
      << ",\"boot_history\":[";
  const auto &boot_history = parent_->boot_history();
  for (uint8_t i = 0; i < boot_history.count; ++i) {
    if (i) out << ',';
    const auto &entry = boot_history.events[i];
    out << "{\"boot_number\":" << entry.boot_number
        << ",\"reason\":\"" << RFLinkUniversal::reset_reason_label(entry.reset_reason)
        << "\",\"reason_code\":" << unsigned(entry.reset_reason) << '}';
  }
  out << "],\"crash_trace_persisted\":" << bool_json(parent_->crash_trace_persisted())
      << ",\"crash_traces\":[";
  const auto &crashes = parent_->crash_trace_history();
  for (uint8_t i = 0; i < crashes.count; ++i) {
    const auto &entry = crashes.records[i];
    if (i) out << ',';
    char pc[12]{};
    snprintf(pc, sizeof(pc), "0x%08" PRIX32, entry.pc);
    out << "{\"boot_number\":" << entry.boot_number
        << ",\"reason\":\"" << json_escape(entry.reason) << "\""
        << ",\"core\":" << unsigned(entry.core)
        << ",\"pc\":\"" << pc << "\""
        << ",\"foreign_build\":" << bool_json(entry.foreign_build != 0)
        << ",\"source_hash\":\"" << entry.source_hash << "\""
        << ",\"backtrace\":[";
    for (uint8_t j = 0; j < entry.backtrace_count; ++j) {
      if (j) out << ',';
      char addr[12]{};
      snprintf(addr, sizeof(addr), "0x%08" PRIX32, entry.backtrace[j]);
      out << '"' << addr << '"';
    }
    out << "],\"other_backtrace\":[";
    for (uint8_t j = 0; j < entry.other_count; ++j) {
      if (j) out << ',';
      char addr[12]{};
      snprintf(addr, sizeof(addr), "0x%08" PRIX32, entry.other_backtrace[j]);
      out << '"' << addr << '"';
    }
    out << "]}";
  }
  out << "],\"free_heap_bytes\":";
#ifdef USE_ESP32
  out << heap_caps_get_free_size(MALLOC_CAP_8BIT);
#else
  out << "null";
#endif
  out << ",\"min_free_heap_bytes\":";
#ifdef USE_ESP32
  out << heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT);
#else
  out << "null";
#endif
  // Total heap includes PSRAM; provide internal-heap fragmentation diagnostics
  // because std::ostringstream/new may fail even with megabytes of free PSRAM.
#if defined(USE_ESP32) && defined(ESP_PLATFORM)
  out << ",\"internal_free_bytes\":" << heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)
      << ",\"largest_internal_block_bytes\":" << heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)
      << ",\"psram_free_bytes\":" << heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
#else
  out << ",\"internal_free_bytes\":null,\"largest_internal_block_bytes\":null,\"psram_free_bytes\":null";
#endif
  out << ",\"led\":{\"status\":\"" << StatusLed::name(led.mode(now))
      << "\",\"status_code\":\"" << StatusLed::code(led.mode(now)) << "\",\"brightness\":"
      << unsigned(led.brightness()) << ",\"quiet\":" << bool_json(led.quiet()) << "},\"uptime_ms\":" << uptime_ms
      << ",\"decoder_count\":" << parent_->decoder_count()
      << ",\"expected_decoders\":"
#ifdef RFLINK_DECODER_EXPECTED_COUNT
      << RFLINK_DECODER_EXPECTED_COUNT
#else
      << "null"
#endif
      << ",\"storage_ready\":" << bool_json(parent_->storage_ready())
      << ",\"storage_ok\":" << bool_json(parent_->storage_ok())
      << ",\"user_library_ready\":" << bool_json(parent_->library_ready())
      << ",\"saved_devices\":" << parent_->devices().devices().size()
      << ",\"total_frames\":" << parent_->total_frames()
      << ",\"decoded_frames\":" << parent_->decoded_frames() << ",\"unknown_frames\":" << parent_->unknown_frames()
      << ",\"noise_frames\":" << parent_->noise_frames()
      << ",\"channels\":[";
  for (uint8_t i = 0; i < parent_->channel_count(); i++) {
    if (i) out << ',';
    const auto &ch = parent_->channel(i);
    char label[20];
    std::snprintf(label, sizeof(label), "%.2f MHz", ch.frequency_hz / 1000000.0f);
    out << "{\"index\":" << unsigned(i) << ",\"name\":\"" << json_escape(ch.name)
        << "\",\"frequency_label\":\"" << label << "\",\"frequency_hz\":" << ch.frequency_hz
        << ",\"mode\":\"" << (ch.kind == FrameKind::PACKET ? "PACKET" : "RX")
        << "\",\"checked\":" << bool_json(ch.checked)
        << ",\"available\":" << bool_json(ch.available)
        << ",\"rx_ready\":" << bool_json(ch.rx_ready) << ",\"tx_ready\":" << bool_json(ch.tx_ready)
        << ",\"diagnosis\":\"" << (ch.checked ? (ch.available ? (ch.rx_ready ? (ch.tx_ready ? "ok" : "tx_failed") : "rx_failed") : "cc1101_init_failed") : "checking") << "\""
        << ",\"cs_pin\":\"" << json_escape(ch.cs_pin) << "\""
        << ",\"gdo0_pin\":\"" << json_escape(ch.gdo0_pin) << "\""
        << ",\"gdo2_pin\":\"" << json_escape(ch.gdo2_pin) << "\""
        << ",\"self_test_done\":" << bool_json(ch.self_test_done)
        << ",\"self_test_spi_ok\":" << bool_json(ch.self_test_spi_ok)
        << ",\"partnum\":" << unsigned(ch.partnum)
        << ",\"version\":" << unsigned(ch.version)
        << ",\"spi_checks\":" << ch.spi_checks
        << ",\"spi_failures\":" << ch.spi_failures
        << ",\"spi_consecutive_failures\":" << unsigned(ch.spi_consecutive_failures)
        << ",\"spi_recovery_attempts\":" << ch.spi_recovery_attempts
        << ",\"spi_recovery_successes\":" << ch.spi_recovery_successes
        << ",\"spi_recovery_pending\":" << bool_json(ch.spi_recovery_pending)
        << ",\"frames\":" << ch.frames
        << ",\"noise_frames\":" << ch.noise_frames
        << ",\"age_ms\":" << age_json(ch.last_seen_ms, now) << ",\"rssi_dbm\":";
    if (ch.has_rssi) out << ch.last_rssi_dbm; else out << "null";
    out << ",\"lqi\":";
    if (ch.has_lqi) out << unsigned(ch.last_lqi); else out << "null";
    out << ",\"frequency_offset_hz\":";
    if (ch.has_frequency_offset) out << ch.last_frequency_offset_hz; else out << "null";
    out << '}';
  }
  out << "],\"activity\":[";
  const auto &activity = parent_->activity();
  for (size_t i = 0; i < activity.size(); i++) {
    if (i) out << ',';
    const auto &a = activity[i];
    out << "{\"channel\":\"" << json_escape(a.channel) << "\",\"channel_index\":" << unsigned(a.channel_index)
        << ",\"direction\":\"" << json_escape(a.direction) << "\",\"protocol\":\"" << json_escape(a.protocol)
        << "\",\"summary\":\"" << json_escape(a.summary) << "\",\"detail\":\"" << json_escape(a.detail)
        << "\",\"noise_suspect\":" << bool_json(a.noise_suspect)
        << ",\"noise_reason\":\"" << json_escape(a.noise_reason)
        << "\",\"age_ms\":" << age_json(a.timestamp_ms, now) << '}';
  }
  out << "]}";
  return out.str();
}

#if !defined(USE_ESP32) || !defined(ESP_PLATFORM)
std::string RFLinkWebHandler::rx_log_json_(AsyncWebServerRequest *request) const {
  // Hard page limit keeps HTTP payloads and temporary heap usage bounded.
  uint32_t max_rows = 40, radio = 0, kind = 0;
  if (request->hasArg("limit") && !parse_unsigned_text(request->arg("limit"), 100, max_rows)) max_rows = 40;
  if (max_rows == 0) max_rows = 1;
  rflink_logging::Filter filter{};
  if (request->hasArg("radio") && parse_unsigned_text(request->arg("radio"), 2, radio)) filter.radio = static_cast<int>(radio);
  if (request->hasArg("kind") && parse_unsigned_text(request->arg("kind"), 1, kind)) filter.kind = static_cast<int>(kind);
  const bool ascending = request->arg("order") == "asc";
  uint64_t cursor = ascending ? 0 : UINT64_MAX;
  if (request->hasArg("cursor")) {
    const std::string value = request->arg("cursor");
    if (!value.empty() && value.size() <= 20 && value.find_first_not_of("0123456789") == std::string::npos)
      cursor = static_cast<uint64_t>(std::strtoull(value.c_str(), nullptr, 10));
  }
  const auto rows = parent_->rx_log_read(cursor, filter, max_rows, ascending);
  std::ostringstream out;
  out << "{\"ok\":true,\"events\":[";
  for (size_t i = 0; i < rows.size(); ++i) {
    if (i) out << ',';
    const auto &row = rows[i];
    // JSON payload is already escaped by record_activity_.
    out << "{\"seq\":" << row.sequence << ",\"radio\":" << unsigned(row.radio)
        << ",\"category\":\"" << (row.kind == rflink_logging::Kind::Noise ? "noise" : "signal")
        << "\",\"uptime_ms\":" << row.uptime_ms << ",\"record\":" << row.json << '}';
  }
  out << "],\"next_cursor\":" << (rows.empty() ? cursor : rows.back().sequence)
      << ",\"has_more\":" << bool_json(rows.size() == max_rows) << "}";
  return out.str();
}


#endif
#if defined(USE_ESP32) && defined(ESP_PLATFORM)
void RFLinkWebHandler::send_rx_log_chunked_(AsyncWebServerRequest *request) const {
  // ESPHome/ESP-IDF's beginResponseStream() still builds ONE contiguous std::string.
  // In the old RXLOG1 path, 80 large records + duplicated response buffers could
  // exhaust fragmented INTERNAL RAM even with several MB of free PSRAM.
  // Send native ESP-IDF HTTP chunks instead; only one event is copied at a time.
  uint32_t max_rows = 20, radio = 0, kind = 0;
  if (request->hasArg("limit") && !parse_unsigned_text(request->arg("limit"), 50, max_rows)) max_rows = 20;
  if (max_rows == 0) max_rows = 1;
  rflink_logging::Filter filter{};
  if (request->hasArg("radio") && parse_unsigned_text(request->arg("radio"), 2, radio))
    filter.radio = static_cast<int>(radio);
  if (request->hasArg("kind") && parse_unsigned_text(request->arg("kind"), 1, kind))
    filter.kind = static_cast<int>(kind);
  const bool ascending = request->arg("order") == "asc";
  uint64_t cursor = ascending ? 0 : UINT64_MAX;
  if (request->hasArg("cursor")) {
    const std::string value = request->arg("cursor");
    if (!value.empty() && value.size() <= 20 && value.find_first_not_of("0123456789") == std::string::npos)
      cursor = static_cast<uint64_t>(std::strtoull(value.c_str(), nullptr, 10));
  }

  // A fixed-size PSRAM scratch buffer avoids std::string and ostringstream
  // allocations for raw events (up to kMaxEventJsonBytes, without truncation).
  constexpr size_t scratch_size = rflink_logging::kMaxEventJsonBytes;
  char *scratch = static_cast<char *>(heap_caps_malloc(scratch_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (!scratch) return this->send_error_(request, 503, "RX log export has insufficient PSRAM");
  const auto rows = parent_->rx_log_read(cursor, filter, max_rows + 1, ascending, false);
  const size_t count = std::min<size_t>(max_rows, rows.size());
  const bool has_more = rows.size() > count;
  const uint64_t next_cursor = count ? rows[count - 1].sequence : cursor;

  auto *response = request->beginResponse(200, "application/json");
  response->addHeader("Cache-Control", "no-store");
  auto send_chunk = [request](const char *buf, size_t len) -> bool {
    return httpd_resp_send_chunk(*request, buf, len) == ESP_OK;
  };
  bool ok = send_chunk("{\"ok\":true,\"events\":[", sizeof("{\"ok\":true,\"events\":[") - 1);
  for (size_t i = 0; ok && i < count; ++i) {
    const auto &entry = rows[i];
    size_t written = 0;
    const bool present = parent_->rx_log_copy_json(entry.sequence, scratch, scratch_size, written);
    char metadata[220];
    const int n = std::snprintf(metadata, sizeof(metadata),
        "%s{\"seq\":%" PRIu64 ",\"radio\":%u,\"category\":\"%s\",\"uptime_ms\":%" PRIu64 ",\"record\":",
        i ? "," : "", entry.sequence, unsigned(entry.radio),
        entry.kind == rflink_logging::Kind::Noise ? "noise" : "signal", entry.uptime_ms);
    if (n <= 0 || static_cast<size_t>(n) >= sizeof(metadata) || !send_chunk(metadata, static_cast<size_t>(n))) {
      ok = false;
      break;
    }
    if (present && written != 0) {
      // The original event was generated as JSON and copied byte-for-byte.
      // A missing record can occur only if it was evicted while responding.
      for (size_t offset = 0; ok && offset < written; offset += 1024)
        ok = send_chunk(scratch + offset, std::min<size_t>(1024, written - offset));
    } else {
      ok = send_chunk("null", 4);
    }
    if (ok) ok = send_chunk(present ? "}" : ",\"expired\":true}", present ? 1 : sizeof(",\"expired\":true}") - 1);
  }
  char tail[110];
  const int tn = std::snprintf(tail, sizeof(tail), "],\"next_cursor\":%" PRIu64 ",\"has_more\":%s}",
                               next_cursor, has_more ? "true" : "false");
  if (ok && tn > 0 && static_cast<size_t>(tn) < sizeof(tail)) ok = send_chunk(tail, static_cast<size_t>(tn));
  if (ok) httpd_resp_send_chunk(*request, nullptr, 0);  // Complete HTTP chunked response.
  heap_caps_free(scratch);
  if (!ok) ESP_LOGW(TAG, "RX log client disconnected during chunked transfer");
}
#endif

void RFLinkWebHandler::handle_rx_log_config_(AsyncWebServerRequest *request) {
  if (!parent_->rx_log_ready()) return this->send_error_(request, 503, "PSRAM RX logger is unavailable");
  const auto current = parent_->rx_log_limits();
  if (request->method() == HTTP_POST) {
    if (request->arg("write_token") != std::to_string(parent_->web_write_token()))
      return this->send_error_(request, 403, "Write token required");
    rflink_logging::Limits update = current;
    uint32_t n = 0;
    for (unsigned i = 0; i < 3; ++i) {
      const std::string key = "signal_" + std::to_string(i);
      if (!request->hasArg(key.c_str()) || !parse_unsigned_text(request->arg(key.c_str()), 4096, n))
        return this->send_error_(request, 400, "Invalid signal limit");
      update.signals[i] = static_cast<uint16_t>(n);
    }
    if (!request->hasArg("noise") || !parse_unsigned_text(request->arg("noise"), 4096, n))
      return this->send_error_(request, 400, "Invalid noise limit");
    update.noise = static_cast<uint16_t>(n);
    // Retention floor is a soft priority, not reserved memory.
    if (request->hasArg("noise_floor")) {
      if (!parse_unsigned_text(request->arg("noise_floor"), 2000, n))
        return this->send_error_(request, 400, "Invalid noise retention floor");
      update.noise_floor = static_cast<uint16_t>(n);
    }
    if (!parent_->rx_log_set_limits(update)) return this->send_error_(request, 400, "Invalid limits or NVS save failed");
  }
  const auto &limits = parent_->rx_log_limits();
  const auto counts = parent_->rx_log_counters();
  std::ostringstream out;
  out << "{\"ok\":true,\"write_token\":" << parent_->web_write_token()
      << ",\"signal_limits\":[" << limits.signals[0] << ',' << limits.signals[1]
      << ',' << limits.signals[2] << "],\"noise_limit\":" << limits.noise
      << ",\"noise_floor\":" << limits.noise_floor
      << ",\"signals\":[" << counts.signals[0] << ',' << counts.signals[1] << ',' << counts.signals[2]
      << "],\"noise_by_radio\":[" << counts.noise_by_radio[0] << ',' << counts.noise_by_radio[1]
      << ',' << counts.noise_by_radio[2] << "],\"noise_total\":" << counts.noise_total
      << ",\"log_bytes_used\":" << counts.bytes_used_payload
      << ",\"arena_bytes\":" << counts.allocated_arena_bytes
      << ",\"free_blocks\":" << counts.free_blocks
      << ",\"dropped_busy\":" << parent_->rx_log_busy_drops()
      << ",\"evicted_limit\":[";
  for (int i = 0; i < 4; ++i) { if (i) out << ','; out << counts.evicted_limit[i]; }
  out << "],\"evicted_memory\":[";
  for (int i = 0; i < 4; ++i) { if (i) out << ','; out << counts.evicted_memory[i]; }
  out << "],\"rejected\":[";
  for (int i = 0; i < 4; ++i) { if (i) out << ','; out << counts.rejected[i]; }
  out << "]";
#if defined(USE_ESP32) && defined(ESP_PLATFORM)
  out << ",\"internal_free\":" << heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)
      << ",\"psram_free\":" << heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
      << ",\"largest_internal_block\":" << heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
#endif
  out << '}';
  return this->send_json_(request, out.str());
}

std::string RFLinkWebHandler::system_trace_json_() const {
  // Separate from physical RF receive logs. Only bounded, developer-selected
  // decisions and state transitions; no raw frame contents or credentials.
  std::ostringstream out;
  out<<"{\"ok\":true,\"capacity\":"<<SystemTrace::CAPACITY
     <<",\"limit\":"<<parent_->trace().limit()
     <<",\"boot_session_id\":"<<parent_->boot_session_id()
     <<",\"server_uptime_ms\":"<<millis()
     <<",\"dropped\":"<<parent_->trace().dropped()
     <<",\"gdo_lab\":"<<bool_json(parent_->gdo_serial_lab_enabled())
     <<",\"probe_lab\":"<<bool_json(parent_->ramses_tx_probe_enabled())
     <<",\"adapter_registered\":"<<bool_json(parent_->serial_tx_registered())
     <<",\"adapter_ready\":"<<bool_json(parent_->serial_tx_ready())
     <<",\"sticky_fault\":"<<bool_json(parent_->serial_tx_fault())
     <<",\"radio_868_mode\":"<<parent_->radio_mode_868_state()
     <<",\"events\":[";
  const auto entries=parent_->trace().snapshot();
  bool first=true;
  for(const auto &e:entries){
    if(!first) out<<",";
    first=false;
    out<<"{\"seq\":"<<e.sequence<<",\"ms\":"<<e.uptime_ms
       <<",\"level\":\""<<json_escape(e.level)
       <<"\",\"subsystem\":\""<<json_escape(e.subsystem)
       <<"\",\"uid\":\""<<json_escape(e.uid)
       <<"\",\"message\":\""<<json_escape(e.message)<<"\"}";
  }
  out<<"]}";return out.str();
}

std::string RFLinkWebHandler::activity_export_json_() const {
  const uint32_t now = millis();
  std::ostringstream out;
  out << "{\"generated_uptime_ms\":" << now << ",\"activity\":[";
  const auto &activity = parent_->activity();
  for (size_t i = 0; i < activity.size(); ++i) {
    if (i) out << ',';
    const auto &a = activity[i];
    out << "{\"timestamp_ms\":" << a.timestamp_ms
        << ",\"age_ms\":" << age_json(a.timestamp_ms, now)
        << ",\"channel\":\"" << json_escape(a.channel) << "\",\"channel_index\":" << unsigned(a.channel_index)
        << ",\"direction\":\"" << json_escape(a.direction) << "\",\"protocol\":\"" << json_escape(a.protocol)
        << "\",\"summary\":\"" << json_escape(a.summary) << "\",\"detail\":\"" << json_escape(a.detail)
        << "\",\"full_detail\":\"" << json_escape(a.export_detail)
        << "\",\"noise_suspect\":" << bool_json(a.noise_suspect)
        << ",\"noise_reason\":\"" << json_escape(a.noise_reason) << "\"}";
  }
  out << "]}";
  return out.str();
}

std::string RFLinkWebHandler::devices_json_() const {
  const uint32_t now = millis();
  std::ostringstream out;
  out << "{\"storage_ok\":" << bool_json(parent_->storage_ok())
      << ",\"mqtt_configured\":" << bool_json(parent_->mqtt_configured())
      << ",\"mqtt_connected\":" << bool_json(parent_->mqtt_connected())
      << ",\"tx_controls_enabled\":" << bool_json(parent_->tx_controls_enabled())
      << ",\"devices\":[";
  const auto &devices = parent_->devices().devices();
  for (size_t i = 0; i < devices.size(); i++) {
    if (i) out << ',';
    const auto &d = devices[i];
    // Stored v1 devices do not persist RX radio; infer only from an unambiguous
    // built-in band, otherwise retain UNKNOWN rather than inventing a frequency.
    uint8_t resolved_channel = d.channel;
    if (resolved_channel == 255) {
      const auto *known = capabilities::find(d.protocol.c_str());
      if (known != nullptr) {
        const std::string band = known->learn_band ? known->learn_band : "";
        const std::string tx = known->tx_radio ? known->tx_radio : "";
        if (band.find("433,42") != std::string::npos || tx == "433.42") resolved_channel = 0;
        else if (band.find("433,92") != std::string::npos || tx == "433.92") resolved_channel = 1;
        else if (band.find("868,30") != std::string::npos || tx == "868.30") resolved_channel = 2;
      }
    }
    out << "{\"uid\":\"" << json_escape(d.uid) << "\",\"name\":\"" << json_escape(d.name)
        << "\",\"protocol\":\"" << json_escape(d.protocol) << "\",\"profile_id\":\"" << json_escape(d.profile_id)
        << "\",\"channel\":" << unsigned(resolved_channel) << ",\"local_tx_enabled\":" << bool_json(d.local_transmit_enabled)
        << ",\"device_type\":\"" << json_escape(d.device_type)
        << "\",\"age_ms\":" << age_json(d.last_seen_ms, now) << ",\"identities\":" << d.rf_identities.size()
        << ",\"rf_identities\":";
    append_strings(out, d.rf_identities);
    out << ",\"pairing_supported\":" << bool_json(d.pairing_supported) << ",\"transmit_supported\":" << bool_json(d.transmit_supported)
        << ",\"ha_enabled\":" << bool_json(d.home_assistant_enabled) << ",\"tx_enabled\":" << bool_json(d.transmit_enabled) << ",\"tx_eligible\":" << bool_json(parent_->tx_eligible(d.uid));
    if (d.protocol == "RamsesII") {
      out << ",\"ramses\":{\"pair_stage\":\"" << json_escape(parent_->ramses_pair_stage(d.uid))
          << "\",\"lab_enabled\":" << bool_json(parent_->ramses_pair_lab_ready())
          << ",\"adapter_ready\":" << bool_json(parent_->serial_tx_ready())
          << ",\"test_sent\":" << bool_json(parent_->ramses_pair_test_sent(d.uid)) << "}";
    }
    if (d.protocol == "SomfyRTS56") {
      uint32_t address = 0; uint16_t rolling = 0; bool paired = false;
      const bool configured = parent_->somfy_status(d.uid, address, rolling, paired);
      out << ",\"somfy\":{\"configured\":" << bool_json(configured)
          << ",\"paired\":" << bool_json(configured && paired)
          << ",\"address\":" << (configured ? address : 0)
          << ",\"next_rolling_code\":" << (configured ? rolling : 0) << "}";
    }
    if (d.protocol == "IthoRFT") {
      uint32_t address = 0, sequence = 0; uint8_t device_type = 0; bool paired = false;
      const bool configured = parent_->itho_status(d.uid, address, sequence, device_type, paired);
      out << ",\"itho\":{\"configured\":" << bool_json(configured)
          << ",\"paired\":" << bool_json(configured && paired)
          << ",\"address\":" << (configured ? address : 0)
          << ",\"next_sequence\":" << (configured ? sequence : 0)
          << ",\"device_type\":" << (configured ? unsigned(device_type) : 0) << "}";
    }
    out << ",\"tx_ready_actions\":[";
    bool first_tx_action=true;
    auto append_tx_ready=[&](const std::string &action) {
      if(!action.empty() && parent_->tx_action_ready(d.uid,action)) {
        if(!first_tx_action)out<<',';
        first_tx_action=false;
        out << '"' << json_escape(action) << '"';
      }
    };
    for(const auto &action:d.actions)append_tx_ready(action);
    if(!d.profile_id.empty()){
      const auto *active_profile=profiles::find(d.profile_id.c_str());
      if(active_profile && d.protocol==active_profile->protocol){
        const std::string controls=active_profile->controls;
        size_t start=0;
        while(start<controls.size()){
          const size_t end=controls.find('|',start);
          const auto action=controls.substr(start,end==std::string::npos?end:end-start);
          if(std::find(d.actions.begin(),d.actions.end(),action)==d.actions.end())append_tx_ready(action);
          if(end==std::string::npos)break;
          start=end+1;
        }
      }
    }
    out << ']';
    out << ",\"profile_suggestions\":[";
    const auto d_suggestions = profile_suggestions::rank(d.protocol,d.device_type,d.actions);
    for(size_t j=0;j<d_suggestions.size();++j) {
      if(j) out << ',';
      out << "{\"id\":\"" << json_escape(d_suggestions[j].id)
          << "\",\"reason\":\"" << (d_suggestions[j].action_evidence ? "observed_actions" : "protocol_only")
          << "\",\"confidence\":\"possible\"}";
    }
    out << "]";
    out << ",\"actions\":";
    append_strings(out, d.actions);
    out << ",\"fields\":";
    append_fields(out, d.fields);
    const auto *pending = parent_->devices().pending(d.uid);
    out << ",\"pending_actions\":";
    if (pending) append_strings(out, pending->actions);
    else out << "[]";
    out << ",\"pending_fields\":";
    if (pending) append_fields(out, pending->fields);
    else out << "[]";
    out << '}';
  }
  out << "],\"candidates\":[";
  const auto &candidates = parent_->devices().candidates();
  for (size_t i = 0; i < candidates.size(); i++) {
    if (i) out << ',';
    const auto &c = candidates[i];
    const auto *suggest = parent_->devices().suggest_similar(c);
    out << "{\"identity\":\"" << json_escape(c.identity_key) << "\",\"channel\":" << unsigned(c.channel) << ",\"protocol\":\"" << json_escape(c.protocol)
        << "\",\"device_type\":\"" << json_escape(c.device_type) << "\",\"observations\":" << c.observations
        << ",\"ready\":" << bool_json(parent_->devices().candidate_ready(c.identity_key)) << ",\"age_ms\":" << age_json(c.last_seen_ms, now)
        << ",\"ha_available\":" << bool_json(!c.actions.empty() || std::any_of(c.fields.begin(), c.fields.end(),
                                             [](const FieldValue &f) { return ha::profile(f.key) != nullptr; }));
    out << ",\"profile_suggestions\":[";
    const auto c_suggestions = profile_suggestions::rank(c.protocol,c.device_type,c.actions);
    for(size_t j=0;j<c_suggestions.size();++j) {
      if(j) out << ',';
      out << "{\"id\":\"" << json_escape(c_suggestions[j].id)
          << "\",\"reason\":\"" << (c_suggestions[j].action_evidence ? "observed_actions" : "protocol_only")
          << "\",\"confidence\":\"possible\"}";
    }
    out << "]";
    out << ",\"actions\":";
    append_strings(out, c.actions);
    out << ",\"fields\":";
    append_fields(out, c.fields);
    out
        << ",\"suggest_uid\":\"" << (suggest ? json_escape(suggest->uid) : "") << "\",\"suggest_name\":\""
        << (suggest ? json_escape(suggest->name) : "") << "\"}";
  }
  out << "]}";
  return out.str();
}

std::string RFLinkWebHandler::developer_json_() const {
  const auto &dev = parent_->developer();
  const auto *best = dev.best_new_candidate();
  std::ostringstream out;
  out << "{\"channel\":" << unsigned(dev.channel()) << ",\"mode\":\"" << (dev.baseline_mode() ? "baseline" : "listen")
      << "\",\"label\":\"" << json_escape(dev.label()) << "\",\"captures\":[";
  for (size_t i = 0; i < dev.captures().size(); ++i) {
    const auto &sample = dev.captures()[i];
    if (i) out << ',';
    out << "{\"label\":\"" << json_escape(sample.label) << "\",\"channel\":"
        << unsigned(sample.channel) << ",\"kind\":\"" << (sample.kind == FrameKind::RAW ? "raw" : "packet")
        << "\",\"length\":" << (sample.kind == FrameKind::RAW ? sample.raw.size() : sample.packet.size()) << '}';
  }
  out << "],\"best\":";
  if (best == nullptr) {
    out << "null}";
    return out.str();
  }
  out << "{\"hash\":" << best->hash << ",\"observations\":" << best->observations << ",\"pulse_count\":" << best->pulse_count
      << ",\"kind\":\"" << (best->frame_kind == FrameKind::PACKET ? "packet" : "raw") << "\",\"rssi_dbm\":";
  if (best->has_rssi) out << best->best_rssi_dbm; else out << "null";
  const std::string snippet = best->frame_kind == FrameKind::PACKET
                                  ? dev.generate_packet_decoder_snippet(*best)
                                  : dev.generate_raw_decoder_snippet(*best);
  out << ",\"protocol\":\"" << json_escape(best->protocol) << "\",\"identity\":\"" << json_escape(best->identity_key)
      << "\",\"action\":\"" << json_escape(best->action) << "\",\"decoder_confidence\":" << unsigned(best->decoder_confidence)
      << ",\"snippet\":\"" << json_escape(snippet) << "\"}}";
  return out.str();
}

std::string RFLinkWebHandler::decoders_json_() const {
  std::ostringstream out;
  out << "{\"decoders\":[";
  const auto &registry = decoder_registry();
  for (size_t i = 0; i < registry.size(); i++) {
    if (i) out << ',';
    out << "\"" << json_escape(registry[i] == nullptr ? "<null>" : registry[i]->name()) << "\"";
  }
  out << "],\"count\":" << registry.size() << ",\"capability_schema\":2,\"ha_fields\":[";
  size_t profile_count = 0;
  const auto *profiles = capabilities::fields(profile_count);
  for (size_t i=0; i<profile_count; ++i) {
    if (i) out << ',';
    out << "{\"key\":\"" << json_escape(profiles[i].key) << "\",\"label\":\""
        << json_escape(profiles[i].display) << "\"}";
  }
  out << "],\"protocols\":[";
  size_t count = 0;
  const auto *protocols = capabilities::protocols(count);
  for (size_t i = 0; i < count; ++i) {
    if (i) out << ',';
    const auto &p = protocols[i];
    const auto *radio_decl = radio_catalog::policy().find(p.name);
    out << "{\"name\":\"" << json_escape(p.name) << "\",\"pack\":\"" << json_escape(p.pack)
        << "\",\"stable_id\":\"decoder:" << json_escape(p.name) << "\",\"source\":\"builtin\",\"library_version\":1"
        << ",\"rx_actions\":\"" << json_escape(p.rx_actions)
        << "\",\"fields\":\"" << json_escape(p.fields)
        << "\",\"tx_encoder\":\"" << json_escape(p.encoder)
        << "\",\"tx_status\":\"" << json_escape(p.tx_status)
        << "\",\"tx_radio\":\"" << json_escape(p.tx_radio)
        << "\",\"tx_actions\":\"" << json_escape(p.tx_actions)
        << "\",\"tx_note\":\"" << json_escape(p.tx_note)
        << "\",\"learn_category\":\"" << json_escape(p.learn_category)
        << "\",\"learn_band\":\"" << json_escape(p.learn_band)
        << "\",\"learn_instruction\":\"" << json_escape(p.learn_instruction)
        << "\",\"learn_ha_result\":\"" << json_escape(p.learn_ha_result)
        << "\",\"learn_warning\":\"" << json_escape(p.learn_warning)
        << "\",\"tx_registered\":" << bool_json(tx_engine::resolve(p.name)!=nullptr)
        << ",\"rx_transport\":\"" << (radio_decl ? transport_name(radio_decl->preferred):"not_specified")
        << "\",\"rx_fallback\":\"" << (radio_decl ? transport_name(radio_decl->fallback):"none") << "\"}";
  }
  out << "]}";
  return out.str();
}

std::string RFLinkWebHandler::library_json_() const {
  std::ostringstream out;
  size_t count = 0;
  const auto *protocols = capabilities::protocols(count);
  std::vector<std::string> conflicts;
  for (const auto &built : parent_->firmware_library().decoders()) {
    const auto *local = parent_->user_library().decoder(built.id);
    if (local && UserLibrary::fingerprint(*local) != UserLibrary::fingerprint(built))
      conflicts.push_back(built.id);
  }
  out << "{\"schema\":2,\"persistent_user_library\":" << bool_json(parent_->library_ready())
      << ",\"official_decoders\":" << count
      << ",\"firmware_user_decoders\":" << parent_->firmware_library().decoders().size()
      << ",\"user_decoders\":" << parent_->user_library().decoders().size()
      << ",\"official_profiles\":" << profiles::count() << ",\"user_profiles\":" << parent_->user_library().profiles().size()
      << ",\"attention_count\":" << conflicts.size() << ",\"safe_resolution_count\":0,\"archive_count\":0"
      << ",\"attention\":";
  append_strings(out, conflicts);
  out << ",\"items\":[";
  for (size_t i = 0; i < count; ++i) {
    if (i) out << ',';
    const auto &p = protocols[i];
    out << "{\"id\":\"decoder:" << json_escape(p.name)
        << "\",\"kind\":\"decoder\",\"source\":\"builtin\",\"version\":1"
        << ",\"name\":\"" << json_escape(p.name) << "\",\"pack\":\"" << json_escape(p.pack) << "\"}";
  }
  for (const auto &d : parent_->user_library().decoders()) {
    out << ", {\"id\":\"decoder:" << json_escape(d.id)
        << "\",\"kind\":\"decoder\",\"source\":\"user\",\"version\":" << d.version
        << ",\"name\":\"" << json_escape(d.name) << "\",\"enabled\":" << bool_json(d.enabled) << '}';
  }
  size_t official_profile_count = 0;
  const auto *official_profiles = profiles::all(official_profile_count);
  for (size_t i = 0; i < official_profile_count; ++i) {
    const auto &p = official_profiles[i];
    out << ", {\"id\":\"profile:" << json_escape(p.id)
        << "\",\"kind\":\"profile\",\"source\":\"builtin\",\"version\":1"
        << ",\"name\":\"" << json_escape(p.name) << "\",\"protocol\":\"" << json_escape(p.protocol) << "\"}";
  }
  for (const auto &p : parent_->user_library().profiles()) {
    out << ", {\"id\":\"profile:" << json_escape(p.id)
        << "\",\"kind\":\"profile\",\"source\":\"user\",\"version\":" << p.version
        << ",\"name\":\"" << json_escape(p.name) << "\"}";
  }
  for (const auto &d : parent_->firmware_library().decoders()) {
    out << ", {\"id\":\"decoder:" << json_escape(d.id)
        << "\",\"kind\":\"decoder\",\"source\":\"firmware\",\"version\":" << d.version
        << ",\"name\":\"" << json_escape(d.name) << "\"}";
  }
  out << "]}";
  return out.str();
}

std::string RFLinkWebHandler::user_library_json_() const {
  std::ostringstream out;
  out << "{\"schema\":1,\"ready\":" << bool_json(parent_->library_ready())
      << ",\"write_token\":\"" << parent_->web_write_token() << "\",\"decoders\":[";
  const auto &library = parent_->user_library();
  const auto &firmware = parent_->firmware_library();
  bool first_decoder = true;
  auto append_decoder = [&](const UserDecoder &d, const char *source) {
    if (!first_decoder) out << ',';
    first_decoder = false;
    out << "{\"id\":\"" << json_escape(d.id) << "\",\"name\":\"" << json_escape(d.name)
        << "\",\"source\":\"" << source
        << "\",\"radio\":" << unsigned(d.radio) << ",\"kind\":\""
        << (d.kind == FrameKind::RAW ? "raw" : "packet") << "\",\"radio_profile\":\""
        << (d.kind == FrameKind::RAW ? "board_raw" : "board_packet_63")
        << "\",\"enabled\":" << bool_json(d.enabled)
        << ",\"version\":" << d.version << ",\"fingerprint\":" << UserLibrary::fingerprint(d)
        << ",\"rx_source\":\"" << json_escape(d.rx_source) << "\",\"tx_source\":\""
        << json_escape(d.tx_source) << "\",\"actions\":";
    append_strings(out, d.actions);
    // Samples are intentionally not part of the automatic library listing.
    // A populated user library may contain up to 12 RAW captures per decoder.
    // Serializing *all* captures and all built-in translations into one
    // std::ostringstream caused an allocation failure -> abort on ESP32-S3.
    out << ",\"sample_count\":" << d.samples.size() << ",\"samples\":[]}";
  };
  for (const auto &d : library.decoders()) append_decoder(d, "user");
  for (const auto &d : firmware.decoders()) if (!library.decoder(d.id)) append_decoder(d, "firmware");
  out << "],\"profiles\":[";
  bool first_profile = true;
  auto append_profile = [&](const UserProfile &p, const char *source) {
    if (!first_profile) out << ',';
    first_profile = false;
    out << "{\"id\":\"" << json_escape(p.id) << "\",\"name\":\"" << json_escape(p.name)
        << "\",\"source\":\"" << source << "\",\"protocol\":\"" << json_escape(p.protocol) << "\",\"version\":" << p.version
        << ",\"expected_actions\":";
    append_strings(out, p.expected_actions);
    out << ",\"learn_help\":\"" << json_escape(p.learn_help)
        << "\",\"controls\":";
    append_strings(out,p.controls);
    out << ",\"setup_steps_i18n\":{\"en\":"; append_strings(out,p.setup_steps);
    out << ",\"nl\":"; append_strings(out,p.setup_steps);
    out << ",\"fr\":"; append_strings(out,p.setup_steps);
    out << ",\"de\":"; append_strings(out,p.setup_steps);
    out << "},\"pair_method\":\"" << json_escape(p.pair_method)
        << "\",\"pair_tx_required\":false}";
  };
  for (const auto &p : library.profiles()) append_profile(p, "user");
  for (const auto &p : firmware.profiles())
    if (std::none_of(library.profiles().begin(), library.profiles().end(),
                     [&](const UserProfile &local) { return local.id == p.id; })) append_profile(p, "firmware");
  size_t official_profile_count = 0;
  const auto *official_profiles = profiles::all(official_profile_count);
  auto profile_shadowed = [&](const char *id) {
    return std::any_of(library.profiles().begin(), library.profiles().end(),
                       [&](const UserProfile &p) { return p.id == id; }) ||
           std::any_of(firmware.profiles().begin(), firmware.profiles().end(),
                       [&](const UserProfile &p) { return p.id == id; });
  };
  for (size_t i = 0; i < official_profile_count; ++i) {
    const auto &p = official_profiles[i];
    if (profile_shadowed(p.id)) continue;
    if (!first_profile) out << ',';
    first_profile = false;
    // Built-in translations, model lists, sources and pairing instructions are
    // loaded one profile at a time via /profile-catalog?id=.  Never include
    // the complete catalog in the editable user-library GET response.
    out << "{\"id\":\"" << json_escape(p.id) << "\",\"name\":\"" << json_escape(p.name)
        << "\",\"source\":\"builtin\",\"protocol\":\"" << json_escape(p.protocol)
        << "\",\"version\":1,\"manufacturer\":\"" << json_escape(p.manufacturer)
        << "\",\"family\":\"" << json_escape(p.family) << "\",\"controls\":";
    append_token_strings(out,p.controls);
    out << ",\"expected_actions\":";append_token_strings(out,p.expected_actions);
    out << ",\"pair_method\":\"" << json_escape(p.pair_method) << "\"}";
  }
  out << "],\"protocols\":[";
  size_t protocol_count = 0;
  const auto *protocols = capabilities::protocols(protocol_count);
  for (size_t i = 0; i < protocol_count; ++i) {
    if (i) out << ',';
    out << "{\"id\":\"" << json_escape(protocols[i].name) << "\",\"name\":\""
        << json_escape(protocols[i].name) << "\",\"source\":\"builtin\",\"actions\":";
    append_token_strings(out, protocols[i].rx_actions);
    out << '}';
  }
  out << "],\"radio_profiles\":["
      << "{\"id\":\"board_raw\",\"kind\":\"raw\",\"radios\":[0,1],\"status\":\"board_validated\",\"user_selectable\":false,\"max_raw_pulses\":512},"
      << "{\"id\":\"board_packet_63\",\"kind\":\"packet\",\"radios\":[2],\"status\":\"board_validated\",\"user_selectable\":false,\"packet_length\":63}"
      << "],\"state\":{";
  bool first = true;
  for (const auto &entry : library.state()) {
    if (!first) out << ',';
    first = false;
    out << '"' << json_escape(entry.first) << "\":" << entry.second;
  }
  out << "}}";
  return out.str();
}

void RFLinkWebHandler::handle_user_library_(AsyncWebServerRequest *request) {
  if (request->method() == HTTP_GET) {
    const uint32_t start = millis();
#if defined(USE_ESP32) && defined(ESP_PLATFORM)
    ESP_LOGI(TAG, "HTTP /user-library summary: internal_free=%u largest_block=%u",
             unsigned(heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)),
             unsigned(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)));
#else
    ESP_LOGD(TAG, "HTTP /user-library: begin serialization");
#endif
    const std::string json = this->user_library_json_();
    const uint32_t elapsed = millis()-start;
    if (elapsed >= 200u || json.size() >= 32768u) {
      parent_->system_trace(elapsed >= 1000u ? "warn" : "info", "api", "",
        std::string("GET user-library serialization ")+std::to_string(elapsed)+" ms, "+std::to_string(json.size())+" bytes");
      ESP_LOGW(TAG, "HTTP /user-library serialization: %u ms, %u bytes",unsigned(elapsed),unsigned(json.size()));
    }
    return this->send_json_(request,json);
  }
  if (request->method() != HTTP_POST) return this->send_error_(request, 405, "POST required");
  if (request->arg("write_token") != std::to_string(parent_->web_write_token()))
    return this->send_error_(request, 403, "session token missing or expired");
  if (!parent_->library_ready()) return this->send_error_(request, 507, "user library storage unavailable");
  const auto action = request->arg("action");
  const auto id = request->arg("id");
  std::string error;
  bool ok = false;
  if (action == "create") {
    UserDecoder decoder;
    if (!UserLibrary::codebook(id, request->arg("name"), parent_->developer().captures(), decoder, error))
      return this->send_error_(request, 422, error);
    ok = parent_->save_user_decoder(std::move(decoder), error);
  } else if (action == "save_decoder") {
    UserDecoder decoder;
    decoder.id = id;
    decoder.name = request->arg("name");
    const auto radio = request->arg("radio");
    if (radio.size() != 1 || radio[0] < '0' || radio[0] > '2')
      return this->send_error_(request, 400, "radio must be 0, 1 or 2");
    decoder.radio = uint8_t(radio[0] - '0');
    decoder.kind = request->arg("kind") == "packet" ? FrameKind::PACKET : FrameKind::RAW;
    decoder.rx_source = request->arg("rx_source");
    decoder.tx_source = request->arg("tx_source");
    const std::string actions = request->arg("actions");
    if (decoder.rx_source.size() > user_vm::MAX_SOURCE || decoder.tx_source.size() > user_vm::MAX_SOURCE ||
        actions.size() > 255) return this->send_error_(request, 413, "decoder program too large");
    std::istringstream input(actions);
    std::string key;
    while (std::getline(input, key, ',')) decoder.actions.push_back(key);
    // Updates retain their original test vectors. New decoders use the live
    // labelled captures; no untested program can become active.
    if (const auto *previous = parent_->effective_user_decoder(id)) decoder.samples = previous->samples;
    else decoder.samples = parent_->developer().captures();
    ok = parent_->save_user_decoder(std::move(decoder), error);
  } else if (action == "import_decoder") {
    UserDecoder decoder;
    decoder.id = id;
    decoder.name = request->arg("name");
    uint32_t radio = 0, sample_count = 0;
    if (!parse_unsigned_text(request->arg("radio"), 2, radio) ||
        !parse_unsigned_text(request->arg("sample_count"), UserLibrary::MAX_SAMPLES, sample_count))
      return this->send_error_(request, 400, "invalid imported radio or sample count");
    decoder.radio = uint8_t(radio);
    const std::string imported_kind = request->arg("kind");
    if (imported_kind != "raw" && imported_kind != "packet")
      return this->send_error_(request, 400, "invalid imported frame kind");
    decoder.kind = imported_kind == "packet" ? FrameKind::PACKET : FrameKind::RAW;
    decoder.rx_source = request->arg("rx_source");
    decoder.tx_source = request->arg("tx_source");
    const std::string actions = request->arg("actions");
    if (decoder.rx_source.size() > user_vm::MAX_SOURCE || decoder.tx_source.size() > user_vm::MAX_SOURCE ||
        actions.size() > 255) return this->send_error_(request, 413, "decoder program too large");
    std::istringstream action_input(actions);
    std::string key;
    while (std::getline(action_input, key, ',')) decoder.actions.push_back(key);
    for (uint32_t n = 0; n < sample_count; ++n) {
      const auto prefix = std::string("s") + std::to_string(n) + "_";
      CapturedFrame sample;
      sample.label = request->arg((prefix + "label").c_str());
      uint32_t channel = 0;
      if (!parse_unsigned_text(request->arg((prefix + "channel").c_str()), 2, channel))
        return this->send_error_(request, 400, "invalid imported sample channel");
      sample.channel = uint8_t(channel);
      const std::string sample_kind = request->arg((prefix + "kind").c_str());
      if (sample_kind != "raw" && sample_kind != "packet")
        return this->send_error_(request, 400, "invalid imported sample kind");
      sample.kind = sample_kind == "packet" ? FrameKind::PACKET : FrameKind::RAW;
      if (!parse_raw_csv(request->arg((prefix + "raw").c_str()), sample.raw) ||
          !parse_byte_csv(request->arg((prefix + "packet").c_str()), sample.packet))
        return this->send_error_(request, 400, "invalid imported sample data");
      decoder.samples.push_back(std::move(sample));
    }
    ok = parent_->save_user_decoder(std::move(decoder), error);
  } else if (action == "restore_state") {
    uint32_t value = 0;
    if (id.empty() || id.size() > 128 || !parse_unsigned_text(request->arg("value"), UINT32_MAX, value) || value == 0)
      return this->send_error_(request, 400, "invalid protocol state");
    ok = parent_->restore_user_state(id, value, error);
  } else if (action == "enable" || action == "disable") {
    ok = parent_->enable_user_decoder(id, action == "enable", error);
  } else if (action == "remove_decoder") {
    ok = parent_->remove_user_decoder(id, error);
  } else if (action == "save_profile") {
    UserProfile profile;
    profile.id = id; profile.name = request->arg("name"); profile.protocol = request->arg("protocol");
    profile.learn_help = request->arg("learn_help");
    profile.pair_method = request->arg("pair_method").empty() ? "none" : request->arg("pair_method");
    // Profiles are declarative. The TX dispatcher never trusts these controls
    // unless the installed decoder, approved action and physical radio agree.
    { std::istringstream commands(request->arg("controls")); std::string item;
      while(std::getline(commands,item,',')) if(!item.empty()) profile.controls.push_back(item); }
    { std::istringstream steps(request->arg("setup_steps")); std::string line;
      while(std::getline(steps,line,'\n')) if(!line.empty()) profile.setup_steps.push_back(line); }
    std::istringstream input(request->arg("actions"));
    std::string key;
    while (std::getline(input, key, ',')) profile.expected_actions.push_back(key);
    ok = parent_->save_user_profile(std::move(profile), error);
  } else if (action == "remove_profile") {
    ok = parent_->remove_user_profile(id, error);
  } else return this->send_error_(request, 400, "unknown user library action");
  if (!ok) return this->send_error_(request, 422, error.empty() ? "operation failed" : error);
  return this->send_json_(request, "{\"ok\":true}");
}

void RFLinkWebHandler::handle_user_tx_(AsyncWebServerRequest *request) {
  if (request->method() != HTTP_POST) return this->send_error_(request, 405, "POST required");
  if (request->arg("write_token") != std::to_string(parent_->web_write_token()))
    return this->send_error_(request, 403, "session token missing or expired");
  const auto action = request->arg("action");
  const auto uid = request->arg("uid");
  std::string error;
  if (action == "approve" || action == "revoke") {
    if (!parent_->approve_user_transmit(uid, action == "approve", error))
      return this->send_error_(request, 409, error);
  } else if (action == "send") {
    if (!parent_->user_transmit(uid, request->arg("command"), error))
      return this->send_error_(request, 409, error);
  } else if (action == "profile") {
    if (!parent_->set_user_profile(uid, request->arg("id"), error))
      return this->send_error_(request, 409, error);
  } else if (action == "preview") {
    user_vm::TxFrame frame;
    if (!parent_->preview_user_transmit(request->arg("id"), request->arg("command"), frame, error,
                                        request->arg("uid")))
      return this->send_error_(request, 422, error);
    std::ostringstream out;
    out << "{\"ok\":true,\"radio\":" << unsigned(frame.radio)
        << ",\"repeats\":" << unsigned(frame.repeats)
        << ",\"raw\":[";
    for (size_t i = 0; i < frame.raw.size(); ++i) { if (i) out << ','; out << frame.raw[i]; }
    out << "],\"packet\":[";
    for (size_t i = 0; i < frame.packet.size(); ++i) { if (i) out << ','; out << unsigned(frame.packet[i]); }
    out << "],\"next_state\":{";
    bool first_state = true;
    for (const auto &entry : frame.next_state) {
      if (!first_state) out << ',';
      first_state = false;
      out << '"' << json_escape(entry.first) << "\":" << entry.second;
    }
    out << "}}";
    return this->send_json_(request, out.str());
  } else return this->send_error_(request, 400, "unknown user TX action");
  return this->send_json_(request, "{\"ok\":true}");
}

void RFLinkWebHandler::handle_candidate_(AsyncWebServerRequest *request) {
  if (request->method() != HTTP_POST) return this->send_error_(request, 405, "POST vereist");
  const auto action = request->arg("action");
  const auto identity = request->arg("identity");
  if (identity.empty()) return this->send_error_(request, 400, "identity ontbreekt");
  if (!parent_->storage_ready() || !parent_->storage_ok())
    return this->send_error_(request, 507, "apparaatopslag niet beschikbaar; geen wijziging uitgevoerd");
  if (action == "confirm") {
    const auto name = request->arg("name");
    // New wizard passes both lists explicitly. Historical callers without
    // either list retain their old behaviour; half-specified requests fail.
    const bool has_actions = request->hasArg("approved_actions");
    const bool has_fields = request->hasArg("approved_fields");
    if (has_actions != has_fields) return this->send_error_(request, 400, "beide functielijsten zijn verplicht");
    std::vector<std::string> approved_actions, approved_fields;
    if (has_actions) {
      const auto *candidate = parent_->devices().find_candidate(identity);
      if (!candidate) return this->send_error_(request, 409, "signaal niet meer beschikbaar");
      const auto parse = [](const std::string &text, std::vector<std::string> &target) {
        if (text.size() > 4096) return false;
        size_t pos = 0;
        while (pos < text.size()) {
          const size_t end = text.find(',', pos);
          const auto item = text.substr(pos, end == std::string::npos ? end : end - pos);
          if (item.empty() || item.size() > 255 || target.size() >= 32 ||
              std::find(target.begin(), target.end(), item) != target.end()) return false;
          target.push_back(item);
          if (end == std::string::npos) break;
          pos = end + 1;
          if (pos == text.size()) return false;
        }
        return true;
      };
      if (!parse(request->arg("approved_actions"), approved_actions) ||
          !parse(request->arg("approved_fields"), approved_fields))
        return this->send_error_(request, 400, "ongeldige functieselectie");
      for (const auto &key : approved_actions)
        if (std::find(candidate->actions.begin(), candidate->actions.end(), key) == candidate->actions.end())
          return this->send_error_(request, 400, "onbekende knopactie");
      for (const auto &key : approved_fields)
        if (std::none_of(candidate->fields.begin(), candidate->fields.end(),
                         [&](const FieldValue &f) { return f.key == key && capabilities::field(key.c_str()) != nullptr; }))
          return this->send_error_(request, 400, "onbekende meetwaarde");
    }
    auto *device = parent_->confirm_candidate(identity, name,
                                               has_actions ? &approved_actions : nullptr,
                                               has_fields ? &approved_fields : nullptr,
                                               request->arg("profile_id"));
    if (device == nullptr) return this->send_error_(request, 409, "candidate is nog niet klaar of bestaat niet");
    // Inleren is confirmed by the user; optional HA publication is a separate
    // explicit checkbox, never initiated by an unconfirmed RF observation.
    const std::string uid = device->uid;
    const bool requested = request->arg("ha") == "1";
    const bool published = requested && parent_->set_home_assistant_enabled(uid, true);
    return this->send_json_(request, std::string("{\"ok\":true,\"uid\":\"") + json_escape(uid) +
                            "\",\"ha_requested\":" + bool_json(requested) +
                            ",\"ha_enabled\":" + bool_json(published) + "}");
  }
  if (action == "dismiss") {
    if (!parent_->dismiss_candidate(identity)) return this->send_error_(request, 404, "voorstel niet gevonden");
    return this->send_json_(request, "{\"ok\":true}");
  }
  if (action == "link") {
    const auto uid = request->arg("uid");
    if (uid.empty()) return this->send_error_(request, 400, "uid ontbreekt");
    if (!parent_->link_candidate(identity, uid)) return this->send_error_(request, 409, "koppelen mislukt");
    return this->send_json_(request, "{\"ok\":true}");
  }
  this->send_error_(request, 400, "onbekende candidate-actie");
}

void RFLinkWebHandler::handle_device_(AsyncWebServerRequest *request) {
  if (request->method() != HTTP_POST) return this->send_error_(request, 405, "POST vereist");
  const auto action = request->arg("action");
  const auto uid = request->arg("uid");
  if (uid.empty() || uid.size() > 40 || parent_->devices().find_by_uid(uid) == nullptr)
    return this->send_error_(request, 404, "apparaat niet gevonden");
  if (!parent_->storage_ready() || !parent_->storage_ok())
    return this->send_error_(request, 507, "apparaatopslag niet beschikbaar; geen wijziging uitgevoerd");
  if (action == "set_profile") {
    std::string error;
    if (!parent_->set_user_profile(uid, request->arg("profile_id"), error))
      return this->send_error_(request, 409, error.empty() ? "profile update failed" : error);
    return this->send_json_(request, "{\"ok\":true}");
  }
  if (action == "rename") {
    const auto name = request->arg("name");
    if (name.empty() || name.size() > 80) return this->send_error_(request, 400, "naam moet 1-80 tekens bevatten");
    if (!parent_->rename_device(uid, name)) return this->send_error_(request, 507, "opslaan mislukt; wijziging niet toegepast");
    return this->send_json_(request, "{\"ok\":true}");
  }
  if (action == "approve_action" || action == "approve_field" ||
      action == "remove_action" || action == "remove_field") {
    const bool approve = action.rfind("approve_", 0) == 0;
    const std::string kind = action.find("_action") != std::string::npos ? "action" : "field";
    const std::string key = request->arg("key");
    if (key.empty() || key.size() > 255) return this->send_error_(request, 400, "ongeldige functie");
    if (!approve && parent_->devices().find_by_uid(uid)->home_assistant_enabled && !parent_->mqtt_connected())
      return this->send_error_(request, 503, "MQTT offline: functie niet verwijderd; voorkom verweesde HA-entiteiten");
    if (!parent_->change_capability(uid, kind, key, approve))
      return this->send_error_(request, 409, "functie niet gevonden, publiceren of opslaan mislukt; controleer MQTT en opslag");
    return this->send_json_(request, "{\"ok\":true}");
  }
  if (action == "somfy_pair") {
    std::string error;
    if (!parent_->somfy_pair(uid, error)) return this->send_error_(request, 409, error);
    return this->send_json_(request, "{\"ok\":true}");
  }
  if (action == "somfy_restore") {
    uint32_t address = 0, rolling = 0;
    if (!tx::unsigned_decimal(request->arg("address"), 0xFFFFFFu, address) || address == 0 ||
        !tx::unsigned_decimal(request->arg("rolling"), 0xFFFFu, rolling) || rolling == 0 ||
        (request->arg("paired") != "0" && request->arg("paired") != "1"))
      return this->send_error_(request, 400, "invalid Somfy backup state");
    std::string error;
    if (!parent_->restore_somfy_state(uid, address, static_cast<uint16_t>(rolling),
                                      request->arg("paired") == "1", error))
      return this->send_error_(request, 409, error);
    return this->send_json_(request, "{\"ok\":true}");
  }
  if (action == "ramses_pair") {
    if(request->arg("confirm")!="START_1FC9_EXPERIMENT")return this->send_error_(request,400,"Explicit RAMSES pairing confirmation required");
    std::string error;
    if(!parent_->enqueue_ramses_operation(uid,false,error))
      return this->send_error_(request,409,error);
    return this->send_json_(request,"{\"ok\":true,\"queued\":true,\"sent_unconfirmed\":false}");
  }
  if (action == "ramses_pair_test") {
    if(request->arg("confirm")!="TEST_AUTO_ONE_SHOT")return this->send_error_(request,400,"Explicit experimental AUTO test confirmation required");
    std::string error;
    if(!parent_->enqueue_ramses_operation(uid,true,error))
      return this->send_error_(request,409,error);
    return this->send_json_(request,"{\"ok\":true,\"queued\":true,\"radio_reported_sent\":false,\"device_confirmed\":false}");
  }
  if (action == "ramses_pair_verify") {
    if(request->arg("confirm")!="DEVICE_REACTION_CONFIRMED")return this->send_error_(request,400,"Physical user verification required");
    std::string error;
    if(!parent_->ramses_pair_verify(uid,error)) {
      parent_->system_trace("warn","pair",uid,"Pairing verification rejected: "+error);
      return this->send_error_(request,409,error);
    }
    return this->send_json_(request,"{\"ok\":true}");
  }
  if (action == "itho_pair") {
    const auto mode = request->arg("mode");
    if (!mode.empty() && mode != "standard" && mode != "compat")
      return this->send_error_(request, 400, "unsupported Itho pairing mode");
    std::string error;
    if (!parent_->itho_pair(uid, mode == "compat", error)) return this->send_error_(request, 409, error);
    return this->send_json_(request, "{\"ok\":true}");
  }
  if (action == "itho_unpair") {
    std::string error;
    if (!parent_->itho_unpair(uid, error)) return this->send_error_(request, 409, error);
    return this->send_json_(request, "{\"ok\":true}");
  }
  if (action == "itho_restore") {
    uint32_t address = 0, sequence = 0, device_type = 0;
    if (!tx::unsigned_decimal(request->arg("address"), 0xFFFFFFu, address) || address == 0 ||
        !tx::unsigned_decimal(request->arg("sequence"), 0xFFFFFFFEu, sequence) || sequence == 0 ||
        !tx::unsigned_decimal(request->arg("device_type"), 0xFFu, device_type) ||
        (request->arg("paired") != "0" && request->arg("paired") != "1"))
      return this->send_error_(request, 400, "invalid Itho backup state");
    std::string error;
    if (!parent_->restore_itho_state(uid, address, sequence, static_cast<uint8_t>(device_type),
                                     request->arg("paired") == "1", error))
      return this->send_error_(request, 409, error);
    return this->send_json_(request, "{\"ok\":true}");
  }
  if (action == "native_send") {
    const std::string command = request->arg("command");
    if (command.empty() || command.size() > 32 || command == "prog" ||
        !std::all_of(command.begin(), command.end(), [](char c) {
          return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
        }))
      return this->send_error_(request, 400, "unsupported direct command");
    std::string error;
    if (!parent_->native_transmit(uid, command, error)) return this->send_error_(request, 409, error);
    return this->send_json_(request, "{\"ok\":true}");
  }
  if (action == "tx_enable" || action == "tx_disable") {
    if (action == "tx_enable" && !parent_->tx_eligible(uid))
      return this->send_error_(request,409,"TX is not ready for this device; pair it or relearn it and verify the required radio");
    if (!parent_->set_transmit_enabled(uid,action == "tx_enable"))
      return this->send_error_(request,409,"TX toestemming niet opgeslagen of MQTT offline; geen wijziging");
    return this->send_json_(request,"{\"ok\":true}");
  }
  if (action == "ha_enable" || action == "ha_disable") {
    if (action == "ha_enable" && !ha::has_entities(*parent_->devices().find_by_uid(uid)))
      return this->send_error_(request, 409, "Geen ondersteunde sensorwaarden of knopacties bekend; eerst een signaal inleren");
    if (!parent_->mqtt_connected()) return this->send_error_(request, 503,
        "MQTT niet verbonden. Controleer HA MQTT-instellingen en broker. Geen wijziging uitgevoerd.");
    if (!parent_->set_home_assistant_enabled(uid, action == "ha_enable"))
      return this->send_error_(request, 507, "HA publicatie of opslag mislukt; controleer MQTT en probeer opnieuw");
    return this->send_json_(request, "{\"ok\":true}");
  }
  if (action == "delete") {
    const auto *d = parent_->devices().find_by_uid(uid);
    if (d != nullptr && d->home_assistant_enabled && !parent_->mqtt_connected())
      return this->send_error_(request, 503, "MQTT offline: zet HA eerst veilig uit of herstel broker. Apparaat behouden.");
    if (!parent_->remove_device(uid)) return this->send_error_(request, 507, "verwijderen of opslaan mislukt");
    return this->send_json_(request, "{\"ok\":true}");
  }
  if (action == "unlink_identity") {
    const auto identity = request->arg("identity");
    if (identity.empty() || identity.size() > 255) return this->send_error_(request, 400, "ongeldige RF-identiteit");
    if (parent_->devices().find_by_uid(uid)->rf_identities.size() <= 1)
      return this->send_error_(request, 409, "laatste signaal: verwijder het apparaat of vervang de code");
    if (!parent_->unlink_identity(uid, identity))
      return this->send_error_(request, 409, "ontkoppelen mislukt; niets gewijzigd");
    return this->send_json_(request, "{\"ok\":true}");
  }
  if (action == "replace_identity") {
    const auto previous = request->arg("identity");
    const auto replacement = request->arg("replacement");
    if (previous.empty() || replacement.empty() || previous.size() > 255 || replacement.size() > 255)
      return this->send_error_(request, 400, "ongeldige RF-identiteit");
    if (!parent_->replace_identity(uid, previous, replacement))
      return this->send_error_(request, 409, "nieuwe code niet bevestigd, al gekoppeld of ander protocol; niets gewijzigd");
    return this->send_json_(request, "{\"ok\":true}");
  }
  return this->send_error_(request, 400, "onbekende apparaat-actie");
}

void RFLinkWebHandler::handle_developer_(AsyncWebServerRequest *request) {
  const auto action = request->arg("action");
  if (action.empty()) return this->send_json_(request, this->developer_json_());
  int channel = 0;
  if (request->hasArg("channel")) channel = std::atoi(request->arg("channel").c_str());
  if (channel < 0 || channel >= parent_->channel_count()) return this->send_error_(request, 400, "ongeldig kanaal");
  if ((action == "baseline" || action == "listen") && !parent_->channel(channel).rx_ready) {
    return this->send_error_(request, 409, parent_->channel(channel).available ? "ontvanger niet beschikbaar" : "radiomodule niet aangesloten");
  }
  if (action == "baseline") {
    parent_->led_start_learning(static_cast<uint8_t>(channel), true, "baseline");
  } else if (action == "finish_baseline") {
    parent_->led_finish_baseline();
  } else if (action == "listen") {
    parent_->led_start_learning(static_cast<uint8_t>(channel), false, request->arg("label"));
  } else if (action == "reset") {
    parent_->led_stop_learning();
  } else {
    return this->send_error_(request, 400, "onbekende developer-actie");
  }
  this->send_json_(request, "{\"ok\":true}");
}

void RFLinkWebHandler::handle_led_settings_(AsyncWebServerRequest *request) {
  if (request->method() != HTTP_POST) return this->send_error_(request, 405, "POST vereist");
  const auto value = request->arg("brightness");
  const auto quiet = request->arg("quiet");
  if (value.empty() || value.size() > 3 || quiet.size() != 1 || (quiet != "0" && quiet != "1"))
    return this->send_error_(request, 400, "ongeldige LED-instellingen");
  for (char c : value) if (c < '0' || c > '9') return this->send_error_(request, 400, "ongeldige helderheid");
  const int brightness = std::atoi(value.c_str());
  if (brightness < 1 || brightness > 100) return this->send_error_(request, 400, "helderheid 1-100 vereist");
  parent_->set_led_settings(static_cast<uint8_t>(brightness), quiet == "1");
  this->send_json_(request, "{\"ok\":true}");
}

void RFLinkWebHandler::handle_mqtt_settings_(AsyncWebServerRequest *request) {
  if (request->method() == HTTP_GET) {
    std::string result = "{\"ok\":true,\"compiled\":";
    result += bool_json(parent_->mqtt_runtime_supported());
    result += ",\"write_token\":" + std::to_string(parent_->web_write_token());
    result += ",\"connected\":" + std::string(bool_json(parent_->mqtt_connected()));
    result += ",\"enabled\":" + std::string(bool_json(parent_->mqtt_runtime_enabled()));
    result += ",\"saved_password\":" + std::string(bool_json(parent_->mqtt_runtime_has_password()));
    result += ",\"host\":\"" + json_escape(parent_->mqtt_runtime_host()) + "\"";
    result += ",\"username\":\"" + json_escape(parent_->mqtt_runtime_user()) + "\"";
    result += ",\"port\":" + std::to_string(parent_->mqtt_runtime_port()) + "}";
    return this->send_json_(request, result);
  }
  if (request->method() != HTTP_POST) return this->send_error_(request, 405, "POST required");
  if (!parent_->mqtt_runtime_supported()) return this->send_error_(request, 501, "MQTT package not compiled");
  if (request->arg("write_token") != std::to_string(parent_->web_write_token()))
    return this->send_error_(request, 403, "Write token required");
  uint32_t port = 0;
  if (!parse_unsigned_text(request->arg("port"), 65535, port) || port == 0)
    return this->send_error_(request, 400, "Invalid MQTT port");
  const std::string enabled_arg = request->arg("enabled");
  if (enabled_arg != "0" && enabled_arg != "1") return this->send_error_(request, 400, "Invalid MQTT switch");
  if (!parent_->mqtt_runtime_save(request->arg("host"), static_cast<uint16_t>(port),
                                   request->arg("username"), request->arg("password"), enabled_arg == "1"))
    return this->send_error_(request, 400, "Invalid MQTT profile or storage unavailable");
  this->send_json_(request, "{\"ok\":true}");
}

void RFLinkWebHandler::handleRequest(AsyncWebServerRequest *request) {
  const auto url = this->url_(request);
  if (url == "/") {
    // ESPHome's ESP-IDF backend maps unsupported beginResponse() status codes
    // to HTTP 500. Use its native redirect helper so 302 is emitted correctly.
    request->redirect("/rflink");
    return;
  }
  if (url == "/favicon.ico") {
    auto *response = request->beginResponse(200, "image/x-icon", RFLINK_FAVICON_ICO, RFLINK_FAVICON_ICO_SIZE);
    response->addHeader("Cache-Control", "no-cache");
    request->send(response);
    return;
  }
  if (url == "/rflink/favicon.svg") {
    auto *response = request->beginResponse(200, "image/svg+xml", reinterpret_cast<const uint8_t *>(RFLINK_FAVICON_SVG), RFLINK_FAVICON_SVG_SIZE);
    response->addHeader("Cache-Control", "no-cache");
    request->send(response);
    return;
  }
  if (url == "/rflink" || url == "/rflink/") {
    auto *response = request->beginResponse(200, "text/html", reinterpret_cast<const uint8_t *>(RFLINK_UI_HTML), RFLINK_UI_HTML_SIZE);
    response->addHeader("Cache-Control", "no-cache");
    request->send(response);
    return;
  }
  if (url == "/rflink/api/status") return this->send_json_(request, this->status_json_());
  if (url == "/rflink/api/system-trace") {
    if(request->method()!=HTTP_GET)return this->send_error_(request,405,"GET required");
    return this->send_json_(request,this->system_trace_json_());
  }
  if (url == "/rflink/api/system-trace-config") {
    if (request->method() == HTTP_POST) {
      // This shares the existing LAN-only write-token model. NOT authentication.
      if (request->arg("write_token") != std::to_string(parent_->web_write_token()))
        return this->send_error_(request,403,"Write token required");
      uint32_t limit=0;
      if (!request->hasArg("limit") || !parse_unsigned_text(request->arg("limit"), SystemTrace::CAPACITY, limit))
        return this->send_error_(request,400,"Invalid system log limit");
      if (!parent_->trace_mutable().set_limit(limit))
        return this->send_error_(request,409,"System log limit update failed");
    } else if (request->method()!=HTTP_GET) return this->send_error_(request,405,"GET/POST required");
    std::ostringstream out;
    out << "{\"ok\":true,\"limit\":" << parent_->trace().limit()
        << ",\"max_limit\":" << SystemTrace::CAPACITY
        << ",\"write_token\":" << parent_->web_write_token() << '}';
    return this->send_json_(request,out.str());
  }
  if (url == "/rflink/api/lab-tx-probe") {
    if(request->method()!=HTTP_POST)return this->send_error_(request,405,"POST required");
    if(request->arg("confirm")!="LAB_10E0_ONE_SHOT")return this->send_error_(request,400,"Explicit lab confirmation required");
    std::string error;
    if(!parent_->ramses_tx_probe(error)) {
      parent_->record_radio_tx_event(2,"RamsesII","TX-test afgewezen: 10E0","",error,"error");
      return this->send_error_(request,409,error);
    }
    return this->send_json_(request,"{\"ok\":true,\"radio_reported_sent\":true,\"device_confirmed\":false}");
  }
  if (url == "/rflink/api/activity-export") return this->send_json_(request, this->activity_export_json_());
  if (url == "/rflink/api/rx-log") {
    if (request->method() != HTTP_GET) return this->send_error_(request, 405, "GET required");
    if (!parent_->rx_log_ready()) return this->send_error_(request, 503, "RX logger unavailable");
#if defined(USE_ESP32) && defined(ESP_PLATFORM)
    return this->send_rx_log_chunked_(request);
#else
    return this->send_json_(request, this->rx_log_json_(request));
#endif
  }
  if (url == "/rflink/api/rx-log-config") return this->handle_rx_log_config_(request);
  if (url == "/rflink/api/language") {
    if (request->method() == HTTP_GET)
      return this->send_json_(request, std::string("{\"language\":\"") + parent_->language() + "\"}");
    const auto language = request->arg("language");
    if (language != "en" && language != "nl" && language != "fr" && language != "de")
      return this->send_error_(request, 400, "Unsupported language");
    if (!parent_->set_language(language)) return this->send_error_(request, 503, "Could not save language");
    return this->send_json_(request, std::string("{\"ok\":true,\"language\":\"") + parent_->language() + "\"}");
  }
  if (url == "/rflink/api/led") return this->handle_led_settings_(request);
  if (url == "/rflink/api/mqtt-settings") return this->handle_mqtt_settings_(request);
  if (url == "/rflink/api/devices") {
    // Keep timing diagnostics separate from the RF/RAW log. A missing profile is
    // not assumed to be the cause of a panic: instrument the actual API path.
    const uint32_t start = millis();
    ESP_LOGD(TAG, "HTTP /devices: begin serialization");
    const std::string json = this->devices_json_();
    const uint32_t elapsed = millis() - start;
    if (elapsed >= 200u || json.size() >= 32768u) {
      parent_->system_trace(elapsed >= 1000u ? "warn" : "info", "api", "",
        std::string("GET devices serialization ") + std::to_string(elapsed) + " ms, " +
        std::to_string(json.size()) + " bytes, " +
        std::to_string(parent_->devices().devices().size()) + " devices");
      ESP_LOGW(TAG, "HTTP /devices serialization: %u ms, %u bytes", unsigned(elapsed), unsigned(json.size()));
    }
    return this->send_json_(request, json);
  }
  if (url == "/rflink/api/device") return this->handle_device_(request);
  if (url == "/rflink/api/developer") return this->handle_developer_(request);
  if (url == "/rflink/api/decoders") return this->send_json_(request, this->decoders_json_());
  if (url == "/rflink/api/library") {
    const uint32_t start = millis();
    ESP_LOGD(TAG, "HTTP /library: begin serialization");
    const std::string json = this->library_json_();
    const uint32_t elapsed = millis()-start;
    if (elapsed >= 200u || json.size() >= 32768u) {
      parent_->system_trace(elapsed >= 1000u ? "warn" : "info", "api", "",
         std::string("GET library serialization ")+std::to_string(elapsed)+" ms, "+std::to_string(json.size())+" bytes");
      ESP_LOGW(TAG, "HTTP /library serialization: %u ms, %u bytes",unsigned(elapsed),unsigned(json.size()));
    }
    return this->send_json_(request,json);
  }
  // Lightweight read-only catalog: avoid serializing user decoder captures and
  // the full user library just to populate a device profile selector.
  if (url == "/rflink/api/profile-catalog") {
    if (request->method() != HTTP_GET) return this->send_error_(request,405,"GET required");
    const std::string id = request->arg("id");
    if (id.size() > 32) return this->send_error_(request,400,"invalid profile id");
    std::ostringstream out;
    out << "{\"schema\":1,\"profiles\":[";
    size_t count=0;
    const auto *builtins=profiles::all(count);
    bool first=true, found=false;
    const auto &custom=parent_->user_library().profiles();
    for (const auto &p : custom) {
      if (!id.empty() && p.id != id) continue;
      if (!first) out << ',';
      first=false;found=true;
      out << "{\"id\":\"" << json_escape(p.id) << "\",\"name\":\"" << json_escape(p.name)
          << "\",\"protocol\":\"" << json_escape(p.protocol) << "\",\"source\":\"user\",\"version\":" << p.version
          << ",\"controls\":";append_strings(out,p.controls);
      out << ",\"expected_actions\":";append_strings(out,p.expected_actions);
      out << ",\"pair_method\":\"" << json_escape(p.pair_method) << "\"";
      if(!id.empty()) {
        out << ",\"learn_help\":\"" << json_escape(p.learn_help) << "\",\"setup_steps_i18n\":{\"en\":";
        append_strings(out,p.setup_steps);out << ",\"nl\":";append_strings(out,p.setup_steps);
        out << ",\"fr\":";append_strings(out,p.setup_steps);out << ",\"de\":";append_strings(out,p.setup_steps);out << '}';
      }
      out << '}';
    }
    for (size_t i=0;i<count;i++) {
      const auto &p=builtins[i];
      if (!id.empty() && id != p.id) continue;
      if (std::any_of(custom.begin(),custom.end(),[&](const UserProfile &u){return u.id==p.id;}))continue;
      if (!first) out << ',';
      first=false;found=true;
      out << "{\"id\":\"" << json_escape(p.id) << "\",\"name\":\"" << json_escape(p.name)
          << "\",\"protocol\":\"" << json_escape(p.protocol)
          << "\",\"source\":\"builtin\",\"manufacturer\":\"" << json_escape(p.manufacturer)
          << "\",\"family\":\"" << json_escape(p.family)
          << "\",\"device_type\":\"" << json_escape(p.device_type)
          << "\",\"compatibility\":\"" << json_escape(p.compatibility)
          << "\",\"hardware_validation\":\"" << json_escape(p.hardware_validation)
          << "\",\"pair_method\":\"" << json_escape(p.pair_method)
          << "\",\"pair_tx_required\":" << bool_json(p.pair_tx_required) << ",\"controls\":";
      append_token_strings(out,p.controls);
      out << ",\"expected_actions\":";append_token_strings(out,p.expected_actions);
      if(!id.empty()) {
        out << ",\"models\":";append_token_strings(out,p.models);
        out << ",\"learn_help\":\"" << json_escape(p.learn_en)
            << "\",\"learn_help_i18n\":{\"en\":\"" << json_escape(p.learn_en)
            << "\",\"nl\":\"" << json_escape(p.learn_nl)
            << "\",\"fr\":\"" << json_escape(p.learn_fr)
            << "\",\"de\":\"" << json_escape(p.learn_de)
            << "\"},\"notes_i18n\":{\"en\":\"" << json_escape(p.notes_en)
            << "\",\"nl\":\"" << json_escape(p.notes_nl)
            << "\",\"fr\":\"" << json_escape(p.notes_fr)
            << "\",\"de\":\"" << json_escape(p.notes_de) << "\"},\"setup_steps_i18n\":{\"en\":";
        append_token_strings(out,p.setup_en);out << ",\"nl\":";append_token_strings(out,p.setup_nl);
        out << ",\"fr\":";append_token_strings(out,p.setup_fr);
        out << ",\"de\":";append_token_strings(out,p.setup_de);out << '}';
      }
      out << '}';
    }
    out << "]}";
    if(!id.empty()&&!found)return this->send_error_(request,404,"profile not found");
    return this->send_json_(request,out.str());
  }
  if (url == "/rflink/api/user-decoder-detail") {
    if (request->method() != HTTP_GET) return this->send_error_(request, 405, "GET required");
    const std::string id = request->arg("id");
    if (id.empty() || id.size() > 32) return this->send_error_(request, 400, "invalid decoder id");
    const auto *d = parent_->user_library().decoder(id);
    if (!d) d = parent_->firmware_library().decoder(id);
    if (!d) return this->send_error_(request, 404, "decoder not found");
    // Keep even the decoder detail small. A 12-capture RAW codebook can
    // exceed available contiguous internal heap if serialized at once.
    const std::string sample_arg=request->arg("sample");
    uint32_t sample_index=0;
    if (!sample_arg.empty() && (!parse_unsigned_text(sample_arg, UserLibrary::MAX_SAMPLES-1, sample_index)
        || sample_index >= d->samples.size())) return this->send_error_(request, 404, "sample not found");
    std::ostringstream out;
    out << "{\"id\":\"" << json_escape(d->id) << "\",\"name\":\"" << json_escape(d->name)
        << "\",\"rx_source\":\"" << json_escape(d->rx_source)
        << "\",\"tx_source\":\"" << json_escape(d->tx_source)
        << "\",\"sample_count\":" << d->samples.size() << ",\"samples\":[";
    if (!sample_arg.empty()) {
      const auto &v=d->samples[sample_index];
      out << "{\"label\":\"" << json_escape(v.label) << "\",\"channel\":" << unsigned(v.channel)
          << ",\"kind\":\"" << (v.kind==FrameKind::RAW ? "raw" : "packet") << "\",\"raw\":[";
      for(size_t k=0;k<v.raw.size();++k){if(k)out<<',';out<<v.raw[k];}
      out << "],\"packet\":[";
      for(size_t k=0;k<v.packet.size();++k){if(k)out<<',';out<<unsigned(v.packet[k]);}
      out << "]}";
    }
    out << "]}";
    return this->send_json_(request, out.str());
  }
  if (url == "/rflink/api/user-library") return this->handle_user_library_(request);
  if (url == "/rflink/api/user-tx") return this->handle_user_tx_(request);
  if (url == "/rflink/api/candidate") return this->handle_candidate_(request);
  this->send_error_(request, 404, "endpoint niet gevonden");
}

}  // namespace rflink_universal
}  // namespace esphome

#endif
