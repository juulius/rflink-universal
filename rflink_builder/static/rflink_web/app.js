/* Native Radioactive web app. No ESPHome runtime and no legacy RFLink REST calls.
 * All radio actions are read-only in this stage; never simulate pairing or TX.
 */
'use strict';
// The Builder serves these vetted assets through Home Assistant Ingress.
// Every device write carries the Builder's existing CSRF header.
const builderProxyCsrf = document.querySelector('meta[name="builder-proxy-csrf"]')?.content || '';
if (builderProxyCsrf) {
  const nativeFetch = window.fetch.bind(window);
  window.fetch = (url, options = {}) => {
    const method = String(options.method || 'GET').toUpperCase();
    if (method === 'GET' || method === 'HEAD') return nativeFetch(url, options);
    const headers = new Headers(options.headers || {});
    headers.set('X-RFLink-CSRF', builderProxyCsrf);
    return nativeFetch(url, {...options, headers});
  };
  const xhrOpen = XMLHttpRequest.prototype.open;
  XMLHttpRequest.prototype.open = function(method, ...args) {
    this._rfBuilderMethod = String(method).toUpperCase();
    return xhrOpen.call(this, method, ...args);
  };
  const xhrSend = XMLHttpRequest.prototype.send;
  XMLHttpRequest.prototype.send = function(body) {
    if (this._rfBuilderMethod === 'POST') this.setRequestHeader('X-RFLink-CSRF', builderProxyCsrf);
    return xhrSend.call(this, body);
  };
}

const $=id=>document.getElementById(id);
const pages=[...document.querySelectorAll('.page')];
const nav=[...document.querySelectorAll('[data-page]')];
const radioNames=['433.42 MHz','433.92 MHz','868.30 MHz'];
let currentPage='status', lastStatus=null, lastEvents=[], logSource='all';
let consoleConnected=false, consoleCursor=0, consoleBootId=null, consoleBusy=false;
let statusBusy=false, logsBusy=false, historyBusy=false;
let logPageCursors=[0],logSnapshot=0,logHasOlder=false,logLivePaused=false;
const webPages=['status','devices','logs','learn','diagnostics','settings'];
const webActions=['page_open','log_source','log_radio','log_noise','log_expand',
  'log_collapse','log_export','log_older','log_newer','log_latest',
  'console_connect','console_disconnect','console_clear','diagnostics_refresh',
  'settings_submit','led_submit','ota_confirm','ota_result','api_client_error',
  'radio_inspect'];
const webEndpoints=['api/status','api/diagnostics/crashes','api/diagnostics/console',
  'api/logs','api/logs.csv','api/logs/config','api/logs/ui','api/led',
  'api/events','api/events.csv','api/mqtt/status','api/mqtt/config',
  'api/ota','setup','api/radio/capture'];
const webErrorTimes=new Map();
function auditUi(action,value=0,endpoint=0){
  if(!webActions.includes(action)||!webPages.includes(currentPage))return;
  // Only predefined action/page IDs reach RFLink. Never transmit form contents,
  // captured RF data, passwords, OTA filenames or request URL queries.
  fetch('api/logs/ui',{method:'POST',headers:{'Content-Type':'application/json'},
    body:JSON.stringify({action,page:currentPage,value,endpoint}),keepalive:true})
    .catch(()=>{}); // Logging must never block primary UI actions.
}
function auditApiFailure(path,status){
  const route=String(path).split('?')[0];
  const endpoint=webEndpoints.indexOf(route)+1;
  const key=endpoint+':'+status, now=Date.now();
  if(now-(webErrorTimes.get(key)||0)<60000)return;
  webErrorTimes.set(key,now);
  auditUi('api_client_error',status,endpoint);
}
function resetLogPages(){
  logPageCursors=[0];logSnapshot=0;logHasOlder=false;logLivePaused=false;
  expandedLogKeys.clear();
  $('log-latest').textContent='Latest events';
}
let nextErrorShown=false,otaUploading=false;
const fmt=n=>typeof n==='number'&&Number.isFinite(n)?n.toLocaleString():'—';
const yesNo=v=>v===true?'Yes':v===false?'No':'—';
const hexByte=n=>Number.isInteger(n)&&n>=0&&n<=255?
  '0x'+n.toString(16).toUpperCase().padStart(2,'0'):'—';
// DD.HH.MM.SS shows duration since boot, not clock time.
const uptime=seconds=>{
  if(!Number.isFinite(seconds)||seconds<0)return '—';
  const s=Math.floor(seconds),d=Math.floor(s/86400);
  const h=Math.floor((s%86400)/3600),m=Math.floor((s%3600)/60);
  return [d,h,m,s%60].map(n=>String(n).padStart(2,'0')).join('.');
};
const uptimeMs=milliseconds=>{
  if(!Number.isFinite(milliseconds)||milliseconds<0)return '—';
  const ms=Math.floor(milliseconds),sec=Math.floor(ms/1000);
  const day=Math.floor(sec/86400),hour=Math.floor((sec%86400)/3600);
  const min=Math.floor((sec%3600)/60);
  const pad=n=>String(n).padStart(2,'0');
  return (day?day+'d ':'')+pad(hour)+':'+pad(min)+':'+pad(sec%60)+
    ','+String(ms%1000).padStart(3,'0');
};
const bytes=n=>Number.isFinite(n)&&n>=0?
  (n>=1048576?(n/1048576).toFixed(2)+' MB':
    n>=1024?(n/1024).toFixed(1)+' KB':Math.round(n)+' B'):'—';
function markStatus(el,label,level){
  el.textContent=label;
  el.classList.remove('online','offline','warn');
  if(level)el.classList.add(level);
}
function showPage(page){
  if(!pages.some(p=>p.dataset.view===page))return;
  currentPage=page;
  auditUi('page_open');
  for(const section of pages)section.classList.toggle('active',section.dataset.view===page);
  for(const item of nav){
    const active=item.dataset.page===page;
    item.classList.toggle('active',active);
    if(active)item.setAttribute('aria-current','page');
    else item.removeAttribute('aria-current');
  }
  if(page==='logs')refreshLogs();
  if(page==='diagnostics'){refreshHistory();pollConsole();}
  if(page==='settings')refreshSettings();
  if(page==='status')refreshEvents();
  if(page!=='diagnostics')disconnectConsole();
  window.scrollTo(0,0);
}
for(const item of nav)item.addEventListener('click',()=>showPage(item.dataset.page));
for(const item of document.querySelectorAll('[data-goto]'))
  item.addEventListener('click',()=>showPage(item.dataset.goto));
