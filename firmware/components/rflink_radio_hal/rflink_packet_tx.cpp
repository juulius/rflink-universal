#include "rflink_packet_tx.hpp"
#include "freertos/task.h"
#include "esp_log.h"
namespace rflink {
PhysicalTxResult Cc1101PacketTx::transmit(RadioId radio,const RfProfile& p,const EncodedTxFrame& frame){
  if(!packet_tx_preset_valid(p) || !profile_compatible_with_radio(p,radio) ||
     !frame.wire || frame.length!=p.packet_length || frame.length>63 ||
     frame.repeats<1 || frame.repeats>10 || frame.gap_ms>1000)
    return {false,true,TxFailureStage::unsupported};
  if(!bus_.state(radio).present)return {false,true,TxFailureStage::unsupported};
  Cc1101TxSession session(bus_,radio);
  bool sent=false;
  TxFailureStage stage=TxFailureStage::none;
  do {
    if(!session.prepare(p,false,p.packet_length)){stage=TxFailureStage::modem;break;}
    bool success=true;
    for(uint8_t n=0;n<frame.repeats;n++){
      if(bus_.strobe(radio,0x36)!=ESP_OK || bus_.strobe(radio,0x3b)!=ESP_OK ||
         bus_.write_tx_fifo(radio,frame.wire,frame.length)!=ESP_OK){
        stage=TxFailureStage::fifo_write;success=false;break;
      }
      if(bus_.strobe(radio,0x35)!=ESP_OK){stage=TxFailureStage::enter_tx;success=false;break;}
      // Do not report sent merely because a FIFO write succeeded. The radio
      // must be SEEN transmitting and then SEEN idle with an empty FIFO.
      bool seen_tx=false,completed=false;
      for(unsigned i=0;i<250;i++) {
        uint8_t marc=0xff,other=0xff,bytes=0xff;
        if(bus_.read_runtime_status(radio,&marc,&other)!=ESP_OK ||
           bus_.read_tx_fifo_status(radio,&bytes)!=ESP_OK){break;}
        const auto mode=marc&0x1f;
        if(mode==0x13)seen_tx=true;
        if(mode==0x16 || (bytes&0x80))break; // underflow
        if(seen_tx && mode==0x01 && (bytes&0x7f)==0){completed=true;break;}
        vTaskDelay(pdMS_TO_TICKS(1));
      }
      if(!completed){stage=TxFailureStage::fifo_wait;success=false;break;}
      if(n+1<frame.repeats && frame.gap_ms)vTaskDelay(pdMS_TO_TICKS(frame.gap_ms));
    }
    sent=success;
  }while(false);
  if(!session.restore())return {false,false,TxFailureStage::restore};
  return {sent,true,sent?TxFailureStage::none:stage};
}
} // namespace rflink
