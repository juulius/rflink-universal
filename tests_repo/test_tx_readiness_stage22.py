"""Stage 22 TX readiness and fail-closed RX restoration using production C++."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
HAL=ROOT/"firmware/components/rflink_radio_hal"

CPP = r"""
#include <cassert>
#include <vector>
#include "rflink_radio_manager.hpp"
using namespace rflink;

struct FakeRadio final: RadioTransport {
  std::vector<int> calls;
  bool stop_ok=true;
  bool restart_ok=true;
  PhysicalTxResult outcome{true,true,TxFailureStage::none};
  bool start_packet_rx(RadioId id,const RfProfile& p) override {
    assert(id==RadioId::rf86830);
    assert(p.id==RfProfileId::itho_868_packet);
    calls.push_back(1);return restart_ok;
  }
  bool stop_packet_rx(RadioId id) override {
    assert(id==RadioId::rf86830);
    calls.push_back(2);return stop_ok;
  }
  bool start_pulse_rx(RadioId id,const RfProfile& p) override {
    assert(id==RadioId::rf43342 || id==RadioId::rf43392);
    assert(p.id==(id==RadioId::rf43342 ? RfProfileId::ook_43342_pulses : RfProfileId::ook_43392_pulses));
    calls.push_back(4);return restart_ok;
  }
  bool stop_pulse_rx(RadioId) override {calls.push_back(5);return stop_ok;}
  PhysicalTxResult transmit_uart(RadioId id,const RfProfile& p,const EncodedTxFrame&) override {
    assert(id==RadioId::rf86830 && p.id==RfProfileId::ramses_868_packet);
    calls.push_back(3);return outcome;
  }
  PhysicalTxResult transmit_rmt(RadioId id,const RfProfile& p,const EncodedTxFrame&) override {
    assert(id==RadioId::rf43342 || id==RadioId::rf43392);
    assert(p.tx_transport==TxTransport::gdo_rmt);
    calls.push_back(6);return outcome;
  }
  PhysicalTxResult transmit_packet(RadioId id,const RfProfile& p,const EncodedTxFrame&) override {
    assert(id==RadioId::rf86830 && p.id==RfProfileId::packet_868_bench);
    calls.push_back(7);return outcome;
  }
};

