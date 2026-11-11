"""Integration of experimental 1FC9 UI path with mocked ESP32 responses, no RF."""
from pathlib import Path
from playwright.sync_api import sync_playwright
import json
R=Path(__file__).resolve().parents[1]
html=(R/'ui/index.html').read_text().replace('/*__RFLINK_LOCALES__*/',(R/'ui/locales.bundle.json').read_text())
script=html.split('<script>',1)[1].split('</script>',1)[0]
static=html.replace('<script>'+script+'</script>','')
profiles=[]
for f in (R/'profiles').glob('*.json'):
 p=json.loads(f.read_text());profiles.append({**p,'source':'builtin','learn_help_i18n':p['learn_help'],'notes_i18n':p['notes'],'setup_steps_i18n':p.get('setup_steps',{}),'pair_method':p['pairing']['method'],'pair_tx_required':p['pairing']['required_tx']})
protos=[{'name':'RamsesII','learn_category':'control','learn_band':'868.30 MHz','learn_instruction':'Test','rx_actions':'mode_command|status','tx_actions':'away|auto|low|medium|high|timer_15|timer_30|timer_60','fields':'','tx_encoder':'ramses_ii','tx_status':'host_tested','tx_registered':True}]
with sync_playwright() as pw:
 browser=pw.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox','--disable-dev-shm-usage'])
 for width in (1440,390):
  page=browser.new_page(viewport={'width':width,'height':900})
  errs=[];page.on('pageerror',lambda x:errs.append(str(x)))
  page.set_content(static)
  page.evaluate('''args=>{
  window.orconState={stage:'not_paired',sent:false,lab:true,commands:[]};
  const {profiles,protos}=args;
  window.confirm=()=>true;
  window.fetch=async(url,opt={})=>{
   const s=window.orconState;let data={ok:true};
   if(url==='/rflink/api/language')data={language:'nl'};
   else if(url==='/rflink/api/status')data={uptime_ms:100000,storage_ready:true,storage_ok:true,saved_devices:1,decoder_count:13,expected_decoders:13,channels:[],activity:[]};
   else if(url==='/rflink/api/devices')data={storage_ok:true,tx_controls_enabled:true,mqtt_connected:false,devices:[{uid:'dev_o',name:'Ventilatiebox',protocol:'RamsesII',profile_id:'orcon_mvs15r_15rf',channel:2,ha_enabled:false,tx_enabled:false,tx_eligible:s.stage==='paired',actions:['mode_command'],fields:[],rf_identities:['29:204810'],pending_actions:[],pending_fields:[],tx_ready_actions:s.stage==='paired'?['auto','low','medium']:[],ramses:{lab_enabled:s.lab,pair_stage:s.stage,adapter_ready:true,test_sent:s.sent}}],candidates:[]};
   else if(url==='/rflink/api/decoders')data={protocols:protos,decoders:['RamsesII'],ha_fields:[]};
   else if(url==='/rflink/api/library')data={attention:[],attention_count:0,items:[]};
   else if(String(url)==='/rflink/api/profile-catalog'||String(url).startsWith('/rflink/api/profile-catalog?id='))out={profiles};
   else if(url==='/rflink/api/user-library')data={schema:1,write_token:'x',profiles,decoders:[]};
   else if(url.includes('action=ramses_pair&')){s.stage='waiting_ack';s.commands.push('pair');}
   else if(url.includes('action=ramses_pair_test&')){s.sent=true;s.commands.push('test');}
   else if(url.includes('action=ramses_pair_verify&')){s.stage='paired';s.commands.push('verify');}
   else if(url.startsWith('/rflink/api/'))data={ok:true};
   return {ok:true,json:async()=>data,text:async()=>''};
  }
 }''',{'profiles':profiles,'protos':protos})
  page.evaluate(script)
  page.wait_for_function('window.rfUserLibraryCache?.profiles?.length===26')
  page.evaluate("document.querySelector('[data-page=devices]').click()")
  page.locator('[data-device-open="dev_o"]').click()
  page.locator('[data-pair-open="dev_o"]').click()
  dlg=page.locator('#universalPairModal')
  for _ in range(15):
   if dlg.locator('[data-pair-action="ramses_pair"]').count():break
   dlg.locator('[data-pair-next]').first.click()
  assert dlg.locator('[data-pair-action="ramses_pair"]').count()==1
  dlg.locator('[data-pair-action="ramses_pair"]').click()
  page.wait_for_function("window.orconState.commands.includes('pair')")
  assert dlg.locator('[data-pair-verify]').count()==0, 'must not verify before ACK/test'
  page.evaluate("window.orconState.stage='awaiting_user'")
  dlg.locator('[data-pair-check]').click()
  assert dlg.locator('[data-pair-test]').count()==1
  dlg.locator('[data-pair-test]').click()
  page.wait_for_function("window.orconState.commands.includes('test')")
  assert dlg.locator('[data-pair-verify]').count()==1
  dlg.locator('[data-pair-verify]').click()
  page.wait_for_function("window.orconState.commands.includes('verify')")
  assert not errs,errs
  assert page.evaluate('window.orconState.commands')==['pair','test','verify']
  page.close()
 browser.close()
print('Orcon experimental wizard: 1FC9 -> ACK wait -> explicit Auto test -> user verification, 1440/390px PASS')
