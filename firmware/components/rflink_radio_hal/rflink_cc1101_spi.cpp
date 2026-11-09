#include "rflink_cc1101_spi.hpp"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include <cstring>

namespace rflink {
namespace {
constexpr const char* TAG = "rflink.spi";
constexpr uint8_t kRead = 0x80;
constexpr uint8_t kBurst = 0x40;
constexpr uint8_t kPartnum = 0x30;  // status-register address
constexpr uint8_t kVersion = 0x31;  // status-register address
constexpr uint8_t kSres = 0x30;     // reset strobe, only during boot
constexpr uint8_t kMarcstate = 0x35;
constexpr uint8_t kRxbytes = 0x3b;
constexpr int kLockTimeoutMs = 250;
// Datasheet section 10.1: after CSn goes low SO/MISO must go low
// (CHIP_RDYn) before the first SPI clock. A bounded timeout also means
// one unplugged CC1101 cannot indefinitely block the other radios.
constexpr unsigned kReadyTimeoutUs = 3000;
constexpr unsigned kReadyPollUs = 10;
static_assert(kSpiPins.clock_hz <= 6000000, "Initial safe SPI speed unexpected");
}

Cc1101SpiBus::~Cc1101SpiBus() {
  for (auto &d : dev_) if (d) { spi_bus_remove_device(d); d = nullptr; }
  if (ready_) { spi_bus_free(SPI2_HOST); ready_ = false; }
  if (lock_) { vSemaphoreDelete(lock_); lock_ = nullptr; }
}

esp_err_t Cc1101SpiBus::begin() {
  if (ready_) return ESP_OK;
  if (!lock_) lock_ = xSemaphoreCreateMutex();
  if (!lock_) return ESP_ERR_NO_MEM;
  // Drive all CS pins inactive before enabling SPI. We deliberately use
  // software CS: ESP-IDF hardware-CS cannot wait for CC1101 CHIP_RDYn.
  // Each command, burst and strobe then uses one exclusive CS window.
  for (const auto &pins: kRadioPins) {
    esp_err_t ce = gpio_reset_pin(static_cast<gpio_num_t>(pins.cs));
    if (ce == ESP_OK) ce = gpio_set_level(static_cast<gpio_num_t>(pins.cs), 1);
    if (ce == ESP_OK) ce = gpio_set_direction(static_cast<gpio_num_t>(pins.cs), GPIO_MODE_OUTPUT);
    if (ce != ESP_OK) return ce;
  }
  spi_bus_config_t bus{};
  bus.mosi_io_num = kSpiPins.mosi;
  bus.miso_io_num = kSpiPins.miso;
  bus.sclk_io_num = kSpiPins.sclk;
  bus.quadwp_io_num = -1;
  bus.quadhd_io_num = -1;
  bus.max_transfer_sz = 64;
  esp_err_t err = spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_DISABLED);
  if (err != ESP_OK) return err;
  for (size_t i=0;i<dev_.size();++i) {
    spi_device_interface_config_t cfg{};
    cfg.clock_speed_hz = kSpiPins.clock_hz;
    cfg.mode = 0;
    cfg.spics_io_num = -1; // software CS + SO/CHIP_RDYn guard (see transaction_burst)
    cfg.queue_size = 1;
    err = spi_bus_add_device(SPI2_HOST, &cfg, &dev_[i]);
    if (err != ESP_OK) {
      for (auto &d : dev_) if (d) { spi_bus_remove_device(d); d = nullptr; }
      spi_bus_free(SPI2_HOST);
      return err;
    }
  }
  ready_ = true;
  ESP_LOGI(TAG, "SPI2 %lu Hz board=%s hash=%s; software CS + CC1101 SO ready check",
           static_cast<unsigned long>(kSpiPins.clock_hz), kBoardId, kBoardHash);
  return ESP_OK;
}

