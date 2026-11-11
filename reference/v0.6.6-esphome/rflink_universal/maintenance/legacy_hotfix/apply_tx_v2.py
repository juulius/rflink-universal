from pathlib import Path
import json
root=Path('/mnt/data/rflink_universal_txv2/rflink_universal')
comp=root/'components/rflink_universal'
p=comp/'radio_contract_v2.h'
s=p.read_text()
needle='struct RadioInputSpec {'
assert needle in s
s=s.replace(needle,'''// Protocol-neutral transmit transport selection. No protocol name or model is
// special-cased by the physical radio dispatcher.
enum class TxTransport : uint8_t { NONE, RMT_RAW, SPI_FIFO_PACKET, GDO_SERIAL };
struct RadioTxSpec {
  const char *protocol;
  uint8_t channel;
  TxTransport preferred;
  TxTransport fallback;
  const char *rf_profile; // Reviewed physical preset; not arbitrary register writes.
  bool enabled;           // Mirrors capabilities; cannot grant transmit authority.
};
inline bool tx_transport_kind_compatible(TxTransport route, FrameKind kind) {
  return route == TxTransport::RMT_RAW ? kind == FrameKind::RAW :
         (route == TxTransport::SPI_FIFO_PACKET || route == TxTransport::GDO_SERIAL) ?
           kind == FrameKind::PACKET : false;
}
// Eligibility is independent of protocol and independent of UI state. Only an
// available, reviewed hardware transport may be selected.
inline TxTransport choose_tx_transport(const RadioTxSpec &spec, FrameKind kind,
                                       bool rmt_available, bool spi_available,
                                       bool gdo_available) {
  if (!spec.enabled) return TxTransport::NONE;
  const auto available=[&](TxTransport t) {
    return tx_transport_kind_compatible(t,kind) &&
           ((t==TxTransport::RMT_RAW && rmt_available) ||
            (t==TxTransport::SPI_FIFO_PACKET && spi_available) ||
            (t==TxTransport::GDO_SERIAL && gdo_available));
  };
  if (available(spec.preferred)) return spec.preferred;
  if (available(spec.fallback)) return spec.fallback;
  return TxTransport::NONE;
}
'''+needle)
p.write_text(s)
p=comp/'radio_contract_v2.py'; s=p.read_text();
s=s.replace('TX_METHODS = {"legacy_tx_engine", "disabled"}', '''TX_METHODS = {"legacy_tx_engine", "disabled"}
TX_TRANSPORTS = {"none": "NONE", "rmt_raw": "RMT_RAW", "spi_fifo_packet": "SPI_FIFO_PACKET", "gdo_serial": "GDO_SERIAL"}
''')
s=s.replace('''        if not isinstance(tx, dict) or set(tx) != {"method", "status", "band"}:
            raise ValueError(f"{protocol}: invalid TX contract")''','''        if not isinstance(tx, dict) or set(tx) != {"method", "status", "band", "transport"}:
            raise ValueError(f"{protocol}: invalid TX contract")
        transport = tx["transport"]
        if not isinstance(transport, dict) or set(transport) != {"preferred", "fallback", "profile"}:
            raise ValueError(f"{protocol}: TX transport requires preferred, fallback and profile")
        primary, backup, profile = transport["preferred"], transport["fallback"], transport["profile"]
        if primary not in TX_TRANSPORTS or backup not in TX_TRANSPORTS or primary == "none" and backup != "none":
            raise ValueError(f"{protocol}: invalid TX transport")
        if not isinstance(profile, str) or not profile or len(profile) > 64:
            raise ValueError(f"{protocol}: invalid TX physical preset")
        if declared_status := cap.get("transmit", {}).get("status"):
            if declared_status == "disabled" and cap["transmit"]["encoder"] == "none" and backup != "none":
                raise ValueError(f"{protocol}: disabled TX cannot have a fallback")
        # Existing reviewed encoders are pulse or packet encoders respectively;
        # a new physical route requires an explicit compatible encoder first.
        encoder_id = cap["transmit"]["encoder"]
        if encoder_id != "none":
            expected = "spi_fifo_packet" if encoder_id == "itho_rft" else "rmt_raw"
            if primary != expected or backup != "none":
                raise ValueError(f"{protocol}: route must match reviewed TX encoder")''')
