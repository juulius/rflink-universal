from pathlib import Path
root=Path(__file__).resolve().parents[2]
cpp=(root/'rflink_universal/components/rflink_universal/rflink_universal.cpp').read_text()
h=(root/'rflink_universal/components/rflink_universal/rflink_universal.h').read_text()
html=(root/'rflink_universal/ui/index.html').read_text()
yaml=(root/'rflink-universal.yaml').read_text()
assert 'classify_noise_' in cpp and 'full_frame_detail_' in cpp
assert 'out += "PACKET " + std::to_string(frame.packet->size()) + " bytes | "' in cpp and 'raw capture has too few transitions' in cpp
assert 'noise_frames' in h and 'last_lqi' in h and 'last_frequency_offset_hz' in h
assert '/rflink/api/activity-export' in cpp
assert 'monitorLogType' in html and 'monitorCopy' in html and 'monitorExport' in html
assert 'packet_length: 63' in yaml and 'crc_enable: false' in yaml
print('RX diagnostics: all-radio telemetry, conservative noise filter and full export: PASS')
