const csrf = document.querySelector('meta[name="csrf"]').content;
const statusPill = document.getElementById('status-pill');
const sourceTitle = document.getElementById('source-title');
const sourceInfo = document.getElementById('source-info');
const upload = document.getElementById('upload-file');
const buildBtn = document.getElementById('build-btn');
const cancelBtn = document.getElementById('cancel-btn');
const message = document.getElementById('build-message');
const logOutput = document.getElementById('log-output');
const logState = document.getElementById('log-state');
const logSize = document.getElementById('log-size');
const output = document.getElementById('output');
let lastJob = '', logOffset = 0, inFlight = false;
let toolchainReady = false;

async function api(path, options = {}) {
  const headers = { 'X-RFLink-CSRF': csrf, ...(options.headers || {}) };
  const response = await fetch(path, { ...options, headers, cache: 'no-store' });
  let payload;
  try { payload = await response.json(); } catch { throw new Error('Invalid response from the Builder'); }
  if (!response.ok) throw new Error(payload.error || `Error ${response.status}`);
  return payload;
}

function setMessage(text, isError = false) {
  message.textContent = text;
  message.style.color = isError ? '#f19c95' : '';
}

async function refresh() {
  if (inFlight) return;
  inFlight = true;
  try {
    const s = await api('api/status');
    if (s.source) {
      sourceTitle.textContent = s.source.name || 'RFLink source available';
      sourceInfo.textContent = s.source.sha256 ? `SHA-256 ${s.source.sha256.slice(0, 12)}… · ${s.source.files} files` : 'Stored project';
    } else {
      sourceTitle.textContent = 'No RFLink source available';
      sourceInfo.textContent = 'Upload an RFLink Universal ZIP';
    }
    const busy = s.status === 'running' || s.status === 'cancelling';
    upload.disabled = busy;
    buildBtn.disabled = busy || !s.source || !toolchainReady;
    cancelBtn.hidden = !busy;
    cancelBtn.disabled = s.status === 'cancelling';
    output.hidden = !(s.status === 'success' && s.ota_ready && s.bundle_ready);
    document.getElementById('output-release-label').textContent = s.build_release?.tag
      ? `Selected GitHub Release: ${s.build_release.tag} · rebuilt locally with your board profile`
      : 'Built from current local source.';
    if (s.job_id !== lastJob) {
      lastJob = s.job_id;
      logOffset = 0;
      logOutput.textContent = '';
    }
    const labels = {idle:'Ready to build',running:'Building…',cancelling:'Stopping build…',success:'Build succeeded',failed:'Build failed',canceled:'Build canceled'};
    const kinds = {running:'running',cancelling:'running',success:'success',failed:'failed',canceled:'failed'};
    statusPill.className = `pill ${kinds[s.status] || 'muted'}`;
    statusPill.innerHTML = '<span class="dot"></span>';
    statusPill.append(document.createTextNode(' ' + (labels[s.status] || s.status)));
    logState.textContent = labels[s.status] || 'Unknown';
    if (s.status === 'failed') setMessage(s.error || 'Build failed; see log', true);
    else if (s.status === 'canceled') setMessage('Build canceled', true);
    else if (s.status === 'success') setMessage((s.build_release?.tag?('Release '+s.build_release.tag+' · '):'')+'Firmware files ready · hardware not tested');
    else if (s.status === 'running') setMessage('ESP-IDF is building on Home Assistant');
    else if (s.status === 'idle') setMessage('Waiting for a command');
    const log = await api(`api/log?offset=${logOffset}`);
    if (log.text) {
      const atBottom = logOutput.scrollHeight - logOutput.scrollTop <= logOutput.clientHeight + 50;
      if (logOffset === 0) logOutput.textContent = '';
      logOutput.textContent += log.text;
      if (logOutput.textContent.length > 250000) logOutput.textContent = logOutput.textContent.slice(-180000);
      if (atBottom) logOutput.scrollTop = logOutput.scrollHeight;
    }
    logOffset = log.next_offset;
    logSize.textContent = `${Math.round((log.size || 0)/1024)} KB`;
  } catch (error) { setMessage(error.message, true); }
  finally { inFlight = false; }
}

upload.addEventListener('change', async () => {
  const file = upload.files?.[0];
  if (!file) return;
  try {
    if (file.size > 32 * 1024 * 1024) throw new Error('File exceeds 32 MB');
    setMessage('Uploading source…');
    const preview=await api('api/source/preview', {method:'POST',headers:{'Content-Type':'application/zip','X-RFLink-Filename':file.name},body:file});
    showPreview(preview);
    setMessage('ZIP compared: review the changes and select Apply changes');
  } catch (error) { setMessage(error.message, true); }
  finally { upload.value = ''; }
});

