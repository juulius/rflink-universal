"""Browser integration smoke for RXLOG1: radio/noise filtering, runtime NVS settings UI, CSV ZIP."""
from pathlib import Path
import json, zipfile, tempfile
from playwright.sync_api import sync_playwright
root=Path(__file__).resolve().parents[1]
html=(root/'ui/index.html').read_text().replace('/*__RFLINK_LOCALES__*/',(root/'ui/locales.bundle.json').read_text())
script=html.split('<script>',1)[1].split('</script>',1)[0]
static=html.replace('<script>'+script+'</script>','')
with sync_playwright() as p:
 browser=p.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox','--disable-dev-shm-usage'])
 page=browser.new_page(accept_downloads=True)
 errors=[]
 page.on('pageerror',lambda e:errors.append(str(e)))
 page.set_content(static)
 page.evaluate(r'''() => {
   window.__writes=[];
   const channels=['433.42','433.92','868.30 Itho'].map((name,index)=>({index,name,frequency_label:['433.42 MHz','433.92 MHz','868.30 MHz'][index],available:true,checked:true,rx_ready:true,tx_ready:true,frames:3,age_ms:500,mode:index===2?'PACKET':'RX'}));
   const events=[
     {seq:1,radio:0,category:'signal',uptime_ms:95000,record:{channel_index:0,channel:'433.42',direction:'RX',protocol:'Unknown',summary:'Important 433 signal',detail:'RAW 30 pulses',full_detail:'RAW hash ABCD | 30 pulses | 11,-12',noise_suspect:false,noise_reason:'',timestamp_ms:95000}},
     {seq:2,radio:2,category:'noise',uptime_ms:96000,record:{channel_index:2,channel:'868.30 Itho',direction:'RX',protocol:'Noise',summary:'Suspected RF noise',detail:'PACKET 63 bytes',full_detail:'PACKET 63 bytes | FF FF FF',noise_suspect:true,noise_reason:'Itho unmatched',timestamp_ms:96000}},
     {seq:3,radio:1,category:'noise',uptime_ms:97000,record:{channel_index:1,channel:'433.92',direction:'RX',protocol:'Noise',summary:'Suspected RF noise',detail:'RAW 4 pulses',full_detail:'RAW hash 33 | 4 pulses | 11,-12',noise_suspect:true,noise_reason:'too short',timestamp_ms:97000}}
   ];
   window.fetch=async (url,opt={})=>{
     const path=String(url);
     let result={};
     if(path.startsWith('/rflink/api/rx-log?')){
       const q=new URLSearchParams(path.split('?')[1]);let filtered=events.filter(e=>(!q.has('radio')||e.radio===Number(q.get('radio')))&&(!q.has('kind')||Number(q.get('kind'))===(e.category==='noise'?1:0)));
       if(q.get('order')==='asc')filtered=filtered.filter(e=>e.seq>Number(q.get('cursor')||0));
       else filtered=filtered.filter(e=>e.seq<Number(q.get('cursor')||1e8)).reverse();
       result={events:filtered,next_cursor:filtered.length?filtered[filtered.length-1].seq:0};
     } else if(path==='/rflink/api/rx-log-config'){
       if(opt.method==='POST'){window.__writes.push(String(opt.body));}
       result={ok:true,write_token:123,signal_limits:[250,250,250],noise_limit:2000,noise_total:2,
         signals:[1,0,0],noise_by_radio:[0,1,1],log_bytes_used:3000,arena_bytes:1572864};
     } else if(path==='/rflink/api/status') result={uptime_ms:100000,total_frames:3,decoded_frames:0,unknown_frames:1,noise_frames:2,firmware_version:'RXLOG1 TEST',firmware_revision:'RXLOG1',boot_session_id:123,channels,activity:[],led:{status:'ready',brightness:20,quiet:false}};
     else if(path==='/rflink/api/language')result={language:'en'};
     else if(path==='/rflink/api/devices')result={devices:[],candidates:[]};
     else if(path==='/rflink/api/decoders')result={decoders:[]};
     else if(path==='/rflink/api/activity-export')result={activity:[]};
     else if(path==='/rflink/api/user-library')result={write_token:123,decoders:[],profiles:[],protocols:[]};
     else if(path.startsWith('/rflink/api/developer'))result={channel:0,mode:'listen',best:null};
     return {ok:true,json:async()=>result,text:async()=>JSON.stringify(result)};
   };
 }''')
 page.evaluate(script)
 page.wait_for_timeout(500)
 page.locator('.side button[data-page="monitor"]').click()
 page.wait_for_timeout(200)
 assert 'Important 433 signal' in page.locator('#monitorActivity').inner_text()
 page.locator('#monitorLogType').select_option('noise')
 page.wait_for_timeout(200)
 assert 'Itho' in page.locator('#monitorActivity').inner_text()
 assert 'Important 433 signal' not in page.locator('#monitorActivity').inner_text()
 page.locator('#monitorRadios button[data-mon="1"]').click()
 page.wait_for_timeout(200)
 assert '433.92' in page.locator('#monitorActivity').inner_text()
 assert 'Itho' not in page.locator('#monitorActivity').inner_text()
 page.locator('.side button[data-page="settings"]').click()
 page.wait_for_timeout(200)
 page.locator('#rxLogLimit0').fill('300')
 page.locator('#rxLogSave').click()
 page.wait_for_timeout(200)
 writes=page.evaluate('window.__writes')
 assert any('signal_0=300' in x and 'write_token=123' in x for x in writes),writes
 page.locator('.side button[data-page="monitor"]').click()
 page.wait_for_timeout(200)
 with page.expect_download() as d:page.locator('#monitorExport').click()
 download=d.value
 with tempfile.NamedTemporaryFile(suffix='.zip') as target:
  download.save_as(target.name)
  with zipfile.ZipFile(target.name) as z:
   assert z.testzip() is None
   assert len(z.namelist())==8
   assert 'Important 433 signal' in z.read('all_events.csv').decode('utf8')
   assert 'Itho unmatched' in z.read('868.30_noise.csv').decode('utf8')
 assert not errors,errors
 print('RXLOG1 browser: radio and noise filtering, runtime POST settings, ZIP export: PASS')
 browser.close()
