"""Regression: compile the real web translation unit to catch broken JSON ostream chaining."""
from pathlib import Path
import subprocess
root=Path(__file__).resolve().parents[1]
component=root/'components/rflink_universal'
cpp=component/'rflink_universal.cpp'
s=cpp.read_text()
assert '"profile_suggestions"' not in s or 'profile_suggestions' in s
assert '}))\n    out <<' not in s
assert 'out << "]";\n        <<' not in s
subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror','-DUSE_RFLINK_UNIVERSAL_WEB', '-fsyntax-only', '-I',str(root/'tests/stubs'), '-I',str(component), str(cpp)],check=True)
print('PROFILEFLOW1 COMPILEFIX1: web firmware C++ syntax-only PASS')
