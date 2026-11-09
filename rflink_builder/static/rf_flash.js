/* Single dark flash console for direct PC USB, HA-host USB and OTA.
 * Direct USB uses locally packaged Apache-2.0 esptool-js (not ESP Web Tools
 * installer dialogs). A browser-controlled serial-port picker remains mandatory.
 * All images come from the existing exact-device/provenance-validated backend.
 */
const rfFlashConsole=document.getElementById('browser-usb-console');
const rfFlashProgress=document.getElementById('browser-usb-progress');
const rfFlashProgressLabel=document.getElementById('browser-usb-progress-label');
const rfFlashProgressRow=document.getElementById('browser-usb-progress-row');
const rfFlashNative=document.getElementById('browser-usb-native');
const rfFlashStop=document.getElementById('browser-usb-stop-monitor');
let rfFlashMonitorPort=null,rfFlashMonitorReader=null,rfFlashMonitorActive=false;
let rfFlashHostActive=false,rfFlashLog='',rfFlashBusy=false;
let rfFlashIdentityTimer=null,rfFlashIdentityPolling=false;
let rfFlashNetworkTimer=null,rfFlashNetworkDeviceId='',rfFlashNetworkCursor=0;
let rfFlashNetworkBootId=null;
let rfFlashNetworkLog='',rfFlashDeploymentLog='',rfFlashNetworkBusy=false;
let rfFlashNetworkCrashLoaded=false,rfFlashNetworkWarning=false;
const RF_FLASH_LOG_LIMIT=180000;

