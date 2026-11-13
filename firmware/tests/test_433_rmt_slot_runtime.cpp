// Host simulation of the real RmtPulseRx ISR/worker handshake.
// The FreeRTOS queues and ESP-IDF RMT calls below are deliberately mocked:
// this validates slot ownership/backpressure, not ESP32-S3 RF timing.
#include "rflink_rmt_rx.hpp"
#include "esp_clk_tree.h"
#include "driver/gpio.h"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <deque>
#include <vector>

namespace {
struct Queue {
  size_t max_items, bytes;
  std::deque<std::vector<uint8_t>> items;
};
struct PreFilterEvidence {
  size_t pulses=0;
  uint16_t symbols=0;
  uint8_t stage=255;
  std::vector<int32_t> durations;
};
struct CaptureSink : rflink::RxFrameSink {
  std::vector<std::vector<int32_t>> frames;
  void on_packet(rflink::RadioId,uint32_t,const uint8_t*,size_t) override {}
  void on_pulses(rflink::RadioId,uint32_t,const int32_t* data,size_t n) override {
    frames.emplace_back(data,data+n);
  }
};
rmt_rx_event_callbacks_t callbacks{};
void* cb_ctx=nullptr;
rmt_symbol_word_t* current_buffer=nullptr;
rmt_symbol_word_t* completed_buffer=nullptr;
bool fail_next_arm=false;
unsigned arm_count=0;
}
extern "C" QueueHandle_t xQueueCreate(unsigned count,unsigned item_size) {
  return new Queue{count,item_size,{}};
}
extern "C" void vQueueDelete(QueueHandle_t h){delete static_cast<Queue*>(h);}
extern "C" int xQueueReceive(QueueHandle_t h,void* p,unsigned){
  auto* q=static_cast<Queue*>(h);
  if(!q || q->items.empty())return pdFALSE;
  std::memcpy(p,q->items.front().data(),q->bytes);
  q->items.pop_front();
  return pdTRUE;
}
extern "C" int xQueueSend(QueueHandle_t h,const void* p,unsigned){
  auto* q=static_cast<Queue*>(h);
  if(!q || q->items.size()>=q->max_items)return pdFALSE;
  const auto* b=static_cast<const uint8_t*>(p);
  q->items.emplace_back(b,b+q->bytes);
  return pdTRUE;
}
extern "C" int xQueueReceiveFromISR(QueueHandle_t h,void* p,BaseType_t*){
  return xQueueReceive(h,p,0);
}
extern "C" int xQueueSendFromISR(QueueHandle_t h,const void* p,BaseType_t*){
  return xQueueSend(h,p,0);
}
extern "C" esp_err_t esp_clk_tree_src_get_freq_hz(soc_module_clk_t,
    esp_clk_tree_src_freq_precision_t,uint32_t* hz){*hz=80000000;return ESP_OK;}