buildBtn.addEventListener('click', async () => {
  buildBtn.disabled = true;
  try { await api('api/build',{method:'POST'}); logOffset = 0; await refresh(); }
  catch (error) { setMessage(error.message, true); buildBtn.disabled = false; }
});
cancelBtn.addEventListener('click', async () => {
  try { await api('api/cancel',{method:'POST'}); await refresh(); }
  catch (error) { setMessage(error.message, true); }
});
document.getElementById('copy-log').addEventListener('click', async () => {
  try { await navigator.clipboard.writeText(logOutput.textContent); setMessage('Log copied'); }
  catch { setMessage('Browser did not permit copying', true); }
});
refresh();
setInterval(refresh, 1250);

/* Minimal Builder navigation: everyday work on the Build page, advanced
   functions on dedicated pages, without removing existing functionality. */
for (const tab of document.querySelectorAll('.nav-tab')) {
  tab.addEventListener('click', () => {
    const selected = tab.dataset.page;
    for (const t of document.querySelectorAll('.nav-tab')) {
      const active = t === tab;
      t.classList.toggle('selected', active);
      t.setAttribute('aria-current', active ? 'page' : 'false');
    }
    for (const section of document.querySelectorAll('[data-page-section]')) {
      section.classList.toggle('active-view', section.dataset.pageSection === selected);
    }
  });
}
const setupState = document.getElementById('toolchain-state');
const setupMessage = document.getElementById('toolchain-message');
const setupLog = document.getElementById('toolchain-log');
const setupRetry = document.getElementById('toolchain-retry');
const setupPanel = document.getElementById('toolchain-console');
let setupOffset = 0, setupPolling = false, setupCollapsed = false;
async function refreshToolchain() {
  if (setupPolling) return;
  setupPolling = true;
  try {
    const s = await api('api/toolchain/status');
    toolchainReady = !!s.ready;
    const labels = { idle: 'Pending', running: 'Installing', ready: 'Ready', failed: 'Failed' };
    setupState.textContent = labels[s.status] || s.status;
    setupState.dataset.state = s.status;
    setupMessage.textContent = s.status === 'ready'
      ? 'Official ESP-IDF 5.5.6 installed. Ready to compile.'
      : s.status === 'running'
        ? 'Preparing the compiler in the background. Follow the console below.'
        : s.status === 'failed'
          ? 'Setup failed: ' + (s.error || 'See console and retry.')
          : 'Compiler setup has not started yet.';
    setupRetry.hidden = s.status !== 'failed' && s.status !== 'idle';
    if (s.status === 'ready' && !setupCollapsed) {
      setupPanel.open = false;
      setupCollapsed = true;
    }
    if (s.status === 'failed') setupPanel.open = true;
    else if (s.status === 'running' && !setupCollapsed) setupPanel.open = true;
    if (!toolchainReady) buildBtn.disabled = true;
    const historicalBuild = document.getElementById('release-build');
    if (historicalBuild) historicalBuild.disabled = !toolchainReady || !selectedRelease;
    const chunk = await api('api/toolchain/log?offset=' + setupOffset);
    if (chunk.next_offset < setupOffset) {
      setupOffset = 0;
      setupLog.textContent = '';
    }
    if (chunk.text) {
      const atBottom = setupLog.scrollHeight - setupLog.scrollTop < setupLog.clientHeight + 40;
      if (setupOffset === 0) setupLog.textContent = '';
      setupLog.textContent += chunk.text;
      if (setupLog.textContent.length > 150000) setupLog.textContent = setupLog.textContent.slice(-110000);
      if (atBottom) setupLog.scrollTop = setupLog.scrollHeight;
    }
    setupOffset = chunk.next_offset;
    document.getElementById('toolchain-log-size').textContent =
      '· ' + Math.round((chunk.size || 0) / 1024) + ' KB';
  } catch (error) {
    setupMessage.textContent = 'Cannot read setup status: ' + error.message;
  } finally {
    setupPolling = false;
  }
}
setupRetry.addEventListener('click', async () => {
  setupRetry.disabled = true;
  try {
    await api('api/toolchain/retry', { method: 'POST' });
    setupOffset = 0;
    setupLog.textContent = '';
    setupCollapsed = false;
    setupPanel.open = true;
    await refreshToolchain();
  } catch (error) {
    setupMessage.textContent = error.message;
  } finally {
    setupRetry.disabled = false;
  }
});
refreshToolchain();
setInterval(refreshToolchain, 1750);



// Stage 0.2 — separate firmware deployment and broker management.
const otaIp = document.getElementById('ota-ip');
const otaDevice = document.getElementById('ota-device');
let otaKeyAvailable = false;
const usbPort = document.getElementById('usb-port');
const usbSend = document.getElementById('usb-send');
const otaSend = document.getElementById('ota-send');
const flashConfirm = document.getElementById('flash-confirm');
const deployState = document.getElementById('deploy-state');
const deployLog = document.getElementById('deploy-log');
let hasRelease = false;
let deploymentBusy = false;

