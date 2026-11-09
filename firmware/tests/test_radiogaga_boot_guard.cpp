// Executed host simulation of the v0.6.6 pre-SPI electrical guard.
// No ESPHome runtime, no radio TX or an implied physical hardware test.
#include "rflink_native_boot_guard.hpp"
#include "rflink_radio_hal.hpp"
#include "driver/gpio.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>

namespace {
std::array<int,49> mode{},pull{},value{};
std::array<bool,3> ready{{true,false,true}};
uint64_t elapsed_us=0;
int forced_gpio_failure=-1;
uint32_t miso_reads=0,cs_assertions=0,unsafe_outputs=0;
bool first_post_settle_gpio=false;
int current_cs(){
  int current=-1;
  for(unsigned i=0;i<rflink::kRadioPins.size();i++){
    if(value[rflink::kRadioPins[i].cs]==0 && mode[rflink::kRadioPins[i].cs]==GPIO_MODE_OUTPUT){
      assert(current==-1);current=int(i);
    }
  }
  return current;
}
}
extern "C" void esp_rom_delay_us(unsigned us){elapsed_us+=us;}
extern "C" esp_err_t gpio_set_pull_mode(gpio_num_t pin,int p){
  assert(pin>=0 && pin<49);
  if(pin==forced_gpio_failure)return ESP_ERR_INVALID_STATE;
  pull[pin]=p;return ESP_OK;
}
extern "C" esp_err_t gpio_set_direction(gpio_num_t pin,int m){
  assert(pin>=0 && pin<49);
  if(pin==forced_gpio_failure)return ESP_ERR_INVALID_STATE;
  if(m==GPIO_MODE_OUTPUT){
    if(elapsed_us<200000)unsafe_outputs++;
    first_post_settle_gpio=true;
  }
  mode[pin]=m;return ESP_OK;
}
extern "C" esp_err_t gpio_set_level(gpio_num_t pin,int v){
  assert(pin>=0 && pin<49);
  if(pin==forced_gpio_failure)return ESP_ERR_INVALID_STATE;
  if(pin==4||pin==9||pin==15){
    if(v==0){
      assert(elapsed_us>=200000);
      cs_assertions++;
    }
  }
  value[pin]=v;return ESP_OK;
}
extern "C" int gpio_get_level(gpio_num_t pin){
  assert(pin==rflink::kSpiPins.miso);
  miso_reads++;
  const int selected=current_cs();
  return selected>=0 && ready[size_t(selected)] ? 0 : 1;
}
int main(){
  for(const auto& radio:rflink::kRadioPins)value[radio.cs]=1;
  const auto result=rflink::prepare_radio_pins_before_spi();
  assert(result.gpio_setup_ok());
  assert(result.chip_ready[0] && !result.chip_ready[1] && result.chip_ready[2]);
  assert(result.miso_idle_high_before && result.miso_idle_high_after);
  assert(first_post_settle_gpio && unsafe_outputs==0);
  assert(cs_assertions>=7 && miso_reads>5);
  assert(elapsed_us>=200000+25000+5*10000);
  for(const auto& radio:rflink::kRadioPins){
    assert(value[radio.cs]==1);
    assert(mode[radio.cs]==GPIO_MODE_OUTPUT);
    assert(pull[radio.cs]==GPIO_PULLUP_ONLY);
    assert(mode[radio.gdo0]==GPIO_MODE_INPUT);
    assert(mode[radio.gdo2]==GPIO_MODE_INPUT);
    assert(pull[radio.gdo0]==GPIO_FLOATING);
    assert(pull[radio.gdo2]==GPIO_FLOATING);
  }
  assert(mode[rflink::kSpiPins.sclk]==GPIO_MODE_OUTPUT);
  assert(mode[rflink::kSpiPins.mosi]==GPIO_MODE_OUTPUT);
  assert(value[rflink::kSpiPins.sclk]==0 && value[rflink::kSpiPins.mosi]==0);
  assert(pull[rflink::kSpiPins.sclk]==GPIO_PULLDOWN_ONLY);
  assert(pull[rflink::kSpiPins.mosi]==GPIO_PULLDOWN_ONLY);
  assert(mode[rflink::kSpiPins.miso]==GPIO_MODE_INPUT);
  assert(pull[rflink::kSpiPins.miso]==GPIO_PULLUP_ONLY);

  // Fail-closed on *real GPIO configuration errors*, but NOT on a radio that
  // fails a non-destructive CHIP_RDYn probe (same behavior as v0.6.6).
  elapsed_us=0;
  forced_gpio_failure=rflink::kRadioPins[0].gdo2;
  const auto failed=rflink::prepare_radio_pins_before_spi();
  assert(!failed.gpio_setup_ok());
  assert(elapsed_us==0); // no SPI phase and no unsafe output direction
  puts("PASS native Radio Gaga GPIO/CHIP_RDYn boot guard: 200ms high-Z, 25ms gap, missing radio optional");
}
