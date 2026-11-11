// Regression test: execute the actual embedded JavaScript with a minimal browser mock.
const fs=require('fs'), vm=require('vm'), assert=require('assert');
const path=require('path');
const rawHtml=fs.readFileSync(path.join(__dirname,'../ui/index.html'),'utf8');
const bundle=fs.readFileSync(path.join(__dirname,'../ui/locales.bundle.json'),'utf8');
const html=rawHtml.replace('/*__RFLINK_LOCALES__*/',bundle);
const script=html.split('<script>')[1].split('</script>')[0];
const elements=new Map();
function el(selector){
 if(!elements.has(selector))elements.set(selector, {
   textContent:'', innerHTML:'', value:'', checked:false, hidden:false, disabled:false,
   addEventListener(){},setAttribute(){},appendChild(){},querySelector:el,querySelectorAll:()=>[],
   style:{}, dataset:{}, classList:{add(){},remove(){},toggle(){}},
 });
 return elements.get(selector);
}
const calls=[];
let savedQuiet=true;
let savedBrightness=20;
const status=()=>({total_frames:0, decoded_frames:0,unknown_frames:0, uptime_ms:10000,
 channels:[],activity:[],led:{status:'gereed',quiet:savedQuiet,brightness:savedBrightness}});
const ctx={
 document:{querySelector:el,querySelectorAll:()=>[],addEventListener(){},dispatchEvent(){},documentElement:{lang:'en'}},
 CustomEvent:class {constructor(type){this.type=type}},
 MutationObserver:class {observe(){}},
 localStorage:{getItem:()=>null,setItem(){}},
 window:{scrollTo(){}},
 setInterval(){},setTimeout(){},clearTimeout(){},
 fetch:async (url,opt)=>{
  calls.push({url,method:opt?.method||'GET'});
  if(url.startsWith('/rflink/api/led?')) {
   const p=new URL(url,'http://localhost').searchParams;
   savedQuiet=p.get('quiet')==='1'; savedBrightness=Number(p.get('brightness'));
   return {ok:true,json:async()=>({ok:true})};
  }
  let result;
  if(url==='/rflink/api/status') result=status();
  else if(url==='/rflink/api/devices')result={devices:[],candidates:[]};
  else if(url==='/rflink/api/decoders')result={decoders:[]};
  else if(url==='/rflink/api/language')result={language:'nl'};
  else if(url==='/rflink/api/user-library')result={decoders:[],profiles:[],write_token:'test'};
  else throw Error('Unexpected fetch '+url);
  return {ok:true,json:async()=>result};
 },
};
function flush(){return new Promise(resolve=>setImmediate(resolve));}
(async()=>{
 vm.runInNewContext(script,ctx,{timeout:1000});
 await flush();await flush();
 const checkbox=el('#ledHeartbeat'), save=el('#ledSave');
 // Old saved 'quiet=true' MUST display new positive switch as off.
 assert.strictEqual(checkbox.checked,false,'saved quiet=true should display heartbeat disabled');
 assert.strictEqual(el('#ledStatusSettings').textContent,'Gereed','LED status must be translated for the saved Dutch UI language');
 el('#ledBrightness').value='100';checkbox.checked=true;checkbox.onchange();
 await save.onclick();
 assert.strictEqual(savedQuiet,false,'checked=true should save quiet=0');
 assert.strictEqual(savedBrightness,100);
 assert.strictEqual(checkbox.checked,true,'backend reload should display heartbeat enabled');
 assert(calls.some(c=>c.url.includes('quiet=0')),'real POST should send quiet=0');
 checkbox.checked=false;checkbox.onchange();await save.onclick();
 assert.strictEqual(savedQuiet,true,'unchecked should save quiet=1');
 assert.strictEqual(checkbox.checked,false,'backend reload should display heartbeat disabled');
 assert(calls.some(c=>c.url.includes('quiet=1')),'real POST should send quiet=1');
 console.log('LED UI: actual JS load, inverse mapping, POST and reload: PASS');
})().catch(e=>{console.error(e);process.exitCode=1});
