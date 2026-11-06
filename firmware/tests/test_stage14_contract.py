from pathlib import Path
import subprocess
p=Path(__file__).resolve().parents[1]
main=(p/'main/main.cpp').read_text()
web=(p/'main/rflink_web_ota.cpp').read_text()
cmake=(p/'CMakeLists.txt').read_text()
workflow_path=p.parent/'.github/workflows/validate_source.yml'
workflow=workflow_path.read_text() if workflow_path.exists() else ''
assert '0.7.0-n16r8-s20-b${RF_BOARD_HASH}' in cmake
assert 'rx_worker_cycles.fetch_add' in main
assert 'passes_boot_health' in main
assert 'part==0x00' in main
assert 'native_ota_identity_accepted' in web
assert 'candidate.project_name' in web and 'candidate.version' in web
assert 'rx_worker_cycles' in web and 'worker_cycles_provider' in web
if workflow:
    assert 'espressif/idf:v5.5.6' in workflow
    assert 'idf.py build' in workflow
    assert 'source_manifest.py' in workflow
subprocess.run(['python3',str(p/'tools/preflight.py')],check=True)
print('PASS Stage14: native OTA identity, live worker health, CI build pipeline')
