"""PROFILESAFE1 lazy decoder detail and per-capture retrieval for editing."""
from pathlib import Path
from playwright.sync_api import sync_playwright
r=Path(__file__).resolve().parents[1]
html=(r/'ui/index.html').read_text().replace('/*__RFLINK_LOCALES__*/',(r/'ui/locales.bundle.json').read_text())
script=html.split('<script>',1)[1].split('</script>',1)[0]
static=html.replace('<script>'+script+'</script>','')
with sync_playwright() as pw:
 browser=pw.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox','--disable-dev-shm-usage'])
 page=browser.new_page(viewport={'width':1400,'height':900})
 errors=[];page.on('pageerror',lambda e:errors.append(str(e)+'\n'+str(getattr(e,'stack',None))))
 page.set_content(static)
 page.evaluate('''()=>{
 window._sampleFetches=[];
 const d={id:'user_test',name:'Test decoder',source:'user',enabled:true,version:1,radio:0,kind:'raw',radio_profile:'board_raw',actions:['on','off'],rx_source:'PULSE 100\\nEND',tx_source:'PULSE 120\\nEND',samples:[],sample_count:2};
 const samples=[{label:'on',channel:0,kind:'raw',raw:[100,200,300],packet:[]},{label:'on',channel:0,kind:'raw',raw:[105,195,299],packet:[]}];
 window.fetch=async url=>{
  let value={ok:true},status=200;const u=String(url);
  if(u==='/rflink/api/language')value={language:'nl'};
  else if(u==='/rflink/api/status')value={channels:[],activity:[],storage_ok:true,storage_ready:true,decoder_count:13,expected_decoders:13};
  else if(u==='/rflink/api/devices')value={devices:[],candidates:[],storage_ok:true};
  else if(u==='/rflink/api/decoders')value={protocols:[{name:'RamsesII',rx_actions:'mode_command'}],ha_fields:[]};
  else if(u==='/rflink/api/user-library')value={schema:1,write_token:'123',decoders:[d],profiles:[],protocols:[{id:'RamsesII',name:'RamsesII',actions:['status']}],radio_profiles:[]};
  else if(u==='/rflink/api/profile-catalog')value={profiles:[]};
  else if(u.startsWith('/rflink/api/user-decoder-detail')){
   const args=new URL('http://device'+u).searchParams;
   if(args.has('sample')){const i=Number(args.get('sample'));window._sampleFetches.push(i);value={samples:[samples[i]]};}
   else value={id:d.id,name:d.name,rx_source:d.rx_source,tx_source:d.tx_source,sample_count:2,samples:[]};
  }
  else if(u==='/rflink/api/library')value={items:[],attention:[],attention_count:0};
  return {ok:status===200,status,json:async()=>value,text:async()=>JSON.stringify(value)};
 };
 }''')
 page.evaluate(script)
 page.locator('.side [data-page="settings"]').click()
 page.locator('[data-settings-open="custom"]').first.click()
 page.locator('[data-user-edit="user_test"]').first.wait_for(timeout=4000)
 page.locator('[data-user-edit="user_test"]').first.click()
 page.wait_for_function('window._sampleFetches.length===2',timeout=4000)
 assert page.evaluate('window._sampleFetches')==[0,1]
 assert page.locator('#userRxSource').input_value().startswith('PULSE')
 assert page.locator('#userTxSource').input_value().startswith('PULSE')
 assert not errors, errors
 browser.close()
print('PROFILESAFE1 lazy decoder samples editing browser PASS')
