"""Responsive desktop workspace regression for wide settings/developer UI."""
import json
from pathlib import Path
from playwright.sync_api import sync_playwright

root = Path(__file__).resolve().parents[1]
html = (root / 'ui/index.html').read_text().replace(
    '/*__RFLINK_LOCALES__*/', (root / 'ui/locales.bundle.json').read_text()
)
script = html.split('<script>', 1)[1].split('</script>', 1)[0]
static = html.replace('<script>' + script + '</script>', '')

protocol = {
    'name': 'TestProtocol', 'pack': 'test', 'learn_category': 'control',
    'learn_band': '433,92 MHz', 'learn_instruction': 'Press each button three times.',
    'learn_ha_result': 'Buttons', 'learn_warning': 'Test protocol.',
    'tx_encoder': 'test', 'tx_status': 'software_tested', 'tx_registered': True,
    'rx_actions': 'on|off', 'fields': '', 'tx_actions': 'on|off'
}
fixture = {'decoders': ['test'], 'protocols': [protocol], 'ha_fields': []}

with sync_playwright() as runner:
    browser = runner.chromium.launch(
        headless=True, executable_path='/usr/bin/chromium',
        args=['--no-sandbox', '--disable-dev-shm-usage']
    )
    for width, height in ((1920, 1080), (1280, 900), (390, 844)):
        page = browser.new_page(viewport={'width': width, 'height': height})
        errors = []
        page.on('pageerror', lambda e: errors.append(str(e)))
        page.set_content(static)
        page.evaluate('''fixture => {
          window.fetch=async (url,opt={})=>{
            let out={ok:true};
            if(url==='/rflink/api/language')out={language:'nl'};
            else if(url==='/rflink/api/decoders')out=fixture;
            else if(url==='/rflink/api/library')out={schema:1,persistent_user_library:true,official_decoders:1,user_decoders:1,official_profiles:0,user_profiles:1,attention_count:0,safe_resolution_count:0,archive_count:0,attention:[],items:[]};
            else if(url==='/rflink/api/user-library')out={schema:1,write_token:'token',decoders:[{id:'user_demo',name:'Demo remote',source:'user',version:1,enabled:true,radio:1,kind:'raw',radio_profile:'board_raw',actions:['on','off'],rx_source:'RAWCOUNT 10\\nEND',tx_source:'MARKUS 100\\nSPACEUS 200\\nEND',samples:[]}],profiles:[{id:'demo_profile',name:'Demo profile',source:'user',protocol:'user_demo',expected_actions:['on','off'],learn_help:'Press each button three times'}],protocols:[],radio_profiles:[]};
            else if(url==='/rflink/api/devices')out={devices:[],candidates:[],storage_ok:true,mqtt_connected:true};
            else if(url==='/rflink/api/status')out={uptime_ms:1000,total_frames:0,decoded_frames:0,unknown_frames:0,decoder_count:1,expected_decoders:1,storage_ready:true,storage_ok:true,saved_devices:0,activity:[],led:{status:'ok',brightness:20,quiet:false},channels:[{index:0,name:'433.42',frequency_label:'433.42 MHz',checked:true,available:true,rx_ready:true,tx_ready:true,frames:0,age_ms:null},{index:1,name:'433.92',frequency_label:'433.92 MHz',checked:true,available:true,rx_ready:true,tx_ready:true,frames:0,age_ms:null},{index:2,name:'Itho',frequency_label:'868.30 MHz',checked:true,available:true,rx_ready:true,tx_ready:true,frames:0,age_ms:null}]};
            else if(url==='/rflink/api/developer')out={mode:'idle',best:null,captures:[]};
            else if(url.startsWith('/rflink/api/'))out={ok:true};
            return {ok:true,json:async()=>out,text:async()=>''};
          };
        }''', fixture)
        page.evaluate(script)
        page.wait_for_function("document.documentElement.lang === 'nl'")
        page.wait_for_timeout(140)

        nav = '.side button[data-page="settings"]' if width > 680 else '.bottom button[data-page="settings"]'
        page.locator(nav).click()
        page.wait_for_timeout(120)
        assert page.evaluate('document.documentElement.scrollWidth<=window.innerWidth+2'), 'Settings overflow'

        main = page.locator('.main').bounding_box()
        page.locator('[data-settings-open="decoders"]').click()
        if width == 1920:
            assert main['width'] > 1650, main
            lib = page.locator('#decoderLibraryCard').bounding_box()
            assert lib['width'] > 1500, lib
        page.locator('[data-settings-open="custom"]').click()
        panes = page.locator('.user-library-layout > div')
        assert panes.count() == 2
        left, right = panes.nth(0).bounding_box(), panes.nth(1).bounding_box()
        if width == 1920:
            assert right['x'] > left['x'] + left['width'] - 2, (left, right)
        if width == 390:
            assert abs(left['x'] - right['x']) < 3 and right['y'] > left['y'], (left, right)

        nav = '.side button[data-page="learn"]' if width > 680 else '.bottom button[data-page="learn"]'
        page.locator(nav).click()
        page.locator('#learnAdvancedMode').evaluate('(e)=>e.open=true')
        page.locator('[data-learn="decoder"]').click()
        page.wait_for_timeout(80)
        page.locator('#userAdvanced').evaluate('(e)=>e.open=true')
        basics = [page.locator('.user-basics-grid.simple-basics label').nth(i).bounding_box() for i in range(2)]
        rules = [page.locator('.rule-grid > div').nth(i).bounding_box() for i in range(2)]
        if width >= 1280:
            assert max(abs(x['y'] - basics[0]['y']) for x in basics) < 3, basics
            assert rules[1]['x'] > rules[0]['x'] + rules[0]['width'] - 2, rules
        else:
            assert basics[1]['y'] > basics[0]['y'], basics
            assert rules[1]['y'] > rules[0]['y'], rules
        assert page.evaluate('document.documentElement.scrollWidth<=window.innerWidth+2'), 'Learn overflow'
        assert not errors, errors
        page.close()
    browser.close()
print('Workspace UI Chromium: fluid 1920px desktop, 2-pane user library, simple 2-field decoder basics, side-by-side RX/TX, stacked mobile, no overflow: PASS')
