"""Verify that the original v0.6.6 bounded user interpreter runs natively.

This is a host regression of the reusable recipe engine, NOT a test of
NVS persistence, capture/learn UI or authorized on-air transmission.
"""
import pathlib
import shutil
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
INC = ROOT / 'firmware/components/rflink_decoder_core/include'
SRC = ROOT / 'firmware/components/rflink_decoder_core/src/rflink_user_vm.cpp'

TEST_CPP = r'''
#include "rflink_user_vm.hpp"
#include <cassert>
#include <map>
#include <string>
#include <vector>
using namespace rflink;
using namespace rflink::user_vm;

int main() {
  std::string error;
  Program rx{};
  // Backslash escapes in this C++ literal become real LF characters.
  const std::string actual =
      "LEN\nCONST 4\nEQ\nREQUIRE\nMATCHRAW 0 500\nREQUIRE\n"
      "CONST 123\nID\nACTION on\nACCEPT\n";
  assert(compile(actual, Mode::RX, rx, error));
  std::vector<int32_t> pulses{505,-400,505,-400};
  RFFrame frame{};
  frame.kind = FrameKind::RAW; frame.channel = 1; frame.frequency_hz = 433920000;
  frame.raw = &pulses;
  DecodeResult output{};
  assert(receive(rx, "user_switch", frame, output, error));
  assert(output.valid && output.identity_key == "7B" && output.action == "on");
  pulses[0] = 980;
  assert(!receive(rx, "user_switch", frame, output, error));
  assert(!output.valid);
  assert(!compile("MATCHRAW 512 500\nACCEPT\n", Mode::RX, rx, error));
  assert(!compile("LEN\nSEND\n", Mode::RX, rx, error));
  assert(!compile("MARKUS 350\nACCEPT\n", Mode::TX, rx, error));
  assert(!compile(std::string(4097, 'a'), Mode::RX, rx, error));

  // Interpreter steps are bounded even if an advanced rule loops forever.
  assert(compile("JMP 0\nACCEPT\n", Mode::RX, rx, error));
  assert(!receive(rx, "user_loop", frame, output, error));
  assert(error == "instruction limit exceeded");

  Program packet_rx{};
  assert(compile("MATCHBYTE 0 165\nREQUIRE\nCONST 7\nID\nACTION pressed\nACCEPT\n",
                 Mode::RX, packet_rx, error));
  std::vector<uint8_t> bytes(63,0);
  bytes[0]=165;
  RFFrame fsk{};
  fsk.kind=FrameKind::PACKET; fsk.channel=2; fsk.frequency_hz=868300000;
  fsk.packet=&bytes;
  assert(receive(packet_rx,"user_packet",fsk,output,error));
  assert(output.action=="pressed" && output.identity_key=="7");

  // Preview of fixed-code TX constructs a frame but does NOT send anything.
  Program fixed_tx{};
  assert(compile("MARKUS 400\nSPACEUS 800\nMARKUS 400\nSPACEUS 800\nREPEATS 3\nSEND\n",
                 Mode::TX,fixed_tx,error));
  TxFrame planned{};
  const std::map<std::string,uint32_t> counters{};
  assert(transmit(fixed_tx,"on",1,counters,planned,error));
  assert(planned.radio==1 && planned.repeats==3 && planned.raw.size()==4);
  assert(planned.raw[0]==400 && planned.raw[1]==-800);

  Program counter_tx{};
  assert(compile("BUMP rolling\nDROP\nBYTEVAL 165\nSEND\n",Mode::TX,counter_tx,error));
  const std::map<std::string,uint32_t> old_state{{"rolling",5}};
  assert(transmit(counter_tx,"on",2,old_state,planned,error));
  assert(planned.next_state.at("rolling")==6);
  assert(old_state.at("rolling")==5);
  assert(planned.packet.size()==1 && planned.packet[0]==165);
  assert(!transmit(counter_tx,"on",2,{{"rolling",UINT32_MAX}},planned,error));

  assert(identifier("user_sensor_1"));
  assert(!identifier("User sensor"));
  return 0;
}
'''

class NativeUserVMPortTests(unittest.TestCase):
    def test_no_esphome_dependency_and_native_component_registration(self):
        contents = SRC.read_text()
        header = (INC / 'rflink_user_vm.hpp').read_text()
        cmake = (ROOT / 'firmware/components/rflink_decoder_core/CMakeLists.txt').read_text()
        self.assertNotIn('namespace esphome', contents)
        self.assertNotIn('#include "user_vm.h"', contents)
        self.assertIn('src/rflink_user_vm.cpp', cmake)
        for contract in ('MAX_SOURCE = 4096', 'MAX_INSTRUCTIONS = 192', 'next_state'):
            self.assertIn(contract, header)
        for guard in ('MAX_STEPS = 2048', 'MAX_STACK = 32', 'tx->raw.size() >= 512',
                      'tx->packet.size() >= 64', 'result.transmit_supported = false'):
            self.assertIn(guard, contents)

    @unittest.skipUnless(shutil.which('g++'), 'g++ required')
    def test_actual_native_vm_build_and_execute(self):
        with tempfile.TemporaryDirectory() as folder:
            file = pathlib.Path(folder) / 'test.cpp'
            exe = pathlib.Path(folder) / 'vm-test'
            file.write_text(TEST_CPP)
            build = subprocess.run(
                ['g++','-std=c++17','-Wall','-Wextra','-Werror',
                 '-I'+str(INC),str(file),str(SRC),'-o',str(exe)],
                capture_output=True,text=True,timeout=45)
            self.assertEqual(build.returncode,0,build.stderr)
            run = subprocess.run([str(exe)],capture_output=True,text=True,timeout=15)
            self.assertEqual(run.returncode,0,run.stderr)


if __name__=='__main__':
    unittest.main()
