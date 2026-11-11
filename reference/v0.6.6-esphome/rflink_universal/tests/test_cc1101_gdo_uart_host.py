"""Compile the real GDO-UART adapter against deterministic ESP-IDF/CC1101
mocks; exercises UART install order, precise errors and RX rollback.
This is NOT a real ESPHome compile or physical RF test.
"""
from pathlib import Path
from tempfile import TemporaryDirectory
import subprocess
root=Path(__file__).resolve().parents[1]
header=(root/'components/rflink_universal/cc1101_gdo_uart_tx.h').read_text().replace('#pragma once\n','')
for line in header.splitlines():
    if line.startswith('#include "'):
        header=header.replace(line+'\n','')
mock=r'''#include <array>
#include <cassert>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#define USE_ESP32
using esp_err_t=int;
constexpr int ESP_OK=0, ESP_ERR_INVALID_STATE=0x103, ESP_ERR_INVALID_ARG=0x102;
inline const char *esp_err_to_name(esp_err_t v) {return v==ESP_OK?"ESP_OK":v==ESP_ERR_INVALID_STATE?"ESP_ERR_INVALID_STATE":"ESP_ERR_INVALID_ARG";}
constexpr int UART_NUM_1=1,UART_PIN_NO_CHANGE=-1,GPIO_MODE_OUTPUT=1;
constexpr int UART_DATA_8_BITS=8,UART_PARITY_DISABLE=0,UART_STOP_BITS_1=1;
constexpr int UART_HW_FLOWCTRL_DISABLE=0,UART_SCLK_DEFAULT=0;
using gpio_num_t=int;
struct uart_config_t {int baud_rate,data_bits,parity,stop_bits,flow_ctrl,source_clk;};
inline int pdMS_TO_TICKS(unsigned t){return t;}
static bool installed=false,fail_config=false,fail_register=false;
static int installs=0, configs=0, write_calls=0;
static bool pin_is_input=true, packet_irq=false, uart_released=false;
static int irq_rearms=0;
inline bool uart_is_driver_installed(int){return installed;}
inline esp_err_t uart_driver_install(int,int,int,int,void*,int){installed=true;++installs;return ESP_OK;}
inline esp_err_t uart_param_config(int,const uart_config_t*){++configs;return !installed?ESP_ERR_INVALID_STATE:fail_config?ESP_ERR_INVALID_ARG:ESP_OK;}
inline esp_err_t uart_set_pin(int,int,int,int,int){return installed?ESP_OK:ESP_ERR_INVALID_STATE;}
inline esp_err_t uart_driver_delete(int){installed=false;uart_released=true;return ESP_OK;}
inline int uart_write_bytes(int,const uint8_t*,size_t n){++write_calls;return static_cast<int>(n);}
inline esp_err_t uart_wait_tx_done(int,int){return ESP_OK;}
inline esp_err_t gpio_set_direction(int,int){pin_is_input=false;return ESP_OK;}
inline esp_err_t gpio_set_level(int,int){return ESP_OK;}
inline esp_err_t gpio_reset_pin(int){pin_is_input=false;packet_irq=false;return ESP_OK;}
namespace esphome { inline void delay(int){} namespace cc1101 {
 struct CC1101Component {
  std::array<uint8_t,64> regs{};uint8_t marc=0x0d;
  CC1101Component(){reset();}
  bool is_failed()const{return false;}
  void set_idle(){marc=0x01;}
  void set_packet_mode(bool enabled){
    if(enabled){ assert(!installed && uart_released && pin_is_input);
      packet_irq=true;++irq_rearms;
    }else{packet_irq=false;}
  }
  void begin_rx(){assert(!installed);pin_is_input=true;marc=0x0d;}
  void reset(){regs.fill(0);regs[0]=0x2e;regs[2]=0x2e;marc=0x0d;}
 };}
 namespace rflink_universal {
 class RadioModeManager {public:bool itho_ready()const{return true;}};
 namespace ramses_spi {
  class CC1101Bus {
    cc1101::CC1101Component &dev;
  public:
    explicit CC1101Bus(cc1101::CC1101Component &r):dev(r){}
    bool read(uint8_t r,uint8_t &v){v=r==0x35?dev.marc:dev.regs.at(r);if(r==0x15)v&=0xF7;/* DEVIATN bit 3 is reserved/read-zero */if(fail_register&&r==0x08&&dev.regs[0x08]==0x30)v^=0x01;return true;}
    bool write(uint8_t r,uint8_t v){dev.regs.at(r)=v;return true;}
    bool strobe(uint8_t cmd){if(cmd==0x35)dev.marc=0x13;if(cmd==0x36)dev.marc=0x01;return true;}
  };}
 }}
'''
# Compile the real shared rearm helper within the same CC1101 mock.
helper=(root/'components/rflink_universal/cc1101_packet_rx_restore.h').read_text()
helper='\n'.join(x for x in helper.splitlines() if not x.startswith('#include ') and x!='#pragma once')
main=r'''using namespace esphome::rflink_universal;
static std::vector<std::string> phases;
static void cb(void*,const char *stage,bool ok){ phases.push_back(std::string(stage)+(ok?" OK":" FAILED")); }
int main(){
  const uint8_t payload[5]={0xff,0,0x33,0x55,0x53};
  const GdoUartPreset preset{"fsk_86830_ramses_gdo_serial_lab",38400,0x30,0x5a,0x83,0,0x40};
  {esphome::cc1101::CC1101Component radio;RadioModeManager mode;
   CC1101GdoUartTx tx(radio,mode,2,16,preset,true);tx.set_trace_observer(cb,nullptr);
   std::string err;assert(tx.send(2,preset.name,payload,5,1,0,err));
   assert(err.empty()&&installs==1&&configs==1&&write_calls==1&&!installed);
   assert(radio.marc==0x0d && packet_irq && pin_is_input && irq_rearms==1);
   assert(std::find(phases.begin(),phases.end(),"CC1101 modem registers verified OK")!=phases.end());
  }
  {esphome::cc1101::CC1101Component radio;RadioModeManager mode;
   CC1101GdoUartTx tx(radio,mode,2,16,preset,true);std::string err;fail_config=true;
   assert(!tx.send(2,preset.name,payload,5,1,0,err));
   assert(err.find("UART1 8N1/baud config: ESP_ERR_INVALID_ARG (0x102)")!=std::string::npos);
   assert(!installed&&radio.marc==0x0d&&!tx.sticky_fault()&&packet_irq&&pin_is_input);fail_config=false;
  }
  {esphome::cc1101::CC1101Component radio;RadioModeManager mode;
   CC1101GdoUartTx tx(radio,mode,2,16,preset,true);std::string err;fail_register=true;
   assert(!tx.send(2,preset.name,payload,5,1,0,err));
   assert(err.find("register 0x08 readback mismatch")!=std::string::npos);
   assert(err.find("expected 0x30, got 0x31")!=std::string::npos);
   assert(!installed&&radio.marc==0x0d&&!tx.sticky_fault()&&packet_irq&&pin_is_input);fail_register=false;
  }
  std::cout<<"CC1101 GDO UART host mock: order/errors/RX rollback PASS\\n";
}
'''
# With gdo_serial_tx_session.h available as real firmware header.
with TemporaryDirectory() as d:
    d=Path(d)
    (d/'mock.cpp').write_text(mock+'\n#include "gdo_serial_tx_session.h"\n#include "serial_tx_adapter.h"\n#include <algorithm>\n'+helper+'\n'+header+'\n'+main)
    cmd=['g++','-std=c++17','-Wall','-Wextra','-Werror','-pedantic','-I',str(root/'components/rflink_universal'),str(d/'mock.cpp'),'-o',str(d/'mock')]
    subprocess.run(cmd,check=True)
    subprocess.run([str(d/'mock')],check=True)
