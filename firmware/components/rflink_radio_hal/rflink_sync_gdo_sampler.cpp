#include "rflink_sync_gdo_sampler.hpp"
#include "driver/gpio.h"
#include "esp_memory_utils.h"
#include "esp_intr_alloc.h"
#include "esp_timer.h"
#include "soc/gpio_reg.h"
#include "soc/soc.h"

namespace rflink {

Cc1101SyncGdoSampler::~Cc1101SyncGdoSampler() {
  // A manager must stop the sampler while the radio and worker are quiescent.
  (void)stop();
}

void IRAM_ATTR Cc1101SyncGdoSampler::on_clock(void* arg) {
  auto* self = static_cast<Cc1101SyncGdoSampler*>(arg);
  if (!self || !self->active_.load(std::memory_order_relaxed)) return;
  const int pin = static_cast<int>(self->data_gpio_);
  // A direct GPIO input register read avoids an opaque high-rate driver
  // function call from cache-disabled IRAM interrupt context.
  const uint32_t input = pin < 32 ? REG_READ(GPIO_IN_REG) : REG_READ(GPIO_IN1_REG);
  const bool high = (input & (1u << (pin < 32 ? pin : pin - 32))) != 0;
  const uint64_t now_us = static_cast<uint64_t>(esp_timer_get_time());
  (void)self->ring_.push_isr(high, now_us);
}

esp_err_t Cc1101SyncGdoSampler::start(RadioId radio,
                                       uint32_t epoch, bool rising_edge) {
  if (armed()) return ESP_ERR_INVALID_STATE;
  if (radio != RadioId::rf86830 || epoch == 0) return ESP_ERR_INVALID_ARG;
  const auto& pins = kRadioPins[static_cast<unsigned>(radio)];
  if (pins.gdo0 == pins.gdo2 ||
      pins.gdo0 < 0 || pins.gdo2 < 0 ||
      !GPIO_IS_VALID_GPIO(pins.gdo0) || !GPIO_IS_VALID_GPIO(pins.gdo2))
    return ESP_ERR_INVALID_ARG;
  // Both the callback object and ring must remain available with flash cache
  // disabled. Heap/PSRAM placement violates the ISR contract.
  if (!esp_ptr_internal(this)) return ESP_ERR_INVALID_ARG;

  data_gpio_ = static_cast<gpio_num_t>(pins.gdo0);
  clock_gpio_ = static_cast<gpio_num_t>(pins.gdo2);
  epoch_ = epoch;
  ring_.reset_epoch(); // safe: no ISR installed and no active producer
  if (!capture_.begin_epoch(epoch)) return ESP_ERR_INVALID_ARG;

  gpio_config_t cfg{};
  cfg.pin_bit_mask = (uint64_t(1) << pins.gdo0) |
                     (uint64_t(1) << pins.gdo2);
  cfg.mode = GPIO_MODE_INPUT;
  cfg.pull_up_en = GPIO_PULLUP_DISABLE;
  cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
  cfg.intr_type = GPIO_INTR_DISABLE;
  esp_err_t e = gpio_config(&cfg);
  if (e != ESP_OK) return e;

  if (!irq_service_owned_) {
    // Fail closed if another global GPIO service is installed: its IRAM
    // configuration is unknown. Do not uninstall a service we do not own.
    e = gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    if (e != ESP_OK) return e;
    irq_service_owned_ = true;
  }

  e = gpio_set_intr_type(clock_gpio_,
                         rising_edge ? GPIO_INTR_POSEDGE : GPIO_INTR_NEGEDGE);
  if (e != ESP_OK) return e;
  e = gpio_isr_handler_add(clock_gpio_, &Cc1101SyncGdoSampler::on_clock, this);
  if (e != ESP_OK) return e;
  handler_installed_ = true;
  active_.store(true, std::memory_order_release);
  e = gpio_intr_enable(clock_gpio_);
  if (e != ESP_OK) {
    active_.store(false, std::memory_order_release);
    (void)gpio_isr_handler_remove(clock_gpio_);
    handler_installed_ = false;
  }
  return e;
}

void Cc1101SyncGdoSampler::poll_once(size_t max_edges) {
  // Must be serialized with start/stop by the manager's RX worker.
  if (!epoch_) return;
  constexpr size_t kAbsoluteLimit = SyncClockEdgeRing::kCapacity;
  if (max_edges > kAbsoluteLimit) max_edges = kAbsoluteLimit;
  SyncClockEdge edge{};
  for (size_t i = 0; i < max_edges && ring_.pop(edge); ++i) {
    if (edge.lost_before) {
      capture_.report_lost_edges(edge.lost_before);
      ++discontinuities_;
    }
    if (!capture_.append(edge.data_high, edge.timestamp_us, epoch_))
      ++rejected_;
    else
      ++transferred_;
  }
}

esp_err_t Cc1101SyncGdoSampler::stop() {
  active_.store(false, std::memory_order_release);
  esp_err_t err = ESP_OK;
  if (handler_installed_) {
    err = gpio_intr_disable(clock_gpio_);
    const esp_err_t rm = gpio_isr_handler_remove(clock_gpio_);
    if (err == ESP_OK) err = rm;
    handler_installed_ = false;
  }
  if (epoch_) {
    // Hardware interrupt is no longer a producer. Drain in original order;
    // if last samples were dropped while full, mark their loss explicitly.
    while (ring_.pending() > 0) poll_once(SyncClockEdgeRing::kCapacity);
    const uint32_t trailing = ring_.trailing_loss_after_stop();
    if (trailing) {
      capture_.report_lost_edges(trailing);
      ++discontinuities_;
    }
    capture_.end_window();
    epoch_ = 0;
  }
  return err;
}

SyncGdoSamplerStats Cc1101SyncGdoSampler::stats() const {
  SyncGdoSamplerStats s{};
  s.armed = armed();
  s.epoch = epoch_;
  s.ring_pending = ring_.pending();
  s.ring_dropped = ring_.dropped_total();
  s.transferred_edges = transferred_;
  s.invalid_samples = rejected_;
  s.reported_discontinuities = discontinuities_;
  return s;
}

} // namespace rflink
