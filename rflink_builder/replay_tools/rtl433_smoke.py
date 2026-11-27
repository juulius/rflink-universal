"""Build-time check of actual rtl_433 pulse-file input, not hardware evidence."""
from rf_rtl433 import compare

# Synthetic WH2 fixture already used in firmware/tests/test_native_decode.cpp.
# This is an invented weather sensor ID and values, not a household capture.
bits="".join(f"{b:08b}" for b in bytes.fromhex("FF45A12C37A5"))
pulses=[duration for bit in bits for duration in (1500 if bit=="1" else 500,-1000)]
# Synthetic fixture only: a measured-frame delimiter is modeled explicitly.
pulses[-1]=-12000
cap=dict(radio=1,format="signed_pulses_us",capture_phase="pre_filter_rmt",
         frequency_hz=433920000,capture_id="synthetic_wh2",pulses_us=pulses)
rows=[{}]
report=compare([cap],rows)
assert report["state"]=="complete", report["summary"]
assert report["summary"].get("tested")==1, report["summary"]
assert any(g["model"]=="Fineoffset-WH2" and "CRC" in g["integrity"]
           for g in report["models"]), report["models"]
print("rtl_433 packaged OOK smoke check passed")
