"""CRASHTRACE: NVS crash records show under their real reset or unattached if foreign."""
from pathlib import Path
from playwright.sync_api import sync_playwright
R=Path(__file__).resolve().parents[1]
html=(R/'ui/index.html').read_text().replace('/*__RFLINK_LOCALES__*/',(R/'ui/locales.bundle.json').read_text())
script=html.split('<script>',1)[1].split('</script>',1)[0]
static=html.replace('<script>'+script+'</script>','')
for width in [1440,390]:
 with sync_playwright() as pw:
  browser=pw.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox','--disable-dev-shm-usage'])
  page=browser.new_page(viewport={'width':width,'height':850})
  errors=[];page.on('pageerror',lambda e:errors.append(str(e)))
  page.set_content(static)
  page.evaluate("""() => {window.fetch=async (url)=>{
   const u=String(url);let d={};
   if(u==='/rflink/api/status')d={firmware_version:'v0.5.4',firmware_name:'Radiolove',
    boot_count:126,boot_session_id:126,reset_reason:'software',boot_history_persisted:true,
    boot_history:[{boot_number:126,reason:'software',reason_code:3},{boot_number:125,reason:'panic',reason_code:4},
      {boot_number:124,reason:'panic',reason_code:4},{boot_number:123,reason:'software',reason_code:3}],
    crash_trace_persisted:true,crash_traces:[
      {boot_number:125,reason:'Abort',core:1,pc:'0x4037D364',foreign_build:false,source_hash:'abcabcabcabc',
       backtrace:['0x4037D361','0x420D3356','0x4201F3E2'],other_backtrace:[]},
      {boot_number:0,reason:'Task wdt',core:0,pc:'0x420000AB',foreign_build:true,source_hash:'',backtrace:['0x420000AA'],other_backtrace:[]}],
    uptime_ms:20000,channels:[],activity:[],total_frames:0,decoded_frames:0,unknown_frames:0,noise_frames:0,storage_ready:true,storage_ok:true,decoder_count:13,expected_decoders:13};
   else if(u==='/rflink/api/language')d={language:'nl'};
   else if(u==='/rflink/api/system-trace')d={boot_session_id:126,server_uptime_ms:20000,events:[{seq:1,ms:450,subsystem:'boot',level:'info',message:'Boot #126 after software'}]};
   else if(u.startsWith('/rflink/api/rx-log'))d={events:[],has_more:false};
   else if(u==='/rflink/api/devices')d={devices:[],candidates:[],storage_ok:true};
   else if(u==='/rflink/api/decoders')d={protocols:[],ha_fields:[]};
   else if(u==='/rflink/api/profile-catalog')d={profiles:[]};
   else if(u==='/rflink/api/user-library')d={profiles:[],decoders:[]};
   else if(u==='/rflink/api/library')d={items:[]};
   return {ok:true,status:200,json:async()=>d,text:async()=>JSON.stringify(d)};
  }}""")
  page.evaluate(script)
  page.evaluate('window.rfDiagnosticStatus=null')
  page.evaluate("document.querySelector('[data-page=monitor]').click()")
  page.wait_for_function("document.querySelector('#logbookTimeline').textContent.includes('Start #125')")
  page.locator('[data-log-view="system"]').click()
  page.locator('#logbookEventType').select_option('boot')
  entries=page.locator('#logbookTimeline details')
  row=page.locator('#logbookTimeline details[data-key="history-125"]')
  assert 'Backtrace beschikbaar' in row.inner_text(),(width,row.inner_text())
  row.locator('summary').click()
  d=row.inner_text()
  assert '0x4037D364' in d and '0x4201F3E2' in d and 'Core: 1' in d,d
  foreign=page.locator('#logbookTimeline details[data-key="crashtrace-0"]')
  assert foreign.count()==1,(width,entries.all_inner_texts())
  foreign.locator('summary').click()
  assert 'Andere/oude build' in foreign.inner_text()
  if width==1440: page.screenshot(path='/mnt/data/RFLink_v054_CrashTrace_voorbeeld.png')
  no=page.locator('#logbookTimeline details[data-key="history-124"]')
  no.locator('summary').click()
  assert 'Stacktrace' not in no.inner_text() and 'niet bewaard' in no.inner_text(),no.inner_text()
  assert not errors,(width,errors)
  print('CRASHTRACE browser PASS',width)
  browser.close()
