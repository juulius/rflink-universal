"""NAV4 browser routes, filters, MQTT security and persistence (mock API)."""
from pathlib import Path
from playwright.sync_api import sync_playwright
root=Path(__file__).resolve().parents[1]
html=(root/'ui/index.html').read_text().replace('/*__RFLINK_LOCALES__*/',(root/'ui/locales.bundle.json').read_text())
script=html.split('<script>',1)[1].split('</script>',1)[0]
static=html.replace('<script>'+script+'</script>','')
with sync_playwright() as pw:
 browser=pw.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox','--disable-dev-shm-usage'])
 for size in ((1680,900),(1280,800),(390,844)):
  p=browser.new_page(viewport={'width':size[0],'height':size[1]}); errors=[]; p.on('pageerror',lambda e:errors.append(str(e)))
  p.set_content(static)
  p.evaluate('''()=>{
   window.__posts=[];
   window.fetch=async (url,opts={})=>{
    const u=String(url);let out={ok:true};
    if(u==='/rflink/api/language')out={language:'nl'};
    else if(u==='/rflink/api/status')out={uptime_ms:120000,total_frames:45,decoded_frames:0,unknown_frames:0,decoder_count:0,storage_ready:true,storage_ok:true,saved_devices:3,activity:[],led:{status:'ready',brightness:20,quiet:false},channels:[{index:0,name:'433.42',frequency_label:'433.42 MHz',checked:true,available:true,rx_ready:true,tx_ready:true,frames:4,age_ms:100}]};
    else if(u==='/rflink/api/decoders')out={decoders:[],protocols:[],ha_fields:[]};
    else if(u==='/rflink/api/library')out={schema:1,attention:[],items:[],attention_count:0};
    else if(u==='/rflink/api/user-library')out={write_token:'123',decoders:[],profiles:[],protocols:[]};
    else if(u==='/rflink/api/devices')out={storage_ok:true,mqtt_connected:false,mqtt_configured:true,devices:[
      {uid:'a',name:'Garden sensor',protocol:'Oregon',channel:0,rf_identities:[],actions:[],fields:[],pending_actions:[],pending_fields:[]},
      {uid:'b',name:'Attic fan',protocol:'IthoRFT',channel:2,rf_identities:[],actions:[],fields:[],pending_actions:[],pending_fields:[]},
      {uid:'c',name:'Door remote',protocol:'SomfyRTS56',channel:1,rf_identities:[],actions:[],fields:[],pending_actions:[],pending_fields:[]}
    ].concat(window.__injectUnknown?[{uid:'d',name:'Spare RF',protocol:'Raw',channel:255,rf_identities:[],actions:[],fields:[],pending_actions:[],pending_fields:[]}]:[]),candidates:[]};
    else if(u==='/rflink/api/developer')out={mode:'idle',best:null,captures:[]};
    else if(u==='/rflink/api/mqtt-settings'){
      if(opts.method==='POST'){window.__posts.push({url:u,body:String(opts.body)});out={ok:true}}
      else out={ok:true,compiled:true,enabled:false,connected:false,host:'192.168.1.3',port:1883,username:'rflink',saved_password:true,write_token:123};
    }
    else if(u==='/rflink/api/rx-log-config')out={ok:true,signal_limits:[250,250,250],noise_limit:2000,signals:[0,0,0],noise_total:0,write_token:123};
    else if(u.startsWith('/rflink/api/rx-log'))out={events:[],next_cursor:0,has_more:false};
    return {ok:true,json:async()=>out,text:async()=>JSON.stringify(out)};
   };
  }''')
  p.evaluate(script)
  p.wait_for_function("document.documentElement.lang==='nl'")
  nav='.side' if size[0]>680 else '.bottom'
  p.locator(nav+' [data-page="devices"]').click()
  p.wait_for_function("document.querySelectorAll('#deviceList .card.device').length===3")
  p.locator('#deviceSearch').fill('garden')
  p.wait_for_function("document.querySelectorAll('#deviceList .card.device').length===1")
  assert 'Garden sensor' in p.locator('#deviceList').inner_text()
  # Refresh is ongoing; search and input must remain
  p.wait_for_timeout(3200)
  assert p.locator('#deviceSearch').input_value()=='garden'
  assert p.locator('#deviceList .card.device').count()==1
  p.locator('#deviceClearFilter').click()
  assert p.locator('[data-dev-radio="all"]').get_attribute('aria-pressed')=='true'
  assert '3' in p.locator('[data-radio-device-count="all"]').inner_text()
  p.locator('[data-dev-radio="2"]').click()
  p.wait_for_function("document.querySelectorAll('#deviceList .card.device').length===1")
  assert p.locator('[data-dev-radio="2"]').get_attribute('aria-pressed')=='true'
  assert p.locator('[data-dev-radio="all"]').get_attribute('aria-pressed')=='false'
  assert 'Attic fan' in p.locator('#deviceList').inner_text()
  p.wait_for_timeout(3200)
  assert p.locator('[data-dev-radio="2"]').get_attribute('aria-pressed')=='true'
  assert 'Attic fan' in p.locator('#deviceList').inner_text()
  assert p.locator('#deviceUnknownRadioChoice').is_hidden()
  # Unknown-radio devices must remain directly reachable without a dropdown.
  p.evaluate('window.__injectUnknown=true')
  p.locator('[data-dev-radio="all"]').click()
  p.wait_for_function("document.querySelectorAll('#deviceList .card.device').length===4")
  assert p.locator('#deviceUnknownRadioChoice').is_visible()
  p.locator('[data-dev-radio="unknown"]').click()
  p.wait_for_function("document.querySelectorAll('#deviceList .card.device').length===1")
  assert 'Spare RF' in p.locator('#deviceList').inner_text()
  assert p.locator('[data-dev-radio="unknown"]').get_attribute('aria-pressed')=='true'
  p.locator('#deviceClearFilter').click()
  p.wait_for_function("document.querySelectorAll('#deviceList .card.device').length===4")
  assert p.locator('[data-dev-radio="all"]').get_attribute('aria-pressed')=='true'
  if size[0]==1280:p.screenshot(path='/mnt/data/RFLink_Apparaten_Frequentiekaarten_Desktop.png')
  if size[0]==390:p.screenshot(path='/mnt/data/RFLink_Apparaten_Frequentiekaarten_Mobiel.png')
  p.locator(nav+' [data-page="settings"]').click()
  p.locator('[data-settings-open="diagnostics"]').click()
  assert '#settings/diagnostics' in p.url
  p.go_back()
  p.wait_for_function("location.hash==='#settings'")
  assert p.locator('#settingsPanelDiagnostics').is_hidden()
  p.go_back()
  p.wait_for_function("location.hash==='#devices'")
  assert p.locator('.page[data-view="devices"]').is_visible()
  p.go_forward();p.wait_for_function("location.hash==='#settings'")
  p.go_forward();p.wait_for_function("location.hash==='#settings/diagnostics'")
  assert p.locator('#diagOverview').is_visible()
  # MQTT runtime save: password may only appear in POST body, never URL.
  p.locator('[data-i18n="settings.mqtt_setup"]').first.click()
  p.locator('#mqttPassword').fill('Secret!_password')
  p.locator('#mqttEnabled').check()
  p.locator('#mqttSave').click()
  p.wait_for_function('window.__posts.length===1')
  post=p.evaluate('window.__posts[0]')
  assert 'password=Secret' in post['body'] and 'Secret' not in post['url']
  assert p.locator('#mqttPassword').input_value()==''
  assert 'Secret' not in p.locator('#mqttRuntimeWrap').inner_text()
  assert p.evaluate('document.documentElement.scrollWidth <= innerWidth+2'),size
  assert not errors,(size,errors)
  p.close()
 browser.close()
print('NAV4 browser smoke: search persist, radio filter, back/forward, MQTT POST body and responsive PASS')
