"""Desktop/mobile user-facing decoder library against all REAL pack manifests."""
import json
from pathlib import Path
from playwright.sync_api import sync_playwright
root=Path(__file__).resolve().parents[1]
html=(root/'ui/index.html').read_text().replace('/*__RFLINK_LOCALES__*/', (root/'ui/locales.bundle.json').read_text())
script=html.split('<script>',1)[1].split('</script>',1)[0]
static=html.replace('<script>'+script+'</script>','')
protocols=[]
fields={}
for folder in sorted((root/'decoders').iterdir()):
 if not folder.is_dir() or folder.name.startswith('_'):continue
 m=json.loads((folder/'manifest.json').read_text())
 if not m.get('enabled',True):continue
 for cap in m['capabilities']:
  meta=cap['learn'];tx=cap['transmit']
  protocols.append(dict(name=cap['protocol'],pack=folder.name,learn_category=meta['category'],learn_band=meta['band'],learn_instruction=meta['instruction'],learn_ha_result=meta['ha_result'],learn_warning=meta['warning'],tx_encoder=tx['encoder'],tx_status=tx['status'],tx_registered=tx['encoder']!='none',rx_actions='|'.join(cap['rx_actions']),fields='|'.join(x['id'] for x in cap['fields']),tx_actions='|'.join(x['id'] for x in tx['actions'])))
  for f in cap['fields']:
   if f.get('ha'):fields[f['id']]=f['ha']['label']
