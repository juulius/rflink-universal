"""UX3 simulated user acceptance: navigation, primary actions, blocked TX, 4 locales, mobile.
No simulated API response is evidence of RF hardware success.
"""
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
protocols=[]
for name in sorted({p['protocol'] for p in profiles}):
 protocols.append({'name':name,'learn_category':'control' if name!='AcuRite609TXC' else 'sensor','rx_actions':'mode_command|status','tx_actions':'low|medium|high','tx_status':'disabled' if name=='RamsesII' else 'ready','tx_encoder':'none' if name=='RamsesII' else 'packet'})
base=[
 {'uid':'dev_orcon','name':'Ventilatiebox','protocol':'RamsesII','profile_id':'orcon_mvs15r_15rf','channel':2,'ha_enabled':False,'tx_enabled':False,'tx_eligible':False,'transmit_enabled':False,'actions':['mode_command'], 'fields':[],'pending_actions':[],'pending_fields':[],'rf_identities':['29:204810'],'tx_ready_actions':[]},
 {'uid':'dev_unknown','name':'Nieuwe afstandsbediening','protocol':'RamsesII','profile_id':'','channel':2,'ha_enabled':False,'tx_enabled':False,'tx_eligible':False,'transmit_enabled':False,'actions':['mode_command'], 'fields':[],'pending_actions':[],'pending_fields':[],'rf_identities':['29:209901'],'tx_ready_actions':[]},
 {'uid':'dev_itho','name':'Ventilatie tweede verdieping','protocol':'IthoRFT','profile_id':'itho_rft_5360124','channel':2,'ha_enabled':True,'tx_enabled':False,'tx_eligible':True,'transmit_enabled':True,'itho':{'paired':True},'actions':['low','medium','high'], 'fields':[],'pending_actions':[],'pending_fields':[],'rf_identities':['itho:112233'],'tx_ready_actions':['low','medium','high']}
]
with sync_playwright() as pw:
 b=pw.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox','--disable-dev-shm-usage'])
 for width in (1440,390):
  p=b.new_page(viewport={'width':width,'height':900}); p.set_default_timeout(4500)
  errs=[];p.on('pageerror',lambda e:errs.append(str(e)))
  p.set_content(static)
  p.evaluate('''data=>{
    window.__ux3Devices=data.devices;
    window.__ux3Writes=[];
    window.__ux3Reads=0;
    window.fetch=async(url,opts={})=>{
      const u=String(url);let out={};
      if(u==='/rflink/api/language')out={language:'nl'};
      else if(u==='/rflink/api/status')out={uptime_ms:50000,total_frames:12,decoded_frames:12,unknown_frames:0,decoder_count:13,expected_decoders:13,storage_ok:true,storage_ready:true,channels:[],activity:[]};
      else if(u==='/rflink/api/devices'){window.__ux3Reads++;out={devices:window.__ux3Devices,candidates:[],storage_ok:true,tx_controls_enabled:true,mqtt_configured:true,mqtt_connected:true};}
      else if(u==='/rflink/api/decoders')out={protocols:data.protocols,ha_fields:[]};
      else if(u==='/rflink/api/library')out={items:[],attention:[],attention_count:0};
      else if(u==='/rflink/api/user-library')out={profiles:data.profiles,decoders:[],protocols:[],write_token:'unit-test-token'};
      else if(u.startsWith('/rflink/api/')){
       window.__ux3Writes.push(u);
       const q=new URLSearchParams(opts.body ? String(opts.body) : (u.split('?')[1]||''));
       if(q.get('action')==='profile')window.__ux3Devices.find(d=>d.uid===q.get('uid')).profile_id=q.get('id');
       out={ok:true};
      }
      return {ok:true,json:async()=>out,text:async()=>JSON.stringify(out)};
    };
   }''',dict(devices=base,profiles=profiles,protocols=protocols))
  p.evaluate(script)
  p.wait_for_function('window.rfUserLibraryCache?.profiles?.length===26')
  nav='.side' if width>700 else '.bottom'
  print(width,'nav',flush=True);p.locator(nav+' [data-page="devices"]').click()
  p.locator('#deviceList .card.device').first.wait_for()
  assert p.locator('#deviceList .card.device').count()==3
  first=p.locator('[data-device-uid="dev_orcon"]')
  assert first.locator('[data-profile-command]').count()==0, 'Orcon preview buttons must not look available'
  assert first.locator('[data-pair-open]').count()==1, 'Guided setup directly visible'
  assert first.locator('.device-more').evaluate('(e)=>!e.open')
  assert first.locator('[data-profile-panel]').evaluate('(e)=>!e.open')
  print(width,'orcon setup',flush=True);first.locator('[data-pair-open]').click()
  dlg=p.locator('#universalPairModal');assert dlg.is_visible()
  assert '1FC9' in dlg.inner_text()
  for i in range(12):
   if dlg.locator('[data-pair-next]').count():dlg.locator('[data-pair-next]').click()
   else:break
  assert dlg.locator('[data-pair-action]').count()==0
  assert dlg.locator('[data-pair-end]').count()==1, 'RX-only flow may close, never claim TX success'
  dlg.locator('[data-pair-end]').click()
  assert not any('pair' in u or 'send' in u for u in p.evaluate('window.__ux3Writes'))
  missing=p.locator('[data-device-uid="dev_unknown"]')
  assert missing.locator('[data-profile-open]').count()==1
  assert missing.locator('[data-pair-open]').count()==0
  print(width,'unknown select',flush=True);missing.locator('[data-profile-open]').click()
  select=missing.locator('[data-device-profile]');assert select.is_visible()
  assert select.evaluate('(e)=>document.activeElement===e')
  # Delayed background update must not replace a dropdown currently in use.
  p.evaluate('window.__ux3Devices[1].name="Nieuwe afstandsbediening (gewijzigd)"')
  p.wait_for_timeout(3150)
  assert select.evaluate('(e)=>document.activeElement===e'), 'Polling destroyed focused profile picker'
  print(width,'profile POST',flush=True);select.select_option('orcon_mvs15r_15rf')
  p.wait_for_function('window.__ux3Devices[1].profile_id==="orcon_mvs15r_15rf"')
  p.wait_for_function('document.querySelector("[data-device-uid=dev_unknown] [data-pair-open]")!==null')
  ready=p.locator('[data-device-uid="dev_itho"]')
  assert ready.locator('[data-profile-command]:not([disabled])').count()==3
  # Primary action stays visible rather than buried in management.
  assert ready.locator('.device-operation').is_visible()
  p.once('dialog',lambda dialog:dialog.accept())
  print(width,'native command',flush=True);ready.locator('[data-profile-command][data-command="low"]').click()
  p.wait_for_timeout(500)
  print("writes",p.evaluate("window.__ux3Writes"), "toast", p.locator("#toast").inner_text(), "errors", errs,flush=True)
  assert p.evaluate('window.__ux3Writes.some(x=>x.includes("native_send"))')
  # Primary card wording must be translated in all four languages, including dynamic nodes.
  wording={'en':'Receiving only','nl':'Alleen ontvangst','fr':'Réception uniquement','de':'Nur Empfang'}
  for language,expected in wording.items():
   p.evaluate('(language)=>rfApplyLanguage(language)',language)
   p.wait_for_function("([language,expected])=>document.documentElement.lang===language && document.querySelector('[data-device-uid=\"dev_orcon\"] .device-summary')?.textContent.includes(expected)",arg=[language,expected])
   assert not p.locator('[data-device-uid="dev_orcon"] [data-profile-command]').count()
  p.evaluate('rfApplyLanguage("nl")')
  # Check no accidental horizontal scrolling and adequate clarity across widths.
  assert p.evaluate('document.documentElement.scrollWidth <= innerWidth + 2'),width
  if width==1440:p.screenshot(path='/mnt/data/RFLink_UX3_Apparaten_Desktop.png',full_page=True)
  else:p.screenshot(path='/mnt/data/RFLink_UX3_Apparaten_Mobiel.png',full_page=True)
  assert not errs,(width,errs)
  p.close()
 b.close()
print('UX3 end-user paths, TX blocked, profile select, polling focus, working command, mobile/desktop PASS')
