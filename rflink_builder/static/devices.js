/* RFLink device dashboard. Existing app.js owns the authenticated API and the
 * device list. Never assume a GitHub release is a tested OTA update.
 */
const dashCards=document.getElementById('device-cards');
const dashFound=document.getElementById('discovered-cards');
const dashSearch=document.getElementById('dashboard-search');
const dashFilter=document.getElementById('dashboard-filter');
const dashStatus=document.getElementById('dashboard-status');
const foundWizard=document.getElementById('wizard-found');
const addWizard=document.getElementById('device-add-dialog');
const detailWizard=document.getElementById('device-detail-dialog');
let detailDeviceId='';
let latestFirmwareRelease=null;

function uiElement(tag,klass,txt){
  const el=document.createElement(tag);
  if(klass)el.className=klass;
  if(txt!==undefined)el.textContent=String(txt);
  return el;
}
function uiAction(label,cls,callback){
  const el=uiElement('button',cls,label);
  el.type='button';el.addEventListener('click',callback);
  return el;
}
function fwVersion(value){
  const m=String(value||'').match(/^v?(\d+)\.(\d+)\.(\d+)/i);
  return m?m.slice(1,4).map(Number):null;
}
function newerPublishedRelease(device){
  const current=fwVersion(device && device.firmware);
  const latest=fwVersion(latestFirmwareRelease && latestFirmwareRelease.tag);
  if(!current||!latest)return false;
  for(let i=0;i<3;i++){
    if(latest[i]>current[i])return true;
    if(latest[i]<current[i])return false;
  }
  return false;
}
function lastSeen(device){
  if(!device.last_seen)return 'Not seen yet';
  const d=new Date(device.last_seen);
  return Number.isNaN(d.valueOf())?'Last seen unknown':'Last seen '+d.toLocaleString();
}
function showAddWizard(){
  renderFoundWizard();
  addWizard.showModal();
}
function renderDashboard(){
  if(!dashCards)return;
  const managed=discoveredDevices.filter(d=>d.managed);
  const found=discoveredDevices.filter(d=>!d.managed&&d.online);
  const search=dashSearch.value.trim().toLowerCase();
  const matches=d=>{
    if(search&&!((d.name||'')+' '+(d.ip||'')+' '+(d.firmware||'')).toLowerCase().includes(search))return false;
    if(dashFilter.value==='online')return !!d.online;
    if(dashFilter.value==='offline')return !d.online;
    if(dashFilter.value==='updates')return newerPublishedRelease(d);
    return true;
  };
  dashCards.replaceChildren();
  const filtered=managed.filter(matches);
  for(const d of filtered){
    const card=uiElement('article','device-card');
    const head=uiElement('div','device-card-head');
    const identity=uiElement('div','device-card-identity');
    identity.append(uiElement('span','device-icon','⌁'));
    const names=uiElement('div','device-card-name');
    names.append(uiElement('strong','',d.name||'RFLink'),uiElement('small','',d.ip||'IP unknown'));
    identity.append(names);
    head.append(identity,uiElement('span','device-badge '+(d.online?'online':'offline'),d.online?'Online':'Offline'));
    const meta=uiElement('div','device-card-details');
    meta.append(uiElement('span','','Firmware: '+(d.firmware||'Unknown')),
                uiElement('span','',d.board_id||'Hardware not verified'));
    if(!d.online)meta.append(uiElement('span','',lastSeen(d)));
    if(newerPublishedRelease(d)){
      meta.append(uiElement('span','newer-release','New published release '+latestFirmwareRelease.tag+' · verify build/compatibility'));
    }
    const actions=uiElement('div','device-card-actions');
    actions.append(uiAction('Manage','secondary',()=>showDeviceDetail(d.id)));
    if(d.online)actions.append(uiAction('Open','text-button',()=>openDeviceSite(d.id)));
    if(newerPublishedRelease(d))actions.append(uiAction('Review update','text-button',()=>showReleasePage(d.id)));
    card.append(head,meta,actions);dashCards.append(card);
  }
  document.getElementById('dashboard-total').textContent=managed.length;
  document.getElementById('dashboard-online').textContent=managed.filter(d=>d.online).length;
  document.getElementById('dashboard-updates').textContent=
    latestFirmwareRelease?managed.filter(newerPublishedRelease).length:'—';
  document.getElementById('device-empty').hidden=managed.length>0;
  dashStatus.textContent=discoveryBusy?'Searching for local RFLinks…':
    managed.length?filtered.length+' of '+managed.length+' devices displayed · auto status refresh enabled':
    'No managed RFLinks yet. Add an existing gateway or prepare a new test board.';
  document.getElementById('discovered-section').hidden=found.length===0;
  dashFound.replaceChildren();
  for(const d of found){
    const card=uiElement('article','device-card discovered-card');
    card.append(uiElement('strong','',d.name),
                uiElement('p','minor',d.ip+' · '+d.firmware),
                uiAction('Adopt device','secondary',()=>{showAddWizard();chooseFound(d.id);}));
    dashFound.append(card);
  }
  renderFoundWizard();
}
function renderFoundWizard(){
  foundWizard.replaceChildren();
  const newDevices=discoveredDevices.filter(d=>!d.managed&&d.online);
  if(!newDevices.length)foundWizard.append(uiElement('p','minor','No new native RFLinks found. Search the LAN or enter an IP address below.'));
  for(const d of newDevices){
    const row=uiElement('div','wizard-item');
    const info=uiElement('div','');
    info.append(uiElement('strong','',d.name),uiElement('small','',d.ip+' · '+d.firmware));
    row.append(info,uiAction('Add','secondary',()=>chooseFound(d.id)));
    foundWizard.append(row);
  }
}
function chooseFound(id){
  const d=deviceById(id);
  if(!d)return;
  foundWizard.replaceChildren();
  foundWizard.append(uiElement('p','minor','Choose a name for the RFLink at '+d.ip+'. Its OTA password remains separate.'));
  const name=uiElement('input','');
  name.maxLength=48;name.placeholder='RFLink Living Room';name.value=d.name||'RFLink';
  const row=uiElement('div','flex-input');
  row.append(name,uiAction('Add device','primary',async()=>{
    try{
      await deviceRequest('adopt',{device_id:d.id,name:name.value});
      await refreshDevices();
      addWizard.close();
      showDeviceDetail(d.id);
    }catch(error){document.getElementById('wizard-status').textContent=error.message;}
  }));
  foundWizard.append(row,uiAction('Back','text-button',renderFoundWizard));
  name.focus();
}
function openDeviceSite(id){
  const d=deviceById(id);
  if(!d||!d.online||!/^(\d{1,3}\.){3}\d{1,3}$/.test(d.ip||''))return;
  window.open('http://'+d.ip+'/','_blank','noopener,noreferrer');
}
function showDeviceDetail(id){
  const d=deviceById(id);
  if(!d||!d.managed)return;
  detailDeviceId=id;
  document.getElementById('detail-title').textContent=d.name;
  document.getElementById('detail-summary').textContent=d.online?'Online · '+d.ip:'Offline · '+lastSeen(d);
  const meta=document.getElementById('detail-metadata');meta.replaceChildren();
  for(const [label,value] of [
    ['Address',d.ip],['Firmware',d.firmware],['Board',d.board_id],
    ['OTA password',d.has_key?'Saved for this device':'Not configured'],
    ['Release',newerPublishedRelease(d)?'New release '+latestFirmwareRelease.tag+' (build required)':'No verified new update']]){
    const item=uiElement('div','detail-row');
    item.append(uiElement('span','',label),uiElement('strong','',value||'Unknown'));
    meta.append(item);
  }
  document.getElementById('detail-name').value=d.name;
  document.getElementById('detail-feedback').textContent='';
  document.getElementById('detail-open').disabled=!d.online;
  document.getElementById('detail-ota').disabled=!d.online||!d.has_key;
  if(!detailWizard.open)detailWizard.showModal();
}
function showReleasePage(id){
  const d=deviceById(id);
  if(d&&d.managed){otaDevice.value=id;syncSelectedDevice();}
  document.querySelector('.nav-tab[data-page="versions"]').click();
  document.getElementById('release-history').scrollIntoView({behavior:'smooth',block:'start'});
}
document.getElementById('dashboard-add').addEventListener('click',showAddWizard);
document.getElementById('dashboard-empty-add').addEventListener('click',showAddWizard);
document.getElementById('dashboard-refresh').addEventListener('click',async()=>{await rescanDevices();dashStatus.textContent='Searching the LAN…';});
dashSearch.addEventListener('input',renderDashboard);
dashFilter.addEventListener('change',renderDashboard);
document.getElementById('wizard-rescan').addEventListener('click',async()=>{
  await rescanDevices();document.getElementById('wizard-status').textContent='Checking private LAN addresses…';
});
document.getElementById('wizard-manual-add').addEventListener('click',async()=>{
  const ip=document.getElementById('wizard-manual-ip').value.trim();
  try{
    const result=await deviceRequest('add',{ip});
    await refreshDevices();
    const d=result.devices.find(x=>x.ip===ip);
    if(d){addWizard.close();showDeviceDetail(d.id);}
  }catch(error){document.getElementById('wizard-status').textContent=error.message;}
});
document.getElementById('wizard-go-build').addEventListener('click',()=>{
  addWizard.close();document.querySelector('.nav-tab[data-page="build"]').click();
});
for(const button of document.querySelectorAll('[data-close-dialog]')){
  button.addEventListener('click',()=>document.getElementById(button.dataset.closeDialog).close());
}
for(const dialog of [addWizard,detailWizard]){
  dialog.addEventListener('click',e=>{if(e.target===dialog)dialog.close();});
}
document.getElementById('detail-rename').addEventListener('click',async()=>{
  try{
    await deviceRequest('rename',{device_id:detailDeviceId,name:document.getElementById('detail-name').value});
    await refreshDevices();showDeviceDetail(detailDeviceId);
    document.getElementById('detail-feedback').textContent='Name saved.';
  }catch(error){document.getElementById('detail-feedback').textContent=error.message;}
});
document.getElementById('detail-password').addEventListener('click',()=>{
  const id=detailDeviceId;detailWizard.close();
  document.querySelector('.nav-tab[data-page="settings"]').click();
  document.getElementById('devices-panel').open=true;
  deviceKeySelect.value=id;syncSelectedDevice();
  document.getElementById('devices-panel').scrollIntoView({behavior:'smooth',block:'start'});
});
document.getElementById('detail-ota').addEventListener('click',()=>{
  const id=detailDeviceId;detailWizard.close();
  otaDevice.value=id;syncSelectedDevice();
  document.querySelector('.nav-tab[data-page="build"]').click();
  otaSend.scrollIntoView({behavior:'smooth',block:'center'});
});
document.getElementById('detail-open').addEventListener('click',()=>openDeviceSite(detailDeviceId));
document.getElementById('detail-remove').addEventListener('click',async()=>{
  const d=deviceById(detailDeviceId);
  if(!d||!confirm('Remove '+d.name+' and delete its saved OTA password from Builder?'))return;
  try{
    await deviceRequest('remove',{device_id:detailDeviceId});
    detailWizard.close();await refreshDevices();
  }catch(error){document.getElementById('detail-feedback').textContent=error.message;}
});
async function refreshLatestRelease(){
  try{
    const data=await api('api/releases?scope=recent');
    latestFirmwareRelease=data.latest||null;
  }catch(error){latestFirmwareRelease=null;}
  renderDashboard();
}
refreshLatestRelease();
setInterval(refreshLatestRelease,300000);
// Backend probes previously discovered devices every 60 seconds.
setInterval(refreshDevices,30000);
