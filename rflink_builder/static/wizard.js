/* Guided new-device creation: copy global defaults, edit this device only,
 * generate a strong random OTA key, pin firmware and prepare first USB flash.
 * No secrets go to GitHub. The existing firmware still uses plaintext NVS keys.
 */
const wizardName=document.getElementById('wizard-name');
const wizardWifi=document.getElementById('wizard-wifi');
const wizardWifiPass=document.getElementById('wizard-wifi-pass');
const wizardInherit=document.getElementById('wizard-inherit-wifi');
const wizardOta=document.getElementById('wizard-ota-key');
const wizardFirmware=document.getElementById('wizard-firmware');
const wizardStatus=document.getElementById('wizard-status');
const wizardNew=document.getElementById('wizard-new');
const wizardExisting=document.getElementById('wizard-existing');
const wizardFinished=document.getElementById('wizard-finished');
const wizardPins=document.getElementById('wizard-pin-grid');
let wizardBoard=null;
let wizardEditingId=null;
let wizardCreatedId=null;
let wizardReleases=new Map();
let wizardDefaultNetwork='';

function wizardMode(which){
  const newMode=which==='new';
  wizardNew.hidden=!newMode;
  wizardExisting.hidden=newMode;
  wizardFinished.hidden=true;
  document.getElementById('wizard-mode-new').classList.toggle('selected',newMode);
  document.getElementById('wizard-mode-existing').classList.toggle('selected',!newMode);
}
function wizardEye(input,button){
  button.addEventListener('click',async()=>{
    if(input.type==='text'){
      input.type='password';
      button.textContent='◉';button.setAttribute('aria-label','Show password');
      return;
    }
    if(!input.value && input===wizardWifiPass && !wizardInherit.checked && wizardEditingId){
      try{
        const r=await api('api/devices/wifi/reveal',{method:'POST',
          headers:{'Content-Type':'application/json'},
          body:JSON.stringify({device_id:wizardEditingId})});
        input.value=r.value||'';
      }catch(e){wizardStatus.textContent=e.message;return;}
    }
    input.type='text';button.textContent='⊘';button.setAttribute('aria-label','Hide password');
  });
}
wizardEye(wizardWifiPass,document.getElementById('wizard-wifi-eye'));
wizardEye(wizardOta,document.getElementById('wizard-ota-eye'));
wizardInherit.addEventListener('change',()=>{
  wizardWifiPass.disabled=wizardInherit.checked;
  wizardWifiPass.placeholder=wizardInherit.checked?'Saved default Wi-Fi password':'Wi-Fi password (8–63 characters)';
  if(wizardInherit.checked)wizardWifiPass.value='';
});
wizardWifi.addEventListener('input',()=>{
  if(wizardWifi.value!==wizardDefaultNetwork&&wizardInherit.checked){
    wizardInherit.checked=false;
    wizardInherit.dispatchEvent(new Event('change'));
  }
});
async function generateWizardPassword(){
  try{
    const key=await api('api/devices/key/generate',{method:'POST'});
    wizardOta.value=key.key||'';
    wizardOta.type='password';
    document.getElementById('wizard-ota-eye').textContent='◉';
  }catch(e){wizardStatus.textContent=e.message;}
}
document.getElementById('wizard-generate-key').addEventListener('click',generateWizardPassword);
document.getElementById('wizard-copy-key').addEventListener('click',async()=>{
  try{
    if(!wizardOta.value)throw new Error('Generate a password first.');
    await navigator.clipboard.writeText(wizardOta.value);
    wizardStatus.textContent='OTA password copied to clipboard.';
  }catch(e){wizardStatus.textContent=e.message;}
});
function pinInput(label,initial,set){
  const item=document.createElement('label');item.className='wizard-pin';
  const caption=document.createElement('span');caption.textContent=label;
  const input=document.createElement('input');
  input.type='number';input.min='0';input.max='48';input.step='1';input.value=initial;
  input.addEventListener('input',()=>{set(input.value===''?null:Number(input.value));});
  item.append(caption,input);return item;
}
function renderWizardPinout(){
  wizardPins.replaceChildren();
  if(!wizardBoard)return;
  const spi=wizardBoard.spi;
  const group=document.createElement('div');group.className='wizard-pin-group';
  const heading=document.createElement('strong');heading.textContent='Shared SPI bus';group.append(heading);
  for(const key of ['sclk','mosi','miso','clock_hz']){
    const control=pinInput(key.toUpperCase(),spi[key],v=>{spi[key]=v;});
    if(key==='clock_hz'){
      control.querySelector('input').max='6000000';
      control.querySelector('input').min='100000';
      control.querySelector('input').step='100000';
    }
    group.append(control);
  }
  wizardPins.append(group);
  for(const radio of wizardBoard.radios){
    const item=document.createElement('div');item.className='wizard-pin-group';
    const title=document.createElement('strong');title.textContent=radio.id+' MHz';item.append(title);
    for(const key of ['cs','gdo0','gdo2']){
      item.append(pinInput(key.toUpperCase(),radio[key],v=>{radio[key]=v;}));
    }
    wizardPins.append(item);
  }
}
async function loadWizardReleases(current){
  wizardReleases.clear();wizardFirmware.replaceChildren();
  const local=document.createElement('option');local.value='current';
  local.textContent='Current RFLink source (development)';wizardFirmware.append(local);
  try{
    const releases=await api('api/releases?scope=recent');
    const pool=[releases.latest,...(releases.recent||[])].filter(Boolean);
    for(const r of pool){
      if(wizardReleases.has(r.tag))continue;
      const option=document.createElement('option');
      option.value=r.tag;option.textContent=r.tag+' · '+(r.name||'Published release');
      wizardFirmware.append(option);
      wizardReleases.set(r.tag,{tag:r.tag,release_id:r.id});
    }
    if(current && typeof current==='object'&&wizardReleases.has(current.tag)){
      wizardFirmware.value=current.tag;
    }
  }catch(e){
    wizardStatus.textContent='GitHub releases unavailable. You can use the locally saved source.';
  }
}
async function beginWizard(editId=null){
  wizardEditingId=editId;wizardCreatedId=null;
  wizardMode('new');
  wizardStatus.textContent='Loading saved defaults…';
  document.getElementById('wizard-heading').textContent=editId?'Edit RFLink':'Add RFLink';
  document.getElementById('wizard-create').textContent=editId?'Save device configuration':'Create device';
  document.getElementById('wizard-mode-existing').hidden=!!editId;
  document.getElementById('wizard-generate-key').hidden=!!editId;
  document.getElementById('wizard-copy-key').hidden=!!editId;
  wizardOta.disabled=!!editId;
  wizardOta.value='';wizardOta.placeholder=editId?'Password managed on device overview':'';
  wizardWifiPass.value='';wizardWifiPass.type='password';
  wizardInherit.checked=false;wizardInherit.disabled=!!editId;
  try{
    const values=editId
      ? await api('api/devices/config?id='+encodeURIComponent(editId))
      : await api('api/devices/defaults');
    wizardBoard=JSON.parse(JSON.stringify(values.board));
    wizardName.value=values.name||'';
    wizardWifi.value=values.wifi_ssid||'';
    wizardDefaultNetwork=values.wifi_ssid||'';
    if(!editId && values.wifi_password_saved){
      wizardInherit.checked=true;
    }
    wizardInherit.dispatchEvent(new Event('change'));
    renderWizardPinout();
    await loadWizardReleases(values.firmware_choice);
    wizardFirmware.disabled=!!editId;
    if(!editId)await generateWizardPassword();
    wizardStatus.textContent='';
  }catch(e){
    wizardStatus.textContent='Could not load default settings: '+e.message;
  }
}
document.getElementById('wizard-mode-new').addEventListener('click',()=>{beginWizard();});
document.getElementById('wizard-mode-existing').addEventListener('click',()=>{wizardMode('existing');renderFoundWizard();});
document.getElementById('wizard-create').addEventListener('click',async()=>{
  if(!wizardBoard)return;
  const button=document.getElementById('wizard-create');
  button.disabled=true;
  wizardStatus.textContent='Validating and saving the device…';
  try{
    const payload={
      name:wizardName.value.trim(),
      wifi_ssid:wizardWifi.value,
      wifi_password:wizardInherit.checked?'':wizardWifiPass.value,
      inherit_wifi_password:wizardInherit.checked,
      board:wizardBoard
    };
    let result;
    if(wizardEditingId){
      payload.device_id=wizardEditingId;
      result=await api('api/devices/config/save',{method:'POST',
        headers:{'Content-Type':'application/json'},body:JSON.stringify(payload)});
      if(payload.name){
        await deviceRequest('rename',{device_id:wizardEditingId,name:payload.name});
      }
      wizardCreatedId=wizardEditingId;
    }else{
      payload.ota_key=wizardOta.value;
      const pinned=wizardReleases.get(wizardFirmware.value);
      payload.firmware=pinned||'current';
      result=await api('api/devices/create',{method:'POST',
        headers:{'Content-Type':'application/json'},body:JSON.stringify(payload)});
      wizardCreatedId=result.config.device_id;
    }
    await refreshDevices();
    wizardNew.hidden=true;
    wizardExisting.hidden=true;
    wizardFinished.hidden=false;
    document.getElementById('wizard-finished-title').textContent=(wizardEditingId?'Configuration saved: ':'Created: ')+payload.name;
    document.getElementById('wizard-finish-build').disabled=false;
    wizardOta.value='';wizardWifiPass.value='';
    wizardStatus.textContent='';
  }catch(e){wizardStatus.textContent=e.message;}
  finally{button.disabled=false;}
});
async function buildConfiguredDevice(id){
  // The wizard and the device card both share the same modal/live-log
  // workflow. No return to the large legacy Build page.
  if(!id)return;
  await beginFirmwareBuild(id);
}
document.getElementById('wizard-finish-build').addEventListener('click',()=>{
  const id=wizardCreatedId;addWizard.close();showDeviceDetail(id);
  buildConfiguredDevice(id);
});
document.getElementById('wizard-finish-later').addEventListener('click',()=>{
  addWizard.close();if(wizardCreatedId)showDeviceDetail(wizardCreatedId);
});
document.getElementById('detail-edit-config').addEventListener('click',()=>{
  if(!currentDetailDevice()?.configured)return;
  beginWizard(detailDeviceId);addWizard.showModal();
});
function updateDeviceConfigActions(d){
  const edit=document.getElementById('detail-edit-config');
  const note=document.getElementById('detail-configuration-note');
  const linkPanel=document.getElementById('detail-link-panel');
  edit.hidden=!d.configured;edit.disabled=!!d.provisioned;
  note.textContent=d.configured
    ? (d.provisioned?'Provisioned device; Wi-Fi and pinout editing is locked to avoid mismatched firmware.':'Individual board and Wi-Fi settings saved locally. Edit before first USB flash.')
    : 'Existing RFLink: password and board information are read from its live status.';
  linkPanel.hidden=!(d.configured && d.provisioned && !d.physical_id);
  if(!linkPanel.hidden){
    const selector=document.getElementById('detail-link-select');
    const current=selector.value;
    selector.replaceChildren();
    const blank=document.createElement('option');blank.value='';blank.textContent='Choose detected RFLink';selector.append(blank);
    for(const discovered of discoveredDevices.filter(x=>!x.managed&&x.online&&x.board_hash===d.board_hash)){
      const opt=document.createElement('option');opt.value=discovered.id;
      opt.textContent=discovered.ip+' · '+discovered.firmware;selector.append(opt);
    }
    selector.value=current;
  }
}
document.getElementById('detail-link').addEventListener('click',async()=>{
  const selected=document.getElementById('detail-link-select').value;
  if(!detailDeviceId||!selected)return;
  if(!confirm('Link the discovered physical RFLink to this saved configuration? Verify its IP and hardware first.'))return;
  try{
    await deviceRequest('link',{config_id:detailDeviceId,discovered_id:selected});
    await refreshDevices();
  }catch(e){document.getElementById('detail-feedback').textContent=e.message;}
});