async function api(path,options={}){
  // Only fixed native same-origin API paths. The browser never calls the old
  // ESPHome /rflink/api paths and never sends credentials in the URL.
  if(!path.startsWith('api/'))throw new Error('Unsupported API path');
  let response;
  try{response=await fetch(path,{cache:'no-store',...options});}
  catch{auditApiFailure(path,0);throw new Error('Cannot reach RFLink over the network');}
  if(!response.ok){
    auditApiFailure(path,response.status);
    throw new Error('RFLink HTTP '+response.status);
  }
  try{return await response.json();}
  catch{auditApiFailure(path,0);throw new Error('RFLink returned invalid JSON');}
}
function offline(error){
  markStatus($('top-state'),'Offline','offline');
  $('side-connection').textContent='RFLink not responding';
  $('side-dot').classList.remove('online');$('side-dot').classList.add('offline');
  $('system-health').textContent='Not reachable';
  $('system-hint').textContent=error.message;
  renderOtaReadiness(null);
  if(!nextErrorShown){console.warn('Native RFLink status unavailable:',error.message);nextErrorShown=true;}
}
function renderStatus(s){
  lastStatus=s;
  nextErrorShown=false;
  renderLogCaptureDiagnostics();
  const connected=s.wifi_connected===true;
  const initialized=s.radio_initialized===true;
  const complete=s.startup_complete===true;
  markStatus($('top-state'),complete?'Online':initialized?'Initializing':'Starting',complete?'online':'warn');
  $('side-connection').textContent=complete?'Device online':'Firmware starting';
  $('side-dot').classList.remove('offline');$('side-dot').classList.add('online');
  $('system-health').textContent=complete?'Running':'Starting up';
  $('system-hint').textContent=complete?'Application startup finished; RF performance not yet verified':
    'Network diagnostics may be available before radio initialization';
  $('firmware-version').textContent=s.firmware||'Unknown';
  $('firmware-meta').textContent=(s.release_channel||'Native')+
    (Number.isInteger(s.release_build)?' · build '+s.release_build:'');
  $('version-pill').textContent=s.firmware_display||s.firmware||'Native v0.7';
  $('side-version').textContent=s.firmware||'v0.7 native ESP-IDF';
  $('settings-firmware').textContent=(s.firmware_display||s.firmware||'Unknown firmware')+
    ' · '+(s.board_id||'Unknown board');
  $('radio-count').textContent=initialized?(s.cc1101_present||[]).filter(Boolean).length+' / 3':'— / 3';
  $('radio-hint').textContent=initialized?'SPI-probed CC1101 modules':'Radio probing not complete';
  const ready=[s.rx43342_listening===true,s.rx43392_listening===true,s.rx868_listening===true]
    .map((listening,i)=>listening&&s.cc1101_present?.[i]===true&&
      s.cc1101_spi_healthy?.[i]===true);
  $('rx-count').textContent=initialized?ready.filter(Boolean).length+' / 3':'— / 3';
  const frames=[s.rx43342_frames,s.rx43392_frames,s.rx868_packets];
  for(let i=0;i<3;i++){
    const present=initialized&&Array.isArray(s.cc1101_present)?s.cc1101_present[i]===true:false;
    const label=!initialized?'Starting':!present?'Not detected':ready[i]?'Listening':'RX inactive';
    markStatus($('radio-status-'+i),label,!initialized?'warn':!present?'offline':ready[i]?'online':'warn');
    $('radio-hw-'+i).textContent=!initialized?'Checking':present?'Detected':'Missing';
    $('radio-frames-'+i).textContent=initialized?fmt(frames[i]):'—';
    $('radio-spi-'+i).textContent=initialized?fmt((s.spi_ready_timeouts||[])[i]):'—';
    $('diag-cc1101-'+i).textContent=!initialized?'Probing not complete':
      (present?'Detected':'Not detected')+
      ' · SPI '+(s.cc1101_spi_healthy?.[i]===true?'healthy':'unavailable')+
      ' · PARTNUM '+hexByte(s.cc1101_partnum?.[i])+
      ' · VERSION '+hexByte(s.cc1101_version?.[i])+
      ' · boot readback '+(s.cc1101_boot_readback_verified?.[i]===true?'passed':'not verified');
  }
  $('info-ip').textContent=s.ipv4||'Not assigned';
  $('info-wifi').textContent=connected?'Connected':'Not connected';
  $('info-mqtt').textContent=s.mqtt_connected?'Connected':'Not connected';
  renderUptimeWithBoot(s);
  $('info-clock').textContent=s.clock_synced?
    (localTimestamp(s.now_utc_ms)||'Synchronizing')+' · NTP':
    'Awaiting NTP (uptime available)';
  $('info-heap').textContent=bytes(s.free_heap_bytes);
  $('diag-memory-uptime').textContent='Uptime '+uptime(s.uptime_s);
  for(const kind of ['internal','psram']){
    const total=s[kind+'_total_bytes'],free=s[kind+'_free_bytes'];
    const available=Number.isFinite(total)&&total>0&&Number.isFinite(free);
    const base='memory-'+kind+'-';
    $(base+'used').textContent=available?
      bytes(Math.max(0,total-free))+' / '+bytes(total)+' ('+
        Math.round((total-free)/total*100)+'% used)':'Not available';
    $(base+'free').textContent=available?bytes(free):'—';
    $(base+'min').textContent=available?bytes(s[kind+'_min_free_bytes']):'—';
    $(base+'largest').textContent=available?bytes(s[kind+'_largest_block_bytes']):'—';
  }
  renderOtaReadiness(s);
  $('settings-wifi').textContent=connected?'Connected':'Not connected';
  const failed=initialized&&ready.some((listening,i)=>s.cc1101_present?.[i]===true&&!listening);
  $('warning-summary').textContent=!initialized?'Radio initialization in progress':
    failed?'At least one detected radio is not listening. Open Diagnostics for detail.':
    'No receiver startup warning reported. RF on-air reception still needs validation.';
  $('warning-summary').classList.toggle('warn',Boolean(failed));
}
async function refreshStatus(){
  if(statusBusy)return;
  statusBusy=true;
  try{renderStatus(await api('api/status'));}
  catch(error){offline(error);}
  finally{statusBusy=false;}
}
function renderRecent(events){
  const target=$('recent-events');target.replaceChildren();
  const rows=events.slice(-6).reverse();
  if(!rows.length){const p=document.createElement('p');p.className='subtle small';
    p.textContent='No decoded RF activity captured yet.';target.append(p);return;}
  for(const e of rows){
    const row=document.createElement('div');row.className='event-row';
    const label=document.createElement('span');
    label.textContent=(radioNames[e.radio]||'RF')+' · '+String(e.kind||'Event');
    const time=document.createElement('small');time.textContent=uptimeMs(e.uptime_ms)+' uptime';
    row.append(label,time);target.append(row);
  }
}
async function refreshEvents(){
  try{const r=await api('api/events');lastEvents=Array.isArray(r.events)?r.events:[];renderRecent(lastEvents);}
  catch{$('recent-events').textContent='Receiver event feed temporarily unavailable.';}
}
// RMT capture counts are NOT the number of decoded/logged pulse trains.
// Expose each stage to troubleshoot "frames increase but no logs" without
// spamming the shared noise ring or touching the receive interrupt.
function renderReceiverEvidence(){
  const s=lastStatus;
  if(!s)return;
  const details=['43342','43392'].map(id=>{
    const prefix='rx'+id+'_';
    if(!Number.isFinite(s[prefix+'frames']))return id+' telemetry unavailable';
    const channel=id==='43342'?'433.42':'433.92';
    return channel+' MHz: '+fmt(s[prefix+'frames'])+' captures, '+
      fmt(s[prefix+'forwarded_frames'])+' forwarded, '+
      fmt(s[prefix+'full_frames'])+' full, '+
      fmt(s[prefix+'queue_drops'])+' completion queue losses, last full '+
      fmt(s[prefix+'last_full_symbols'])+'/'+
      fmt(s[prefix+'capture_capacity'])+' RMT symbols'+
      (Number.isFinite(s[prefix+'last_forwarded_pulses'])?
       ', last forwarded '+fmt(s[prefix+'last_forwarded_pulses'])+' pulses':'');
  });
  const userStates=['Empty','Loaded','Recovered (read-only)','Invalid NVS snapshots','NVS read error'];
  const userCode=Number(s.user_library_storage_state);
  const userInfo='User RX recipes: '+fmt(s.user_decoders_enabled)+
    ' enabled · '+fmt(s.user_rx_matches)+' matches · '+
    fmt(s.user_rx_ambiguous)+' conflicts · storage '+
    (userStates[userCode]||'Not available')+
    '. Matching does not save or authorize a device.';
  $('diag-rx433-summary').textContent=details.join(' · ')+' · '+userInfo;
  const prefix=Number(s.decode_unknown_prefix),unrecognized=Number(s.decode_unknown_payload);
  $('diag-rx868-summary').textContent='868.30 MHz: '+
    fmt(s.rx868_packets)+' fixed-length FIFO reads · '+
    fmt((s.decode_itho||0)+(s.decode_ramses||0))+' decoded protocol messages · '+
    (Number.isFinite(prefix)?fmt(prefix):'—')+' preamble mismatches · '+
    (Number.isFinite(unrecognized)?fmt(unrecognized):'—')+
    ' preamble matched but undecoded · '+fmt(s.rx868_fifo_overflows)+
    ' FIFO overflows. CRC validation is disabled in this profile.';
}
function renderLogCaptureDiagnostics(){
  renderReceiverEvidence();
  const node=$('log-capture-diagnostics');
  const selected=$('log-radio').value;
  const inspect=(logSource==='radio'||logSource==='all')&&
    (selected==='0'||selected==='1');
  node.hidden=!inspect;
  if(!inspect)return;
  const radio=selected==='0'?'43342':'43392';
  const display=selected==='0'?'433.42 MHz':'433.92 MHz';
  const s=lastStatus;
  if(!s || !Number.isFinite(s['rx'+radio+'_forwarded_frames'])){
    node.textContent=display+': RMT capture-to-decoder counters not available on this firmware. '+
      'Install the updated firmware to see why frames were discarded.';
    return;
  }
  const name='rx'+radio+'_';
  const capture=s[name+'frames'],forwarded=s[name+'forwarded_frames'],
    full=s[name+'full_frames'],filtered=s[name+'filtered_frames'],
    short=s[name+'short_frames'];
  const tooShort=Math.max(0,short-full);
  const decoded=s['decode_pulses_'+radio];
  node.textContent=display+': '+fmt(capture)+' RMT captures · '+
    fmt(forwarded)+' forwarded to decoder · '+fmt(decoded)+
    ' recorded raw pulse trains · '+fmt(tooShort)+' too short · '+
    fmt(full)+' incomplete/full-buffer captures (capacity '+
    fmt(s[name+'capture_capacity']||256)+' symbols; last full '+
    fmt(s[name+'last_full_symbols'])+' symbols) · '+
    fmt(s[name+'queue_drops'])+' completion queue losses · '+fmt(filtered)+
    ' discarded after 30 µs filter. '+
    (!$('log-noise').checked?
      'Raw 433 MHz events are classified as noise: enable Include noise to see them.':
      'If no rows appear, the shared noise ring may have overwritten older events; select this frequency and export CSV to check retained records.');
}
function selectedLogQuery(paging=false){
  const p=new URLSearchParams(),radio=$('log-radio').value,noise=$('log-noise').checked;
  if(['system','web'].includes(logSource))p.set('stream',logSource);
  else if(logSource==='radio')p.set('view','radio');
  if(['radio','all'].includes(logSource)&&radio)p.set('radio',radio);
  if(noise&&['all','radio'].includes(logSource))p.set('noise','1');
  if(paging){
    if(logSnapshot)p.set('snapshot',String(logSnapshot));
    if(logPageCursors[logPageCursors.length-1])
      p.set('before',String(logPageCursors[logPageCursors.length-1]));
  }
  return p;
}
function eventIsVisible(event){
  if(logSource==='radio'&&!['noise','signal'].includes(event.stream) &&
    !(event.stream==='system'&&[11,12,13,35,36,37,43,44,45].includes(event.code)))return false;
    if(logSource==='system'&&event.stream!=='system')return false;
  if(logSource==='web'&&event.stream!=='web')return false;
  if(!$('log-noise').checked&&event.stream==='noise')return false;
  return true;
}
// The log ring carries sanitized metadata, not the original RF packet bytes.
// 433 capture summary: 0=forwarded; 1=short; 2=saturated; 3=filtered; 4=queue full.
const rxCaptureReasons=['forwarded to decoder','too short','RMT buffer saturated',
  'discarded by 30 µs pulse filter','RMT completion queue full'];
