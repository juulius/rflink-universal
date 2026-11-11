// Guard the existing generic decoder registry while adding experimental RAMSES-II.
// The old run_tests.sh referred to this test but did not ship the source.
#include <cassert>
#include <iostream>
#include "tx_engine.h"
using namespace esphome::rflink_universal;
int main() {
  tx_engine::ensure_linked();
  for(const char *protocol : {"Nexa","ByronSX","Dooya","RCSwitch","SomfyRTS56","IthoRFT","RamsesII"}) {
    auto *e=tx_engine::resolve(protocol);
    assert(e && "enabled TX manifest requires an actual registered encoder");
  }
  for(const char *protocol : {"KeeLoq","UnknownProtocol",""})
    assert(!tx_engine::resolve(protocol));
  DeviceRecord d;
  d.uid="test-unknown";d.protocol="RamsesII";d.profile_id="orcon_mvs15r_15rf";
  d.transmit_supported=true;
  tx_engine::Plan plan;
  const tx_engine::Encoder *encoder=nullptr;
  // Neither a matching profile nor received telegrams make a virtual remote paired.
  assert(!tx_engine::prepare(d,"auto",plan,encoder));
  d.tx_state={{"ramses_controller",std::to_string((37u<<18)|123u),""},
              {"ramses_fan",std::to_string((21u<<18)|321u),""},
              {"ramses_paired","0",""}};
  assert(!tx_engine::prepare(d,"auto",plan,encoder));
  d.tx_state.back().value="1";
  assert(tx_engine::prepare(d,"auto",plan,encoder));
  assert(encoder && plan.transport==TxTransport::GDO_SERIAL && plan.radio==2);
  assert(!tx_engine::prepare(d,"prog",plan,encoder));
  std::cout << "TX remaining: existing encoders registered, RAMSES pairing gate intact PASS\n";
}
