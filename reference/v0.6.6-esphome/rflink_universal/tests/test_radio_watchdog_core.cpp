#include <cassert>
#include <iostream>
#include "rflink_universal.h"

using esphome::rflink_universal::RFLinkUniversal;
using esphome::cc1101::CC1101Component;

int main() {
  RFLinkUniversal core;
  core.set_packet_channel(0, 868299900, "watchdog-test");
  CC1101Component radio;
  core.set_radio_cc1101(0, &radio);

  radio.spi_read_value = 0x14;
  core.run_radio_watchdog(0, false);
  auto &ch = core.channel(0);
  assert(ch.spi_checks == 1 && ch.spi_failures == 0);
  assert(ch.self_test_spi_ok && core.radio_watchdog_healthy(0));
  assert(!core.take_radio_recovery_request(0));

  radio.spi_read_value = 0xFF;
  core.run_radio_watchdog(0, false);
  core.run_radio_watchdog(0, false);
  assert(core.radio_watchdog_healthy(0));
  core.run_radio_watchdog(0, false);
  assert(!core.radio_watchdog_healthy(0));
  assert(ch.spi_failures == 3 && ch.spi_consecutive_failures == 3);
  assert(ch.spi_recovery_attempts == 1);
  assert(core.take_radio_recovery_request(0));
  assert(!core.take_radio_recovery_request(0));

  // Cooldown prevents a reset storm while hardware remains absent.
  rflink_host_millis += 10000;
  core.run_radio_watchdog(0, false);
  assert(ch.spi_recovery_attempts == 1);
  rflink_host_millis += 21000;
  core.run_radio_watchdog(0, false);
  assert(ch.spi_recovery_attempts == 2);
  assert(core.take_radio_recovery_request(0));

  // A valid response after a recovery clears the consecutive failure count
  // and records a successful recovery.
  radio.spi_read_value = 0x14;
  core.run_radio_watchdog(0, false);
  assert(ch.spi_consecutive_failures == 0);
  assert(core.radio_watchdog_healthy(0));
  assert(ch.spi_recovery_successes == 1);

  // If ESPHome itself is failed while the chip responds, request a full
  // component reinitialisation after the cooldown.
  rflink_host_millis += 31000;
  core.run_radio_watchdog(0, true);
  assert(ch.spi_recovery_attempts == 3);
  assert(core.take_radio_recovery_request(0));

  std::cout << "Radio watchdog: 3-failure threshold, cooldown and recovery telemetry: PASS\n";
}
