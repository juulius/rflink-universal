#include <cassert>
#include <iostream>
#include "status_led.h"

using namespace esphome::rflink_universal;
using Mode = StatusLed::Mode;
int main() {
  StatusLed led;
  led.boot(100);
  assert(led.mode(200) == Mode::BOOT);
  led.wifi(true);
  assert(led.mode(200)==Mode::BOOT);
  assert(led.mode(3101)==Mode::READY);
  assert(led.sample(3101).green==1.0f);
  led.learn(true);
  assert(led.mode(3101)==Mode::LEARNING);
  led.capture(3200);
  assert(led.mode(3210)==Mode::CAPTURE);
  assert(led.mode(3990)==Mode::LEARNING);
  led.tx(4000);
  assert(led.mode(4010)==Mode::LEARNING); // TX must not override learning.
  led.pair_start(4000);
  assert(led.mode(4010)==Mode::PAIRING);
  led.pair_end(4100);
  assert(led.mode(4800)==Mode::PAIRING);
  assert(led.mode(5010)==Mode::LEARNING);
  led.learned(5100);
  assert(led.mode(5150)==Mode::LEARNED);
  led.learn(false);
  assert(led.mode(5151)==Mode::LEARNED); // success survives immediate stop
  led.learn(true);
  led.ota(true);
  assert(led.mode(5150)==Mode::OTA); // highest priority.
  led.error(5200);
  assert(led.mode(5250)==Mode::OTA);
  led.ota(false);
  assert(led.mode(5250)==Mode::ERROR);
  assert(led.mode(8751)==Mode::LEARNING);
  led.learn(false);
  led.wifi(false);
  assert(led.mode(8900)==Mode::OFFLINE);
  led.wifi(true);
  led.settings(2,true);
  assert(led.brightness()==2 && led.quiet());
  assert(led.mode(8900)==Mode::READY && led.sample(8900).brightness==0.0f);
  led.settings(200,false);
  assert(led.brightness()==100);
  led.tx(9000);
  assert(led.mode(9001)==Mode::TX);
  assert(led.mode(9200)==Mode::READY);
  led.fault(true);
  assert(led.mode(9200)==Mode::ERROR);
  led.ota(true);
  assert(led.mode(9200)==Mode::OTA);
  led.ota(false);
  led.fault(false);
  assert(led.mode(9200)==Mode::READY);
  // A board without any CC1101 must not show green READY. OTA must have
  // higher priority than missing hardware so recovery remains possible.
  StatusLed no_radios;
  no_radios.boot(10);
  no_radios.wifi(true);
  no_radios.radio_missing(true);
  assert(no_radios.mode(4000) == Mode::RADIO_MISSING);
  assert(no_radios.has_missing_radio());
  assert(no_radios.sample(6000).red == 1.0f);
  assert(no_radios.sample(6000).green > 0.0f);
  assert(no_radios.sample(6000).blue == 0.0f);
  assert(no_radios.sample(6000).brightness > 0.0f);
  no_radios.ota(true);
  assert(no_radios.mode(6000) == Mode::OTA);
  assert(no_radios.sample(6000).red == 1.0f && no_radios.sample(6000).blue == 1.0f);
  no_radios.ota(false);
  assert(no_radios.mode(6000) == Mode::RADIO_MISSING);
  no_radios.fault(true);
  assert(no_radios.mode(6000) == Mode::ERROR); // A fatal error outranks an absent radio.
  no_radios.fault(false);
  no_radios.radio_missing(false);
  assert(no_radios.mode(6000) == Mode::READY);
  // 32-bit timer wrap: events still expire correctly.
  StatusLed rollover;
  rollover.wifi(true);
  rollover.tx(0xFFFFFFF0u);
  assert(rollover.mode(0xFFFFFFF5u)==Mode::TX);
  assert(rollover.mode(200u)==Mode::READY);
  StatusLed abandoned_pair;
  abandoned_pair.wifi(true);
  abandoned_pair.pair_start(1000);
  assert(abandoned_pair.mode(2000)==Mode::PAIRING);
  assert(abandoned_pair.mode(16001)==Mode::READY); // send script failed, not stuck purple
  std::cout << "LED status priority, transitions, brightness and wraparound: PASS\n";
}
