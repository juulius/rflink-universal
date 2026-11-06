#include "rflink_ook_rx.hpp"
#include "rflink_rf_profiles.hpp"
#include "esp_log.h"
namespace rflink {
namespace {
constexpr uint8_t kSidle=0x36,kSfrx=0x3a,kSrx=0x34;
constexpr char TAG[]="rflink.ook";
struct SemRelease { SemaphoreHandle_t s; ~SemRelease(){if(s)xSemaphoreGive(s);} };
}
esp_err_t OokPulseReceiver::start(const RfProfile& p) {
  if(!operation_lock_)operation_lock_=xSemaphoreCreateMutex();
  if(!operation_lock_)return ESP_ERR_NO_MEM;
  xSemaphoreTake(operation_lock_,portMAX_DELAY);
  SemRelease guard{operation_lock_};
  if ((radio_!=RadioId::rf43342 && radio_!=RadioId::rf43392) ||
      p.rx_transport!=RxTransport::gdo_rmt ||
      !profile_compatible_with_radio(p,radio_) || !profile_has_registers(p))
    return ESP_ERR_INVALID_ARG;
  if (!bus_.state(radio_).present) return ESP_ERR_NOT_FOUND;
  esp_err_t e=bus_.strobe(radio_,kSidle);
  if(e!=ESP_OK)return e;
  const auto stop_on_failure=[this](esp_err_t failure){
    (void)bus_.strobe(radio_,kSidle);
    return failure;
  };
  uint32_t freq=cc1101_frequency_word(p.frequency_hz);
  const RegisterSetting f[]={{0x0d,uint8_t(freq>>16)},{0x0e,uint8_t(freq>>8)},{0x0f,uint8_t(freq)}};
  for(const auto& r:f){ e=bus_.write_register(radio_,r.address,r.value);if(e!=ESP_OK)return stop_on_failure(e); }
  for(size_t i=0;i<p.register_count;i++){
    auto r=p.modem_registers[i];
    e=bus_.write_register(radio_,r.address,r.value);
    if(e!=ESP_OK)return stop_on_failure(e);
    uint8_t got=0;
    e=bus_.read_register(radio_,r.address,&got);
    if(e!=ESP_OK)return stop_on_failure(e);
    const uint8_t mask=r.address==0x15?0xf7:0xff;
    if((got&mask)!=(r.value&mask)){
      ESP_LOGE(TAG,"radio%u register 0x%02x expected 0x%02x got 0x%02x",
        unsigned(radio_),r.address,r.value,got);
      return stop_on_failure(ESP_ERR_INVALID_RESPONSE);
    }
  }
  e=bus_.strobe(radio_,kSfrx);
  if(e!=ESP_OK)return stop_on_failure(e);
  e=capture_.start(radio_);
  if(e!=ESP_OK)return stop_on_failure(e);
  e=bus_.strobe(radio_,kSrx);
  if(e!=ESP_OK){(void)capture_.stop();return stop_on_failure(e);}
  ESP_LOGW(TAG,"433.%02u OOK RX started: experimental hardware profile (not decoded)",radio_==RadioId::rf43342?42:92);
  return ESP_OK;
}
esp_err_t OokPulseReceiver::stop(){
  if(!operation_lock_)return ESP_ERR_INVALID_STATE;
  xSemaphoreTake(operation_lock_,portMAX_DELAY);
  SemRelease guard{operation_lock_};
  auto capture_error=capture_.stop();
  auto radio_error=bus_.strobe(radio_,kSidle);
  return capture_error!=ESP_OK?capture_error:radio_error;
}
void OokPulseReceiver::poll_once(){
  if(!operation_lock_ || xSemaphoreTake(operation_lock_,0)!=pdTRUE)return;
  SemRelease guard{operation_lock_};
  capture_.poll_once();
}
} // namespace rflink
