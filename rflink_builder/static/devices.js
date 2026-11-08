/* RFLink Builder — device-first dashboard and per-device configuration.
 * Native statuses are verified by the Builder, never presumed from GitHub.
 * Firmware installation keeps the test-board and board-hash safety gates.
 */
const dashCards=document.getElementById('device-cards');
const dashFound=document.getElementById('discovered-cards');
const dashSearch=document.getElementById('dashboard-search');
const dashStatus=document.getElementById('dashboard-status');
let discoveryScope='192.168.178.0/24 + 192.168.1.0/24';
const foundWizard=document.getElementById('wizard-found');
const addWizard=document.getElementById('device-add-dialog');
const workspace=document.querySelector('.device-workspace');
const detailContent=document.getElementById('detail-content');
const detailKey=document.getElementById('detail-key');
const detailEye=document.getElementById('detail-key-eye');
let detailDeviceId='';
let latestFirmwareRelease=null;
let compatibleBuild=null;
let revealedFromStorage=false;

const eyeVisible='<svg viewBox="0 0 24 24" width="18" height="18" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><path d="M2 12s3.5-7 10-7 10 7 10 7-3.5 7-10 7-10-7-10-7Z"/><circle cx="12" cy="12" r="3"/></svg>';
const eyeHidden='<svg viewBox="0 0 24 24" width="18" height="18" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><path d="M3 3l18 18M10 5.2A11 11 0 0 1 12 5c6.5 0 10 7 10 7a13 13 0 0 1-3 4M6.6 6.6C3.4 8.9 2 12 2 12s3.5 7 10 7c1.5 0 2.8-.4 4.1-1"/></svg>';

