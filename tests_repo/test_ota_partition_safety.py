"""A physical ESP32 OTA slot must not be its currently executing app.

Host simulations and source contracts; hardware/flash state remains unverified.
"""
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch
import importlib.util
import os

ROOT=Path(__file__).resolve().parents[1]


class OtaPartitionSafetyTests(unittest.TestCase):
    def test_dual_slot_guard_never_returns_running_partition(self):
        code=r'''
#include "rflink_ota_slot_guard.hpp"
#include <cassert>
#include <cstring>
using namespace rflink;
int main(){
  constexpr OtaSlotDescriptor zero{true,16,0x20000,0x400000};
  constexpr OtaSlotDescriptor one{true,17,0x420000,0x400000};
  constexpr OtaSlotDescriptor absent{};
  constexpr auto a=choose_native_ota_slot(zero,zero,one);
  constexpr auto b=choose_native_ota_slot(one,zero,one);
  static_assert(a.safe && a.target_index==1);
  static_assert(b.safe && b.target_index==0);
  assert(!choose_native_ota_slot(zero,zero,absent).safe);
  assert(!choose_native_ota_slot(one,absent,one).safe);
  assert(!choose_native_ota_slot(absent,zero,one).safe);
  assert(!choose_native_ota_slot({true,16,0x25000,0x400000},zero,one).safe);
  assert(!choose_native_ota_slot(zero,zero,{true,17,0x20000,0x400000}).safe);
  assert(!choose_native_ota_slot(zero,zero,{true,17,0x420000,0x200000}).safe);
  assert(!choose_native_ota_slot({true,0,0x10000,0x20000},zero,one).safe);
  assert(std::strcmp(choose_native_ota_slot(zero,zero,absent).reason,
                     "missing_second_ota_slot")==0);
}
'''
        with tempfile.TemporaryDirectory() as directory:
            src=Path(directory)/'test.cpp';out=Path(directory)/'test'
            src.write_text(code)
            build=subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror',
                '-I'+str(ROOT/'firmware/main'),str(src),'-o',str(out)],
                text=True,capture_output=True,timeout=35)
            self.assertEqual(build.returncode,0,build.stderr)
            ran=subprocess.run([str(out)],capture_output=True,text=True,timeout=15)
            self.assertEqual(ran.returncode,0,ran.stderr)

    def test_native_ota_rejects_unsafe_partition_before_begin_or_flash(self):
        src=(ROOT/'firmware/main/rflink_web_ota.cpp').read_text()
        guard=src[src.index('static esp_err_t ota_post('):]
        self.assertIn('inspect_ota_layout()',guard)
        self.assertLess(guard.index('if(!ota_layout.plan.safe'),
                        guard.index('esp_ota_begin(next,'))
        self.assertIn('const esp_partition_t* next=ota_layout.target;',guard)
        self.assertNotIn('esp_ota_get_next_update_partition(nullptr)',guard)
        self.assertIn('ota_busy=false;',guard)
        self.assertIn('ota_running_address',src)
        self.assertIn('ota_target_address',src)
        self.assertIn('ota_layout_issue',src)

    def test_builder_classifies_existing_500_conflict(self):
        server=(ROOT/'rflink_builder/server.py').read_text()
        self.assertIn('ESP_ERR_OTA_PARTITION_CONFLICT',server)
        self.assertIn("status.get('ota_update_safe') is False",server)
        self.assertIn('OTA blocked before upload',server)
        self.assertIn('Live flash slots:',server)
        self.assertIn('The previous firmware is still running.',server)


if __name__=='__main__':
    unittest.main()
