/* Read-only RF Replay in HA Ingress. Data remains in this browser tab.
 * Never render imported RF metadata as HTML or forward it to external URLs.
 */
(() => {
'use strict';
const e=id=>document.getElementById(id), files={a:null,b:null};
const MAX=16*1024*1024, PAGE=40;
let current=0, busy=false, selected433Group=null;
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
 invalid_fifo_hex_or_length:'Ongeldige FIFO-data',
 carrier_window_observation_only:'Carrier window · observation only',
 sync_no_match_in_window:'No match in retained bit window',
 sync_itho_structure_only:'Itho structure only (unverified)',
 sync_ramses_overlap_or_partial:'RAMSES match with overlap/partial evidence',
 sync_ramses_checksum_valid:'RAMSES-II checksum-valid window',
 sync_window_scan_failed:'Window scan failed'
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
 speed:r.speed??null,candidates:r.candidates||[],sync_scan:r.sync_scan??null});
}
function label(row){
 if(!row?.result)return '—';
 const r=row.result;
 if(r.sync_scan)return reasons[r.reason]||String(r.reason||'Unknown');
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
 for(const match of (Array.isArray(row?.rtl433?.matches)?row.rtl433.matches:[]).slice(0,16)){
  cell.append(make('small','rtl_433: '+String(match.model||'').slice(0,80)+' · '+
    String(match.integrity||'not reported').slice(0,20)+
    (match.device_candidate?' · candidate '+String(match.device_candidate).slice(0,12):'')+
    ' · independent decoder match'));
 }
 const sync=row?.rf868_evidence;
 if(sync){
  cell.append(make('small','868 RAW · '+sync.bit_count+' bits · prefix '+sync.alternating_prefix+
   (row.format==='carrier_bits_hex'?' · window transitions ':' · tail transitions ')+sync.tail_transitions+' · carrier: '+sync.carrier_state));
  if(row.format==='carrier_bits_hex'&&Number.isFinite(row.activity_id))cell.append(make('small','Carrier activity #'+row.activity_id));
  if(sync.group_count>=2)cell.append(make('small','Identical bit windows '+sync.group_id+
   ' · '+sync.group_count+' distinct capture IDs · not verified repeat transmissions'));
  const c=row.carrier_context;
  if(c?.state==='sampled'&&Number.isFinite(c.min_rssi_dbm_x2)&&Number.isFinite(c.max_rssi_dbm_x2))
   cell.append(make('small','Carrier high '+c.carrier_samples+'/'+c.samples+
    ' · RSSI '+(c.min_rssi_dbm_x2/2).toFixed(1)+' to '+(c.max_rssi_dbm_x2/2).toFixed(1)+' dBm'));
  const scan=row.result?.sync_scan;
  if(scan)cell.append(make('small','Itho structures '+scan.itho_structure_only+
   ' · RAMSES checksum hits '+scan.ramses_checksum_valid+
   ' · embedded '+scan.ramses_embedded_in_itho+
   (scan.hits_truncated?' · partial hit list':'')));
 }
 const evidence=row?.rf433_evidence;
 if(evidence?.group_id && evidence?.group_count>=2)
  cell.append(make('small','433 herhalingskandidaat '+evidence.group_id+
   ' · '+evidence.group_count+' afzonderlijke frames · niet gevalideerd'));
 if(Number.isFinite(evidence?.duration_count_delta)){
  const countDelta=evidence.duration_count_delta;
  cell.append(make('small','RMT → decoder: '+evidence.pre_filter_durations+' → '+
   evidence.decoder_input_durations+' duurwaarden (verschil '+(countDelta>0?'+':'')+countDelta+
   '; geen bewezen RF-verlies)'));
  if(Number.isFinite(evidence.pre_filter_median_duration_us)&&
     Number.isFinite(evidence.decoder_input_median_duration_us))
   cell.append(make('small','Mediaan pulstijd (µs): '+
    evidence.pre_filter_median_duration_us+' → '+evidence.decoder_input_median_duration_us+
    ' · zeer kort (<80 µs): '+evidence.pre_filter_short_durations+
    ' → '+evidence.decoder_input_short_durations));
 }
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
 const external=report.rtl433_comparison;
 if(external?.version===1){
  const stats=external.summary||{};
  el.append(make('p','rtl_433 · '+String(external.state)+' · '+Number(stats.tested||0)+' / '+
   Number(stats.eligible||0)+' original OOK captures tested · '+Number(stats.matched_captures||0)+
   ' decoder matches · '+Number(stats.repeated_device_candidates||0)+' repeated anonymous device candidates.','minor'));
  for(const g of (Array.isArray(external.models)?external.models:[]).slice(0,8))
   el.append(make('p',String(g.model||'').slice(0,80)+' · '+(radios[g.radio]||'433 MHz')+
    ' · '+Number(g.captures)+' matching captures. Device not verified.','minor'));
  for(const g of (Array.isArray(external.device_candidates)?external.device_candidates:[]).slice(0,8))
   el.append(make('p',String(g.label||'').slice(0,12)+' · '+String(g.model||'').slice(0,80)+
    ' · '+Number(g.captures)+' captures with same reported ID/channel (anonymous).','minor'));
 }
 const families=report.signal_families;
 if(families?.version===1){
  el.append(make('p','Timing families: '+Number(families.summary?.repeated_families||0)+
   ' repeated families; not verified devices or protocols.','minor'));
  for(const g of (Array.isArray(families.families)?families.families:[]).slice(0,8)){
   const kind=({pulse_distance:'pulse distance',pulse_width:'pulse width',complementary_widths:'complementary widths'})[g.kind]||'timing hypothesis';
   el.append(make('p',(radios[g.radio]||'433 MHz')+' · '+kind+' · '+Number(g.bit_count)+
    ' candidate bits · '+Number(g.captures)+' captures · '+Number(g.distinct_patterns)+' patterns · '+
    Number(g.fixed_positions)+' fixed / '+Number(g.variable_positions)+' variable positions'+
    (g.interval_count>0?' · median observed gap '+(Number(g.median_interval_ms)/1000).toFixed(2)+' s':'')+
    (g.inverse32_tested>0?' · inverted 32-bit fields: '+Number(g.inverse32_exact)+' exact / '+Number(g.inverse32_tested)+' tested':''),
    'minor'));
  }
 }
 const sync=report.rf868_insights?.summary;
 if(sync?.windows)el.append(make('p',sync.windows+' automatic 868 windows ('+(sync.carrier_windows||0)+' carrier-triggered) · '+
  (sync.exact_repeat_groups||0)+' identical-window groups · '+
  (sync.windows_with_carrier_high||0)+' windows with sampled carrier high.','minor'));
}
// Share a bounded, deliberately whitelisted diagnostic summary rather than
// thousands of RF records. No pulse arrays, 868 payload, candidate bitstrings,
// IPs, credentials or raw CSV metadata can enter this object.
function compactReplayDiagnosis(report){
 if(!valid(report))return null;
 const counts=report.summary||{},input=report.input_counters||{},
  info=report.rf433_insights||{},stats=info.summary||{};
 const countKeys=[
  'event_rows','events_without_rf_details','non_rf_details',
  'missing_or_expired_rf_details','invalid_captures','capture_limit_reached'
 ];
 const summaryKeys=[
  'insufficient_signed_pulses','no_stable_timing_candidate','candidate_timings_only',
  'not_alternating_high_low','not_enough_pairs','legacy_preamble_absent',
  'built_in_decoder_rejected','native_decoder_not_run','433_with_timing_hypotheses',
  'carrier_window_observation_only','sync_no_match_in_window','sync_itho_structure_only','sync_ramses_overlap_or_partial',
  'sync_ramses_checksum_valid','sync_window_scan_failed'
 ];
 const insightKeys=[
  'paired_pre_post_filter','pairs_with_duration_count_change',
  'physical_capture_candidates','eligible_for_grouping',
  'too_short_or_irregular_for_grouping','groups_found_including_singletons',
  'repeated_groups','frames_in_repeated_groups','comparison_budget_exhausted'
 ];
 const waveformKeys=[
  'paired_pre_post_filter','pairs_with_duration_count_change',
  'physical_capture_candidates','eligible','too_few_durations',
  'non_alternating_phases','insufficient_high_low','no_timing_scale',
  'count_1_to_15','count_16_to_23','count_24_to_63','count_64_plus',
  'max_duration_count','terminal_idle_candidates_removed',
  'frames_with_under_80us','frames_with_over_4000us',
  'distinct_length_polarity_buckets','frames_with_length_polarity_peer',
  'length_polarity_buckets_with_peers','same_length_similarity_checks',
  'same_length_similarity_matches','same_length_similarity_rejections'
 ];
 const safeNumber=n=>Number.isSafeInteger(n)&&n>=0?n:0;
 const selectCounts=(obj,keys)=>Object.fromEntries(keys.map(k=>[k,safeNumber(obj[k])]));
 const samples={};
 for(const radio of [0,1]){
  // The post-filter representative avoids displaying the same RX frame twice.
  const selected=report.results.filter(r=>Number(r.radio)===radio&&
    r.format==='signed_pulses_us'&&
    !(r.capture_phase==='pre_filter_rmt'&&r.rf433_evidence?.paired_capture_id));
  selected.sort((a,b)=>safeNumber(b.count)-safeNumber(a.count));
  samples['radio_'+radio]=selected.slice(0,6).map(r=>({
   capture_id:String(r.capture_id||'').slice(0,20),
   boot_id:String(r.boot_id??'').slice(0,32),
   sequence:String(r.sequence??'').slice(0,32),
   uptime_ms:Number.isSafeInteger(Number(r.uptime_ms))?Number(r.uptime_ms):null,
   phase:String(r.capture_phase||'').slice(0,28),
   duration_count:safeNumber(r.count),
   reason:String(r.result?.reason||'').slice(0,50),
   pre_filter_durations:safeNumber(r.rf433_evidence?.pre_filter_durations),
   decoder_input_durations:safeNumber(r.rf433_evidence?.decoder_input_durations),
   paired:Boolean(r.rf433_evidence?.paired_capture_id),
   pre_post_morphology:r.rf433_evidence?.paired_capture_id?{
    pre_median_us:Number.isFinite(r.rf433_evidence.pre_filter_median_duration_us)
       ?Math.max(0,Math.min(5000000,r.rf433_evidence.pre_filter_median_duration_us)):null,
    post_median_us:Number.isFinite(r.rf433_evidence.decoder_input_median_duration_us)
       ?Math.max(0,Math.min(5000000,r.rf433_evidence.decoder_input_median_duration_us)):null,
    pre_under_80us:safeNumber(r.rf433_evidence.pre_filter_short_durations),
    post_under_80us:safeNumber(r.rf433_evidence.decoder_input_short_durations)
   }:null,
   waveform:r.rf433_waveform?{
    grouping_eligibility:['eligible','too_few_durations','non_alternating_phases',
      'insufficient_high_low','no_timing_scale'].includes(r.rf433_waveform.grouping_eligibility)
      ?r.rf433_waveform.grouping_eligibility:'unknown',
    high_durations:safeNumber(r.rf433_waveform.high_durations),
    low_durations:safeNumber(r.rf433_waveform.low_durations),
    alternation_breaks:safeNumber(r.rf433_waveform.alternation_breaks),
    median_duration_us:Number.isFinite(r.rf433_waveform.median_duration_us)
      ?Math.max(0,Math.min(5000000,r.rf433_waveform.median_duration_us)):null,
    p90_duration_us:safeNumber(r.rf433_waveform.p90_duration_us),
    max_duration_us:safeNumber(r.rf433_waveform.max_duration_us),
    durations_under_80us:safeNumber(r.rf433_waveform.durations_under_80us),
    durations_over_4000us:safeNumber(r.rf433_waveform.durations_over_4000us),
    terminal_idle_candidate_removed:Boolean(r.rf433_waveform.terminal_idle_candidate_removed)
   }:null
  }));
 }
 const repeats=Array.isArray(info.repeated_groups)?info.repeated_groups:[];
 return {
  schema:'rflink-replay-diagnostic-summary-v1',
  report_schema:report.schema,
  capture_id_scheme:String(report.capture_id_scheme||'legacy').slice(0,40),
  decoder_mode:String(report.decoder_mode||'unknown').slice(0,40),
  decoder_fingerprint:String(report.decoder_fingerprint||'unknown').slice(0,32),
  capture_count:safeNumber(report.capture_count),
  analyzed_results:report.results.length,
  results_truncated:Boolean(report.limits?.results_truncated),
  warning_count:Array.isArray(report.warnings)?report.warnings.length:0,
  input_counters:selectCounts(input,countKeys),
  per_radio:selectCounts(counts,['radio_0','radio_1','radio_2']),
  outcome_counts:selectCounts(counts,summaryKeys),
  rf433_insights:{
   available:info.version===1,
   summary:selectCounts(stats,insightKeys),
   per_radio_waveform:{
    radio_0:selectCounts(info.per_radio_waveform?.radio_0||{},waveformKeys),
    radio_1:selectCounts(info.per_radio_waveform?.radio_1||{},waveformKeys)
   },
   repeated_groups:repeats.slice(0,12).map(g=>({
    group_id:String(g.group_id||'').slice(0,16),
    radio:Number(g.radio)===1?1:0,
    independent_frame_count:safeNumber(g.count),
    verified_protocol:false
   }))
  },
  rf868_insights:{available:report.rf868_insights?.version===1,
   summary:selectCounts(report.rf868_insights?.summary||{},['windows','carrier_windows','exact_repeat_groups',
    'windows_in_repeat_groups','windows_with_carrier_high','carrier_sampled','carrier_not_recorded',
    'carrier_invalid_timing','carrier_history_unavailable','carrier_no_matching_samples'])},
  signal_families:{available:report.signal_families?.version===1,
   summary:selectCounts(report.signal_families?.summary||{},['original_captures','timing_candidates','duplicate_rows_ignored',
    'group_limit_skipped','repeated_families','captures_in_repeated_families']),
   families:(Array.isArray(report.signal_families?.families)?report.signal_families.families:[]).slice(0,12).map(g=>({
    radio:Number(g.radio)===1?1:0,
    ...selectCounts(g,['bit_count','captures','distinct_patterns','largest_exact_repeat','fixed_positions',
     'variable_positions','inverse32_exact','inverse32_one_difference','inverse32_tested','interval_count','median_interval_ms']),
    protocol_verified:false
   }))},
  rtl433_comparison:{available:report.rtl433_comparison?.version===1,
   summary:selectCounts(report.rtl433_comparison?.summary||{},['eligible','tested','runs','errors',
    'not_tested_limit','duplicates_ignored','matched_captures','model_groups','device_candidates','repeated_device_candidates'])},
  longest_433_capture_examples:samples,
  interpretation:[
   'Groepen zijn onzekere timingovereenkomsten, geen herkende apparaten.',
   'Voor/na filtering vergelijkt aantallen duurwaarden, niet bewezen verloren RF-pulsen.',
   'RAW-data, mogelijke codebits en 868 FIFO-payload ontbreken bewust in deze diagnose.'
  ]
 };
}
function compactReplayComparison(a,b){
 if(!valid(a)||!valid(b))return null;
 const am=index(a),bm=index(b);
 let shared=0,same=0,changed=0;
 for(const [id,row] of am){
  if(!bm.has(id))continue;
  shared++;
  if(signature(row)===signature(bm.get(id)))same++;else changed++;
 }
 return {
  compatible_capture_id_scheme:a.capture_id_scheme==='stable-source-v2'&&
                                b.capture_id_scheme==='stable-source-v2',
  shared_capture_ids:shared,
  identical_decoder_results:same,
  changed_decoder_results:changed,
  only_a:am.size-shared,
  only_b:bm.size-shared,
  note:'Aantal verschillen kan door nieuwe/overschreven snapshots komen. '+
       'Een veranderde decoderuitkomst vereist identiek capture-ID.'
 };
}
function draw433Insights(slot){
 const node=e('rf-replay-insights-'+slot);
 const report=files[slot],evidence=report?.rf433_insights;
 node.replaceChildren();
 const heading=make('strong','Analyse '+slot.toUpperCase());
 node.append(heading);
 if(!report){node.append(make('p','Nog geen snapshot geladen.','minor'));return;}
 if(!evidence||evidence.version!==1||!evidence.summary){
  node.append(make('p','Dit oudere Replay-rapport heeft nog geen 433-herhalingsanalyse. Importeer of analyseer de captures opnieuw.','minor'));
  return;
 }
 const summary=evidence.summary;
 const frames=Number(summary.physical_capture_candidates||0),
  paired=Number(summary.paired_pre_post_filter||0),
  changed=Number(summary.pairs_with_duration_count_change||0),
  repeated=Number(summary.repeated_groups||0);
 node.append(make('p',frames+' framekandidaten · '+paired+' originele/gefilterde paren · '+
  changed+' paren met gewijzigde aantallen duurwaarden · '+repeated+
  ' herhalingsgroepen.','minor'));
 const perRadio=evidence.per_radio_waveform||{};
 for(const radio of [0,1]){
  const d=perRadio['radio_'+radio];
  if(!d)continue;
  const count=Number(d.physical_capture_candidates||0),
        short=Number(d.too_few_durations||0),
        alternating=Number(d.non_alternating_phases||0),
        eligible=Number(d.eligible||0),
        longCount=Number(d.count_64_plus||0);
  const peers=Number(d.frames_with_length_polarity_peer||0),
        checked=Number(d.same_length_similarity_checks||0),
        rejected=Number(d.same_length_similarity_rejections||0);
  node.append(make('p',radios[radio]+': '+count+' framekandidaten; '+
    short+' te kort voor groepering; '+alternating+' met onregelmatige hoog/laag-afwisseling; '+
    eligible+' voldoende regelmatig. '+longCount+' met minstens 64 duurwaarden. '+
    peers+' delen lengte én beginpolariteit met minstens één ander frame; '+
    checked+' exacte vormvergelijkingen ('+rejected+' afgewezen).' ,'minor'));
 }
 if(!repeated){
  node.append(make('p','Geen voldoende gelijkende lange 433 MHz-pulstreinen gevonden. Dit sluit geldige RF-berichten niet uit.','minor'));
  return;
 }
 const groups=Array.isArray(evidence.repeated_groups)?evidence.repeated_groups:[];
 for(const group of groups.slice(0,8)){
  if(!group||!['0','1'].includes(String(group.radio))||!Number.isInteger(group.count)||
     typeof group.group_id!=='string'||group.group_id.length>16)continue;
  const item=make('div',undefined,'rf-replay-group-item');
  item.append(make('span',(radios[group.radio]||'433 MHz')+
   ' · groep '+group.group_id+' · '+group.count+' herhalingen'));
  const view=make('button','Bekijk groep','secondary');
  view.type='button';
  view.addEventListener('click',()=>{
   selected433Group={slot,id:group.group_id};
   current=0;draw();
  });
  item.append(view);
  node.append(item);
 }
 if(groups.length>8){
  node.append(make('p','Toont 8 van '+groups.length+
   ' groepen. De overige staan in het JSON-rapport.','minor'));
 }
 if(Number(summary.comparison_budget_exhausted||0)>0)
  node.append(make('p','Een deel van de ruisachtige captures is niet gegroepeerd wegens de veiligheidslimiet voor vergelijkingen.','minor'));
}
function draw(){
 summary('a');summary('b');
 draw433Insights('a');draw433Insights('b');
 e('rf-replay-group-clear').hidden=!selected433Group;
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
  if(selected433Group &&
     (selected433Group.slot==='a'?x?.rf433_evidence?.group_id:y?.rf433_evidence?.group_id)!==
     selected433Group.id)continue;
  pairs.push({id,x,y,row,state});
 }
 e('rf-replay-compare').textContent=files.a&&files.b
  ?'Gedeelde capture-ID’s: '+shared+' · zelfde resultaat: '+unchanged+
    ' · gewijzigde decoderresultaten: '+changed+
    '. Verschillende IDs betekenen niet automatisch een decoderwijziging.'
  :files.a?'Analyse A: '+a.size+' captures. Importeer B voor vergelijking.'
  :files.b?'Analyse B: '+b.size+' captures. Importeer A voor vergelijking.'
  :'Importeer een CSV of RF capture JSON om te beginnen.';
 if(files.a&&files.b &&
    (files.a.capture_id_scheme!=='stable-source-v2'||
     files.b.capture_id_scheme!=='stable-source-v2')){
  e('rf-replay-compare').textContent+=
    ' · Let op: een oud Replay-rapport heeft tijdsafhankelijke capture-ID’s. '+
    'A/B-matches kunnen daardoor ontbreken. Haal beide snapshots opnieuw op '+
    'met dezelfde Builder-versie voor betrouwbare vergelijking.';
 }
 if(selected433Group){
  e('rf-replay-compare').textContent+=' · Filter: groep '+selected433Group.id+
   ' uit analyse '+selected433Group.slot.toUpperCase()+'.';
 }
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
 e('rf-replay-copy-diagnosis').disabled=!files.a&&!files.b;
 e('rf-replay-save-report').disabled=!files.a||preparingExport;
 e('rf-replay-show-export').disabled=!files.a&&!files.b;
 e('rf-replay-export-kind').options[1].disabled=!files.a;
 if(!files.a)e('rf-replay-export-kind').value='compact';
 if(e('rf-replay-export-panel').open)refreshExportText();
}
function setLiveButtons(enabled){
 e('rf-replay-live-a').disabled=!enabled||busy;
 e('rf-replay-live-b').disabled=!enabled||busy;
}
async function refreshLiveDevices(){
 const select=e('rf-replay-live-device'),note=e('rf-replay-live-status');
 const prior=select.value;
 if(busy)return;
 e('rf-replay-live-refresh').disabled=true;
 note.textContent='Gekoppelde RFLink-apparaten controleren…';
 try{
  const response=await fetch('api/devices',{cache:'no-store'});
  if(!response.ok)throw new Error('Apparatenlijst HTTP '+response.status);
  const data=await response.json();
  if(!data||!Array.isArray(data.devices))throw new Error('Ongeldige apparatenlijst');
  // Device IDs come only from Builder's saved, verified hardware inventory.
  // The server will independently re-verify identity before RF access.
  const devices=data.devices.filter(d=>d && d.managed===true &&
    d.online===true && d.identity_verified===true &&
    typeof d.id==='string' && /^(?:rf-[0-9a-f]{12}|cfg-[0-9a-f]{18})$/.test(d.id));
  const options=[make('option','Kies een online RFLink')];
  options[0].value='';
  for(const device of devices){
   const label=String(device.name||'RFLink').slice(0,60);
   const option=make('option',label+' · '+device.id);
   option.value=device.id;
   options.push(option);
  }
  select.replaceChildren(...options);
  if(devices.some(d=>d.id===prior))select.value=prior;
  else if(devices.length===1)select.value=devices[0].id;
  setLiveButtons(Boolean(select.value));
  note.textContent=devices.length
   ?devices.length+' gekoppelde online RFLink(s) beschikbaar. Klik op Ophalen en analyseren.'
   :'Geen gekoppelde, geverifieerde RFLink online. Controleer Devices en probeer opnieuw.';
 }catch(error){
  select.replaceChildren(make('option','Apparaten niet beschikbaar'));
  select.value='';
  setLiveButtons(false);
  note.textContent='Apparaten ophalen mislukt: '+String(error?.message||'Onbekend').slice(0,160);
 }finally{e('rf-replay-live-refresh').disabled=false;}
}
async function importLiveDevice(slot){
 if(busy)return;
 const identifier=e('rf-replay-live-device').value;
 const note=e('rf-replay-live-status');
 if(!/^(?:rf-[0-9a-f]{12}|cfg-[0-9a-f]{18})$/.test(identifier)){
  note.textContent='Selecteer eerst een geverifieerde online RFLink.';return;
 }
 busy=true;setLiveButtons(false);
 e('rf-replay-live-refresh').disabled=true;
 const status=e('rf-replay-status-'+slot);
 note.textContent='RFLink maakt een tijdelijke, volledige CSV-snapshot. Daarna analyseert de Builder de nog bewaarde RF-captures…';
 status.textContent='Ophalen en analyseren vanuit RFLink…';
 try{
  const response=await fetch('api/replay/device',{
   method:'POST',cache:'no-store',
   headers:{'X-RFLink-CSRF':csrf,'Content-Type':'application/json'},
   body:JSON.stringify({device_id:identifier})
  });
  let report=null;
  try{report=await response.json();}
  catch{throw new Error('Builder gaf geen geldige JSON terug');}
  if(!response.ok)throw new Error(String(report.error||'Live analyse mislukt'));
  if(!valid(report))throw new Error('Ongeldig Replay v1-rapport ontvangen');
  files[slot]=report;selected433Group=null;current=0;draw();
  const truncated=report.limits?.results_truncated?' (5.000-resultatenlimiet)':'';
  status.textContent=report.capture_count+' RF-captures uit gekoppelde RFLink geanalyseerd'+truncated+
    ' · alleen in deze browsersessie.';
  note.textContent='Snapshot '+(slot==='a'?'A':'B')+' opgehaald. Geen bestand handmatig nodig. '+
    'Overschreven RF-details kunnen niet worden teruggehaald.';
 }catch(error){
  const message=String(error?.message||'Onbekende fout').slice(0,220);
  status.textContent='Live analyse mislukt: '+message;
  note.textContent='Geen nieuw rapport geladen. '+message;
 }finally{
  busy=false;
  e('rf-replay-live-refresh').disabled=false;
  setLiveButtons(Boolean(e('rf-replay-live-device').value));
 }
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
  files[slot]=report;selected433Group=null;current=0;
  status.textContent=String(report.capture_count)+' captures geanalyseerd · '+String(report.decoder_mode||'offline')+
   ' · alleen in deze browsersessie.';
  draw();
 }catch(err){status.textContent='Analyse mislukt: '+String(err?.message||'Onbekende fout').slice(0,220);}
 finally{busy=false;button.disabled=false;button.removeAttribute('aria-busy');}
}
e('rf-replay-live-device').addEventListener('change',()=>setLiveButtons(Boolean(e('rf-replay-live-device').value)));
e('rf-replay-live-refresh').addEventListener('click',()=>{void refreshLiveDevices();});
e('rf-replay-live-a').addEventListener('click',()=>{void importLiveDevice('a');});
e('rf-replay-live-b').addEventListener('click',()=>{void importLiveDevice('b');});
document.querySelector('.nav-tab[data-page="replay"]')?.addEventListener('click',()=>{void refreshLiveDevices();});
void refreshLiveDevices();
e('header-replay').addEventListener('click',()=>{
 document.querySelector('.nav-tab[data-page="replay"]')?.click();
});
for(const slot of ['a','b'])e('rf-replay-run-'+slot).addEventListener('click',()=>importFile(slot));
for(const key of ['rf-replay-radio','rf-replay-outcome'])
 e(key).addEventListener('change',()=>{current=0;draw();});
