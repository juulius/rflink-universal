// Behavioral regression for the native log reader's live/frozen snapshot.
// Uses the actual functions extracted from webui/app.js, not a copied rewrite.
'use strict';
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const vm=require('node:vm');
const source=fs.readFileSync(path.join(__dirname,'../main/webui/app.js'),'utf8');
function section(start,next){
  const begin=source.indexOf(start);
  assert(begin>=0,'missing '+start);
  const end=source.indexOf(next,begin+start.length);
  assert(end>begin,'missing boundary '+next);
  return source.slice(begin,end);
}
const nodes=new Map();
const scroller={scrollTop:126};
function $(id){
  if(!nodes.has(id))nodes.set(id,{textContent:'',disabled:false,href:'',dataset:{}});
  return nodes.get(id);
}
$('log-rows').closest=()=>scroller;
const pending=[];
const rendered=[];
const context=vm.createContext({
  URLSearchParams,$,
  api(url){return new Promise(resolve=>pending.push({url,resolve}));},
  renderLogs(events){rendered.push(events);$('log-feedback').textContent='Rendered';},
  refreshLogBufferSummary(){},
  console
});
vm.runInContext(`
let logPageCursors=[0],logSnapshot=0,logHasOlder=false,logLivePaused=false,logsBusy=false;
const expandedLogKeys=new Set();
`+section('function resetLogPages(){','let nextErrorShown=')+
section('function pauseLogViewing(){','const txTransportNames=')+`
function selectedLogQuery(){
  const query=new URLSearchParams();
  if(logSnapshot)query.set('snapshot',String(logSnapshot));
  return query;
}
`+section('async function refreshLogs(jumpToTop=false){',"for(const button of document.querySelectorAll('[data-log-source]'))")+`
function state(){return {logSnapshot,logLivePaused,logsBusy,expanded:expandedLogKeys.size};}
`,context,{filename:'native-log-live-freeze-under-test.js'});
const run=expr=>vm.runInContext(expr,context);
async function test(){
  // First poll is live, establishing the snapshot at the top of the page.
  const first=run('refreshLogs()');
  assert.equal(pending.shift().url,'/api/logs');
  // Initial request was shifted; resolve its promise from a retained reference.
}
async function scenario(){
  const initial=run('refreshLogs()');
  assert.equal(pending.length,1);
  const first=pending.shift();
  assert.equal(first.url,'/api/logs');
  first.resolve({snapshot:10,cursor:1,events:[{sequence:10}],has_more:false});
  await initial;
  assert.equal(rendered.length,1);
  assert.equal(run('state().logSnapshot'),10);
  run('pauseLogViewing()');
  assert.equal(run('state().logLivePaused'),true);
  assert.equal($('log-latest').disabled,false);
  assert.equal($('log-latest').textContent,'Resume live');
  assert.match($('log-feedback').textContent,/new events are still recorded/);

  // Explicit reload while inspecting must pin the same original snapshot.
  const fixed=run('refreshLogs()');
  assert.match(pending[0].url,/snapshot=10/);
  pending.shift().resolve({snapshot:10,cursor:1,events:[{sequence:10}],has_more:false});
  await fixed;
  assert.equal(rendered.length,2);
  assert.equal(run('state().logSnapshot'),10);

  // Race: an in-flight live request completes AFTER the row was expanded.
  run('resetLogPages()');
  const stale=run('refreshLogs()');
  assert.equal(pending[0].url,'/api/logs');
  run('pauseLogViewing()');
  pending.shift().resolve({snapshot:11,cursor:1,events:[{sequence:11}],has_more:false});
  await stale;
  assert.equal(rendered.length,2,'late poll must never shift an expanded row');
  assert.equal(run('state().logLivePaused'),true);

  // Resume requests newest events, explicitly jumps to top, and clears pause.
  run('resetLogPages()');
  const resumed=run('refreshLogs(true)');
  assert.equal(pending[0].url,'/api/logs');
  pending.shift().resolve({snapshot:12,cursor:1,events:[{sequence:12}],has_more:false});
  await resumed;
  assert.equal(rendered.length,3);
  assert.equal(run('state().logLivePaused'),false);
  assert.equal($('log-latest').textContent,'Latest events');
  assert.equal($('log-latest').disabled,true);
  assert.equal(scroller.scrollTop,0);
  assert.equal(run('state().logSnapshot'),12);

  // Both regular timer and tab-visibility polling stop while inspecting.
  const pausedGuards=source.match(/currentPage==='logs'&&logPageCursors\.length===1&&!logLivePaused\)refreshLogs\(\)/g)||[];
  assert.equal(pausedGuards.length,2);
  // The latest-button handler must explicitly jump to the new top.
  assert.match(source,/\$\('log-latest'\)\.addEventListener\('click',[\s\S]*?refreshLogs\(true\);/);
  console.log('PASS live log reader: pause, stable snapshot, late-response guard, manual resume/top scroll');
}
scenario().catch(e=>{console.error(e);process.exitCode=1;});