int main(){
  FakeRadio fake;
  RadioManager manager(fake);
  uint8_t wire[16]={};
  const EncodedTxFrame ramses{wire,4,1,0};
  const EncodedTxFrame packet{wire,16,1,0};
  const TxPulsePair pulses[]={{350,700}};
  const EncodedTxFrame ook{nullptr,0,1,0,pulses,1};

  assert(manager.request_transmit(RadioId::rf86830,RfProfileId::ramses_868_packet)==ManagerError::no_hardware);
  manager.set_presence(RadioId::rf86830,true,true);
  assert(manager.start_receive(RadioId::rf86830,RfProfileId::itho_868_packet)==ManagerError::none);
  fake.calls.clear();

  // A listening 868 RX is eligible for TX. Preflight may not stop RX.
  assert(manager.request_transmit(RadioId::rf86830,RfProfileId::ramses_868_packet)==ManagerError::none);
  assert(manager.request_transmit(RadioId::rf86830,RfProfileId::packet_868_bench)==ManagerError::none);
  assert(fake.calls.empty());
  assert(manager.status(RadioId::rf86830).operation==RadioOperation::receiving);
  assert(manager.status(RadioId::rf86830).profile==RfProfileId::itho_868_packet);

  assert(manager.transmit_encoded(RadioId::rf86830,RfProfileId::ramses_868_packet,ramses)==ManagerError::none);
  assert((fake.calls==std::vector<int>{2,3,1}));
  assert(manager.status(RadioId::rf86830).operation==RadioOperation::receiving);
  assert(manager.status(RadioId::rf86830).profile==RfProfileId::itho_868_packet);
  assert(manager.last_tx_stage(RadioId::rf86830)==TxFailureStage::none);

  fake.calls.clear();fake.outcome={false,true,TxFailureStage::uart_drain};
  assert(manager.transmit_encoded(RadioId::rf86830,RfProfileId::ramses_868_packet,ramses)==ManagerError::tx_failure);
  assert((fake.calls==std::vector<int>{2,3,1}));
  assert(manager.last_tx_stage(RadioId::rf86830)==TxFailureStage::uart_drain);
  assert(manager.status(RadioId::rf86830).operation==RadioOperation::receiving);

  // A new invalid attempt never reports the failure stage of a *previous* TX.
  assert(manager.transmit_encoded(RadioId::rf86830,RfProfileId::ramses_868_packet,{wire,3,1,0})==ManagerError::invalid_payload);
  assert(manager.last_tx_stage(RadioId::rf86830)==TxFailureStage::none);

  fake.stop_ok=false;fake.calls.clear();
  assert(manager.transmit_encoded(RadioId::rf86830,RfProfileId::ramses_868_packet,ramses)==ManagerError::restore_failure);
  assert((fake.calls==std::vector<int>{2})); // no TX when RX could not pause
  assert(manager.last_tx_stage(RadioId::rf86830)==TxFailureStage::rx_pause);
  assert(manager.status(RadioId::rf86830).operation==RadioOperation::fault);
  assert(manager.request_transmit(RadioId::rf86830,RfProfileId::ramses_868_packet)==ManagerError::busy);

  fake.stop_ok=true;fake.outcome={true,true,TxFailureStage::none};
  manager.set_presence(RadioId::rf86830,true,true);
  assert(manager.start_receive(RadioId::rf86830,RfProfileId::itho_868_packet)==ManagerError::none);
  fake.calls.clear();
  assert(manager.transmit_encoded(RadioId::rf86830,RfProfileId::packet_868_bench,packet)==ManagerError::none);
  assert((fake.calls==std::vector<int>{2,7,1}));
  assert(manager.status(RadioId::rf86830).profile==RfProfileId::itho_868_packet);

  // Both 433 MHz modules have the same RX-eligible, profile-defined RMT path.
  for(RadioId id: {RadioId::rf43342,RadioId::rf43392}){
    const auto profile=id==RadioId::rf43342?RfProfileId::ook_43342_pulses:RfProfileId::ook_43392_pulses;
    manager.set_presence(id,true,true);
    assert(manager.start_receive(id,profile)==ManagerError::none);
    fake.calls.clear();
    assert(manager.request_transmit(id,profile)==ManagerError::none);
    assert(fake.calls.empty());
    assert(manager.transmit_encoded(id,profile,ook)==ManagerError::none);
    assert((fake.calls==std::vector<int>{5,6,4}));
    assert(manager.status(id).operation==RadioOperation::receiving);
    assert(manager.status(id).profile==profile);
  }

  // Explicit numeric assignments of existing stages are not needed, but the
  // new rx_pause stage must be appended for existing API numeric stability.
  assert(static_cast<unsigned>(TxFailureStage::rx_pause)>static_cast<unsigned>(TxFailureStage::restore));
}
"""

class TxReadinessRegressionTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"),"g++ host compiler unavailable")
    def test_preflight_and_rx_restore_on_three_radios(self):
        with tempfile.TemporaryDirectory(prefix="rflink-tx-s22-") as directory:
            directory=Path(directory)
            source=directory/"test.cpp"
            binary=directory/"test"
            source.write_text(CPP,encoding="utf-8")
            subprocess.run([
                "g++","-std=c++17","-O2","-Wall","-Wextra","-Werror","-pthread",
                "-I"+str(HAL/"include"),
                str(source),str(HAL/"rflink_rf_profiles.cpp"),
                str(HAL/"rflink_radio_manager.cpp"),"-o",str(binary)
            ],check=True,text=True,capture_output=True,timeout=60)
            subprocess.run([str(binary)],check=True,text=True,capture_output=True,timeout=15)

if __name__=="__main__":
    unittest.main()