const expandedLogKeys=new Set();
// Keep the current log snapshot fixed while someone is inspecting expanded
// details. Logging continues on the ESP32; only this browser's live view pauses.
function pauseLogViewing(){
  if(logPageCursors.length!==1||logLivePaused)return;
  logLivePaused=true;
  $('log-latest').disabled=false;
  $('log-latest').textContent='Resume live';
  $('log-page-number').textContent='Page 1 · paused';
  $('log-feedback').textContent=
    'Viewing paused — new events are still recorded. Select Resume live to catch up.';
}
const txTransportNames=['Packet FIFO','RMT','UART'];
const txErrorNames=['None','Invalid radio','No hardware','Busy','Incompatible profile',
  'Missing profile','Unsupported transport','Driver failure','Invalid payload',
  'TX failure','RX restore failure'];
const txStageNames=['None','Unsupported','Snapshot','Modem','GPIO UART','Enter TX',
  'UART write','UART drain','FIFO write','FIFO wait','RMT configuration',
  'RMT send','RMT wait','Restore','RX pause'];
function logRowKey(e){return String(e.stream||'unknown')+'-'+String(e.sequence);}
const webActionNames=['Page opened','Log source changed','Radio filter changed','Noise visibility changed',
  'Log expanded','Log collapsed','CSV export requested','Older logs requested',
  'Newer logs requested','Latest logs requested','Console connected',
  'Console disconnected','Console cleared','Diagnostics refreshed',
  'Logging settings submitted','LED settings submitted','OTA installation confirmed',
  'OTA result reported','Browser API error','Radio logs inspected'];
