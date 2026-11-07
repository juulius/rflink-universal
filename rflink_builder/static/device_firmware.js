/* Device-centred RFLink firmware workflow.
 * The old compiler screen remains available under Advanced.
 * Build and deploy always use the same, existing server-side safety checks.
 */
const firmwareDialog=document.getElementById('firmware-build-dialog');
const firmwareLog=document.getElementById('firmware-build-log');
const firmwareState=document.getElementById('firmware-build-state');
const firmwareMessage=document.getElementById('firmware-build-message');
const firmwareStop=document.getElementById('firmware-build-stop');
const firmwareInstall=document.getElementById('firmware-build-install');
const inlineInstall=document.getElementById('detail-install-panel');
const installStatus=document.getElementById('detail-install-status');
const installConfirm=document.getElementById('detail-install-confirm');
const installStart=document.getElementById('detail-install-start');
const installPort=document.getElementById('detail-install-port');
let firmwareBuilds=[];
let activeFirmwareModalDevice='';
let activeFirmwareModalJob='';
let activeFirmwareLogOffset=0;
let activeFirmwareRequest=false;
let firmwareBuildFinishedHandled='';
let firmwareLastSnapshot=null;
let inlineInstallMethod='ota';
let trackedInstallId='';
let savedDownloadBusy=false;
let trackedInstallBusy=false;
let firmwarePollInFlight=false;

