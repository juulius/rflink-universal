#pragma once
#include <cstddef>
#include <cstdint>
#include <mutex>
#include "esp_err.h"
#include "driver/rmt_rx.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "rflink_rf_profiles.hpp"
#include "rflink_rx_sink.hpp"

namespace rflink {
// RMT captures raw pulses, NEVER decodes protocols. No TX, no rolling-code state.
// Owner: RadioManager -> RmtPulseRx; no other component may attach to these GDO2 pins.
struct PulseStats {
  bool listening=false;
  uint32_t captures=0;
  uint32_t symbols=0;
  uint32_t queue_drops=0;
  uint32_t driver_errors=0;
  uint32_t short_frames=0;
  uint32_t last_symbols=0;
};
class RmtPulseRx {
public:
  RmtPulseRx()=default;
  ~RmtPulseRx();
  RmtPulseRx(const RmtPulseRx&)=delete;
  RmtPulseRx& operator=(const RmtPulseRx&)=delete;
  esp_err_t start(RadioId radio);
  esp_err_t stop();
  // Call from a regular task; nonblocking. Later a decoder input queue will
  // copy durations here before the underlying RMT receive buffer is reused.
  void poll_once();
  void set_frame_sink(RxFrameSink* sink) { sink_ = sink; }
  PulseStats stats() const;
  RadioId radio() const {return radio_;}
private:
  static bool IRAM_ATTR on_rx_done(rmt_channel_handle_t, const rmt_rx_done_event_data_t*, void*);
  esp_err_t arm();
  static constexpr size_t kMaxSymbols=256;
  rmt_symbol_word_t symbols_[kMaxSymbols]{};
  rmt_channel_handle_t channel_=nullptr;
  QueueHandle_t done_=nullptr;
  RadioId radio_=RadioId::rf43342;
  RxFrameSink* sink_ = nullptr;
  mutable std::mutex stats_mutex_;
  PulseStats stats_{};
};
} // namespace rflink
