"""Radio Gaga 0.6 UX audit: mock API browser journeys, no physical RF validation."""
from pathlib import Path
from playwright.sync_api import sync_playwright
import json

root=Path(__file__).resolve().parents[1]
html=(root/'ui/index.html').read_text().replace('/*__RFLINK_LOCALES__*/',(root/'ui/locales.bundle.json').read_text())
js=html.split('<script>',1)[1].split('</script>',1)[0]
static=html.replace('<script>'+js+'</script>','')
profiles=[]
for p in (root/'profiles').glob('*.json'):
    if p.name=='README.md': continue
    d=json.loads(p.read_text())
    profiles.append({**d,'source':'builtin','pair_method':d.get('pairing',{}).get('method','none')})

for size in (1440,390):
  with sync_playwright() as pw:
    browser=pw.chromium.launch(executable_path='/usr/bin/chromium',headless=True,args=['--no-sandbox','--disable-dev-shm-usage'])
    page=browser.new_page(viewport={'width':size,'height':850},color_scheme='dark')
    errors=[];page.on('pageerror',lambda e:errors.append(str(e)))
    page.set_content(static)
    page.evaluate('''profiles=>{
      window.rf60Devices=[
       {uid:'fan_orcon',name:'Ventilator box',protocol:'RamsesII',channel:2,profile_id:'orcon_mvs15r_15rf',rf_identities:['28:10'],age_ms:120000,actions:['auto'],fields:[],pending_actions:[],pending_fields:[],ha_enabled:false,tx_eligible:true,tx_enabled:false,ramses:{pair_stage:'unpaired',lab_enabled:true},tx_ready_actions:[]},
       {uid:'fan_itho',name:'Ventilatie boven',protocol:'IthoRFT',channel:2,profile_id:'itho_rft_5360124',rf_identities:['11:21'],age_ms:10000,actions:['low','medium'],fields:[],pending_actions:[],pending_fields:[],ha_enabled:false,tx_eligible:true,tx_enabled:false,itho:{paired:true},tx_ready_actions:['low','medium']},
       {uid:'sensor',name:'Buitentemperatuur',protocol:'Acurite609TXC',channel:0,profile_id:'acurite_609txc',rf_identities:['11:33'],age_ms:5500,actions:[],fields:[],pending_actions:[],pending_fields:[],ha_enabled:false,tx_eligible:false,tx_enabled:false,tx_ready_actions:[]}
      ];
      window.fetch=async(input,options={})=>{
        const u=String(input);let out={ok:true};
        if(u==='/rflink/api/language')out={language:'nl'};
        else if(u==='/rflink/api/status')out={firmware_version:'v0.6',firmware_name:'Radio Gaga',firmware_build_hash:'testhash',uptime_ms:2300000,total_frames:17,decoded_frames:5,noise_frames:6,unknown_frames:6,decoder_count:13,expected_decoders:13,saved_devices:3,storage_ready:true,storage_ok:true,boot_count:24,boot_session_id:1,boot_history:[{boot_number:24,reason:'software'},{boot_number:23,reason:'panic'}],channels:[{index:0,frequency_label:'433.42 MHz',name:'433.42',mode:'raw',available:true,rx_ready:true,tx_ready:true,checked:true,frames:3,spi_failures:0,age_ms:1000},{index:1,frequency_label:'433.92 MHz',name:'433.92',mode:'raw',available:true,rx_ready:true,tx_ready:true,checked:true,frames:0,spi_failures:0,age_ms:null},{index:2,frequency_label:'868.30 MHz',name:'868.30',mode:'packet',available:true,rx_ready:true,tx_ready:true,checked:true,frames:14,spi_failures:0,age_ms:3000}],activity:[{channel:'868,30',summary:'RX signal',age_ms:1000},{channel:'868,30',summary:'TX request',age_ms:2300}]};
        else if(u==='/rflink/api/devices')out={devices:window.rf60Devices,candidates:[],storage_ok:true,mqtt_connected:false,mqtt_configured:false,tx_controls_enabled:true};
        else if(u==='/rflink/api/decoders')out={protocols:[{name:'RamsesII',learn_category:'control',tx_actions:'auto|low|medium'},{name:'IthoRFT',learn_category:'control',tx_actions:'low|medium'},{name:'Acurite609TXC',learn_category:'sensor'}],ha_fields:[]};
        else if(u==='/rflink/api/profile-catalog'||u.startsWith('/rflink/api/profile-catalog?id='))out={profiles};
        else if(u==='/rflink/api/user-library')out={profiles,decoders:[],write_token:'test'};
        else if(u==='/rflink/api/library')out={schema:2,items:[],attention_count:0};
        else if(u==='/rflink/api/system-trace')out={events:[],server_uptime_ms:2300000};
        else if(u==='/rflink/api/rx-log-config')out={signal_limits:[250,250,250],noise_limit:2000};
        else if(u==='/rflink/api/system-trace-config')out={limit:128};
        else if(u.startsWith('/rflink/api/rx-log?'))out={events:[]};
        return {ok:true,status:200,json:async()=>out,text:async()=>JSON.stringify(out)};
      };
    }''',profiles)
    page.evaluate(js)
    page.wait_for_function("document.getElementById('rf60Radios').textContent.includes('3 / 3')")
    assert page.locator('.rf60-metric:visible').count()==4
    assert page.locator('.rf60-radio-row:visible').count()==3
    assert page.locator('#rf60Devices').inner_text()=='3'
    assert page.locator('#rf60Attention').inner_text()=='1'
    assert page.locator('.rf60-radio-row').first.is_enabled()
    page.screenshot(path=f'/mnt/data/RFLink_RadioGaga_v060_Status_{size}.png',full_page=True)
    page.locator('[data-page="devices"]:visible').first.click()
    page.locator('.rf60-device').first.wait_for()
    assert page.locator('.rf60-device').count()==3
    assert page.locator('.rf60-device .device-overview-delete').count()==0
    assert page.locator('.rf60-device .rf60-device-last').count()==3
    page.screenshot(path=f'/mnt/data/RFLink_RadioGaga_v060_Apparaten_{size}.png',full_page=True)
    # Configured but unavailable Orcon: clear setup button, no false TX-ready claim.
    assert 'Alleen ontvangst' in page.locator('[data-device-uid="fan_orcon"]').inner_text()
    page.locator('[data-device-open="fan_orcon"]').click()
    page.locator('.device-detail-card').wait_for()
    assert page.locator('.device-setup-flow').count()==1
    assert page.locator('[data-pair-open="fan_orcon"]').count()==1
    assert page.locator('.rf61-device-secondary').is_visible()
    assert page.locator('.device-secondary>summary').count()==0, 'no nested master accordion'
    assert page.locator('.rf61-info-card').count()>=4
    assert page.locator('[data-rename="fan_orcon"]').is_visible()
    assert page.locator('[data-ha-toggle="fan_orcon"]').is_visible()
    assert page.locator('[data-delete="fan_orcon"]').count()==1
    page.screenshot(path=f'/mnt/data/RFLink_RadioGaga_v061_Apparaat_{size}.png',full_page=True)
    assert page.locator('[data-device-signals]').count()>=1
    page.locator('[data-device-signals] summary').click()
    assert page.locator('[data-device-signals]').evaluate('(e)=>e.open')
    page.locator('#deviceDetailBack').click()
    page.locator('[data-device-uid="fan_itho"] .rf60-quick summary').first.click()
    assert page.locator('[data-profile-command="fan_itho"]').count()>0
    # Desktop width no horizontal overflow; mobile bottom nav can be visible.
    page.locator('[data-page="learn"]:visible').first.click()
    assert page.locator('[data-view="learn"].active').is_visible()
    page.screenshot(path=f'/mnt/data/RFLink_RadioGaga_v060_Toevoegen_{size}.png',full_page=True)
    page.locator('[data-page="settings"]:visible').first.click()
    assert page.locator('[data-view="settings"].active').is_visible()
    page.screenshot(path=f'/mnt/data/RFLink_RadioGaga_v060_Instellingen_{size}.png',full_page=True)
    assert page.evaluate('document.documentElement.scrollWidth <= innerWidth+1'),size
    assert not errors,(size,errors)
    browser.close()
    print('Radio Gaga status/devices/detail + safe quick controls PASS',size,flush=True)
