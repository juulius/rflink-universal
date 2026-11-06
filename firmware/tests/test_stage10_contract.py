from pathlib import Path
import re, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
cm=(root/'main/CMakeLists.txt').read_text()
assert 'rflink_decoder_core' in cm
core=(root/'components/rflink_decoder_core/src/rflink_native_decoder.cpp').read_text()
assert 'decodePacked' in core and 'itho::decode_packet' in core
assert 'overlap_' in core and 'ramses_ii' in core
packet=(root/'components/rflink_radio_hal/rflink_packet_rx.cpp').read_text()
assert 'sink_->on_packet(' in packet
rmt=(root/'components/rflink_radio_hal/rflink_rmt_rx.cpp').read_text()
assert 'sink_->on_pulses(' in rmt and 'copy.data()' in rmt
assert rmt.index('const auto e=arm();') > rmt.index('std::array<int32_t,kMaxSymbols*2> copy{}')
assert 'if(complete && n>=2 && sink_)' in rmt
page=(root/'main/rflink_web_ota.cpp').read_text()
assert 'decode_overlap' in page and 'decode_ramses' in page
for script in re.findall(r'<script>(.*?)</script>',page,re.S):
    with tempfile.NamedTemporaryFile(mode='w',suffix='.js') as t:
        t.write(script);t.flush()
        subprocess.run(['node','--check',t.name],check=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
assert 'project(rflink_universal)' in (root/'CMakeLists.txt').read_text()
print('Stage10 source, RX ownership, website JS and decoder connection: PASS')
