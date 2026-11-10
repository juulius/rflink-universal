"""Persisted OTA health: host-only C++ journal and sanitized UI contracts.

These tests do not replace a real ESP32 reboot/rollback acceptance test.
"""
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
FW=ROOT/'firmware'
sys.path.insert(0,str(ROOT/'rflink_builder'))
from diagnostic_proxy import _safe_crash_report


class OtaHealthPersistenceTests(unittest.TestCase):
    def test_host_report_is_bounded_versioned_and_corruption_checked(self):
        code=r'''
#include "rflink_ota_health_report.hpp"
#include <cassert>
#include <cstring>
using namespace rflink;
int main() {
  auto pending=ota_health_make(17,"0.7.0-exp.12-n16r8-b5e871b06",
    OtaHealthOutcome::checking,false,false,false,0,0,0);
  assert(ota_health_valid(pending));
  assert(std::strcmp(ota_health_name(OtaHealthOutcome::checking),"checking")==0);
  auto failed=ota_health_make(17,"0.7.0-exp.12-n16r8-b5e871b06",
    OtaHealthOutcome::failed,true,true,true,1234,0,3);
  assert(ota_health_valid(failed));
  // Simulated raw NVS blob survives a firmware-partition switch.
  char saved[sizeof(failed)];
  std::memcpy(saved,&failed,sizeof(failed));
  OtaHealthReport after_rollback{};
  std::memcpy(&after_rollback,saved,sizeof(saved));
  assert(ota_health_valid(after_rollback));
  assert(after_rollback.active_receivers==0 && after_rollback.responsive_radios==3);
  assert(after_rollback.cycles==1234);
  after_rollback.cycles++;
  assert(!ota_health_valid(after_rollback));
  failed.magic=0;
  assert(!ota_health_valid(failed));
  auto cleaned=ota_health_make(1,"v<script>password",
    OtaHealthOutcome::valid,true,true,true,110,1,1);
  assert(ota_health_valid(cleaned));
  assert(!std::strchr(cleaned.firmware,'<'));
  assert(!std::strchr(cleaned.firmware,'>'));
  auto invalid=ota_health_make(0,"test",OtaHealthOutcome::valid,
    true,true,true,100,1,1);
  assert(!ota_health_valid(invalid));
  return 0;
}
'''
        with tempfile.TemporaryDirectory() as d:
            file=Path(d)/'ota.cpp'
            binary=Path(d)/'ota'
            file.write_text(code)
            compile=subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror',
                '-I'+str(FW/'main'),str(file),'-o',str(binary)],
                capture_output=True,text=True,timeout=35)
            self.assertEqual(compile.returncode,0,compile.stderr)
            run=subprocess.run([str(binary)],capture_output=True,text=True,timeout=15)
            self.assertEqual(run.returncode,0,run.stderr)

    def test_builder_only_allows_known_ota_fields_and_exact_types(self):
        health={'schema':1,'boot':17,'firmware':'0.7.0-exp.12-n16r8-b5e871b06',
                'outcome':'failed','spi_ok':True,'web_ok':True,'worker_ok':True,
                'cycles':1142,'active_receivers':0,'responsive_radios':3,
                'password':'DO_NOT_EXPOSE','stack':'SECRET'}
        safe=_safe_crash_report({'ota_health':health})['ota_health']
        self.assertEqual(safe['outcome'],'failed')
        self.assertEqual(safe['active_receivers'],0)
        self.assertNotIn('DO_NOT_EXPOSE',str(safe))
        for altered in ({'outcome':'<script>'},{'active_receivers':True},
                        {'cycles':-1},{'spi_ok':'yes'},{'firmware':'PASSWORD/123'},
                        {'boot':False}):
            value=dict(health,**altered)
            self.assertIsNone(_safe_crash_report({'ota_health':value})['ota_health'])
        self.assertIsNone(_safe_crash_report({})['ota_health'])

    def test_firmware_commits_final_report_before_rollback(self):
        main=(FW/'main/main.cpp').read_text()
        crash=(FW/'main/rflink_crash_report.cpp').read_text()
        # exp.12 put this partition walk immediately after crash logging,
        # causing a hardware LoadProhibited in esp_partition_next().
        # Preserve pending-OTA evidence but only after essential startup.
        self.assertLess(main.index('rflink_start_web_ota()'),
                        main.index('esp_ota_get_state_partition(running,&state)'))
        self.assertLess(main.index('bus.begin()'),
                        main.index('esp_ota_get_state_partition(running,&state)'))
        self.assertLess(main.index('rflink_publish_radio_providers()'),
                        main.index('esp_ota_get_state_partition(running,&state)'))
        self.assertLess(main.index('esp_ota_get_state_partition(running,&state)'),
                        main.index('OtaHealthOutcome::checking'))
        self.assertLess(main.index('OtaHealthOutcome::checking'),
                        main.index('vTaskDelay(pdMS_TO_TICKS(10000))'))
        self.assertLess(main.index('OtaHealthOutcome::failed'),
                        main.index('esp_ota_mark_app_invalid_rollback_and_reboot'))
        self.assertIn('OtaHealthOutcome::validation_error',main)
        self.assertIn('OtaHealthOutcome::valid',main)
        self.assertIn('nvs_set_blob(handle,kOtaHealthKey',crash)
        self.assertIn('nvs_commit(handle)',crash)
        self.assertIn('ota_health_valid(saved_ota)',crash)
        self.assertIn('ota_health',crash)
        self.assertNotIn('nvs_erase_all(',crash)

    def test_ota_dialog_reads_boot_report_independent_of_console(self):
        js=(ROOT/'rflink_builder/static/rf_flash.js').read_text()
        self.assertIn('rfFlashHealthSummary(health)',js)
        self.assertIn("api/diagnostics/device/crashes?device_id=",js)
        self.assertIn('Checking durable boot history separately',js)
        self.assertIn("health.firmware===rfFlashTargetVersion",js)
        self.assertIn('savedHealth.boot===latestBoot',js)
        self.assertIn('savedHealth.boot+1===latestBoot',js)
        self.assertIn('rfFlashRollbackSeen',js)
        self.assertIn('Exact rollback cause unavailable',js)
        self.assertIn('firmware requested rollback',js)


if __name__=='__main__':
    unittest.main()
