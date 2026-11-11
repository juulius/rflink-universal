"""Chromium regression for four locales, dynamic panels and saved preference.

Browser/API mock only. No claim of physical NVS verification. The test deliberately
uses disconnected radios so the same dynamic states seen on a radio-less bench setup
are covered in every supported language.
"""
import json
from pathlib import Path
from playwright.sync_api import sync_playwright

root = Path(__file__).resolve().parents[1]
html = (root / 'ui/index.html').read_text(encoding='utf-8').replace(
    '/*__RFLINK_LOCALES__*/', (root / 'ui/locales.bundle.json').read_text(encoding='utf-8'))
script = html.split('<script>', 1)[1].split('</script>', 1)[0]
static = html.replace('<script>' + script + '</script>', '')
catalog = {lang: json.loads((root / 'ui/locales' / f'{lang}.json').read_text())
           for lang in ('en', 'nl', 'fr', 'de')}
assert all(set(catalog['en']) == set(x) for x in catalog.values())

with sync_playwright() as pw:
    browser = pw.chromium.launch(headless=True, executable_path='/usr/bin/chromium',
                                 args=['--no-sandbox', '--disable-dev-shm-usage'])
    page = browser.new_page(viewport={'width': 1280, 'height': 800})
    errors = []
    page.set_default_timeout(3500)
    page.on('pageerror', lambda e: errors.append(str(e)))
    page.set_content(static)
    page.evaluate('''() => {
      window.__lang={saved:'en',writes:[],fail:false};
      window.fetch=async (url,opt={})=>{
        let out={};
        if (url==='/rflink/api/language') out={language:window.__lang.saved};
        else if (url.startsWith('/rflink/api/language?')) {
          let v=new URLSearchParams(url.split('?')[1]).get('language');
          window.__lang.writes.push(v);
          if(window.__lang.fail) return {ok:false,text:async()=> 'test write failure'};
          window.__lang.saved=v;out={ok:true,language:v};
        } else if(url==='/rflink/api/status')out={uptime_ms:70000,total_frames:0,decoded_frames:0,unknown_frames:0,
          decoder_count:12,expected_decoders:12,storage_ready:true,storage_ok:true,saved_devices:0,activity:[],
          led:{status:'ok',brightness:20,quiet:false},channels:[
            {index:0,frequency_label:'433.42 MHz',name:'433.42',mode:'RX',checked:true,available:false,rx_ready:false,tx_ready:false,frames:0,age_ms:null},
            {index:1,frequency_label:'433.92 MHz',name:'433.92',mode:'RX',checked:true,available:false,rx_ready:false,tx_ready:false,frames:0,age_ms:null},
            {index:2,frequency_label:'868.30 MHz',name:'868.30 Itho',mode:'PACKET',checked:true,available:false,rx_ready:false,tx_ready:false,frames:0,age_ms:null}
          ]};
        else if(url==='/rflink/api/rx-log-config')out={ok:true,signal_limits:[250,250,250],noise_limit:2000,signals:[0,0,0],noise_total:0,write_token:123};
        else if(url.startsWith('/rflink/api/rx-log?'))out={ok:true,events:[],next_cursor:0,has_more:false};
        else if(url==='/rflink/api/devices')out={devices:[],candidates:[],storage_ok:true,mqtt_connected:false,mqtt_configured:false};
        else if(url==='/rflink/api/decoders')out={protocols:[],decoders:[],ha_fields:[]};
        else if(url.startsWith('/rflink/api/developer'))out={mode:'idle',best:null};
        else throw Error('Unexpected URL '+url);
        return {ok:true,json:async()=>out};
      };
    }''')
    page.evaluate(script)
    page.wait_for_function("document.querySelector('#uiLanguage').value === 'en'")
    page.locator('.side button[data-page="settings"]').click()
    for lang in ('nl', 'fr', 'de', 'en'):
        page.locator('.side button[data-page="settings"]').click()
        page.locator('#uiLanguage').select_option(lang)
        page.wait_for_function('(lang)=>document.documentElement.lang===lang && window.__lang.saved===lang', arg=lang)
        assert page.locator('.side button[data-page="devices"]').inner_text() == catalog[lang]['static.005']
        assert page.locator('.side button[data-page="settings"]').inner_text() == catalog[lang]['static.008']
        assert page.locator('#languageSaveStatus').inner_text() == catalog[lang]['language.saved']
        assert page.locator('#uiLanguage').input_value() == lang
        assert page.locator('#ledStatusSettings').inner_text() == catalog[lang]['led.ready']
        # Dynamic status/radio text must follow the same language, not stay hard-coded Dutch.
        page.locator('.side button[data-page="status"]').click()
        page.wait_for_timeout(80)
        assert page.locator('#healthTitle').text_content() == catalog[lang]['dyn.hardware_incomplete']
        assert page.locator('#ledStatus').text_content() == catalog[lang]['led.ready']
        assert page.locator('#radioCards .rf60-radio-row').count()==3
        assert page.locator('#rf60Radios').inner_text() == '0 / 3'
        assert catalog[lang]['diag55.offline'] in page.locator('#radioCards').inner_text()
        page.locator('.side button[data-page="learn"]').click()
        page.wait_for_timeout(80)
        assert catalog[lang]['dyn.no_working_receivers'] in page.locator('#learnRadioHelp').inner_text()
        # The idle learning wizard is static UI too: it must never stay English after a language change.
        assert page.locator('#wizardTitle').inner_text() == catalog[lang]['static.057']
        assert page.locator('#wizardHelp').inner_text() == catalog[lang]['static.058']
        assert page.locator('#wizardStart').inner_text() == catalog[lang]['static.059']
        assert page.locator('#baselineState').inner_text() == catalog[lang]['static.074']
        assert page.locator('#signalLabel').inner_text() == catalog[lang]['dyn.waiting_capture']
        page.locator('.side button[data-page="monitor"]').click()
        page.wait_for_timeout(80)
        assert catalog[lang]['dyn.all_radios'] in page.locator('#monitorRadios').inner_text()
        assert catalog[lang]['dyn.no_monitor_activity'] in page.locator('#monitorActivity').inner_text()
        page.locator('.side button[data-page="devices"]').click()
        page.wait_for_timeout(80)
        assert catalog[lang]['dyn.storage_ok'] in page.locator('#deviceStorageNotice').inner_text()
        assert catalog[lang]['dyn.no_devices'] in page.locator('#deviceList').inner_text()
        assert page.locator('#deviceName').get_attribute('placeholder') == catalog[lang]['placeholder.device_name']
    page.locator('.side button[data-page="settings"]').click()
    page.evaluate('window.__lang.fail=true')
    page.locator('#uiLanguage').select_option('fr')
    page.wait_for_function('(expected)=>document.querySelector("#languageSaveStatus").textContent === expected', arg=catalog['en']['language.failed'])
    assert page.locator('#uiLanguage').input_value() == 'en', 'Failed write must roll back selection'
    assert page.evaluate('window.__lang.saved') == 'en', 'Failed write must not touch previous preference'
    assert page.evaluate('window.__lang.writes') == ['nl', 'fr', 'de', 'en', 'fr']
    assert not errors, errors
    browser.close()
print('Localization Chromium: four languages, dynamic panels, server saves, DOM/HTML lang, failed-write rollback: PASS')
