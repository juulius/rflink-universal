from pathlib import Path
from playwright.sync_api import sync_playwright
import json
R=Path(__file__).resolve().parents[1]
html=(R/'ui/index.html').read_text().replace('/*__RFLINK_LOCALES__*/',(R/'ui/locales.bundle.json').read_text())
script=html.split('<script>',1)[1].split('</script>',1)[0]
static=html.replace('<script>'+script+'</script>','')
profiles=[]
for path in (R/'profiles').glob('*.json'):
 p=json.loads(path.read_text());profiles.append({**p,'source':'builtin','learn_help_i18n':p['learn_help'],'notes_i18n':p['notes'],'setup_steps_i18n':p.get('setup_steps',{}),'pair_method':p['pairing']['method'],'pair_tx_required':p['pairing']['required_tx']})
for width in (1440,390):
 with sync_playwright() as pw:
  browser=pw.chromium.launch(executable_path='/usr/bin/chromium',headless=True,args=['--no-sandbox','--disable-dev-shm-usage'])
  page=browser.new_page(viewport={'width':width,'height':900})
  errors=[];page.on('pageerror',lambda e: errors.append(str(e)))
  page.set_content(static)
  page.evaluate('''profiles=>{
   window._d=[{uid:'dev_1',name:'Ventilator box',protocol:'RamsesII',channel:2,ha_enabled:false,tx_enabled:false,tx_eligible:false,profile_id:'',rf_identities:['29:20'],actions:['status'],fields:[],pending_fields:[],pending_actions:[]},
      {uid:'dev_2',name:'Itho Fan',protocol:'IthoRFT',channel:2,ha_enabled:false,tx_enabled:false,tx_eligible:true,profile_id:'itho_rft_5360124',rf_identities:['29:10'],actions:['low','medium'],fields:[],pending_fields:[],pending_actions:[],itho:{paired:true},tx_ready_actions:['low','medium']}];
   window.fetch=async(url,opt={})=>{
    let out={ok:true};const path=String(url);
    if(path==='/rflink/api/language')out={language:'nl'};
    else if(path==='/rflink/api/status')out={channels:[],activity:[],storage_ok:true,storage_ready:true,decoder_count:13,expected_decoders:13};
    else if(path==='/rflink/api/devices')out={devices:window._d,candidates:[],storage_ok:true,mqtt_connected:false,tx_controls_enabled:true};
    else if(path==='/rflink/api/decoders')out={protocols:[{name:'RamsesII',learn_category:'control',tx_actions:'auto|low|medium'},{name:'IthoRFT',learn_category:'control',tx_actions:'low|medium'}],ha_fields:[]};
    else if(path==='/rflink/api/profile-catalog'||path.startsWith('/rflink/api/profile-catalog?id='))out={profiles};
    else if(path==='/rflink/api/user-library')out={profiles,decoders:[],write_token:'test'};
    else if(path==='/rflink/api/library')out={attention:[],attention_count:0,items:[]};
    else if(path==='/rflink/api/user-tx')out={ok:true};
    return {ok:true,json:async()=>out,text:async()=>JSON.stringify(out)};
   };
  }''',profiles)
  page.evaluate(script)
  page.wait_for_function("window.rfUserLibraryCache?.profiles?.length===26",timeout=3000)
  page.evaluate("document.querySelector('.side [data-page=devices]').click()")
  page.locator('[data-device-open="dev_1"]').wait_for(timeout=3000)
  assert page.locator('.device-overview').count()==2
  assert page.locator('.device-setup-card').count()==0
  assert page.locator('[data-device-open="dev_2"]').count()==1
  page.locator('[data-device-open="dev_1"]').click()
  page.locator('#deviceDetailBack').wait_for(timeout=3000)
  assert page.locator('.device-detail-card').count()==1
  assert page.locator('#deviceList .device-unconfigured').count()==1
  assert page.locator('[data-device-profile="dev_1"]').is_visible()
  assert page.locator('[data-profile-open="dev_1"]').count()==0
  assert page.locator('[data-pair-open="dev_1"]').count()==0
  # Full user journey: back to list, configured details, wizard remains accessible
  page.locator('#deviceDetailBack').click()
  page.locator('[data-device-open="dev_2"]').click()
  page.locator('[data-pair-open="dev_2"]').wait_for(timeout=3000)
  # Check live rebuild retains selection
  page.evaluate("document.dispatchEvent(new Event('rflink-user-library-changed'))")
  page.wait_for_timeout(300)
  assert page.locator('#deviceDetailTitle').inner_text()=='Itho Fan'
  assert page.locator('[data-pair-open="dev_2"]').count()==1
  assert not errors,errors
  print(f'DeviceUX4 {width}px: overview/setup/paired, 2-click setup, refresh PASS')
  browser.close()