function uiElement(tag,klass,txt){
  const el=document.createElement(tag);
  if(klass)el.className=klass;
  if(txt!==undefined)el.textContent=String(txt);
  return el;
}
function uiAction(label,klass,callback){
  const el=uiElement('button',klass,label);
  el.type='button';el.addEventListener('click',callback);
  return el;
}
function fwVersion(value){
  // Native firmware strings contain a board signature after the semver.
  const m=String(value||'').match(/^v?(\d+)\.(\d+)\.(\d+)(?:$|[-+])/i);
  return m?m.slice(1,4).map(Number):null;
}
function newerPublishedRelease(d){
  const current=fwVersion(d?.firmware);
  const latest=fwVersion(latestFirmwareRelease?.tag);
  if(!current||!latest)return false;
  for(let i=0;i<3;i++){
    if(latest[i]>current[i])return true;
    if(latest[i]<current[i])return false;
  }
  return false;
}
function lastSeen(d){
  if(!d?.last_seen)return 'Not seen yet';
  const time=new Date(d.last_seen);
  return Number.isNaN(time.valueOf())?'Unknown':time.toLocaleString();
}
function navigate(page){
  document.querySelector('.nav-tab[data-page="'+page+'"]').click();
  document.getElementById('header-home').hidden=page==='devices';
  window.scrollTo({top:0,behavior:'auto'});
}
function hideDetailPassword(){
  detailKey.value='';
  detailKey.type='password';
  revealedFromStorage=false;
  detailEye.innerHTML=eyeVisible;
  detailEye.setAttribute('aria-label','Show saved OTA password');
  detailEye.setAttribute('aria-pressed','false');
}
function selectDetailTab(tab){
  for(const button of document.querySelectorAll('.detail-tab')){
    const active=button.dataset.detailTab===tab;
    button.classList.toggle('selected',active);
    button.setAttribute('aria-selected',String(active));
  }
  for(const section of document.querySelectorAll('[data-detail-section]')){
    section.classList.toggle('active-detail-tab',section.dataset.detailSection===tab);
  }
}
function currentDetailDevice(){return discoveredDevices.find(d=>d.id===detailDeviceId&&d.managed);}
function canDeploy(d){
  return !!(d?.online && d?.has_key && compatibleBuild &&
            compatibleBuild.board_id===d.board_id &&
            compatibleBuild.board_profile_hash===d.board_hash &&
            (!d.configured || (compatibleBuild.builder_device_id===d.id &&
                compatibleBuild.device_config_revision===d.config_revision)));
}
function detailReleaseText(d){
  if(d?.expected_physical_id){
    return d.discovery_hint==='expected_version_mismatch'
      ? 'Correct ESP32 ID found, but it runs '+(d.observed_firmware||'another firmware')+
        '. Expected '+d.expected_firmware_version+'. Open diagnosis to investigate.'
      : 'Firmware written. Waiting for ESP32 '+d.expected_physical_id+
        ' to come online with '+d.expected_firmware_version+'. No manual link is needed.';
  }
  if(newerPublishedRelease(d)){
    return 'A newer release ('+latestFirmwareRelease.tag+') is published. A matching firmware build is still required.';
  }
  if(d?.configured && !d.provisioned)return 'This saved RFLink is not linked to hardware. If already flashed, link the detected device below; otherwise build and install via USB.';
  return latestFirmwareRelease?'Current firmware: '+(d.firmware||'unknown')+'. No verified newer update available.':
    'Release status is unknown. No update has been verified.';
}
function showDeviceDetail(id, tab='overview'){
  const d=discoveredDevices.find(entry=>entry.id===id&&entry.managed);
  if(!d)return;
  const changed=id!==detailDeviceId;
  detailDeviceId=id;
  if(typeof updateDeviceConfigActions==='function')updateDeviceConfigActions(d);
  detailContent.hidden=false;
  document.getElementById('detail-placeholder').hidden=true;
  workspace.classList.add('device-open');
  if(changed)hideDetailPassword();
  document.getElementById('detail-title').textContent=d.name||'RFLink';
  const badge=document.getElementById('detail-online');
  badge.className='device-badge '+(d.online?'online':d.expected_physical_id?'waiting':'offline');
  badge.textContent=d.online?'● Online':d.expected_physical_id?'◌ Waiting for ESP32':
    (d.configured&&!d.provisioned?'○ Not linked':'● Offline');
  document.getElementById('detail-address').textContent=d.ip||'IP unknown';
  document.getElementById('detail-version').textContent=d.firmware||'Firmware unknown';
  const name=document.getElementById('detail-name');
  if(changed || document.activeElement!==name)name.value=d.name||'';
  document.getElementById('detail-key-status').textContent=d.has_key?'Password saved for this RFLink':'No OTA password saved';
  document.getElementById('detail-wifi').textContent=d.expected_physical_id&&!d.online
    ? '◌ Waiting for first Wi-Fi report from the flashed ESP32'
    : !d.online?'● Offline':(d.wifi_connected?'● Connected':'Reachable · Wi-Fi status unknown');
  document.getElementById('detail-last-seen').textContent=d.online?'Just now':
    d.expected_physical_id?'Waiting for ESP32 first contact':lastSeen(d);
  document.getElementById('detail-ip').textContent=d.ip||'Unknown';
  document.getElementById('detail-connectivity').textContent=d.online?'Online':'Offline';
  const pairingHint=d.expected_physical_id
    ? (d.discovery_hint==='expected_version_mismatch'
      ? 'The expected ESP32 was detected, but its running firmware differs from the flashed image.'
      : 'Auto-link armed for '+d.expected_physical_id+'. Waiting for it to announce the selected firmware.')
    : d.discovery_hint==='link_required'
    ? 'The RFLink answers at this IP, but has not been linked to this saved configuration. Scan and link the detected board.'
    :d.discovery_hint==='identity_mismatch'
      ? 'A different RFLink identity answered at the saved IP. Connection is blocked until you verify the physical device.'
      :'';
  document.getElementById('detail-identity-note').textContent=pairingHint ||
    (d.identity_verified
      ? 'Stable RFLink identity recognized.'
      : 'Legacy firmware: this RFLink is identified by its IP. Reserve its DHCP address.');
  document.getElementById('detail-board').textContent=d.board_id||'Unknown';
  document.getElementById('detail-identity-kind').textContent=d.identity_verified?'Stable device ID':'Legacy IP-bound';
  document.getElementById('detail-update-summary').textContent=detailReleaseText(d);
  document.getElementById('detail-firmware-status').textContent=detailReleaseText(d);
  // The device-scoped firmware controller owns install readiness and saved
  // build selection; never use a global build from another RFLink by mistake.
  if(typeof updateDeviceFirmwarePanel==='function')updateDeviceFirmwarePanel(d);
  if(typeof updateFirmwareCatalog==='function')updateFirmwareCatalog(d);
  document.querySelectorAll('.device-list-item').forEach(card=>
    card.classList.toggle('is-selected',card.dataset.deviceId===id));
  if(changed)document.getElementById('detail-feedback').textContent='';
  selectDetailTab(tab);
}
function closeMobileDetail(){
  workspace.classList.remove('device-open');
  hideDetailPassword();
  document.getElementById('detail-feedback').textContent='';
}
function openDeviceSite(id){
  const d=discoveredDevices.find(x=>x.id===id);
  if(!d?.online||!/^(\d{1,3}\.){3}\d{1,3}$/.test(d.ip||''))return;
  window.open('http://'+d.ip+'/','_blank','noopener,noreferrer');
}
function renderDashboard(){
  const managed=discoveredDevices.filter(d=>d.managed);
  const found=discoveredDevices.filter(d=>!d.managed&&d.online&&
    !managed.some(x=>x.expected_physical_id===d.id));
  const query=dashSearch.value.trim().toLowerCase();
  const shown=managed.filter(d=>(d.name+' '+d.ip+' '+d.firmware).toLowerCase().includes(query));
  dashCards.replaceChildren();
  for(const d of shown){
    const row=uiElement('article','device-list-item');
    row.dataset.deviceId=d.id;
    row.tabIndex=0;row.setAttribute('role','button');
    row.setAttribute('aria-label','Manage '+d.name);
    row.setAttribute('aria-current',String(d.id===detailDeviceId));
    if(d.id===detailDeviceId)row.classList.add('is-selected');
    const glyph=uiElement('div','device-list-icon','⌁');
    const content=uiElement('div','device-list-copy');
    content.append(uiElement('strong','',d.name),uiElement('small','',d.ip||'IP unavailable'),
                   uiElement('small','',d.expected_physical_id
                     ? 'Waiting for '+d.expected_firmware_version : d.firmware||'Firmware unknown'));
    const actions=uiElement('div','device-list-end');
    actions.append(uiElement('span','device-badge '+(d.online?'online':d.expected_physical_id?'waiting':'offline'),
      d.online?'● Online':d.expected_physical_id?'◌ Waiting':
      (d.configured&&!d.provisioned?'○ Not linked':'● Offline')));
    if(newerPublishedRelease(d))actions.append(uiElement('span','device-new-release','New release'));
    actions.append(uiElement('span','device-list-arrow','›'));
    row.append(glyph,content,actions);
    row.addEventListener('click',()=>showDeviceDetail(d.id));
    row.addEventListener('keydown',e=>{
      if(e.key==='Enter'||e.key===' '){e.preventDefault();showDeviceDetail(d.id);}
    });
    dashCards.append(row);
  }
  document.getElementById('device-empty').hidden=managed.length>0;
  const onlineCount=managed.filter(d=>d.online).length;
  dashStatus.textContent=discoveryBusy?'Scanning '+discoveryScope+' for RFLinks…':
    managed.length? shown.length+' of '+managed.length+' devices · '+onlineCount+' online'
      +(onlineCount===0 && managed.some(d=>d.expected_physical_id)
        ? ' · Waiting for flashed ESP32 to come online'
        : onlineCount===0?' · Check IP or link detected device':'')
    :'No devices added yet · Scan '+discoveryScope+' or enter an IP';
  const section=document.getElementById('discovered-section');
  section.hidden=found.length===0;
  dashFound.replaceChildren();
  for(const d of found){
    const item=uiElement('div','discovered-item');
    item.append(uiElement('span','device-list-icon','⌁'));
    const name=uiElement('div','discovered-copy');
    name.append(uiElement('strong','',d.name),uiElement('small','',d.ip));
    // Browser USB flashes cannot mark a board as physically installed until
    // it is discovered. An earlier Builder may have saved 'provisioned:false'
    // even though this exact RFLink is already running on the LAN.
    const candidates=managed.filter(x=>x.configured&&!x.physical_id&&
      x.board_id===d.board_id&&x.board_hash===d.board_hash);
    async function linkDiscovered(target){
      const identity=d.identity_verified?'verified ESP32 ID':'legacy IP-based identity';
      const note='Link the detected RFLink at '+d.ip+' ('+identity+', firmware '+
        (d.firmware||'unknown')+') to your saved '+target.name+' configuration?\n\n'+
        'Only continue if this is the ESP32 you flashed. Matching board pins do not '+ 
        'verify the stored Wi-Fi/OTA password, so OTA may need attention afterwards. '+ 
        'The saved device name, Wi-Fi, OTA password and firmware choice will be retained.';
      if(!confirm(note))return;
      try{
        await deviceRequest('link',{config_id:target.id,discovered_id:d.id});
        await refreshDevices();showDeviceDetail(target.id,'connection');
      }catch(e){dashStatus.textContent='Could not link RFLink: '+e.message;}
    }
    if(candidates.length===1){
      item.append(name,uiAction('Link to '+candidates[0].name,'secondary',
        ()=>linkDiscovered(candidates[0])));
    }else if(candidates.length>1){
      // Several saved boards share the same pinout: force an explicit choice
      // instead of guessing or offering an unnecessary second Add device.
      const selection=document.createElement('select');
      selection.setAttribute('aria-label','Saved RFLink configuration to link');
      const placeholder=document.createElement('option');
      placeholder.value='';placeholder.textContent='Choose saved RFLink';
      selection.append(placeholder);
      for(const candidate of candidates){
        const option=document.createElement('option');
        option.value=candidate.id;option.textContent=candidate.name;
        selection.append(option);
      }
      item.append(name,selection,uiAction('Link','secondary',()=>{
        const target=candidates.find(x=>x.id===selection.value);
        if(target)linkDiscovered(target);
      }));
    }else item.append(name,uiAction('Add','secondary',()=>{showAddWizard();chooseFound(d.id);}));
    dashFound.append(item);
  }
  renderFoundWizard();
  // Start with the device list on mobile; auto-open the first device on desktop.
  if(!detailDeviceId && managed.length && window.matchMedia('(min-width: 880px)').matches){
    showDeviceDetail(managed[0].id);
  } else if(detailDeviceId){
    const existing=managed.find(d=>d.id===detailDeviceId);
    if(existing)showDeviceDetail(existing.id,document.querySelector('.detail-tab.selected')?.dataset.detailTab||'overview');
    else {
      detailDeviceId='';detailContent.hidden=true;
      workspace.classList.remove('device-open');
      document.getElementById('detail-placeholder').hidden=false;
      hideDetailPassword();
    }
  }
}
function showAddWizard(){
  renderFoundWizard();
  if(typeof beginWizard==='function')beginWizard();
  document.getElementById('wizard-status').textContent='';
  addWizard.showModal();
}
function renderFoundWizard(){
  foundWizard.replaceChildren();
  const available=discoveredDevices.filter(d=>!d.managed&&d.online&&
    !discoveredDevices.some(x=>x.managed && x.expected_physical_id===d.id));
  if(!available.length)foundWizard.append(uiElement('p','minor','No new RFLinks detected yet. Search again or add by IP.'));
  for(const d of available){
    const item=uiElement('div','wizard-item');
    const label=uiElement('div','');
    label.append(uiElement('strong','',d.name),uiElement('small','',d.ip+' · '+d.firmware));
    item.append(label,uiAction('Add','secondary',()=>chooseFound(d.id)));
    foundWizard.append(item);
  }
}
function chooseFound(id){
  const d=discoveredDevices.find(x=>x.id===id);
  if(!d)return;
  foundWizard.replaceChildren();
  foundWizard.append(uiElement('p','minor','Name your RFLink '+d.ip));
  const name=uiElement('input','');name.maxLength=48;name.value='RFLink';name.placeholder='e.g. RFLink Living Room';
  const row=uiElement('div','flex-input');
  row.append(name,uiAction('Add RFLink','primary',async()=>{
    try{
      await deviceRequest('adopt',{device_id:id,name:name.value});
      await refreshDevices();addWizard.close();
      showDeviceDetail(id);
    }catch(e){document.getElementById('wizard-status').textContent=e.message;}
  }));
  foundWizard.append(row,uiAction('Back','text-button',renderFoundWizard));
  name.focus();
}
async function refreshBuildCompatibility(){
  try{
    const status=await api('api/status');
    compatibleBuild=(status.status==='success' && status.ota_ready)
      ? await api('api/download/manifest'):null;
  }catch{compatibleBuild=null;}
  if(detailDeviceId)showDeviceDetail(detailDeviceId,document.querySelector('.detail-tab.selected')?.dataset.detailTab||'overview');
}
async function refreshLatestRelease(){
  try{
    const r=await api('api/releases?scope=recent');
    latestFirmwareRelease=r.latest||null;
  }catch{latestFirmwareRelease=null;}
  renderDashboard();
}
function showReleasePage(){
  const d=currentDetailDevice();
  if(d){otaDevice.value=d.id;syncSelectedDevice();}
  navigate('versions');
  document.getElementById('release-history').scrollIntoView({block:'start',behavior:'smooth'});
}