esp_err_t Cc1101SpiBus::transaction(RadioId id, const uint8_t* tx, uint8_t* rx, size_t bytes) {
  if(bytes>16)return ESP_ERR_INVALID_ARG;
  return transaction_burst(id,tx,rx,bytes);
}
esp_err_t Cc1101SpiBus::transaction_burst(RadioId id, const uint8_t* tx, uint8_t* rx, size_t bytes) {
  const auto i = index(id);
  if (!ready_ || i>=dev_.size() || !dev_[i] || !tx || bytes==0 || bytes>64) return ESP_ERR_INVALID_ARG;
  if (xSemaphoreTake(lock_, pdMS_TO_TICKS(kLockTimeoutMs)) != pdTRUE) return ESP_ERR_TIMEOUT;
  const auto cs = static_cast<gpio_num_t>(kRadioPins[i].cs);
  esp_err_t e = gpio_set_level(cs, 0);
  if (e == ESP_OK) {
    bool chip_ready = false;
    for (unsigned elapsed=0; elapsed<=kReadyTimeoutUs; elapsed+=kReadyPollUs) {
      if (gpio_get_level(static_cast<gpio_num_t>(kSpiPins.miso)) == 0) {
        chip_ready=true;
        break;
      }
      if(elapsed < kReadyTimeoutUs) esp_rom_delay_us(kReadyPollUs);
    }
    if (!chip_ready) {
      ready_timeouts_[i].fetch_add(1,std::memory_order_relaxed);
      e=ESP_ERR_TIMEOUT; // Do not clock a radio that is not ready.
    } else {
      spi_transaction_t t{};
      t.length=bytes*8;
      t.tx_buffer=tx;
      t.rx_buffer=rx;
      e=spi_device_polling_transmit(dev_[i],&t);
      if(e!=ESP_OK)transfer_errors_[i].fetch_add(1,std::memory_order_relaxed);
    }
    // CS must return high even after timeout or SPI error. Never hold CS low
    // while other CC1101 operations run or when the calling task exits.
  }
  // Also attempt deassertion when asserting CS reported an error. A GPIO
  // failure must never silently leave a second radio selected.
  const esp_err_t released=gpio_set_level(cs,1);
  if(e==ESP_OK)e=released;
  if(released!=ESP_OK)transfer_errors_[i].fetch_add(1,std::memory_order_relaxed);
  xSemaphoreGive(lock_);
  return e;
}

esp_err_t Cc1101SpiBus::read_register(RadioId id, uint8_t addr, uint8_t* value) {
  if (!value || addr>=0x30) return ESP_ERR_INVALID_ARG;
  uint8_t tx[2] = {static_cast<uint8_t>(addr | kRead),0};
  uint8_t rx[2]{};
  const auto err=transaction(id,tx,rx,2);
  if (err==ESP_OK) *value=rx[1];
  return err;
}

esp_err_t Cc1101SpiBus::write_register(RadioId id, uint8_t addr, uint8_t value) {
  if (addr>=0x30) return ESP_ERR_INVALID_ARG;
  const uint8_t tx[2]={addr,value};
  return transaction(id,tx,nullptr,2);
}

esp_err_t Cc1101SpiBus::read_status_register(RadioId id, uint8_t addr, uint8_t* value) {
  if (!value || addr<0x30 || addr>0x3d) return ESP_ERR_INVALID_ARG;
  // CC1101 status registers require both READ and BURST bits (0xC0).
  uint8_t tx[2]={static_cast<uint8_t>(addr | kRead | kBurst),0};
  uint8_t rx[2]{};
  const auto err=transaction(id,tx,rx,2);
  if(err==ESP_OK) *value=rx[1];
  return err;
}

esp_err_t Cc1101SpiBus::read_rx_fifo(RadioId id, uint8_t* out, size_t length) {
  if (!out || length == 0 || length > 63) return ESP_ERR_INVALID_ARG;
  // FIFO burst must remain one CS-protected transfer with the same CHIP_RDYn
  // validation as all status, register, strobe and PATABLE accesses.
  uint8_t tx[64] = {static_cast<uint8_t>(0x3f | kRead | kBurst)};
  uint8_t rx[64] = {};
  const esp_err_t err=transaction_burst(id,tx,rx,length+1);
  if (err == ESP_OK) memcpy(out, rx + 1, length);
  return err;
}

