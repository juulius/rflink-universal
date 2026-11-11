"""Somfy RTS UI regression: simple pairing and control, technical state stays Advanced."""
import json
from pathlib import Path
from playwright.sync_api import sync_playwright

root=Path(__file__).resolve().parents[1]
html=(root/'ui/index.html').read_text().replace('/*__RFLINK_LOCALES__*/',(root/'ui/locales.bundle.json').read_text())
script=html.split('<script>',1)[1].split('</script>',1)[0]
static=html.replace('<script>'+script+'</script>','')
protocol={
    'name':'SomfyRTS56','pack':'somfy_rts_56','learn_category':'control','learn_band':'433,42 MHz',
    'learn_instruction':'Press the same button several times.','learn_ha_result':'Received and paired controls.',
    'learn_warning':'Pair RFLink first.','tx_encoder':'somfy_rts_56','tx_status':'host_tested','tx_registered':True,
    'rx_actions':'up|down|my|prog','fields':'rolling_code','tx_actions':'up|down|my|prog'
}

def install(page):
    page.evaluate('''protocol=>{
      window.__posts=[];window.__paired=false;window.confirm=()=>true;
      const device=()=>({uid:'dev_00560',name:'Zonnescherm',protocol:'SomfyRTS56',profile_id:'somfy_telis1_rts',device_type:'cover_remote',
        local_tx_enabled:false,age_ms:1000,identities:1,rf_identities:['somfy_rts:112233'],pairing_supported:true,transmit_supported:true,
        ha_enabled:false,tx_enabled:false,tx_eligible:window.__paired,actions:['up','down','my','prog'],fields:[],pending_actions:[],pending_fields:[],
        somfy:window.__paired?{configured:true,paired:true,address:66051,next_rolling_code:2}:{configured:false,paired:false,address:0,next_rolling_code:0}});
      window.fetch=async(url,opt={})=>{
        window.__posts.push(String(url));
        if(String(url).includes('action=somfy_pair'))window.__paired=true;
        let out={};
        if(url==='/rflink/api/language')out={language:'nl'};
        else if(url==='/rflink/api/status')out={uptime_ms:100000,total_frames:1,decoded_frames:1,unknown_frames:0,decoder_count:1,expected_decoders:1,storage_ready:true,storage_ok:true,saved_devices:1,activity:[],led:{status_code:'ready',brightness:20,quiet:false},channels:[{index:0,name:'433.42',frequency_label:'433.42 MHz',mode:'RX/TX',checked:true,available:true,rx_ready:true,tx_ready:true,frames:1,age_ms:1000}]};
        else if(url==='/rflink/api/devices')out={storage_ok:true,mqtt_connected:false,mqtt_configured:false,tx_controls_enabled:true,devices:[device()],candidates:[]};
        else if(url==='/rflink/api/decoders')out={decoders:['somfy_rts_56'],protocols:[protocol],ha_fields:[]};
        else if(url==='/rflink/api/library')out={schema:1,persistent_user_library:true,official_decoders:1,official_profiles:2,user_decoders:0,user_profiles:0,attention_count:0,safe_resolution_count:0,archive_count:0,attention:[],items:[]};
        else if(url==='/rflink/api/user-library')out={schema:1,write_token:'fixture',decoders:[],profiles:[],protocols:[{id:'SomfyRTS56',name:'Somfy RTS',source:'builtin',actions:['up','down','my','prog']}],radio_profiles:[]};
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
    panel=page.locator('.somfy-control')
    assert panel.count()==1
    assert 'RFLink koppelen' in panel.inner_text()
    assert page.locator('[data-native-command]').count()==0,'Movement controls stay hidden before pairing'
    page.locator('[data-somfy-pair]').click();page.wait_for_timeout(180)
    assert page.locator('[data-native-command]').count()==3,'Up/My/Down appear after pairing'
    assert 'Somfy-bediening klaar om te testen' in panel.inner_text()
    tech=panel.locator('details.decoder-tech')
    assert tech.get_attribute('open') is None,'Virtual address/counter stay Advanced'
    assert '010203' not in panel.inner_text(),'Technical transmitter state is hidden while Advanced is closed'
    tech.locator('summary').click()
    assert '010203' in panel.inner_text() and '2' in panel.inner_text()
    page.locator('[data-native-command="up"]').click();page.wait_for_timeout(80)
    assert any('action=native_send' in u and 'command=up' in u for u in page.evaluate('window.__posts'))
    assert page.evaluate('document.documentElement.scrollWidth<=window.innerWidth+2')
    assert not errors,errors
    browser.close()
print('Somfy UI Chromium: one-step pairing, simple Up/My/Down controls and Advanced-only rolling state: PASS')