async function updateSerial() {
  try {
    const ports = (await api('api/serial')).ports;
    const old = usbPort.value;
    usbPort.replaceChildren();
    const empty = document.createElement('option'); empty.value=''; empty.textContent = ports.length ? 'Select a USB port' : 'No serial ports connected';
    usbPort.append(empty);
    for (const port of ports) {
      const node = document.createElement('option'); node.value=port; node.textContent=port; usbPort.append(node);
    }
    if (ports.includes(old)) usbPort.value=old;
    enableDeployment();
  } catch(e) {deployState.textContent = e.message;}
}
function enableDeployment() {
  const ok=hasRelease && !deploymentBusy && flashConfirm.value.trim()==='TESTBOARD';
  otaSend.disabled=!ok || !otaDevice.value || !otaKeyAvailable;
  usbSend.disabled=!ok || !usbPort.value;
}
for (const control of [flashConfirm,usbPort,otaDevice]) control.addEventListener('input',enableDeployment);

async function deploymentRefresh() {
  try {
    const [build, dep] = await Promise.all([api('api/status'),api('api/deploy/status')]);
    hasRelease=build.status==='success' && build.ota_ready && build.bundle_ready;
    deploymentBusy=dep.status==='running';
    enableDeployment();
    const label={idle:'No installation started',running:'Installing firmware…',success:'Installation completed; verify the device',failed:'Installation failed'};
    deployState.textContent=(label[dep.status]||dep.status)+(dep.message ? ' · '+dep.message:'')+(dep.error?' · '+dep.error:'');
    deployState.classList.toggle('error',dep.status==='failed');
    if (dep.status!=='idle') deployLog.textContent=(await api('api/deploy/log')).text;
  } catch(err) {deployState.textContent=err.message;deployState.classList.add('error');}
}
async function startDeploy(method) {
  if (!confirm('Use a separate test ESP32 only. Continue with '+method.toUpperCase()+'?')) return;
  try {
    const payload={confirm:flashConfirm.value.trim()};
    if (method==='ota') payload.device_id=otaDevice.value;
    else payload.port=usbPort.value;
    await api('api/deploy/'+method,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(payload)});
    deployState.textContent='Installation started; follow the log.';
    await deploymentRefresh();
  } catch(err) {deployState.textContent=err.message;deployState.classList.add('error');}
}
otaSend.addEventListener('click',()=>startDeploy('ota'));
usbSend.addEventListener('click',()=>startDeploy('usb'));
document.getElementById('usb-refresh').addEventListener('click',updateSerial);

const mqttStatus = document.getElementById('mqtt-status');
const mqttHost = document.getElementById('mqtt-host');
const mqttPort = document.getElementById('mqtt-port');
const mqttUser = document.getElementById('mqtt-username');
const mqttPass = document.getElementById('mqtt-password');
const mqttTls = document.getElementById('mqtt-tls');
async function loadMQTT() {
  try {
    const c=await api('api/mqtt');
    mqttHost.value=c.host||'';mqttPort.value=c.port||1883;
    mqttUser.value=c.username||'';mqttPass.value='';mqttTls.checked=!!c.tls;
    const src=c.source==='home_assistant'?'Home Assistant broker found':c.source==='manual'?'Manual profile saved':'No broker found';
    mqttStatus.textContent=src+' · '+c.note+(c.password_saved?' · password saved':'');
    mqttStatus.classList.remove('error');
  } catch(e) {mqttStatus.textContent=e.message;mqttStatus.classList.add('error');}
}
document.getElementById('mqtt-save').addEventListener('click',async()=>{
  try {
    await api('api/mqtt/save',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({
      host:mqttHost.value.trim(),port:Number(mqttPort.value),username:mqttUser.value,
      password:mqttPass.value,tls:mqttTls.checked})});
    await loadMQTT();mqttStatus.textContent='Settings saved in the Home Assistant Builder. Not yet sent to RFLink.';
  } catch(e) {mqttStatus.textContent=e.message;mqttStatus.classList.add('error');}
});
document.getElementById('mqtt-test').addEventListener('click',async()=>{
  mqttStatus.textContent='Checking broker connection…';
  try {const r=await api('api/mqtt/test',{method:'POST'});mqttStatus.textContent=r.message;mqttStatus.classList.remove('error');}
  catch(e) {mqttStatus.textContent=e.message;mqttStatus.classList.add('error');}
});
document.getElementById('mqtt-auto').addEventListener('click',async()=>{
  try {await api('api/mqtt/reset',{method:'POST'});await loadMQTT();}
  catch(e) {mqttStatus.textContent=e.message;mqttStatus.classList.add('error');}
});
updateSerial();loadMQTT();deploymentRefresh();setInterval(deploymentRefresh,2500);