extern "C" esp_err_t gpio_set_pull_mode(gpio_num_t,int){return ESP_OK;}
extern "C" esp_err_t rmt_new_rx_channel(const rmt_rx_channel_config_t* cfg,rmt_channel_handle_t* out){
  assert(cfg->mem_block_symbols==96 && !cfg->flags.with_dma);
  *out=reinterpret_cast<void*>(0x1);
  return ESP_OK;
}
extern "C" esp_err_t rmt_rx_register_event_callbacks(rmt_channel_handle_t,
    const rmt_rx_event_callbacks_t* c,void* ctx){
  callbacks=*c;cb_ctx=ctx;return ESP_OK;
}
extern "C" esp_err_t rmt_enable(rmt_channel_handle_t){return ESP_OK;}
extern "C" esp_err_t rmt_disable(rmt_channel_handle_t){return ESP_OK;}
extern "C" esp_err_t rmt_del_channel(rmt_channel_handle_t){return ESP_OK;}
extern "C" esp_err_t rmt_receive(rmt_channel_handle_t,void* buf,size_t bytes,
    const rmt_receive_config_t* cfg){
  assert(cfg->signal_range_max_ns==5000000 && bytes==512*sizeof(rmt_symbol_word_t));
  if(fail_next_arm){fail_next_arm=false;current_buffer=nullptr;return ESP_FAIL;}
  current_buffer=static_cast<rmt_symbol_word_t*>(buf);
  ++arm_count;
  return ESP_OK;
}
extern "C" const char* esp_err_to_name(esp_err_t){return "mock";}
static void emit(size_t count,unsigned start){
  assert(current_buffer && callbacks.on_recv_done);
  assert(count<=512);
  for(size_t i=0;i<count;++i){
    current_buffer[i].duration0=start+i;
    current_buffer[i].level0=1;
    current_buffer[i].duration1=start+100+i;
    current_buffer[i].level1=0;
  }
  auto* const finishing=current_buffer;
  rmt_rx_done_event_data_t event{count};
  callbacks.on_recv_done(reinterpret_cast<void*>(0x1),&event,cb_ctx);
  completed_buffer=finishing;
}
int main(){
  rflink::RmtPulseRx rx;
  CaptureSink sink;
  std::vector<PreFilterEvidence> captures;
  rx.set_frame_sink(&sink);
  rx.set_capture_observer([](void* ctx,rflink::RadioId,const int32_t* pulses,
                             size_t count,uint16_t symbols,uint8_t stage){
    auto& out=*static_cast<std::vector<PreFilterEvidence>*>(ctx);
    out.push_back({count,symbols,stage,{pulses,pulses+count}});
  },&captures);
  assert(rx.start(rflink::RadioId::rf43392)==ESP_OK && arm_count==1);
  auto* first_slot=current_buffer;
  emit(2,100);
  assert(current_buffer!=first_slot && arm_count==2);
  auto* second_slot=current_buffer;
  emit(2,200);
  assert(current_buffer!=second_slot && current_buffer!=first_slot && arm_count==3);
  auto* third_slot=current_buffer;
  // Both completion slots are owned by the queue; third completion drops
  // *itself* and rearms its own slot, never overwriting the older two.
  emit(2,300);
  assert(current_buffer==third_slot && rx.stats().queue_drops==1 && arm_count==4);
  rx.poll_once();
  rx.poll_once();
  assert(sink.frames.size()==2);
  assert(captures.size()==2 && captures[0].pulses==4);
  assert(captures[0].symbols==2 && captures[0].stage==0);
  assert(captures[0].durations[0]==100 && captures[0].durations[1]==-200);
  assert(sink.frames[0].size()==5 && sink.frames[0][0]==100);
  assert(sink.frames[0].back()==5000); // ESPHome synthetic idle-level tail
  assert(sink.frames[1].size()==5 && sink.frames[1][0]==200);
  assert(sink.frames[1].back()==5000);
  assert(rx.stats().forwarded_frames==2);
  // Saturated one-frame receive buffer is still incomplete, never decoded.
  emit(512,500);
  rx.poll_once();
  assert(rx.stats().full_frames==1 && sink.frames.size()==2);
  assert(captures.size()==3 && captures.back().symbols==512);
  assert(captures.back().stage==2 && captures.back().pulses==1024);
  // Zero-duration EOF terminates capture even if later RMT words contain data.
  emit(3,600);
  assert(completed_buffer && completed_buffer!=current_buffer);
  completed_buffer[1].duration0=0;
  rx.poll_once();
  assert(sink.frames.size()==3);
  assert(sink.frames.back().size()==3 && sink.frames.back()[0]==600);
  assert(sink.frames.back()[2]==5000);
  assert(rx.stats().last_forwarded_pulses==3);
  // A single RMT symbol contains TWO raw durations: it can be forwarded.
  // Previously the legacy short counter also counted it as discarded,
  // making the capture-stage totals inconsistent.
  emit(1,1000);
  rx.poll_once();
  assert(rx.stats().forwarded_frames==4);
  assert(rx.stats().short_frames==1); // Only the saturated frame so far.
  assert(rx.stats().last_raw_pulses==2);
  assert(rx.stats().last_filtered_pulses==2);
  assert(rx.stats().last_capture_outcome==0);
  // Two raw durations can also collapse to one after the 30us filter.
  emit(1,20);
  rx.poll_once();
  assert(rx.stats().filtered_frames==1);
  assert(rx.stats().last_raw_pulses==2);
  assert(rx.stats().last_filtered_pulses==1);
  assert(rx.stats().last_capture_outcome==3);
  assert(rx.stats().forwarded_frames==4);
  // An actual single-duration capture is correctly considered short.
  emit(1,100);
  completed_buffer[0].duration1=0;
  rx.poll_once();
  const auto final_stats=rx.stats();
  assert(final_stats.last_raw_pulses==1);
  assert(final_stats.last_filtered_pulses==0);
  assert(final_stats.last_capture_outcome==1);
  assert(final_stats.short_frames==2); // 1 saturated + 1 raw-short.
  assert(final_stats.captures==7);
  assert(final_stats.captures==
         final_stats.forwarded_frames+
         final_stats.filtered_frames+
         (final_stats.short_frames-final_stats.full_frames)+
         final_stats.full_frames);
  // ISR rearm errors are propagated to a normal task and fail closed.
  fail_next_arm=true;
  emit(2,700);
  rx.poll_once();
  assert(!rx.stats().listening);
  assert(rx.stats().driver_errors>=1);
  assert(rx.stats().last_driver_stage==rflink::RmtRxFailureStage::rearm);
  assert(rx.stop()==ESP_OK);
  return 0;
}
