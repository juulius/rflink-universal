"""v0.5.3 user acceptance: short release identity and historical crash reasons in logbook."""
from pathlib import Path
from playwright.sync_api import sync_playwright
ROOT=Path(__file__).resolve().parents[1]
html=(ROOT/'ui/index.html').read_text().replace('/*__RFLINK_LOCALES__*/',(ROOT/'ui/locales.bundle.json').read_text())
js=html.split('<script>',1)[1].split('</script>',1)[0]
static=html.replace('<script>'+js+'</script>','')
for width in (1440,390):
 with sync_playwright() as pw:
  browser=pw.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox','--disable-dev-shm-usage'])
  page=browser.new_page(viewport={'width':width,'height':950},accept_downloads=True)
  errors=[];page.on('pageerror',lambda e:errors.append(str(e)))
  page.set_content(static)
  page.evaluate('''() => {
    window.fetch=async(url,options={})=>{
      const u=String(url);let out={ok:true};
      if(u==='/rflink/api/status')out={firmware_version:'v0.5.3',firmware_name:'Radiolove',firmware_build_hash:'5ecc2ed3582e',firmware_revision:'v0.5.3-5ecc2ed3582e',firmware_built:'Nov 3 2026',boot_count:123,boot_session_id:1020,reset_reason:'power_on',reset_reason_code:1,boot_history_persisted:true,boot_history:[{boot_number:123,reason:'power_on',reason_code:1},{boot_number:122,reason:'panic',reason_code:4},{boot_number:121,reason:'software',reason_code:3},{boot_number:120,reason:'brownout',reason_code:15}],uptime_ms:34000,channels:[],activity:[],total_frames:0,decoded_frames:0,unknown_frames:0,noise_frames:0,storage_ready:true,storage_ok:true,decoder_count:13,expected_decoders:13};
      else if(u==='/rflink/api/language')out={language:'nl'};
      else if(u==='/rflink/api/system-trace')out={ok:true,server_uptime_ms:34000,boot_session_id:1020,events:[{seq:1,ms:450,subsystem:'boot',level:'info',message:'Boot #123 after power_on; previous reset reason only'}]};
      else if(u.startsWith('/rflink/api/rx-log?'))out={ok:true,events:[]};
      else if(u==='/rflink/api/devices')out={devices:[],candidates:[],storage_ok:true};
      else if(u==='/rflink/api/decoders')out={protocols:[],ha_fields:[]};
      else if(u==='/rflink/api/profile-catalog')out={profiles:[]};
      else if(u==='/rflink/api/library')out={items:[],attention_count:0};
      else if(u==='/rflink/api/user-library')out={profiles:[],decoders:[]};
      else if(u==='/rflink/api/system-trace-config')out={limit:128};
      else if(u==='/rflink/api/rx-log-config')out={signal_limits:[250,250,250],noise_limit:2000};
      return {ok:true,status:200,json:async()=>out,text:async()=>JSON.stringify(out)};
    };
  }''')
  page.evaluate(js)
  page.wait_for_function("document.querySelector('#firmwareVersion').textContent==='v0.5.3 Radiolove'")
  assert page.locator('#firmwareRevision').inner_text()=='5ecc2ed3582e'
  assert 'BOOTDIAG1-' not in page.locator('#firmwareAboutCard').inner_text()
  assert 'v0.6' in page.locator('#firmwareNotes').inner_text()
  page.evaluate("document.querySelector('[data-page=\"monitor\"]').click()")
  page.wait_for_function("document.querySelector('#logbookTimeline').textContent.includes('Start #122')",timeout=5000)
  page.locator('[data-log-view="system"]').click()
  page.locator('#logbookEventType').select_option('boot')
  text=page.locator('#logbookTimeline').inner_text()
  assert 'Start #122' in text and 'Crash / panic' in text,(width,text)
  assert 'Start #120' in text and 'Brownout' in text,(width,text)
  assert 'Start #121' in text and 'Software' in text,(width,text)
  assert text.count('Boot #123')==1,(width,text)
  assert 'Eerdere start' in text and '00:00:00,450' in text,(width,text)
  assert page.locator('#logbookTimeline details').count()==4,(width,text)
  page.screenshot(path=f'/mnt/data/RFLink_v051_CrashHistory_{width}.png')
  with page.expect_download(timeout=5000) as item:page.locator('#logbookExport').click()
  import json
  rows=json.loads(Path(item.value.path()).read_text())['events']
  assert sum(bool(e.get('historical')) for e in rows)==3
  assert not errors,(width,errors)
  print('RFLink v0.5.3 crash/history + version UI PASS',width,flush=True)
  browser.close()