function rfFlashAppend(text){
  rfFlashLog=(rfFlashLog+rfNormalizeLog(text)).slice(-RF_FLASH_LOG_LIMIT);
  const nearEnd=rfFlashConsole.scrollHeight-rfFlashConsole.scrollTop <
    rfFlashConsole.clientHeight+70;
  rfFlashConsole.textContent=rfFlashLog;
  if(nearEnd)rfFlashConsole.scrollTop=rfFlashConsole.scrollHeight;
}
function rfFlashReplace(text){
  rfFlashLog=rfNormalizeLog(text).slice(-RF_FLASH_LOG_LIMIT);
  rfFlashConsole.textContent=rfFlashLog;
  rfFlashConsole.scrollTop=rfFlashConsole.scrollHeight;
}
function rfFlashSetProgress(percentage,description){
  rfFlashProgressRow.hidden=false;
  rfFlashProgressLabel.textContent=description;
  if(Number.isFinite(percentage)){
    rfFlashProgress.value=Math.round(Math.max(0,Math.min(100,percentage)));
    rfFlashProgress.removeAttribute('aria-busy');
    document.getElementById('browser-usb-percent').textContent=rfFlashProgress.value+'%';
  }else{
    rfFlashProgress.removeAttribute('value');
    rfFlashProgress.setAttribute('aria-busy','true');
    document.getElementById('browser-usb-percent').textContent='…';
  }
}
function rfFlashOpen(method,name){
  if(rfFlashIdentityTimer!==null){clearInterval(rfFlashIdentityTimer);rfFlashIdentityTimer=null;}
  if(rfFlashNetworkTimer!==null){clearInterval(rfFlashNetworkTimer);rfFlashNetworkTimer=null;}
  rfFlashNetworkDeviceId='';rfFlashNetworkCursor=0;
  rfFlashNetworkBootId=null;rfFlashNetworkLog='';
  rfFlashDeploymentLog='';rfFlashNetworkCrashLoaded=false;rfFlashNetworkWarning=false;
  rfFlashHostActive=method!=='browser';
  document.getElementById('browser-usb-heading').textContent=
    method==='browser'?'USB · This PC':method==='usb'?'USB · Home Assistant':'OTA · Network';
  document.getElementById('browser-usb-subtitle').textContent=
    (/^RFLink\b/i.test(name||'') ? name : 'RFLink '+(name||'device'))+
    ' · verified firmware installation';
  const note=document.querySelector('#browser-usb-dialog .rf-console-note');
  note.textContent=method==='ota'
    ? 'OTA updates the application partition and retains existing Wi-Fi/OTA settings. '+ 
      'If ESP32 boot health fails it may roll back. 100% is only shown after the new '+ 
      'firmware boots and reports a valid OTA health state.'
    :method==='usb'
      ? 'USB via Home Assistant may overwrite device flash and saved settings. '+ 
        'Verify the port and firmware identity before starting. Console output confirms '+ 
        'the write command; use Diagnosis to check the ESP32 boot.'
      : 'USB on this computer installs a full ESP32-S3 image containing private Wi-Fi '+ 
        'and OTA credentials. The flash can erase existing settings; choose the correct COM port. '+ 
        'Verify ESP32 boot and radio status after installation.';
  rfFlashLog='';rfFlashConsole.textContent='';
  rfFlashSetProgress(null,'Preparing firmware…');
  rfFlashStop.hidden=true;
  if(!browserUsbDialog.open)browserUsbDialog.showModal();
}
function rfFlashHostStart(method,device){
  rfFlashOpen(method,device.name);
  if(method==='ota')rfFlashNetworkDeviceId=device.id;
  browserUsbDirect.hidden=true;browserUsbExternal.hidden=true;
  rfFlashNative.hidden=true;
  browserUsbMessage('Installation started. Keep the device powered.');
  rfFlashAppend('— '+(method==='ota'?'Network OTA':'Home Assistant USB')+
    ' installation · waiting for verified Builder log —\n');
}
async function rfFlashHostWaitForIdentity(deployment){
  if(!deployment?.device_id)return;
  try{
    const state=await api('api/devices');
    const saved=state.devices?.find(d=>d.id===deployment.device_id);
    const expected=deployment.target_version||'';
    if(saved?.expected_physical_id){
      rfFlashWaitForHardware(deployment.device_id,saved.expected_physical_id,
        saved.expected_firmware_version||expected);
    }else if(saved?.online && saved?.identity_verified && saved?.firmware===expected){
      rfFlashAppend('ESP32 online: '+saved.physical_id+' · running '+expected+
        ' · matching original RFLink configuration.\n');
      rfFlashSetProgress(100,'ESP32 online · firmware and identity confirmed');
      browserUsbMessage('USB written and RFLink is online with the expected firmware.');
    }else{
      rfFlashAppend('USB write finished. Hardware ID not yet confirmed; check diagnosis.\n');
    }
  }catch(error){
    rfFlashAppend('Could not verify RFLink after USB write: '+error.message+'\n');
  }
}
function rfFlashComposeHostLog(){
  rfFlashReplace((rfFlashDeploymentLog+'\n'+rfFlashNetworkLog).slice(-RF_FLASH_LOG_LIMIT));
}
function rfFlashAddNetwork(text){
  rfFlashNetworkLog=(rfFlashNetworkLog+rfNormalizeLog(text)).slice(-RF_FLASH_LOG_LIMIT/2);
  rfFlashComposeHostLog();
}
async function rfFlashReadNetworkDiagnostics(){
  if(rfFlashNetworkBusy||!rfFlashNetworkDeviceId||!browserUsbDialog.open)return;
  rfFlashNetworkBusy=true;
  try{
    const id=rfFlashNetworkDeviceId;
    let result=await api('api/diagnostics/device/console?device_id='+
      encodeURIComponent(id)+'&since='+rfFlashNetworkCursor);
    if(id!==rfFlashNetworkDeviceId||!browserUsbDialog.open)return;
    if(rfFlashNetworkBootId!==null && result.boot_id!==rfFlashNetworkBootId){
      rfFlashNetworkCrashLoaded=false;
      rfFlashNetworkCursor=0;
      rfFlashAddNetwork('— ESP32 rebooted · new console session —\n');
      result=await api('api/diagnostics/device/console?device_id='+
        encodeURIComponent(id)+'&since=0');
      if(id!==rfFlashNetworkDeviceId||!browserUsbDialog.open)return;
    }else if(result.cursor<rfFlashNetworkCursor){
      rfFlashNetworkCrashLoaded=false;
      rfFlashAddNetwork('— ESP32 rebooted · console restarted —\n');
    }
    rfFlashNetworkBootId=result.boot_id;
    if(result.truncated)
      rfFlashAddNetwork('— Earlier ESP-IDF lines overwritten in console ring —\n');
    rfFlashNetworkCursor=result.cursor;
    if(result.text)rfFlashAddNetwork(result.text);
    if(!rfFlashNetworkCrashLoaded){
      const crash=await api('api/diagnostics/device/crashes?device_id='+encodeURIComponent(id));
      if(id!==rfFlashNetworkDeviceId||!browserUsbDialog.open)return;
      rfFlashAddNetwork('— Saved boot diagnosis · last reset: '+
        (crash.last_reset||'unknown')+' · crash data: '+
        (crash.core_dump?.integrity||'unknown')+' —\n');
      for(const item of (crash.boot_phases||[]).slice(0,12)){
        rfFlashAddNetwork('Boot #'+item.boot+' · '+item.stage+
          (item.startup_complete?' · ready':' · last reached')+'\n');
      }
      rfFlashNetworkCrashLoaded=true;
    }
  }catch(error){
    // OTA can disconnect Wi-Fi for several seconds. Do not turn a temporary
    // gap into a false permanent offline diagnosis.
    if(!rfFlashNetworkWarning&&browserUsbDialog.open){
      rfFlashAddNetwork('— Awaiting identity-verified ESP-IDF console after OTA: '+
        String(error.message||error).slice(0,110)+' —\n');
      rfFlashNetworkWarning=true;
    }
  }finally{rfFlashNetworkBusy=false;}
}
function rfFlashStartNetworkDiagnostics(){
  if(!rfFlashNetworkDeviceId||rfFlashNetworkTimer!==null)return;
  rfFlashAddNetwork('\n— ESP32 Wi-Fi diagnosis · same application console as USB (ROM output is USB-only) —\n');
  rfFlashNetworkTimer=setInterval(()=>{
    if(!browserUsbDialog.open){
      clearInterval(rfFlashNetworkTimer);rfFlashNetworkTimer=null;
      return;
    }
    rfFlashReadNetworkDiagnostics();
  },2500);
  rfFlashReadNetworkDiagnostics();
}
function rfFlashHostUpdate(deployment,log){
  if(!rfFlashHostActive)return;
  const state=deployment?.status||'idle';
  const phase=deployment?.phase||'';
  const ota=deployment?.kind==='ota';
  if(typeof log==='string' && log!==rfFlashDeploymentLog){
    rfFlashDeploymentLog=log;
    rfFlashComposeHostLog();
  }
  if(ota && ['accepted','reconnecting','validating'].includes(phase) ||
     ota && ['success','unverified','rolled_back'].includes(state)){
    rfFlashStartNetworkDiagnostics();
  }
  const stages={
    preflight:'1/4 · Verifying device and firmware',
    uploading:'2/4 · Sending firmware to ESP32',
    accepted:'3/4 · OTA written · reboot pending',
    reconnecting:'3/4 · Waiting for ESP32 to reconnect',
    validating:'4/4 · Waiting for ESP-IDF boot health check'
  };
  if(state==='running'){
    browserUsbMessage(deployment.message||'Installing firmware…');
    // No fake upload percentages: urllib transfers the entire image in one
    // blocking operation. The progress remains indeterminate until proven.
    rfFlashSetProgress(null,ota?(stages[phase]||'OTA installing…'):'USB flashing…');
  }else if(state==='success' && ota && deployment.verified===true){
    const version=deployment.running_version||deployment.target_version||'RFLink';
    browserUsbMessage('OTA verified: ESP32 rebooted into '+version+
      ' and passed its boot health check.');
    rfFlashSetProgress(100,'Verified · new firmware running · health VALID');
    rfFlashHostActive=false;
  }else if(state==='success'){
    browserUsbMessage('USB image written. Looking for the exact ESP32 hardware ID…');
    rfFlashSetProgress(null,'USB written · waiting for matching ESP32 online');
    rfFlashHostActive=false;
    rfFlashHostWaitForIdentity(deployment);
  }else if(state==='rolled_back'){
    browserUsbMessage('OTA rollback suspected: '+(deployment.error||'Previous firmware is running.'));
    rfFlashSetProgress(null,'Rollback detected · review serial diagnosis');
    rfFlashHostActive=false;
  }else if(state==='unverified'){
    browserUsbMessage('OTA written, but reboot and running firmware could not be verified.');
    rfFlashSetProgress(null,'Not verified · open RFLink diagnosis');
    rfFlashHostActive=false;
  }else if(state==='failed'){
    browserUsbMessage('Installation failed: '+(deployment.error||'See console.'));
    rfFlashSetProgress(null,'Installation failed');
    rfFlashHostActive=false;
  }
}
async function rfFlashStopSerialMonitor(){
  rfFlashMonitorActive=false;
  const reader=rfFlashMonitorReader;
  if(reader){try{await reader.cancel();}catch{}}
  // The monitor loop releases its own lock before closing.
  rfFlashStop.hidden=true;
}
async function rfFlashSerialAfterInstall(port){
  rfFlashMonitorActive=true;
  rfFlashMonitorPort=port;
  rfFlashStop.hidden=false;
  rfFlashAppend('\n— Firmware written. Attaching ESP32 serial at 115200 baud. —\n');
  try{
    await rfSerialMonitorWithRecovery(port,
      ()=>rfFlashMonitorActive&&browserUsbDialog.open,rfFlashAppend,
      (event,detail)=>{
        if(event==='connected'){
          browserUsbMessage('Flash completed · '+(detail>1?'USB reconnected':'USB monitor attached')+
            ' · live ESP32 output');
          rfFlashAppend('— USB monitor '+(detail>1?'reconnected':'connected')+
            ' · press ESP32 reset to capture a full boot —\n');
        }else if(event==='lost'){
          rfFlashAppend('— ESP32 serial disconnected; waiting for the same approved port —\n');
        }else if(event==='retry'&&detail%5===1){
          browserUsbMessage('USB monitor reconnecting ('+detail+'s / 120s)…');
        }else if(event==='error'){
          rfFlashAppend('— USB serial error: '+detail+' —\n');
        }else if(event==='timeout'){
          rfFlashAppend('— USB reconnect timed out; use Open diagnosis to connect again —\n');
          browserUsbMessage('ESP32 USB not available. Open Diagnosis and select its COM port.');
        }
      },
      reader=>{rfFlashMonitorReader=reader;},
      nextPort=>{rfFlashMonitorPort=nextPort;});
  }finally{
    rfFlashMonitorActive=false;
    rfFlashMonitorPort=null;
    rfFlashMonitorReader=null;
    rfFlashStop.hidden=true;
  }
}
function rfFlashWaitForHardware(configId, hardwareId, expectedVersion){
  // A completed serial write is only a first step. Query the locally stored
  // device state until the very same ESP32 reports the intended firmware.
  const stopAt=Date.now()+120000;
  const poll=async()=>{
    if(rfFlashIdentityPolling)return;
    if(!browserUsbDialog.open){clearInterval(rfFlashIdentityTimer);rfFlashIdentityTimer=null;return;}
    rfFlashIdentityPolling=true;
    try{
      const state=await api('api/devices');
      const saved=state.devices?.find(d=>d.id===configId);
      if(saved?.online && saved.physical_id===hardwareId && saved.firmware===expectedVersion){
        rfFlashAppend('ESP32 online: '+hardwareId+' · '+expectedVersion+
          ' · linked to the original RFLink configuration.\n');
        rfFlashSetProgress(100,'ESP32 online · device identity and firmware verified');
        browserUsbMessage('RFLink is back online and automatically linked to its saved configuration.');
        clearInterval(rfFlashIdentityTimer);rfFlashIdentityTimer=null;
        await refreshDevices();
      }else if(Date.now()>=stopAt){
        rfFlashAppend('No matching ESP32 response in the initial 2-minute window. '+
          'The saved hardware ID remains pending; the Builder will link it on a later scan.\n');
        browserUsbMessage('Firmware written, waiting for the ESP32. Check Wi-Fi/VLAN and Open diagnosis.');
        rfFlashSetProgress(null,'Not yet online · hardware ID saved for later detection');
        clearInterval(rfFlashIdentityTimer);rfFlashIdentityTimer=null;
      }
    }catch(e){
      if(Date.now()>=stopAt){
        rfFlashAppend('RFLink discovery status unavailable: '+e.message+'\n');
        clearInterval(rfFlashIdentityTimer);rfFlashIdentityTimer=null;
      }
    }finally{rfFlashIdentityPolling=false;}
  };
  rfFlashAppend('Waiting for '+hardwareId+' to announce '+expectedVersion+
    ' via the RFLink network API…\n');
  rfFlashSetProgress(null,'Firmware written · waiting for ESP32 to come online');
  rfFlashIdentityTimer=setInterval(poll,3000);
  poll();
}
async function rfNativeFlashClick(){
  // The first awaited browser operation MUST be requestPort(), initiated by
  // a real user click. No intermediate fetch/promise before the permission UI.
  if(rfFlashBusy||!browserPrepared)return;
  if(!installConfirm.checked){
    browserUsbMessage('Confirm firmware overwrite in the RFLink Firmware tab before selecting a COM port.');
    return;
  }
  if(!rfBrowserSerialAvailable()){
    browserUsbMessage('Web Serial is not allowed here. Use the secure legacy flasher.');
    return;
  }
  if(rfDiagReading){
    browserUsbMessage('Disconnect the USB diagnosis console before flashing.');
    return;
  }
  let port;
  try{
    port=await navigator.serial.requestPort();
  }catch(error){
    browserUsbMessage(error.name==='NotFoundError'?
      'No COM port selected; nothing was written.':
      'USB port selection failed: '+error.message);
    return;
  }
  rfFlashBusy=true;
  rfFlashNative.disabled=true;
  rfFlashNative.hidden=true;
  browserUsbExternal.hidden=true;
  browserFlashCleanup=()=>{}; // Keeps the modal open during the write operation.
  rfFlashSetProgress(null,'Connecting to ESP32-S3…');
  browserUsbMessage('USB port selected. Flashing automatically without additional dialogs.');
  rfFlashAppend('Selected USB port · checking ESP32-S3 chip\n');
  let transport=null,flashed=false,hardwareId=null;
  const prepared=browserPrepared;
  try{
    const device=currentDetailDevice();
    if(!device||!deviceBuildMatches(device,savedBuildForDevice(device))||
       device.id!==browserPrepared.deviceId){
      throw new Error('RFLink firmware selection changed. Reopen installation.');
    }
    // Re-check the already verified build on the server, not the private NVS
    // image content. No extra confirmation dialog is necessary here.
    await activateDeviceFirmware(device);
    const {ESPLoader,Transport}=await import('./vendor/esptool-js/bundle.js');
    transport=new Transport(port,false);
    const loader=new ESPLoader({
      transport,baudrate:115200,debugLogging:false,
      terminal:{
        clean(){rfFlashReplace('');},
        writeLine(line){rfFlashAppend(String(line)+'\n');},
        write(text){rfFlashAppend(String(text));}
      }
    });
    const chip=await loader.main();
    rfFlashAppend('Detected: '+chip+'\n');
    if(!String(chip).includes('ESP32-S3')){
      throw new Error('Expected ESP32-S3, detected '+chip+'. Flash aborted.');
    }
    // ESP-IDF uses esp_read_mac(..., ESP_MAC_WIFI_STA), derived from this
    // immutable efuse base MAC. Read it BEFORE erasing anything.
    const mac=String(await loader.chip.readMac(loader)).toLowerCase();
    if(!/^[0-9a-f]{2}(?::[0-9a-f]{2}){5}$/.test(mac)){
      throw new Error('The ESP32 hardware MAC is unavailable; auto-binding cannot be made safe.');
    }
    hardwareId='rf-'+mac.replaceAll(':','');
    if(device.physical_id?.startsWith('rf-') && device.physical_id!==hardwareId){
      throw new Error('Wrong ESP32 selected: '+hardwareId+
        ' is not the hardware linked to '+device.name+'. No flash was written.');
    }
    rfFlashAppend('Physical ESP32 identity: '+hardwareId+
      ' · will be bound only after this image is written and the board returns online.\n');
    rfFlashSetProgress(0,'Erasing and writing verified factory firmware…');
    const binary=new Uint8Array(browserPrepared.image);
    if(binary.length<0x20000||binary.length>16*1024*1024||binary[0]!==0xe9){
      throw new Error('Prepared factory image is invalid');
    }
    await loader.writeFlash({
      fileArray:[{address:0,data:binary}],
      flashMode:'dio',flashFreq:'40m',flashSize:'16MB',
      eraseAll:true,compress:true,
      reportProgress(_index,written,total){
        const value=total>0?written/total*100:0;
        rfFlashSetProgress(value,'Writing ESP32-S3 firmware…');
      }
    });
    flashed=true;
    rfFlashSetProgress(100,'Firmware written · starting ESP32');
    rfFlashAppend('Flash write completed · actual boot must still be verified\n');
    try{await loader.after('hard_reset');}
    catch(e){rfFlashAppend('Automatic reset: '+e.message+'\n');}
  }catch(error){
    browserUsbMessage('USB flash failed: '+error.message);
    rfFlashAppend('ERROR: '+error.message+'\n');
    rfFlashSetProgress(null,'Flash failed · check console');
  }finally{
    if(transport){
      try{await transport.disconnect();}catch(e){
        rfFlashAppend('USB disconnect: '+e.message+'\n');
      }
    }
    rfFlashBusy=false;
    browserFlashCleanup=null;
    rfFlashNative.disabled=false;
    if(!flashed)rfFlashNative.hidden=false;
    browserUsbExternal.hidden=false;
  }
  if(flashed){
    browserUsbExternal.hidden=true; // A live serial monitor owns the port now.
    if(/^0\.7\.0-exp\.[12]-/.test(String(prepared.version||''))){
      // Published Experimental 1 and 2 predate the hardware-ID status field.
      // Don't arm a watcher that can never be satisfied by that firmware.
      rfFlashAppend('This older Experimental firmware does not report hardware IDs. '+
        'To auto-link, flash Experimental 3 or newer. For this image, use the legacy '+
        'Link RFLink option once it appears on the LAN.\n');
      browserUsbMessage('Firmware written. This older release needs a one-time manual link.');
    }else{
      try{
        await fwRequest('api/devices/usb-flash/identify',{
          device_id:prepared.deviceId,job_id:prepared.jobId,hardware_id:hardwareId
        });
        rfFlashAppend('Hardware '+hardwareId+
          ' registered for automatic RFLink discovery. No manual Link step needed.\n');
        rfFlashWaitForHardware(prepared.deviceId,hardwareId,prepared.version);
      }catch(error){
        rfFlashAppend('Flash completed, but automatic discovery registration failed: '+
          error.message+'\nUse manual Link in Advanced if necessary.\n');
        browserUsbMessage('Flash written; automatic device linking not yet registered.');
      }
    }
    rfFlashSerialAfterInstall(port); // Read-only; user can stop/close.
  }
}
rfFlashNative.addEventListener('click',rfNativeFlashClick);
rfFlashStop.addEventListener('click',rfFlashStopSerialMonitor);
document.getElementById('browser-usb-copy-log').addEventListener('click',async()=>{
  try{await navigator.clipboard.writeText(rfFlashLog);}
  catch(error){browserUsbMessage('Copy failed: '+error.message);}
});
document.getElementById('browser-usb-save-log').addEventListener('click',()=>{
  const url=URL.createObjectURL(new Blob([rfFlashLog],{type:'text/plain;charset=utf-8'}));
  const a=document.createElement('a');a.href=url;
  a.download='RFLink_flash_'+new Date().toISOString().replace(/[:.]/g,'-')+'.txt';
  document.body.append(a);a.click();a.remove();
  setTimeout(()=>URL.revokeObjectURL(url),5000);
});