s=s.replace('''        if tx != {"method": expected_method, "status": expected_status, "band": expected_band}:
            raise ValueError(f"{protocol}: TX contract must mirror reviewed encoder; cannot grant TX")''','''        if {k: tx[k] for k in ("method", "status", "band")} != {"method": expected_method, "status": expected_status, "band": expected_band}:
            raise ValueError(f"{protocol}: TX contract must mirror reviewed encoder; cannot grant TX")
        if declared["encoder"] != "none" and primary == "none":
            raise ValueError(f"{protocol}: reviewed TX encoder missing physical route")''')
# fix possible m semantics when generator
needle='''    lines += ["}; n=sizeof(table)/sizeof(table[0]); return table; }\\n",'''
assert needle in s
# after lines helper reassignment output append further functions just before namespaces close, using find
old='''    lines[start:] = [
        "inline const RadioInputIndex &index() { size_t n=0; const auto *p=specs(n); static const RadioInputIndex idx(p,n); return idx; }\\n",
        "}}}\\n",
    ]
    return "".join(lines)'''
new='''    lines[start:] = [
        "inline const RadioInputIndex &index() { size_t n=0; const auto *p=specs(n); static const RadioInputIndex idx(p,n); return idx; }\\n",
        "inline const RadioTxSpec *tx_specs(size_t &n) { static const RadioTxSpec table[] = {\\n",
    ]
    for _, manifest in packs:
        for cap in manifest["capabilities"]:
            tx = cap["radio_v2"]["tx"]
            t = tx["transport"]
            band = tx["band"] or (manifest["bands"][0] if len(manifest["bands"]) == 1 else "example")
            lines.append("  {" + ", ".join([json.dumps(cap["protocol"]), str(BANDS[band]),
                "TxTransport::" + TX_TRANSPORTS[t["preferred"]],
                "TxTransport::" + TX_TRANSPORTS[t["fallback"]],
                json.dumps(t["profile"]),
                "true" if cap["transmit"]["encoder"] != "none" and cap["transmit"]["status"] != "disabled" else "false",
            ]) + "},\\n")
    lines.extend([
        "}; n=sizeof(table)/sizeof(table[0]); return table; }\\n",
        "inline const RadioTxSpec *tx_for(const char *protocol) { size_t n=0; const auto *p=tx_specs(n); for(size_t i=0;i<n;++i) if(protocol && std::strcmp(p[i].protocol,protocol)==0) return &p[i]; return nullptr; }\\n",
        "}}}\\n",
    ])
    return "".join(lines)'''
assert old in s
s=s.replace(old,new)
p.write_text(s)
for p in (root/'decoders').glob('*/manifest.json'):
    m=json.loads(p.read_text())
    if not m.get('enabled',True) or not all('radio_v2' in c for c in m['capabilities']):continue
    for cap in m['capabilities']:
        t=cap['radio_v2']['tx']
        encoder=cap['transmit']['encoder']
        if encoder=='none':
            primary='gdo_serial' if cap['protocol']=='RamsesII' else 'none'
        elif encoder=='itho_rft':primary='spi_fifo_packet'
        else:primary='rmt_raw'
        prof={'rmt_raw':f'ook_raw_{cap["transmit"].get("radio",m["bands"][0]).replace(".","")}',
              'spi_fifo_packet':'fsk_86830_itho_packet',
              'gdo_serial':'fsk_86830_ramses_gdo_serial_lab',
              'none':'disabled'}[primary]
        t['transport']={'preferred':primary,'fallback':'none','profile':prof}
    p.write_text(json.dumps(m,indent=2,ensure_ascii=False)+'\n')