const fwRequest=(path,data)=>api(path,{
  method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(data||{})
});
function deviceBuildMatches(d,b){
  if(!d||!b||b.board_profile_hash!==d.board_hash)return false;
  if(d.configured){
    return b.device_id===d.id && b.config_revision===d.config_revision;
  }
  return !b.device_id;
}
function savedBuildForDevice(d){
  return firmwareBuilds.find(b=>deviceBuildMatches(d,b))||null;
}
async function loadDeviceFirmwareBuilds(){
  try{
    const result=await api('api/releases/built');
    firmwareBuilds=Array.isArray(result.builds)?result.builds:[];
  }catch(e){
    firmwareBuilds=[];
    if(currentDetailDevice())document.getElementById('detail-ota-note').textContent='Could not check saved firmware builds: '+e.message;
  }
  const current=currentDetailDevice();
  if(current)updateDeviceFirmwarePanel(current);
}
function buildRunningFor(d,status){
  return status && (status.status==='running'||status.status==='cancelling') &&
    (d?.configured? status.device_id===d.id :
      !status.device_id && activeFirmwareModalDevice===d?.id);
}
function deviceOtaEligible(d){
  return Boolean(d?.online && d?.has_key && d?.ip &&
    (!d.configured || (d.provisioned && d.physical_id)));
}
function updateDeviceFirmwarePanel(d){
  if(!d)return;
  const latest=savedBuildForDevice(d);
  const running=buildRunningFor(d,firmwareLastSnapshot);
  const ota=document.getElementById('detail-ota');
  const build=document.getElementById('detail-build');
  ota.disabled=!latest;
  ota.textContent=latest?'Install firmware':'Build required';
  build.textContent='Build firmware';
  document.getElementById('detail-build-log').hidden=!(firmwareLastSnapshot?.job_id &&
    (activeFirmwareModalDevice===d.id || (d.configured && firmwareLastSnapshot.device_id===d.id)));
  const summary=document.getElementById('detail-update-summary');
  const note=document.getElementById('detail-ota-note');
  if(running){
    summary.textContent='Firmware build running for '+d.name+'. You can close the log without stopping.';
    note.textContent='Build is running. Installation will be available after a verified build succeeds.';
  }else if(latest){
    summary.textContent='Ready to install · '+firmwareReleaseLabel(latest.release_tag)+
      ' · local build '+latest.job_id.slice(0,6);
    note.textContent=deviceOtaEligible(d)
      ? 'Choose OTA, USB, or a download for another computer.'
      : 'Choose USB on this computer or download. OTA needs an online, linked device with its saved password.';
  }else{
    summary.textContent=d.configured&&!d.provisioned
      ? 'Configuration saved. Build firmware for the first USB installation.'
      : 'No compatible locally built firmware for this device.';
    note.textContent='The Builder checks this RFLink’s pinout and the saved configuration before enabling installation.';
    inlineInstall.hidden=true;
  }
  if(!inlineInstall.hidden){
    const selected=firmwareBuilds.find(b=>b.job_id===inlineInstall.dataset.jobId);
    if(!selected||!deviceBuildMatches(d,selected)){
      inlineInstall.hidden=true;
      installConfirm.checked=false;
    }else{
      document.getElementById('detail-install-build').textContent=
        firmwareReleaseLabel(selected.release_tag)+' · local build '+selected.job_id.slice(0,6);
      renderInstallationControls(d);
    }
  }
}
function showBuildDialog(d){
  if(!firmwareDialog.open)firmwareDialog.showModal();
  document.getElementById('firmware-build-target').textContent=d?.name||'RFLink device';
}
function setFirmwareModalState(state,message){
  firmwareState.textContent=state;
  firmwareState.className='device-badge'+(state==='Succeeded'?' online':state==='Failed'?' offline':'');
  firmwareMessage.textContent=message||'';
}
function closeFirmwareDialog(){
  if(firmwareDialog.open)firmwareDialog.close();
}
document.getElementById('firmware-build-close').addEventListener('click',closeFirmwareDialog);
document.getElementById('firmware-build-close-icon').addEventListener('click',closeFirmwareDialog);
document.getElementById('firmware-build-copy').addEventListener('click',async()=>{
  try{
    await navigator.clipboard.writeText(firmwareLog.textContent);
    firmwareMessage.textContent='Build log copied.';
  }catch(e){firmwareMessage.textContent='Could not copy: '+e.message;}
});
firmwareStop.addEventListener('click',async()=>{
  if(!activeFirmwareModalJob)return;
  if(!confirm('Stop this firmware build? Closing the popup does not stop it.'))return;
  try{
    const state=await api('api/status');
    if(state.job_id!==activeFirmwareModalJob||!['running','cancelling'].includes(state.status))return;
    await api('api/cancel',{method:'POST'});
    setFirmwareModalState('Stopping','Stopping the compiler…');
  }catch(e){setFirmwareModalState('Error',e.message);}
});
firmwareInstall.addEventListener('click',async()=>{
  const d=discoveredDevices.find(item=>item.id===activeFirmwareModalDevice&&item.managed);
  closeFirmwareDialog();
  if(!d)return;
  showDeviceDetail(d.id);
  if(!savedBuildForDevice(d))await loadDeviceFirmwareBuilds();
  openDeviceInstallation(d);
});

async function beginFirmwareBuild(id){
  const device=discoveredDevices.find(item=>item.id===id&&item.managed);
  if(!device)return;
  showBuildDialog(device);
  const previous=firmwareLastSnapshot;
  // Reopen the same active build rather than accidentally starting it again.
  if(buildRunningFor(device,previous) && previous.job_id){
    activeFirmwareModalDevice=id;
    if(activeFirmwareModalJob!==previous.job_id){
      activeFirmwareModalJob=previous.job_id;
      activeFirmwareLogOffset=0;
      firmwareLog.textContent='';
    }
    setFirmwareModalState('Building…','Showing this RFLink’s active build.');
    await pollDeviceFirmware();
    return;
  }
  if(activeFirmwareRequest)return;
  activeFirmwareRequest=true;
  activeFirmwareModalDevice=id;
  activeFirmwareModalJob='';
  activeFirmwareLogOffset=0;
  firmwareBuildFinishedHandled='';
  firmwareLog.textContent='';
  firmwareInstall.hidden=true;
  firmwareStop.hidden=true;
  setFirmwareModalState('Preparing','Checking the ESP-IDF compiler and saved RFLink configuration…');
  try{
    const status=await api('api/status');
    if(['running','cancelling'].includes(status.status)){
      if(device.configured&&status.device_id===device.id){
        activeFirmwareModalJob=status.job_id;
        firmwareLastSnapshot=status;
        await pollDeviceFirmware();
        return;
      }
      throw new Error('Another firmware build is already running. Open its device to see its log, or try again after it finishes.');
    }
    const tools=await api('api/toolchain/status');
    if(!tools.ready){
      throw new Error('ESP-IDF is still being prepared. Open Advanced → Compiler setup to view the toolchain log.');
    }
    const result=device.configured
      ? await fwRequest('api/build/device',{device_id:device.id})
      : await api('api/build',{method:'POST'});
    activeFirmwareModalJob=result.job_id;
    setFirmwareModalState('Building…','Compiler started. The log updates automatically.');
    firmwareStop.hidden=false;
    await pollDeviceFirmware();
  }catch(e){
    setFirmwareModalState('Error',e.message);
  }finally{
    activeFirmwareRequest=false;
  }
}

