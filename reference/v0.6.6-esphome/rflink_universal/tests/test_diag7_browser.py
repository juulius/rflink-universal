"""DIAG7 browser acceptance: boot incident, exported metadata, time format, filtered log, local search."""
from pathlib import Path
import json
from playwright.sync_api import sync_playwright
R=Path(__file__).resolve().parents[1]
html=(R/'ui/index.html').read_text().replace('/*__RFLINK_LOCALES__*/',(R/'ui/locales.bundle.json').read_text())
script=html.split('<script>',1)[1].split('</script>',1)[0]
static=html.replace('<script>'+script+'</script>','')
for width in (1440,390):
 with sync_playwright() as pw:
  browser=pw.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox','--disable-dev-shm-usage'])
  page=browser.new_page(viewport={'width':width,'height':900},accept_downloads=True)
  errors=[];page.on('pageerror',lambda e:errors.append(str(e)))
  page.set_content(static)
  page.evaluate('''() => {
    window.calls={devices:0};
    window.fetch=async (url,options={})=>{
      const u=String(url);let out={ok:true};
      if(u==='/rflink/api/language')out={language:'nl'};
      else if(u==='/rflink/api/status')out={boot_count:107,boot_session_id:34,reset_reason:'panic',reset_reason_code:4,boot_history_persisted:true,boot_history:[{boot_number:107,reason:'panic'},{boot_number:106,reason:'panic'},{boot_number:105,reason:'software'}],firmware_revision:'UNIFIED6-DIAG7',firmware_version:'v0.3.3',firmware_built:'Nov 1',uptime_ms:60000,channels:[],activity:[],total_frames:0,decoded_frames:0,unknown_frames:0,noise_frames:0,storage_ready:true,storage_ok:true,decoder_count:13,expected_decoders:13,internal_free_bytes:200000,largest_internal_block_bytes:100000,psram_free_bytes:2000000};
      else if(u==='/rflink/api/system-trace')out={ok:true,server_uptime_ms:60000,events:[{seq:1,ms:380,subsystem:'boot',level:'error',message:'Boot #107 after panic; previous reset reason only'},{seq:2,ms:420,subsystem:'radio',level:'info',message:'Radio ready'}]};
      else if(u.startsWith('/rflink/api/rx-log?'))out={ok:true,events:[]};
      else if(u==='/rflink/api/devices'){window.calls.devices++;out={devices:[{uid:'dev_1',name:'Ventilator',protocol:'RamsesII',channel:2,profile_id:'',actions:[],fields:[],pending_actions:[],pending_fields:[],rf_identities:[],ha_enabled:false}],candidates:[],storage_ok:true}}
      else if(u==='/rflink/api/decoders')out={protocols:[],ha_fields:[]};
      else if(u==='/rflink/api/profile-catalog')out={profiles:[]};
      else if(u==='/rflink/api/library')out={items:[],attention_count:0};
      else if(u==='/rflink/api/user-library')out={profiles:[],decoders:[]};
      else if(u==='/rflink/api/rx-log-config')out={signal_limits:[250,250,250],noise_limit:2000};
      else if(u==='/rflink/api/system-trace-config')out={limit:128};
      return {ok:true,status:200,json:async()=>out,text:async()=>JSON.stringify(out)};
    };
  }''')
  page.evaluate(script)
  page.wait_for_function("document.querySelector('#crashStatusNotice').hidden===false")
  assert 'crashes' in page.locator('#crashStatusTitle').inner_text().lower()
  page.screenshot(path=f'/mnt/data/RFLink_DIAG7_Status_{width}.png')
  page.locator('#crashStatusOpen').click()
  page.wait_for_function("document.querySelector('#logbookTimeline').textContent.includes('Boot #107')")
  assert page.locator('#logbookEventType').input_value()=='boot'
  assert '00:00:00,380' in page.locator('#logbookTimeline').inner_text()
  assert 'Radio ready' not in page.locator('#logbookTimeline').inner_text()
  page.locator('#logbookEventType').select_option('all')
  assert 'Radio ready' in page.locator('#logbookTimeline').inner_text()
  page.screenshot(path=f'/mnt/data/RFLink_DIAG7_Logboeken_{width}.png')
  with page.expect_download(timeout=3000) as dl:page.locator('#logbookDiagnosticExport').click()
  report=json.loads(Path(dl.value.path()).read_text())
  assert report['boot']['number']==107
  assert report['boot']['previous_reset_reason']=='panic'
  assert report['memory']['largest_internal_block_bytes']==100000
  assert report['format']=='RFLink-DIAG7'
  dev=page.locator('[data-page="devices"]').last if width==390 else page.locator('[data-page="devices"]').first
  dev.click()
  page.wait_for_function('window.calls.devices>=1')
  before=page.evaluate('window.calls.devices')
  page.locator('#deviceSearch').fill('vent')
  page.wait_for_timeout(200)
  assert page.evaluate('window.calls.devices')==before,'Searching cached list must not make new device API calls'
  assert page.locator('[data-device-open="dev_1"]').count()==1
  page.locator('[data-device-open="dev_1"]').click()
  page.screenshot(path=f'/mnt/data/RFLink_DIAG7_Apparaat_{width}.png')
  assert not errors,(width,errors)
  print('DIAG7 browser PASS',width,flush=True)
  browser.close()