// RFLink native MQTT management bridge (Stage 0.3).
const mqttPush=document.getElementById('mqtt-push');
const mqttTargetIp=document.getElementById('mqtt-target-ip');
const mqttTargetKey=document.getElementById('mqtt-target-key');
const mqttConfirm=document.getElementById('mqtt-confirm');
function updateMqttPush(){mqttPush.disabled=!(mqttTargetIp.value.trim()&&mqttTargetKey.value.length>=12&&mqttConfirm.value.trim()==='CONFIGURE');}
for(const c of [mqttTargetIp,mqttTargetKey,mqttConfirm])c.addEventListener('input',updateMqttPush);
mqttPush.addEventListener('click',async()=>{
  if(!confirm('Send broker settings to this RFLink and restart it?'))return;
  mqttPush.disabled=true;mqttStatus.textContent='Sending RFLink configuration…';
  try{
    const r=await api('api/mqtt/push',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({
      ip:mqttTargetIp.value.trim(),key:mqttTargetKey.value,confirm:mqttConfirm.value.trim()})});
    mqttStatus.textContent=r.message;
    mqttStatus.classList.remove('error');
  }catch(e){mqttStatus.textContent=e.message;mqttStatus.classList.add('error');}
  finally{mqttTargetKey.value='';updateMqttPush();}
});
updateMqttPush();

// Explicitly replace an older persisted Stage 19 source after app upgrade.
document.getElementById('default-source').addEventListener('click',async()=>{
  if(!confirm('Replace current source with the included Radioactive Stage 20?'))return;
  try{await api('api/source/reset',{method:'POST'});lastJob='';logOffset=0;setMessage('Stage 20 source loaded');await refresh();}
  catch(e){setMessage(e.message,true);}
});


// v0.4 — repository-aware source updates and persistent local settings.
let syncToken='';
const repoName=document.getElementById('repo-name');
const repoStatus=document.getElementById('repo-status');
const syncResult=document.getElementById('sync-result');
const applySource=document.getElementById('source-apply');
const boardSpeed=document.getElementById('board-speed');
const boardStatus=document.getElementById('board-status');
let boardValue=null;
const pins=[['spi.sclk','SPI SCLK'],['spi.mosi','SPI MOSI'],['spi.miso','SPI MISO'],
  ['radios.0.cs','433,42 CS'],['radios.0.gdo0','433,42 GDO0'],['radios.0.gdo2','433,42 GDO2'],
  ['radios.1.cs','433,92 CS'],['radios.1.gdo0','433,92 GDO0'],['radios.1.gdo2','433,92 GDO2'],
  ['radios.2.cs','868,30 CS'],['radios.2.gdo0','868,30 GDO0'],['radios.2.gdo2','868,30 GDO2']];
function pinValue(board,path){const [a,b,c]=path.split('.');return c?board.radios[Number(b)][c]:board.spi[b];}
function setPinValue(board,path,val){const [a,b,c]=path.split('.');if(c)board.radios[Number(b)][c]=val;else board.spi[b]=val;}
function renderBoard(board){
  boardValue=board; boardSpeed.value=board.spi.clock_hz;
  const grid=document.getElementById('pin-grid');grid.replaceChildren();
  for(const [path,title] of pins){
    const wrap=document.createElement('div');const label=document.createElement('label');label.textContent=title;
    const input=document.createElement('input');input.type='number';input.min='0';input.max='48';input.step='1';input.value=pinValue(board,path);
    input.dataset.pin=path;wrap.append(label,input);grid.append(wrap);
  }
}
async function loadBoard(){try{const r=await api('api/board');renderBoard(r.board);boardStatus.textContent=r.local_override?'Local custom board profile active':'Default GitHub board profile active';}
  catch(e){boardStatus.textContent=e.message;}}
document.getElementById('board-save').addEventListener('click',async()=>{
  try{const b=structuredClone(boardValue);b.spi.clock_hz=Number(boardSpeed.value);
    document.querySelectorAll('[data-pin]').forEach(i=>setPinValue(b,i.dataset.pin,Number(i.value)));
    const r=await api('api/board/save',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({board:b})});
    renderBoard(r.board);boardStatus.textContent=`Saved · board hash ${r.hash}. ${r.note}`;
  }catch(e){boardStatus.textContent=e.message;}
});
document.getElementById('board-reset').addEventListener('click',async()=>{
  if(!confirm('Restore the default pinout and SPI speed from GitHub?'))return;
  try{await api('api/board/reset',{method:'POST'});await loadBoard();}catch(e){boardStatus.textContent=e.message;}
});
async function loadSecrets(){try{const r=await api('api/secrets');
  document.getElementById('secret-wifi').value=r.wifi_ssid||'';
  document.getElementById('secret-pass').value='';document.getElementById('secret-ota').value='';
  document.getElementById('secrets-status').textContent=`Wi-Fi password: ${r.wifi_password_set?'set':'not set'} · OTA key: ${r.ota_key_set?'set':'not set'}`;
  }catch(e){document.getElementById('secrets-status').textContent=e.message;}}
