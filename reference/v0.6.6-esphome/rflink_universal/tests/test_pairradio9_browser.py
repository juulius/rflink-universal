"""PAIRRADIO9: TX attempts/rejections must appear in Radio and All, not as RF noise."""
from pathlib import Path
from playwright.sync_api import sync_playwright
R=Path(__file__).resolve().parents[1]
html=(R/'ui/index.html').read_text().replace('/*__RFLINK_LOCALES__*/',(R/'ui/locales.bundle.json').read_text())
script=html.split('<script>',1)[1].split('</script>',1)[0]
static=html.replace('<script>'+script+'</script>','')
for width in (1440,390):
 with sync_playwright() as pw:
  browser=pw.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox','--disable-dev-shm-usage'])
  page=browser.new_page(viewport={'width':width,'height':920})
  errors=[];page.on('pageerror',lambda e:errors.append(str(e)))
  page.set_content(static)
  page.evaluate('''() => {
    window.fetch=async(url)=>{
      const u=String(url);let out={ok:true};
      if(u==='/rflink/api/language')out={language:'nl'};
      else if(u==='/rflink/api/status')out={uptime_ms:23000,boot_count:9,channels:[{index:2,name:'868.30 Itho',frequency_label:'868.30 MHz',available:true,rx_ready:true,tx_ready:true}],activity:[],storage_ok:true,storage_ready:true};
      else if(u==='/rflink/api/system-trace')out={ok:true,server_uptime_ms:23000,events:[]};
      else if(u.startsWith('/rflink/api/rx-log?'))out={ok:true,events:[
        {seq:13,uptime_ms:22300,radio:2,category:'signal',record:{direction:'TX',protocol:'RamsesII',summary:'Koppelverzoek mislukt: 1FC9',level:'error',identity:'dev_00001',detail:'approved radio unavailable/busy'}},
        {seq:12,uptime_ms:22100,radio:2,category:'signal',record:{direction:'TX',protocol:'RamsesII',summary:'TX afgewezen: 1FC9 offer',level:'error',identity:'dev_00001',detail:'Radio route denied'}},
        {seq:11,uptime_ms:22000,radio:2,category:'signal',record:{direction:'TX',protocol:'RamsesII',summary:'Koppelverzoek aangevraagd: 1FC9',level:'info',identity:'dev_00001'}},
        {seq:10,uptime_ms:21000,radio:2,category:'noise',record:{direction:'RX',protocol:'Noise',summary:'63 byte noise'}}]};
      else if(u==='/rflink/api/devices')out={devices:[],candidates:[],storage_ok:true};
      else if(u==='/rflink/api/decoders')out={protocols:[],ha_fields:[]};
      else if(u==='/rflink/api/library')out={items:[],attention_count:0};
      else if(u==='/rflink/api/user-library')out={profiles:[],decoders:[],write_token:'stub'};
      else if(u==='/rflink/api/system-trace-config')out={ok:true,limit:128};
      else if(u==='/rflink/api/rx-log-config')out={ok:true,signal_limits:[250,250,250],noise_limit:2000};
      return {ok:true,status:200,json:async()=>out,text:async()=>JSON.stringify(out)};
    };
  }''')
  page.evaluate(script)
  page.locator('[data-page="monitor"]').last.click() if width==390 else page.locator('[data-page="monitor"]').first.click()
  page.wait_for_function("document.querySelector('#logbookTimeline').innerText.includes('Koppelverzoek mislukt')")
  assert page.locator('#logbookTimeline .src-radio').count()==3, (width,page.locator('#logbookTimeline').inner_text())
  assert '63 byte noise' not in page.locator('#logbookTimeline').inner_text()
  page.locator('[data-log-view="radio"]').click()
  page.wait_for_function("document.querySelector('#logbookTimeline').innerText.includes('TX afgewezen')")
  assert page.locator('#logbookTimeline details').count()==3
  page.locator('#logbookSeverity').select_option('error')
  assert page.locator('#logbookTimeline details').count()==2
  assert page.locator('#logbookTimeline .lv-error').count()==2
  page.locator('#logbookTimeline details').first.locator('summary').click()
  assert 'approved radio unavailable/busy' in page.locator('#logbookTimeline').inner_text()
  assert 'dev_00001' in page.locator('#logbookTimeline').inner_text()
  assert not errors,(width,errors)
  browser.close()
  print('PAIRRADIO9 logbook browser PASS',width)
