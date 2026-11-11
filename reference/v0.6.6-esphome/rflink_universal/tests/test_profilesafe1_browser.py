"""ProfileSAFE1: lightweight catalog survives an unavailable user library;
device removal requires explicit confirmation and remains on the overview.
"""
import json
from pathlib import Path
from playwright.sync_api import sync_playwright
R=Path(__file__).resolve().parents[1]
html=(R/'ui/index.html').read_text().replace('/*__RFLINK_LOCALES__*/',(R/'ui/locales.bundle.json').read_text())
script=html.split('<script>',1)[1].split('</script>',1)[0]
static=html.replace('<script>'+script+'</script>','')
profiles=[]
for f in (R/'profiles').glob('*.json'):
 p=json.loads(f.read_text())
 profiles.append({'id':p['id'],'name':p['name'],'protocol':p['protocol'],'manufacturer':p['manufacturer'],'family':p['family'],'controls':p.get('controls',[]),'source':'builtin','pair_method':p['pairing']['method']})
assert len(profiles)==26
with sync_playwright() as pw:
 browser=pw.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox','--disable-dev-shm-usage'])
 for width in (1440,390):
  page=browser.new_page(viewport={'width':width,'height':880})
  page.on('dialog',lambda dialog: dialog.dismiss())
  errors=[];page.on('pageerror',lambda e:errors.append(str(e)+' STACK '+str(getattr(e,'stack',None))))
  page.set_content(static)
  page.evaluate('''profiles=>{
    window._dev={uid:'dev_00001',name:'ventilator box',protocol:'RamsesII',channel:2,profile_id:'',actions:[],fields:[],pending_fields:[],pending_actions:[],rf_identities:['29:20'],ha_enabled:false,tx_eligible:false};
    window._deleteRequests=0;
    window.fetch=async(url,opts={})=>{
      const path=String(url);let data={},status=200;
      if(path==='/rflink/api/language')data={language:'nl'};
      else if(path==='/rflink/api/status')data={channels:[],activity:[],decoder_count:13,expected_decoders:13,storage_ok:true};
      else if(path==='/rflink/api/devices')data={devices:[window._dev],candidates:[],storage_ok:true};
      else if(path==='/rflink/api/decoders')data={protocols:[{name:'RamsesII',rx_actions:'mode_command',tx_actions:'auto|low|high',learn_category:'control'}],ha_fields:[]};
      else if(path==='/rflink/api/profile-catalog'||path.startsWith('/rflink/api/profile-catalog?id='))data={profiles};
      else if(path==='/rflink/api/user-library'){status=503;data={ok:false,error:'temporarily unavailable'}}
      else if(path==='/rflink/api/library')data={official_profiles:26,items:[],attention:[]};
      else if(path.startsWith('/rflink/api/device'))window._deleteRequests++;
      return {ok:status===200,status,json:async()=>data,text:async()=>JSON.stringify(data)};
    };
   }''',profiles)
  page.evaluate(script)
  page.wait_for_function('window.rfUserLibraryCache?.profiles?.length===26',timeout=3500)
  page.locator('.side [data-page="settings"]' if width>850 else '.bottom [data-page="settings"]').click()
  page.locator('[data-settings-open="decoders"]').first.click()
  page.locator('#catalogProfiles').click()
  assert page.locator('[data-catalog-profile]').count()==26
  page.locator('.side [data-page="devices"]' if width>850 else '.bottom [data-page="devices"]').click()
  page.locator('[data-device-open="dev_00001"]').wait_for(timeout=2000)
  assert page.locator('[data-delete="dev_00001"]').count()==1
  page.locator('[data-delete="dev_00001"]').click()
  page.wait_for_timeout(80)
  assert page.evaluate('window._deleteRequests')==0,'Cancel should never send device DELETE'
  page.locator('[data-device-open="dev_00001"]').click()
  page.locator('[data-device-profile="dev_00001"]').wait_for(timeout=2000)
  assert page.locator('[data-device-profile="dev_00001"] option').count()==17
  assert not errors, errors
  page.close()
 browser.close()
print('PROFILESAFE1 Browser: independent 26-profile catalog, 16 Orcon options, delete confirmation, desktop/mobile PASS')
