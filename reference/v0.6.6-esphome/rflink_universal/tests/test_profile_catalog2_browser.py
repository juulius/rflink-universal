"""Browser regression: all 26 profiles are discoverable by manufacturer, family, protocol; mobile and safety."""
import json
from pathlib import Path
from playwright.sync_api import sync_playwright
root=Path(__file__).resolve().parents[1]
html=(root/'ui/index.html').read_text().replace('/*__RFLINK_LOCALES__*/',(root/'ui/locales.bundle.json').read_text())
script=html.split('<script>',1)[1].split('</script>',1)[0]
static=html.replace('<script>'+script+'</script>','')
raw=[json.loads(f.read_text()) for f in (root/'profiles').glob('*.json')]
profiles=[]
for p in raw:
 profiles.append({**p,'source':'builtin','learn_help_i18n':p['learn_help'],'notes_i18n':p['notes'],'setup_steps_i18n':p.get('setup_steps',{}),'controls':p.get('controls',[])})
protocols=[{'name':name,'pack':name.lower(),'learn_category':'control','learn_band':'868,30 MHz','learn_instruction':'Press three times.','learn_ha_result':'Events','learn_warning':'Safety check.','tx_encoder':'none','tx_status':'host_tested','tx_registered':False,'rx_actions':'mode_command|status','fields':'','tx_actions':''} for name in sorted(set(p['protocol'] for p in raw))]

def setup(page):
 page.evaluate('''args=>{
 const profiles=args.profiles, protocols=args.protocols;
 window.fetch=async(url,opt={})=>{
 let out={};
 if(url==='/rflink/api/language')out={language:'nl'};
 else if(url==='/rflink/api/status')out={uptime_ms:7200000,total_frames:2,decoded_frames:2,unknown_frames:0,decoder_count:1,expected_decoders:1,storage_ready:true,storage_ok:true,saved_devices:0,activity:[],led:{status_code:'ready',brightness:20,quiet:false},channels:[]};
 else if(url==='/rflink/api/devices')out={storage_ok:true,mqtt_connected:false,mqtt_configured:false,devices:[],candidates:[]};
 else if(url==='/rflink/api/decoders')out={protocols,decoders:protocols.map(x=>x.name),ha_fields:[]};
 else if(url==='/rflink/api/library')out={persistent_user_library:true,official_decoders:protocols.length,official_profiles:profiles.length,attention_count:0,attention:[],items:[]};
 else if(String(url)==='/rflink/api/profile-catalog'||String(url).startsWith('/rflink/api/profile-catalog?id='))out={profiles};
 else if(url==='/rflink/api/user-library')out={schema:1,write_token:'fixture',decoders:[],profiles,protocols:[]};
 else if(url.startsWith('/rflink/api/'))out={ok:true};
 return {ok:true,json:async()=>out,text:async()=>''};
 };}''',{'profiles':profiles,'protocols':protocols})
 page.evaluate(script)
 page.wait_for_function('document.documentElement.lang===\'nl\' && window.rfUserLibraryCache?.profiles?.length===26')
 page.locator('.side [data-page="settings"]' if page.viewport_size["width"]>850 else '.bottom [data-page="settings"]').click()
 page.locator('[data-settings-open="decoders"]').first.click()
 page.wait_for_timeout(110)

with sync_playwright() as pw:
 browser=pw.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox','--disable-dev-shm-usage'])
 for width in (1300,390):
  page=browser.new_page(viewport={'width':width,'height':900});errors=[];page.on('pageerror',lambda e:errors.append(str(e)))
  page.set_content(static);setup(page)
  # Normal decoder view compact; related profiles are only a summary with a working jump.
  page.locator('[data-decoder-select="RamsesII"]').first.click()
  assert '16' in page.locator('#openProtocolProfiles').inner_text()
  page.locator('#openProtocolProfiles').click()
  assert page.locator('#catalogProfiles').get_attribute('aria-pressed')=='true'
  assert page.locator('#profileCatalogProtocol').input_value()=='RamsesII'
  assert page.locator('[data-catalog-profile]').count()==16
  # User can return to all profiles and narrow by manufacturer/family.
  page.locator('#catalogProfiles').click()
  assert page.locator('[data-catalog-profile]').count()==26
  page.locator('#profileCatalogBrand').select_option('Orcon')
  assert page.locator('[data-catalog-profile]').count()==16
  page.locator('#profileCatalogFamily').select_option('MVS')
  assert page.locator('[data-catalog-profile]').count()==2
  page.locator('[data-catalog-profile="orcon_mvs15r_15rf"]').click()
  d=page.locator('#decoderDetail').inner_text()
  assert 'RamsesII' in d and 'MVS' in d and 'Auto' in d and 'nog niet beschikbaar' in d,d
  page.locator('#profileOpenProtocol').click()
  assert page.locator('#catalogProtocols').get_attribute('aria-pressed')=='true'
  assert 'RamsesII' in page.locator('#decoderDetail').inner_text()
  page.locator('#catalogProfiles').click()
  page.locator('#profileCatalogSearch').fill('ventiflow')
  assert page.locator('[data-catalog-profile]').count()==1
  assert 'HRC Ventiflow' in page.locator('#decoderSelector').inner_text()
  assert page.evaluate('document.documentElement.scrollWidth<=window.innerWidth+2'), f'horizontal overflow at {width}'
  assert not errors,errors
  page.close()
 browser.close()
print('Profile catalog Chromium PASS: protocol ↔ profiles, all 26, brand/family/protocol filtering, search, 390px mobile, RX/TX safety')
