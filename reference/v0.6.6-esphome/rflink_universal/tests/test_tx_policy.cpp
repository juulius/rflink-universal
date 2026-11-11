#include <cassert>
#include <iostream>
#include "tx_policy.h"
using namespace esphome::rflink_universal;
static DeviceRecord nexa() {
  DeviceRecord d;
  d.uid="dev_00042"; d.name="Nexa lamp"; d.protocol="Nexa";
  d.identity_confidence=IdentityConfidence::HIGH;
  d.transmit_supported=true;
  d.rf_identities={"nexa:012ABCD"};
  d.actions={"on","off"};
  d.fields={{"channel","2",""},{"group","false",""},
            {"level","0",""},{"tx_radio","1",""}};
  return d;
}
int main() {
  tx::NexaCommand cmd;
  auto d=nexa();
  assert(tx::eligible(d) && tx::nexa(d,"on",cmd));
  assert(cmd.device==0x12ABCD && cmd.channel==2 && cmd.state && cmd.group==0);
  assert(tx::nexa(d,"off",cmd) && !cmd.state);
  assert(!tx::nexa(d,"toggle",cmd));
  d.fields.back().value="0"; assert(!tx::eligible(d));
  d=nexa(); d.identity_confidence=IdentityConfidence::LOW;assert(!tx::eligible(d));
  d=nexa(); d.rf_identities.push_back("nexa:0000042");assert(!tx::eligible(d));
  d=nexa(); d.protocol="Somfy";assert(!tx::eligible(d));
  d=nexa(); d.actions={"on"};assert(!tx::nexa(d,"off",cmd));
  d=nexa(); d.fields.pop_back();assert(!tx::eligible(d)); // old device needs new RX capture
  d=nexa();d.fields[0].value="16";assert(!tx::eligible(d));
  d=nexa();d.rf_identities={"nexa:xxbad"};assert(!tx::eligible(d));
  // Active TX always uses locked, manually approved snapshot, not new RX fields.
  d=nexa();d.transmit_enabled=true;
  d.fields.push_back({"tx_lock_radio","1",""});
  d.fields.push_back({"tx_lock_channel","2",""});
  d.fields.push_back({"tx_lock_group","false",""});
  d.fields.push_back({"tx_lock_level","0",""});
  d.fields[0].value="11";d.fields[3].value="2";
  assert(tx::nexa(d,"on",cmd) && cmd.channel==2);
  d.fields.back().value="99";assert(!tx::eligible(d));
  std::cout << "TX1 policy: Nexa eligibility, opt-in identities, safe channel, locked snapshot, invalid actions: PASS\n";
}
