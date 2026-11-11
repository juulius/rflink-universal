"""Logbooks must fetch saved NVS reset history without prior Status view."""
from pathlib import Path
from playwright.sync_api import sync_playwright
P=Path(__file__).resolve().parents[1]
html=(P/'ui/index.html').read_text().replace('/*__RFLINK_LOCALES__*/',(P/'ui/locales.bundle.json').read_text())
js=html.split('<script>',1)[1].split('</script>',1)[0]
static=html.replace('<script>'+js+'</script>','')
for width in (1440,390):
 with sync_playwright() as pw:
  browser=pw.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox','--disable-dev-shm-usage'])
  page=browser.new_page(viewport={'width':width,'height':850})
  errors=[];page.on('pageerror',lambda e:errors.append(str(e)))
  page.set_content(static)
  page.evaluate("""() => {
    window.statusRequestCount=0;
    window.fetch=async(url,opts={})=>{
      const u=String(url);let data={ok:true};
      if(u==='/rflink/api/status'){
        window.statusRequestCount++;
        data={firmware_version:'v0.5.3',firmware_name:'Radiolove',boot_count:123,
        boot_session_id:45,boot_history_persisted:true,reset_reason:'power_on',
        boot_history:[{boot_number:123,reason:'power_on',reason_code:1},
        {boot_number:122,reason:'panic',reason_code:4},
        {boot_number:121,reason:'brownout',reason_code:15}],
        uptime_ms:20000,channels:[],activity:[],total_frames:0,
        decoded_frames:0,unknown_frames:0,noise_frames:0,
        storage_ready:true,storage_ok:true,decoder_count:13,expected_decoders:13};
      } else if(u==='/rflink/api/language')data={language:'nl'};
      else if(u==='/rflink/api/system-trace')data={boot_session_id:45,server_uptime_ms:20000,events:[{seq:1,ms:450,subsystem:'boot',level:'info',message:'Boot #123 after power_on; previous reset reason only'}]};
      else if(u.startsWith('/rflink/api/rx-log'))data={events:[],has_more:false};
      else if(u==='/rflink/api/devices')data={devices:[],candidates:[],storage_ok:true};
      else if(u==='/rflink/api/decoders')data={protocols:[],ha_fields:[]};
      else if(u==='/rflink/api/profile-catalog')data={profiles:[]};
      else if(u==='/rflink/api/user-library')data={profiles:[],decoders:[]};
      else if(u==='/rflink/api/library')data={items:[]};
      return {ok:true,status:200,json:async()=>data,text:async()=>JSON.stringify(data)};
    };
  }""")
  page.evaluate(js)
  # Simulate opening logbooks with no status cache (exact reported symptom).
  page.evaluate('window.rfDiagnosticStatus=null')
  before=page.evaluate('window.statusRequestCount')
  page.evaluate("document.querySelector('[data-page=monitor]').click()")
  page.wait_for_function("document.querySelector('#logbookTimeline').textContent.includes('Start #122')",timeout=7000)
  page.locator('[data-log-view="system"]').click()
  page.locator('#logbookEventType').select_option('boot')
  result=page.locator('#logbookTimeline').inner_text()
  assert 'Start #122' in result and 'Start #121' in result,(width,result)
  assert 'Crash / panic' in result and 'Brownout' in result,(width,result)
  assert page.evaluate('window.statusRequestCount')>before,'Expected /api/status request on entering logbook'
  assert result.count('Boot #123')==1,(width,result)
  assert not errors,(width,errors)
  print('Boot history independent logbook PASS',width)
  browser.close()