document.getElementById('secrets-save').addEventListener('click',async()=>{
  const payload={wifi_ssid:document.getElementById('secret-wifi').value,
    wifi_password:document.getElementById('secret-pass').value,ota_key:document.getElementById('secret-ota').value};
  try{await api('api/secrets/save',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(payload)});
    await loadSecrets();document.getElementById('secrets-status').textContent+=' · Saved';}
  catch(e){document.getElementById('secrets-status').textContent=e.message;}
});
document.getElementById('secrets-clear').addEventListener('click',async()=>{
  if(!confirm('Clear all locally saved Wi-Fi and OTA credentials?'))return;
  await api('api/secrets/clear',{method:'POST'});await loadSecrets();
});
async function loadRepo(){try{const r=await api('api/github');repoName.value=r.repo||'juulius/rflink-universal';document.getElementById('repo-display').textContent=repoName.value;document.getElementById('known-repo').textContent=repoName.value;}catch(e){repoStatus.textContent=e.message;}}
document.getElementById('repo-save').addEventListener('click',async()=>{
  try{const r=await api('api/github/save',{method:'POST',headers:{'Content-Type':'application/json'},
    body:JSON.stringify({repo:repoName.value.trim()})});repoStatus.textContent='Repository saved: '+r.repo;document.getElementById('repo-display').textContent=r.repo;document.getElementById('known-repo').textContent=r.repo;allPageLoaded=false;releasePage=1;releaseAll.hidden=true;await refreshRecent();}
  catch(e){repoStatus.textContent=e.message;}
});
document.getElementById('repo-check').addEventListener('click',async()=>{
  repoStatus.textContent='Checking GitHub versions…';
  try{const r=await api('api/github/check',{method:'POST'});
    repoStatus.textContent=`GitHub source: ${r.revision.slice(0,12)} · ${r.release.available?'Firmware release '+r.release.version:'No stable firmware release yet'}.`;
  }catch(e){repoStatus.textContent=e.message;}
});
function showPreview(r){
  syncToken=r.token;applySource.hidden=false;syncResult.hidden=false;
  const c=r.counts;const line=`New: ${c.new} · Changed: ${c.updated} · Unchanged: ${c.same} · Conflict: ${c.conflict} · Removed upstream: ${c.obsolete}`;
  syncResult.replaceChildren(document.createTextNode(line));
  const list=document.createElement('ul');list.className='sync-list';
  for(const row of r.changes.filter(x=>x.status!=='same').slice(0,40)){
    const li=document.createElement('li');li.textContent=`${row.status}: ${row.path}`;list.append(li);
  }
  syncResult.append(list);
}
document.getElementById('repo-preview').addEventListener('click',async()=>{
  try{repoStatus.textContent='Downloading and comparing source…';
    const r=await api('api/github/preview',{method:'POST'});showPreview(r);
    repoStatus.textContent=`GitHub ${r.revision.slice(0,12)} compared. Review the changes before applying.`;
  }catch(e){repoStatus.textContent=e.message;}
});
document.getElementById('source-preview-upload').addEventListener('click',()=>document.getElementById('source-compare-file').click());
document.getElementById('source-compare-file').addEventListener('change',async(e)=>{
  const f=e.target.files?.[0];if(!f)return;
  try{const r=await api('api/source/preview',{method:'POST',headers:{'Content-Type':'application/zip'},body:f});showPreview(r);repoStatus.textContent='Uploaded ZIP compared';}
  catch(err){repoStatus.textContent=err.message;}finally{e.target.value='';}
});
applySource.addEventListener('click',async()=>{
  if(!syncToken || !confirm('Apply new and changed source? Conflicts and local settings remain intact.'))return;
  try{const r=await api('api/source/apply',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({token:syncToken})});
    syncResult.textContent=`${r.applied} files updated. ${r.counts.conflict} conflicts preserved.`;
    applySource.hidden=true;syncToken='';await refresh();
  }catch(e){repoStatus.textContent=e.message;}
});
loadRepo();loadBoard();loadSecrets();