const readableWebPage=n=>webPages[n-1]||'Unknown page';
const readableWebEndpoint=n=>webEndpoints[n-1]||'Unknown API endpoint';
const readableWebAction=n=>webActionNames[n-1]||'Unknown UI action';
function readableWebActionValue(e){
  // Action IDs and numeric values are defined by the native v0.7 audit
  // endpoint, never by user-supplied strings.
  if(e.code!==39)return '';
  if(e.count===2){
    const names=['All','Radio','Noise (legacy)','System','Web'];
    return names[e.value]?' · '+names[e.value]:'';
  }
  if(e.count===3){
    const frequencies=['All radios','433.42 MHz','433.92 MHz','868.30 MHz'];
    return frequencies[e.value]?' · '+frequencies[e.value]:'';
  }
  if(e.count===4)return e.value===1?' · noise shown':' · noise hidden';
  if(e.count>=8&&e.count<=10)return ' · page '+String(e.value);
  return '';
}
function logDetails(e){
  if(e.code===5)return String(e.count??0)+' RF units · RX recipe matched (not an enrolled device)';
  if(e.code===6)return String(e.count??0)+' RF units · multiple user recipes matched; no device chosen';
  if(e.code===2)return String(e.count??0)+' bytes · '+
    (e.value===1?'Supported Itho/RAMSES preamble absent':
     e.value===2?'Preamble present; both decoders rejected packet':
     'Unknown packet reason (older firmware)');
  if(e.code===35)return String(e.count??0)+' capture(s) '+(rxCaptureReasons[e.value]||'unknown outcome');
  if(e.code===46)return String(e.count??0)+' raw pulse durations · '+
    (e.value===2?'incomplete 512-symbol buffer':e.value===1?'tiny capture':'pre-filter candidate')+
    ' · '+String(e.profile??'?')+' RMT symbols · expand to inspect/export';
  if(e.code===36)return 'PARTNUM '+hexByte((Number(e.count)>>8)&255)+
    ' · VERSION '+hexByte(Number(e.count)&255)+
    ' · boot readback '+((Number(e.flags)&1)?'passed':'not verified')+
    ' · result '+(e.value===0?'OK':String(e.value??'unknown'));
  if(e.code===37)return 'Three consecutive SPI identity checks failed; radio marked offline';
  if(e.code===38)return 'Opened '+readableWebPage(e.profile);
  if(e.code===39 || e.code===42)return readableWebAction(e.count)+
    readableWebActionValue(e)+' · '+readableWebPage(e.profile)+
    (e.code===42?' · HTTP '+e.value:'');
  if(e.code===40 || e.code===41)return ((Number(e.flags)&1)?'POST ':'GET ')+
    readableWebEndpoint(e.count)+
    ' · HTTP '+e.value;
  if(e.code===43)return 'RX started successfully';
  if(e.code===44)return 'RX stopped for recovery';
  if(e.code===45)return 'RX start failed · driver status '+e.value;
  const parts=[];
  if(e.code>=1&&e.code<=4 && Number.isFinite(e.count))
    parts.push(e.count+(e.code===1?' pulses':' bytes'));
  else if(e.count)parts.push(e.count+' items');
  if(e.code===4 && Number.isInteger(e.value)&&e.value>0)
    parts.push('RAMSES command 0x'+e.value.toString(16).toUpperCase().padStart(4,'0'));
  if(e.correlation_id)parts.push(e.code>=1&&e.code<=6?
    'RX capture #'+e.correlation_id:'TX #'+e.correlation_id);
  if(e.code>=30&&e.code<=34){
    if(e.profile!==undefined)parts.push('profile '+e.profile);
    if(e.tx_driver_attempted)parts.push('driver '+(e.tx_driver_accepted?'accepted (RF unverified)':'failed'));
    if(e.tx_rx_previous)parts.push('RX '+(e.tx_rx_restored?'restored':'NOT restored'));
  }else if(e.value && e.code!==4)parts.push('status '+e.value);
  return parts.length?parts.join(' · '):'—';
}
function logEventTitle(e){
  if(e.code===38)return 'Page opened · '+readableWebPage(e.profile);
  if(e.code===39 || e.code===42)return readableWebAction(e.count);
  if(e.code===40)return 'API failed · '+readableWebEndpoint(e.count);
  if(e.code===41)return 'API updated · '+readableWebEndpoint(e.count);
  return String(e.kind||'Unknown event');
}
// Keep monotonic uptime primary: the NTP boot timestamp is an optional
// human-readable second line, never a substitute for device uptime.
function renderUptimeWithBoot(status){
  const node=$('info-uptime');
  node.replaceChildren(document.createTextNode(uptime(status.uptime_s)));
  const boot=Number.isInteger(status.boot_id)&&status.boot_id>0?
    ' · boot #'+status.boot_id:'';
  const bootTime=status.clock_synced===true?
    localTimestamp(status.boot_started_utc_ms):null;
  const hint=document.createElement('small');
  hint.className='uptime-boot-hint';
  hint.textContent=(bootTime?'Started '+bootTime+' (local)':
    'Start time unknown · awaiting NTP')+boot;
  node.append(hint);
}
function localTimestamp(epochMs){
  const value=Number(epochMs);
  if(!Number.isFinite(value)||value<1704067200000||value>=4102444800000)return null;
  return new Date(value).toLocaleString('nl-NL',{
    year:'numeric',month:'2-digit',day:'2-digit',hour:'2-digit',
    minute:'2-digit',second:'2-digit',hour12:false
  })+','+String(Math.floor(value%1000)).padStart(3,'0');
}
function logMetadataFields(e){
  const code=Number(e.code);
  const isRx=code>=1&&code<=6, isTx=code>=30&&code<=34;
  const severity=['Info','Warning','Error'][e.severity]||'Unknown';
  const fields=[
    ['Date/time',localTimestamp(e.timestamp_utc_ms)||'Unknown · awaiting SNTP'],
    ['Uptime',uptimeMs(e.uptime_ms)+' (raw '+String(e.uptime_ms??'—')+' ms)'],
    ['Sequence','#'+String(e.sequence??'—')],
    ['Source',String(e.stream||'—')],['Severity',severity],
    ['Event',String(e.kind||'Unknown')+' (code '+String(e.code??'—')+')']
  ];
  if(Number.isInteger(e.radio)&&e.radio>=0&&e.radio<radioNames.length)
    fields.push(['Radio',radioNames[e.radio]]);
  if(isRx){
    fields.push([(code===1 || (code>=5&&code<=6&&Number(e.radio)<2))?'Pulse count':'Packet length',
      String(e.count??'—')+((code===1 || (code>=5&&code<=6&&Number(e.radio)<2))?' pulses':' bytes')]);
    if(code===4){
      fields.push(['Protocol','RAMSES-II']);
      fields.push(['Command',Number.isInteger(e.value)&&e.value>0?
        '0x'+e.value.toString(16).toUpperCase().padStart(4,'0')+' ('+e.value+')':
        'Not decoded']);
    }else if(code===5||code===6){
      fields.push(['Recognition',code===5?'A user RX recipe matched':'Multiple user RX recipes matched']);
      fields.push(['Device status','Not enrolled; RX only; no RF TX authorization']);
      fields.push(['Decision',code===6?'Ambiguous: no device selected':'Recipe accepted; no unique device claim']);
    }else if(code===3){
      fields.push(['Protocol','Itho']);
      fields.push(['Command','Not recorded in the native event ring']);
    }else{
      fields.push(['Recognition',code===1?'Raw pulse input':'Unknown packet']);
      if(code===2){
        fields.push(['Why unknown',e.value===1?'Supported preamble absent':
          e.value===2?'Preamble present, neither decoder accepted':
          'Reason unavailable in older firmware']);
        fields.push(['CRC / LQI','Not validated by current 868 fixed-length modem profile']);
      }
    }
    fields.push(['Decoder overlap',(Number(e.flags)&1)?'Yes':'No']);
    fields.push(['Raw RF capture',e.correlation_id?
      'Linked capture #'+e.correlation_id+'; expand this row for the volatile GDO2 timings or FIFO bytes':
      'No linked raw capture in this firmware']);
    fields.push(['Retention','Existing bounded radio PSRAM detail buffer; fallback keeps 8 samples if detail storage is unavailable']);
  }else if(isTx){
    fields.push(['TX correlation ID',e.correlation_id?'#'+e.correlation_id:'Not assigned']);
    fields.push(['Profile',String(e.profile??'—')]);
    fields.push(['Transport',txTransportNames[e.tx_transport]||'Unknown / not selected']);
    fields.push(['TX units',String(e.count??'—')]);
    fields.push(['Repeats',String(e.tx_repeats??'—')]);
    fields.push(['Failure stage',txStageNames[e.tx_stage]||'Unknown']);
    fields.push(['Driver status',e.tx_driver_attempted?
      (e.tx_driver_accepted?'Accepted by driver (not RF verification)':'Rejected or failed'):
      'Not attempted']);
    fields.push(['RX before TX',e.tx_rx_previous?'Active':'Inactive']);
    fields.push(['RX restoration',e.tx_rx_restored?'Restored':'Not reported restored']);
    fields.push(['Hardware restoration',e.tx_hardware_restored?'Restored':'Not reported restored']);
    fields.push(['Result code',txErrorNames[e.value]||String(e.value??'—')]);
    fields.push(['RF on-air confirmation','Not available from the driver trace']);
  }else if(code===36){
    fields.push(['PARTNUM',hexByte((Number(e.count)>>8)&255)]);
    fields.push(['VERSION',hexByte(Number(e.count)&255)]);
    fields.push(['Boot-only CHANNR readback',(Number(e.flags)&1)?'Passed':'Not verified (or not performed)']);
    fields.push(['SPI result code',String(e.value??'—')]);
    fields.push(['Meaning','An identity response is not proof of RF reception']);
  }else if(code===37){
    fields.push(['Cause','Three consecutive read-only SPI identity checks failed']);
    fields.push(['Action','RX capture stopped; hardware unavailable; automatic re-probe scheduled']);
  }else if(code===38||code===39||code===42){
    fields.push(['Browser page',readableWebPage(e.profile)]);
    fields.push(['User action',readableWebAction(e.count)]);
    if(e.value)fields.push(['Action value / HTTP status',String(e.value)]);
    if(code===42)fields.push(['API',readableWebEndpoint(e.tx_transport)]);
    fields.push(['Privacy','Only predefined action IDs are stored, never inputs or passwords']);
  }else if(code===40||code===41){
    fields.push(['HTTP method',(Number(e.flags)&1)?'POST':'GET']);
    fields.push(['API endpoint',readableWebEndpoint(e.count)]);
    fields.push(['HTTP status',String(e.value)]);
    fields.push(['Privacy','Request content, URL query, headers and secrets are not logged']);
  }else if(code===43||code===44||code===45){
    fields.push(['Receiver action',code===43?'Started':code===44?'Stopped':'Driver failed']);
    fields.push(['Driver error',String(e.value??'—')]);
  }else if(code===46){
    fields.push(['Capture phase','Before the 30µs software filter; exact sampled GDO2 levels']);
    fields.push(['RMT symbols',String(e.profile??'—')]);
    fields.push(['Duration count',String(e.count??'—')]);
    fields.push(['Capture outcome',e.value===2?'Full/incomplete; cannot decode as a whole frame':
      e.value===1?'Too short':'Complete RMT candidate (may still be filtered later)']);
    fields.push(['Retention','Stored in the existing 433 MHz-specific PSRAM detail arena']);
  }else if(code===35){
    fields.push(['Outcome',rxCaptureReasons[e.value]||'Unknown']);
    fields.push(['Occurrences in the previous ~30 s',String(e.count??0)]);
    if(Number(e.value)===2){
      fields.push(['Meaning','Capture reached the RMT host buffer limit and was discarded as incomplete. Could be a long signal or continuous RF noise.']);
      const id=Number(e.radio)===0?'43342':'43392';
      fields.push(['Current buffer limit',String(lastStatus?.['rx'+id+'_capture_capacity']??'Unknown')+' RMT symbols (current status, not event-time data)']);
    }
    if(Number(e.value)===4)fields.push(['Meaning','The RX callback could not queue a completion. No complete pulse frame was delivered.']);
    fields.push(['Data source','RMT capture-stage counters, not a decoded RF payload']);
  }else if(e.value!==undefined){
    fields.push(['Status / error code',String(e.value)]);
  }
  return fields;
}
// Radio details use isolated, bounded per-category PSRAM FIFO stores.
// Cache on-demand snapshots for this browser page only; never send RF
// packet bytes, rolling codes or GDO2 timings to UI-audit/System/Web logging.
const rawCaptureRequests=new Map();
async function populateRawCapture(node){
  const radio=Number(node.dataset.rawRadio),seq=Number(node.dataset.rawSeq);
  if(!Number.isSafeInteger(radio)||radio<0||radio>2||
     !Number.isSafeInteger(seq)||seq<1)return;
  const pre=node.dataset.rawPre==='1';
  const key=radio+':'+seq+':'+(pre?'pre':'decoded');
  if(!rawCaptureRequests.has(key)){
    if(rawCaptureRequests.size>40)rawCaptureRequests.clear();
    rawCaptureRequests.set(key,api('api/radio/capture?radio='+radio+'&seq='+seq+
      (pre?'&pre=1':''))
      .catch(error=>({available:false,reason:error.message})));
  }
  const frame=await rawCaptureRequests.get(key);
  if(!node.isConnected)return;
  if(!frame.available){
    node.textContent='Raw capture unavailable ('+String(frame.reason||'overwritten')+
      '). Details are retained in the isolated PSRAM FIFO until overwritten or reboot.';
    return;
  }
  const hash=Number.isInteger(frame.hash)?
    ' · RF hash '+frame.hash.toString(16).toUpperCase().padStart(8,'0'):'';
  if(frame.format==='signed_pulses_us'&&Array.isArray(frame.pulses_us)){
    const highs=frame.pulses_us.filter(x=>x>0);
    const lows=frame.pulses_us.filter(x=>x<0).map(x=>-x);
    const periods=[];
    for(let i=0;i+1<frame.pulses_us.length;++i)
      if(frame.pulses_us[i]>0&&frame.pulses_us[i+1]<0)
        periods.push(frame.pulses_us[i]-frame.pulses_us[i+1]);
    const mostCommon=values=>{
      const histogram=new Map();
      for(const width of values) {
        const bucket=Math.round(width/50)*50;
        histogram.set(bucket,(histogram.get(bucket)||0)+1);
      }
      return [...histogram].sort((a,b)=>b[1]-a[1]).slice(0,6)
        .map(([width,count])=>width+'µs ×'+count).join(' · ')||'none';
    };
    node.textContent=(pre?'GDO2 PRE-FILTER capture':'GDO2 decoded-input capture')+
      ' · '+frame.pulses_us.length+' signed pulse durations · '+frame.frequency_hz+hash+
      ' Hz\nHigh: '+highs.length+' · Low: '+lows.length+
      '\nPulse widths (high; 50µs bins): '+mostCommon(highs)+
      '\nGap widths (low; 50µs bins): '+mostCommon(lows)+
      '\nPulse periods (high + next gap): '+mostCommon(periods)+
      '\nExact signed pulse durations (µs; positive=high, negative=low):\n'+
      frame.pulses_us.join(', ')+
      (pre?'\nPre-filter physical capture; may be noise or an incomplete RF frame.':
        '\nPost-filter decoder input, possibly including synthetic 5ms idle tail.')+
      '\nThese are GDO2 timings, NOT decoded bytes; no protocol claimed.';
  }else if(frame.format==='fifo_bytes_hex'&&typeof frame.hex==='string'){
    node.textContent='CC1101 FIFO capture · '+frame.count+' bytes · '+frame.frequency_hz+hash+
      ' Hz\n'+frame.hex+
      (Number.isFinite(frame.rssi_dbm)&&Number.isFinite(frame.lqi)&&
       Number.isFinite(frame.frequency_offset_hz)?
        '\nRSSI '+frame.rssi_dbm.toFixed(1)+' dBm · LQI '+frame.lqi+
        ' (lower is better) · FREQEST '+frame.frequency_offset_hz+' Hz':
        '\nRSSI / LQI / frequency offset unavailable for this capture')+
      '\nBytes are unverified RF input; CRC is disabled and packet length is not protocol proof.';
  }else node.textContent='Raw capture format not recognized on this firmware.';
}
// System/Web/TX use the same on-demand detail mechanism without fetching RF
// contents, HTTP bodies, secrets or a complete log history.
const diagnosticDetailRequests=new Map();
async function populateDiagnosticDetail(node){
  const bucket=Number(node.dataset.detailBucket);
  const radio=Number(node.dataset.detailRadio);
  const seq=Number(node.dataset.detailSeq);
  if(!Number.isSafeInteger(bucket)||bucket<0||bucket>5||
     !Number.isSafeInteger(radio)||radio<0||radio>255||
     !Number.isSafeInteger(seq)||seq<1)return;
  const key=bucket+':'+radio+':'+seq;
  if(!diagnosticDetailRequests.has(key)){
    if(diagnosticDetailRequests.size>50)diagnosticDetailRequests.clear();
    diagnosticDetailRequests.set(key,
      fetch('api/logs/detail?bucket='+bucket+'&radio='+radio+'&seq='+seq,
            {cache:'no-store'})
        .then(async response=>{
          if(!response.ok)throw new Error('HTTP '+response.status);
          return response.text();
        }).catch(error=>'Additional detail unavailable: '+error.message));
  }
  const text=await diagnosticDetailRequests.get(key);
  if(node.isConnected)node.textContent=text;
}
function renderLogs(events){
  const tbody=$('log-rows');
  const scroller=tbody.closest('.table-wrap');
  const savedScroll=scroller?.scrollTop||0;
  const focusedKey=document.activeElement?.dataset?.logToggle;
  const displayed=events.filter(eventIsVisible).slice().reverse();
  const visibleKeys=new Set(displayed.map(logRowKey));
  for(const key of expandedLogKeys)if(!visibleKeys.has(key))expandedLogKeys.delete(key);
  tbody.replaceChildren();
  $('log-feedback').textContent=displayed.length+
    ' retained events on this page · select Older to read the full buffer · click an event to expand';
  renderLogCaptureDiagnostics();
  if(!displayed.length){
    const tr=document.createElement('tr'),td=document.createElement('td');td.colSpan=5;
    td.textContent='No retained events match these filters.';
    tr.append(td);tbody.append(tr);return;
  }
  for(const e of displayed){
    const key=logRowKey(e);
    const open=expandedLogKeys.has(key);
    const detailId='log-detail-'+key.replace(/[^a-zA-Z0-9_-]/g,'_');
    const tr=document.createElement('tr');tr.className='log-main-row';
    if(open)tr.classList.add('is-expanded');
    tr.dataset.logKey=key;
    if(e.severity===2)tr.classList.add('is-error');
    else if(e.severity===1)tr.classList.add('is-warning');
    const values=[localTimestamp(e.timestamp_utc_ms)||'Time not synchronized',e.stream||'—',
      radioNames[e.radio]||(e.stream==='web'?'Web UI / API':'System'),
      logEventTitle(e),logDetails(e)];
    for(let i=0;i<values.length;i++){
      const td=document.createElement('td');
      if(i===3){
        const button=document.createElement('button');
        button.type='button';button.className='log-expand-button';
        button.dataset.logToggle=key;
        button.setAttribute('aria-expanded',String(open));
        button.setAttribute('aria-controls',detailId);
        button.setAttribute('aria-label',(open?'Collapse':'Expand')+' details: '+values[3]);
        const arrow=document.createElement('span');arrow.className='log-expand-chevron';
        arrow.setAttribute('aria-hidden','true');arrow.textContent=open?'▾':'▸';
        const title=document.createElement('span');title.textContent=values[3];
        button.append(arrow,title);td.append(button);
      }else if(i===0){
        td.textContent=values[i];
        const elapsed=document.createElement('small');
        elapsed.className='log-time-uptime';
        elapsed.textContent='Uptime '+uptimeMs(e.uptime_ms);
        td.append(elapsed);
      }else td.textContent=values[i];
      if(i===4)td.className='detail';
      tr.append(td);
    }
    const detailRow=document.createElement('tr');
    detailRow.className='log-expanded-row';detailRow.id=detailId;
    detailRow.hidden=!open;detailRow.dataset.logDetail=key;
    const cell=document.createElement('td');cell.colSpan=5;
    const panel=document.createElement('div');panel.className='log-expanded-panel';
    const heading=document.createElement('strong');
    heading.textContent='Event details · '+String(e.kind||'Unknown');
    const grid=document.createElement('dl');grid.className='log-metadata-grid';
    for(const [label,value] of logMetadataFields(e)){
      const dt=document.createElement('dt');dt.textContent=label;
      const dd=document.createElement('dd');dd.textContent=String(value);
      grid.append(dt,dd);
    }
    panel.append(heading,grid);
    let raw=null, diagnostic=null;
    if(((e.code>=1&&e.code<=4)||e.code===46)&&
       Number.isInteger(e.radio)&&e.radio>=0&&e.radio<3&&
       Number.isInteger(e.code===46?e.sequence:e.correlation_id)&&
       (e.code===46?e.sequence:e.correlation_id)>0){
      raw=document.createElement('pre');
      raw.className='log-rf-raw';
      raw.dataset.rawRadio=String(e.radio);
      raw.dataset.rawSeq=String(e.code===46?e.sequence:e.correlation_id);
      raw.dataset.rawPre=e.code===46?'1':'0';
      raw.textContent='Loading bounded raw RF capture…';
      panel.append(raw);
      const exportButton=document.createElement('button');
      exportButton.type='button';
      exportButton.className='secondary';
      exportButton.dataset.exportRfSeq=raw.dataset.rawSeq;
      exportButton.dataset.exportRfRadio=raw.dataset.rawRadio;
      exportButton.dataset.exportRfPre=raw.dataset.rawPre;
      exportButton.textContent='Export RF capture JSON';
      exportButton.title='Download exact pulses/FIFO bytes for offline protocol analysis';
      panel.append(exportButton);
    }else{
      const bucket=e.stream==='system'?4:e.stream==='web'?5:
        (Number.isInteger(e.radio)&&e.radio>=0&&e.radio<3?e.radio:-1);
      if(bucket>=0 && Number.isSafeInteger(e.sequence)&&e.sequence>0){
        diagnostic=document.createElement('pre');
        diagnostic.className='log-rf-raw';
        diagnostic.dataset.detailBucket=String(bucket);
        diagnostic.dataset.detailRadio=String(bucket>=4?255:e.radio);
        diagnostic.dataset.detailSeq=String(e.sequence);
        diagnostic.textContent='Loading additional diagnostic details…';
        panel.append(diagnostic);
      }
    }
    cell.append(panel);detailRow.append(cell);
    tbody.append(tr,detailRow);
    if(open&&raw)void populateRawCapture(raw);
    if(open&&diagnostic)void populateDiagnosticDetail(diagnostic);
  }
  if(focusedKey){
    const button=[...tbody.querySelectorAll('button[data-log-toggle]')]
      .find(b=>b.dataset.logToggle===focusedKey);
    if(button)button.focus({preventScroll:true});
  }
  if(scroller)scroller.scrollTop=savedScroll;
}
$('log-rows').addEventListener('click',event=>{
  const row=event.target.closest('tr[data-log-key]');
  if(!row)return;
  const button=row.querySelector('button[data-log-toggle]');
  const detail=row.nextElementSibling;
  if(!button||!detail||detail.dataset.logDetail!==row.dataset.logKey)return;
  const nowOpen=detail.hidden;
  detail.hidden=!nowOpen;
  button.setAttribute('aria-expanded',String(nowOpen));
  button.setAttribute('aria-label',(nowOpen?'Collapse':'Expand')+
    ' details: '+button.querySelector('span:last-child').textContent);
  button.querySelector('.log-expand-chevron').textContent=nowOpen?'▾':'▸';
  row.classList.toggle('is-expanded',nowOpen);
  if(nowOpen){
    expandedLogKeys.add(row.dataset.logKey);
    pauseLogViewing();
    const raw=detail.querySelector('[data-raw-seq]');
    if(raw)void populateRawCapture(raw);
    const diagnostic=detail.querySelector('[data-detail-seq]');
    if(diagnostic)void populateDiagnosticDetail(diagnostic);
  }else expandedLogKeys.delete(row.dataset.logKey);
  auditUi(nowOpen?'log_expand':'log_collapse');
});

