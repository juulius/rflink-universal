#include "rflink_status_led.hpp"
#include <atomic>
#include <cstdint>
#include "driver/gpio.h"
#include "driver/rmt_encoder.h"
#include "driver/rmt_tx.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"

namespace rflink {
namespace {
constexpr char TAG[]="rflink.led";
constexpr char kNamespace[]="rf_led";
constexpr char kSettingsKey[]="settings";
constexpr uint32_t kResolutionHz=10000000; // 0.1us per RMT tick
std::atomic<bool> started{false},driver_ready{false};
std::atomic<uint8_t> enabled{1},brightness{12};
std::atomic<bool> wifi_connected{false},startup_complete{false},ota_active{false};
std::atomic<uint8_t> detected_radios{0},active_receivers{0};
std::atomic<uint32_t> error_until_ms{0};
std::atomic<uint8_t> mode{static_cast<uint8_t>(LedMode::boot)};
std::atomic<uint32_t> frames_sent{0},errors{0};
std::atomic<int32_t> last_error{0};
uint32_t millis_now(){return static_cast<uint32_t>(esp_timer_get_time()/1000);}
void load_settings() {
  LedSettings stored{};
  nvs_handle_t handle{};
  if(nvs_open(kNamespace,NVS_READONLY,&handle)!=ESP_OK)return;
  size_t size=sizeof(stored);
  const auto err=nvs_get_blob(handle,kSettingsKey,&stored,&size);
  nvs_close(handle);
  if(err==ESP_OK && size==sizeof(stored) && led_settings_valid(stored)){
    enabled.store(stored.enabled,std::memory_order_release);
    brightness.store(stored.brightness,std::memory_order_release);
  }
}
LedInputs inputs_at(uint32_t now){
  const auto until=error_until_ms.load(std::memory_order_relaxed);
  const bool recent_error=until!=0 && static_cast<int32_t>(until-now)>0;
  LedInputs in{};
  in.startup_complete=startup_complete.load(std::memory_order_relaxed);
  in.wifi_connected=wifi_connected.load(std::memory_order_relaxed);
  in.detected_radios=detected_radios.load(std::memory_order_relaxed);
  in.active_receivers=active_receivers.load(std::memory_order_relaxed);
  in.ota_active=ota_active.load(std::memory_order_relaxed);
  in.error=recent_error;
  return in;
}
struct LedHardware {
  rmt_channel_handle_t channel=nullptr;
  rmt_encoder_handle_t encoder=nullptr;
  bool active=false;
  bool initialise() {
    // The RF RX channels have already been allocated, by design.
    // A missing TX channel must NEVER disable/deinit either radio receiver.
    rmt_tx_channel_config_t config{};
    config.gpio_num=static_cast<gpio_num_t>(kStatusLedGpio);
    config.clk_src=RMT_CLK_SRC_DEFAULT;
    config.resolution_hz=kResolutionHz;
    config.mem_block_symbols=48;
    config.trans_queue_depth=1;
    config.flags.with_dma=false;
    esp_err_t e=rmt_new_tx_channel(&config,&channel);
    if(e==ESP_OK){
      rmt_copy_encoder_config_t encoder_config{};
      e=rmt_new_copy_encoder(&encoder_config,&encoder);
    }
    if(e==ESP_OK)e=rmt_enable(channel);
    if(e!=ESP_OK){
      errors.fetch_add(1,std::memory_order_relaxed);
      last_error.store(e,std::memory_order_release);
      if(encoder){(void)rmt_del_encoder(encoder);encoder=nullptr;}
      if(channel){(void)rmt_del_channel(channel);channel=nullptr;}
      ESP_LOGW(TAG,"WS2812 disabled: RMT TX unavailable (%s); radio RX unaffected",
               esp_err_to_name(e));
      return false;
    }
    active=true;
    ESP_LOGI(TAG,"Onboard WS2812 ready: GPIO%d, dedicated RMT TX, dim by default",
             kStatusLedGpio);
    return true;
  }
  bool send(LedRgb rgb) {
    if(!active)return false;
    // WS2812: 24-bit GRB, MSB first. 0: 0.4/0.9 us, 1: 0.8/0.5 us.
    // Data output low at end; 100-ms LED task period supplies reset >50us.
    const uint32_t grb=(uint32_t(rgb.green)<<16) |
                       (uint32_t(rgb.red)<<8) | rgb.blue;
    rmt_symbol_word_t frame[24]{};
    for(unsigned bit=0;bit<24;++bit) {
      const bool one=(grb>>(23-bit))&1u;
      frame[bit].duration0=one?8:4;
      frame[bit].level0=1;
      frame[bit].duration1=one?5:9;
      frame[bit].level1=0;
    }
    rmt_transmit_config_t tx{};
    tx.loop_count=0;
    tx.flags.eot_level=0;
    auto e=rmt_transmit(channel,encoder,frame,sizeof(frame),&tx);
    if(e==ESP_OK)e=rmt_tx_wait_all_done(channel,100);
    if(e!=ESP_OK){
      errors.fetch_add(1,std::memory_order_relaxed);
      last_error.store(e,std::memory_order_release);
      // Avoid high-frequency logging or repeated radio/resource allocation.
      if(errors.load(std::memory_order_relaxed)%20==1)
        ESP_LOGW(TAG,"WS2812 update failed: %s",esp_err_to_name(e));
      return false;
    }
    frames_sent.fetch_add(1,std::memory_order_relaxed);
    last_error.store(0,std::memory_order_release);
    return true;
  }
};
void led_task(void*) {
  LedHardware led{};
  bool attempted=false;
  bool last_valid=false;
  LedRgb last{};
  while(true){
    const auto now=millis_now();
    const LedSettings settings{kLedSettingsMagic,1,
      enabled.load(std::memory_order_relaxed),
      brightness.load(std::memory_order_relaxed),0};
    const auto state=led_mode(settings,inputs_at(now));
    mode.store(static_cast<uint8_t>(state),std::memory_order_release);
    if(settings.enabled && !attempted){
      attempted=true;
      driver_ready.store(led.initialise(),std::memory_order_release);
    }
    const auto pixel=led_sample(state,now,settings.brightness);
    if(driver_ready.load(std::memory_order_acquire) &&
       (!last_valid || pixel.red!=last.red || pixel.green!=last.green ||
        pixel.blue!=last.blue)){
      if(led.send(pixel)){last=pixel;last_valid=true;}
    }
    // RMT and GPIO operations live on this low-priority task, not the radio
    // poll loop, Wi-Fi callback, OTA HTTP task or a high-priority ISR.
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
} // namespace

bool status_led_start(){
  if(started.exchange(true))return true;
  load_settings();
  const auto success=xTaskCreate(led_task,"rf_led",4096,nullptr,2,nullptr)==pdPASS;
  if(!success){
    started=false;
    errors.fetch_add(1,std::memory_order_relaxed);
    ESP_LOGW(TAG,"LED task could not start; radios remain unaffected");
  }
  return success;
}
void status_led_set_wifi_connected(bool connected){wifi_connected.store(connected);}
void status_led_set_radios(unsigned detected,unsigned active){
  detected_radios.store(static_cast<uint8_t>(detected>3?3:detected));
  active_receivers.store(static_cast<uint8_t>(active>3?3:active));
}
void status_led_set_startup_complete(bool complete){startup_complete.store(complete);}
void status_led_set_ota_active(bool active){ota_active.store(active);}
void status_led_signal_error(){error_until_ms.store(millis_now()+10000u);}
LedSnapshot status_led_snapshot(){
  LedSnapshot result{};
  result.settings.enabled=enabled.load(std::memory_order_relaxed);
  result.settings.brightness=brightness.load(std::memory_order_relaxed);
  result.mode=static_cast<LedMode>(mode.load(std::memory_order_relaxed));
  result.driver_ready=driver_ready.load(std::memory_order_acquire);
  result.frames_sent=frames_sent.load(std::memory_order_relaxed);
  result.errors=errors.load(std::memory_order_relaxed);
  result.last_error=last_error.load(std::memory_order_relaxed);
  return result;
}
bool status_led_save_settings(const LedSettings& settings){
  if(!led_settings_valid(settings))return false;
  nvs_handle_t handle{};
  if(nvs_open(kNamespace,NVS_READWRITE,&handle)!=ESP_OK)return false;
  esp_err_t e=nvs_set_blob(handle,kSettingsKey,&settings,sizeof(settings));
  if(e==ESP_OK)e=nvs_commit(handle);
  nvs_close(handle);
  if(e!=ESP_OK){
    ESP_LOGW(TAG,"LED setting save failed: %s",esp_err_to_name(e));
    return false;
  }
  brightness.store(settings.brightness,std::memory_order_release);
  enabled.store(settings.enabled,std::memory_order_release);
  ESP_LOGI(TAG,"LED settings saved: %s brightness=%u%%",
           settings.enabled?"enabled":"disabled",unsigned(settings.brightness));
  return true;
}
} // namespace rflink
