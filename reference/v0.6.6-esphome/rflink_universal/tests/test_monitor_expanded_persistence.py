"""Browser regression: expanded activity records must survive live polling and 20-row pagination."""
from pathlib import Path
import json
from playwright.sync_api import sync_playwright
root = Path(__file__).resolve().parents[1]
html = (root / 'ui/index.html').read_text(encoding='utf-8').replace('/*__RFLINK_LOCALES__*/', (root/'ui/locales.bundle.json').read_text(encoding='utf-8'))
script = html.split('<script>',1)[1].split('</script>',1)[0]
static = html.replace('<script>'+script+'</script>', '')
with sync_playwright() as p:
    browser = p.chromium.launch(headless=True, executable_path='/usr/bin/chromium',args=['--no-sandbox','--disable-dev-shm-usage'])
    page = browser.new_page(viewport={'width':1280,'height':720})
    errors=[]
    page.on('pageerror',lambda e:errors.append(str(e)))
    page.set_content(static)
    page.evaluate('''() => {
      window.__events = Array.from({length:20},(_,i)=>({
        seq:i+1,radio:2,category:'noise',uptime_ms:80000+i*100,
        record:{channel_index:2,channel:'868.30 Itho',direction:'RX',protocol:'Noise',
        summary:'Noise '+(i+1),detail:'RAW detail for sequence '+(i+1),noise_suspect:true,
        noise_reason:'test',timestamp_ms:80000+i*100}}));
      const channels=['433.42','433.92','868.30 Itho'].map((name,index)=>({index,name,frequency_label:name+' MHz',available:true,checked:true,rx_ready:true,tx_ready:true,age_ms:500}));
      window.fetch=async (url,opt={})=>{
        const path=String(url);let result={};
        if(path.startsWith('/rflink/api/rx-log?')){
          const q=new URLSearchParams(path.split('?')[1]);
          let filtered=window.__events.filter(e=>(!q.has('radio')||e.radio===Number(q.get('radio')))&&(!q.has('kind')||Number(q.get('kind'))===(e.category==='noise'?1:0)));
          filtered=filtered.filter(e=>e.seq<Number(q.get('cursor')||1e8)).reverse().slice(0,Number(q.get('limit')||20));
          result={events:filtered,next_cursor:filtered.at(-1)?.seq||0,has_more:filtered.length===20};
        }else if(path==='/rflink/api/status') result={uptime_ms:100000,total_frames:20,firmware_version:'TEST',boot_session_id:412,channels,activity:[],led:{status:'ready',brightness:20}};
        else if(path==='/rflink/api/rx-log-config')result={signal_limits:[250,250,250],noise_limit:2000,noise_total:20,signals:[0,0,0],log_bytes_used:25600,arena_bytes:1572864};
        else if(path==='/rflink/api/language')result={language:'en'};
        else if(path==='/rflink/api/devices')result={devices:[],candidates:[]};
        else if(path==='/rflink/api/decoders')result={decoders:[]};
        else if(path==='/rflink/api/user-library')result={write_token:123,decoders:[],profiles:[],protocols:[]};
        else if(path.startsWith('/rflink/api/developer'))result={channel:0,mode:'listen',best:null};
        return {ok:true,json:async()=>result,text:async()=>JSON.stringify(result)};
      }
    }''')
    page.evaluate(script)
    page.locator('.side button[data-page="monitor"]').click()
    page.locator('#monitorLogType').select_option('noise')
    target=page.locator('#monitorActivity [data-monitor-key="rx:1:0"]')
    target.wait_for()
    target.locator('summary').click()
    assert target.locator('details').evaluate('(el)=>el.open')
    page.evaluate('window.__keepNode=document.querySelector("[data-monitor-key=\\"rx:1:0\\"]")')
    target.scroll_into_view_if_needed()
    page.wait_for_timeout(200)
    oldtop=target.evaluate('(el)=>el.getBoundingClientRect().top')
    # Simulate two new radio receptions; original entry drops off newest-20 API page.
    page.evaluate('''() => {
      for (let seq=21; seq<=22; seq++) window.__events.push({seq,radio:2,category:'noise',uptime_ms:80000+seq*100,record:{channel_index:2,channel:'868.30 Itho',direction:'RX',protocol:'Noise',summary:'Noise '+seq,detail:'RAW detail for sequence '+seq,noise_suspect:true,noise_reason:'test',timestamp_ms:80000+seq*100}});
    }''')
    page.wait_for_timeout(3600)
    assert target.count()==1, 'Opened entry disappeared after an incoming event'
    assert target.locator('details').evaluate('(el)=>el.open'), 'Details closed by live refresh'
    assert page.evaluate('window.__keepNode===document.querySelector("[data-monitor-key=\\"rx:1:0\\"]")'), 'Opened DOM element was recreated'
    assert page.locator('#monitorActivity [data-monitor-key="rx:22:0"]').count()==1, 'New signal absent'
    newtop=target.evaluate('(el)=>el.getBoundingClientRect().top')
    assert abs(newtop-oldtop)<8,(oldtop,newtop)
    # Multiple polls, still one node, no duplicate.
    page.wait_for_timeout(3200)
    assert target.count()==1 and target.locator('details').evaluate('(el)=>el.open')
    target.locator('summary').click()
    page.wait_for_timeout(3500)
    assert target.count()==0, 'Expired pinned record retained after closing'
    # Changing filter must not keep expanded records from a previous filter.
    newest=page.locator('#monitorActivity [data-monitor-key="rx:22:0"]')
    newest.locator('summary').click()
    page.locator('#monitorLogType').select_option('signal')
    page.wait_for_timeout(300)
    assert page.locator('#monitorActivity [data-monitor-key="rx:22:0"]').count()==0
    assert not errors,errors
    print('Monitor expanded details: survives live reception, pagination, viewport, filter; PASS')
    browser.close()