// Export is an explicit browser-only action inside the existing Logs panel.
// RF IDs and rolling counters stay out of the regular metadata CSV and UI audit.
$('log-rows').addEventListener('click',async event=>{
  const button=event.target.closest('button[data-export-rf-seq]');
  if(!button || button.disabled)return;
  const radio=Number(button.dataset.exportRfRadio);
  const seq=Number(button.dataset.exportRfSeq);
  const pre=button.dataset.exportRfPre==='1';
  if(!Number.isSafeInteger(seq)||seq<1||!Number.isInteger(radio)||
     radio<0||radio>2)return;
  button.disabled=true;button.textContent='Preparing RF JSON…';
  try{
    const raw=await api('api/radio/capture?radio='+radio+'&seq='+seq+
      (pre?'&pre=1':''));
    if(!raw.available)throw new Error('Capture overwritten or unavailable');
    const evidence={schema:'rflink-rf-capture-v1',type:pre?'pre_filter_rmt':'decoder_input',
      firmware:lastStatus?.firmware??'unknown',radio,radio_label:radioNames[radio],
      exported_at_utc:new Date().toISOString(),capture:raw};
    const blob=new Blob([JSON.stringify(evidence,null,2)+'\n'],{type:'application/json'});
    const url=URL.createObjectURL(blob),link=document.createElement('a');
    link.href=url;link.download='RFLink_RF_'+radio+'_'+seq+(pre?'_pre':'')+'.json';
    document.body.append(link);link.click();link.remove();
    setTimeout(()=>URL.revokeObjectURL(url),1500);
    button.textContent='Export RF capture JSON';
  }catch(error){button.textContent='Export failed: '+error.message;}
  finally{button.disabled=false;}
});
async function refreshLogBufferSummary(){
  try{
    const r=await api('api/logs/config');
    const display=$('log-buffer-summary'),totals=$('log-buffer-totals');
    if(!r.ready){
      display.textContent='Log storage is not ready';
      totals.textContent='Not initialized';
      return;
    }
    const names=['433.42 MHz','433.92 MHz','868.30 MHz','Noise','System','Web'];
    const limits=[...(r.signal||[]),r.noise,512,512];
    const fragment=document.createDocumentFragment();
    let usedTotal=0,overwrittenTotal=0;
    for(let i=0;i<names.length;i++){
      const used=Number(r.used?.[i])||0,overwritten=Number(r.overwritten?.[i])||0;
      usedTotal+=used;overwrittenTotal+=overwritten;
      const tile=document.createElement('div');tile.className='log-buffer-item';
      const name=document.createElement('strong');name.textContent=names[i];
      const capacity=document.createElement('span');
      const detailUsed=Number(r.details?.used?.[i])||0;
      const detailCap=Number(r.details?.capacity?.[i])||0;
      capacity.textContent=fmt(used)+' / '+fmt(limits[i])+
        (r.details?.ready?' · details '+(detailUsed/1024).toFixed(0)+
         ' / '+(detailCap/1024).toFixed(0)+' KiB':'');
      const dropped=document.createElement('small');
      const detailEvicted=Number(r.details?.evicted?.[i])||0;
      dropped.textContent=(overwritten?fmt(overwritten)+' overwritten':'No overwritten entries')+
        (r.details?.ready?' · '+fmt(detailEvicted)+' detail FIFO evictions':'');
      if(overwritten)dropped.classList.add('has-overwritten');
      tile.append(name,capacity,dropped);fragment.append(tile);
    }
    display.replaceChildren(fragment);
    totals.textContent=fmt(usedTotal)+' retained · '+fmt(overwrittenTotal)+' overwritten';
  }catch(error){$('log-buffer-summary').textContent='Buffer statistics unavailable: '+error.message;}
}
function syncLogFilterControls(){
  // System and Web never use the radio/noise filters; keep their previous
  // values so returning to Radio does not lose the user's selection.
  $('log-filter-controls').hidden=logSource==='system'||logSource==='web';
}
if(typeof window.matchMedia==='function' && window.matchMedia('(max-width:750px)').matches)
  $('log-buffer-details').open=false;