async function pollDeviceFirmware(){
  if(firmwarePollInFlight)return;
  firmwarePollInFlight=true;
  try{
    const s=await api('api/status');
    firmwareLastSnapshot=s;
    const d=currentDetailDevice();
    if(d)updateDeviceFirmwarePanel(d);
    if(!activeFirmwareModalJob||s.job_id!==activeFirmwareModalJob){
      // A different build may have been activated while this dialog was closed.
      return;
    }
    const names={idle:'Waiting',running:'Building…',cancelling:'Stopping…',
      success:'Succeeded',failed:'Failed',canceled:'Canceled'};
    setFirmwareModalState(names[s.status]||s.status,
      s.status==='failed'?(s.error||'Build failed. See the log.'):
      s.status==='success'?'Firmware compiled and validated. Hardware not yet tested.':
      s.status==='running'?'ESP-IDF is compiling. You can close this popup.':
      s.status==='canceled'?'Build canceled.':'');
    firmwareStop.hidden=!['running','cancelling'].includes(s.status);
    firmwareStop.disabled=s.status==='cancelling';
    if(firmwareDialog.open){
      const result=await api('api/log?offset='+activeFirmwareLogOffset);
      if(result.text){
        const nearEnd=firmwareLog.scrollHeight-firmwareLog.scrollTop <= firmwareLog.clientHeight+40;
        firmwareLog.textContent+=result.text;
        if(firmwareLog.textContent.length>260000){
          firmwareLog.textContent=firmwareLog.textContent.slice(-190000);
        }
        if(nearEnd)firmwareLog.scrollTop=firmwareLog.scrollHeight;
      }
      activeFirmwareLogOffset=result.next_offset;
    }
    if(s.status==='success'&&firmwareBuildFinishedHandled!==s.job_id){
      firmwareBuildFinishedHandled=s.job_id;
      await loadDeviceFirmwareBuilds();
    }
    if(s.status==='success'){
      const saved=firmwareBuilds.find(item=>item.job_id===s.job_id);
      const device=discoveredDevices.find(x=>x.id===activeFirmwareModalDevice);
      firmwareInstall.hidden=!(device&&deviceBuildMatches(device,saved));
    }else{
      firmwareInstall.hidden=true;
    }
  }catch(e){
    if(firmwareDialog.open)setFirmwareModalState('Error','Status unavailable: '+e.message);
  }finally{firmwarePollInFlight=false;}
}

