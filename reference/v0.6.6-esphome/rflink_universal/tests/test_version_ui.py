"""VERSIONUI1 regression: running firmware identity in status API and all language views."""
from pathlib import Path
import json
import re
from playwright.sync_api import sync_playwright
root = Path(__file__).resolve().parents[1]
cpp = (root/'components/rflink_universal/rflink_universal.cpp').read_text()
ident = (root/'components/rflink_universal/firmware_identity.h').read_text()
html = (root/'ui/index.html').read_text()
assert '#include "firmware_identity.h"' in cpp
assert '"firmware_version\\\":\\\""' in cpp, 'Version must be returned by API'
assert '"firmware_revision\\\":\\\""' in cpp, 'Revision must be returned by API'
assert '"firmware_built\\\":\\\""' in cpp, 'Build timestamp must be returned by API'
assert 'RFLINK_FIRMWARE_VERSION = "v0.6.4"' in ident
assert 'RFLINK_FIRMWARE_NAME = "Radio Gaga"' in ident
assert 'RFLINK_FIRMWARE_BUILD_HASH' in ident
for lang in ('nl','en','fr','de'):
    d=json.loads((root/'ui/locales'/f'{lang}.json').read_text())
    for k in ('heading','version','revision','build_more','built'):
        assert d.get('about.'+k), (lang,k)
html=html.replace('/*__RFLINK_LOCALES__*/',(root/'ui/locales.bundle.json').read_text())
js=html.split('<script>',1)[1].split('</script>',1)[0]
static=html.replace('<script>'+js+'</script>','')
with sync_playwright() as playwright:
    browser=playwright.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox','--disable-dev-shm-usage'])
    page=browser.new_page(viewport={'width':390,'height':844},accept_downloads=True)
    errors=[];page.on('pageerror',lambda e:errors.append(str(e)))
    page.set_content(static)
    page.evaluate('''() => { window.fetch=async(url,opts={})=>{const d=url==='/rflink/api/status'?{boot_session_id:1,boot_count:1,boot_history_persisted:true,boot_history:[],reset_reason:'power_on',reset_reason_code:1,uptime_ms:999999,free_heap_bytes:64000,min_free_heap_bytes:60000,total_frames:0,decoded_frames:0,unknown_frames:0,noise_frames:0,decoder_count:12,expected_decoders:12,saved_devices:0,storage_ready:true,storage_ok:true,channels:[],activity:[],led:{status_code:'ready',brightness:20,quiet:false},firmware_version:'v0.6.2',firmware_name:'Radio Gaga',firmware_build_hash:'HASH_PLACEHOLDER',firmware_revision:'v0.6.2-HASH_PLACEHOLDER',firmware_built:'Oct 24 2026 21:00:00'}:url==='/rflink/api/language'?{language:'nl'}:url==='/rflink/api/devices'?{storage_ok:true,devices:[],candidates:[]}:url==='/rflink/api/decoders'?{decoders:[],protocols:[],ha_fields:[]}:url==='/rflink/api/library'?{schema:1,persistent_user_library:true,official_decoders:0,official_profiles:0,user_decoders:0,user_profiles:0,attention_count:0,items:[]}:url==='/rflink/api/user-library'?{schema:1,decoders:[],profiles:[],protocols:[],radio_profiles:[]}:{ok:true};return {ok:true,json:async()=>d,text:async()=>''}} }''')
    page.evaluate(js)
    page.wait_for_function("document.getElementById('firmwareVersion').textContent==='v0.6.2 Radio Gaga'")
    page.locator('.bottom button[data-page="settings"]').click()
    page.locator('#firmwareBuilt').evaluate('(e)=>e.closest("details").open=true')
    assert page.locator('#firmwareRevision').inner_text()=='HASH_PLACEHOLDER'
    assert page.locator('#firmwareBuilt').inner_text()=='Oct 24 2026 21:00:00'
    for lang in ('nl','en','fr','de'):
        page.evaluate('(lang)=>rfApplyLanguage(lang)',lang)
        page.wait_for_function(f"document.documentElement.lang==='{lang}'")
        assert page.locator('#firmwareAboutCard .settings-group-title').inner_text()==json.loads((root/'ui/locales'/f'{lang}.json').read_text())['about.heading']
        assert page.locator('#firmwareVersion').inner_text()=='v0.6.2 Radio Gaga'
    assert page.evaluate('document.documentElement.scrollWidth <= window.innerWidth+1'), 'Mobile overflow'
    assert not errors,errors
    browser.close()
print('VERSIONUI v0.6.2: short firmware identity + mobile settings + 4 languages: PASS')
