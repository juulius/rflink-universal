#include "rflink_ook_rx.hpp"
#include "rflink_rf_profiles.hpp"
#include "esp_log.h"
namespace rflink {
namespace {
constexpr uint8_t kSidle=0x36;
constexpr char TAG[]="rflink.ook";
struct SemRelease { SemaphoreHandle_t s; ~SemRelease(){if(s)xSemaphoreGive(s);} };
}
esp_err_t OokPulseReceiver::start(const RfProfile& p) {
  {
    std::lock_guard<std::mutex> lock(diag_mutex_);
    ++diagnostics_.attempts;
    diagnostics_.listening=false;
  }
  // This captures the FIRST failing step of the v0.6.6 radio startup path:
  // a generic ManagerError 7 is not proof that RMT allocation failed.
  const auto fail=[this](OokStartStage stage,esp_err_t error,
                         int addr=-1,int expected=-1,int observed=-1){
    {
      std::lock_guard<std::mutex> lock(diag_mutex_);
      ++diagnostics_.failures;
      diagnostics_.stage=stage;
      diagnostics_.esp_error=static_cast<int32_t>(error);
      diagnostics_.register_address=addr;
      diagnostics_.expected=expected;
      diagnostics_.observed=observed;
      diagnostics_.listening=false;
    }
    // The host syntax stubs intentionally compile away log macro arguments.
    (void)stage;(void)addr;(void)expected;(void)observed;
    ESP_LOGE(TAG,"433 radio%u RX start FAILED: step=%u esp_err=%d (%s) register=%d wanted=%d got=%d",
      unsigned(radio_),unsigned(stage),int(error),esp_err_to_name(error),
      addr,expected,observed);
    return error;
  };
  if(!operation_lock_)operation_lock_=xSemaphoreCreateMutex();
  if(!operation_lock_)return fail(OokStartStage::lock,ESP_ERR_NO_MEM);
  xSemaphoreTake(operation_lock_,portMAX_DELAY);
  SemRelease guard{operation_lock_};
  if ((radio_!=RadioId::rf43342 && radio_!=RadioId::rf43392) ||
      p.rx_transport!=RxTransport::gdo_rmt ||
      !profile_compatible_with_radio(p,radio_) || !profile_has_registers(p))
    return fail(OokStartStage::profile,ESP_ERR_INVALID_ARG);
  if (!bus_.state(radio_).present)
    return fail(OokStartStage::hardware,ESP_ERR_NOT_FOUND);
  esp_err_t e=bus_.strobe(radio_,kSidle);
  if(e!=ESP_OK)return fail(OokStartStage::idle,e);
  const auto stop_on_failure=[this](esp_err_t error){
    (void)bus_.strobe(radio_,kSidle);
    return error;
  };
  const uint32_t freq=cc1101_frequency_word(p.frequency_hz);
  const RegisterSetting frequencies[]={
    {0x0d,uint8_t(freq>>16)},{0x0e,uint8_t(freq>>8)},{0x0f,uint8_t(freq)}
  };
  for(const auto& reg:frequencies){
    e=bus_.write_register(radio_,reg.address,reg.value);
    if(e!=ESP_OK)return stop_on_failure(
      fail(OokStartStage::frequency,e,reg.address,reg.value));
  }
  for(size_t i=0;i<p.register_count;i++){
    const auto reg=p.modem_registers[i];
    e=bus_.write_register(radio_,reg.address,reg.value);
    if(e!=ESP_OK)return stop_on_failure(
      fail(OokStartStage::modem_write,e,reg.address,reg.value));
    uint8_t got=0;
    e=bus_.read_register(radio_,reg.address,&got);
    if(e!=ESP_OK)return stop_on_failure(
      fail(OokStartStage::modem_read,e,reg.address,reg.value));
    const uint8_t mask=reg.address==0x15?0xf7:0xff;
    if((got&mask)!=(reg.value&mask)){
      return stop_on_failure(fail(OokStartStage::modem_verify,
        ESP_ERR_INVALID_RESPONSE,reg.address,reg.value,got));
    }
  }
  e=capture_.start(radio_);
  if(e!=ESP_OK)return stop_on_failure(fail(OokStartStage::rmt,e));
  e=bus_.enter_rx_verified(radio_);
  if(e!=ESP_OK){
    // Do not leave a live RMT channel when the CC1101 could not enter RX.
    (void)capture_.stop();
    return stop_on_failure(fail(OokStartStage::enter_rx,e));
  }
  {
    std::lock_guard<std::mutex> lock(diag_mutex_);
    diagnostics_.stage=OokStartStage::none;
    diagnostics_.esp_error=0;
    diagnostics_.register_address=-1;
    diagnostics_.expected=-1;
    diagnostics_.observed=-1;
    diagnostics_.listening=true;
  }
  ESP_LOGW(TAG,"433.%02u OOK RX started: experimental hardware profile (not decoded)",
     radio_==RadioId::rf43342?42:92);
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
