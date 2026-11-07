"""Offline checks for first ESP32-S3 v0.7 experimental build identity."""
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'firmware' / 'tools'))
from package_release import firmware_identity
from source_manifest import verify


class ExperimentalIdentityTests(unittest.TestCase):
    def test_experimental_and_rc_identity_stays_in_esp_app_desc(self):
        for tag, channel, build, version in [
            ('firmware/v0.7.0-exp.1', 'experimental', 1,
             '0.7.0-exp.1-n16r8-b9d3226e6'),
            ('firmware/v0.7.0-exp.20', 'experimental', 20,
             '0.7.0-exp.20-n16r8-b9d3226e6'),
            ('firmware/v0.7.0-rc.3', 'pre-release', 3,
             '0.7.0-rc.3-n16r8-b9d3226e6'),
            ('firmware/v0.7.0', 'stable', 0,
             '0.7.0-n16r8-b9d3226e6'),
        ]:
            with self.subTest(tag=tag):
                value = firmware_identity(tag, '9d3226e6')
                self.assertEqual(value['version'], version)
                self.assertEqual(value['channel'], channel)
                self.assertEqual(value['build_number'], build)
                self.assertLessEqual(len(value['version']), 31)

    def test_local_build_is_explicitly_development(self):
        value = firmware_identity('', '9d3226e6')
        self.assertIsNone(value['release_tag'])
        self.assertEqual(value['channel'], 'development')
        self.assertEqual(value['version'], '0.7.0-n16r8-s20-b9d3226e6')

    def test_invalid_or_overlong_tags_are_rejected(self):
        for tag in ('main', 'firmware/v0.7.0-exp.0', 'firmware/v0.7.0-exp.01',
                    'firmware/v0.7.1-exp.1', 'firmware/v0.7.0-rc.0',
                    'firmware/v0.7.0-exp.' + '9' * 30):
            with self.subTest(tag=tag), self.assertRaises(ValueError):
                firmware_identity(tag, '9d3226e6')

    def test_cmake_and_builder_match_package_identity(self):
        cmake = (ROOT / 'firmware/CMakeLists.txt').read_text()
        builder = (ROOT / 'rflink_builder/server.py').read_text()
        web = (ROOT / 'firmware/main/rflink_web_ota.cpp').read_text()
        self.assertIn('RFLINK_RELEASE_TAG', cmake)
        self.assertIn('0.7.0-exp.', cmake)
        self.assertIn('0.7.0-rc.', cmake)
        self.assertIn("env['RFLINK_RELEASE_TAG'] = pinned['tag']", builder)
        self.assertIn("built.get('release_tag') != pinned['tag']", builder)
        self.assertIn('release_channel', web)
        self.assertIn('firmware_display_version', web)

    def test_firmware_source_manifest_is_current(self):
        self.assertIn('source_fingerprint', verify())


if __name__ == '__main__':
    unittest.main()
