"""Portable launcher may only use a locally bundled SDK, never Docker/install/network."""
from pathlib import Path
r=Path(__file__).resolve().parents[1]
a=(r/'Maak_release.bat').read_text(encoding='ascii').lower()
for snippet in ['portable_sdk', 'idf_tools_path', 'idf_path', 'python\\python.exe', 'git\\cmd\\git.exe', 'export.bat', 'where ninja', 'where cmake', 'where xtensa-esp32s3-elf-gcc', 'build_offline.bat', 'pause', 'exit /b 1']:
    assert snippet in a, snippet
for forbidden in ['docker.exe', 'docker pull', 'docker run', 'where docker', 'winget', 'choco ', 'pip install', 'idf_tools.py install', 'curl ', 'powershell', 'wget', 'docker pull', 'idf.py flash', 'esptool.py write_flash']:
    assert forbidden not in a, forbidden
assert not (r/'BUILD_DOCKER_OFFLINE.bat').exists()
assert not (r/'BUILD_DOCKER_OFFLINE.sh').exists()
assert 'portable_sdk' in (r/'tools/source_manifest.py').read_text()
assert 'Home Assistant' in (r/'README.md').read_text()
print('PASS: optional native portable launcher, no Docker/install/download/flash; main workflow is Home Assistant Builder')
