#include "rflink_rmt_rx.hpp"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/task.h"
#include <array>
#include <algorithm>

namespace rflink {
namespace {
constexpr const char* TAG="rflink.rmt";
constexpr uint32_t kResolutionHz=1000000;  // 1 microsecond per tick
constexpr uint32_t kMinPulseNs=30000;      // v0.6.6 filter=30us
constexpr uint32_t kIdleNs=5000000;        // v0.6.6 idle=5ms
struct Completion { uint32_t symbols; };
}
RmtPulseRx::~RmtPulseRx(){ (void)stop(); }
bool IRAM_ATTR RmtPulseRx::on_rx_done(rmt_channel_handle_t, const rmt_rx_done_event_data_t* data, void* ctx){
  auto* self=static_cast<RmtPulseRx*>(ctx);
  if(!self || !self->done_ || !data)return false;
  Completion event{static_cast<uint32_t>(data->num_symbols)};
  BaseType_t woke=pdFALSE;
  // Queue only the symbol count; ISR does not decode, log, or touch SPI.
  if(xQueueSendFromISR(self->done_,&event,&woke)!=pdTRUE){
    // No counter mutations in ISR; overflow is visible as missing frames.
  }
  return woke==pdTRUE;
}
esp_err_t RmtPulseRx::arm(){
  rmt_receive_config_t cfg{};
  cfg.signal_range_min_ns=kMinPulseNs;
  cfg.signal_range_max_ns=kIdleNs;
  return rmt_receive(channel_,symbols_,sizeof(symbols_),&cfg);
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
    ESP_LOGE(TAG,"433 RX radio=%u GDO2=%d start stage=%u (%s) error=%s (0x%x)",
             unsigned(radio_),int(pin),unsigned(stage),step,esp_err_to_name(e),unsigned(e));
    return e;
  };
  done_=xQueueCreate(4,sizeof(Completion));
  if(!done_)return fail(ESP_ERR_NO_MEM,RmtRxFailureStage::queue,"queue");
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
    if(done_){vQueueDelete(done_);done_=nullptr;}
    return e;
  }
  {
    std::lock_guard<std::mutex> g(stats_mutex_);
    stats_.listening=true;
    stats_.last_driver_error=0;
    stats_.last_driver_stage=RmtRxFailureStage::none;
  }
  ESP_LOGI(TAG,"RX started: radio=%u GDO2=%d pulldown=on non-DMA symbols=96 filter=%uus idle=%ums",
           unsigned(radio_),int(pin),30,5);
  return ESP_OK;
}
esp_err_t RmtPulseRx::stop(){
  if(!channel_)return ESP_OK;
  { std::lock_guard<std::mutex> g(stats_mutex_); stats_.listening=false; }
  esp_err_t e=rmt_disable(channel_);
  esp_err_t d=rmt_del_channel(channel_);
  if(e==ESP_OK)e=d;
  channel_=nullptr;
  if(done_){vQueueDelete(done_);done_=nullptr;}
  return e;
}
void RmtPulseRx::poll_once(){
  if(!stats().listening || !done_)return;
  Completion event{};
  if(xQueueReceive(done_,&event,0)!=pdTRUE)return;
  // The RMT RX buffer is reused by arm(). Make a bounded, immutable copy
  // BEFORE rearming. Decode only in this task, never inside the RMT ISR.
  const size_t bounded=std::min(size_t(event.symbols),kMaxSymbols);
  std::array<int32_t,kMaxSymbols*2> copy{};
  size_t n=0;
  for(size_t i=0;i<bounded;++i){
    const auto& w=symbols_[i];
    if(w.duration0)copy[n++]=w.level0?int32_t(w.duration0):-int32_t(w.duration0);
    if(w.duration1)copy[n++]=w.level1?int32_t(w.duration1):-int32_t(w.duration1);
  }
  // Snapshot is also read by the HTTP worker on the other core.
  // Never expose unsynchronized non-atomic counter mutations to that task.
  const bool complete=event.symbols<kMaxSymbols;
  {
    std::lock_guard<std::mutex> g(stats_mutex_);
    stats_.last_symbols=event.symbols;
    ++stats_.captures;
    stats_.symbols+=event.symbols;
    if(event.symbols<2 || !complete)++stats_.short_frames;
  }
  const auto e=arm();
  if(e!=ESP_OK){
    {
      std::lock_guard<std::mutex> g(stats_mutex_);
      ++stats_.driver_errors;
      stats_.listening=false;
      stats_.last_driver_error=static_cast<int32_t>(e);
      stats_.last_driver_stage=RmtRxFailureStage::rearm;
    }
    ESP_LOGE(TAG,"RX rearm radio=%u stage=7 error=%s (0x%x)",unsigned(radio_),esp_err_to_name(e),unsigned(e));
  }
  if(complete && n>=2 && sink_)
    sink_->on_pulses(radio_,radio_==RadioId::rf43342?433420000u:433920000u,copy.data(),n);
}
PulseStats RmtPulseRx::stats() const {std::lock_guard<std::mutex> g(stats_mutex_);return stats_;}
} // namespace rflink
