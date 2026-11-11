"""Library/update UX: quiet when clean, attention on dashboard, Advanced/help on demand."""
import json
from pathlib import Path
from playwright.sync_api import sync_playwright
root=Path(__file__).resolve().parents[1]
html=(root/'ui/index.html').read_text().replace('/*__RFLINK_LOCALES__*/',(root/'ui/locales.bundle.json').read_text())
script=html.split('<script>',1)[1].split('</script>',1)[0]
static=html.replace('<script>'+script+'</script>','')
base_status={'uptime_ms':1000,'total_frames':0,'decoded_frames':0,'unknown_frames':0,'decoder_count':1,'expected_decoders':1,'storage_ready':True,'storage_ok':True,'saved_devices':0,'activity':[],'led':{'status':'ok','brightness':20,'quiet':False},'channels':[]}
decoders={'decoders':['Example'],'protocols':[{'name':'Example','pack':'example','stable_id':'decoder:Example','source':'builtin','library_version':1,'learn_category':'development','learn_band':'Test','learn_instruction':'Test','learn_ha_result':'None','learn_warning':'Test','tx_encoder':'none','tx_status':'disabled','tx_registered':False,'rx_actions':'','fields':'','tx_actions':''}],'ha_fields':[]}
with sync_playwright() as pw:
    browser=pw.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox','--disable-dev-shm-usage'])
    page=browser.new_page(viewport={'width':390,'height':844})
    errors=[]; page.on('pageerror',lambda e:errors.append(str(e)))
    attention={'schema':1,'persistent_user_library':True,'official_decoders':12,'user_decoders':2,'official_profiles':8,'user_profiles':4,'attention_count':2,'safe_resolution_count':1,'archive_count':1,'attention':[{'id':'decoder:x'}],'items':[{'id':'decoder:Example','source':'builtin','version':1,'pack':'example'}]}
    page.set_content(static)
    page.evaluate('''([status,decoders,library])=>{window.fetch=async url=>{let out={};if(url==='/rflink/api/language')out={language:'en'};else if(url==='/rflink/api/status')out=status;else if(url==='/rflink/api/devices')out={devices:[],candidates:[],storage_ok:true,mqtt_connected:false};else if(url==='/rflink/api/developer')out={mode:'idle',best:null};else if(url==='/rflink/api/decoders')out=decoders;else if(url==='/rflink/api/library')out=library;else throw Error('Unexpected '+url);return {ok:true,json:async()=>out}}}''',[base_status,decoders,attention])
    page.evaluate(script); page.wait_for_function("document.documentElement.lang==='en'"); page.wait_for_timeout(150)
    assert page.locator('#libraryAttentionCard').is_visible()
    assert '2' in page.locator('#libraryAttentionText').inner_text()
    page.locator('#libraryAttentionOpen').click(); page.wait_for_timeout(50)
    assert page.locator('#libraryCard').is_visible()
    # Desktop readability regression: Library & updates and Decoder library both
    # intentionally span the full settings grid; decoder details get most of that width.
    page.set_viewport_size({'width':1280,'height':850}); page.wait_for_timeout(50)
    library=page.locator('#libraryCard').bounding_box()
    page.locator('[data-settings-open="decoders"]').click()
    full=page.locator('#decoderLibraryCard').bounding_box()
    selector=page.locator('#decoderSelector').bounding_box(); detail=page.locator('#decoderDetail').bounding_box()
    assert full and library and full['width'] > 900 and abs(full['width']-library['width']) < 4, (full,library)
    assert selector and detail and detail['width'] > selector['width'] * 1.45, (selector,detail)
    page.locator('[data-settings-open="library"]').click()
    page.locator('#libraryAdvanced').click(); assert page.locator('#libraryAdvancedPanel').is_visible()
    page.locator('#libraryCard [data-help="library"]').click(); assert page.locator('#helpModal').is_visible()
    assert 'library' in page.locator('#helpModalTitle').inner_text().lower()
    page.locator('#helpModalClose').click(); assert page.locator('#helpModal').is_hidden()
    assert not errors, errors
    page.close(); browser.close()
print('Library UX Chromium: dashboard attention, settings handoff, Advanced and contextual help: PASS')
