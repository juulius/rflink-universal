// Native ESP32-S3 RMT RX filter parity with the working ESPHome v0.6.6.
// Host simulation: does NOT prove over-the-air RF reception.
#include "rflink_rmt_filter.hpp"
#include "rflink_rmt_rx.hpp"
#include "esp_clk_tree.h"
#include "driver/gpio.h"
#include <cassert>
#include <cstdint>
#include <cstdio>

static_assert(rflink::rmt_effective_filter_ns(30000,80000000)==3187,
              "ESP32-S3 80 MHz RMT source clock needs 3187 ns, not 30000 ns");
static_assert(rflink::rmt_effective_filter_ns(30000,40000000)==6375,
              "Different RMT clock must adjust the hardware 8-bit filter");
static_assert(rflink::rmt_effective_filter_ns(30000,1000000)==30000,
              "No unnecessary reduction when requested filter is supported");
static_assert(rflink::rmt_effective_filter_ns(30000,0)==0,
              "Unavailable clock must be an explicit start failure");
static_assert(rflink::rmt_effective_filter_ns(0,80000000)==0);

namespace {
uint32_t source_hz=80000000;
esp_err_t clock_result=ESP_OK;
uint32_t clock_queries=0;
uint32_t receives=0,creates=0,deletes=0,enables=0,disables=0;
uint32_t max_clock_threshold=0;
rmt_receive_config_t last_cfg{};
unsigned last_gpio=0,first_gpio=0;
}

extern "C" esp_err_t esp_clk_tree_src_get_freq_hz(soc_module_clk_t,
    esp_clk_tree_src_freq_precision_t, uint32_t* value){
  ++clock_queries;
  if(clock_result!=ESP_OK)return clock_result;
  *value=source_hz;
  return ESP_OK;
}
extern "C" QueueHandle_t xQueueCreate(unsigned count,unsigned width){
  assert(count==4 && width==sizeof(uint32_t));
  return reinterpret_cast<void*>(uintptr_t{0x1234});
}
extern "C" void vQueueDelete(QueueHandle_t q){assert(q!=nullptr);}
extern "C" int xQueueReceive(QueueHandle_t,void*,unsigned){return pdFALSE;}
extern "C" int xQueueSendFromISR(QueueHandle_t,const void*,BaseType_t*){return pdTRUE;}
extern "C" esp_err_t gpio_set_pull_mode(gpio_num_t pin,int mode){
  assert((pin==6||pin==14) && mode==GPIO_PULLDOWN_ONLY);
  return ESP_OK;
}
extern "C" esp_err_t rmt_new_rx_channel(const rmt_rx_channel_config_t* cfg,
                                         rmt_channel_handle_t* out){
  assert(cfg->resolution_hz==1000000 && cfg->mem_block_symbols==96);
  assert(!cfg->flags.with_dma);
  assert(cfg->gpio_num==6||cfg->gpio_num==14);
  first_gpio=first_gpio?first_gpio:unsigned(cfg->gpio_num);
  last_gpio=unsigned(cfg->gpio_num);
  *out=reinterpret_cast<void*>(uintptr_t{0x1000}+creates++);
  return ESP_OK;
}
extern "C" esp_err_t rmt_rx_register_event_callbacks(
    rmt_channel_handle_t,const rmt_rx_event_callbacks_t* cb,void* ctx){
  assert(cb->on_recv_done!=nullptr && ctx!=nullptr);
  return ESP_OK;
}
extern "C" esp_err_t rmt_enable(rmt_channel_handle_t){++enables;return ESP_OK;}
extern "C" esp_err_t rmt_disable(rmt_channel_handle_t){++disables;return ESP_OK;}
extern "C" esp_err_t rmt_del_channel(rmt_channel_handle_t){++deletes;return ESP_OK;}
extern "C" esp_err_t rmt_receive(rmt_channel_handle_t,void* ptr,size_t len,
                                    const rmt_receive_config_t* cfg){
  assert(ptr!=nullptr && len>=96*sizeof(rmt_symbol_word_t));
  last_cfg=*cfg;
  ++receives;
  assert(source_hz>0);
  max_clock_threshold=static_cast<uint32_t>(
     uint64_t{255}*1000000000ULL/source_hz);
  if(cfg->signal_range_min_ns>max_clock_threshold)return ESP_ERR_INVALID_ARG;
  return ESP_OK;
}
int main(){
  using namespace rflink;
  RmtPulseRx receiver0,receiver1;
  assert(receiver0.start(RadioId::rf43342)==ESP_OK);
  assert(receiver1.start(RadioId::rf43392)==ESP_OK);
  assert(clock_queries==2 && receives==2);
  assert(first_gpio==6 && last_gpio==14);
  assert(last_cfg.signal_range_min_ns==3187);
  assert(last_cfg.signal_range_max_ns==5000000);
  assert(receiver0.stats().listening && receiver1.stats().listening);
  assert(receiver0.stats().last_driver_error==0);
  assert(receiver1.stats().last_driver_error==0);
  receiver0.poll_once();receiver1.poll_once();
  assert(receiver0.stop()==ESP_OK && receiver1.stop()==ESP_OK);
  assert(deletes==2 && disables==2);

  // Reproduce 0.7.0-exp.9 regression: 30,000 ns is rejected at 80 MHz.
  assert(kRmtRequestedFilterNs>max_clock_threshold);

  // Match the upstream ESPHome clock-based cap if the source clock changes.
  source_hz=40000000;
  assert(receiver0.start(RadioId::rf43342)==ESP_OK);
  assert(last_cfg.signal_range_min_ns==6375);
  assert(receiver0.stop()==ESP_OK);

  // Unlike a blind fixed value, absence of a valid clock fails closed.
  clock_result=ESP_FAIL;
  assert(receiver1.start(RadioId::rf43392)==ESP_FAIL);
  assert(receiver1.stats().last_driver_stage==RmtRxFailureStage::arm);
  assert(receiver1.stats().last_driver_error==ESP_FAIL);
  assert(!receiver1.stats().listening);
  assert(deletes==4);
  clock_result=ESP_OK;
  source_hz=0;
  assert(receiver1.start(RadioId::rf43392)==ESP_ERR_INVALID_STATE);
  assert(receiver1.stats().last_driver_stage==RmtRxFailureStage::arm);
  assert(deletes==5);

  puts("PASS RMT clock filter: ESPHome parity, dual 96-symbol RX, 5ms idle, source clock failures closed");
}
