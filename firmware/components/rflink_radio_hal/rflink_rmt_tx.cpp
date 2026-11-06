#include "rflink_rmt_tx.hpp"
#include <array>
#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"
#include "driver/gpio.h"
#include "freertos/task.h"
#include "esp_log.h"
namespace rflink {
namespace {constexpr size_t kMaxPulses=256; constexpr uint32_t kResolution=1000000;}
PhysicalTxResult Cc1101GdoRmtTx::transmit(RadioId radio,const RfProfile& p,const EncodedTxFrame& frame){
  if(!rmt_tx_preset_valid(p) || !profile_compatible_with_radio(p,radio) ||
     !frame.pulses || frame.pulse_count<1 || frame.pulse_count>kMaxPulses ||
     frame.repeats<1 || frame.repeats>10 || frame.gap_ms>1000)
    return {false,true,TxFailureStage::unsupported};
  const unsigned idx=static_cast<unsigned>(radio);
  if(idx>=kRadioPins.size() || !bus_.state(radio).present)return {false,true,TxFailureStage::unsupported};
  std::array<rmt_symbol_word_t,kMaxPulses> symbols{};
  uint64_t total_us=0;
  for(size_t i=0;i<frame.pulse_count;i++){
    const auto pulse=frame.pulses[i];
    if(!pulse.high_us || !pulse.low_us || pulse.high_us>32767 || pulse.low_us>32767)
      return {false,true,TxFailureStage::unsupported};
    symbols[i].duration0=pulse.high_us; symbols[i].level0=1;
    symbols[i].duration1=pulse.low_us; symbols[i].level1=0;
    total_us+=pulse.high_us+pulse.low_us;
  }
  if(total_us>500000) return {false,true,TxFailureStage::unsupported}; // half-second per repeat
  const int pin=kRadioPins[idx].gdo0;
  Cc1101TxSession session(bus_,radio);
  rmt_channel_handle_t channel=nullptr;
  rmt_encoder_handle_t encoder=nullptr;
  bool enabled=false, sent=false;
  TxFailureStage stage=TxFailureStage::none;
  do {
    if(!session.prepare(p,true,0)){stage=TxFailureStage::modem;break;}
    rmt_tx_channel_config_t cfg{};
    cfg.gpio_num=static_cast<gpio_num_t>(pin);
    cfg.clk_src=RMT_CLK_SRC_DEFAULT;
    cfg.resolution_hz=kResolution;
    cfg.mem_block_symbols=64;
    cfg.trans_queue_depth=1;
    cfg.flags.with_dma=false;
    if(rmt_new_tx_channel(&cfg,&channel)!=ESP_OK){stage=TxFailureStage::rmt_config;break;}
    rmt_copy_encoder_config_t enc_cfg{};
    if(rmt_new_copy_encoder(&enc_cfg,&encoder)!=ESP_OK){stage=TxFailureStage::rmt_config;break;}
    if(rmt_enable(channel)!=ESP_OK){stage=TxFailureStage::rmt_config;break;}
    enabled=true;
    rmt_transmit_config_t tx_cfg{};
    tx_cfg.loop_count=0;
    tx_cfg.flags.eot_level=0;
    bool success=true;
    for(uint8_t n=0;n<frame.repeats;n++) {
      if(bus_.strobe(radio,0x35)!=ESP_OK){stage=TxFailureStage::enter_tx;success=false;break;}
      uint8_t marc=0xff,other=0xff;
      bool tx_state=false;
      for(int j=0;j<10;j++){
        if(bus_.read_runtime_status(radio,&marc,&other)==ESP_OK && (marc&0x1f)==0x13){tx_state=true;break;}
        vTaskDelay(pdMS_TO_TICKS(1));
      }
      if(!tx_state){stage=TxFailureStage::enter_tx;success=false;break;}
      if(rmt_transmit(channel,encoder,symbols.data(),frame.pulse_count*sizeof(rmt_symbol_word_t),&tx_cfg)!=ESP_OK){
        stage=TxFailureStage::rmt_send;success=false;break;
      }
      const int wait_ms=static_cast<int>((total_us+999)/1000+150);
      if(rmt_tx_wait_all_done(channel,wait_ms)!=ESP_OK){stage=TxFailureStage::rmt_wait;success=false;break;}
      if(bus_.strobe(radio,0x36)!=ESP_OK){stage=TxFailureStage::rmt_wait;success=false;break;}
      if(n+1<frame.repeats && frame.gap_ms)vTaskDelay(pdMS_TO_TICKS(frame.gap_ms));
    }
    sent=success;
  }while(false);
  // No RMT payload memory is recycled before queue drained or channel stopped.
  bool cleaned=true;
  if(channel && enabled && rmt_disable(channel)!=ESP_OK)cleaned=false;
  if(encoder && rmt_del_encoder(encoder)!=ESP_OK)cleaned=false;
  if(channel && rmt_del_channel(channel)!=ESP_OK)cleaned=false;
  if(session.touched() && gpio_reset_pin(static_cast<gpio_num_t>(pin))!=ESP_OK)cleaned=false;
  if(!session.restore() || !cleaned)return {false,false,TxFailureStage::restore};
  return {sent,true,sent?TxFailureStage::none:stage};
}
} // namespace rflink
