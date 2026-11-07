/* Published firmware catalog for each configured RFLink.
 * Three channel cards are the ONLY channel selector. All versions are
 * returned by GitHub Releases via the authenticated Builder API and the
 * chosen immutable tag + release ID are verified server-side before saving.
 */
const firmwareChannelButtons=Array.from(document.querySelectorAll('[data-firmware-channel]'));
const firmwareVersionList=document.getElementById('detail-firmware-versions');
const firmwareMore=document.getElementById('detail-firmware-load-more');
const firmwareSelectionFeedback=document.getElementById('detail-firmware-selection-message');
const firmwareChosenTitle=document.getElementById('detail-firmware-selected-version');
const firmwareChosenNotes=document.getElementById('detail-firmware-selected-notes');
const firmwareChosenSource=document.getElementById('detail-firmware-source');
let firmwareCatalog=[],firmwareCatalogPage=0,firmwareCatalogMore=true;
let firmwareCatalogPending=null,firmwareCatalogDevice='',firmwareChannel='experimental';
let firmwareSelecting=false,firmwareCatalogError='';

function firmwareChoiceTag(d){
  const selected=d?.firmware_choice;
  return selected && typeof selected==='object' && typeof selected.tag==='string'
    ? selected.tag:'current';
}
function firmwareChannelOf(tag){
  if(/-exp[.][1-9][0-9]*$/.test(tag))return 'experimental';
  if(/-rc[.][1-9][0-9]*$/.test(tag))return 'pre-release';
  return tag==='current'?'experimental':'stable';
}
function firmwareChoiceReady(d){
  // A channel card click only filters the list. It never silently changes
  // the release that a build will compile.
  return !d?.configured || firmwareChannelOf(firmwareChoiceTag(d))===firmwareChannel;
}
function firmwareCatalogItem(tag){
  return firmwareCatalog.find(r=>r.tag===tag)||null;
}
function firmwareLabel(r){
  return r?.display||r?.tag||'Unknown release';
}
function firmwareCatalogText(el,text){
  el.textContent=text||'';
}
function firmwareCatalogRender(d){
  if(!d || d.id!==firmwareCatalogDevice)return;
  const selected=firmwareChoiceTag(d);
  const canChoose=!!d.configured && !firmwareSelecting && !activeFirmwareRequest &&
    !['running','cancelling'].includes(firmwareLastSnapshot?.status);
  for(const button of firmwareChannelButtons){
    const active=button.dataset.firmwareChannel===firmwareChannel;
    button.classList.toggle('selected',active);
    button.setAttribute('aria-pressed',String(active));
  }
  const visible=firmwareCatalog.filter(r=>r.channel===firmwareChannel);
  firmwareVersionList.replaceChildren();
  if(!visible.length && !firmwareCatalogPending){
    const item=document.createElement('p');item.className='minor';
    item.textContent=firmwareCatalogError || ('No published '+(firmwareChannel==='pre-release'?'Early adopter':firmwareChannel)+
      ' firmware available. Select another channel or load more versions.');
    firmwareVersionList.append(item);
  }
  for(const r of visible){
    const button=document.createElement('button');
    button.type='button';button.className='firmware-version-choice';
    button.setAttribute('role','radio');
    const active=selected===r.tag;
    button.setAttribute('aria-checked',String(active));
    button.classList.toggle('selected',active);
    button.disabled=!canChoose;
    const mark=document.createElement('span');mark.className='firmware-radio';
    mark.setAttribute('aria-hidden','true');
    const copy=document.createElement('span');
    const name=document.createElement('strong');name.textContent=firmwareLabel(r);
    const meta=document.createElement('small');
    meta.textContent=r.published_at?'Published '+r.published_at.slice(0,10):'Published release';
    copy.append(name,meta);button.append(mark,copy);
    button.addEventListener('click',()=>saveFirmwareChoice(d,{tag:r.tag,release_id:r.id}));
    firmwareVersionList.append(button);
  }
  if(firmwareChannel==='experimental'){
    const local=document.createElement('button');
    local.type='button';local.className='firmware-version-choice';
    const active=selected==='current';
    local.classList.toggle('selected',active);
    local.setAttribute('role','radio');local.setAttribute('aria-checked',String(active));
    local.disabled=!canChoose;
    const mark=document.createElement('span');mark.className='firmware-radio';
    mark.setAttribute('aria-hidden','true');
    const copy=document.createElement('span');
    const title=document.createElement('strong');title.textContent='Current local development source';
    const small=document.createElement('small');small.textContent='Advanced · not a published or versioned release';
    copy.append(title,small);local.append(mark,copy);
    local.addEventListener('click',()=>saveFirmwareChoice(d,'current'));
    firmwareVersionList.append(local);
  }
  firmwareMore.hidden=!firmwareCatalogMore;
  firmwareMore.disabled=Boolean(firmwareCatalogPending);
  firmwareMore.textContent=firmwareCatalogPending?'Loading…':'Load more versions';
  if(!d.configured){
    firmwareCatalogText(firmwareChosenTitle,'No device-specific configuration');
    firmwareCatalogText(firmwareChosenNotes,'Configure this RFLink with the correct board profile and Wi-Fi settings before choosing released firmware.');
    firmwareCatalogText(firmwareChosenSource,'This device can still use the Advanced local Builder.');
    document.getElementById('detail-build').disabled=false;
    return;
  }
  const choice=firmwareCatalogItem(selected);
  const formatted=selected==='current'?'Current local development source':firmwareLabel(choice)||selected;
  firmwareCatalogText(firmwareChosenTitle,choice?firmwareLabel(choice):
    selected==='current'?'Current local development source':selected);
  firmwareCatalogText(firmwareChosenNotes,choice?
    (choice.name||'Published firmware source')+(choice.published_at?' · '+choice.published_at.slice(0,10):''):
    selected==='current'?'Unreleased source. For the v0.7 hardware test, select a published Experimental build.':
    'Saved release not present in loaded history. Use Load more versions to find it.');
  firmwareCatalogText(firmwareChosenSource,selected==='current'
    ?'Source: local development snapshot (not an official release)'
    :'Source: immutable GitHub Release · '+selected);
  document.getElementById('detail-build').disabled=firmwareSelecting||!firmwareChoiceReady(d);
  if(!firmwareSelecting && !firmwareCatalogError && firmwareChannelOf(selected)!==firmwareChannel){
    firmwareCatalogText(firmwareSelectionFeedback,
      'Choose a version in this channel before building. Your saved selection remains unchanged.');
  }
}
async function firmwareCatalogLoadNext(){
  if(firmwareCatalogPending || !firmwareCatalogMore)return;
  const page=firmwareCatalogPage+1;
  firmwareCatalogPending=(async()=>{
    try{
      const response=await api('api/releases?scope=all&page='+page);
      if(response.configured===false)throw new Error('Configure your GitHub repository in Builder settings.');
      const items=Array.isArray(response.releases)?response.releases:[];
      const seen=new Set(firmwareCatalog.map(r=>r.tag));
      for(const item of items){
        if(!item || typeof item.tag!=='string' || !['stable','pre-release','experimental'].includes(item.channel) ||
           !Number.isSafeInteger(item.id) || item.id<1 || seen.has(item.tag))continue;
        firmwareCatalog.push(item);seen.add(item.tag);
      }
      firmwareCatalogPage=page;firmwareCatalogMore=response.has_more===true && page<20;
      firmwareCatalogError='';
    }catch(error){
      firmwareCatalogError='Could not load published releases: '+error.message;
      firmwareCatalogMore=true; // Let the user retry without losing prior pages.
    }
  })();
  try{await firmwareCatalogPending;}finally{
    firmwareCatalogPending=null;
    const d=currentDetailDevice();if(d)firmwareCatalogRender(d);
  }
}
async function saveFirmwareChoice(d,choice){
  if(!d.configured||firmwareSelecting)return;
  firmwareSelecting=true;
  firmwareCatalogText(firmwareSelectionFeedback,'Verifying and saving the published firmware version…');
  firmwareCatalogRender(d);
  try{
    const response=await fwRequest('api/devices/firmware/select',
      {device_id:d.id,firmware:choice});
    if(response.device_id!==d.id)throw new Error('The Builder returned a different device.');
    // A previous build may still be valid for its old release, but may never
    // be accidentally installed after changing this device's chosen version.
    inlineInstall.hidden=true;
    installConfirm.checked=false;
    inlineInstall.removeAttribute('data-job-id');
    await refreshDevices();
    await loadDeviceFirmwareBuilds();
    firmwareCatalogText(firmwareSelectionFeedback,
      'Saved. Build this exact version, or install an already verified matching build.');
  }catch(error){
    firmwareCatalogText(firmwareSelectionFeedback,'Could not save firmware choice: '+error.message);
  }finally{
    firmwareSelecting=false;
    const current=currentDetailDevice();if(current)firmwareCatalogRender(current);
  }
}
function updateFirmwareCatalog(d){
  if(!d)return;
  if(d.id!==firmwareCatalogDevice){
    firmwareCatalogDevice=d.id;
    firmwareChannel=firmwareChannelOf(firmwareChoiceTag(d));
    firmwareCatalogText(firmwareSelectionFeedback,'');
  }
  firmwareCatalogRender(d);
  if(!firmwareCatalogPage&&!firmwareCatalogPending)firmwareCatalogLoadNext();
}
for(const button of firmwareChannelButtons){
  button.addEventListener('click',()=>{
    firmwareChannel=button.dataset.firmwareChannel;
    const d=currentDetailDevice();if(d)firmwareCatalogRender(d);
  });
}
firmwareMore.addEventListener('click',firmwareCatalogLoadNext);
document.getElementById('detail-overview-firmware').addEventListener('click',()=>{
  const d=currentDetailDevice();
  if(!d)return;
  selectDetailTab('firmware');
  updateFirmwareCatalog(d);
});