e('rf-replay-group-clear').addEventListener('click',()=>{
 selected433Group=null;current=0;draw();
});
e('rf-replay-prev').addEventListener('click',()=>{current--;draw();});
e('rf-replay-next').addEventListener('click',()=>{current++;draw();});
function diagnosisPayload(){
 return {
  schema:'rflink-replay-diagnosis-bundle-v1',
  A:files.a?compactReplayDiagnosis(files.a):null,
  B:files.b?compactReplayDiagnosis(files.b):null,
  comparison:compactReplayComparison(files.a,files.b)
 };
}
let preparedReport=null, preparingExport=false, downloadRun=0, downloading=false;
function clearDownloadLink(){
 const link=e('rf-replay-download-link');
 link.hidden=true;link.removeAttribute('href');preparedReport=null;
 downloadRun++;downloading=false;
 e('rf-replay-download-progress').hidden=true;
}
function refreshExportText(){
 if(preparedReport&&preparedReport!==files.a)clearDownloadLink();
 const kind=e('rf-replay-export-kind').value;
 const payload=kind==='full'?files.a:(files.a||files.b?diagnosisPayload():null);
 e('rf-replay-export-text').value=payload?JSON.stringify(payload,null,2):'';
}
function showExport(kind,select=true){
 const panel=e('rf-replay-export-panel');
 panel.hidden=false;panel.open=true;
 e('rf-replay-export-kind').value=kind==='full'&&files.a?'full':'compact';
 refreshExportText();
 const field=e('rf-replay-export-text');
 if(select){field.focus();field.select();field.setSelectionRange(0,field.value.length);}
 return field;
}
async function copyExport(kind){
 const field=showExport(kind);
 const status=e('rf-replay-export-status');
 try{
  if(!navigator.clipboard?.writeText)throw Error('clipboard unavailable');
  await navigator.clipboard.writeText(field.value);
  status.textContent='Rapport gekopieerd.';
  return;
 }catch{
  field.focus();field.select();
  try{
   if(document.execCommand('copy')){
    status.textContent='Rapport gekopieerd via browserfallback.';
    return;
   }
  }catch{}
  status.textContent='Automatisch kopiëren geblokkeerd. De tekst is geselecteerd; kopieer handmatig.';
 }
}
e('rf-replay-show-export').addEventListener('click',()=>showExport('compact'));
e('rf-replay-export-kind').addEventListener('change',()=>{refreshExportText();e('rf-replay-export-status').textContent='';});
e('rf-replay-select-export').addEventListener('click',()=>showExport(e('rf-replay-export-kind').value));
e('rf-replay-copy-diagnosis').addEventListener('click',()=>{if(files.a||files.b)void copyExport('compact');});
e('rf-replay-copy-report').addEventListener('click',()=>{if(files.a)void copyExport('full');});
// Same native anchor action and read-only status polling as ESP full CSV.
e('rf-replay-download-link').addEventListener('click',event=>{
 const link=e('rf-replay-download-link');
 if(downloading||!preparedReport||preparedReport!==files.a||!link.hasAttribute('href')){
  event.preventDefault();return;
 }
 downloading=true;
 const run=++downloadRun, started=Date.now();
 const box=e('rf-replay-download-progress'),meter=e('rf-replay-download-meter');
 const label=e('rf-replay-download-percent'),state=e('rf-replay-download-state');
 const feedback=e('rf-replay-download-feedback');
 const random=new Uint8Array(12);
 if(window.crypto&&window.crypto.getRandomValues)window.crypto.getRandomValues(random);
 else for(let i=0;i<random.length;i++)random[i]=Math.floor(Math.random()*256);
 const id=Array.from(random,v=>v.toString(16).padStart(2,'0')).join('');
 const url=new URL(link.href,window.location.href);
 url.searchParams.set('export_id',id);
 link.href=url.href;
 const statusUrl=new URL(url.href);statusUrl.pathname+='/status';
 box.hidden=false;meter.removeAttribute('value');label.textContent='—';
 state.textContent='JSON-overdracht voorbereiden…';feedback.textContent='Builder maakt een tijdelijke download klaar.';
 const finish=(ok,message)=>{
  if(run!==downloadRun)return;
  downloading=false;
  if(ok){meter.value=100;label.textContent='100%';}
  else{meter.removeAttribute('value');label.textContent='Mislukt';}
  state.textContent=ok?'JSON-overdracht voltooid':'JSON-overdracht mislukt';
  feedback.textContent=message;
 };
 const poll=async()=>{
  if(run!==downloadRun||!downloading)return;
  try{
   const response=await fetch(statusUrl.href,{cache:'no-store'});
   if(!response.ok)throw Error('HTTP '+response.status);
   const p=await response.json();
   if(run!==downloadRun)return;
   const sent=Number(p.sent)||0,total=Number(p.total)||0;
   if(p.state==='done'){
    finish(true,Math.ceil(sent/1024)+' KiB via Home Assistant doorgestuurd. Controleer je downloads.');return;
   }
   if(p.state==='failed'){
    const reasons={report_expired:'Rapport verlopen. Bereid de download opnieuw voor.',
     size_limit:'Rapport overschrijdt de exportlimiet.',
     client_disconnected:'Verbinding met Home Assistant of browser onderbroken.',
     downstream_disconnected:'Versturen naar Home Assistant of browser afgebroken.',
     upstream_timeout:'Tijdslimiet bij overdracht bereikt.'};
    finish(false,reasons[p.error]||'Overdracht afgebroken ('+String(p.error||'onbekend')+').');return;
   }
   if(p.state==='streaming'&&total>0){
    const pct=Math.min(99,Math.floor(sent/total*100));
    meter.value=pct;label.textContent=pct+'%';
    state.textContent='JSON naar je browser versturen…';
    feedback.textContent=Math.ceil(sent/1024)+' / '+Math.ceil(total/1024)+' KiB doorgestuurd.';
   }
  }catch(error){
   if(run!==downloadRun)return;
   feedback.textContent='Voortgang tijdelijk niet bereikbaar: '+String(error?.message||error).slice(0,100);
  }
  if(Date.now()-started>180000){
   finish(false,'Geen bevestigde overdracht. Controleer je downloads of probeer opnieuw.');return;
  }
  setTimeout(poll,400);
 };
 // Do not prevent the native anchor default action, as in the ESP log viewer.
 setTimeout(poll,250);
});
e('rf-replay-save-report').addEventListener('click',async()=>{
 if(!files.a||preparingExport)return;
 const report=files.a;
 const contents=JSON.stringify(report);
 const field=showExport('full',false);
 const status=e('rf-replay-export-status'),button=e('rf-replay-save-report');
 clearDownloadLink();
 const size=new TextEncoder().encode(contents).length;
 if(size>16*1024*1024){
  status.textContent='Rapport groter dan 16 MiB ('+size+' bytes). Kopieer het handmatig.';
  field.focus();field.select();return;
 }
 preparingExport=true;button.disabled=true;
 status.textContent='Download voorbereiden ('+Math.ceil(size/1024)+' KiB)…';
 try{
  const response=await fetch('api/replay/export',{
   method:'POST',cache:'no-store',
   headers:{'X-RFLink-CSRF':csrf,'Content-Type':'application/json'},
   body:contents
  });
  let payload=null;
  try{payload=await response.json();}catch{}
  if(!response.ok)throw Error('HTTP '+response.status+(typeof payload?.error==='string'?' – '+payload.error:''));
  if(typeof payload?.token!=='string'||!/^[A-Za-z0-9_-]{43}$/.test(payload.token))
   throw Error('HTTP '+response.status+' – geen geldige downloadlink ontvangen');
  if(files.a!==report)throw Error('Analyse A is gewijzigd; probeer opnieuw');
  const link=e('rf-replay-download-link');
  link.href='api/replay/download/'+payload.token;
  link.hidden=false;preparedReport=report;
  status.textContent='Rapport klaar ('+Math.ceil(size/1024)+' KiB). Klik op Download JSON-bestand. Link geldig 5 minuten.';
 }catch(error){
  status.textContent='Download voorbereiden mislukt: '+String(error?.message||error).slice(0,220)+'. Het rapport blijft hieronder kopieerbaar.';
 }finally{preparingExport=false;button.disabled=!files.a;}
});
draw();
})();