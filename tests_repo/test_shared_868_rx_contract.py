"""Integration contract: one 868.30 MHz packet RX for Itho and RAMSES-II.

Uses the actual protocol decoder and RadioManager, with a mock physical
transport. The test cannot prove reception or transmission on RF hardware.
No private captures, keys or device identities are required.
"""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
FW=ROOT/"firmware"
HAL=FW/"components/rflink_radio_hal"
DECODE=FW/"components/rflink_decoder_core"

TEST_PROGRAM=r"""
#include <cassert>
#include <cstdio>
#include <vector>
#include "rflink_radio_manager.hpp"
#include "rflink_native_decoder.hpp"
#include "fixtures/synthetic_ramses.hpp"

using namespace rflink;
struct MockTransport final: RadioTransport {
  std::vector<int> calls;
  PhysicalTxResult outcome{true,true,TxFailureStage::none};
  bool start_packet_rx(RadioId id,const RfProfile& p) override {
    assert(id==RadioId::rf86830);
    assert(p.id==RfProfileId::itho_868_packet);
    calls.push_back(1);return true;
  }
  bool stop_packet_rx(RadioId id) override {
    assert(id==RadioId::rf86830);
    calls.push_back(2);return true;
  }
  bool start_pulse_rx(RadioId,const RfProfile&) override {
    assert(false && "RAMSES must never swap 868 RX to pulse mode");
    return false;
  }
  bool stop_pulse_rx(RadioId) override {
    assert(false && "RAMSES must never stop a pulse RX for 868");
    return false;
  }
  PhysicalTxResult transmit_uart(RadioId id,const RfProfile& p,const EncodedTxFrame& f) override {
    assert(id==RadioId::rf86830);
    assert(p.id==RfProfileId::ramses_868_packet);
    assert(f.wire && f.length==4);
    calls.push_back(3);
    return outcome;
  }
};

int main() {
  // The 868.30 MHz 63-byte packet RX is shared by both protocol decoders.
  const auto& shared=rf_profile(RfProfileId::itho_868_packet);
  const auto& ramses_tx=rf_profile(RfProfileId::ramses_868_packet);
  assert(shared.frequency_hz==ramses_tx.frequency_hz);
  assert(shared.rx_transport==RxTransport::packet_fifo);
  assert(shared.packet_length==63);
  assert(profile_has_registers(shared));
  assert(!profile_has_registers(ramses_tx)); // not an alternate RX profile
  assert(ramses_tx.tx_transport==TxTransport::gdo_uart);
  assert(uart_tx_preset_valid(ramses_tx));
  NativeDecoder decoder;
  const auto frame=synthetic_ramses::frame(0x31D9);
  decoder.on_packet(RadioId::rf86830,shared.frequency_hz,frame.data(),frame.size());
  const auto decoded=decoder.snapshot();
  assert(decoded.packet_inputs==1);
  assert(decoded.ramses_messages==1);
  assert(decoded.last_protocol==RecognizedProtocol::ramses_ii);
  // RAMSES transmit temporarily stops shared RX and then restores *that*
  // same physical receive profile, even if UART TX fails and is cleaned up.
  MockTransport driver;
  RadioManager radio(driver);
  radio.set_presence(RadioId::rf86830,true,true);
  assert(radio.start_receive(RadioId::rf86830,RfProfileId::ramses_868_packet)==ManagerError::missing_profile);
  assert(radio.start_receive(RadioId::rf86830,RfProfileId::itho_868_packet)==ManagerError::none);
  const uint8_t wire[]={1,2,3,4};
  const EncodedTxFrame payload{wire,sizeof(wire),1,0};
  assert(radio.transmit_encoded(RadioId::rf86830,RfProfileId::ramses_868_packet,payload)==ManagerError::none);
  assert((driver.calls==std::vector<int>{1,2,3,1}));
  auto after=radio.status(RadioId::rf86830);
  assert(after.operation==RadioOperation::receiving);
  assert(after.profile==RfProfileId::itho_868_packet);
  driver.calls.clear();
  driver.outcome={false,true,TxFailureStage::uart_drain};
  assert(radio.transmit_encoded(RadioId::rf86830,RfProfileId::ramses_868_packet,payload)==ManagerError::tx_failure);
  assert((driver.calls==std::vector<int>{2,3,1}));
  after=radio.status(RadioId::rf86830);
  assert(after.operation==RadioOperation::receiving);
  assert(after.profile==RfProfileId::itho_868_packet);
  puts("PASS: shared 868 RX decodes RAMSES and is restored after UART TX success/failure");
}
"""


class Shared868RxContractTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "g++ host compiler not installed")
    def test_shared_rx_and_ramses_tx_restore(self):
        with tempfile.TemporaryDirectory(prefix="rflink-shared-rx-") as tmp:
            source=Path(tmp)/"shared_868.cpp"
            executable=Path(tmp)/"shared_868"
            source.write_text(TEST_PROGRAM,encoding="utf-8")
            command=[
                "g++","-std=c++17","-O2","-Wall","-Wextra","-Werror","-pthread",
                "-I"+str(HAL/"include"),
                "-I"+str(DECODE/"include"),
                "-I"+str(FW/"tests"),
                str(source),
                str(HAL/"rflink_rf_profiles.cpp"),
                str(HAL/"rflink_radio_manager.cpp"),
                str(DECODE/"src/rflink_native_decoder.cpp"),
                str(DECODE/"src/rflink_user_rx_runtime.cpp"),
                str(DECODE/"src/rflink_user_library.cpp"),
                str(DECODE/"src/rflink_user_vm.cpp"),
                str(DECODE/"src/rflink_itho_codec.cpp"),
                str(DECODE/"src/rflink_ramses_codec.cpp"),
                str(DECODE/"src/rflink_ramses_tx.cpp"),
                "-o",str(executable),
            ]
            subprocess.run(command,check=True,timeout=60,capture_output=True,text=True)
            result=subprocess.run([str(executable)],check=True,timeout=15,capture_output=True,text=True)
            self.assertIn("PASS: shared 868 RX",result.stdout)

    def test_no_ramses_specific_rx_transition_in_firmware(self):
        main=(FW/"main/main.cpp").read_text(encoding="utf-8")
        self.assertIn("start_receive(rflink::RadioId::rf86830,rflink::RfProfileId::itho_868_packet)",main)
        self.assertNotIn("start_receive(rflink::RadioId::rf86830,rflink::RfProfileId::ramses_868_packet)",main)
        rx=(HAL/"rflink_packet_rx.cpp").read_text(encoding="utf-8")
        self.assertIn("rf_profile(RfProfileId::itho_868_packet)",rx)
        decoder=(DECODE/"src/rflink_native_decoder.cpp").read_text(encoding="utf-8")
        self.assertIn("rflink_ramses_ii::decodePacked",decoder)
        self.assertIn("itho::decode_packet",decoder)
        self.assertIn("if(is_ramses && is_itho)",decoder)

if __name__=="__main__":
    unittest.main()
