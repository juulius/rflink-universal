// Executed host simulation of CS ownership, SO/CHIP_RDYn guard and error recovery.
// This is NOT a physical CC1101 test or full ESP-IDF build.
#include "rflink_cc1101_spi.hpp"
#include "rflink_packet_rx.hpp"
#include "driver/gpio.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <mutex>
#include <thread>
#include <vector>
#include <cstdint>
#include <algorithm>

namespace {
std::array<int,49> pins{};
std::array<int,3> cs{{4,9,15}};
std::array<int,3> devices{{0,1,2}};
int current_device=-1;
unsigned spi_calls=0;
unsigned clock_before_ready=0;
unsigned ready_checks=0;
unsigned simulated_ready_delay=0;
bool force_ready_high=false;
bool fail_next_spi=false;
std::array<bool,3> simulated_absent{{false,false,false}};
std::array<std::array<uint8_t,0x30>,3> registers{};
std::array<uint8_t,3> radio_marc{{0x0d,0x0d,0x0d}};
std::array<uint8_t,3> radio_rxbytes{{0,0,0}};
struct Mutex {std::mutex m;};
int active_cs(){
  int active=-1;
  for(unsigned i=0;i<cs.size();i++) if(pins[cs[i]]==0){assert(active==-1);active=i;}
  return active;
}
}
extern "C" void* xSemaphoreCreateMutex(){return new Mutex();}
extern "C" int xSemaphoreTake(void* sem,unsigned){static_cast<Mutex*>(sem)->m.lock();return pdTRUE;}
extern "C" int xSemaphoreGive(void* sem){static_cast<Mutex*>(sem)->m.unlock();return pdTRUE;}
extern "C" void vSemaphoreDelete(void* sem){delete static_cast<Mutex*>(sem);}
extern "C" void esp_rom_delay_us(unsigned){}
extern "C" const char* esp_err_to_name(esp_err_t){return "mock";}
extern "C" esp_err_t gpio_reset_pin(gpio_num_t gpio){assert(gpio>=0 && gpio<49);pins[gpio]=1;return ESP_OK;}
extern "C" esp_err_t gpio_set_level(gpio_num_t gpio,int v){assert(gpio>=0 && gpio<49);pins[gpio]=v;return ESP_OK;}
extern "C" esp_err_t gpio_set_direction(gpio_num_t gpio,int mode){assert(gpio>=0&&gpio<49&&mode==GPIO_MODE_OUTPUT);return ESP_OK;}
extern "C" int gpio_get_level(gpio_num_t gpio){
  assert(gpio==13);
  assert(active_cs()>=0);
  ready_checks++;
  if(force_ready_high)return 1;
  if(simulated_ready_delay){--simulated_ready_delay;return 1;}
  return 0;
}
extern "C" esp_err_t spi_bus_initialize(int host,const spi_bus_config_t* cfg,int dma){
  assert(host==SPI2_HOST&&dma==SPI_DMA_DISABLED);
  assert(cfg->miso_io_num==13 && cfg->mosi_io_num==11 && cfg->sclk_io_num==12);
  assert(active_cs()<0);
  return ESP_OK;
}
extern "C" esp_err_t spi_bus_add_device(int host,const spi_device_interface_config_t* cfg,spi_device_handle_t* handle){
  assert(host==SPI2_HOST);
  assert(cfg->spics_io_num==-1); // hardware-CS would prevent SO wait
  assert(current_device<2);
  *handle=&devices[++current_device];
  return ESP_OK;
}
extern "C" esp_err_t spi_bus_remove_device(spi_device_handle_t){return ESP_OK;}
extern "C" esp_err_t spi_bus_free(int){return ESP_OK;}
extern "C" esp_err_t spi_device_polling_transmit(spi_device_handle_t handle,spi_transaction_t* t){
  const int id=*static_cast<int*>(handle);
  assert(active_cs()==id);
  assert(ready_checks>0);
  assert(t->length%8==0 && t->length<=512);
  ++spi_calls;
  if(force_ready_high){++clock_before_ready;assert(false && "SPI must not clock before CHIP_RDYn");}
  if(fail_next_spi){fail_next_spi=false;return ESP_ERR_INVALID_RESPONSE;}
  const auto* tx=static_cast<const uint8_t*>(t->tx_buffer);
  auto* rx=static_cast<uint8_t*>(t->rx_buffer);
  if(rx){
    for(size_t i=0;i<t->length/8;i++)rx[i]=0;
    if(t->length>=16 && (tx[0]==0xF0 || tx[0]==0xF1))
      rx[1]=simulated_absent[id]?0x00:(tx[0]==0xF0?0x00:0x14);
    else if(tx[0]==0xF5 && t->length==16)rx[1]=radio_marc[id];
    else if(tx[0]==0xFB && t->length==16)rx[1]=radio_rxbytes[id];
    else if(tx[0]==0xFF){for(size_t i=1;i<t->length/8;i++)rx[i]=uint8_t(i);}
    else if((tx[0]&0xc0)==0x80 && (tx[0]&0x3f)<0x30 && t->length==16)
      rx[1]=registers[id][tx[0]&0x3f];
  }
  if(t->length==16 && tx[0]<0x30)registers[id][tx[0]]=tx[1];
  return ESP_OK;
}
int main(){
  for(auto cs_pin:cs)pins[cs_pin]=1;
  rflink::Cc1101SpiBus b;
  assert(b.begin()==ESP_OK);
  for(auto cs_pin:cs)assert(pins[cs_pin]==1);
  uint8_t part=0xff,version=0xff;
  simulated_ready_delay=3;
  auto old_calls=spi_calls;
  assert(b.read_identity(rflink::RadioId::rf43342,&part,&version)==ESP_OK);
  assert(part==0x00 && version==0x14);
  assert(spi_calls==old_calls+2 && ready_checks>=5);
  assert(active_cs()==-1);
  // Absent or unready radio must never receive an SPI clock; other radios
  // remain accessible afterwards without rebooting the shared bus.
  force_ready_high=true;old_calls=spi_calls;
  assert(b.read_identity(rflink::RadioId::rf43342,&part,&version)==ESP_ERR_TIMEOUT);
  assert(spi_calls==old_calls && active_cs()==-1);
  assert(b.health(rflink::RadioId::rf43342).ready_timeouts==1);
  force_ready_high=false;
  assert(b.read_identity(rflink::RadioId::rf43392,&part,&version)==ESP_OK);
  assert(active_cs()==-1);
  // Failure to clock must not leave CS asserted.
  fail_next_spi=true;
  assert(b.write_register(rflink::RadioId::rf86830,0x10,0x9a)==ESP_ERR_INVALID_RESPONSE);
  assert(active_cs()==-1);
  assert(b.health(rflink::RadioId::rf86830).transfer_errors==1);
  // FIFO read shares guard and one atomic CS window; no duplicated SPI calls.
  uint8_t rx[63]{};
  old_calls=spi_calls;
  assert(b.read_rx_fifo(rflink::RadioId::rf86830,rx,sizeof(rx))==ESP_OK);
  assert(spi_calls==old_calls+1 && rx[0]==1 && rx[62]==63);
  // A boot reset must work after the initial CS pulse and normal CHIP_RDYn check.
  const auto boot=b.boot_reset_and_probe(rflink::RadioId::rf43342);
  assert(boot.reset_issued && boot.plausible && boot.partnum==0 && boot.version==0x14);
  assert(active_cs()==-1);
  // When the PCB's weak MISO pull-down makes SO appear ready even without a
  // transceiver, PARTNUM/VERSION must still reject the absent radio.
  simulated_absent[1]=true;
  assert(!b.probe(rflink::RadioId::rf43392).plausible);
  assert(!b.state(rflink::RadioId::rf43392).present);
  simulated_absent[1]=false;
  assert(b.probe(rflink::RadioId::rf43392).plausible);
  // The 868 RX worker must detect that a modem silently fell back to IDLE,
  // re-arm only after two successful samples, and stop claiming "listening"
  // after repeated SPI failures instead of silently returning forever.
  rflink::PacketRx868 packet_rx(b);
  assert(packet_rx.start()==ESP_OK);
  assert(packet_rx.stats().listening);
  radio_marc[2]=0x01;
  packet_rx.poll_once();
  assert(packet_rx.stats().idle_rearms==0);
  packet_rx.poll_once();
  auto rx_stats=packet_rx.stats();
  assert(rx_stats.idle_rearms==1);
  assert(rx_stats.recoveries==1 && rx_stats.listening);
  radio_marc[2]=0x0d;
  packet_rx.poll_once();
  assert(packet_rx.stats().idle_rearms==1);
  for(int i=0;i<3;++i){
    fail_next_spi=true;
    packet_rx.poll_once();
  }
  rx_stats=packet_rx.stats();
  assert(rx_stats.spi_errors==3 && rx_stats.consecutive_spi_errors==3);
  assert(!rx_stats.listening);
  const auto calls_after_fault=spi_calls;
  packet_rx.poll_once(); // fail closed until the central manager restarts RX
  assert(spi_calls==calls_after_fault);
  assert(packet_rx.stop()==ESP_OK);
  assert(packet_rx.start()==ESP_OK);
  rx_stats=packet_rx.stats();
  assert(rx_stats.listening && rx_stats.consecutive_spi_errors==0);
  assert(packet_rx.stop()==ESP_OK);
  // Cross-radio concurrent requests cannot overlap CS windows.
  std::vector<std::thread> threads;
  for(int i=0;i<3;i++)threads.emplace_back([&,i]{
    for(int j=0;j<40;j++){
      uint8_t value=0;
      assert(b.read_register(static_cast<rflink::RadioId>(i),0x10,&value)==ESP_OK);
    }
  });
  for(auto &t:threads)t.join();
  assert(active_cs()==-1 && clock_before_ready==0);
  puts("PASS: CHIP_RDYn, CS release, absent-module rejection, FIFO atomicity, 868 lost-RX recovery, SPI-fault lockdown and 3-radio concurrency");
}
