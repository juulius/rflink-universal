/* Safe, on-demand firmware storage report. This panel is read-only until a
 * user explicitly confirms a cleanup. Never send a device ID or filesystem
 * path to the deletion API; the server computes its own protected set. */
const firmwareStoragePanel=document.getElementById('firmware-storage');
const storageText=(id,value)=>{document.getElementById(id).textContent=value;};
const storageBytes=value=>{
  if(!Number.isFinite(value)||value<0)return '—';
  if(value>=1073741824)return (value/1073741824).toFixed(2)+' GB';
  if(value>=1048576)return (value/1048576).toFixed(1)+' MB';
  if(value>=1024)return (value/1024).toFixed(1)+' KB';
  return Math.round(value)+' B';
};
let storageLoading=false;
let lastStorage=null;
function renderFirmwareStorage(report){
  lastStorage=report;
  storageText('storage-count',String(report.saved_count??'—'));
  storageText('storage-bytes',storageBytes(report.saved_bytes));
  storageText('storage-clean-count',String(report.reclaimable_count??'—'));
  storageText('storage-clean-bytes',storageBytes(report.reclaimable_bytes));
  const clean=document.getElementById('storage-clean');
  clean.disabled=report.busy===true || !Number.isInteger(report.reclaimable_count) ||
    report.reclaimable_count<1;
  const list=document.getElementById('storage-device-list');
  list.replaceChildren();
  const entries=Array.isArray(report.devices)?report.devices:[];
  for(const [i,item] of entries.entries()){
    const row=document.createElement('div');
    const name=document.createElement('span');
    name.textContent=item.device_id?
      'RFLink '+String(item.device_id).slice(-6):'Unassigned / advanced builds';
    const size=document.createElement('strong');
    size.textContent=String(item.count)+' package(s) · '+storageBytes(item.bytes)+
      (item.ota_verified?' · OTA-verified':
       item.needs_confirmation?' · firmware not verified by Builder':'');
    row.append(name,size);
    list.append(row);
  }
  if(!entries.length){
    const empty=document.createElement('p');
    empty.className='minor';empty.textContent='No saved firmware packages.';
    list.append(empty);
  }
  storageText('storage-feedback',report.busy?
    'Build or installation in progress; cleanup disabled.':
    report.reclaimable_count?
      'Review protected packages below before cleanup.':
      'No obsolete packages eligible for cleanup.');
}
async function refreshFirmwareStorage(force=false){
  if(!force && !firmwareStoragePanel.classList.contains('active-view'))return;
  if(storageLoading)return;
  storageLoading=true;
  try{
    const report=await api('api/storage');
    renderFirmwareStorage(report);
  }catch(error){
    storageText('storage-feedback','Storage report unavailable: '+error.message);
  }finally{storageLoading=false;}
}
document.getElementById('storage-refresh').addEventListener('click',()=>refreshFirmwareStorage(true));
document.querySelector('.nav-tab[data-page="settings"]').addEventListener('click',()=>{
  refreshFirmwareStorage(true);
});
document.getElementById('storage-clean').addEventListener('click',async()=>{
  if(!lastStorage || lastStorage.busy || !lastStorage.reclaimable_count)return;
  const message='Clean up '+lastStorage.reclaimable_count+' old firmware package(s) ('+
    storageBytes(lastStorage.reclaimable_bytes)+')?\n\n'+
    'The active build and any recorded OTA-verified firmware remain protected. '+
    'For devices without confirmed OTA, two newest packages are kept; the '+
    'currently installed USB firmware may be older.\n\n'+
    'Source, settings, OTA passwords and the ESP-IDF toolchain are not deleted.';
  if(!window.confirm(message))return;
  const button=document.getElementById('storage-clean');
  button.disabled=true;
  storageText('storage-feedback','Removing eligible old firmware packages…');
  try{
    const report=await api('api/storage/cleanup',{
      method:'POST',headers:{'Content-Type':'application/json'},
      body:JSON.stringify({confirm:'CLEAN BUILDS'})});
    renderFirmwareStorage(report.storage);
    storageText('storage-feedback','Removed '+report.removed_count+' package(s), recovered '+
      storageBytes(report.freed_bytes)+'.');
    // The advanced version selector must not display packages deleted on disk.
    if(typeof loadSavedBuilds==='function')await loadSavedBuilds();
  }catch(error){
    storageText('storage-feedback','Cleanup failed: '+error.message);
    await refreshFirmwareStorage(true);
  }
});
