// Host-side execution of radio availability, RX/TX telemetry and bounded history.
#include <cassert>
#include <iostream>
#include <vector>
#include "rflink_universal.h"

using esphome::rflink_universal::RFLinkUniversal;

int main() {
  RFLinkUniversal radio;
  radio.set_packet_channel(2, 868299900, "868.30 Itho");
  radio.set_radio_available(2, false, true, true);
  assert(radio.channel(2).checked && !radio.channel(2).available);
  assert(!radio.channel(2).rx_ready && !radio.channel(2).tx_ready);
  radio.record_tx_command(2, "must not appear");
  radio.receive_packet(2, {0x01, 0x02, 0x03}, -35, 10, 0);
  assert(radio.activity().empty());

  radio.set_radio_available(2, true, false, true);
  radio.receive_packet(2, {0x03, 0x02, 0x01}, -35, 10, 0);
  assert(radio.activity().empty());
  radio.record_tx_command(2, "Itho speed request");
  assert(radio.activity().size() == 1 && radio.activity()[0].direction == "TX");
  assert(radio.activity()[0].channel_index == 2);
  assert(radio.activity()[0].detail.find("geen bevestiging") != std::string::npos);

  radio.set_radio_available(2, true, true, false);
  radio.record_tx_command(2, "should not register");
  assert(radio.activity().size() == 1);
  radio.receive_packet(2, {0xAA, 0x12, 0xFF}, -50.0f, 80, 0);
  assert(radio.activity().size() == 2 && radio.activity()[0].direction == "RX");
  assert(radio.activity()[0].detail.find("AA 12 FF") != std::string::npos);
  assert(radio.channel(2).frames == 1);

  // RXDIAG1: a FIFO-sized packet that does not match the active Itho profile
  // is retained for diagnostics/export but filtered from the normal unknown flow.
  const uint32_t unknown_before_noise = radio.unknown_frames();
  radio.receive_packet(2, std::vector<uint8_t>(63, 0x7E), -99.0f, 91, 1250.0f);
  assert(radio.activity().size() == 3);
  assert(radio.activity()[0].noise_suspect);
  assert(radio.activity()[0].noise_reason.find("63-byte") != std::string::npos);
  assert(radio.activity()[0].export_detail.find("PACKET 63 bytes") != std::string::npos);
  assert(radio.activity()[0].export_detail.find("LQI 91") != std::string::npos);
  assert(radio.channel(2).noise_frames == 1 && radio.noise_frames() == 1);
  assert(radio.channel(2).has_lqi && radio.channel(2).last_lqi == 91);
  assert(radio.unknown_frames() == unknown_before_noise);

  radio.set_radio_available(2, true, true, true);
  for (int i=0; i<65; ++i) radio.record_tx_command(2, "bounded event");
  assert(radio.activity().size() == 32);
  for (auto &entry : radio.activity()) assert(entry.channel_index == 2);
  // Exercise the *production* availability aggregation for all three slots,
  // not only the isolated StatusLed policy class.
  RFLinkUniversal tri;
  tri.set_raw_channel(0, nullptr, 433420000, "433.42");
  tri.set_raw_channel(1, nullptr, 433920000, "433.92");
  tri.set_packet_channel(2, 868299900, "868.30 Itho");
  tri.status_led().wifi(true);
  tri.set_radio_available(0, false, false, false);
  tri.set_radio_available(1, true, true, true);
  tri.set_radio_available(2, true, true, true);
  assert(tri.status_led().mode(9000) == esphome::rflink_universal::StatusLed::Mode::RADIO_MISSING);
  tri.status_led().ota(true);
  assert(tri.status_led().mode(9000) == esphome::rflink_universal::StatusLed::Mode::OTA);
  tri.status_led().ota(false);
  tri.set_radio_available(0, true, true, true);
  assert(tri.status_led().mode(9000) == esphome::rflink_universal::StatusLed::Mode::READY);
  tri.set_radio_available(2, true, true, false);
  assert(tri.status_led().mode(9000) == esphome::rflink_universal::StatusLed::Mode::RADIO_MISSING);
  std::cout << "Core monitor: missing-radio guard, RX/TX split, all-radio diagnostics, noise filtering and 32-event limit: PASS\n";
}
