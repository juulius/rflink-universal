"""Manifest-based radio policy: every decoder declares hardware requirements."""
import importlib.util
import json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
MODULE=ROOT/'components/rflink_universal/radio_transport_manifest.py'
spec=importlib.util.spec_from_file_location('radio_contract',MODULE)
module=importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
packs=[]
for folder in sorted((ROOT/'decoders').iterdir()):
    manifest=folder/'manifest.json'
    if manifest.is_file():
        m=json.loads(manifest.read_text())
        if m.get('enabled',True):
            module.validate_radio_transport(m)
            packs.append((folder,m))
header=module.generate_radio_header(packs)
assert header==(ROOT/'components/rflink_universal/radio_transport_catalog_generated.h').read_text()
assert '"RamsesII", 2, RxTransport::UART_STREAM, RxTransport::SPI_FIFO_SYNC' in header
assert '"IthoRFT", 2, RxTransport::SPI_PACKET' in header
original=[m for _,m in packs if m['id']=='ramses_ii'][0]
for bad in [dict(original['radio_transport'],preferred_rx='random'),
            dict(original['radio_transport'],radio='433.42'),
            dict(original['radio_transport'],fallback_rx='uart_stream'),
            dict(original['radio_transport'],max_window_ms=90000)]:
    clone=dict(original,radio_transport=bad)
    try: module.validate_radio_transport(clone)
    except ValueError: pass
    else: raise AssertionError(f'bad radio configuration accepted: {bad}')
print('Decoder radio manifest policies: PASS')