// v0.5 — GitHub Releases history, isolated historical builds and saved bundles.
const releaseChoice=document.getElementById('release-choice');
const releaseStatus=document.getElementById('release-status');
const releaseSelected=document.getElementById('release-selected');
const releaseAll=document.getElementById('release-all-panel');
const releaseList=document.getElementById('release-list');
const releaseCurrent=document.getElementById('release-current');
const releaseBuild=document.getElementById('release-build');
const releasePageEl=document.getElementById('release-page');
let releaseOptions=new Map(), selectedRelease=null,releasePage=1,allPageLoaded=false;
function formatReleaseDate(s){if(!s)return 'unknown date'; const d=new Date(s);return Number.isNaN(d.valueOf())?s:d.toLocaleDateString('en-GB');}
function showSelectedRelease(item){
  selectedRelease=item||null;
  releaseBuild.disabled=!item || !toolchainReady;
  releaseSelected.replaceChildren();
  if(!item){releaseSelected.textContent='No release selected.';return;}
  const h=document.createElement('strong');h.textContent=`${item.tag} · ${item.name}`;releaseSelected.append(h);
  const meta=document.createElement('p');meta.className='minor';meta.textContent=`Published: ${formatReleaseDate(item.published_at)}${item.prerelease?' · Prerelease':''}`;releaseSelected.append(meta);
  const detail=document.createElement('details');const summary=document.createElement('summary');summary.textContent='Changes and release notes';detail.append(summary);
  const notes=document.createElement('pre');notes.className='release-notes';notes.textContent=item.notes||'No release notes available.';detail.append(notes);releaseSelected.append(detail);
}
function optionRelease(item){releaseOptions.set(item.id,item);const opt=document.createElement('option');opt.value=String(item.id);opt.textContent=`${item.tag} · ${formatReleaseDate(item.published_at)}`;return opt;}
async function refreshRecent(){
  releaseStatus.textContent='Loading GitHub Releases…';
  try{
    const version=releaseCurrent.value.trim();
    const r=await api('api/releases?scope=recent&current='+encodeURIComponent(version));
    releaseChoice.replaceChildren();releaseOptions=new Map();
    const empty=document.createElement('option');empty.value='';empty.textContent=r.recent?.length?'Select an earlier version':'No earlier releases found';releaseChoice.append(empty);
    for(const x of (r.recent||[]))releaseChoice.append(optionRelease(x));
    showSelectedRelease(null);
    releaseStatus.textContent=r.configured?`Comparing from ${r.current||'latest version'}. ${r.recent.length} previous release(s) found.`:'Save your GitHub repository first.';
  }catch(e){releaseStatus.textContent=e.message;}
}
releaseChoice.addEventListener('change',()=>showSelectedRelease(releaseOptions.get(Number(releaseChoice.value))));
document.getElementById('release-refresh').addEventListener('click',refreshRecent);
document.getElementById('release-read-device').addEventListener('click',async()=>{
  const ip=otaIp.value.trim();if(!ip){releaseStatus.textContent='Enter the RFLink IP address under Install firmware first.';return;}
  try{
    const r=await api('api/releases/device',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({ip})});
    const match=(r.version||'').match(/v?\d+\.\d+(?:\.\d+)?/i);
    if(!match)throw new Error('Could not parse the RFLink version: '+r.version);
    releaseCurrent.value=match[0];await refreshRecent();
  }catch(e){releaseStatus.textContent=e.message;}
});
function renderFullReleases(result){
  releaseList.replaceChildren();
  for(const item of result.releases||[]){
    const row=document.createElement('div');row.className='release-row';
    const body=document.createElement('div');body.className='release-row-description';
    const head=document.createElement('strong');head.textContent=item.tag+' · '+item.name;body.append(head);
    const date=document.createElement('p');date.className='minor';date.textContent=formatReleaseDate(item.published_at)+(item.prerelease?' · Prerelease':'');body.append(date);
    const d=document.createElement('details');const summary=document.createElement('summary');summary.textContent='Release notes';d.append(summary);
    const notes=document.createElement('pre');notes.className='release-notes';notes.textContent=item.notes||'No release notes';d.append(notes);body.append(d);
    const select=document.createElement('button');select.className='secondary';select.type='button';select.textContent='Select';
    select.addEventListener('click',()=>{showSelectedRelease(item);releaseChoice.value='';releaseStatus.textContent=`${item.tag} selected from all releases.`;});
    row.append(body,select);releaseList.append(row);
  }
  if(!result.releases?.length){const empty=document.createElement('p');empty.className='minor';empty.textContent='No releases on this page.';releaseList.append(empty);}
  releasePageEl.textContent='Page '+releasePage;
  document.getElementById('release-prev').disabled=releasePage<=1;
  document.getElementById('release-next').disabled=!result.has_more;
}
async function loadFullReleases(){
  releaseStatus.textContent='Loading all GitHub Releases…';
  try{const r=await api('api/releases?scope=all&page='+releasePage);renderFullReleases(r);releaseStatus.textContent='All published releases, including newer versions and prereleases.';}
  catch(e){releaseStatus.textContent=e.message;}
}
document.getElementById('release-all-btn').addEventListener('click',async()=>{
  releaseAll.hidden=!releaseAll.hidden;
  if(!releaseAll.hidden && !allPageLoaded){allPageLoaded=true;await loadFullReleases();}
});
document.getElementById('release-prev').addEventListener('click',async()=>{if(releasePage>1){releasePage--;await loadFullReleases();}});
document.getElementById('release-next').addEventListener('click',async()=>{if(releasePage<20){releasePage++;await loadFullReleases();}});
releaseBuild.addEventListener('click',async()=>{
  const item=selectedRelease;if(!item)return;
  if(!confirm(`GitHub release ${item.tag} again using your LOCAL pinout/SPI settings? The current source remains unchanged.`))return;
  releaseBuild.disabled=true;
  try{
    await api('api/build/release',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({tag:item.tag,release_id:item.id})});
    releaseStatus.textContent=`Build of ${item.tag} started. See the build log below.`;
    logOffset=0;await refresh();
  }catch(e){releaseStatus.textContent=e.message;}
  finally{releaseBuild.disabled=false;}
});
const savedSelect=document.getElementById('saved-build-choice');
const savedStatus=document.getElementById('saved-build-status');
async function loadSavedBuilds(){
  try{const r=await api('api/releases/built');savedSelect.replaceChildren();
    const blank=document.createElement('option');blank.value='';blank.textContent='Choose a previous build';savedSelect.append(blank);
    for(const b of r.builds){const opt=document.createElement('option');opt.value=b.job_id;opt.textContent=`${b.release_tag||'Current source'} · ${formatReleaseDate(b.created_at)} · ${b.job_id}`;savedSelect.append(opt);}
    savedStatus.textContent=`${r.builds.length} validated local build(s) saved.`;
  }catch(e){savedStatus.textContent=e.message;}
}
document.getElementById('saved-build-refresh').addEventListener('click',loadSavedBuilds);
document.getElementById('saved-build-use').addEventListener('click',async()=>{
  if(!savedSelect.value)return;
  try{const r=await api('api/releases/use',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({job_id:savedSelect.value})});
    savedStatus.textContent='Build '+r.build.job_id+' selected for download/OTA/USB. Verify the target device first.';
    await refresh();await deploymentRefresh();
  }catch(e){savedStatus.textContent=e.message;}
});
loadSavedBuilds();

