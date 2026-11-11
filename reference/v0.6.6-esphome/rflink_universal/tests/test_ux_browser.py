"""Actual Chromium UX flow against a deterministic local mock; no device/network writes."""
from pathlib import Path
from playwright.sync_api import sync_playwright

root = Path(__file__).resolve().parents[1]
html = (root / 'ui/index.html').read_text().replace('/*__RFLINK_LOCALES__*/', (root / 'ui/locales.bundle.json').read_text())
script = html.split('<script>', 1)[1].split('</script>', 1)[0]
static = html.replace('<script>' + script + '</script>', '')

with sync_playwright() as runner:
    browser = runner.chromium.launch(headless=True, executable_path='/usr/bin/chromium', args=['--no-sandbox', '--disable-dev-shm-usage'])
    page = browser.new_page(viewport={'width': 1280, 'height': 850})
    page.set_default_timeout(2300)
    errors = []
    page.on('pageerror', lambda e: errors.append(str(e)))
    page.set_content(static)
    page.evaluate('''() => {
      const originalTimeout=window.setTimeout, originalInterval=window.setInterval;
      window.setTimeout=(fn,delay,...args)=>originalTimeout(fn,delay===5000?60:delay,...args);
      window.setInterval=(fn,delay,...args)=>originalInterval(fn,delay===3000?350:delay,...args);
      window.__state={missing:[0,1,2], storageOk:true,decoderCount:12,expected:12, saved:[], candidate:null, events:[], wifi:true,mqtt:false};
      window.fetch=async (url,opt={})=>{
       const s=window.__state;
       if(!s.wifi)return {ok:false,text:async()=> 'offline'};
       let out={ok:true};
       if(url==='/rflink/api/language')out={language:'nl'};
       else if(url==='/rflink/api/status')out={uptime_ms:100001,total_frames:0,decoded_frames:0,unknown_frames:0,
         decoder_count:s.decoderCount,expected_decoders:s.expected,storage_ready:true,storage_ok:s.storageOk,saved_devices:s.saved.length,
         led:{status:'radio_ontbreekt',brightness:20,quiet:false},activity:s.events,
         channels:[['433.42','433.42 MHz'],['433.92','433.92 MHz'],['868.30 Itho','868.30 MHz']].map(([name,label],i)=>({index:i,name,frequency_label:label,
          checked:true,available:!s.missing.includes(i),rx_ready:!s.missing.includes(i),tx_ready:!s.missing.includes(i),frames:0,age_ms:null,rssi_dbm:null,mode:i===2?'PACKET':'RX'}))};
       else if(url==='/rflink/api/devices')out={devices:s.saved,candidates:s.candidate?[s.candidate]:[],storage_ok:s.storageOk,mqtt_connected:s.mqtt};
       else if(url==='/rflink/api/decoders')out={decoders:Array(s.decoderCount).fill('Decoder'),ha_fields:[{key:'temperature',label:'Temperatuur'},{key:'humidity',label:'Luchtvochtigheid'}],protocols:[{name:'TestProtocol',pack:'test',learn_category:'sensor',learn_band:'433,92 MHz',learn_instruction:'Wacht op sensormetingen.',learn_ha_result:'Temperatuur',learn_warning:'Testdecoder.',tx_encoder:'none',tx_status:'disabled',tx_registered:false,rx_actions:'measurement',fields:'temperature',tx_actions:''}]};
       else if(url.startsWith('/rflink/api/developer')){if(url.includes('action=listen'))s.candidate={identity:'test:123',channel:1,protocol:'TestProtocol',device_type:'sensor',observations:3,ready:true,age_ms:0,suggest_uid:'',suggest_name:'',ha_available:true,actions:['measurement'],fields:[{key:'temperature',value:'21',unit:'°C'}]};out={channel:1,mode:'listen',best:null}}
       else if(url.startsWith('/rflink/api/candidate?')){
         if(!s.storageOk)return {ok:false,text:async()=> 'storage failed'};
         const q=new URLSearchParams(url.split('?')[1]);const name=q.get('name')||'Unnamed';
         s.saved.push({uid:'dev_00001',name,protocol:'TestProtocol',device_type:'sensor',actions:['measurement'],identities:2,rf_identities:['test:123','test:old2'],ha_enabled:q.get('ha')==='1',pairing_supported:false,fields:[{key:'temperature',value:'21',unit:'°C'}],pending_actions:[],pending_fields:[]});s.candidate=null;out={ok:true,uid:'dev_00001',ha_requested:q.get('ha')==='1',ha_enabled:q.get('ha')==='1'};
       }else if(url.startsWith('/rflink/api/device?')){
         const q=new URLSearchParams(url.split('?')[1]);if(q.get('action')==='rename')s.saved[0].name=q.get('name');
         if(q.get('action')?.startsWith('approve_')){
           const kind=q.get('action').slice(8), key=q.get('key'),dev=s.saved[0];
           if(kind==='action'){dev.pending_actions=dev.pending_actions.filter(x=>x!==key);dev.actions.push(key)}
           else {const val=dev.pending_fields.find(x=>x.key===key);dev.pending_fields=dev.pending_fields.filter(x=>x.key!==key);dev.fields.push(val)}
         }
         if(q.get('action')?.startsWith('remove_')){
           const kind=q.get('action').slice(7),key=q.get('key'),dev=s.saved[0];
           if(kind==='action')dev.actions=dev.actions.filter(x=>x!==key);
           else dev.fields=dev.fields.filter(x=>x.key!==key);
         }
         if(q.get('action')==='delete')s.saved=[];
         if(q.get('action')==='unlink_identity'){
           if(s.saved[0].rf_identities.length<=1)return {ok:false,text:async()=> 'last identity'};
           s.saved[0].rf_identities=s.saved[0].rf_identities.filter(x=>x!==q.get('identity'));
           s.saved[0].identities=s.saved[0].rf_identities.length;
         }
         if(q.get('action')==='replace_identity'){
           if(!s.candidate||!s.candidate.ready||q.get('replacement')!==s.candidate.identity)return {ok:false,text:async()=> 'not ready'};
           s.saved[0].rf_identities=s.saved[0].rf_identities.map(x=>x===q.get('identity')?q.get('replacement'):x);
           s.candidate=null;
         }
       }else if(url.startsWith('/rflink/api/led?')){}else throw Error('unhandled '+url);
       return {ok:true,json:async()=>out};
      }
    }''')
    page.evaluate(script)
    page.wait_for_function("document.documentElement.lang === 'nl'")
    page.wait_for_timeout(200)
    assert 'Hardware niet compleet' in page.locator('#healthTitle').inner_text()
    page.locator('.side button[data-page="learn"]').click()
    assert page.locator('#wizardStart').is_disabled()
    assert page.locator('#learnRadios button[disabled]').count() == 3
    page.evaluate('window.__state.missing=[];window.__state.mqtt=true')
    page.wait_for_timeout(350)
    assert page.locator('#wizardStart').is_enabled()
    assert page.locator('#healthTitle').inner_text() == 'Alles gereed'
    page.locator('#learnRadios button[data-ch="1"]').click()
    page.locator('#wizardStart').click()
    assert page.locator('#radioSelectCard').is_hidden(), 'Frequency choice should disappear once learning starts'
    assert 'Achtergrond meten' in page.locator('#wizardTitle').inner_text()
    page.wait_for_timeout(180)
    assert any(x in page.locator('#wizardTitle').inner_text() for x in ('Bedien je apparaat','Apparaat gevonden','Controleer en bewaar')), page.locator('#wizardTitle').inner_text()
    page.wait_for_timeout(350)
    assert 'Controleer en bewaar' in page.locator('#wizardTitle').inner_text(), page.locator('#wizardTitle').inner_text()
    assert page.locator('#wizardHaWrap').is_visible(), 'MQTT on + supported candidate must show HA checkbox'
    assert page.locator('#wizardHa').is_checked(), 'HA publication should be default opt-in on confirmed enrollment'
    assert page.locator('#wizardTechnicalDetails').get_attribute('open') is None, 'Protocol details stay Advanced'
    assert page.locator('#wizardRetry').is_visible(), 'A simple retry action should be available next to Save'
    page.locator('#wizardName').fill('Sensor woonkamer')
    page.locator('#wizardConfirm').click()
    page.wait_for_timeout(200)
    assert 'Apparaat opgeslagen' in page.locator('#wizardDone').inner_text()
    assert page.locator('#radioSelectCard').is_visible(), 'After saving, starting another device should be straightforward'
    page.locator('.side button[data-page="devices"]').click()
    assert 'Sensor woonkamer' in page.locator('#deviceList').inner_text()
    assert page.evaluate('window.__state.saved[0].ha_enabled'), 'Explicit device save with checked HA option must request HA'
    # New decoder output is proposed, NEVER approved or published without a user click.
    page.evaluate("window.__state.saved[0].pending_actions=['measurement'];window.__state.saved[0].pending_fields=[{key:'humidity',value:'54',unit:'%'}]")
    page.locator('.side button[data-page="devices"]').click()
    page.wait_for_timeout(130)
    page.locator('[data-device-open="dev_00001"]').click()
    assert page.locator('[data-device-capabilities]').get_attribute('open') is not None, 'New functions must be visible without extra clicks'
    assert 'Luchtvochtigheid' in page.locator('[data-device-capabilities]').inner_text()
    assert page.locator('[data-feature="approve_action"]').count()==0, 'Weather packet actions must not appear as HA events'
    assert page.locator('[data-feature="approve_field"]').count()==1
    page.locator('[data-feature="approve_field"]').click()
    page.wait_for_timeout(130)
    assert page.evaluate("window.__state.saved[0].fields.some(f=>f.key==='humidity')")
    page.once('dialog',lambda dialog:dialog.accept())
    page.locator('.device-approved-list summary').click()
    page.locator('[data-feature="remove_field"][data-feature-key="humidity"]').click()
    page.wait_for_timeout(150)
    assert page.evaluate("!window.__state.saved[0].fields.some(f=>f.key==='humidity')")
    assert page.evaluate("window.__state.saved[0].fields.some(f=>f.key==='temperature')"), 'Removing humidity must not remove temperature'
    page.evaluate('window.__state.mqtt=false')
    page.locator('.side button[data-page="devices"]').click()
    page.wait_for_timeout(130)
    page.locator('[data-device-open="dev_00001"]').click()
    assert page.locator('[data-feature="remove_field"]').first.is_disabled(), 'Offline MQTT must disable removal of linked capability'
    page.evaluate('window.__state.mqtt=true')
    page.locator('.side button[data-page="devices"]').click()
    page.wait_for_timeout(130)
    page.locator('[data-device-open="dev_00001"]').click()
    page.once('dialog',lambda dialog:dialog.accept('Sensor slaapkamer'))
    page.locator('[data-device-more]').locator('summary').first.click()
    page.locator('[data-rename]').click()
    page.wait_for_timeout(150)
    assert 'Sensor slaapkamer' in page.locator('#deviceDetailTitle').inner_text()
    page.locator('[data-device-signals]').locator('summary').click()
    assert page.locator('[data-unlink-id]').count() == 2
    assert page.locator('[data-replace-id]').first.is_disabled(), 'No candidate must prevent arbitrary ID replacement'
    page.evaluate("window.__state.candidate={identity:'test:replacement',channel:1,protocol:'TestProtocol',device_type:'sensor',observations:3,ready:true,age_ms:0,suggest_uid:'dev_00001',suggest_name:'Sensor slaapkamer'}")
    page.wait_for_timeout(400)
    page.locator('[data-replace-select]').first.select_option('test:replacement')
    page.once('dialog',lambda dialog:dialog.accept())
    page.locator('[data-replace-id]').first.click()
    page.wait_for_timeout(170)
    assert page.evaluate("window.__state.saved[0].rf_identities.includes('test:replacement')")
    assert not page.evaluate("window.__state.saved[0].rf_identities.includes('test:123')")
    assert page.locator('[data-device-signals]').get_attribute('open') is not None
    page.once('dialog',lambda dialog:dialog.accept())
    page.locator('[data-unlink-id]').last.click()
    page.wait_for_timeout(160)
    assert page.evaluate('window.__state.saved[0].rf_identities.length') == 1
    assert page.locator('[data-unlink-id]').first.is_disabled(), 'Never unlink the final RF identity'
    page.once('dialog',lambda dialog:dialog.accept())
    page.locator('[data-device-danger]').locator('summary').click()
    page.locator('[data-delete]').click()
    page.wait_for_timeout(150)
    assert 'Nog geen bevestigde apparaten' in page.locator('#deviceList').inner_text()
    page.evaluate('window.__state.decoderCount=0')
    page.wait_for_function("document.querySelector('#healthTitle').textContent==='Decoderbibliotheek niet compleet'",timeout=2000)
    page.evaluate('window.__state.storageOk=false')
    page.wait_for_function("document.querySelector('#healthTitle').textContent==='Opslag heeft aandacht nodig'",timeout=2000)
    page.locator('.side button[data-page="learn"]').click()
    assert page.locator('#wizardStart').is_disabled(), 'Storage failure must block device enrollment'
    assert not errors, errors
    print('UX Chromium: HA enrollment checkbox, pending approval, isolated sensor removal, broker-offline guard, RF-code replacement, storage faults: PASS')
    browser.close()
