#include "rflink_sync_868_bench_rx.hpp"
#include "driver/gpio.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "rflink_rf_profiles.hpp"

namespace rflink {
namespace {
constexpr RadioId kRadio=RadioId::rf86830;
constexpr uint8_t kSidle=0x36;
constexpr char TAG[]="rflink.sync868";
struct SemaphoreRelease {
  SemaphoreHandle_t sem;
  ~SemaphoreRelease(){if(sem)xSemaphoreGive(sem);}
};
}

Cc1101Sync868BenchRx::~Cc1101Sync868BenchRx() {
  if(guard_) {
    (void)stop(kRadio);
    vSemaphoreDelete(guard_);
    guard_=nullptr;
  }
  if(evidence_memory_)heap_caps_free(evidence_memory_);
  evidence_memory_=nullptr;
}

bool Cc1101Sync868BenchRx::make_mutex() {
  if(!guard_)guard_=xSemaphoreCreateMutex();
  return guard_!=nullptr;
}

esp_err_t Cc1101Sync868BenchRx::install_profile(const RfProfile& p) {
  // RadioManager exclusively owns radio 2; its prior receiver is stopped.
  // Turn GDO0/GDO2 into high-impedance ESP32 inputs BEFORE the CC1101
  // starts driving the sync DATA/CLOCK outputs, including after UART TX.
  const auto& pins=kRadioPins[static_cast<unsigned>(kRadio)];
  if(pins.gdo0<0 || pins.gdo2<0 || pins.gdo0==pins.gdo2 ||
     !GPIO_IS_VALID_GPIO(pins.gdo0) || !GPIO_IS_VALID_GPIO(pins.gdo2))
    return ESP_ERR_INVALID_ARG;
  gpio_config_t gpio{};
  gpio.pin_bit_mask=(uint64_t(1)<<pins.gdo0)|(uint64_t(1)<<pins.gdo2);
  gpio.mode=GPIO_MODE_INPUT;
  gpio.pull_up_en=GPIO_PULLUP_DISABLE;
  gpio.pull_down_en=GPIO_PULLDOWN_DISABLE;
  gpio.intr_type=GPIO_INTR_DISABLE;
  esp_err_t e=gpio_config(&gpio);
  if(e!=ESP_OK)return e;

  const uint32_t frequency=cc1101_frequency_word(p.frequency_hz);
  const RegisterSetting freq[]={
    {0x0d,static_cast<uint8_t>(frequency>>16)},
    {0x0e,static_cast<uint8_t>(frequency>>8)},
    {0x0f,static_cast<uint8_t>(frequency)}
  };
  for(const auto& r:freq) {
    uint8_t actual=0xff;
    if((e=bus_.write_register(kRadio,r.address,r.value))!=ESP_OK ||
       (e=bus_.read_register(kRadio,r.address,&actual))!=ESP_OK)return e;
    if(actual!=r.value)return ESP_ERR_INVALID_RESPONSE;
  }
  for(size_t i=0;i<p.register_count;++i) {
    const auto& r=p.modem_registers[i];
    uint8_t actual=0xff;
    if((e=bus_.write_register(kRadio,r.address,r.value))!=ESP_OK ||
       (e=bus_.read_register(kRadio,r.address,&actual))!=ESP_OK)return e;
    // DEVIATN bit 3 is reserved/reads as zero on CC1101.
    const uint8_t mask=r.address==0x15?0xf7:0xff;
    if((actual&mask)!=(r.value&mask))return ESP_ERR_INVALID_RESPONSE;
  }
  return ESP_OK;
}

esp_err_t Cc1101Sync868BenchRx::start(RadioId radio,const RfProfile& p) {
  if(radio!=kRadio || p.id!=RfProfileId::sync_868_bench ||
     p.rx_transport!=RxTransport::gdo_sync ||
     !profile_compatible_with_radio(p,radio) || !profile_has_registers(p))
    return ESP_ERR_INVALID_ARG;
  if(!make_mutex())return ESP_ERR_NO_MEM;
  xSemaphoreTake(guard_,portMAX_DELAY);
  SemaphoreRelease release{guard_};
  if(listening_ || sampler_.armed())return ESP_ERR_INVALID_STATE;
  if(!bus_.state(kRadio).present)return ESP_ERR_NOT_FOUND;
  if(!evidence_memory_) {
    evidence_memory_=static_cast<ClockedBitBlock*>(heap_caps_calloc(
      kEvidenceBlocks,sizeof(ClockedBitBlock),
      MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
    if(!evidence_memory_ || !evidence_.attach(evidence_memory_,kEvidenceBlocks))
      return ESP_ERR_NO_MEM;
  }
  // Do not invent an alternate FIFO or UART RX when sync initialization fails.
  // RadioManager receives a failure and marks this radio FAULT.
  esp_err_t e=bus_.strobe(kRadio,kSidle);
  if(e==ESP_OK)e=install_profile(p);
  if(e==ESP_OK) {
    uint32_t next=epoch_+1;
    if(!next)next=1;
    epoch_=next;
    // Rising edge: TI's documented candidate for noninverted GDO clock.
    // The actual sampling edge must be confirmed with a logic analyzer.
    e=sampler_.start(kRadio,epoch_,true);
  }
  if(e==ESP_OK)e=bus_.enter_rx_verified(kRadio);
  if(e!=ESP_OK) {
    (void)sampler_.stop();
    // Fail closed: leave the radio in IDLE, not FIFO listening by accident.
    (void)bus_.strobe(kRadio,kSidle);
    ++start_failures_;
    ESP_LOGE(TAG,"bench 868 synchronous RX start failed: %s",
             esp_err_to_name(e));
    return e;
  }
  activity_.reset_window();
  last_activity_attempt_us_=static_cast<uint64_t>(esp_timer_get_time());
  listening_=true;
  ESP_LOGW(TAG,"EXPERIMENTAL sync 868 GDO receiver armed epoch=%lu; RF/ISR not bench-validated",
           static_cast<unsigned long>(epoch_));
  return ESP_OK;
}

void Cc1101Sync868BenchRx::collect_completed() {
  ClockedBitBlock block{};
  while(capture_.pop(block)) {
    ++complete_blocks_;
    if(block.discontinuity_before)++blocks_with_gap_;
    last_bit_count_=block.bit_count;
    last_bit_time_us_=block.last_sample_us;
    // Evidence export is optional and independent of live interpretation.
    // Do not copy raw bits into ordinary logs. An observer merely hands the
    // immutable block to a bounded queue in the RX worker context (not ISR).
    (void)evidence_.append(block);
    const uint32_t sequence=++emitted_block_sequence_;
    if(block_observer_)block_observer_(block_context_,block,sequence);
  }
}

esp_err_t Cc1101Sync868BenchRx::stop_locked() {
  // Stop GPIO ISR before IDLE, UART TX pin claim or any CC1101 RF switch.
  const esp_err_t sampler_e=sampler_.stop();
  collect_completed();
  const esp_err_t idle_e=bus_.strobe(kRadio,kSidle);
  listening_=false;
  activity_.reset_window();
  return sampler_e!=ESP_OK?sampler_e:idle_e;
}

esp_err_t Cc1101Sync868BenchRx::stop(RadioId radio) {
  if(radio!=kRadio)return ESP_ERR_INVALID_ARG;
  if(!guard_)return ESP_ERR_INVALID_STATE;
  xSemaphoreTake(guard_,portMAX_DELAY);
  SemaphoreRelease release{guard_};
  if(!listening_ && !sampler_.armed())return ESP_ERR_INVALID_STATE;
  return stop_locked();
}

void Cc1101Sync868BenchRx::poll_once() {
  if(!guard_ || xSemaphoreTake(guard_,0)!=pdTRUE)return;
  SemaphoreRelease release{guard_};
  if(!listening_)return;
  sampler_.poll_once(1024);
  collect_completed();
  sample_activity(); // After bit draining, never from the clock ISR.
}

void Cc1101Sync868BenchRx::sample_activity(){
  const auto begin=static_cast<uint64_t>(esp_timer_get_time());
  if(begin-last_activity_attempt_us_<RxActivityTracker::kSampleIntervalUs)return;
  last_activity_attempt_us_=begin;
  uint8_t rssi=0;bool carrier=false;
  const auto result=bus_.try_read_rx_activity(kRadio,&rssi,&carrier);
  const auto end=static_cast<uint64_t>(esp_timer_get_time());
  const auto elapsed=static_cast<uint32_t>(end-begin);
  if(result==ESP_OK)activity_.observe(end,carrier,rssi,elapsed);
  else activity_.missed(elapsed);
  carrier_history_.append(epoch_,begin,end,result==ESP_OK,carrier,
    RxActivityTracker::approximate_rssi_x2(rssi),activity_.stats().bursts);
}

CaptureCarrierContext Cc1101Sync868BenchRx::try_correlate_activity(CaptureCarrierContext context) const {
  if(context.state!=CarrierContextState::history_unavailable ||
     !guard_ || xSemaphoreTake(guard_,0)!=pdTRUE)return context;
  SemaphoreRelease release{guard_};
  return carrier_history_.correlate(context);
}

bool Cc1101Sync868BenchRx::arm_evidence() {
  if(!guard_)return false;
  xSemaphoreTake(guard_,portMAX_DELAY);
  SemaphoreRelease release{guard_};
  if(!listening_ || evidence_export_active_)return false;
  // Drain all available pre-arm samples then discard the partial evidence
  // window. Acquiring the mutex is not an RF frame boundary: the first stored
  // bit may still lie in the middle of a real transmission.
  sampler_.poll_once(SyncClockEdgeRing::kCapacity);
  capture_.end_window();
  collect_completed();
  return evidence_.arm();
}

bool Cc1101Sync868BenchRx::freeze_evidence() {
  if(!guard_)return false;
  xSemaphoreTake(guard_,portMAX_DELAY);
  SemaphoreRelease release{guard_};
  if(evidence_export_active_)return false;
  // The bounded recorder automatically freezes at capacity, so an operator's
  // later "freeze" request is idempotent and does not report a fake failure.
  if(!evidence_.stats().recording)return evidence_.stats().full;
  // Record every already queued bit and publish one partial terminal block.
  // RF demodulation continues; no modem switch or UART/FIFO time sharing.
  if(listening_)sampler_.poll_once(SyncClockEdgeRing::kCapacity);
  capture_.end_window();
  collect_completed();
  return evidence_.freeze() || evidence_.stats().full;
}

SyncBitEvidenceStats Cc1101Sync868BenchRx::evidence_stats() const {
  SyncBitEvidenceStats out{};
  if(!guard_)return out;
  xSemaphoreTake(guard_,portMAX_DELAY);
  out=evidence_.stats();
  xSemaphoreGive(guard_);
  return out;
}

bool Cc1101Sync868BenchRx::begin_evidence_export(
    SyncBitEvidenceStats* snapshot) {
  if(!snapshot || !guard_)return false;
  xSemaphoreTake(guard_,portMAX_DELAY);
  const auto stats=evidence_.stats();
  const bool allowed=stats.ready && !stats.recording &&
                     !evidence_export_active_;
  if(allowed) {
    evidence_export_active_=true;
    *snapshot=stats;
  }
  xSemaphoreGive(guard_);
  return allowed;
}

bool Cc1101Sync868BenchRx::copy_evidence(
    uint32_t index, ClockedBitBlock* block) const {
  if(!block || !guard_)return false;
  xSemaphoreTake(guard_,portMAX_DELAY);
  const bool copied=evidence_export_active_ && evidence_.copy(index,block);
  xSemaphoreGive(guard_);
  return copied;
}

void Cc1101Sync868BenchRx::end_evidence_export() {
  if(!guard_)return;
  xSemaphoreTake(guard_,portMAX_DELAY);
  evidence_export_active_=false;
  xSemaphoreGive(guard_);
}

Sync868BenchStats Cc1101Sync868BenchRx::stats() const {
  Sync868BenchStats out{};
  if(!guard_)return out;
  xSemaphoreTake(guard_,portMAX_DELAY);
  out.listening=listening_ && sampler_.armed();
  out.epoch=epoch_;
  out.start_failures=start_failures_;
  out.complete_blocks=complete_blocks_;
  out.blocks_with_gap=blocks_with_gap_;
  out.last_bit_count=last_bit_count_;
  out.last_bit_time_us=last_bit_time_us_;
  out.sampler=sampler_.stats();
  out.capture=capture_.stats();
  out.activity=activity_.stats();
  xSemaphoreGive(guard_);
  return out;
}

} // namespace rflink