assert len(protocols)==13
fixture=json.dumps({'decoders':['builtins','itho','weather'],'protocols':protocols,'ha_fields':[{'key':k,'label':v} for k,v in fields.items()]},ensure_ascii=False)
user_library={'schema':1,'ready':True,'write_token':'fixture','decoders':[],'profiles':[{'id':'nexa_selflearn','name':'Nexa / Jula Anslut self-learning ON/OFF family','source':'builtin','protocol':'Nexa','version':1,'manufacturer':'Nexa / Jula Anslut','models':['408-063','408-064','LWST-605'],'expected_actions':['on','off'],'learn_help':'Use ON and OFF','learn_help_i18n':{'en':'Use ON and OFF','nl':'Gebruik AAN en UIT','fr':'Utilisez ON et OFF','de':'Verwende EIN und AUS'},'notes':'Source based','notes_i18n':{'en':'Source based','nl':'Uit bronnen','fr':'Basé sur des sources','de':'Quellenbasiert'},'hardware_validation':'pending_on_rflink_universal','sources':[{'kind':'legacy_compatibility','label':'RFLink list','url':'https://rflink.nl/devlist.php'}]}]}
with sync_playwright() as runner:
 browser=runner.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox','--disable-dev-shm-usage'])
 for width,height in ((1280,850),(390,844)):
  page=browser.new_page(viewport={'width':width,'height':height})
  errors=[]
  page.on('pageerror',lambda e:errors.append(str(e)))
  page.set_content(static)
  page.evaluate('''([fixture,userLibrary]) => {
    window.fetch=async (url,opt={})=>{
      let out={};
      if(url==='/rflink/api/language')out={language:'nl'};
      else if(url==='/rflink/api/decoders')out=fixture;
      else if(url==='/rflink/api/library')out={schema:1,persistent_user_library:false,official_decoders:13,user_decoders:0,official_profiles:1,user_profiles:0,attention_count:0,safe_resolution_count:0,archive_count:0,attention:[],items:(fixture.protocols||[]).map(p=>({id:'decoder:'+p.name,kind:'decoder',source:'builtin',version:1,name:p.name,pack:p.pack}))};
      else if(url==='/rflink/api/user-library')out=userLibrary;
      else if(url==='/rflink/api/devices')out={devices:[],candidates:[],storage_ok:true,mqtt_connected:false};
      else if(url==='/rflink/api/status')out={uptime_ms:1000,total_frames:0,decoded_frames:0,unknown_frames:0,decoder_count:13,expected_decoders:13,storage_ready:true,storage_ok:true,saved_devices:0,activity:[],led:{status:'ok',brightness:20,quiet:false},channels:[{index:0,name:'433.42',frequency_label:'433.42 MHz',checked:true,available:true,rx_ready:true,tx_ready:true,frames:0,age_ms:null},{index:1,name:'433.92',frequency_label:'433.92 MHz',checked:true,available:true,rx_ready:true,tx_ready:true,frames:0,age_ms:null},{index:2,name:'Itho',frequency_label:'868.30 MHz',checked:true,available:true,rx_ready:true,tx_ready:true,frames:0,age_ms:null}]};
      else if(url==='/rflink/api/developer')out={mode:'idle',best:null};
      else throw Error('Unexpected endpoint '+url);
      return {ok:true,json:async()=>out};
    }
  }''',[json.loads(fixture),user_library])
  page.evaluate(script)
  page.wait_for_function("document.documentElement.lang === 'nl'")
  page.wait_for_timeout(150)
  if width>680: page.locator('.side button[data-page="settings"]').click()
  else: page.locator('.bottom button[data-page="settings"]').click()
  page.wait_for_timeout(100)
  assert page.locator('.decoder-select').count()==12,'Normal library hides ExampleSwitch'
  assert page.locator('#decoderDetail').count()==1,'Exactly one decoder detail panel must exist'
  page.locator('.decoder-select').filter(has_text='Nexa').click()
  assert 'Nexa' in page.locator('#decoderDetail').inner_text()
  assert 'softwaretest' in page.locator('#decoderDetail').inner_text()
  assert 'Bediening' in page.locator('#decoderDetail').inner_text()
  assert 'Nog nodig' in page.locator('#decoderDetail').inner_text()
  assert 'Bekende apparaatprofielen' in page.locator('#decoderDetail').inner_text()
  assert '408-063' in page.locator('#decoderDetail').inner_text()
  assert page.locator('#decoderDetail .profile-sources a').count()==1
  assert page.locator('#selectedDecoderAdvancedPanel').is_hidden(),'Technical decoder details stay hidden by default'
  page.locator('#selectedDecoderAdvanced').click()
  assert page.locator('#selectedDecoderAdvancedPanel').is_visible(),'Advanced exposes technical decoder details on demand'
  assert page.locator('#selectedDecoderAdvancedPanel .mono').first.is_visible()
  page.locator('[data-decoder-filter="sensor"]').click()
  assert page.locator('.decoder-select').count()==4
  assert page.locator('.decoder-select').filter(has_text='AlectoV1').count()==1
  page.locator('[data-decoder-filter="development"]').click()
  assert page.locator('.decoder-select').count()==1
  assert 'ExampleSwitch' in page.locator('#decoderDetail').inner_text()
  assert page.locator('#selectedDecoderLearn').count()==0
  page.locator('[data-decoder-filter="control"]').click()
  assert page.locator('.decoder-select').count()==8
  page.locator('.decoder-select').filter(has_text='IthoRFT').click()
  detail=page.locator('#decoderDetail').inner_text().lower()
  assert 'softwaretest' in detail
  assert 'rf-hardware' in detail or 'echte rf-test' in detail
  page.locator('.decoder-select').filter(has_text='Dooya').click()
  page.locator('#selectedDecoderLearn').click()
  assert page.locator('#learnProtocolGuide').is_visible()
  assert 'Dooya' in page.locator('#learnProtocolGuide').inner_text()
  assert 'geen protocolfilter' in page.locator('#learnProtocolGuide').inner_text()
  assert page.locator('#decoderTools').is_hidden()
  assert page.evaluate('document.documentElement.scrollWidth<=window.innerWidth+2'),'Mobile page has horizontal overflow'
  assert not errors,errors
  page.close()
 browser.close()
print('Decoder UX Chromium: single selected decoder detail, Advanced-on-demand technical data, 12 normal/1 developer, 4 sensors, 8 controls, learning guidance, desktop+390px mobile, no overflow: PASS')
