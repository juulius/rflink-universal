"""LOGBOOK1 in-browser user journeys: all/radio/system/web, filters, export and limits."""
from pathlib import Path
from playwright.sync_api import sync_playwright
R=Path(__file__).resolve().parents[1]
html=(R/'ui/index.html').read_text().replace('/*__RFLINK_LOCALES__*/',(R/'ui/locales.bundle.json').read_text())
script=html.split('<script>',1)[1].split('</script>',1)[0]
static=html.replace('<script>'+script+'</script>','')
for width in (1440,390):
 with sync_playwright() as pw:
  browser=pw.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox','--disable-dev-shm-usage'])
  page=browser.new_page(viewport={'width':width,'height':920},accept_downloads=True)
  errors=[];page.on('pageerror',lambda e:errors.append(str(e)))
  page.set_content(static)
  page.evaluate('''() => {
    window.logbookMock={sys:128,api:[],radioReads:0};
    window.fetch=async(url,options={})=>{
      const u=String(url),method=options.method||'GET';window.logbookMock.api.push(method+' '+u.split('?')[0]);
      let out={ok:true};
      if(u==='/rflink/api/language')out={language:'en'};
      else if(u==='/rflink/api/status')out={uptime_ms:20000,total_frames:3,decoded_frames:2,unknown_frames:1,channels:[],activity:[],storage_ok:true,storage_ready:true,decoder_count:13,firmware_version:'v0.3.3-legacy',firmware_revision:'legacy-UXREVIEW5',firmware_built:'Nov 1 2026'};
      else if(u==='/rflink/api/system-trace')out={ok:true,server_uptime_ms:20000,capacity:256,limit:window.logbookMock.sys,events:[
        {seq:1,ms:10000,subsystem:'learn',level:'info',uid:'devA',message:'Device learned'},
        {seq:2,ms:11000,subsystem:'tx',level:'warn',uid:'devA',message:'Missing TX encoder'}]};
      else if(u.startsWith('/rflink/api/rx-log?')){window.logbookMock.radioReads++;out={ok:true,events:[
        {seq:51,uptime_ms:11001,radio:2,category:'signal',record:{direction:'RX',summary:'RAMSES Auto',protocol:'RamsesII'}},
        {seq:52,uptime_ms:11002,radio:2,category:'noise',record:{summary:'Background noise',reason:'weak signal'}}]};}
      else if(u==='/rflink/api/rx-log-config')out={ok:true,write_token:777,signal_limits:[250,250,250],noise_limit:2000,signals:[0,0,0],noise_total:0,log_bytes_used:0,arena_bytes:100000};
      else if(u==='/rflink/api/system-trace-config'){
        if(method==='POST'){window.logbookMock.sys=Number(new URLSearchParams(options.body).get('limit'));}
        out={ok:true,write_token:777,limit:window.logbookMock.sys,max_limit:256};
      }
      else if(u==='/rflink/api/devices')out={devices:[],candidates:[],storage_ok:true};
      else if(u==='/rflink/api/decoders')out={protocols:[],ha_fields:[]};
      else if(u==='/rflink/api/library')out={items:[],attention_count:0};
      else if(u==='/rflink/api/user-library')out={profiles:[],decoders:[],write_token:'stub'};
      return {ok:true,status:200,json:async()=>out,text:async()=>JSON.stringify(out)};
    };
  }''')
  page.evaluate(script)
  page.locator('[data-page="monitor"]').last.click() if width==390 else page.locator('[data-page="monitor"]').first.click()
  page.wait_for_function("document.querySelectorAll('#logbookTimeline details').length>=3")
  assert page.locator('#logbookTimeline details').count()>=3
  assert 'Missing TX encoder' in page.locator('#logbookTimeline').inner_text()
  assert 'RAMSES Auto' in page.locator('#logbookTimeline').inner_text()
  assert 'Background noise' not in page.locator('#logbookTimeline').inner_text()
  page.locator('#logbookNoise').check()
  page.wait_for_function("document.querySelector('#logbookTimeline').innerText.includes('Background noise')")
  page.locator('#logbookSeverity').select_option('warn')
  assert page.locator('#logbookTimeline details').count()==1
  page.locator('#logbookSeverity').select_option('all')
  page.locator('#logbookSearch').fill('devA')
  assert page.locator('#logbookTimeline details').count()==2
  page.locator('#logbookSearch').fill('')
  page.locator('#logbookTimeline details').first.locator('summary').click()
  assert page.locator('#logbookTimeline details').first.evaluate('(el)=>el.open')
  page.locator('#logbookRefresh').click()
  page.wait_for_timeout(150)
  assert page.locator('#logbookTimeline details').first.evaluate('(el)=>el.open')
  with page.expect_download() as download: page.locator('#logbookExport').click()
  assert download.value.suggested_filename=='rflink-logbook-all.json'
  page.locator('[data-log-view="web"]').click()
  assert 'Click:' in page.locator('#logbookTimeline').inner_text()
  page.locator('[data-log-view="system"]').click()
  assert 'Missing TX encoder' in page.locator('#logbookTimeline').inner_text()
  assert 'RAMSES Auto' not in page.locator('#logbookTimeline').inner_text()
  page.locator('[data-log-view="radio"]').click()
  assert page.locator('#radioLogView').is_visible()
  assert page.locator('#logbookTimeline').is_visible()
  assert not page.locator('#monitorActivity').is_visible()
  page.locator('[data-page="settings"]').last.click() if width==390 else page.locator('[data-page="settings"]').first.click()
  assert page.locator('#firmwareVersion').inner_text()=='v0.3.3-legacy' # legacy firmware must not be relabeled as the new release
  assert page.locator('#firmwareRevision').text_content()=='legacy-UXREVIEW5'
  assert page.locator('#logSectionRadio').is_visible()
  assert page.locator('#logSectionSystem').is_visible()
  assert page.locator('#logSectionWeb').is_visible()
  # one deliberately visible overview; a click on Advanced only expands technical records
  page.locator('[data-settings-open="library"]').click()
  assert page.locator('#libraryAdvancedPanel').is_visible()
  assert page.locator('#libOfficialDecoders').is_visible()
  assert page.locator('#libraryTechnicalPanel').is_hidden()
  if width==1440:
   page.screenshot(path='/mnt/data/RFLink_UXREVIEW5_Settings_Desktop.png',full_page=True)
  else:
   page.screenshot(path='/mnt/data/RFLink_UXREVIEW5_Settings_Mobile.png',full_page=True)
  page.locator('#libraryAdvanced').click()
  assert page.locator('#libraryTechnicalPanel').is_visible()
  page.locator('[data-settings-back]').first.click()
  page.locator('#systemLogLimit').fill('200')
  page.locator('#webLogLimit').fill('17')
  page.locator('#webLogVerbose').check()
  page.locator('#rxLogSave').click()
  page.wait_for_function('window.logbookMock.sys===200')
  page.locator('[data-page="monitor"]').last.click() if width==390 else page.locator('[data-page="monitor"]').first.click()
  page.locator('[data-log-view="web"]').click()
  page.evaluate('() => { for(let i=0;i<40;i++)rfWebPush("test","Test event "+i) }')
  assert page.locator('#logbookTimeline details').count()<=17
  page.locator('[data-page="settings"]').last.click() if width==390 else page.locator('[data-page="settings"]').first.click()
  for lang,expected in [('en','Logbooks'),('fr','Journaux'),('de','Protokolle'),('nl','Logboeken')]:
   page.evaluate('(lang)=>rfApplyLanguage(lang)',lang)
   assert page.locator('.page[data-view="monitor"] h1').inner_text()==expected
  page.evaluate('rfApplyLanguage("nl")')
  assert page.locator('[data-page="monitor"]').first.inner_text().endswith('Logboeken')
  assert page.evaluate('document.documentElement.scrollWidth <= innerWidth + 2'),width
  page.locator('[data-page="monitor"]').last.click() if width==390 else page.locator('[data-page="monitor"]').first.click()
  page.locator('[data-log-view="all"]').click()
  if width==1440:page.screenshot(path='/mnt/data/RFLink_LOGBOOK1_Desktop.png',full_page=True)
  else:page.screenshot(path='/mnt/data/RFLink_LOGBOOK1_Mobiel.png',full_page=True)
  assert not errors,(width,errors)
  print('LOGBOOK1 browser PASS',width,flush=True)
  browser.close()
