#include "rflink_rmt_rx.hpp"
#include "rflink_pulse_filter.hpp"
#include "esp_clk_tree.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/task.h"
#include <array>
#include <algorithm>

namespace rflink {
namespace {
constexpr const char* TAG="rflink.rmt";
constexpr uint32_t kResolutionHz=1000000;  // 1 microsecond per tick
// Radio Gaga's 30us input filter is a requested *RF* filter, not directly
// valid as the ESP32-S3 RMT hardware glitch threshold. The latter is an
// 8-bit count clocked from the RMT source (~80MHz; 255 ticks ~= 3187ns).
constexpr uint32_t kRequestedFilterUs=30; // v0.6.6 remote_receiver filter
constexpr uint32_t kRequestedFilterNs=kRequestedFilterUs*1000u;
constexpr uint32_t kIdleNs=5000000;       // v0.6.6 idle=5ms
constexpr uint32_t kMaxHardwareFilterTicks=255u;
static_assert(kRequestedFilterUs>0 && kResolutionHz==1000000);
struct Completion { uint32_t symbols; uint8_t slot; };
}
RmtPulseRx::~RmtPulseRx(){ (void)stop(); }
bool IRAM_ATTR RmtPulseRx::on_rx_done(rmt_channel_handle_t, const rmt_rx_done_event_data_t* data, void* ctx){
  auto* self=static_cast<RmtPulseRx*>(ctx);
  if(!self || !self->done_ || !self->free_slots_ || !data)return false;
  BaseType_t woke=pdFALSE;
  const Completion event{static_cast<uint32_t>(data->num_symbols),self->active_slot_isr_};
  uint8_t next_slot=0;
  bool queued=false;
  // Rotate only to a FREE slot: queued/worker-owned buffers are immutable.
  if(xQueueReceiveFromISR(self->free_slots_,&next_slot,&woke)==pdTRUE){
    if(xQueueSendFromISR(self->done_,&event,&woke)==pdTRUE){
      self->active_slot_isr_=next_slot;
      queued=true;
    } else {
      // Keep the slot free if the completed-frame queue cannot accept it.
      (void)xQueueSendFromISR(self->free_slots_,&next_slot,&woke);
    }
  }
  if(!queued){
    // Backpressure: discard this completion, immediately reuse its own slot.
    // Never overwrite an earlier completed frame that the worker is reading.
    portENTER_CRITICAL_ISR(&self->queue_drop_mux_);
    ++self->queue_drops_isr_;
    portEXIT_CRITICAL_ISR(&self->queue_drop_mux_);
  }
  // Match ESPHome's early RMT rearm, without ISR logging/SPI/decoding/alloc.
  // RMT gets a different slot when an old completion has been queued.
  const esp_err_t armed=self->arm();
  if(armed!=ESP_OK){
    portENTER_CRITICAL_ISR(&self->queue_drop_mux_);
    self->rearm_error_isr_=static_cast<int32_t>(armed);
    portEXIT_CRITICAL_ISR(&self->queue_drop_mux_);
  }
  return woke==pdTRUE;
}
esp_err_t IRAM_ATTR RmtPulseRx::arm(){
  return rmt_receive(channel_,symbols_[active_slot_isr_],sizeof(symbols_[0]),&receive_config_);
}
void RmtPulseRx::release_queues(){
  if(done_){vQueueDelete(done_);done_=nullptr;}
  if(free_slots_){vQueueDelete(free_slots_);free_slots_=nullptr;}
}
esp_err_t RmtPulseRx::start(RadioId radio){
  if(channel_ || stats().listening)return ESP_ERR_INVALID_STATE;
  if(radio!=RadioId::rf43342 && radio!=RadioId::rf43392)return ESP_ERR_INVALID_ARG;
  radio_=radio;
  const auto pin=static_cast<gpio_num_t>(kRadioPins[static_cast<unsigned>(radio_)].gdo2);
  // Keep a precise, read-only failure cause available after early boot over Wi-Fi.
  // Stage 7 is reserved for asynchronous rearm failures below.
  const auto fail=[this,pin](esp_err_t e,RmtRxFailureStage stage,const char* step){
    {
      std::lock_guard<std::mutex> g(stats_mutex_);
      ++stats_.driver_errors;
      stats_.listening=false;
      stats_.last_driver_error=static_cast<int32_t>(e);
      stats_.last_driver_stage=stage;
    }
    // Host syntax stubs intentionally compile away ESP_LOGE arguments.
    (void)step;
    ESP_LOGE(TAG,"433 RX radio=%u GDO2=%d start stage=%u (%s) error=%s (0x%x)",
             unsigned(radio_),int(pin),unsigned(stage),step,esp_err_to_name(e),unsigned(e));
    return e;
  };
  done_=xQueueCreate(kCaptureSlots,sizeof(Completion));
  free_slots_=xQueueCreate(kCaptureSlots,sizeof(uint8_t));
  if(!done_ || !free_slots_){
    release_queues();
    return fail(ESP_ERR_NO_MEM,RmtRxFailureStage::queue,"queue");
  }
  active_slot_isr_=0;
  static_assert(kCaptureSlots<=255,"RMT slot indices are uint8_t");
  for(size_t index=1;index<kCaptureSlots;++index){
    const uint8_t slot=static_cast<uint8_t>(index);
    (void)xQueueSend(free_slots_,&slot,0);
  }
  portENTER_CRITICAL(&queue_drop_mux_);
  rearm_error_isr_=0;
  portEXIT_CRITICAL(&queue_drop_mux_);

  // ESPHome remote_receiver_rmt.cpp clamps the configured filter against
  // the RMT SOURCE clock (not the 1MHz output symbol resolution). On S3
  // this source is typically 80MHz: 30us would exceed an 8-bit limit.
  uint32_t source_hz=0;
  const esp_err_t clk_err=esp_clk_tree_src_get_freq_hz(
    static_cast<soc_module_clk_t>(RMT_CLK_SRC_DEFAULT),
    ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED,&source_hz);
  if(clk_err!=ESP_OK || source_hz==0){
    const esp_err_t e=clk_err!=ESP_OK?clk_err:ESP_ERR_INVALID_STATE;
    (void)fail(e,RmtRxFailureStage::channel,"RMT source clock query");
    release_queues();
    return e;
  }
  const uint64_t max_ns=(uint64_t(kMaxHardwareFilterTicks)*1000000000ULL)/source_hz;
  if(max_ns<1000u){
    (void)fail(ESP_ERR_NOT_SUPPORTED,RmtRxFailureStage::channel,"RMT hardware filter source clock");
    release_queues();
    return ESP_ERR_NOT_SUPPORTED;
  }
  // Stay one ns below the calculated 255-tick ceiling to avoid rounding
  // across an ESP-IDF version/clock tree boundary. 30us is preserved in
  // software for normal RF decoding after the RMT frame is copied.
  hardware_filter_ns_=static_cast<uint32_t>(
    std::min<uint64_t>(kRequestedFilterNs,max_ns-1u));
  receive_config_.signal_range_min_ns=hardware_filter_ns_;
  receive_config_.signal_range_max_ns=kIdleNs;
  rmt_rx_channel_config_t cfg{};
  cfg.gpio_num=pin;
  cfg.clk_src=RMT_CLK_SRC_DEFAULT;
  cfg.resolution_hz=kResolutionHz;
  // The proven v0.6.6 ESPHome configuration gave each of the two non-DMA
  // 433 MHz RX channels 96 symbols. Match that instead of the native 64.
  cfg.mem_block_symbols=96;
  cfg.flags.with_dma=false;
  RmtRxFailureStage stage=RmtRxFailureStage::channel;
  const char* step="channel allocation";
  bool enabled=false;
  esp_err_t e=rmt_new_rx_channel(&cfg,&channel_);
  if(e==ESP_OK) {
    // Match v0.6.6 remote_receiver GDO2 input/pulldown. Apply after RMT has
    // assigned the pin so its GPIO configuration does not undo the pull.
    stage=RmtRxFailureStage::gpio_pull;
    step="GDO2 pulldown";
    e=gpio_set_pull_mode(pin,GPIO_PULLDOWN_ONLY);
  }
  if(e==ESP_OK){
    stage=RmtRxFailureStage::callbacks;
    step="receive callback registration";
    rmt_rx_event_callbacks_t cb{};cb.on_recv_done=&RmtPulseRx::on_rx_done;
    e=rmt_rx_register_event_callbacks(channel_,&cb,this);
  }
  if(e==ESP_OK){
    stage=RmtRxFailureStage::enable;
    step="channel enable";
    e=rmt_enable(channel_);
    if(e==ESP_OK)enabled=true;
  }
  if(e==ESP_OK){
    stage=RmtRxFailureStage::arm;
    step="initial receive arm";
    e=arm();
  }
  if(e!=ESP_OK){
    (void)fail(e,stage,step);
    if(channel_){
      if(enabled)(void)rmt_disable(channel_);
      (void)rmt_del_channel(channel_);
      channel_=nullptr;
    }
    release_queues();
    return e;
  }
  {
    std::lock_guard<std::mutex> g(stats_mutex_);
    stats_.listening=true;
    stats_.last_driver_error=0;
    stats_.last_driver_stage=RmtRxFailureStage::none;
  }
  ESP_LOGI(TAG,"RX started: radio=%u GDO2=%d non-DMA symbols=96 hardware_filter=%lu ns software_filter=%lu us idle=5ms",
           unsigned(radio_),int(pin),
           static_cast<unsigned long>(hardware_filter_ns_),
           static_cast<unsigned long>(kRequestedFilterUs));
  return ESP_OK;
}
esp_err_t RmtPulseRx::stop(){
  if(!channel_){release_queues();return ESP_OK;}
  { std::lock_guard<std::mutex> g(stats_mutex_); stats_.listening=false; }
  // Disable RMT before releasing ISR-owned queues or capture slots.
  esp_err_t e=rmt_disable(channel_);
  esp_err_t d=rmt_del_channel(channel_);
  if(e==ESP_OK)e=d;
  channel_=nullptr;
  release_queues();
  return e;
}
void RmtPulseRx::poll_once(){
  int32_t rearm_error=0;
  portENTER_CRITICAL(&queue_drop_mux_);
  rearm_error=rearm_error_isr_;
  rearm_error_isr_=0;
  portEXIT_CRITICAL(&queue_drop_mux_);
  if(rearm_error!=0){
    {
      std::lock_guard<std::mutex> g(stats_mutex_);
      ++stats_.driver_errors;
      stats_.listening=false;
      stats_.last_driver_error=rearm_error;
      stats_.last_driver_stage=RmtRxFailureStage::rearm;
    }
    ESP_LOGE(TAG,"RX ISR rearm radio=%u stage=7 error=%ld",unsigned(radio_),long(rearm_error));
    return; // manager watchdog will stop and restart this radio safely.
  }
  if(!stats().listening || !done_)return;
  Completion event{};
  if(xQueueReceive(done_,&event,0)!=pdTRUE)return;
  // ISR has already armed a *different* slot. Copy only the completed slot;
  // worker/decoder never observe the buffer currently owned by RMT.
  const size_t bounded=std::min(size_t(event.symbols),kMaxSymbols);
  // Reusable receiver-owned scratch, not a 4 KiB automatic on the 8 KiB
  // RX worker stack. Only the serialized poller touches this buffer.
  int32_t* const copy=scratch_.data();
  size_t n=0;
  for(size_t i=0;i<bounded;++i){
    const auto& w=symbols_[event.slot][i];
    // ESPHome stops on the first EOF zero; following RMT words may be stale.
    if(!w.duration0)break;
    copy[n++]=w.level0?int32_t(w.duration0):-int32_t(w.duration0);
    if(!w.duration1)break;
    copy[n++]=w.level1?int32_t(w.duration1):-int32_t(w.duration1);
  }
  // Snapshot is also read by the HTTP worker on the other core.
  // Never expose unsynchronized non-atomic counter mutations to that task.
  const bool complete=event.symbols<kMaxSymbols;
  {
    std::lock_guard<std::mutex> g(stats_mutex_);
    stats_.last_symbols=event.symbols;
    stats_.last_capture_outcome=!complete?2u:(n<2?1u:0u);
    ++stats_.captures;
    stats_.symbols+=event.symbols;
    if(event.symbols<2 || !complete)++stats_.short_frames;
    if(!complete){
      ++stats_.full_frames;
      stats_.last_full_symbols=event.symbols;
    }
  }
  // Release the slot after the immutable copy, not after decoding; another
  // RMT completion can immediately claim it without touching our scratch.
  if(xQueueSend(free_slots_,&event.slot,0)!=pdTRUE){
    {
      std::lock_guard<std::mutex> g(stats_mutex_);
      ++stats_.driver_errors;
      stats_.listening=false;
      stats_.last_driver_error=ESP_FAIL;
      stats_.last_driver_stage=RmtRxFailureStage::rearm;
    }
    ESP_LOGE(TAG,"RX radio=%u free slot queue inconsistent",unsigned(radio_));
    return;
  }
  // Match Radio Gaga/ESPHome: remove <30us RF glitches *in software*.
  // The native RMT backend must use the much smaller hardware-safe
  // threshold; filtering is in place, in the task (never the RMT ISR).
  if(complete && n>=2 && sink_){
    // ESPHome's RMT decoder terminates nonempty frames with a synthetic
    // idle-level 5 ms pulse. Capture the *raw* final level before 30us
    // merging: the trailing glitch may disappear in the software filter.
    const bool last_raw_level_high=copy[n-1]>0;
    const size_t filtered=filter_short_pulses_inplace(
      copy,n,kRequestedFilterUs);
    // A complete 512-symbol target is at most 511 symbols (1022 pulses),
    // so the receiver-owned 1024-entry scratch has room for the idle tail.
    if(filtered>=2 && filtered<scratch_.size()){
      const size_t forwarded=append_idle_termination_inplace(
        copy,filtered,scratch_.size(),kIdleNs/1000u,last_raw_level_high);
      // Count exactly what the decoder receives, including the synthetic
      // idle tail (not another on-air edge).
      {
        std::lock_guard<std::mutex> g(stats_mutex_);
        ++stats_.forwarded_frames;
        stats_.last_forwarded_pulses=static_cast<uint32_t>(forwarded);
      }
      sink_->on_pulses(radio_,radio_==RadioId::rf43342?433420000u:433920000u,
                      copy,forwarded);
    } else {
      std::lock_guard<std::mutex> g(stats_mutex_);
      ++stats_.filtered_frames;
      stats_.last_capture_outcome=3;
    }
  }
}
PulseStats RmtPulseRx::stats() const {
  std::lock_guard<std::mutex> g(stats_mutex_);
  PulseStats snapshot=stats_;
  portENTER_CRITICAL(&queue_drop_mux_);
  snapshot.queue_drops=queue_drops_isr_;
  portEXIT_CRITICAL(&queue_drop_mux_);
  return snapshot;
}
} // namespace rflink
