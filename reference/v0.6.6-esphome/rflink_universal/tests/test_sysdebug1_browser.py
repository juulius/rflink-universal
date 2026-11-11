"""SYSDEBUG1: user-visible internal trace and safe experimental lab controls.
Mocks intentionally do not assert physical RF transmission.
"""
from pathlib import Path
from playwright.sync_api import sync_playwright
import json
R=Path(__file__).resolve().parents[1]
html=(R/'ui/index.html').read_text().replace('/*__RFLINK_LOCALES__*/',(R/'ui/locales.bundle.json').read_text())
script=html.split('<script>',1)[1].split('</script>',1)[0]
static=html.replace('<script>'+script+'</script>','')
for width in (1440,390):
 with sync_playwright() as pw:
  b=pw.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox','--disable-dev-shm-usage'])
  p=b.new_page(viewport={'width':width,'height':950},accept_downloads=True)
  errors=[];p.on('pageerror',lambda e:errors.append(str(e)))
  p.set_content(static)
  p.evaluate('''()=>{
    window.__debugProbeCount=0;
    window.__debugProbeEnabled=false;
    window.__debugEvents=[
      {seq:1,ms:5000,level:'info',subsystem:'boot',uid:'',message:'RFLink started'},
      {seq:2,ms:7000,level:'info',subsystem:'learn',uid:'dev123',message:'Device learned'},
      {seq:3,ms:9000,level:'warn',subsystem:'tx',uid:'dev123',message:'No approved encoder'},
      {seq:4,ms:12000,level:'info',subsystem:'radio',uid:'',message:'868 Itho ready'}];
    window.fetch=async(url,opts={})=>{
      const u=String(url); let out={ok:true};
      if(u==='/rflink/api/language')out={language:'nl'};
      else if(u==='/rflink/api/status')out={uptime_ms:15000,total_frames:0,decoded_frames:0,unknown_frames:0,decoder_count:13,channels:[],activity:[],storage_ok:true,storage_ready:true};
      else if(u==='/rflink/api/system-trace')out={ok:true,capacity:128,dropped:0,gdo_lab:window.__debugProbeEnabled,probe_lab:window.__debugProbeEnabled,adapter_registered:window.__debugProbeEnabled,adapter_ready:window.__debugProbeEnabled,sticky_fault:false,radio_868_mode:0,events:window.__debugEvents};
      else if(u.startsWith('/rflink/api/lab-tx-probe')){window.__debugProbeCount++;out={ok:true,radio_reported_sent:true,device_confirmed:false};}
      else if(u==='/rflink/api/devices')out={devices:[],candidates:[],storage_ok:true};
      else if(u==='/rflink/api/decoders')out={protocols:[],ha_fields:[]};
      else if(u==='/rflink/api/library')out={items:[],attention_count:0};
      else if(u==='/rflink/api/user-library')out={profiles:[],decoders:[],write_token:'stub'};
      return {ok:true,json:async()=>out,text:async()=>JSON.stringify(out)};
    };
  }''')
  p.evaluate(script)
  p.wait_for_timeout(300)
  p.evaluate('document.querySelector("[data-page=settings]").click()')
  p.locator('[data-settings-open="systemtrace"]').click()
  p.wait_for_function('document.querySelectorAll("#systemTraceEvents .row").length===4')
  assert not p.locator('#systemTraceTechnical').get_attribute('open')
  p.locator('#systemTraceTechnical summary').click()
  assert not p.locator('#systemTraceProbe').is_enabled()
  p.locator('#systemTraceFilter').select_option('tx')
  assert p.locator('#systemTraceEvents .row').count()==1
  assert 'No approved encoder' in p.locator('#systemTraceEvents').inner_text()
  p.locator('#systemTraceFilter').select_option('all')
  with p.expect_download() as download:
   p.locator('#systemTraceExport').click()
  assert download.value.suggested_filename=='rflink-system-trace.json'
  p.evaluate('window.__debugProbeEnabled=true')
  p.locator('#systemTraceRefresh').click()
  p.wait_for_function('document.querySelector("#systemTraceProbe").disabled===false')
  p.once('dialog',lambda dialog:dialog.accept())
  p.locator('#systemTraceProbe').click()
  p.wait_for_function('window.__debugProbeCount===1')
  assert 'NIET bevestigd' in p.locator('#systemTraceProbeStatus').inner_text()
  p.evaluate('rfApplyLanguage("en")')
  assert p.locator('#systemTraceCard h2').inner_text()=='TX lab & system diagnostics'
  for language in ('fr','de','nl'):
   p.evaluate('(lang)=>rfApplyLanguage(lang)',language)
   assert p.locator('#systemTraceCount').inner_text().strip()
  assert p.evaluate('document.documentElement.scrollWidth <= innerWidth + 2'),width
  p.evaluate('rfApplyLanguage("nl")')
  if width==1440:p.screenshot(path='/mnt/data/RFLink_SYSDEBUG1_Desktop.png',full_page=True)
  else:p.screenshot(path='/mnt/data/RFLink_SYSDEBUG1_Mobiel.png',full_page=True)
  assert not errors,(width,errors)
  print('SYSDEBUG1 simulated UI PASS at',width,'px',flush=True)
  b.close()
