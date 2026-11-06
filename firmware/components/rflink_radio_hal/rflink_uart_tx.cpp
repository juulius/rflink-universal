#include "rflink_uart_tx.hpp"
#include <array>
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
namespace rflink {
namespace {
constexpr char TAG[] = "rflink.uarttx";
constexpr uint8_t SIDLE=0x36, STX=0x35, SFTX=0x3b;
constexpr std::array<uint8_t,7> kChangedRegs{{0x00,0x02,0x08,0x10,0x11,0x12,0x15}};
constexpr uint8_t kReadMaskDeviatn = 0xf7;
constexpr unsigned kTxMaxWaitMs=2000;
}
PhysicalTxResult Cc1101GdoUartTx::transmit(RadioId radio, const RfProfile& p, const EncodedTxFrame& frame) {
  if (!uart_tx_preset_valid(p) || !profile_compatible_with_radio(p,radio) ||
      !frame.wire || frame.length<4 || frame.length>512 ||
      frame.repeats==0 || frame.repeats>10 || frame.gap_ms>1000)
    return {false,true,TxFailureStage::unsupported};
  const auto i=static_cast<unsigned>(radio);
  if (i>=kRadioPins.size() || !bus_.state(radio).present)
    return {false,true,TxFailureStage::unsupported};
  const int pin=kRadioPins[i].gdo0;
  if (pin<0 || pin>48 || pin==46 || uart_is_driver_installed(UART_NUM_1))
    return {false,true,TxFailureStage::gpio_uart};
  std::array<uint8_t,kChangedRegs.size()> previous{};
  for(size_t r=0;r<kChangedRegs.size();++r) {
    if(bus_.read_register(radio,kChangedRegs[r],&previous[r])!=ESP_OK)
      return {false,true,TxFailureStage::snapshot};
  }
  TxFailureStage stage=TxFailureStage::none;
  bool installed=false, modified=false, sent=false;
  auto fail=[&](TxFailureStage s){ stage=s; return false; };
  do {
    modified=true;
    if(bus_.strobe(radio,SIDLE)!=ESP_OK || bus_.strobe(radio,SFTX)!=ESP_OK){fail(TxFailureStage::modem);break;}
    if(uart_driver_install(UART_NUM_1,512,1024,0,nullptr,0)!=ESP_OK){fail(TxFailureStage::gpio_uart);break;}
    installed=true;
    uart_config_t cfg{};
    cfg.baud_rate=static_cast<int>(p.uart_tx->baud);
    cfg.data_bits=UART_DATA_8_BITS;
    cfg.parity=UART_PARITY_DISABLE;
    cfg.stop_bits=UART_STOP_BITS_1;
    cfg.flow_ctrl=UART_HW_FLOWCTRL_DISABLE;
    cfg.source_clk=UART_SCLK_DEFAULT;
    if(uart_param_config(UART_NUM_1,&cfg)!=ESP_OK ||
       gpio_set_direction(static_cast<gpio_num_t>(pin),GPIO_MODE_OUTPUT)!=ESP_OK ||
       gpio_set_level(static_cast<gpio_num_t>(pin),1)!=ESP_OK ||
       uart_set_pin(UART_NUM_1,pin,UART_PIN_NO_CHANGE,
                    UART_PIN_NO_CHANGE,UART_PIN_NO_CHANGE)!=ESP_OK) {
      fail(TxFailureStage::gpio_uart); break;
    }
    const std::array<uint8_t,7> vals{{0x2e,0x2e,p.uart_tx->pktctrl0,
         p.uart_tx->mdmcfg4,p.uart_tx->mdmcfg3,p.uart_tx->mdmcfg2,p.uart_tx->deviatn}};
    bool registers_ok=true;
    for(size_t r=0;r<kChangedRegs.size();++r){
      uint8_t actual=0xff;
      if(bus_.write_register(radio,kChangedRegs[r],vals[r])!=ESP_OK ||
         bus_.read_register(radio,kChangedRegs[r],&actual)!=ESP_OK ||
         (actual & (kChangedRegs[r]==0x15?kReadMaskDeviatn:0xff)) !=
         (vals[r] & (kChangedRegs[r]==0x15?kReadMaskDeviatn:0xff))) {
        ESP_LOGE(TAG,"TX modem register 0x%02x readback failure",kChangedRegs[r]);
        registers_ok=false;break;
      }
    }
    if(!registers_ok){fail(TxFailureStage::modem);break;}
    bool all_sent=true;
    for(uint8_t repeat=0;repeat<frame.repeats;++repeat){
      if(bus_.strobe(radio,STX)!=ESP_OK){fail(TxFailureStage::enter_tx);all_sent=false;break;}
      bool in_tx=false;
      for(unsigned attempt=0;attempt<10;++attempt){
        uint8_t marc=0xff,rxbytes=0xff;
        if(bus_.read_runtime_status(radio,&marc,&rxbytes)==ESP_OK && (marc&0x1f)==0x13){in_tx=true;break;}
        vTaskDelay(pdMS_TO_TICKS(1));
      }
      if(!in_tx){fail(TxFailureStage::enter_tx);all_sent=false;break;}
      if(uart_write_bytes(UART_NUM_1,frame.wire,frame.length)!=static_cast<int>(frame.length)){
        fail(TxFailureStage::uart_write);all_sent=false;break;
      }
      const uint64_t bits=frame.length*10ull;
      const unsigned wait_ms=static_cast<unsigned>((bits*1000ull+p.uart_tx->baud-1)/p.uart_tx->baud)+250;
      if(wait_ms>kTxMaxWaitMs ||
         uart_wait_tx_done(UART_NUM_1,pdMS_TO_TICKS(wait_ms))!=ESP_OK){
        fail(TxFailureStage::uart_drain);all_sent=false;break;
      }
      esp_rom_delay_us(1000); // final UART stop bit before SIDLE
      if(bus_.strobe(radio,SIDLE)!=ESP_OK){fail(TxFailureStage::uart_drain);all_sent=false;break;}
      if(repeat+1<frame.repeats && frame.gap_ms)
        vTaskDelay(pdMS_TO_TICKS(frame.gap_ms));
    }
    sent=all_sent;
  }while(false);
  // Always release UART and restore all modified modem registers. This is
  // independent of TX success and independent of whether RX was active.
  bool restored=true;
  if(modified && bus_.strobe(radio,SIDLE)!=ESP_OK)restored=false;
  if(installed && uart_driver_delete(UART_NUM_1)!=ESP_OK)restored=false;
  if(modified && gpio_reset_pin(static_cast<gpio_num_t>(pin))!=ESP_OK)restored=false;
  if(modified) {
    for(size_t r=0;r<kChangedRegs.size();++r){
      uint8_t value=0xff;
      if(bus_.write_register(radio,kChangedRegs[r],previous[r])!=ESP_OK ||
         bus_.read_register(radio,kChangedRegs[r],&value)!=ESP_OK ||
         (value & (kChangedRegs[r]==0x15?kReadMaskDeviatn:0xff)) !=
         (previous[r] & (kChangedRegs[r]==0x15?kReadMaskDeviatn:0xff)))restored=false;
    }
  }
  if(!restored){ESP_LOGE(TAG,"Radio %u TX cleanup/restore failure",i);return {false,false,TxFailureStage::restore};}
  if(!sent){ESP_LOGW(TAG,"Radio %u UART TX failed, RX can be reinitialized; stage=%u",i,unsigned(stage));}
  return {sent,true,sent?TxFailureStage::none:stage};
}
} // namespace rflink