document.getElementById('dashboard-add').addEventListener('click',showAddWizard);
document.getElementById('dashboard-empty-add').addEventListener('click',showAddWizard);
document.getElementById('dashboard-refresh').addEventListener('click',rescanDevices);
dashSearch.addEventListener('input',renderDashboard);
document.getElementById('detail-back').addEventListener('click',closeMobileDetail);
for(const b of document.querySelectorAll('.detail-tab')){
  b.addEventListener('click',()=>{selectDetailTab(b.dataset.detailTab);hideDetailPassword();});
}
document.getElementById('detail-overview-advanced').addEventListener('click',()=>selectDetailTab('advanced'));
document.getElementById('detail-rename').addEventListener('click',async()=>{
  if(!currentDetailDevice())return;
  try{
    await deviceRequest('rename',{device_id:detailDeviceId,name:document.getElementById('detail-name').value});
    await refreshDevices();document.getElementById('detail-feedback').textContent='Name saved.';
  }catch(e){document.getElementById('detail-feedback').textContent=e.message;}
});
detailEye.innerHTML=eyeVisible;
detailEye.addEventListener('click',async()=>{
  if(detailKey.type==='text'){
    detailKey.type='password';
    detailEye.innerHTML=eyeVisible;
    detailEye.setAttribute('aria-label','Show saved OTA password');
    detailEye.setAttribute('aria-pressed','false');
    if(revealedFromStorage){detailKey.value='';revealedFromStorage=false;}
    return;
  }
  if(!detailKey.value && currentDetailDevice()?.has_key){
    try{
      const result=await deviceRequest('key/reveal',{device_id:detailDeviceId});
      if(result.value){detailKey.value=result.value;revealedFromStorage=true;}
    }catch(e){document.getElementById('detail-key-status').textContent=e.message;return;}
  }
  detailKey.type='text';
  detailEye.innerHTML=eyeHidden;
  detailEye.setAttribute('aria-label','Hide OTA password');
  detailEye.setAttribute('aria-pressed','true');
});
document.getElementById('detail-key-save').addEventListener('click',async()=>{
  if(!currentDetailDevice())return;
  try{
    await deviceRequest('key/save',{device_id:detailDeviceId,key:detailKey.value});
    hideDetailPassword();await refreshDevices();
    document.getElementById('detail-key-status').textContent='Password saved for this device.';
  }catch(e){document.getElementById('detail-key-status').textContent=e.message;}
});
document.getElementById('detail-open').addEventListener('click',()=>openDeviceSite(detailDeviceId));
// Build, OTA, USB and download actions are bound in device_firmware.js.
document.getElementById('detail-advanced-build').addEventListener('click',()=>navigate('settings'));
document.getElementById('detail-releases').addEventListener('click',showReleasePage);
document.getElementById('detail-advanced-releases').addEventListener('click',showReleasePage);
document.getElementById('detail-remove').addEventListener('click',async()=>{
  const d=currentDetailDevice();
  if(!d || !confirm('Remove '+d.name+' and its stored OTA password from Builder?'))return;
  try{
    await deviceRequest('remove',{device_id:d.id});
    detailDeviceId='';closeMobileDetail();await refreshDevices();
  }catch(e){document.getElementById('detail-feedback').textContent=e.message;}
});
document.getElementById('wizard-rescan').addEventListener('click',async()=>{
  await rescanDevices();
  document.getElementById('wizard-status').textContent='Searching for RFLinks…';
});
document.getElementById('wizard-manual-add').addEventListener('click',async()=>{
  const ip=document.getElementById('wizard-manual-ip').value.trim();
  try{
    const result=await deviceRequest('add',{ip});
    await refreshDevices();
    const device=result.devices.find(d=>d.ip===ip);
    if(device){addWizard.close();showDeviceDetail(device.id);}
  }catch(e){document.getElementById('wizard-status').textContent=e.message;}
});

for(const b of document.querySelectorAll('[data-close-dialog]')){
  b.addEventListener('click',()=>document.getElementById(b.dataset.closeDialog).close());
}
addWizard.addEventListener('click',e=>{if(e.target===addWizard)addWizard.close();});
document.getElementById('header-settings').addEventListener('click',()=>navigate('settings'));
document.getElementById('header-home').addEventListener('click',()=>{closeMobileDetail();navigate('devices');});
document.getElementById('advanced-open-build').addEventListener('click',()=>navigate('build'));
document.getElementById('advanced-open-versions').addEventListener('click',showReleasePage);
for(const tab of document.querySelectorAll('.nav-tab')){
  tab.addEventListener('click',()=>{
    document.getElementById('header-home').hidden=tab.dataset.page==='devices';
    if(tab.dataset.page!=='devices')hideDetailPassword();
  });
}
refreshBuildCompatibility();
refreshLatestRelease();
setInterval(refreshBuildCompatibility,15000);
setInterval(refreshLatestRelease,300000);
setInterval(refreshDevices,30000);