// Use the configured GitHub repository to populate the five previous releases.
refreshRecent();


// RFLink discovery, device-specific credentials and explicit password visibility.
const deviceKeySelect = document.getElementById('device-key-select');
const deviceKeyInput = document.getElementById('device-key');
const deviceKeyStatus = document.getElementById('device-key-status');
const deviceSettingsStatus = document.getElementById('device-settings-status');
const deviceNetwork = document.getElementById('device-network');
const otaInfo = document.getElementById('ota-device-info');
const otaKeyStatus = document.getElementById('ota-key-status');
let discoveredDevices = [], discoveryBusy = false;
function selectOptions(select, items, previous, emptyLabel) {
  select.replaceChildren();
  const blank=document.createElement('option'); blank.value='';blank.textContent=emptyLabel;select.append(blank);
  for (const device of items) {
    const option=document.createElement('option');
    option.value=device.id;
    option.textContent=device.name+' · '+device.ip+(device.online?' · online':' · last seen');
    select.append(option);
  }
  if (items.some(d=>d.id===previous)) select.value=previous;
}
function deviceById(id){return discoveredDevices.find(d=>d.id===id);}
function syncSelectedDevice(){
  const ota=deviceById(otaDevice.value);
  otaIp.value=ota?.ip||'';
  otaKeyAvailable=!!ota?.has_key;
  otaInfo.textContent=ota
    ? (ota.online?'Available':'Not found in last scan')+' · '+ota.firmware+' · '+ota.ip
    : 'Choose a discovered RFLink to install firmware.';
  otaKeyStatus.textContent=ota
    ? (ota.has_key?'OTA password saved for this RFLink.':'No OTA password: save one in Settings.')
    : 'Device-specific passwords are configured in Settings.';
  const keyDevice=deviceById(deviceKeySelect.value);
  deviceKeyStatus.textContent=keyDevice
    ? (keyDevice.has_key?'Password saved for this RFLink.':'No password saved yet.')
    : 'Choose a RFLink to manage its password.';
  enableDeployment();
}
async function refreshDevices(){
  try {
    const result=await api('api/devices');
    discoveryBusy=!!result.scanning;
    discoveredDevices=result.devices||[];
    if(typeof renderDashboard === 'function') renderDashboard();
    const beforeOta=otaDevice.value, beforeKey=deviceKeySelect.value;
    selectOptions(otaDevice,discoveredDevices.filter(d=>d.managed),beforeOta,'Choose RFLink');
    selectOptions(deviceKeySelect,discoveredDevices.filter(d=>d.managed),beforeKey,'Choose RFLink');
    const managedDevices=discoveredDevices.filter(d=>d.managed);
    if (!otaDevice.value && managedDevices.length === 1) otaDevice.value=managedDevices[0].id;
    if (!deviceKeySelect.value && otaDevice.value) deviceKeySelect.value=otaDevice.value;
    deviceNetwork.value=result.network||deviceNetwork.value;
    deviceSettingsStatus.textContent=result.scanning?'Scanning local networks…':
      (result.error || discoveredDevices.filter(d=>d.online).length+' RFLink(s) found');
    syncSelectedDevice();
  } catch(e){deviceSettingsStatus.textContent=e.message;otaInfo.textContent=e.message;}
}
async function deviceRequest(action, body){
  return await api('api/devices/'+action,{
    method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(body||{})
  });
}
async function rescanDevices(){
  try{await deviceRequest('discover',{});await refreshDevices();}
  catch(e){deviceSettingsStatus.textContent=e.message;}
}
document.getElementById('ota-scan').addEventListener('click',rescanDevices);
document.getElementById('device-rescan-settings').addEventListener('click',rescanDevices);
otaDevice.addEventListener('change',()=>{
  if (!deviceKeySelect.value) deviceKeySelect.value=otaDevice.value;
  syncSelectedDevice();
});
deviceKeySelect.addEventListener('change',()=>{
  deviceKeyInput.value='';deviceKeyInput.type='password';
  syncSelectedDevice();
});
document.getElementById('ota-open-settings').addEventListener('click',()=>{
  document.querySelector('.nav-tab[data-page="settings"]').click();
  document.getElementById('devices-panel').open=true;
  if(otaDevice.value) deviceKeySelect.value=otaDevice.value;
  syncSelectedDevice();
  document.getElementById('devices-panel').scrollIntoView({behavior:'smooth',block:'start'});
});
document.getElementById('device-network-save').addEventListener('click',async()=>{
  try{await deviceRequest('network',{network:deviceNetwork.value.trim()});await rescanDevices();}
  catch(e){deviceSettingsStatus.textContent=e.message;}
});
document.getElementById('device-add-btn').addEventListener('click',async()=>{
  const ip=document.getElementById('device-add-ip').value.trim();
  try{
    const r=await deviceRequest('add',{ip});
    await refreshDevices();
    const found=r.devices?.find(d=>d.ip===ip);
    if(found){otaDevice.value=found.id;deviceKeySelect.value=found.id;syncSelectedDevice();}
  }catch(e){deviceSettingsStatus.textContent=e.message;}
});
document.getElementById('device-key-save').addEventListener('click',async()=>{
  if(!deviceKeySelect.value){deviceKeyStatus.textContent='Choose RFLink first.';return;}
  try{
    await deviceRequest('key/save',{device_id:deviceKeySelect.value,key:deviceKeyInput.value});
    deviceKeyInput.value='';deviceKeyInput.type='password';
    await refreshDevices();
    deviceKeyStatus.textContent='Saved only for the selected RFLink.';
  }catch(e){deviceKeyStatus.textContent=e.message;}
});
document.getElementById('device-key-delete').addEventListener('click',async()=>{
  if(!deviceKeySelect.value)return;
  if(!confirm('Remove the saved OTA password for this RFLink?'))return;
  try{await deviceRequest('key/delete',{device_id:deviceKeySelect.value});
    deviceKeyInput.value='';await refreshDevices();deviceKeyStatus.textContent='Password removed.';
  }catch(e){deviceKeyStatus.textContent=e.message;}
});
const eyeVisibleIcon='<svg viewBox="0 0 24 24" width="18" height="18" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><path d="M2 12s3.5-7 10-7 10 7 10 7-3.5 7-10 7-10-7-10-7Z"/><circle cx="12" cy="12" r="3"/></svg>';
const eyeHiddenIcon='<svg viewBox="0 0 24 24" width="18" height="18" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><path d="M10 5.3A11 11 0 0 1 12 5c6.5 0 10 7 10 7a13 13 0 0 1-3.2 4.1M6.8 6.8C3.6 9 2 12 2 12s3.5 7 10 7a10.8 10.8 0 0 0 4.8-1.1"/><path d="M3 3l18 18"/><path d="M9.9 9.9a3 3 0 0 0 4.2 4.2"/></svg>';
// Only user-initiated eye clicks may request a stored password. A normal GET
// never returns secrets, and no eye control exposes other device credentials.
for (const input of document.querySelectorAll('input[type="password"]')){
  const wrap=document.createElement('span');
  wrap.className='password-entry';
  input.parentElement.insertBefore(wrap,input);
  wrap.append(input);
  const eye=document.createElement('button');
  eye.className='eye-button';eye.type='button';eye.setAttribute('aria-label','Show password');
  eye.setAttribute('aria-pressed','false');eye.innerHTML=eyeVisibleIcon;
  eye.addEventListener('click',async()=>{
    if(input.type==='text'){
      input.type='password';eye.innerHTML=eyeVisibleIcon;eye.setAttribute('aria-pressed','false');eye.setAttribute('aria-label','Show password');
      if(input.dataset.revealedFromStorage==='true'){
        input.value='';delete input.dataset.revealedFromStorage;
      }
      return;
    }
    if(!input.value){
      try{
        let secret=null;
        if(input.id==='secret-pass') secret=await api('api/secrets/reveal',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({field:'wifi_password'})});
        else if(input.id==='secret-ota') secret=await api('api/secrets/reveal',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({field:'ota_key'})});
        else if(input.id==='mqtt-password') secret=await api('api/mqtt/reveal',{method:'POST',headers:{'Content-Type':'application/json'},body:'{}'});
        else if(input.id==='device-key' && deviceKeySelect.value) secret=await deviceRequest('key/reveal',{device_id:deviceKeySelect.value});
        if(secret?.value){
          input.value=secret.value;input.dataset.revealedFromStorage='true';
        }
      }catch(e){
        if(input.id==='device-key')deviceKeyStatus.textContent=e.message;
        else if(input.id.startsWith('secret-'))document.getElementById('secrets-status').textContent=e.message;
        else if(input.id==='mqtt-password')mqttStatus.textContent=e.message;
      }
    }
    input.type='text';eye.innerHTML=eyeHiddenIcon;eye.setAttribute('aria-label','Hide password');eye.setAttribute('aria-pressed','true');
  });
  wrap.append(eye);
}
refreshDevices().then(rescanDevices);
setInterval(()=>{if(discoveryBusy)refreshDevices();},1800);
