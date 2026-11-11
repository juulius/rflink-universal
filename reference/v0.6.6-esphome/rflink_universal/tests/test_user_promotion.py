"""Host integration: exported user recipes become reviewed firmware recipes."""
import json
import subprocess
import sys
import tempfile
from pathlib import Path

root = Path(__file__).resolve().parents[1]
script = root / "tools" / "promote_user_library.py"

decoder = {
    "id": "user_host_fixture", "name": "Host fixture", "radio": 0,
    "kind": "raw", "source": "user", "enabled": True, "version": 1,
    "actions": ["on"],
    "rx_source": "LEN\nCONST 4\nEQ\nREQUIRE\nMATCHRAW 0 500\nREQUIRE\nCONST 42\nID\nACTION on\nACCEPT\n",
    "tx_source": "ACTEQ on\nREQUIRE\nCONST 500\nMARK\nCONST 1000\nSPACE\nCONST 500\nMARK\nCONST 1000\nSPACE\nSEND\n",
    "samples": [{"label": "on", "channel": 0, "kind": "raw", "raw": [500 + n, -1000, 500, -1000], "packet": []}
                for n in range(3)],
}
profile = {"id": "host_remote", "name": "Test remote", "protocol": "user_host_fixture",
           "expected_actions": ["on"], "learn_help": "Press the button three times", "source": "user"}
builtin_profile = {"id": "host_nexa", "name": "Nexa presentation", "protocol": "Nexa",
                   "expected_actions": ["on", "off"], "learn_help": "Press on and off", "source": "user"}


def run(path, folder, *args, successful=True):
    result = subprocess.run([sys.executable, str(script), str(path), *args,
                             "--out-dir", str(folder)], capture_output=True, text=True)
    if successful and result.returncode != 0:
        raise AssertionError(result.stderr + result.stdout)
    if not successful and result.returncode == 0:
        raise AssertionError("invalid promotion unexpectedly passed")
    return result


with tempfile.TemporaryDirectory() as temp:
    folder = Path(temp)
    path = folder / "backup.json"
    backup = {"format": "rflink-user-library", "schema": 1,
              "library": {"schema": 1, "decoders": [decoder], "profiles": [profile, builtin_profile]},
              "devices": []}
    path.write_text(json.dumps(backup))
    candidates = folder / "candidates"
    run(path, folder, "--check", "--candidate-out", str(candidates))
    assert not (folder / "firmware_user_recipes.h").exists()
    candidate = candidates / "candidate_user_host_fixture"
    assert (candidate / "decoder.cpp").exists() and (candidate / "manifest.json").exists()
    generated_manifest = json.loads((candidate / "manifest.json").read_text())
    assert generated_manifest["enabled"] is False
    assert generated_manifest["capabilities"][0]["transmit"]["status"] == "disabled"
    run(path, folder, "--merge")
    header = (folder / "firmware_user_recipes.h").read_text()
    assert "RF_RECIPE user_host_fixture" in header and "sample.raw" in header
    assert 'profile.protocol = "Nexa"' in header
    run(path, folder, "--merge")  # idempotent
    backup["library"]["decoders"][0]["tx_source"] = backup["library"]["decoders"][0]["tx_source"].replace("CONST 500", "CONST 501", 1)
    path.write_text(json.dumps(backup))
    run(path, folder, "--merge", successful=False)
    assert (folder / "firmware_user_recipes.h").read_text() == header
    run(path, folder, "--merge", "--replace", "user_host_fixture")
    backup["library"]["profiles"][1]["expected_actions"] = ["definitely_invalid"]
    path.write_text(json.dumps(backup))
    run(path, folder, "--check", successful=False)
    backup["library"]["profiles"][1]["expected_actions"] = ["on", "off"]
    backup["library"]["decoders"][0]["rx_source"] = "ACTION on\nACCEPT\n"
    path.write_text(json.dumps(backup))
    run(path, folder, "--check", successful=False)  # catches invalid RX lacking ID

print("User promotion: check, host replay, merge, idempotence, conflict and rejection PASS")
