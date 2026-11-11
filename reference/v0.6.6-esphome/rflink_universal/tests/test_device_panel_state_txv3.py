"""Browser regression: device detail panels persist through the 3s polling/rebuild."""
from pathlib import Path
from playwright.sync_api import sync_playwright
r=Path(__file__).resolve().parents[1]
html=(r/'ui/index.html').read_text().replace('/*__RFLINK_LOCALES__*/',(r/'ui/locales.bundle.json').read_text())
script=html.split('<script>',1)[1].split('</script>',1)[0]
static=html.replace('<script>'+script+'</script>','')
with sync_playwright() as p:
    b=p.chromium.launch(executable_path='/usr/bin/chromium',headless=True,args=['--no-sandbox','--disable-dev-shm-usage'])
    for width in (1300,390):
        page=b.new_page(viewport={'width':width,'height':840})
        errors=[]; page.on('pageerror',lambda err:errors.append(str(err)))
        page.set_content(static)
        page.evaluate('''() => {
         window.fetch=async url=>{
           let out={};const path=String(url);
           if(path==='/rflink/api/language')out={language:'nl'};
           else if(path==='/rflink/api/status')out={channels:[],activity:[],storage_ok:true,decoder_count:1,expected_decoders:1};
           else if(path==='/rflink/api/devices')out={devices:[{uid:'dev_1',name:'Ventilator box',protocol:'RamsesII',channel:2,ha_enabled:false,tx_enabled:false,tx_eligible:false,transmit_enabled:false,profile_id:'orcon_mvs15r_15rf',rf_identities:['29:204810'],actions:['status'],fields:[],pending_fields:[],pending_actions:[]}],candidates:[],storage_ok:true,mqtt_connected:false};
           else if(path==='/rflink/api/decoders')out={protocols:[{name:'RamsesII',learn_category:'control',learn_band:'868.30 MHz',learn_instruction:'Test',rx_actions:'status',tx_status:'disabled'}],ha_fields:[]};
           else if(path==='/rflink/api/user-library')out={profiles:[],decoders:[],protocols:[]};
           return {ok:true,json:async()=>out,text:async()=>JSON.stringify(out)};
         };
        }''')
        page.evaluate(script)
        page.evaluate("document.querySelector('[data-page=devices]').click()")
        page.locator('#deviceList .card.device').first.wait_for()
        page.locator('[data-device-open="dev_1"]').click()
        target=page.locator('#deviceList .card.device .decoder-tech').first
        target.locator('summary').first.click()
        assert target.evaluate('(e)=>e.open')
        page.wait_for_timeout(4200)
        page.wait_for_timeout(100)
        assert page.locator('#deviceList .card.device .decoder-tech').first.evaluate('(e)=>e.open'), 'details collapsed after refresh'
        # Open the top level management drawer and check both survive together.
        manage=page.locator('#deviceList .card.device .device-more').first
        manage.locator('summary').first.click()
        page.wait_for_timeout(4200)
        page.wait_for_timeout(100)
        assert page.locator('#deviceList .card.device .decoder-tech').first.evaluate('(e)=>e.open')
        assert page.locator('#deviceList .card.device .device-more').first.evaluate('(e)=>e.open')
        assert not errors,errors
        page.close()
    b.close()
print('Device panels persist on desktop and mobile: PASS')
