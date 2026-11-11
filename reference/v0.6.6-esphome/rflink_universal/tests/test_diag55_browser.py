"""v0.5.5: one central, calm diagnosis tab and settings visual regression.
Browser simulation only; does NOT verify a real ESP32 TX or stack overflow.
"""
from pathlib import Path
from playwright.sync_api import sync_playwright
R=Path(__file__).resolve().parents[1]
html=(R/'ui/index.html').read_text(encoding='utf-8').replace('/*__RFLINK_LOCALES__*/',(R/'ui/locales.bundle.json').read_text(encoding='utf-8'))
script=html.split('<script>',1)[1].split('</script>',1)[0]
static=html.replace('<script>'+script+'</script>','')
mock=r'''() => {
window.fetch=async url=>{
 const u=String(url);let d={ok:true};
 if(u==='/rflink/api/language')d={language:'nl'};
 else if(u==='/rflink/api/status')d={boot_count:8,boot_session_id:8,boot_history_persisted:true,boot_history:[{boot_number:8,reason:'software'},{boot_number:7,reason:'panic'},{boot_number:6,reason:'brownout'}],crash_traces:[{boot_number:7,core:0,pc:'0x4037D364',reason:'Abort',backtrace:['0x4037DB62']}],firmware_version:'v0.5.5',firmware_name:'Radiolove',firmware_build_hash:'def94a6ee268',uptime_ms:45000,channels:[{index:0,name:'433.42',available:true,rx_ready:true,checked:true,spi_failures:0,spi_recovery_attempts:0},{index:1,name:'433.92',available:true,rx_ready:true,checked:true,spi_failures:0,spi_recovery_attempts:0},{index:2,name:'868.30',available:true,rx_ready:true,checked:true,spi_failures:2,spi_recovery_attempts:1}],total_frames:2,noise_frames:1,decoded_frames:0,unknown_frames:1,free_heap_bytes:950000,internal_free_bytes:200000,largest_internal_block_bytes:100000,psram_free_bytes:2000000,activity:[],storage_ready:true,storage_ok:true,decoder_count:13,expected_decoders:13};
 else if(u==='/rflink/api/system-trace')d={ok:true,server_uptime_ms:45000,events:[{seq:1,ms:380,subsystem:'boot',level:'info',message:'Started'},{seq:2,ms:400,subsystem:'radio',level:'warn',message:'SPI fault'}],radio_868_mode:0,gdo_lab:true,probe_lab:true,adapter_ready:true};
 else if(u.startsWith('/rflink/api/rx-log?'))d={events:[],has_more:false};
 else if(u==='/rflink/api/devices')d={devices:[],candidates:[],storage_ok:true};
 else if(u==='/rflink/api/decoders')d={protocols:[],ha_fields:[]};
 else if(u==='/rflink/api/profile-catalog')d={profiles:[]};
 else if(u==='/rflink/api/user-library')d={profiles:[],decoders:[]};
 else if(u==='/rflink/api/library')d={items:[],attention_count:0};
 else if(u==='/rflink/api/rx-log-config')d={signal_limits:[250,250,250],noise_limit:2000};
 else if(u==='/rflink/api/system-trace-config')d={limit:128};
 return {ok:true,status:200,json:async()=>d,text:async()=>JSON.stringify(d)};
};
}'''
for width in (1440,390):
 with sync_playwright() as pw:
  browser=pw.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox','--disable-dev-shm-usage'])
  page=browser.new_page(viewport={'width':width,'height':850},color_scheme='dark',accept_downloads=True)
  errors=[];page.on('pageerror',lambda e:errors.append(str(e)))
  page.set_content(static);page.evaluate(mock);page.evaluate(script)
  page.locator('[data-page="monitor"]:visible').first.click()
  page.locator('[data-log-view="diagnostics"]').click()
  page.wait_for_function("document.getElementById('diagFiveRadioSummary').textContent.includes('3/3')")
  assert page.locator('#logbookTimeline').is_hidden(),width
  assert page.locator('.diag-five-card:visible').count()==4,width
  page.screenshot(path=f'/mnt/data/RFLink_v055_Diagnose_Dark_{width}.png',full_page=True)
  assert page.locator('.diag-five-details:visible').count()==0,width
  assert page.locator('#diagFiveRadioSummary').inner_text().startswith('3/3'),width
  page.locator('[data-diag-show="diagFiveBoot"]').click()
  assert page.locator('#diagFiveBoot').is_visible()
  assert '0x4037D364' in page.locator('#diagFiveBootDetails').text_content()
  page.locator('#diagFiveBootDetails details').first.locator('summary').click()
  assert '0x4037D364' in page.locator('#diagFiveBootDetails').inner_text()
  page.locator('[data-diag-show="diagFiveRadio"]').click()
  assert page.locator('#diagFiveBoot').is_hidden()
  assert 'SPI 2' in page.locator('#diagFiveRadioDetails').inner_text()
  page.locator('[data-diag-show="diagFiveLab"]').click()
  assert page.locator('#settingsPanelSystemtrace').is_visible(),width
  # Normal detailed TX probe is deliberately inside the existing Advanced disclosure.
  assert page.locator('#systemTraceProbe').count()==1
  page.locator('[data-log-view="all"]').click()
  assert page.locator('#logbookTimeline').is_visible(),width
  assert page.locator('#logbookDiagnostics').is_hidden(),width
  page.locator('[data-page="settings"]:visible').first.click()
  assert page.locator('.settings-card-title:visible').count()==3,width
  page.screenshot(path=f'/mnt/data/RFLink_v055_Instellingen_Dark_{width}.png',full_page=True)
  assert page.locator('[data-settings-open="diagnostics"]').count()==0,width
  assert page.locator('[data-settings-open="systemtrace"]').count()==0,width
  assert not errors,(width,errors)
  browser.close()
  print('DIAG55 Browser / settings / logbook PASS',width,flush=True)