async function refreshFirmwareUsbPorts(){
  const current=installPort.value;
  const d=currentDetailDevice();
  try{
    const r=await api('api/serial');
    const ports=Array.isArray(r.ports)?r.ports:[];
    installPort.replaceChildren();
    const option=document.createElement('option');option.value='';
    option.textContent=ports.length?'Select a USB port':'No USB ports found';
    installPort.append(option);
    for(const port of ports){
      const element=document.createElement('option');element.value=port;element.textContent=port;
      installPort.append(element);
    }
    if(ports.includes(current))installPort.value=current;
    installStatus.textContent=ports.length?'Select the dedicated test board.':'No USB ports found on the Home Assistant host.';
  }catch(e){installStatus.textContent='USB scan failed: '+e.message;}
  if(d)renderInstallationControls(d);
}
function renderInstallationControls(d){
  for(const b of document.querySelectorAll('[data-install-method]')){
    const active=b.dataset.installMethod===inlineInstallMethod;
    b.classList.toggle('selected',active);
    b.setAttribute('aria-pressed',String(active));
    if(b.dataset.installMethod==='ota'){
      b.disabled=!deviceOtaEligible(d);
      b.title=b.disabled?'Device must be online, linked and have a saved OTA password':'';
    }
  }
  for(const key of ['ota','browser','usb','download']){
    document.getElementById('detail-install-'+key).hidden=inlineInstallMethod!==key;
  }
  const isDownload=inlineInstallMethod==='download';
  document.getElementById('detail-install-confirm-row').hidden=isDownload;
  installStart.hidden=isDownload;
  const built=savedBuildForDevice(d);
  installStart.textContent=inlineInstallMethod==='browser'?'Flash using this computer':
    inlineInstallMethod==='usb'?'Flash via Home Assistant USB':'Install via OTA';
  installStart.disabled=trackedInstallBusy||browserFlashBusy||!built||!installConfirm.checked||
    (inlineInstallMethod==='ota'?!deviceOtaEligible(d):
     inlineInstallMethod==='usb'?!installPort.value:false);
}
function chooseInstallMethod(method){
  const d=currentDetailDevice();
  if(!d)return;
  if(method==='ota'&&!deviceOtaEligible(d))return;
  inlineInstallMethod=method;
  installConfirm.checked=false;
  renderInstallationControls(d);
  if(method==='usb')refreshFirmwareUsbPorts();
}
function openDeviceInstallation(d){
  const saved=savedBuildForDevice(d);
  if(!saved){
    installStatus.textContent='Build a matching firmware image first.';
    inlineInstall.hidden=true;
    return;
  }
  inlineInstall.hidden=false;
  inlineInstall.dataset.jobId=saved.job_id;
  installStatus.textContent='';
  inlineInstallMethod=deviceOtaEligible(d)?'ota':'browser';
  installConfirm.checked=false;
  renderInstallationControls(d);
  if(inlineInstallMethod==='usb')refreshFirmwareUsbPorts();
  inlineInstall.scrollIntoView({behavior:'smooth',block:'nearest'});
}
async function activateDeviceFirmware(d){
  const saved=savedBuildForDevice(d);
  if(!saved||!deviceBuildMatches(d,saved)){
    throw new Error('No validated build for this exact RFLink configuration. Build firmware first.');
  }
  if(inlineInstall.dataset.jobId!==saved.job_id){
    throw new Error('The saved firmware selection changed; reopen installation options.');
  }
  // Re-verify file hashes, hardware pinout and saved config revision on server.
  await fwRequest('api/releases/use',{job_id:saved.job_id,device_id:d.configured?d.id:null});
  return saved;
}
async function installFromDevice(method){
  const d=currentDetailDevice();
  if(!d)return;
  if(method==='browser')return installBrowserFirmware(d);
  if(method==='ota'&&!deviceOtaEligible(d)){
    installStatus.textContent='OTA requires an online, linked RFLink with its saved password.';
    return;
  }
  if(!installConfirm.checked){
    installStatus.textContent='Confirm that you understand this can replace the current firmware.';
    return;
  }
  if(method==='usb'&&!installPort.value){
    installStatus.textContent='Select the USB port of the test board.';
    return;
  }
  installStart.disabled=true;
  installStatus.textContent='Verifying local build and device settings…';
  try{
    await activateDeviceFirmware(d);
    const payload={confirm:'TESTBOARD'};
    if(method==='ota')payload.device_id=d.id;
    else{
      payload.port=installPort.value;
      if(d.configured)payload.device_id=d.id;
    }
    const response=await fwRequest('api/deploy/'+method,payload);
    trackedInstallId=d.id;
    trackedInstallBusy=true;
    installStatus.textContent=(response.deployment?.message||'Installation started')+
      ' · Follow the log below.';
    document.getElementById('detail-install-log-container').open=true;
    await pollInlineInstall();
  }catch(e){installStatus.textContent='Installation blocked: '+e.message;}
  finally{renderInstallationControls(d);}
}
async function downloadDevicePackage(){
  const d=currentDetailDevice();
  if(!d||savedDownloadBusy)return;
  savedDownloadBusy=true;
  installStatus.textContent='Validating private installation package…';
  try{
    await activateDeviceFirmware(d);
    const a=document.createElement('a');
    const b=savedBuildForDevice(d);
    if(!b)throw new Error('Compatible build is no longer available');
    a.href='api/download/device/usb?device_id='+encodeURIComponent(d.id)+'&job_id='+encodeURIComponent(b.job_id);
    a.download='RFLink_USB_OTA.zip';
    document.body.append(a);a.click();a.remove();
    installStatus.textContent='Download requested. Keep the ZIP private (it may contain Wi-Fi and OTA secrets).';
  }catch(e){installStatus.textContent='Download blocked: '+e.message;}
  finally{savedDownloadBusy=false;}
}
async function pollInlineInstall(){
  const d=currentDetailDevice();
  if(!d || inlineInstall.hidden)return;
  try{
    const dep=await api('api/deploy/status');
    // Never label another RFLink's operation as this device's update.
    const belongs=trackedInstallId===d.id ||
      (dep.device_id===d.id && dep.device_id !== null);
    if(!belongs)return;
    const labels={idle:'No installation started',running:'Installing…',
      success:'Installation command completed. Verify the hardware.',
      failed:'Installation failed.'};
    installStatus.textContent=(labels[dep.status]||dep.status)+
      (dep.message?' · '+dep.message:'')+(dep.error?' · '+dep.error:'');
    if(dep.status!=='idle'){
      const output=await api('api/deploy/log');
      document.getElementById('detail-install-log').textContent=(output.text||'').slice(-180000);
    }
    trackedInstallBusy=dep.status==='running';
    renderInstallationControls(d);
  }catch(e){installStatus.textContent='Installation status unavailable: '+e.message;}
}

