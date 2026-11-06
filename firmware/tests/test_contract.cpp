#include "rflink_radio_hal.hpp"
#include <cassert>
int main(){
  using namespace rflink;
  static_assert(kRadioPins.size()==3);
  static_assert(kSpiPins.clock_hz==200000);
  assert(kRadioPins[2].cs==15 && kRadioPins[2].gdo0==16);
  assert(kRadioPins[1].gdo2==14);
  assert(is_online({true,true,RadioMode::switching,false}));
  assert(is_online({true,true,RadioMode::rx,true}));
  assert(!is_online({true,true,RadioMode::fault,false}));
  assert(!is_online({false,true,RadioMode::rx,true}));
}