esp_err_t Cc1101SpiBus::write_tx_fifo(RadioId id, const uint8_t* src, size_t length) {
  if (!src || length==0 || length>63) return ESP_ERR_INVALID_ARG;
  uint8_t tx[64] = {0x7f}; // 0x3F | burst, no READ bit.
  memcpy(tx+1,src,length);
  return transaction_burst(id,tx,nullptr,length+1);
}
esp_err_t Cc1101SpiBus::read_tx_fifo_status(RadioId id,uint8_t* txbytes) {
  return read_status_register(id,0x3a,txbytes);
}
esp_err_t Cc1101SpiBus::read_pa_table(RadioId id,uint8_t* out,size_t length) {
  if (!out || length==0 || length>8) return ESP_ERR_INVALID_ARG;
  uint8_t tx[9] = {0xfe}, rx[9] = {};
  const auto err=transaction_burst(id,tx,rx,length+1);
  if (err == ESP_OK) memcpy(out, rx + 1, length);
  return err;
}
esp_err_t Cc1101SpiBus::write_pa_table(RadioId id,const uint8_t* src,size_t length) {
  if (!src || length==0 || length>8) return ESP_ERR_INVALID_ARG;
  uint8_t tx[9] = {0x7e};
  memcpy(tx+1,src,length);
  return transaction_burst(id,tx,nullptr,length+1);
}
esp_err_t Cc1101SpiBus::strobe(RadioId id,uint8_t cmd,uint8_t* chip_status) {
  if(cmd<kSres || cmd>0x3d) return ESP_ERR_INVALID_ARG;
  uint8_t rx[1]{};
  const uint8_t tx[1]={cmd};
  auto err=transaction(id,tx,rx,1);
  if(err==ESP_OK && chip_status) *chip_status=rx[0];
  return err;
}

esp_err_t Cc1101SpiBus::read_identity(RadioId id,uint8_t* partnum,uint8_t* version) {
  if(!partnum || !version)return ESP_ERR_INVALID_ARG;
  esp_err_t e=read_status_register(id,kPartnum,partnum);
  if(e==ESP_OK)e=read_status_register(id,kVersion,version);
  return e;
}

esp_err_t Cc1101SpiBus::read_runtime_status(RadioId id, uint8_t* marcstate, uint8_t* rxbytes) {
  if (!marcstate || !rxbytes) return ESP_ERR_INVALID_ARG;
  esp_err_t e = read_status_register(id, kMarcstate, marcstate);
  if (e == ESP_OK) e = read_status_register(id, kRxbytes, rxbytes);
  return e;
}

esp_err_t Cc1101SpiBus::enter_rx_verified(RadioId id) {
  // This is called only by a receiver start while the RadioManager owns
  // this radio. The prior TX transport has released UART/RMT/GDO ownership.
  // The validated legacy driver waits for the CC1101 state and checks PLL calibration, whereas
  // the earlier native start accepted SRX without proving reception.
  constexpr uint8_t kSidle=0x36, kSfrx=0x3a, kSrx=0x34;
  constexpr uint8_t kIdleState=0x01, kRxState=0x0d;
  constexpr uint8_t kFscal1=0x25, kPllNotLocked=0x3f;
  constexpr int64_t kStateWaitUs=100000;
  constexpr unsigned kPllRetries=3;
  auto wait_for=[&](uint8_t expected,uint8_t* last)->esp_err_t {
    const int64_t deadline=esp_timer_get_time()+kStateWaitUs;
    do {
      uint8_t marc=0xff,bytes=0xff;
      const esp_err_t e=read_runtime_status(id,&marc,&bytes);
      if(e!=ESP_OK)return e;
      *last=marc&0x1f;
      if(*last==expected)return ESP_OK;
      esp_rom_delay_us(100);
    }while(esp_timer_get_time()<deadline);
    return ESP_ERR_TIMEOUT;
  };
  for(unsigned attempt=1;attempt<=kPllRetries;++attempt) {
    uint8_t last=0xff;
    esp_err_t e=strobe(id,kSidle);
    if(e!=ESP_OK){
      ESP_LOGE(TAG,"radio%u RX restore SIDLE failed: %s",unsigned(id),esp_err_to_name(e));
      return e;
    }
    e=wait_for(kIdleState,&last);
    if(e!=ESP_OK){
      ESP_LOGE(TAG,"radio%u RX restore IDLE wait failed: %s MARCSTATE=0x%02x",unsigned(id),esp_err_to_name(e),last);
      return e;
    }
    e=strobe(id,kSfrx); // SFRX must only be issued from IDLE.
    if(e!=ESP_OK){
      ESP_LOGE(TAG,"radio%u RX restore SFRX failed: %s",unsigned(id),esp_err_to_name(e));
      return e;
    }
    e=strobe(id,kSrx);
    if(e!=ESP_OK){
      ESP_LOGE(TAG,"radio%u RX restore SRX failed: %s",unsigned(id),esp_err_to_name(e));
      return e;
    }
    e=wait_for(kRxState,&last);
    if(e!=ESP_OK){
      ESP_LOGE(TAG,"radio%u RX restore RX wait failed: %s MARCSTATE=0x%02x",unsigned(id),esp_err_to_name(e),last);
      (void)strobe(id,kSidle);
      return e;
    }
    uint8_t pll=0xff;
    e=read_register(id,kFscal1,&pll);
    if(e!=ESP_OK){
      ESP_LOGE(TAG,"radio%u RX restore FSCAL1 read failed: %s",unsigned(id),esp_err_to_name(e));
      (void)strobe(id,kSidle);
      return e;
    }
    if(pll!=kPllNotLocked)return ESP_OK;
    ESP_LOGW(TAG,"radio%u RX PLL unlock FSCAL1=0x%02x attempt=%u/%u",unsigned(id),pll,attempt,kPllRetries);
    // Do not SRES an active shared SPI bus; retry only this radio's
    // calibrated IDLE -> RX transition under the manager lease.
  }
  (void)strobe(id,kSidle);
  ESP_LOGE(TAG,"radio%u RX restore PLL lock failed after %u attempts",unsigned(id),kPllRetries);
  return ESP_ERR_INVALID_RESPONSE;
}

