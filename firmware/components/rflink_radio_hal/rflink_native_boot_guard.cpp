#include "rflink_native_boot_guard.hpp"
#include "rflink_radio_hal.hpp"
#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "esp_log.h"
#include <initializer_list>

namespace rflink {
namespace {
constexpr char TAG[]="rflink.bootguard";
constexpr unsigned kRailSettleUs=200000;   // v0.6.6 startup_delay = 200 ms
constexpr unsigned kPostGuardUs=25000;     // v0.6.6 post_guard_delay = 25 ms
constexpr unsigned kChipReadyUs=10000;     // v0.6.6 ready_timeout = 10 ms
constexpr unsigned kReadyRetries=5;       // v0.6.6 ready_retries = 5
constexpr unsigned kReadyPollUs=20;

esp_err_t set_input_floating(int pin){
  const auto gpio=static_cast<gpio_num_t>(pin);
  esp_err_t e=gpio_set_direction(gpio,GPIO_MODE_INPUT);
  if(e==ESP_OK)e=gpio_set_pull_mode(gpio,GPIO_FLOATING);
  return e;
}
esp_err_t set_output_idle(int pin,int level,gpio_pull_mode_t pull){
  const auto gpio=static_cast<gpio_num_t>(pin);
  esp_err_t e=gpio_set_level(gpio,level);
  if(e==ESP_OK)e=gpio_set_pull_mode(gpio,pull);
  if(e==ESP_OK)e=gpio_set_direction(gpio,GPIO_MODE_OUTPUT);
  if(e==ESP_OK)e=gpio_set_level(gpio,level);
  return e;
}
bool idle_miso_high(){
  esp_rom_delay_us(10);
  return gpio_get_level(static_cast<gpio_num_t>(kSpiPins.miso))!=0;
}
void deselect_all(){
  for(const auto& radio:kRadioPins)
    (void)gpio_set_level(static_cast<gpio_num_t>(radio.cs),1);
}
bool chip_ready(int cs){
  const auto gpio=static_cast<gpio_num_t>(cs);
  for(unsigned attempt=0;attempt<kReadyRetries;attempt++){
    deselect_all();
    (void)gpio_set_level(static_cast<gpio_num_t>(kSpiPins.sclk),0);
    (void)gpio_set_level(static_cast<gpio_num_t>(kSpiPins.mosi),0);
    esp_rom_delay_us(10);
    (void)gpio_set_level(gpio,0);
    bool ready=false;
    for(unsigned elapsed=0;elapsed<kChipReadyUs;elapsed+=kReadyPollUs){
      if(gpio_get_level(static_cast<gpio_num_t>(kSpiPins.miso))==0){
        ready=true;break;
      }
      esp_rom_delay_us(kReadyPollUs);
    }
    (void)gpio_set_level(gpio,1);
    esp_rom_delay_us(10);
    if(ready)return true;
    esp_rom_delay_us(5000);
  }
  return false;
}
} // namespace

NativeBootGuardReport prepare_radio_pins_before_spi(){
  NativeBootGuardReport report{};
  // First, every radio-facing MCU GPIO goes high impedance. The PCB's
  // external CSn pull-ups must be to the RADIO rail, not MCU power.
  // Do not drive an unpowered CC1101 through GPIO before rail settling.
  for(const auto& pins:kRadioPins){
    for(const int pin:{pins.cs,pins.gdo0,pins.gdo2}){
      const esp_err_t e=set_input_floating(pin);
      if(e!=ESP_OK){report.gpio_error=e;return report;}
    }
  }
  for(const int pin:{kSpiPins.sclk,kSpiPins.mosi,kSpiPins.miso}){
    const esp_err_t e=set_input_floating(pin);
    if(e!=ESP_OK){report.gpio_error=e;return report;}
  }
  esp_rom_delay_us(kRailSettleUs);
  // Match RadioBootGuard::configure_safe_bus_idle_() from 0.6.6.
  for(const auto& pins:kRadioPins){
    const esp_err_t e=set_output_idle(pins.cs,1,GPIO_PULLUP_ONLY);
    if(e!=ESP_OK){report.gpio_error=e;return report;}
  }
  for(const int pin:{kSpiPins.sclk,kSpiPins.mosi}){
    const esp_err_t e=set_output_idle(pin,0,GPIO_PULLDOWN_ONLY);
    if(e!=ESP_OK){report.gpio_error=e;return report;}
  }
  report.gpio_error=gpio_set_direction(
    static_cast<gpio_num_t>(kSpiPins.miso),GPIO_MODE_INPUT);
  if(report.gpio_error!=ESP_OK)return report;
  report.gpio_error=gpio_set_pull_mode(
    static_cast<gpio_num_t>(kSpiPins.miso),GPIO_PULLUP_ONLY);
  if(report.gpio_error!=ESP_OK)return report;
  deselect_all();
  report.miso_idle_high_before=idle_miso_high();
  for(unsigned i=0;i<kRadioPins.size();i++)
    report.chip_ready[i]=chip_ready(kRadioPins[i].cs);
  deselect_all();
  (void)gpio_set_level(static_cast<gpio_num_t>(kSpiPins.sclk),0);
  (void)gpio_set_level(static_cast<gpio_num_t>(kSpiPins.mosi),0);
  report.miso_idle_high_after=idle_miso_high();
  esp_rom_delay_us(kPostGuardUs);
  // Readiness is advisory (as in Radio Gaga). Actual CC1101 PARTNUM/VERSION
  // probes happen later via the sole shared native SPI owner.
  ESP_LOGI(TAG,"v0.6.6 pre-SPI guard: CHIP_RDYn radios=%u/%u/%u, MISO idle before=%u after=%u",
    unsigned(report.chip_ready[0]),unsigned(report.chip_ready[1]),
    unsigned(report.chip_ready[2]),unsigned(report.miso_idle_high_before),
    unsigned(report.miso_idle_high_after));
  return report;
}
} // namespace rflink
