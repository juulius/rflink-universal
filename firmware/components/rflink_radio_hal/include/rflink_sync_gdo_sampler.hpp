#pragma once
// Experimental GDO DATA+CLOCK sampling adapter for CC1101 radio 2.
//
// The RadioManager starts this driver only for main/Experimental builds.
// RC/Stable still use the original FIFO transport. RadioManager must
// exclusively own BOTH GDO pins (868 GDO0 also serves UART TX); the live
// task-only decoder never owns RF hardware or modifies modem registers.
//
// Benchmark-only ISR path. A GPIO interrupt per symbol at ~38.4 ksym/s may
// lose bits or consume excessive CPU: do not promote without bench evidence.
#include <cstddef>
#include <cstdint>
#include "esp_attr.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "rflink_radio_hal.hpp"
#include "rflink_clocked_bit_capture.hpp"
#include "rflink_sync_edge_ring.hpp"

namespace rflink {

struct SyncGdoSamplerStats {
  bool armed = false;
  uint32_t epoch = 0;
  uint32_t ring_pending = 0;
  uint32_t ring_dropped = 0;
  uint32_t transferred_edges = 0;
  uint32_t invalid_samples = 0;
  uint32_t reported_discontinuities = 0;
};

class Cc1101SyncGdoSampler {
 public:
  explicit Cc1101SyncGdoSampler(ClockedBitCapture& capture)
      : capture_(capture) {}
  ~Cc1101SyncGdoSampler();

  Cc1101SyncGdoSampler(const Cc1101SyncGdoSampler&) = delete;
  Cc1101SyncGdoSampler& operator=(const Cc1101SyncGdoSampler&) = delete;

  // Required prerequisites (enforced by future RadioManager integration):
  // - caller owns an exclusive RX lease for radio 2, no active TX/FIFO;
  // - CC1101 is already configured for synchronous serial DATA and CLOCK;
  // - the 868 GDO0 and GDO2 lines are inputs to the ESP32 (CC1101 outputs);
  // - 'rising_edge' has been established on a logic analyzer for this board.
  // GPIO service is installed with IRAM flag, or fails CLOSED if a different
  // service already owns the shared GPIO ISR. No SPI/register operations.
  esp_err_t start(RadioId radio, uint32_t epoch, bool rising_edge);

  // Manager must stop ISR and drain evidence before any GDO0 UART TX claim.
  // Serialized with poll_once(); never from ISR.
  esp_err_t stop();
  void poll_once(size_t max_edges = 1024);
  bool armed() const { return handler_installed_; }
  SyncGdoSamplerStats stats() const;

 private:
  static void IRAM_ATTR on_clock(void* arg);

  ClockedBitCapture& capture_;
  SyncClockEdgeRing ring_{};
  bool irq_service_owned_ = false;
  bool handler_installed_ = false;
  gpio_num_t data_gpio_ = static_cast<gpio_num_t>(-1);
  gpio_num_t clock_gpio_ = static_cast<gpio_num_t>(-1);
  uint32_t epoch_ = 0;
  uint32_t transferred_ = 0;
  uint32_t rejected_ = 0;
  uint32_t discontinuities_ = 0;
};

} // namespace rflink
