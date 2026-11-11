"""Boot diagnostics UI: boot id, uptime, memory, history, counter changes, export (browser fixture)."""
from pathlib import Path
from playwright.sync_api import sync_playwright
root=Path(__file__).resolve().parents[1]
html=(root/'ui/index.html').read_text().replace('/*__RFLINK_LOCALES__*/',(root/'ui/locales.bundle.json').read_text())
js=html.split('<script>',1)[1].split('</script>',1)[0]
static=html.replace('<script>'+js+'</script>','')
with sync_playwright() as pw:
    b=pw.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox','--disable-dev-shm-usage'])
    p=b.new_page(viewport={'width':390,'height':844},accept_downloads=True)
    err=[]
    p.on('pageerror',lambda e:err.append(str(e)))
    p.set_content(static)
    p.evaluate('''() => {
       window.__bootFixture={boot_session_id:10,boot_count:2,boot_history_persisted:true,reset_reason:'power_on',reset_reason_code:1,boot_history:[{boot_number:2,reason:'power_on',reason_code:1},{boot_number:1,reason:'software',reason_code:3}],uptime_ms:93784000,free_heap_bytes:123456,min_free_heap_bytes:100000,total_frames:50,decoded_frames:0,unknown_frames:1,noise_frames:49,decoder_count:12,expected_decoders:12,storage_ready:true,storage_ok:true,saved_devices:0,activity:[],channels:[],led:{status:'ready',status_code:'ready',brightness:20,quiet:false}};
       window.fetch=async(url,options={})=>{let out={};
         if(url==='/rflink/api/status')out=window.__bootFixture;
         else if(url==='/rflink/api/language')out={language:'nl'};
         else if(url==='/rflink/api/devices')out={storage_ok:true,mqtt_connected:false,mqtt_configured:false,devices:[],candidates:[]};
         else if(url==='/rflink/api/decoders')out={decoders:[],protocols:[],ha_fields:[]};
         else if(url==='/rflink/api/library')out={schema:1,persistent_user_library:true,official_decoders:0,official_profiles:0,user_decoders:0,user_profiles:0,attention_count:0,safe_resolution_count:0,archive_count:0,items:[]};
         else if(url==='/rflink/api/user-library')out={schema:1,decoders:[],profiles:[],protocols:[],radio_profiles:[]};
         else if(url==='/rflink/api/developer')out={mode:'idle',captures:[]};
         else out={ok:true};
         return {ok:true,json:async()=>out,text:async()=>''};
       }
    }''')
    p.evaluate(js)
    p.wait_for_function("document.documentElement.lang==='nl'")
    p.wait_for_function("document.getElementById('bootDiagnostics').innerText.includes('1d 2h 3m 4s')")
    assert 'Uptime' in p.locator('#healthDetails').inner_text()
    assert '1d 2h 3m 4s' in p.locator('#healthDetails').inner_text()
    assert '2' in p.locator('#bootDiagnostics').inner_text()
    assert '#1' in p.locator('#bootLog').inner_text()
    assert '121 KiB' in p.locator('#bootDiagnostics').inner_text()
    p.evaluate('''() => {window.__bootFixture={...window.__bootFixture,uptime_ms:1000,boot_session_id:11,boot_count:3,reset_reason:'brownout',reset_reason_code:9,boot_history:[{boot_number:3,reason:'brownout',reason_code:9}],total_frames:0}}''')
    p.wait_for_function("document.getElementById('bootLiveEvents').innerText.includes('Herstart gezien')", timeout=5500)
    assert 'Brownout' in p.locator('#bootDiagnostics').inner_text()
    # Same boot id, but RF counter gets smaller: different warning than reboot.
    p.evaluate('''() => {window.__bootFixture={...window.__bootFixture,uptime_ms:2000,total_frames:0}}''')
    p.wait_for_timeout(3200)
    p.evaluate('''() => {window.__bootFixture={...window.__bootFixture,uptime_ms:3000,total_frames:10}}''')
    p.wait_for_timeout(3200)
    p.evaluate('''() => {window.__bootFixture={...window.__bootFixture,uptime_ms:4000,total_frames:2}}''')
    p.wait_for_function("document.getElementById('bootLiveEvents').innerText.includes('Signaalteller gedaald')", timeout=5500)
    p.locator('.bottom button[data-page="settings"]').click()
    p.locator('[data-settings-open="diagnostics"]').click()
    assert p.locator('#diagOverview').is_visible(), 'Diagnostics opens content without a duplicate disclosure click'
    p.locator('#bootLogExport').evaluate('(e)=>e.closest("details").open=true')
    with p.expect_download() as download_info:
        p.locator('#bootLogExport').click()
    download=download_info.value
    assert download.suggested_filename=='rflink-boot-diagnostics.txt'
    assert not err,err
    p.close();b.close()
print('BOOTDIAG1: mobile uptime, persisted boots, heap, reboot-vs-counter, export: PASS')
