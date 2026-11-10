"""Stage 26: exact crash-image provenance is a diagnostics gate, not RF validation."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
FW = ROOT / "firmware"


class CrashIdentityTests(unittest.TestCase):
    def test_elf_identity_prefix_and_malformed_flash_data(self):
        source = r"""
#include "rflink_crash_identity.hpp"
#include <cassert>
#include <cstring>
#include <cstdint>
using namespace rflink;
int main() {
  char running[65]={},dump[65]={};
  const uint8_t full[]="0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
  const uint8_t same[]="0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF";
  const uint8_t short_hash[]="0123456789abcdef";
  const uint8_t other[]="0123456789abcdee";
  const uint8_t invalid[]="0123456789abcde\\n";
  const uint8_t missing_null[]={'0','1','2','3','4','5','6','7','8'};
  assert(copy_elf_sha(full,sizeof(full),running));
  assert(std::strlen(running)==64);
  assert(copy_elf_sha(same,sizeof(same),dump));
  assert(compare_elf_sha(running,dump)==ElfBuildMatch::same_full_sha);
  assert(copy_elf_sha(short_hash,sizeof(short_hash),dump));
  assert(compare_elf_sha(running,dump)==ElfBuildMatch::matching_prefix);
  assert(copy_elf_sha(other,sizeof(other),dump));
  assert(compare_elf_sha(running,dump)==ElfBuildMatch::different);
  assert(!copy_elf_sha(invalid,sizeof(invalid),dump));
  assert(dump[0]==0);
  assert(compare_elf_sha(running,dump)==ElfBuildMatch::unknown);
  assert(!copy_elf_sha(missing_null,sizeof(missing_null),dump));
  assert(!copy_elf_sha(nullptr,0,dump));
  assert(compare_elf_sha(nullptr,running)==ElfBuildMatch::unknown);
  assert(std::strcmp(elf_match_label(ElfBuildMatch::different),"different")==0);
  assert(std::strcmp(elf_match_label(ElfBuildMatch::matching_prefix),"matching_prefix")==0);
  assert(std::strcmp(elf_match_label(ElfBuildMatch::same_full_sha),"same_full_sha")==0);
  return 0;
}
"""
        with tempfile.TemporaryDirectory(prefix="rflink-crash-identity-") as folder:
            path = Path(folder)
            cpp = path / "test.cpp"
            exe = path / "test"
            cpp.write_text(source)
            cmd = ["g++", "-std=c++17", "-Wall", "-Wextra", "-Werror",
                   "-I" + str(FW / "main"), str(cpp), "-o", str(exe)]
            compiled = subprocess.run(cmd, capture_output=True, text=True, timeout=40)
            self.assertEqual(compiled.returncode, 0, compiled.stderr)
            tested = subprocess.run([str(exe)], capture_output=True,
                                    text=True, timeout=15)
            self.assertEqual(tested.returncode, 0, tested.stderr)

    def test_sanitized_crash_provenance_is_exposed_separately_from_current_version(self):
        crash = (FW / "main/rflink_crash_report.cpp").read_text()
        web = (FW / "main/rflink_web_ota.cpp").read_text()
        self.assertIn("esp_app_get_elf_sha256", crash)
        self.assertIn("summary.app_elf_sha256", crash)
        self.assertIn("snapshot.dump_integrity_error = esp_core_dump_image_check()", crash)
        for field in ('running_firmware', 'elf_sha256', 'build_comparison',
                      'integrity', 'invalid_or_unreadable'):
            self.assertIn(field, crash)
        page=(FW / "main/webui/index.html").read_text()
        script=(FW / "main/webui/app.js").read_text()
        self.assertIn('id="diag-compare"', page)
        self.assertIn('id="diag-crash-pc"', page)
        self.assertIn('"/api/diagnostics/crashes"', web)
        self.assertIn("dump.build_comparison", script)
        self.assertIn("dump.summary_available", script)
        self.assertNotIn("esp_core_dump_image_get", web)
        self.assertNotIn("esp_core_dump_image_erase", web)


if __name__ == "__main__":
    unittest.main()
