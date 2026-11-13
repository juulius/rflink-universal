/* Read-only RF Replay in HA Ingress. Data remains in this browser tab.
 * Never render imported RF metadata as HTML or forward it to external URLs.
 */
(() => {
'use strict';
const e=id=>document.getElementById(id), files={a:null,b:null};
const MAX=16*1024*1024, PAGE=40;
let current=0, busy=false;
const radios=['433,42 MHz','433,92 MHz','868,30 MHz'];
const reasons={
 legacy_preamble_absent:'Legacy-preamble ontbreekt',
 built_in_decoder_rejected:'Standaarddecoders: geen match',
 built_in_decoder_valid:'Protocolcontrole geldig',
 native_decoder_not_run:'868-decoder niet uitgevoerd',
 insufficient_signed_pulses:'Te weinig pulsen',
 no_stable_timing_candidate:'Geen stabiel timingpatroon',
 not_alternating_high_low:'Geen afwisselende hoog/laag-pulsen',
 not_enough_pairs:'Te weinig pulsenparen',
 candidate_timings_only:'Mogelijke bits · nog niet gevalideerd',
 invalid_fifo_hex_or_length:'Ongeldige FIFO-data'
};
function make(tag,value,cls){
 const x=document.createElement(tag);
 if(value!==undefined)x.textContent=String(value);
 if(cls)x.className=cls;
 return x;
}
function valid(data){
 return data&&data.schema==='rflink-offline-replay-v1'&&Array.isArray(data.results)&&
        data.results.length<=5000&&data.summary&&typeof data.summary==='object'&&
        data.results.every(row=>row&&typeof row==='object'&&
          typeof row.capture_id==='string'&&row.capture_id.length<=80&&
          row.result&&typeof row.result==='object');
}
function index(report){
 const m=new Map(),seen=new Map();
 for(const row of (report?.results||[])){
  if(!row||typeof row.capture_id!=='string'||row.capture_id.length>80)continue;
  // A standalone capture JSON has no boot/sequence in older exports.
  // Repeated byte-identical frames must remain separate table entries.
  const n=seen.get(row.capture_id)||0;
  seen.set(row.capture_id,n+1);
  m.set(row.capture_id+':'+n,row);
 }
 return m;
}
function verified(row){return Boolean(row?.result?.protocol);}
function signature(row){
 const r=row?.result;
 if(!r)return '';
 return JSON.stringify({protocol:r.protocol||null,reason:r.reason||null,
 command:r.command??null,mode:r.mode??null,itho_type:r.itho_type??null,
 speed:r.speed??null,candidates:r.candidates||[]});
}
function label(row){
 if(!row?.result)return '—';
 const r=row.result;
 if(r.protocol==='ramses_ii'){
   const cmd=Number.isFinite(r.command)?'0x'+r.command.toString(16).toUpperCase().padStart(4,'0'):'';
   return 'RAMSES-II '+cmd+(Number.isFinite(r.mode)?' · modus '+r.mode:'');
 }
 if(r.protocol==='itho_rft')return 'Itho RFT · geldig';
 const c=Array.isArray(r.candidates)?r.candidates:[];
 if(c.length)return 'Timinghypothese: '+[...new Set(c.map(v=>v.kind))].join(', ')+' (onbevestigd)';
 return reasons[r.reason]||String(r.reason||'Onbekend');
}
function resultCell(row){
 const cell=make('td');
 cell.append(make('span',label(row),verified(row)?'replay-valid':'replay-unknown'));
 const candidates=row?.result?.candidates;
 if(Array.isArray(candidates)&&candidates.length){
  const d=make('details',undefined,'replay-candidate');
  d.append(make('summary','Bekijk kandidaat-bits (niet bevestigd)'));
  for(const c of candidates.slice(0,4)){
   d.append(make('p',
    String(c.kind||'timing')+' · '+String(c.bit_count||0)+' bits · '+
    String(c.bits||'').slice(0,512)+(c.inverted_bits?' · inverse '+String(c.inverted_bits).slice(0,512):'')));
  }
  cell.append(d);
 }
 return cell;
}
function summary(slot){
 const el=e('rf-replay-summary-'+slot),report=files[slot];
 el.replaceChildren();el.hidden=!report;
 if(!report)return;
 const counts=report.summary||{};
 const recognized=Object.keys(counts).filter(k=>k.startsWith('protocol_'))
       .reduce((a,k)=>a+(Number(counts[k])||0),0);
 const stats=[
 [report.capture_count||0,'captures'],
 [recognized,'herkend'],
 [counts.radio_0||0,'433,42 MHz'],
 [counts.radio_1||0,'433,92 MHz'],
 [counts.radio_2||0,'868,30 MHz']
 ];
 for(const [v,name] of stats){
  const box=make('div',undefined,'replay-stat');
  box.append(make('strong',Number(v).toLocaleString('nl-NL')),make('small',name));
  el.append(box);
 }
 const note='Codec '+String(report.decoder_fingerprint||'onbekend')+
   (report.limits?.results_truncated?' · MAX 5.000 records':'')+
   (report.input_counters?.invalid_captures?' · ongeldige records: '+report.input_counters.invalid_captures:'');
 el.append(make('p',note,'minor'));
}
function draw(){
 summary('a');summary('b');
 const a=index(files.a),b=index(files.b);
 const ids=[...new Set([...a.keys(),...b.keys()])].reverse();
 const freq=e('rf-replay-radio').value,outcome=e('rf-replay-outcome').value;
 const pairs=[];
 let unchanged=0,changed=0,shared=0;
 for(const id of ids){
  const x=a.get(id)||null,y=b.get(id)||null,row=x||y;
  const state=x&&y?(signature(x)===signature(y)?'same':'changed'):(x?'only_a':'only_b');
  if(x&&y){shared++;if(state==='same')unchanged++;else changed++;}
  if(freq!=='all'&&String(row.radio)!==freq)continue;
  if(outcome==='recognized'&&!verified(x)&&!verified(y))continue;
  if(outcome==='unknown'&&(verified(x)||verified(y)))continue;
  if(outcome==='different'&&state==='same')continue;
  pairs.push({id,x,y,row,state});
 }
 e('rf-replay-compare').textContent=files.a&&files.b
  ?'Gedeelde capture-ID’s: '+shared+' · zelfde resultaat: '+unchanged+
    ' · gewijzigde decoderresultaten: '+changed+
    '. Verschillende IDs betekenen niet automatisch een decoderwijziging.'
  :files.a?'Analyse A: '+a.size+' captures. Importeer B voor vergelijking.'
  :files.b?'Analyse B: '+b.size+' captures. Importeer A voor vergelijking.'
  :'Importeer een CSV of RF capture JSON om te beginnen.';
 const max=Math.max(1,Math.ceil(pairs.length/PAGE));
 current=Math.min(max-1,Math.max(0,current));
 const slice=pairs.slice(current*PAGE,(current+1)*PAGE);
 const fragment=document.createDocumentFragment();
 for(const p of slice){
  const tr=make('tr');
  const number=make('td','#'+p.id.slice(0,12));
  const stamp=make('td',p.row.event_utc||('uptime '+(p.row.uptime_ms??'—')));
  stamp.append(make('small',' seq '+String(p.row.sequence??'—')));
  tr.append(number,stamp,make('td',radios[Number(p.row.radio)]||'onbekend'),
    resultCell(p.x),resultCell(p.y),
    make('td',({same:'Gelijk',changed:'Gewijzigd',only_a:'Alleen A',only_b:'Alleen B'})[p.state]));
  fragment.append(tr);
 }
 if(!slice.length){const tr=make('tr'),td=make('td','Geen resultaten voor deze selectie.');td.colSpan=6;tr.append(td);fragment.append(tr);}
 e('rf-replay-results').replaceChildren(fragment);
 e('rf-replay-prev').disabled=current===0;
 e('rf-replay-next').disabled=(current+1)*PAGE>=pairs.length;
 e('rf-replay-page-status').textContent=pairs.length
  ?String(current*PAGE+1)+'–'+String(Math.min((current+1)*PAGE,pairs.length))+' van '+pairs.length
  :'0 resultaten';
 e('rf-replay-copy-report').disabled=!files.a;
 e('rf-replay-save-report').disabled=!files.a;
}
async function importFile(slot){
 if(busy)return;
 const file=e('rf-replay-file-'+slot).files?.[0];
 const status=e('rf-replay-status-'+slot),button=e('rf-replay-run-'+slot);
 if(!file){status.textContent='Kies eerst een CSV- of JSON-bestand.';return;}
 const name=String(file.name||'').toLowerCase();
 const type=name.endsWith('.csv')?'csv':name.endsWith('.json')?'json':null;
 if(!type||!file.size||file.size>MAX){status.textContent='Alleen CSV/JSON van 1 byte tot 16 MiB.';return;}
 busy=true;button.disabled=true;button.setAttribute('aria-busy','true');
 status.textContent='Bestand veilig analyseren… Andere Builder-functies blijven beschikbaar.';
 try{
  let report=null;
  if(type==='json'){
   let parsed=null;
   try{parsed=JSON.parse(await file.text());}catch{}
   if(valid(parsed))report=parsed;
  }
  if(!report){
   const response=await fetch('api/replay/analyze',{
    method:'POST',cache:'no-store',
    headers:{'X-RFLink-CSRF':csrf,'Content-Type':'application/octet-stream',
             'X-RFLink-Replay-Type':type},
    body:file
   });
   let payload;
   try{payload=await response.json();}catch{throw new Error('Server gaf geen geldige JSON terug');}
   if(!response.ok)throw new Error(String(payload.error||'Analyse mislukt'));
   report=payload;
  }
  if(!valid(report))throw new Error('Geen geldig Replay v1-rapport ontvangen');
  files[slot]=report;current=0;
  status.textContent=String(report.capture_count)+' captures geanalyseerd · '+String(report.decoder_mode||'offline')+
   ' · alleen in deze browsersessie.';
  draw();
 }catch(err){status.textContent='Analyse mislukt: '+String(err?.message||'Onbekende fout').slice(0,220);}
 finally{busy=false;button.disabled=false;button.removeAttribute('aria-busy');}
}
e('header-replay').addEventListener('click',()=>{
 document.querySelector('.nav-tab[data-page="replay"]')?.click();
});
for(const slot of ['a','b'])e('rf-replay-run-'+slot).addEventListener('click',()=>importFile(slot));
for(const key of ['rf-replay-radio','rf-replay-outcome'])
 e(key).addEventListener('change',()=>{current=0;draw();});
e('rf-replay-prev').addEventListener('click',()=>{current--;draw();});
e('rf-replay-next').addEventListener('click',()=>{current++;draw();});
e('rf-replay-copy-report').addEventListener('click',async()=>{
 if(!files.a)return;
 try{
  await navigator.clipboard.writeText(JSON.stringify(files.a,null,2));
  e('rf-replay-status-a').textContent='Rapport A gekopieerd. Bewaar dit privé: kandidaatbits kunnen identificaties bevatten.';
 }catch{e('rf-replay-status-a').textContent='Kopiëren niet toegestaan; gebruik Bewaar rapport.';}
});
e('rf-replay-save-report').addEventListener('click',async()=>{
 if(!files.a)return;
 const contents=JSON.stringify(files.a,null,2);
 const filename='RFLink_Replay_Report_A.json';
 // Prefer a native save dialog, which preserves filenames better than
 // temporary blob downloads inside Home Assistant embedded WebViews.
 if(typeof window.showSaveFilePicker==='function'){
  try{
   const handle=await window.showSaveFilePicker({
    suggestedName:filename,
    types:[{description:'RFLink replay JSON',accept:{'application/json':['.json']}}]
   });
   const writer=await handle.createWritable();
   await writer.write(contents);await writer.close();
   e('rf-replay-status-a').textContent='Replay-rapport privé opgeslagen.';
   return;
  }catch(error){if(error?.name==='AbortError')return;}
 }
 const url=URL.createObjectURL(new Blob([contents],{type:'application/json'}));
 const link=make('a');
 link.href=url;link.download=filename;link.hidden=true;
 document.body.append(link);link.click();link.remove();
 setTimeout(()=>URL.revokeObjectURL(url),30000);
 e('rf-replay-status-a').textContent=
  'Rapport aangeboden aan de browser. Als Ingress de naam wijzigt, gebruik Kopieer rapport A.';
});
draw();
})();