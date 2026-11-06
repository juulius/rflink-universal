#include "rflink_packet_rx.hpp"
#include "rflink_rf_profiles.hpp"
#include <cstring>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

namespace rflink {
namespace {
constexpr RadioId kRadio = RadioId::rf86830;
constexpr char TAG[] = "rflink.rx868";
constexpr uint8_t SIDLE=0x36, SFRX=0x3a, SRX=0x34;
struct Reg { uint8_t addr; uint8_t value; };
struct SemRelease {
  SemaphoreHandle_t sem;
  ~SemRelease() { if(sem)xSemaphoreGive(sem); }
};
// Diagnostic receiver, not yet validated against real CC1101 hardware.
}  // namespace

esp_err_t PacketRx868::start() {
  if (!lock_) lock_ = xSemaphoreCreateMutex();
  if (!operation_lock_) operation_lock_ = xSemaphoreCreateMutex();
  if (!lock_ || !operation_lock_) return ESP_ERR_NO_MEM;
  xSemaphoreTake((SemaphoreHandle_t)operation_lock_, portMAX_DELAY);
  SemRelease op{(SemaphoreHandle_t)operation_lock_};
  auto st=bus_.state(kRadio);
  if (!st.present) return ESP_ERR_NOT_FOUND;
  esp_err_t e=bus_.strobe(kRadio,SIDLE);
  if (e!=ESP_OK) return e;
  // Carrier from the actual configured RF frequency (no extra adjustment).
  const auto& profile = rf_profile(RfProfileId::itho_868_packet);
  if (!profile_compatible_with_radio(profile, kRadio) ||
      profile.rx_transport != RxTransport::packet_fifo ||
      profile.packet_length != 63) return ESP_ERR_INVALID_ARG;
  const uint32_t fword = cc1101_frequency_word(profile.frequency_hz);
  const Reg freq[]={{0x0d,uint8_t((fword>>16)&0xff)},
                    {0x0e,uint8_t((fword>>8)&0xff)},
                    {0x0f,uint8_t(fword&0xff)}};
  for(const auto& r : freq) if((e=bus_.write_register(kRadio,r.addr,r.value))!=ESP_OK) return e;
  for(size_t j=0;j<profile.register_count;++j){
    const auto& r=profile.modem_registers[j];
    if((e=bus_.write_register(kRadio,r.address,r.value))!=ESP_OK) return e;
    uint8_t actual=0xff;
    if((e=bus_.read_register(kRadio,r.address,&actual))!=ESP_OK) return e;
    // Reserved bit 3 in DEVIATN (0x15) always reads zero on CC1101.
    const uint8_t mask = r.address==0x15 ? 0xF7 : 0xFF;
    if((actual&mask)!=(r.value&mask)){ESP_LOGE(TAG,"register 0x%02x expected 0x%02x got 0x%02x",r.address,r.value,actual);return ESP_ERR_INVALID_RESPONSE;}
  }
  if((e=bus_.strobe(kRadio,SFRX))!=ESP_OK) return e;
  if((e=bus_.strobe(kRadio,SRX))!=ESP_OK) return e;
  xSemaphoreTake((SemaphoreHandle_t)lock_, portMAX_DELAY);
  stats_.configured=true; stats_.listening=true;
  xSemaphoreGive((SemaphoreHandle_t)lock_);
  ESP_LOGI(TAG,"868.30 MHz packet RX diagnostic started (fixed 63 bytes, polling)");
  return ESP_OK;
}

esp_err_t PacketRx868::stop() {
  if (!lock_ || !operation_lock_) return ESP_ERR_INVALID_STATE;
  xSemaphoreTake((SemaphoreHandle_t)operation_lock_, portMAX_DELAY);
  SemRelease op{(SemaphoreHandle_t)operation_lock_};
  auto e=bus_.strobe(kRadio,SIDLE);
  xSemaphoreTake((SemaphoreHandle_t)lock_,portMAX_DELAY);
  stats_.listening=false;
  stats_.configured=false;
  xSemaphoreGive((SemaphoreHandle_t)lock_);
  return e;
}

esp_err_t PacketRx868::recover_fifo() {
  esp_err_t e=bus_.strobe(kRadio,SIDLE);
  if(e==ESP_OK) e=bus_.strobe(kRadio,SFRX);
  if(e==ESP_OK) e=bus_.strobe(kRadio,SRX);
  xSemaphoreTake((SemaphoreHandle_t)lock_,portMAX_DELAY);
  ++stats_.recoveries;
  stats_.listening=(e==ESP_OK);
  xSemaphoreGive((SemaphoreHandle_t)lock_);
  return e;
}

void PacketRx868::poll_once() {
  if (!operation_lock_ ||
      xSemaphoreTake((SemaphoreHandle_t)operation_lock_,0)!=pdTRUE)return;
  SemRelease op{(SemaphoreHandle_t)operation_lock_};
  if (!stats().configured) return;
  uint8_t marc=0xff, rxbytes=0xff;
  esp_err_t e=bus_.read_runtime_status(kRadio,&marc,&rxbytes);
  if(e!=ESP_OK){
    xSemaphoreTake((SemaphoreHandle_t)lock_,portMAX_DELAY);++stats_.spi_errors;xSemaphoreGive((SemaphoreHandle_t)lock_);return;
  }
  xSemaphoreTake((SemaphoreHandle_t)lock_,portMAX_DELAY);
  stats_.last_marcstate=marc;
  xSemaphoreGive((SemaphoreHandle_t)lock_);
  // RX FIFO overflow is signalled by bit7; MARCSTATE 0x11 is RXFIFO_OVERFLOW.
  if((rxbytes&0x80) || (marc&0x1f)==0x11){
    xSemaphoreTake((SemaphoreHandle_t)lock_,portMAX_DELAY);++stats_.fifo_overflows;xSemaphoreGive((SemaphoreHandle_t)lock_);
    ESP_LOGW(TAG,"RX FIFO overflow; rearming RX");recover_fifo();return;
  }
  // 63 bytes available => one fixed-size packet. Keep the whole read atomic.
  if((rxbytes&0x7f)>=63){
    uint8_t packet[63]={};
    e=bus_.read_rx_fifo(kRadio,packet,sizeof(packet));
    if(e!=ESP_OK){xSemaphoreTake((SemaphoreHandle_t)lock_,portMAX_DELAY);++stats_.spi_errors;xSemaphoreGive((SemaphoreHandle_t)lock_);recover_fifo();return;}
    xSemaphoreTake((SemaphoreHandle_t)lock_,portMAX_DELAY);
    memcpy(stats_.last_packet,packet,sizeof(packet));stats_.last_length=sizeof(packet);++stats_.packets;
    xSemaphoreGive((SemaphoreHandle_t)lock_);
    // Deliver immutable packet bytes synchronously on the RX worker, not in an ISR.
    if (sink_) sink_->on_packet(kRadio, rf_profile(RfProfileId::itho_868_packet).frequency_hz, packet, sizeof(packet));
  }
}
PacketRxStats PacketRx868::stats() const {
  PacketRxStats result{};
  if(!lock_) return result;
  xSemaphoreTake((SemaphoreHandle_t)lock_,portMAX_DELAY);
  result=stats_;
  xSemaphoreGive((SemaphoreHandle_t)lock_);
  return result;
}
}