ProbeResult Cc1101SpiBus::boot_reset_and_probe(RadioId id) {
  ProbeResult out{};
  const auto i = index(id);
  if (i >= 3 || !ready_) { out.error = ESP_ERR_INVALID_STATE; return out; }
  // Hardware-owning boot procedure. Never reset a radio used by RX/TX.
  // Initial CS wake-up pulse before SRES (only during boot, never RX/TX).
  // This is NOT the full datasheet manual POR pulse sequence because the SPI
  // driver owns SCLK and MOSI; the board's hardware POR must work. Crucially,
  // all subsequent commands still wait for CHIP_RDYn before clocking.
  const auto cs=static_cast<gpio_num_t>(kRadioPins[i].cs);
  if(xSemaphoreTake(lock_,pdMS_TO_TICKS(kLockTimeoutMs))!=pdTRUE){out.error=ESP_ERR_TIMEOUT;return out;}
  esp_err_t ce=gpio_set_level(cs,0);
  if(ce==ESP_OK){esp_rom_delay_us(50);ce=gpio_set_level(cs,1);}
  esp_rom_delay_us(50);
  xSemaphoreGive(lock_);
  if(ce!=ESP_OK){out.error=ce;return out;}
  out.error = strobe(id, kSres);
  if (out.error != ESP_OK) return out;
  out.reset_issued = true;
  esp_rom_delay_us(1500);
  out = probe(id);
  out.reset_issued = true;
  if (out.plausible) {
    esp_err_t e = read_runtime_status(id, &out.marcstate, &out.rxbytes);
    if (e != ESP_OK) {
      out.error = e; out.plausible = false;
      states_[i].present=false; states_[i].spi_healthy=false;
      states_[i].mode=RadioMode::fault;
    }
  }
  ESP_LOGI(TAG, "boot radio%u reset=%d part=0x%02x version=0x%02x marc=0x%02x rxbytes=0x%02x valid=%d err=%s",
    unsigned(i), out.reset_issued, out.partnum, out.version, out.marcstate, out.rxbytes,
    out.plausible, esp_err_to_name(out.error));
  return out;
}

ProbeResult Cc1101SpiBus::probe(RadioId id) {
  ProbeResult p{};
  const auto i=index(id);
  if(i>=3 || !ready_) {p.error=ESP_ERR_INVALID_STATE;return p;}
  // Read-only probe: never reset or reconfigure a potentially active radio.
  p.error=read_status_register(id,kPartnum,&p.partnum);
  if(p.error==ESP_OK) p.error=read_status_register(id,kVersion,&p.version);
  // PARTNUM=0 and a non-floating VERSION are indicative, not proof of RX.
  p.plausible = p.error==ESP_OK && p.partnum==0x00 && p.version!=0x00 && p.version!=0xff;
  states_[i].present=p.plausible;
  states_[i].spi_healthy=p.plausible;
  states_[i].mode=p.plausible?RadioMode::idle:RadioMode::offline;
  states_[i].rx_processing=false;
  ESP_LOGI(TAG,"radio %u CS=%d part=0x%02x version=0x%02x probe=%s",
    unsigned(i),kRadioPins[i].cs,p.partnum,p.version,p.plausible?"plausible":"NOT VERIFIED");
  return p;
}

SpiHealthCounters Cc1101SpiBus::health(RadioId id) const {
  const auto i=index(id);
  if(i>=dev_.size())return {};
  return {ready_timeouts_[i].load(std::memory_order_relaxed),
          transfer_errors_[i].load(std::memory_order_relaxed)};
}

RadioState Cc1101SpiBus::state(RadioId id) const {
  auto i=index(id);
  return i<states_.size()?states_[i]:RadioState{};
}
} // namespace rflink
