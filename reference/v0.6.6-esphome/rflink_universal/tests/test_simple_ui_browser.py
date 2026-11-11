"""Regression for the deliberately simple/default RFLink UI.

Checks that everyday pages do not duplicate advanced controls, that technical sections
stay collapsed until requested, and that mobile does not immediately open a large
decoder detail panel.
"""
import json
from pathlib import Path
from playwright.sync_api import sync_playwright

root=Path(__file__).resolve().parents[1]
html=(root/'ui/index.html').read_text().replace('/*__RFLINK_LOCALES__*/',(root/'ui/locales.bundle.json').read_text())
script=html.split('<script>',1)[1].split('</script>',1)[0]
static=html.replace('<script>'+script+'</script>','')
protocol={'name':'Nexa','pack':'nexa','learn_category':'control','learn_band':'433 MHz','learn_instruction':'Press a button several times.','learn_ha_result':'Approved on/off actions.','learn_warning':'Verify TX with hardware.','tx_encoder':'nexa','tx_status':'software_tested','tx_registered':True,'rx_actions':'on|off','fields':'','tx_actions':'on|off'}

def install_fixture(page):
    page.evaluate('''protocol=>{window.fetch=async(url,opt={})=>{let out={};
      if(url==='/rflink/api/language')out={language:'nl'};
      else if(url==='/rflink/api/status')out={uptime_ms:7200000,total_frames:3,decoded_frames:3,unknown_frames:0,decoder_count:1,expected_decoders:1,storage_ready:true,storage_ok:true,saved_devices:0,activity:[],led:{status:'gereed',status_code:'ready',brightness:20,quiet:false},channels:[{index:0,name:'433.42',frequency_label:'433.42 MHz',mode:'RX',checked:true,available:true,rx_ready:true,tx_ready:false,frames:3,age_ms:1000}]};
      else if(url==='/rflink/api/devices')out={storage_ok:true,mqtt_connected:false,mqtt_configured:false,devices:[],candidates:[]};
      else if(url==='/rflink/api/decoders')out={decoders:['nexa'],protocols:[protocol],ha_fields:[]};
      else if(url==='/rflink/api/library')out={schema:1,persistent_user_library:true,official_decoders:1,official_profiles:0,user_decoders:0,user_profiles:0,attention_count:0,safe_resolution_count:0,archive_count:0,attention:[],items:[]};
      else if(url==='/rflink/api/user-library')out={schema:1,write_token:'fixture',decoders:[],profiles:[],protocols:[{id:'Nexa',name:'Nexa',source:'builtin',actions:['on','off']}],radio_profiles:[]};
      else if(url.startsWith('/rflink/api/developer'))out={mode:'idle',best:null,captures:[]};
      else if(url.startsWith('/rflink/api/'))out={ok:true};
      return {ok:true,json:async()=>out,text:async()=>''};};}''',protocol)

with sync_playwright() as pw:
    browser=pw.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox','--disable-dev-shm-usage'])
    page=browser.new_page(viewport={'width':1920,'height':1080})
    errors=[];page.on('pageerror',lambda e:errors.append(str(e)))
    page.set_content(static);install_fixture(page);page.evaluate(script)
    page.wait_for_function("document.documentElement.lang==='nl'");page.wait_for_timeout(150)
    assert page.locator('.taskgrid.simple .task').count()==2,'Status should expose only two everyday quick actions'
    assert page.locator('#ledStatus').inner_text()=='Gereed','LED state belongs in the main health summary and is localized'
    page.locator('.side button[data-page="settings"]').click();page.wait_for_timeout(100)
    assert page.locator('.settings-primary>.card').count()==3,'Language, Home Assistant and LED are the three primary settings cards'
    assert page.locator('#libraryAdvancedPanel').is_hidden(),'Library details stay Advanced by default'
    assert page.locator('#libUserStat').is_hidden() and page.locator('#libProfileStat').is_hidden(),'Zero custom item counts are hidden'
    assert page.locator('#libAttentionStat').is_hidden(),'Zero attention count is hidden'
    assert page.locator('#userLibraryDetails').get_attribute('open') is None,'Custom protocol/profile editor stays collapsed'
    assert page.locator('#diagnosticsDetails').get_attribute('open') is None,'Diagnostics stay collapsed'
    assert page.locator('summary').filter(has_text='MQTT instellen').first.is_visible()
    assert page.locator('summary').filter(has_text='LED-kleuren en gedrag').first.is_visible()
    # The Status -> diagnostics shortcut must reveal the section it promises to open.
    page.locator('.side button[data-page="status"]').click();page.locator('[data-open-details="diagnosticsDetails"]').click();page.wait_for_timeout(120)
    assert page.locator('[data-view="settings"]').get_attribute('class').endswith('active')
    assert page.locator('#diagnosticsDetails').get_attribute('open') is not None
    assert not errors,errors
    page.close()

    mobile=browser.new_page(viewport={'width':390,'height':844})
    errs=[];mobile.on('pageerror',lambda e:errs.append(str(e)))
    mobile.set_content(static);install_fixture(mobile);mobile.evaluate(script)
    mobile.wait_for_function("document.documentElement.lang==='nl'");mobile.wait_for_timeout(120)
    mobile.locator('.bottom button[data-page="settings"]').click();mobile.wait_for_timeout(80)
    assert mobile.locator('.decoder-select.active').count()==0,'Mobile library should not expand a decoder before the user picks one'
    assert mobile.locator('#decoderDetail').is_hidden(),'Mobile keeps decoder details out of the way until a decoder is selected'
    mobile.locator('[data-settings-open="decoders"]').click()  # decoder library is a named settings tool
    mobile.locator('.decoder-select').first.click()
    assert 'Nexa' in mobile.locator('#decoderDetail').inner_text()
    mobile.locator('.bottom button[data-page="learn"]').click();mobile.wait_for_timeout(80)
    assert mobile.locator('#wizardTitle').inner_text()=='Apparaat toevoegen'
    assert mobile.locator('#wizardStart').inner_text()=='Start'
    assert mobile.locator('#learnAdvancedMode').get_attribute('open') is None
    assert mobile.locator('[data-learn="decoder"]').is_hidden(), 'Decoder builder stays behind Advanced'
    assert '433,92' in mobile.locator('#learnRadioHelp').inner_text()
    assert mobile.evaluate('document.documentElement.scrollWidth<=window.innerWidth+2')
    assert not errs,errs
    mobile.close();browser.close()
print('Simple UI Chromium: two daily actions, compact settings, Advanced-on-demand, simple Add device defaults and localized learner: PASS')