# Add plan route + strict planner -> picked based on declarative transport contract
p=comp/'tx_engine.h';s=p.read_text().replace('#include "device_store.h"','#include "device_store.h"\n#include "radio_contract_v2.h"')
s=s.replace('''  FrameKind kind{FrameKind::RAW};''','''  FrameKind kind{FrameKind::RAW};
  TxTransport transport{TxTransport::NONE}; // picked from the decoder manifest''')
s=s.replace('''  virtual bool encode_packet(std::vector<uint8_t> &, const Plan &) const { return false; }''','''  virtual bool encode_packet(std::vector<uint8_t> &, const Plan &) const { return false; }
  virtual bool encode_serial(std::vector<uint8_t> &, const Plan &) const { return false; }''')
p.write_text(s)
p=comp/'tx_engine.cpp';s=p.read_text().replace('#include "profile_catalog.h"','#include "profile_catalog.h"\n#include "radio_v2_catalog.h"')
s=s.replace('''  candidate.frequency_hz=approved_frequency;
  plan=candidate;''','''  const auto *tx_spec = radio_v2_catalog::tx_for(d.protocol.c_str());
  if (!tx_spec || !tx_spec->enabled || tx_spec->channel != candidate.radio ||
      !tx_transport_kind_compatible(tx_spec->preferred,candidate.kind)) return false;
  candidate.transport=tx_spec->preferred;
  candidate.frequency_hz=approved_frequency;
  plan=candidate;''')
p.write_text(s)
# Native physical adapters use the plan's declared transport, not FrameKind alone.
p=comp/'rflink_universal.cpp'; s=p.read_text()
old='''  if (plan.kind == FrameKind::RAW)
    return plan.radio < raw_transmitters_.size() && raw_transmitters_[plan.radio] != nullptr;
#ifdef USE_ESP32
  return plan.kind == FrameKind::PACKET && plan.radio == 2 && packet_transmitter_ != nullptr;
#else
  return false;
#endif'''
new='''  if (plan.transport == TxTransport::RMT_RAW && plan.kind == FrameKind::RAW)
    return plan.radio < raw_transmitters_.size() && raw_transmitters_[plan.radio] != nullptr;
#ifdef USE_ESP32
  if (plan.transport == TxTransport::SPI_FIFO_PACKET && plan.kind == FrameKind::PACKET)
    return plan.radio == 2 && packet_transmitter_ != nullptr;
#endif
  // GDO serial hardware is not yet validated. Declaring it in a decoder does
  // not make it available, and cannot route it through the wrong adapter.
  return false;'''
assert old in s;s=s.replace(old,new)
s=s.replace('''  if (plan.kind == FrameKind::RAW) {
    auto call = raw_transmitters_[plan.radio]->transmit();''','''  if (plan.transport == TxTransport::RMT_RAW) {
    auto call = raw_transmitters_[plan.radio]->transmit();''')
s=s.replace('''  } else {
#ifdef USE_ESP32
    std::vector<uint8_t> packet;
    if (!encoder.encode_packet(packet, plan) || packet.size() != 63)''','''  } else if (plan.transport == TxTransport::SPI_FIFO_PACKET) {
#ifdef USE_ESP32
    std::vector<uint8_t> packet;
    if (!encoder.encode_packet(packet, plan) || packet.size() != 63)''')
s=s.replace('''    error = "packet TX requires ESP32 hardware"; return false;
#endif
  }
  tx_last_ms_''','''    error = "packet TX requires ESP32 hardware"; return false;
#endif
  } else {
    error = "declared TX transport is not verified or available";
    return false;
  }
  tx_last_ms_''')
p.write_text(s)
# Generate committed host fallback catalog
import sys
sys.path.insert(0,str(comp))
from radio_contract_v2 import validate_contract,generate_contract_header
packs=[]
for d in sorted((root/'decoders').iterdir()):
    p=d/'manifest.json'
    if d.name.startswith('_') or not p.is_file():continue
    m=json.loads(p.read_text())
    if m.get('enabled',True):validate_contract(m);packs.append((d,m))
(comp/'radio_v2_catalog_generated.h').write_text(generate_contract_header(packs))
print('Modified',len(packs),'packs and',sum(len(m['capabilities']) for _,m in packs),'protocols')
