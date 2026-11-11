#pragma once
// Shared CC1101 packet RX recovery after exclusive UART/GDO ownership.
// Caller must have stopped UART and released any temporarily routed GPIO.
// Called only from the RFLink loop, with exclusive access to this CC1101.
#ifdef USE_ESP32
#include "esphome/components/cc1101/cc1101.h"
namespace esphome { namespace rflink_universal {
inline bool restore_cc1101_packet_rx(cc1101::CC1101Component &radio) {
  radio.set_idle();
  radio.set_packet_mode(false); // detach old packet IRQ before reset
  radio.reset();                // ESPHome's configured RX modem profile
  if (radio.is_failed()) return false;
  radio.begin_rx();             // re-establish the CC1101 RX/GDO input path
  radio.set_idle();
  radio.set_packet_mode(true);  // reconnect ESPHome packet IRQ consumer
  radio.begin_rx();             // calibrated RX, with packet mode enabled
  return !radio.is_failed();
}
}} // namespace esphome::rflink_universal
#endif
