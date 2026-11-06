#include "rflink_radio_hal.hpp"
#include <cassert>
#include <set>
int main() {
 using namespace rflink;
 assert(kSpiPins.sclk==12 && kSpiPins.mosi==11 && kSpiPins.miso==13);
 assert(kSpiPins.clock_hz==200000);
 std::set<int> gpio={kSpiPins.sclk,kSpiPins.mosi,kSpiPins.miso};
 for(const auto& r:kRadioPins) {
   assert(gpio.insert(r.cs).second);
   assert(gpio.insert(r.gdo0).second);
   assert(gpio.insert(r.gdo2).second);
 }
 RadioState s{};
 assert(!is_online(s));
 s.present=s.spi_healthy=true;s.mode=RadioMode::rx;
 assert(is_online(s));
 s.mode=RadioMode::switching;
 assert(is_online(s)); // temporary profile switch must not mean OFFLINE
 s.mode=RadioMode::fault;
 assert(!is_online(s));
}
