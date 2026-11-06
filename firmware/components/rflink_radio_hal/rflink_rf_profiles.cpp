#include "rflink_rf_profiles.hpp"
#include <array>
namespace rflink {
namespace {
// Explicit CC1101 register map (datasheet): PKTLEN=0x06,
// PKTCTRL1=0x07, PKTCTRL0=0x08. Previous Stage 5 had these shifted.
constexpr RegisterSetting kItho868[] = {
 {0x00,0x06}, {0x02,0x06},
 {0x06,63}, {0x07,0x00}, {0x08,0x00},
 {0x04,0xab}, {0x05,0xfe},
 {0x10,0x9a}, {0x11,0x83}, {0x12,0x02},
 {0x13,0x20}, {0x14,0x00}, {0x15,0x40},
 {0x17,0x0c}, {0x18,0x18}, {0x0b,0x06},
 {0x0c,0x00}, {0x19,0x16}, {0x1b,0x43},
 {0x1c,0x40}, {0x1d,0x91}
};
// Stage 9 BENCH ONLY. Derived from v0.6.6 ASK/OOK (203 kHz,
// asynchronous GDO2) and CC1101 register map; not yet verified on RF hardware.
// This is a generic modem profile, not an RFLink protocol decoder.
// IOCFG2=0x0D async serial data; PKTCTRL0=0x30 asynchronous serial mode;
// MDMCFG2=0x30 ASK/OOK, no sync; MDMCFG4=0x8C 203.125 kHz RX filter BW
// at 26 MHz crystal; default data-rate exponent C and mantissa 0x22.
constexpr RegisterSetting kOok433[] = {
 {0x00,0x0d}, {0x02,0x2e},
 {0x06,0xff}, {0x07,0x04}, {0x08,0x30},
 {0x10,0x8c}, {0x11,0x22}, {0x12,0x30},
 {0x15,0x00}, {0x17,0x00}, {0x18,0x18}
};
// Retained from the v0.6.6 lab TX configuration. Not yet hardware-validated
// in native ESP-IDF. The reserved bit of DEVIATN must remain clear.
constexpr UartTxPreset kRamsesUart{38400, 0x30, 0x5a, 0x83, 0x00, 0x40};
// Conservative LAB-only PA presets, not a product power calibration.
// ASK needs off/on entries; for packet FSK one entry is used.
constexpr uint8_t kOokPa[] = {0x00,0x60};
constexpr uint8_t kPacketPa[] = {0x60};
constexpr std::array<RfProfile,5> kProfiles{{
 {RfProfileId::itho_868_packet,"Itho 868 packet",868299900,Modulation::fsk2,RxTransport::packet_fifo,TxTransport::gdo_uart,kItho868,sizeof(kItho868)/sizeof(kItho868[0]),63,nullptr,nullptr,0},
 {RfProfileId::ramses_868_packet,"RAMSES-II 868",868299900,Modulation::fsk2,RxTransport::packet_fifo,TxTransport::gdo_uart,nullptr,0,0,&kRamsesUart,nullptr,0},
 {RfProfileId::ook_43342_pulses,"433.42 pulse RX",433420000,Modulation::ook,RxTransport::gdo_rmt,TxTransport::gdo_rmt,kOok433,sizeof(kOok433)/sizeof(kOok433[0]),0,nullptr,kOokPa,sizeof(kOokPa)},
 {RfProfileId::ook_43392_pulses,"433.92 pulse RX",433920000,Modulation::ook,RxTransport::gdo_rmt,TxTransport::gdo_rmt,kOok433,sizeof(kOok433)/sizeof(kOok433[0]),0,nullptr,kOokPa,sizeof(kOokPa)},
 {RfProfileId::packet_868_bench,"868 fixed FIFO TX BENCH",868299900,Modulation::fsk2,RxTransport::packet_fifo,TxTransport::packet_fifo,kItho868,sizeof(kItho868)/sizeof(kItho868[0]),16,nullptr,kPacketPa,sizeof(kPacketPa)}
}};
}
const RfProfile& rf_profile(RfProfileId id) {
  const auto idx=static_cast<size_t>(id);
  return kProfiles[idx<kProfiles.size()?idx:0];
}
bool profile_compatible_with_radio(const RfProfile& p, RadioId radio) {
  const auto idx=static_cast<size_t>(radio);
  if(idx>=kRadioPins.size()) return false;
  const auto& hw=kRadioPins[idx];
  return p.frequency_hz==hw.frequency_hz ||
      (radio==RadioId::rf86830 && p.frequency_hz==868299900);
}
bool profile_has_registers(const RfProfile& p) {
  return p.modem_registers != nullptr && p.register_count > 0;
}
bool uart_tx_preset_valid(const RfProfile& p) {
  const auto* t = p.uart_tx;
  return p.tx_transport == TxTransport::gdo_uart && t &&
    t->baud >= 1200 && t->baud <= 115200 &&
    t->pktctrl0 == 0x30 && (t->deviatn & 0x08u) == 0;
}
bool rmt_tx_preset_valid(const RfProfile& p) {
  return p.tx_transport==TxTransport::gdo_rmt && p.modulation==Modulation::ook &&
    profile_has_registers(p) && p.pa_table && p.pa_table_len==2;
}
bool packet_tx_preset_valid(const RfProfile& p) {
  return p.tx_transport==TxTransport::packet_fifo &&
    profile_has_registers(p) && p.pa_table && p.pa_table_len==1 &&
    p.packet_length>=1 && p.packet_length<=63;
}
uint32_t cc1101_frequency_word(uint32_t hz) {
  return static_cast<uint32_t>((static_cast<uint64_t>(hz)<<16)/26000000u);
}
}
