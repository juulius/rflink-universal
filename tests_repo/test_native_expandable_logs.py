"""Native v0.7 expandable log detail parity (no raw radio payload exposure)."""
import shutil
import subprocess
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
JS=(ROOT/'firmware/main/webui/app.js').read_text(encoding='utf-8')
CSS=(ROOT/'firmware/main/webui/app.css').read_text(encoding='utf-8')
HTML=(ROOT/'firmware/main/webui/index.html').read_text(encoding='utf-8')
WEB=(ROOT/'firmware/main/rflink_web_ota.cpp').read_text(encoding='utf-8')
MAIN=(ROOT/'firmware/main/main.cpp').read_text(encoding='utf-8')


class ExpandableNativeLogTests(unittest.TestCase):
    def test_native_summary_can_expand_without_replacing_layout(self):
        for marker in ('id="log-rows"','id="log-feedback"',
                       'Uptime (HH:MM:SS,ms)','<th>Source</th>',
                       '<th>Radio</th>','<th>Event</th>','<th>Details</th>'):
            self.assertIn(marker,HTML)
        for marker in ('className=\'log-main-row\'','className=\'log-expanded-row\'',
                       'detailRow.hidden=!open','expandedLogKeys.add(row.dataset.logKey)',
                       'expandedLogKeys.delete(row.dataset.logKey)',
                       "button.setAttribute('aria-expanded',String(open))",
                       "button.setAttribute('aria-controls',detailId)",
                       "button.type='button'", 'savedScroll',
                       "button.focus({preventScroll:true})", "document.createElement('dl')",
                       "dd.textContent=String(value)"):
            with self.subTest(marker=marker):
                self.assertIn(marker,JS)
        self.assertIn('.log-expanded-row[hidden]{display:none}',CSS)
        self.assertTrue('Click a log entry to expand' in HTML or\n                        'Tap a log event to expand' in HTML)

    def test_decoder_fields_are_sourced_from_the_actual_native_ring(self):
        self.assertIn('static void log_rx_observer',MAIN)
        self.assertIn('static_cast<int32_t>(command)',MAIN)
        self.assertIn('"value":%ld',WEB.replace('\\',''))
        self.assertIn('"sequence":%llu',WEB.replace('\\',''))
        self.assertIn('"tx_rx_restored"',WEB.replace('\\',''))
        self.assertIn("code===4",JS)
        self.assertIn('RAMSES-II',JS)
        self.assertIn('Packet length',JS)
        self.assertIn('TX correlation ID',JS)
        self.assertIn('RF on-air confirmation',JS)

    def test_does_not_claim_unrecorded_payload(self):
        self.assertIn('Raw RF message',JS)
        self.assertIn('Not stored: payload bytes, pulse durations',JS)
        self.assertIn('rolling codes',HTML)
        self.assertNotIn('raw_payload',WEB)
        self.assertNotIn('raw_bytes',WEB)
        self.assertNotIn('innerHTML',JS[JS.index('const expandedLogKeys='):JS.index('async function refreshLogs(')])

    @unittest.skipUnless(shutil.which('node'), 'Node.js required for JS metadata contract')
    def test_details_formatting_on_real_log_shapes(self):
        start=JS.index('const expandedLogKeys=')
        end=JS.index('function renderLogs(events){',start)
        pure=JS[start:end]
        script=(
            "const assert=require('node:assert/strict');\n"
            "const uptimeMs=n=>'00.00.00.01';\n"
            "const radioNames=['433.42 MHz','433.92 MHz','868.30 MHz'];\n"
            +pure+
            """
const ram={sequence:144,uptime_ms:1200,stream:'signal',radio:2,
  kind:'RAMSES-II',code:4,severity:0,count:9,value:0x10e0,flags:1};
const fields=Object.fromEntries(logMetadataFields(ram));
assert.equal(fields.Command,'0x10E0 (4320)');
assert.equal(fields['Packet length'],'9 bytes');
assert.equal(fields['Decoder overlap'],'Yes');
assert.ok(fields['Raw RF message'].includes('Not stored'));
assert.ok(logDetails(ram).includes('RAMSES command 0x10E0'));
const pulse={sequence:145,uptime_ms:1220,stream:'noise',radio:0,
  kind:'Raw pulses',code:1,severity:0,count:64,value:0,flags:0};
assert.equal(Object.fromEntries(logMetadataFields(pulse))['Pulse count'],'64 pulses');
assert.ok(!logDetails(pulse).includes('bytes'));
const tx={sequence:146,uptime_ms:1230,stream:'signal',radio:2,
  kind:'TX driver accepted',code:32,tx_transport:2,tx_stage:0,
  severity:0,count:1,tx_repeats:3,profile:1,correlation_id:39,
  value:0,tx_rx_previous:true,tx_rx_restored:true,
  tx_driver_attempted:true,tx_driver_accepted:true,tx_hardware_restored:true};
const x=Object.fromEntries(logMetadataFields(tx));
assert.equal(x.Transport,'UART');
assert.equal(x['RX restoration'],'Restored');
assert.equal(x['RF on-air confirmation'],'Not available from the driver trace');
assert.ok(x['Driver status'].includes('RF verification'));
assert.equal(logRowKey(tx),'signal-146');
const sys={sequence:147,uptime_ms:2000,stream:'system',radio:255,
 kind:'Wi-Fi disconnected',code:15,severity:1,value:201};
assert.equal(Object.fromEntries(logMetadataFields(sys))['Status / error code'],'201');
console.log('PASS expandable native log detail metadata');
"""
        )
        p=subprocess.run(['node','-'],input=script,text=True,capture_output=True,timeout=20)
        self.assertEqual(p.returncode,0,p.stderr)


if __name__=='__main__':
    unittest.main()
