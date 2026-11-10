/* Native Radioactive web app. No ESPHome runtime and no legacy RFLink REST calls.
 * All radio actions are read-only in this stage; never simulate pairing or TX.
 */
'use strict';
const $=id=>document.getElementById(id);
const pages=[...document.querySelectorAll('.page')];
const nav=[...document.querySelectorAll('[data-page]')];
const radioNames=['433.42 MHz','433.92 MHz','868.30 MHz'];
let currentPage='status', lastStatus=null, lastEvents=[], logSource='all';
let consoleConnected=false, consoleCursor=0, consoleBootId=null, consoleBusy=false;
let statusBusy=false, logsBusy=false, historyBusy=false;
let nextErrorShown=false,otaUploading=false;
const fmt=n=>typeof n==='number'&&Number.isFinite(n)?n.toLocaleString():'—';
const yesNo=v=>v===true?'Yes':v===false?'No':'—';
// DD.HH.MM.SS shows duration since boot, not clock time.
const uptime=seconds=>{
  if(!Number.isFinite(seconds)||seconds<0)return '—';
  const s=Math.floor(seconds),d=Math.floor(s/86400);
  const h=Math.floor((s%86400)/3600),m=Math.floor((s%3600)/60);
  return [d,h,m,s%60].map(n=>String(n).padStart(2,'0')).join('.');
};
const uptimeMs=milliseconds=>Number.isFinite(milliseconds)&&milliseconds>=0?
  uptime(milliseconds/1000):'—';
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
  if(!path.startsWith('/api/'))throw new Error('Unsupported API path');
  let response;
  try{response=await fetch(path,{cache:'no-store',...options});}
  catch{throw new Error('Cannot reach RFLink over the network');}
  if(!response.ok)throw new Error('RFLink HTTP '+response.status);
  try{return await response.json();}
  catch{throw new Error('RFLink returned invalid JSON');}
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
  const ready=[s.rx43342_listening===true,s.rx43392_listening===true,s.rx868_listening===true];
  $('rx-count').textContent=initialized?ready.filter(Boolean).length+' / 3':'— / 3';
  const frames=[s.rx43342_frames,s.rx43392_frames,s.rx868_packets];
  for(let i=0;i<3;i++){
    const present=initialized&&Array.isArray(s.cc1101_present)?s.cc1101_present[i]===true:false;
    const label=!initialized?'Starting':!present?'Not detected':ready[i]?'Listening':'RX inactive';
    markStatus($('radio-status-'+i),label,!initialized?'warn':ready[i]?'online':present?'warn':'offline');
    $('radio-hw-'+i).textContent=!initialized?'Checking':present?'Detected':'Missing';
    $('radio-frames-'+i).textContent=initialized?fmt(frames[i]):'—';
    $('radio-spi-'+i).textContent=initialized?fmt((s.spi_ready_timeouts||[])[i]):'—';
  }
  $('info-ip').textContent=s.ipv4||'Not assigned';
  $('info-wifi').textContent=connected?'Connected':'Not connected';
  $('info-mqtt').textContent=s.mqtt_connected?'Connected':'Not connected';
  $('info-uptime').textContent=uptime(s.uptime_s);
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
  try{renderStatus(await api('/api/status'));}
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
  try{const r=await api('/api/events');lastEvents=Array.isArray(r.events)?r.events:[];renderRecent(lastEvents);}
  catch{$('recent-events').textContent='Receiver event feed temporarily unavailable.';}
}
function selectedLogQuery(){
  const p=new URLSearchParams(),radio=$('log-radio').value,noise=$('log-noise').checked;
  if(logSource==='system'||logSource==='web')p.set('stream',logSource);
  else if(logSource==='radio'&&!noise)p.set('stream','signal');
  if((logSource==='radio'||logSource==='all')&&radio)p.set('radio',radio);
  if(noise&&['all','radio'].includes(logSource))p.set('noise','1');
  return p;
}
function eventIsVisible(event){
  if(logSource==='radio'&&!['noise','signal'].includes(event.stream))return false;
  if(logSource==='system'&&event.stream!=='system')return false;
  if(logSource==='web'&&event.stream!=='web')return false;
  if(!$('log-noise').checked&&event.stream==='noise')return false;
  return true;
}
function logDetails(e){
  const parts=[];
  if(e.count)parts.push(e.count+' items');
  if(e.correlation_id)parts.push('TX #'+e.correlation_id);
  if(e.profile)parts.push('profile '+e.profile);
  if(e.tx_driver_attempted)parts.push('driver '+(e.tx_driver_accepted?'accepted (RF unverified)':'failed'));
  if(e.tx_rx_previous)parts.push('RX '+(e.tx_rx_restored?'restored':'NOT restored'));
  if(e.value)parts.push('value '+e.value);
  return parts.length?parts.join(' · '):'—';
}
function renderLogs(events){
  const tbody=$('log-rows');tbody.replaceChildren();
  const displayed=events.filter(eventIsVisible).slice().reverse();
  $('log-feedback').textContent=displayed.length+' recent events · uptime DD.HH.MM.SS since boot';
  if(!displayed.length){const tr=document.createElement('tr'),td=document.createElement('td');td.colSpan=5;
    td.textContent='No matching events in the recent ring buffer.';tr.append(td);tbody.append(tr);return;}
  for(const e of displayed){
    const tr=document.createElement('tr');
    if(e.severity===2)tr.className='is-error';
    else if(e.severity===1)tr.className='is-warning';
    const values=[uptimeMs(e.uptime_ms),e.stream||'—',
      radioNames[e.radio]||'System',e.kind||'Unknown',logDetails(e)];
    for(let i=0;i<values.length;i++){
      const td=document.createElement('td');td.textContent=values[i];
      if(i===4)td.className='detail';
      tr.append(td);
    }
    tbody.append(tr);
  }
}
async function refreshLogs(){
  if(logsBusy)return;
  logsBusy=true;
  const query=selectedLogQuery();
  // The native CSV handler uses the same query. When Radio+noise is selected,
  // the backend cannot combine streams; expose this limitation explicitly.
  $('log-export').href='/api/logs.csv'+(query.size?'?'+query.toString():'');
  try{
    const r=await api('/api/logs'+(query.size?'?'+query.toString():''));
    renderLogs(Array.isArray(r.events)?r.events:[]);
    if(logSource==='radio'&&$('log-noise').checked)
      $('log-feedback').textContent+=' · CSV may include System/Web with this combined filter';
  }catch(error){$('log-feedback').textContent='Log feed unavailable: '+error.message;}
  finally{logsBusy=false;}
}
for(const button of document.querySelectorAll('[data-log-source]'))button.addEventListener('click',()=>{
  logSource=button.dataset.logSource;
  for(const tab of document.querySelectorAll('[data-log-source]')){
    const active=tab===button;tab.classList.toggle('selected',active);
    tab.setAttribute('aria-pressed',String(active));
  }
  refreshLogs();
});
$('log-radio').addEventListener('change',refreshLogs);
$('log-noise').addEventListener('change',refreshLogs);
function renderHistory(report){
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
    stage.textContent=phase?(String(phase.stage||'unknown').replaceAll('_',' ')+
      (phase.startup_complete?' · ready':' · last reached')):'Checkpoint unavailable';
    row.append(reason,stage);list.append(row);
  }
}
async function refreshHistory(){
  if(historyBusy)return;historyBusy=true;
  try{renderHistory(await api('/api/diagnostics/crashes'));}
  catch(error){$('diag-boots').textContent='Boot history unavailable: '+error.message;}
  finally{historyBusy=false;}
}
$('diag-refresh').addEventListener('click',refreshHistory);
function disconnectConsole(){
  consoleConnected=false;consoleCursor=0;consoleBootId=null;
  $('console-connect').disabled=false;$('console-disconnect').disabled=true;
  markStatus($('console-state'),'Disconnected');
}
$('console-disconnect').addEventListener('click',disconnectConsole);
$('console-clear').addEventListener('click',()=>{$('console-output').textContent='';});
$('console-connect').addEventListener('click',()=>{
  // The captured console is read-only and already filtered before sharing.
  consoleConnected=true;consoleCursor=0;consoleBootId=null;
  $('console-connect').disabled=true;$('console-disconnect').disabled=false;
  markStatus($('console-state'),'Connecting','warn');pollConsole();
});
async function pollConsole(){
  if(!consoleConnected||consoleBusy||currentPage!=='diagnostics')return;
  consoleBusy=true;
  try{
    let r=await api('/api/diagnostics/console?since='+consoleCursor);
    if(consoleBootId!==null&&r.boot_id!==consoleBootId){
      $('console-output').textContent+='\n— ESP32 rebooted: new application console —\n';
      consoleCursor=0;
      r=await api('/api/diagnostics/console?since=0');
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
    const r=await api('/api/logs/config');
    if(!r.ready){$('log-budget').textContent='Log storage is not initialized yet.';return;}
    for(let i=0;i<3;i++)$('logs-settings-form').elements['signal_'+i].value=r.signal[i];
    $('logs-settings-form').elements.noise.value=r.noise;
    $('log-budget').textContent=Math.round(r.allocated_bytes/1024)+' KB allocated of '+
      Math.round(r.budget_bytes/1024)+' KB · '+
      (r.overwritten||[]).reduce((a,b)=>a+b,0)+' overwritten events';
  }catch(error){$('log-budget').textContent='Log settings unavailable: '+error.message;}
  try{
    const mqtt=await api('/api/mqtt/status');
    $('settings-mqtt').textContent=mqtt.configured?(mqtt.connected?'Connected':'Not connected'):'Not configured';
  }catch{$('settings-mqtt').textContent='Unavailable';}
  try{
    const led=await api('/api/led');
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
  const status=$('led-save-status');
  const brightness=Number($('led-brightness').value);
  if(!Number.isSafeInteger(brightness)||brightness<1||brightness>100){
    status.textContent='Choose brightness between 1 and 100%.';return;
  }
  status.textContent='Saving LED settings…';
  try{
    const result=await api('/api/led',{method:'POST',headers:{
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
  otaUploading=true;progress.value=0;renderOtaReadiness(lastStatus);
  message.textContent='Uploading firmware… keep the board powered.';
  const request=new XMLHttpRequest();
  request.open('POST','/api/ota');
  request.setRequestHeader('Content-Type','application/octet-stream');
  request.setRequestHeader('X-RFLink-Update-Key',password);
  request.upload.onprogress=e=>{
    if(e.lengthComputable)progress.value=Math.min(99,Math.round(e.loaded/e.total*100));
  };
  request.onload=()=>{
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
    message.textContent='Connection lost during OTA. Result unknown; verify device status before retrying.';
    otaUploading=false;renderOtaReadiness(null);
  };
  request.onabort=()=>{
    message.textContent='Upload aborted. Verify the device before retrying.';
    otaUploading=false;renderOtaReadiness(null);
  };
  request.send(firmware);
});
$('logs-settings-form').addEventListener('submit',async e=>{
  e.preventDefault();
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
    const saved=await api('/api/logs/config',{method:'POST',headers:{
      'Content-Type':'application/json'},body:JSON.stringify(data)});
    status.textContent=saved.saved===true?'Saved on RFLink. Settings survive reboot.':
      'Applied for this session, but not saved to flash. '+(saved.warning||'');
    await refreshSettings();
  }catch(error){status.textContent='Could not save: '+error.message;}
});
document.addEventListener('visibilitychange',()=>{
  if(document.visibilityState==='visible'){
    refreshStatus();if(currentPage==='logs')refreshLogs();
    if(currentPage==='diagnostics'){refreshHistory();pollConsole();}
  }
});
async function tick(){
  if(document.visibilityState==='hidden')return;
  await refreshStatus();
  if(currentPage==='status')refreshEvents();
  if(currentPage==='logs')refreshLogs();
  if(currentPage==='diagnostics'){refreshHistory();pollConsole();}
}
refreshStatus().then(refreshEvents);
setInterval(tick,5000);
