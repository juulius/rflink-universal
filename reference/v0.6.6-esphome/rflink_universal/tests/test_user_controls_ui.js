// Execute the shipped UI with a small DOM/API fixture, including the actual
// button handlers. Hardware and browser layout remain separate checks.
const fs = require('fs'), vm = require('vm'), assert = require('assert'), path = require('path');
const {TextEncoder,TextDecoder} = require('util');
const root = path.join(__dirname, '..');
const html = fs.readFileSync(path.join(root, 'ui/index.html'), 'utf8')
  .replace('/*__RFLINK_LOCALES__*/', fs.readFileSync(path.join(root, 'ui/locales.bundle.json'), 'utf8'));
const script = html.split('<script>')[1].split('</script>')[0];
const elements = new Map(), timers = new Map(), sends = [];
let section, deviceVisible = false, downloaded, downloadName;
const device = {uid:'dev_00001',protocol:'user_test_ui',profile_id:'',local_tx_enabled:false,actions:['on']};
const library = {schema:1,ready:true,write_token:'fixture-token',decoders:[],profiles:[]};
const copy = x => JSON.parse(JSON.stringify(x));
function el(selector) {
  if (!elements.has(selector)) elements.set(selector, {
    innerHTML:'',textContent:'',value:'',hidden:false,checked:false,disabled:false,offsetParent:null,
    style:{},dataset:{},classList:{add(){},remove(){},toggle(){}},
    addEventListener(){},setAttribute(){},scrollIntoView(){},click(){},
    appendChild(node){section=node},querySelector(s){return el(s)},querySelectorAll(){return []},
  });
  return elements.get(selector);
}
el('#learnRadios [data-ch].active').dataset.ch='0';
const card = {querySelector(s){
  if (s === '[data-rename]') return {dataset:{rename:device.uid}};
  if (s === '[data-local-tx]') return section || null;
  if (s === '.device-more') return el('#device-more');
  return null;
}};
el('#deviceList').querySelectorAll = s => s === '.card.device' && deviceVisible ? [card] : [];
const ctx = {
  location:{hash:''},history:{pushState(){}},
  TextEncoder,TextDecoder,crypto:require('crypto').webcrypto,
  document:{getElementById:id=>el('#'+id),querySelector:el,querySelectorAll:()=>[],addEventListener(){},dispatchEvent(){},
    createElement:()=>({innerHTML:'',className:'',setAttribute(){},querySelector:el,
      click(){downloadName=this.download}}),documentElement:{lang:'en'}},
  window:{scrollTo(){},addEventListener(){},matchMedia(){return {matches:false,addEventListener(){},removeEventListener(){}}}},localStorage:{getItem:()=>null,setItem(){}},
  CustomEvent:class {constructor(type){this.type=type}},MutationObserver:class {observe(){}},
  URLSearchParams,Blob,URL:{createObjectURL(blob){downloaded=blob;return 'blob:fixture'},revokeObjectURL(){}},
  confirm:()=>true,setTimeout(){},clearTimeout(){},
  setInterval(fn,ms){timers.set(ms,fn)},
  fetch:async (url, options) => {
    if (url === '/rflink/api/user-library' && !options) return {ok:true,json:async()=>copy(library)};
    if (url === '/rflink/api/devices') return {ok:true,json:async()=>({devices:[copy(device)],candidates:[]})};
    if (url === '/rflink/api/status') return {ok:true,json:async()=>({channels:[],activity:[],
      total_frames:0,decoded_frames:0,unknown_frames:0,uptime_ms:5000,
      led:{status:'ready',quiet:false,brightness:20}})};
    if (url === '/rflink/api/decoders') return {ok:true,json:async()=>({decoders:[]})};
    if (url === '/rflink/api/language') return {ok:true,json:async()=>({language:'en'})};
    if (url === '/rflink/api/library') return {ok:true,json:async()=>({schema:1,items:[]})};
    if (url === '/rflink/api/developer') return {ok:true,json:async()=>({captures:[]})};
    assert(options?.method === 'POST', 'UI must POST changes: '+url);
    const p = new URLSearchParams(options.body);
    assert.strictEqual(p.get('write_token'), library.write_token);
    sends.push([url,p.get('action')]);
    if (url.endsWith('/user-library')) {
      if (p.get('action') === 'create') library.decoders.push({
        id:p.get('id'),name:p.get('name'),radio:0,kind:'raw',source:'user',version:1,enabled:false,
        actions:['on'],samples:[{label:'on'},{label:'on'},{label:'on'}],
        rx_source:'CONST 42\nID\nACTION on\nACCEPT',
        tx_source:'ACTEQ on\nREQUIRE\nCONST 500\nMARK\nCONST 1000\nSPACE\nSEND',
      });
      else if (p.get('action') === 'enable') library.decoders.find(d=>d.id===p.get('id')).enabled=true;
      else if (p.get('action') === 'save_profile') library.profiles.push({
        id:p.get('id'),name:p.get('name'),source:'user',protocol:p.get('protocol'),
        version:1,expected_actions:p.get('actions').split(','),learn_help:p.get('learn_help'),
      });
    } else if (url.endsWith('/user-tx')) {
      if (p.get('action') === 'preview') return {ok:true,json:async()=>({ok:true,radio:0,repeats:3,
        raw:[500,-1000],packet:[],next_state:{sequence:7}})};
      if (p.get('action') === 'approve') device.local_tx_enabled=true;
      if (p.get('action') === 'send') assert(device.local_tx_enabled && p.get('command')==='on');
    } else throw Error('Unexpected POST '+url);
    return {ok:true,json:async()=>({ok:true})};
  },
};
async function settle(){for(let i=0;i<8;i++)await new Promise(resolve=>setImmediate(resolve))}
(async()=>{
  vm.runInNewContext(script,ctx,{timeout:1000}); await settle();
  el('#userDecoderId').value='user_test_ui';el('#userDecoderName').value='Test remote';
  await el('#userCreate').onclick();await settle();
  assert(library.decoders[0].enabled);
  assert.deepStrictEqual(sends.slice(0,2),[
    ['/rflink/api/user-library','create'],['/rflink/api/user-library','enable']]);
  assert.strictEqual(el('#userActions').value,'on');
  el('#userProfileId').value='test_model';el('#userProfileName').value='Test model';
  el('#userProfileDecoder').value='user_test_ui';el('#userProfileActions').value='on';
  el('#userProfileHelp').value='Press the button';
  await el('#userSaveProfile').onclick();await settle();
  assert.strictEqual(library.profiles[0].protocol,'user_test_ui');
  await el('#userExport').onclick();
  const backup=JSON.parse(await downloaded.text());
  assert.strictEqual(downloadName,'rflink-user-library-export.json');
  assert.strictEqual(backup.format,'rflink-user-library');
  assert.strictEqual(backup.library.write_token,undefined,'a transient session token must not be exported');
  assert.strictEqual(backup.library.decoders[0].id,'user_test_ui');
  deviceVisible=true;await timers.get(4000)();await settle();
  assert(section,'own device controls must be added');
  el('[data-select-action]').value='on';
  await el('[data-preview-tx]').onclick();
  assert(el('[data-tx-result]').textContent.includes('RAW: 500 -1000'));
  assert(el('[data-tx-result]').textContent.includes('"sequence":7'));
  await el('[data-toggle-local]').onclick();
  assert(device.local_tx_enabled);
  await el('[data-send-tx]').onclick();
  assert(sends.some(([url,action])=>url.endsWith('/user-tx')&&action==='send'));
  console.log('User UI: actual button handlers, decoder, profile, TX preview, approval and send PASS');
})().catch(e=>{console.error(e);process.exitCode=1});