syncLogFilterControls();
async function refreshLogs(jumpToTop=false){
  if(logsBusy)return;
  logsBusy=true;
  // A reader freezes the current snapshot, like manual history paging.
  // Never clear it while a row is expanded or the live view is paused.
  if(logPageCursors.length===1&&!logLivePaused)logSnapshot=0;
  const pausedWhenRequested=logLivePaused;
  const query=selectedLogQuery(true);
  // Export uses every ring irrespective of the visible filter/query.
  $('log-export').href='api/logs.csv';
  try{
    const r=await api('api/logs'+(query.size?'?'+query.toString():''));
    // An already-running live request may return after a user expands a row.
    // Discard it rather than replacing the row they are reading.
    if(logLivePaused&&!pausedWhenRequested&&logPageCursors.length===1)return;
    logSnapshot=Number.isSafeInteger(r.snapshot)?r.snapshot:logSnapshot;
    logHasOlder=r.has_more===true;
    renderLogs(Array.isArray(r.events)?r.events:[]);
    if(jumpToTop){
      const scroller=$('log-rows').closest('.table-wrap');
      if(scroller)scroller.scrollTop=0;
    }
    const page=logPageCursors.length;
    $('log-page-number').textContent='Page '+page+
      (logLivePaused?' · paused':' · up to 64 entries');
    if(logLivePaused)$('log-feedback').textContent=
      'Viewing paused — new events are still recorded. Select Resume live to catch up.';
    $('log-older').disabled=!logHasOlder||!r.cursor;
    $('log-newer').disabled=page===1;
    $('log-latest').disabled=page===1&&!logLivePaused;
    $('log-latest').textContent=logLivePaused?'Resume live':'Latest events';
    $('log-older').dataset.cursor=String(r.cursor||'');
    if(page===1)refreshLogBufferSummary();
  }catch(error){$('log-feedback').textContent='Log feed unavailable: '+error.message;}
  finally{logsBusy=false;}
}
for(const button of document.querySelectorAll('[data-log-source]'))button.addEventListener('click',()=>{
  logSource=button.dataset.logSource;
  resetLogPages();
  syncLogFilterControls();
  // Preserve numeric audit source IDs from v0.7 (2 was the removed Noise tab).
  auditUi('log_source',['all','radio','noise','system','web'].indexOf(logSource));
  for(const tab of document.querySelectorAll('[data-log-source]')){
    const active=tab===button;tab.classList.toggle('selected',active);
    tab.setAttribute('aria-pressed',String(active));
  }
  refreshLogs();
});
$('log-radio').addEventListener('change',()=>{
  resetLogPages();
  auditUi('log_radio',Number($('log-radio').value)+1||0);
  renderLogCaptureDiagnostics();refreshLogs();
});
$('log-noise').addEventListener('change',()=>{
  resetLogPages();
  auditUi('log_noise',$('log-noise').checked?1:0);
  renderLogCaptureDiagnostics();refreshLogs();
});
$('log-export').addEventListener('click',()=>auditUi('log_export'));
$('log-older').addEventListener('click',()=>{
  const cursor=Number($('log-older').dataset.cursor);
  if(!logHasOlder||!Number.isSafeInteger(cursor)||cursor<1)return;
  logPageCursors.push(cursor);
  auditUi('log_older',logPageCursors.length);
  refreshLogs();
});
$('log-newer').addEventListener('click',()=>{
  if(logPageCursors.length===1)return;
  logPageCursors.pop();
  auditUi('log_newer',logPageCursors.length);
  if(logPageCursors.length===1&&!logLivePaused)logSnapshot=0;
  refreshLogs();
});
$('log-latest').addEventListener('click',()=>{
  resetLogPages();
  auditUi('log_latest');
  refreshLogs(true);
});
for(const button of document.querySelectorAll('[data-radio-log]')){
  button.addEventListener('click',()=>{
    logSource='radio';
    resetLogPages();
    syncLogFilterControls();
    auditUi('radio_inspect',Number(button.dataset.radioLog)+1);
    for(const tab of document.querySelectorAll('[data-log-source]')){
      const active=tab.dataset.logSource==='radio';
      tab.classList.toggle('selected',active);
      tab.setAttribute('aria-pressed',String(active));
    }
    $('log-radio').value=button.dataset.radioLog;
    $('log-noise').checked=true;
    showPage('logs');
  });
}
function renderHistory(report){
  const ota=report.ota_health;
  $('diag-ota-state').textContent=ota?'Saved in NVS':'No saved OTA health';
  $('diag-ota-version').textContent=ota?.firmware||'No report';
  $('diag-ota-boot').textContent=ota?.boot?'#'+ota.boot:'—';
  const outcomes={valid:'Validated (ESP-IDF marked valid)',
    failed:'Boot health failed (rollback expected)',
    checking:'Validation started; final result not recorded',
    validation_error:'Could not mark firmware valid'};
  $('diag-ota-outcome').textContent=ota?(outcomes[ota.outcome]||ota.outcome):'Not recorded';
  $('diag-ota-receivers').textContent=ota?
    fmt(ota.active_receivers)+' active · '+fmt(ota.responsive_radios)+
      ' SPI responsive · worker '+(ota.worker_ok?'OK':'not ready'):'Not recorded';
  $('diag-reset').textContent=report.last_reset||'Unknown';
  markStatus($('diag-reset-badge'),'Saved on device',report.history_saved?'online':'warn');
  const dump=report.core_dump||{};
  $('diag-core').textContent=dump.integrity==='valid'?'Valid':
    dump.integrity==='invalid_or_unreadable'?'Unreadable':'Not available';
  $('diag-crash-pc').textContent=dump.summary_available?
    (dump.task||'Unknown task')+' · '+(dump.pc||'PC unknown'):'No usable crash PC';
  $('diag-compare').textContent=dump.build_comparison||'Unknown';
  const phases=new Map((report.boot_phases||[]).map(p=>[p.boot,p]));
  const list=$('diag-boots');list.replaceChildren();
  if(!Array.isArray(report.history)||!report.history.length){
    list.textContent='No persisted restarts available.';return;
  }
  for(const item of report.history.slice(0,12)){
    const row=document.createElement('div');row.className='boot-item';
    const reason=document.createElement('strong');reason.textContent='#'+item.boot+' · '+(item.reason||'Unknown');
    const stage=document.createElement('span');
    const phase=phases.get(item.boot);
    const checkpoint=phase?(String(phase.stage||'unknown').replaceAll('_',' ')+
      (phase.startup_complete?' · ready':' · last reached')):'Checkpoint unavailable';
    const bootTime=localTimestamp(item.boot_started_utc_ms);
    stage.textContent=(bootTime?'Started '+bootTime+' · ':'Date unknown · ')+checkpoint;
    row.append(reason,stage);list.append(row);
  }
}
async function refreshHistory(){
  if(historyBusy)return;historyBusy=true;
  try{renderHistory(await api('api/diagnostics/crashes'));}
  catch(error){$('diag-boots').textContent='Boot history unavailable: '+error.message;}
  finally{historyBusy=false;}
}
$('diag-refresh').addEventListener('click',()=>{auditUi('diagnostics_refresh');refreshHistory();});
function disconnectConsole(){
  consoleConnected=false;consoleCursor=0;consoleBootId=null;
  $('console-connect').disabled=false;$('console-disconnect').disabled=true;
  markStatus($('console-state'),'Disconnected');
}
$('console-disconnect').addEventListener('click',()=>{auditUi('console_disconnect');disconnectConsole();});
$('console-clear').addEventListener('click',()=>{
  auditUi('console_clear');
  $('console-output').textContent='';
});
$('console-connect').addEventListener('click',()=>{
  auditUi('console_connect');
  // The captured console is read-only and already filtered before sharing.
  consoleConnected=true;consoleCursor=0;consoleBootId=null;
  $('console-connect').disabled=true;$('console-disconnect').disabled=false;
  markStatus($('console-state'),'Connecting','warn');pollConsole();
});
async function pollConsole(){
  if(!consoleConnected||consoleBusy||currentPage!=='diagnostics')return;
  consoleBusy=true;
  try{
    let r=await api('api/diagnostics/console?since='+consoleCursor);
    if(consoleBootId!==null&&r.boot_id!==consoleBootId){
      $('console-output').textContent+='\n— ESP32 rebooted: new application console —\n';
      consoleCursor=0;
      r=await api('api/diagnostics/console?since=0');
      refreshHistory();
    }
    consoleBootId=r.boot_id;
    if(r.truncated)$('console-output').textContent+='\n— Earlier console lines overwritten in RAM —\n';
    if(typeof r.text==='string'&&r.text.length){
      const el=$('console-output');
      el.textContent=(el.textContent+r.text).slice(-100000);
      el.scrollTop=el.scrollHeight;
    }
    if(Number.isSafeInteger(r.cursor))consoleCursor=r.cursor;
    markStatus($('console-state'),'Live','online');
  }catch(error){
    markStatus($('console-state'),'Unavailable','warn');
    const text='\n— Live console unavailable: '+error.message+' —\n';
    const out=$('console-output');
    if(!out.textContent.endsWith(text))out.textContent+=text;
    if(/403|404/.test(error.message))disconnectConsole();
  }finally{consoleBusy=false;}
}
async function refreshSettings(){
  try{
    const r=await api('api/logs/config');
    if(!r.ready){$('log-budget').textContent='Log storage is not initialized yet.';return;}
    for(let i=0;i<3;i++)$('logs-settings-form').elements['signal_'+i].value=r.signal[i];
    $('logs-settings-form').elements.noise.value=r.noise;
    $('log-budget').textContent=Math.round(r.allocated_bytes/1024)+' KB allocated of '+
      Math.round(r.budget_bytes/1024)+' KB · '+
      (r.overwritten||[]).reduce((a,b)=>a+b,0)+' overwritten events';
  }catch(error){$('log-budget').textContent='Log settings unavailable: '+error.message;}
  try{
    const mqtt=await api('api/mqtt/status');
    $('settings-mqtt').textContent=mqtt.configured?(mqtt.connected?'Connected':'Not connected'):'Not configured';
  }catch{$('settings-mqtt').textContent='Unavailable';}
  try{
    const led=await api('api/led');
    $('led-enabled').checked=led.enabled===true;
    $('led-brightness').value=Number.isInteger(led.brightness)?led.brightness:20;
    $('led-mode').textContent=String(led.mode||'unknown').replaceAll('_',' ');
    $('led-driver').textContent=led.driver_ready?'Ready on GPIO'+led.gpio:
      'Unavailable (radio receivers have priority)';
  }catch(error){
    $('led-mode').textContent='Unavailable';
    $('led-driver').textContent='Status unavailable: '+error.message;
  }
}
$('led-settings-form').addEventListener('submit',async e=>{
  e.preventDefault();
  auditUi('led_submit');
  const status=$('led-save-status');
  const brightness=Number($('led-brightness').value);
  if(!Number.isSafeInteger(brightness)||brightness<1||brightness>100){
    status.textContent='Choose brightness between 1 and 100%.';return;
  }
  status.textContent='Saving LED settings…';
  try{
    const result=await api('api/led',{method:'POST',headers:{
      'Content-Type':'application/json'
    },body:JSON.stringify({enabled:$('led-enabled').checked,brightness})});
    status.textContent=result.saved===true?
      'Saved to RFLink. The LED updates without reboot and survives firmware OTA.':
      'LED settings were not saved.';
    await refreshSettings();
  }catch(error){status.textContent='Could not save LED settings: '+error.message;}
});
// Local browser upload reuses the OTA API; the password is not stored in
// browser settings or placed in a query string. The ESP32 validates the image.
function renderOtaReadiness(s){
  const label=$('ota-safety');
  const safe=s?.ota_update_safe===true;
  if(!s)label.textContent='Device status unavailable. OTA is disabled.';
  else if(s.ota_busy)label.textContent='OTA already in progress. Do not restart the board.';
  else if(!safe)label.textContent='Browser OTA blocked: '+
    String(s.ota_layout_issue||'no safe inactive slot').replaceAll('_',' ')+
    '. Use a safe USB recovery method instead.';
  else label.textContent='Safe inactive OTA slot available ('+s.ota_slot_count+
    ' slots). The firmware board profile must match exactly.';
  label.classList.toggle('warn',!safe);
  $('ota-install').disabled=!safe||Boolean(s?.ota_busy)||otaUploading;
}
$('ota-form').addEventListener('submit',async event=>{
  event.preventDefault();
  if(otaUploading)return;
  const message=$('ota-feedback'),firmware=$('ota-file').files[0];
  const password=$('ota-key').value,progress=$('ota-progress');
  if(!lastStatus||lastStatus.ota_update_safe!==true||lastStatus.ota_busy){
    message.textContent='OTA is not safe or is already busy. Check the slot status.';return;
  }
  if(!firmware||!/\.bin$/i.test(firmware.name)||firmware.size<256||firmware.size>4*1024*1024){
    message.textContent='Select an ESP-IDF .bin image between 256 bytes and 4 MB.';return;
  }
  if(password.length<12||password.length>64){
    message.textContent='Enter the OTA administrator password (12–64 characters).';return;
  }
  try{
    const header=new Uint8Array(await firmware.slice(0,1).arrayBuffer());
    if(header[0]!==0xe9){
      message.textContent='Invalid ESP32 application image header. Upload cancelled.';return;
    }
  }catch{message.textContent='Cannot inspect the selected firmware file.';return;}
  if(!window.confirm('Install '+firmware.name+' on this RFLink test board? The board will restart after a successful upload.'))return;
  auditUi('ota_confirm');
  otaUploading=true;progress.value=0;renderOtaReadiness(lastStatus);
  message.textContent='Uploading firmware… keep the board powered.';
  const request=new XMLHttpRequest();
  request.open('POST','api/ota');
  request.setRequestHeader('Content-Type','application/octet-stream');
  request.setRequestHeader('X-RFLink-Update-Key',password);
  request.upload.onprogress=e=>{
    if(e.lengthComputable)progress.value=Math.min(99,Math.round(e.loaded/e.total*100));
  };
  request.onload=()=>{
    auditUi('ota_result',request.status);
    if(request.status>=200&&request.status<300){
      progress.value=100;
      message.textContent='Firmware accepted; RFLink is restarting. Check the new version after reconnecting.';
      $('ota-key').value='';
      // Keep new writes disabled until the page is reloaded.
    }else{
      message.textContent='Upload rejected (HTTP '+request.status+'): '+
        (request.responseText||'Check the image, password and partition layout.');
      otaUploading=false;renderOtaReadiness(lastStatus);
    }
  };
  request.onerror=()=>{
    auditUi('ota_result',0);
    message.textContent='Connection lost during OTA. Result unknown; verify device status before retrying.';
    otaUploading=false;renderOtaReadiness(null);
  };
  request.onabort=()=>{
    auditUi('ota_result',0);
    message.textContent='Upload aborted. Verify the device before retrying.';
    otaUploading=false;renderOtaReadiness(null);
  };
  request.send(firmware);
});
$('logs-settings-form').addEventListener('submit',async e=>{
  e.preventDefault();
  auditUi('settings_submit');
  const form=e.currentTarget,status=$('settings-save-status');
  const data={};
  for(const field of ['signal_0','signal_1','signal_2','noise']){
    const input=form.elements[field],value=Number(input.value);
    if(!Number.isSafeInteger(value)||value<0||value>Number(input.max)){
      status.textContent='Enter valid integer limits for all buffers.';return;
    }
    data[field]=value;
  }
  status.textContent='Saving logging settings…';
  try{
    const saved=await api('api/logs/config',{method:'POST',headers:{
      'Content-Type':'application/json'},body:JSON.stringify(data)});
    status.textContent=saved.saved===true?'Saved on RFLink. Settings survive reboot.':
      'Applied for this session, but not saved to flash. '+(saved.warning||'');
    await refreshSettings();
  }catch(error){status.textContent='Could not save: '+error.message;}
});
document.addEventListener('visibilitychange',()=>{
  if(document.visibilityState==='visible'){
    refreshStatus();if(currentPage==='logs'&&logPageCursors.length===1&&!logLivePaused)refreshLogs();
    if(currentPage==='diagnostics'){refreshHistory();pollConsole();}
  }
});
async function tick(){
  if(document.visibilityState==='hidden')return;
  await refreshStatus();
  if(currentPage==='status')refreshEvents();
  if(currentPage==='logs'&&logPageCursors.length===1&&!logLivePaused)refreshLogs();
  if(currentPage==='diagnostics'){refreshHistory();pollConsole();}
}
refreshStatus().then(refreshEvents);
auditUi('page_open');
setInterval(tick,5000);
