"""Itho profile-driven control: pairing then commands without duplicate legacy panels."""
import json
from pathlib import Path
from playwright.sync_api import sync_playwright

root=Path(__file__).resolve().parents[1]
html=(root/'ui/index.html').read_text().replace('/*__RFLINK_LOCALES__*/',(root/'ui/locales.bundle.json').read_text())
script=html.split('<script>',1)[1].split('</script>',1)[0]
static=html.replace('<script>'+script+'</script>','')
protocol={
    'name':'IthoRFT','pack':'itho_rft','learn_category':'control','learn_band':'868,30 MHz',
    'learn_instruction':'Press a speed button several times.','learn_ha_result':'Speed and timer events.',
    'learn_warning':'Pair RFLink once for control.','tx_encoder':'itho_rft','tx_status':'host_tested','tx_registered':True,
    'rx_actions':'standby|low|medium|high|timer_10|timer_20|timer_30','fields':'counter','tx_actions':'standby|low|medium|high|timer_10|timer_20|timer_30'
}

def install(page):
    page.evaluate('''protocol=>{
      window.__posts=[];window.__paired=false;window.confirm=()=>true;
      const device=()=>({uid:'dev_itho',name:'Ventilatie',protocol:'IthoRFT',profile_id:'itho_rft_5360124',device_type:'ventilation_remote',
        local_tx_enabled:false,age_ms:1000,identities:1,rf_identities:['itho_rft:112233'],pairing_supported:true,transmit_supported:true,
        ha_enabled:false,tx_enabled:false,tx_eligible:window.__paired,actions:['standby','low','medium','high','timer_10','timer_20','timer_30'],fields:[],pending_actions:[],pending_fields:[],
        tx_ready_actions:window.__paired?['low','medium','high','timer_10','timer_20','timer_30']:[],
        itho:window.__paired?{configured:true,paired:true,address:66051,next_sequence:2,device_type:22}:{configured:false,paired:false,address:0,next_sequence:0,device_type:0}});
      window.fetch=async(url,opt={})=>{
        window.__posts.push(String(url));
        if(String(url).includes('action=itho_pair'))window.__paired=true;
        if(String(url).includes('action=itho_unpair'))window.__paired=false;
        let out={};
        if(url==='/rflink/api/language')out={language:'nl'};
        else if(url==='/rflink/api/status')out={uptime_ms:100000,total_frames:1,decoded_frames:1,unknown_frames:0,decoder_count:1,expected_decoders:1,storage_ready:true,storage_ok:true,saved_devices:1,activity:[],led:{status_code:'ready',brightness:20,quiet:false},channels:[{index:2,name:'868.30 Itho',frequency_label:'868.30 MHz',mode:'RX/TX',checked:true,available:true,rx_ready:true,tx_ready:true,frames:1,age_ms:1000}]};
        else if(url==='/rflink/api/devices')out={storage_ok:true,mqtt_connected:false,mqtt_configured:false,tx_controls_enabled:true,devices:[device()],candidates:[]};
        else if(url==='/rflink/api/decoders')out={decoders:['itho_rft'],protocols:[protocol],ha_fields:[]};
        else if(url==='/rflink/api/library')out={schema:1,persistent_user_library:true,official_decoders:1,official_profiles:1,user_decoders:0,user_profiles:0,attention_count:0,safe_resolution_count:0,archive_count:0,attention:[],items:[]};
        else if(url==='/rflink/api/user-library')out={schema:1,write_token:'fixture',decoders:[],profiles:[{id:'itho_rft_5360124',name:'Itho Daalderop RFT',protocol:'IthoRFT',controls:['low','medium','high','timer_10','timer_20','timer_30'],pair_method:'receiver_learn',setup_steps_i18n:{nl:['Zet de ontvanger in leerstand','Koppel RFLink']} }],protocols:[{id:'IthoRFT',name:'Itho RFT',source:'builtin',actions:['standby','low','medium','high','timer_10','timer_20','timer_30']}],radio_profiles:[]};
        else if(url.startsWith('/rflink/api/'))out={ok:true};
        return {ok:true,json:async()=>out,text:async()=>''};
      };
    }''', protocol)

with sync_playwright() as pw:
    browser=pw.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox','--disable-dev-shm-usage'])
    page=browser.new_page(viewport={'width':390,'height':844})
    errors=[];page.on('pageerror',lambda e:errors.append(str(e)))
    page.set_content(static);install(page);page.evaluate(script)
    page.wait_for_function("document.documentElement.lang==='nl'")
    page.locator('.bottom button[data-page="devices"]').click();page.wait_for_timeout(150)
    page.locator('[data-device-open="dev_itho"]').click()  # DeviceUX4: setup is on device detail
    card=page.locator('[data-device-uid="dev_itho"]')
    assert card.count()==1
    assert card.locator('[data-pair-open]').count()==1, 'Guided pairing must be discoverable'
    assert card.locator('[data-profile-command]:not([disabled])').count()==0, 'No send before pairing'
    assert card.locator('.device-more').count()==0, 'Old nested management accordion must be gone'
    assert card.locator('[data-ha-toggle]').is_visible(), 'HA settings must be discoverable without another accordion'
    # Existing hardware endpoint: pairing is guarded by confirmation. Mock returns paired.
    page.locator('[data-itho-pair][data-mode="standard"]').click();page.wait_for_timeout(220)
    card=page.locator('[data-device-uid="dev_itho"]')
    for command in ('low','medium','high','timer_10','timer_20','timer_30'):
      assert card.locator('[data-profile-command][data-command="'+command+'"]:not([disabled])').count()==1,command
    assert card.locator('[data-pair-open]').count()==1
    page.locator('[data-profile-command][data-command="high"]').click();page.wait_for_timeout(120)
    assert any('action=native_send' in u and 'command=high' in u for u in page.evaluate('window.__posts'))
    assert page.evaluate('document.documentElement.scrollWidth<=window.innerWidth+2')
    assert not errors,errors
    browser.close()
print('Itho UI Chromium: one-step pairing, profile-controlled speeds/timers and RX-only safety: PASS')