document.getElementById('detail-build').addEventListener('click',()=>{
  const d=currentDetailDevice();if(d)beginFirmwareBuild(d.id);
});
document.getElementById('detail-build-log').addEventListener('click',async()=>{
  const d=currentDetailDevice();
  if(!d)return;
  const current=await api('api/status');
  if(!current.job_id ||
     (current.device_id !== (d.configured?d.id:null)) ||
     (!d.configured&&activeFirmwareModalDevice!==d.id)){
    document.getElementById('detail-feedback').textContent='Build log belongs to a different device.';
    return;
  }
  activeFirmwareModalDevice=d.id;
  if(activeFirmwareModalJob!==current.job_id){
    activeFirmwareModalJob=current.job_id;
    activeFirmwareLogOffset=0;
    firmwareLog.textContent='';
  }
  showBuildDialog(d);
  await pollDeviceFirmware();
});
document.getElementById('detail-ota').addEventListener('click',()=>{
  const d=currentDetailDevice();if(d)openDeviceInstallation(d);
});
for(const element of document.querySelectorAll('[data-install-method]')){
  element.addEventListener('click',()=>chooseInstallMethod(element.dataset.installMethod));
}
installConfirm.addEventListener('input',()=>{
  const d=currentDetailDevice();if(d)renderInstallationControls(d);
});
installPort.addEventListener('change',()=>{
  const d=currentDetailDevice();if(d)renderInstallationControls(d);
});
document.getElementById('detail-install-refresh').addEventListener('click',refreshFirmwareUsbPorts);
installStart.addEventListener('click',()=>installFromDevice(inlineInstallMethod));
document.getElementById('detail-download').addEventListener('click',downloadDevicePackage);
loadDeviceFirmwareBuilds();
pollDeviceFirmware();
setInterval(pollDeviceFirmware,2000);
setInterval(pollInlineInstall,2500);
