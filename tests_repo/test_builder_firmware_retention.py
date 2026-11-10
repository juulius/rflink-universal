"""Disk retention and safety tests; no Home Assistant or ESP-IDF required."""
from __future__ import annotations

import json
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "rflink_builder"))
from firmware_retention import FirmwareRetention


class FirmwareRetentionTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.data = Path(self.tmp.name)
        self.manager = FirmwareRetention(self.data)
        self.data.joinpath("toolchain").mkdir()
        self.data.joinpath("toolchain", "idf.py").write_text("KEEP")
        self.data.joinpath("project").mkdir()
        self.data.joinpath("project", "secret.txt").write_text("KEEP")

    def add(self, job, device, timestamp=None, size=1024):
        folder = self.data / "releases" / job
        folder.mkdir(parents=True)
        (folder / "firmware.bin").write_bytes(b"A" * size)
        (folder / "RFLink_Radioactive_USB_OTA.zip").write_bytes(b"B" * 100)
        (folder / "build_info.json").write_text(json.dumps({
            "job_id": job, "device_id": device, "created_at": timestamp or job,
        }))
        return folder

    def assert_other_data_untouched(self):
        self.assertEqual((self.data / "toolchain" / "idf.py").read_text(), "KEEP")
        self.assertEqual((self.data / "project" / "secret.txt").read_text(), "KEEP")

    def test_many_devices_and_global_builds_never_mix(self):
        g1=self.add("000000000001", None)
        g2=self.add("000000000002", None)
        a1=self.add("000000000003", "rf-a")
        b1=self.add("000000000004", "rf-b")
        a2=self.add("000000000005", "rf-a")
        a3=self.add("000000000006", "rf-a")
        # Automatic maintenance can prune global builds, but it does not
        # guess which USB firmware was installed on an unverified board.
        result=self.manager.cleanup(active_job=a3.name)
        self.assertEqual(result["removed_count"], 1)
        self.assertFalse(g1.exists())
        for folder in (g2,a1,b1,a2,a3):
            self.assertTrue(folder.exists())
        self.assert_other_data_untouched()
        # Operator explicitly removes old unverified builds, keeping the
        # most recent two per device as a safety fallback.
        self.assertEqual(self.manager.report(active_job=a3.name)["reclaimable_count"], 1)
        result=self.manager.cleanup(active_job=a3.name, manual=True)
        self.assertEqual(result["removed_count"], 1)
        self.assertFalse(a1.exists())
        for folder in (g2,b1,a2,a3):
            self.assertTrue(folder.exists())
        self.assertEqual(result["storage"]["saved_count"], 4)

    def test_verified_ota_replaces_previous_device_and_reclaims_old(self):
        old=self.add("000000000001", "rf-a")
        middle=self.add("000000000002", "rf-a")
        new=self.add("000000000003", "rf-a")
        other=self.add("000000000004", "rf-b")
        self.manager.mark_ota_verified("rf-a", middle.name)
        # During a pending build old verified must survive, latest stays.
        report=self.manager.report(active_job=new.name)
        self.assertEqual(report["reclaimable_count"], 1)
        self.manager.cleanup(active_job=new.name)
        self.assertFalse(old.exists())
        self.assertTrue(middle.exists())
        self.assertTrue(new.exists())
        self.assertTrue(other.exists())
        # Physical OTA confirmation explicitly advances protected package.
        self.manager.mark_ota_verified("rf-a", new.name)
        result=self.manager.cleanup(active_job=new.name)
        self.assertEqual(result["removed_count"], 1)
        self.assertFalse(middle.exists())
        self.assertTrue(new.exists())
        self.assertTrue(other.exists())
        self.assertEqual(result["storage"]["saved_count"], 2)
        self.assertTrue(self.manager.report()["devices"][0]["ota_verified"])

    def test_usb_flash_does_not_automatically_delete_only_fallback(self):
        installed=self.add("000000000001", "rf-a")
        pending=self.add("000000000002", "rf-a")
        self.assertEqual(self.manager.cleanup(active_job=pending.name)["removed_count"], 0)
        self.assertTrue(installed.exists())
        self.assertTrue(pending.exists())
        self.assertEqual(self.manager.report()["saved_count"], 2)

    def test_never_deletes_current_active_build_even_when_older(self):
        old=self.add("000000000001", "rf-a")
        mid=self.add("000000000002", "rf-a")
        newest=self.add("000000000003", "rf-a")
        self.manager.mark_ota_verified("rf-a", newest.name)
        result=self.manager.cleanup(active_job=old.name)
        self.assertEqual(result["removed_count"], 1)
        self.assertTrue(old.exists())
        self.assertFalse(mid.exists())
        self.assertTrue(newest.exists())

    def test_unknown_and_symlinked_folders_cannot_be_pruned(self):
        first=self.add("000000000001", None)
        second=self.add("000000000002", None)
        external=self.data / "outside"
        external.mkdir()
        (external / "secret").write_text("SAFE")
        rel=self.data / "releases"
        (rel / "000000000003").symlink_to(external, target_is_directory=True)
        invalid=rel / "000000000004"
        invalid.mkdir()
        (invalid / "build_info.json").write_text('{"job_id":"../evil"}')
        nested=rel / "000000000005"
        nested.mkdir()
        (nested / "build_info.json").write_text('{"job_id":"000000000005","device_id":null}')
        (nested / "link").symlink_to(external, target_is_directory=True)
        report=self.manager.report()
        self.assertEqual(report["saved_count"],2)
        self.assertEqual(self.manager.cleanup(manual=True)["removed_count"],1)
        self.assertFalse(first.exists())
        self.assertTrue(second.exists())
        self.assertEqual((external / "secret").read_text(),"SAFE")
        self.assertTrue(invalid.exists())
        self.assertTrue(nested.exists())

    def test_confirmed_identity_must_match_folder_and_owner(self):
        other=self.add("000000000001","rf-a")
        with self.assertRaises(ValueError):
            self.manager.mark_ota_verified("rf-b",other.name)
        with self.assertRaises(ValueError):
            self.manager.mark_ota_verified("rf-a","../etc/passwd")
        self.manager.mark_ota_verified("rf-a",other.name)
        self.assertTrue(self.manager.state.exists())
        self.assertIn("ota_verified",self.manager.state.read_text())
        self.manager.forget_device("rf-a")
        self.assertEqual(self.manager._confirmed(),{})

    def test_all_unverified_legacy_packages_are_protected_automatically(self):
        for n in range(30):
            self.add(f"{n:012x}", "old-usb-board")
        self.assertEqual(self.manager.cleanup()["removed_count"],0)
        report=self.manager.report()
        self.assertEqual(report["reclaimable_count"],28)
        outcome=self.manager.cleanup(manual=True)
        self.assertEqual(outcome["removed_count"],28)
        self.assertEqual(outcome["storage"]["saved_count"],2)
        self.assert_other_data_untouched()

    def test_stats_sizes_and_busy_flag(self):
        a=self.add("000000000001",None,size=2048)
        b=self.add("000000000002",None,size=4096)
        data=self.manager.report(active_job=b.name,busy=True)
        self.assertEqual(data["saved_count"],2)
        self.assertGreater(data["saved_bytes"],6144)
        self.assertEqual(data["reclaimable_count"],1)
        self.assertTrue(data["busy"])
        expected_bytes=sum(p.stat().st_size for p in a.iterdir())
        self.assertEqual(self.manager.cleanup(active_job=b.name)["freed_bytes"],expected_bytes)

    def test_config_excludes_compiled_private_packages_from_ha_backup(self):
        config=(ROOT / "rflink_builder" / "config.yaml").read_text()
        self.assertIn("- releases/**", config)
        self.assertIn("- toolchain/**", config)
        self.assertIn("- project/build/**",config)
        page=(ROOT/"rflink_builder"/"static"/"index.html").read_text()
        js=(ROOT/"rflink_builder"/"static"/"storage_diag.js").read_text()
        server=(ROOT/"rflink_builder"/"server.py").read_text()
        self.assertIn('id="storage-clean"',page)
        self.assertIn('storage_diag.js',page)
        self.assertIn("confirm:'CLEAN BUILDS'",js)
        self.assertIn("'/api/storage/cleanup'",server)
        self.assertIn("firmware_retention.mark_ota_verified",server)
        self.assertIn("if kind == 'ota' and self.verified",server)
        self.assertIn("'/storage_diag.js'",server)


if __name__=="__main__":
    unittest.main()
