"""NAV3: reach important settings tools within three clicks without duplicate accordion opening.
Browser fixture: no radios or ESP32 required.
"""
import json
from pathlib import Path
from playwright.sync_api import sync_playwright
root=Path(__file__).resolve().parents[1]
html=(root/'ui/index.html').read_text(encoding='utf-8').replace('/*__RFLINK_LOCALES__*/',(root/'ui/locales.bundle.json').read_text())
script=html.split('<script>',1)[1].split('</script>',1)[0]
static=html.replace('<script>'+script+'</script>','')

with sync_playwright() as pw:
 browser=pw.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox','--disable-dev-shm-usage'])
 for width,height in ((1680,900),(1280,800),(390,844)):
  p=browser.new_page(viewport={'width':width,'height':height})
  errors=[];p.on('pageerror', lambda err:errors.append(str(err)))
  p.set_content(static)
  p.evaluate('''()=>{
   window.fetch=async (url,opts={})=>{
    let out={ok:true};
    if(url==='/rflink/api/language')out={language:'nl'};
    else if(url==='/rflink/api/status')out={uptime_ms:1200000,total_frames:50,decoded_frames:0,unknown_frames:0,decoder_count:0,expected_decoders:0,storage_ready:true,storage_ok:true,saved_devices:0,activity:[],led:{status:'ready',brightness:20,quiet:false},channels:[{index:0,name:'433.42',frequency_label:'433.42 MHz',mode:'RX',checked:true,available:true,rx_ready:true,tx_ready:true,frames:10,age_ms:100}]};
    else if(url==='/rflink/api/decoders')out={decoders:[],protocols:[],ha_fields:[]};
    else if(url==='/rflink/api/library')out={schema:1,persistent_user_library:true,official_decoders:0,user_decoders:0,official_profiles:0,user_profiles:0,attention_count:0,safe_resolution_count:0,archive_count:0,attention:[],items:[]};
    else if(url==='/rflink/api/user-library')out={schema:1,write_token:'token',decoders:[],profiles:[],protocols:[],radio_profiles:[]};
    else if(url==='/rflink/api/devices')out={devices:[{uid:'device_1',name:'Example device',protocol:'Example',rf_identities:[],actions:['on'],fields:[],pending_actions:[],pending_fields:[],ha_enabled:true,tx_enabled:false,tx_eligible:false}],candidates:[],storage_ok:true,mqtt_connected:true};
    else if(url==='/rflink/api/developer')out={mode:'idle',best:null,captures:[]};
    else if(url.startsWith('/rflink/api/rx-log'))out={events:[],total:0,has_more:false,signals:[0,0,0],noise:0};
    else if(url.startsWith('/rflink/api/'))out={ok:true};
    return {ok:true,json:async()=>out,text:async()=>''};
   };
  }''')
  p.evaluate(script)
  p.wait_for_function("document.documentElement.lang==='nl'")
  nav='.side' if width>680 else '.bottom'
  # From Status (1 click): diagnostics actually visible, not a closed accordion.
  p.locator('#healthCard [data-open-details="diagnosticsDetails"]').click()
  assert p.locator('.page[data-view="settings"]').is_visible()
  assert p.locator('#settingsPanelDiagnostics').is_visible()
  assert p.locator('#diagnosticsDetails').get_attribute('open') is not None
  assert p.locator('#diagOverview').is_visible()
  # Return via 1 clear back action; then Settings -> Diagnostics is 2 clicks from Status.
  p.locator('#settingsPanelDiagnostics [data-settings-back]').click()
  assert p.locator('#settingsPanelDiagnostics').is_hidden()
  p.locator('[data-settings-open="diagnostics"]').click()
  assert p.locator('#diagOverview').is_visible()
  p.locator('[data-settings-open="diagnostics"]').click()
  assert p.locator('#settingsPanelDiagnostics').is_hidden()
  # Custom settings should show editable content at once, without a second summary click.
  p.locator('[data-settings-open="custom"]').click()
  assert p.locator('#userLibraryDetails').get_attribute('open') is not None
  assert p.locator('#userProfileName').is_visible()
  # Settings -> Custom -> Create decoder -> builder shown: 2 clicks from Settings.
  p.locator('#settingsPanelCustom [data-launch-decoder]').click()
  assert p.locator('.page[data-view="learn"]').is_visible()
  assert p.locator('#expertLearn').is_visible()
  assert p.locator('#userDecoderName').is_visible()
  # Settings -> decoder library -> Create decoder: no intermediate advanced disclosure.
  p.locator(nav+' [data-page="settings"]').click()
  p.locator('[data-settings-open="decoders"]').click()
  assert p.locator('#decoderLibraryCard').is_visible()
  p.locator('#settingsPanelDecoders [data-launch-decoder]').click()
  assert p.locator('#userDecoderName').is_visible()
  # Settings -> Manage HA devices exposes each device's HA toggle without a further accordion click.
  p.locator(nav+' [data-page="settings"]').click()
  p.locator('.page[data-view="settings"] [data-focus-devices="ha"]').click()
  assert p.locator('.page[data-view="devices"]').is_visible()
  assert p.locator('[data-ha-toggle="device_1"]').is_visible()
  # Across all sizes, no horizontal overflow of the document.
  assert p.evaluate('document.documentElement.scrollWidth <= innerWidth+2'),(width,p.evaluate('document.documentElement.scrollWidth'))
  assert not errors,(width,errors)
  p.close()
 browser.close()
print('NAV3 UX regression: diagnostics 1 click from Status, direct Custom/Decoder views, 3-click decoder creation, back and responsive layouts: PASS')
