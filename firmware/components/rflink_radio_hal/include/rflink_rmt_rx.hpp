#pragma once
#include <cstddef>
#include <cstdint>
#include <array>
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
// Stable numeric diagnostic stages exposed through the local status API.
// 0=none, 1=queue, 2=channel, 3=GPIO pull, 4=callbacks,
// 5=enable, 6=initial receive, 7=receive rearm.
enum class RmtRxFailureStage : uint8_t { none, queue, channel, gpio_pull, callbacks, enable, arm, rearm };
struct PulseStats {
  bool listening=false;
  uint32_t captures=0;
  uint32_t symbols=0;
  uint32_t queue_drops=0;
  uint32_t driver_errors=0;
  int32_t last_driver_error=0;
  RmtRxFailureStage last_driver_stage=RmtRxFailureStage::none;
  uint32_t short_frames=0; // too short or capture buffer full (legacy aggregate)
  uint32_t full_frames=0; // >= RMT input buffer size, never forwarded
  uint32_t filtered_frames=0; // >= 2 pulses before filter, < 2 after 30us filter
  uint32_t forwarded_frames=0; // actually delivered to the native decoder
  uint32_t last_symbols=0;
  uint32_t last_full_symbols=0; // Last full/incomplete capture, even after later valid RX.
  uint32_t last_forwarded_pulses=0; // Survived software filtering and sent to decoder.
  uint32_t capture_capacity_symbols=512; // Host RMT receive buffer, NOT mem_block_symbols.
  uint8_t last_capture_outcome=0; // 0=forwarded,1=short,2=full,3=filtered.
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
  // Non-DMA RMT hardware memory stays 96 symbols. The receive target is a
  // separate host-owned buffer: 256 was repeatedly saturated on 433.92 MHz.
  // 512 is a conservative diagnostic increase; never claim it handles
  // arbitrarily long frames or silently forward a truncated capture.
  static constexpr size_t kMaxSymbols=512;
  rmt_symbol_word_t symbols_[kMaxSymbols]{};
  // Keep scratch off the 8 KiB FreeRTOS RX worker stack. It is owned by the
  // receiver and never accessed by the ISR or HTTP task.
  std::array<int32_t,kMaxSymbols*2> scratch_{};
  rmt_channel_handle_t channel_=nullptr;
  QueueHandle_t done_=nullptr;
  // RMT hardware glitch filter, clamped to the actual RMT source clock.
  // The 30us RF pulse filter is applied separately after capture.
  uint32_t hardware_filter_ns_=0;
  RadioId radio_=RadioId::rf43342;
  RxFrameSink* sink_ = nullptr;
  mutable std::mutex stats_mutex_;
  PulseStats stats_{};
  // The ESP32-S3 toolchain does not guarantee std::atomic<uint32_t> is
  // lock-free. Use a short SMP FreeRTOS spinlock, never the std::mutex in ISR.
  // Both the ISR writer and task reader hold this same dedicated mux.
  mutable portMUX_TYPE queue_drop_mux_=portMUX_INITIALIZER_UNLOCKED;
  uint32_t queue_drops_isr_=0;
};
} // namespace rflink
