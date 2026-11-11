"""Chromium integration for guided setup and TX safety (no radio TX occurs)."""
from pathlib import Path
from playwright.sync_api import sync_playwright
import json
R=Path(__file__).resolve().parents[1]
html=(R/'ui/index.html').read_text().replace('/*__RFLINK_LOCALES__*/',(R/'ui/locales.bundle.json').read_text())
script=html.split('<script>',1)[1].split('</script>',1)[0]
static=html.replace('<script>'+script+'</script>','')
profiles=[]
for path in (R/'profiles').glob('*.json'):
 p=json.loads(path.read_text());profiles.append({**p,'source':'builtin','learn_help_i18n':p['learn_help'], 'notes_i18n':p['notes'],'setup_steps_i18n':p.get('setup_steps',{}),'pair_method':p['pairing']['method'],'pair_tx_required':p['pairing']['required_tx']})
proto=[]
for name in sorted(set(p['protocol'] for p in profiles)):
 proto.append({'name':name,'pack':name.lower(),'learn_category':'control','learn_band':'868,30 MHz','learn_instruction':'Press three times','learn_ha_result':'RX sensor','learn_warning':'No RF TX','rx_actions':'status|mode_command','tx_actions':'','fields':'','tx_encoder':'none','tx_status':'disabled','tx_registered':False})
base={'storage_ok':True,'mqtt_configured':False,'mqtt_connected':False,'tx_controls_enabled':True,'candidates':[]}
orcon={'uid':'dev_1','name':'Ventilatiebox','protocol':'RamsesII','profile_id':'orcon_mvs15r_15rf','channel':2,'ha_enabled':False,'transmit_enabled':False,'tx_enabled':False,'tx_eligible':False,'actions':['status'],'fields':[],'identities':1,'rf_identities':['29:204810'],'tx_ready_actions':[],'profile_suggestions':[], 'pending_actions':[],'pending_fields':[]}
itho={**orcon,'uid':'dev_2','name':'Itho test','protocol':'IthoRFT','profile_id':'itho_rft_5360124','channel':2,'pairing_supported':True,'itho':{'paired':False},'actions':['low'],'tx_ready_actions':[]}

def boot(page):
 page.evaluate('''args=>{
 const {profiles,protocols,orcon,itho,base}=args;
 window.pairTestCalls=[];
 window.fetch=async(url,opt={})=>{
 let d={ok:true};
 if(url==='/rflink/api/language')d={language:'nl'};
 else if(url==='/rflink/api/status')d={uptime_ms:70000,decoder_count:13,expected_decoders:13,storage_ready:true,storage_ok:true,saved_devices:2,channels:[],activity:[]};
 else if(url==='/rflink/api/devices')d={...base,devices:[orcon,itho]};
 else if(url==='/rflink/api/decoders')d={protocols,decoders:protocols.map(x=>x.name),ha_fields:[]};
 else if(url==='/rflink/api/library')d={persistent_user_library:true,official_decoders:13,official_profiles:26,attention_count:0,attention:[],items:[]};
 else if(String(url)==='/rflink/api/profile-catalog'||String(url).startsWith('/rflink/api/profile-catalog?id='))out={profiles};
 else if(url==='/rflink/api/user-library')d={schema:1,write_token:'test',profiles,decoders:[],protocols:[]};
 else if(url.startsWith('/rflink/api/')){window.pairTestCalls.push(url); d={ok:true};}
 return {ok:true,json:async()=>d,text:async()=>''};
 };}''',dict(profiles=profiles,protocols=proto,orcon=orcon,itho=itho,base=base))
 page.evaluate(script)
 page.wait_for_function('window.rfUserLibraryCache?.profiles?.length===26')
 page.evaluate("document.querySelector('[data-page=devices]').click()")
 page.wait_for_function('document.querySelectorAll("[data-device-open]").length===2')

with sync_playwright() as pw:
 browser=pw.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox','--disable-dev-shm-usage'])
 for width in (1300,390):
  page=browser.new_page(viewport={'width':width,'height':840})
  errs=[];page.on('pageerror',lambda e:errs.append(str(e)))
  page.set_content(static);boot(page)
  page.locator('[data-device-open="dev_1"]').click()
  page.locator('[data-pair-open="dev_1"]').click()
  dlg=page.locator('#universalPairModal');assert dlg.is_visible()
  # Proceed through instructions; Orcon MUST never emit or claim pairing success.
  for i in range(12):
   if dlg.locator('[data-pair-next]').count(): dlg.locator('[data-pair-next]').click()
   else:break
  assert '1FC9' in dlg.inner_text(),dlg.inner_text()
  assert dlg.locator('[data-pair-action]').count()==0
  assert page.evaluate('window.pairTestCalls.length')==0
  dlg.locator('[data-pair-cancel]').click()
  page.locator('#deviceDetailBack').click()
  page.locator('[data-device-open="dev_2"]').click()
  page.locator('[data-pair-open="dev_2"]').click()
  assert 'Itho' in dlg.inner_text()
  assert not errs,errs
  page.close()
 browser.close()
print('PAIRING1 browser: Orcon blocked, wizard guidance, Itho selectable, 390px, no invented TX PASS')
