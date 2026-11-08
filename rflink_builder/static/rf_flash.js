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
  rfFlashHostActive=method!=='browser';
  document.getElementById('browser-usb-heading').textContent=
    method==='browser'?'USB · This PC':method==='usb'?'USB · Home Assistant':'OTA · Network';
  document.getElementById('browser-usb-subtitle').textContent=
    'RFLink '+(name||'device')+' · verified firmware installation';
  rfFlashLog='';rfFlashConsole.textContent='';
  rfFlashSetProgress(null,'Preparing firmware…');
  rfFlashStop.hidden=true;
  if(!browserUsbDialog.open)browserUsbDialog.showModal();
}
function rfFlashHostStart(method,device){
  rfFlashOpen(method,device.name);
  browserUsbDirect.hidden=true;browserUsbExternal.hidden=true;
  rfFlashNative.hidden=true;
  browserUsbMessage('Installation started. Keep the device powered.');
  rfFlashAppend('— '+(method==='ota'?'Network OTA':'Home Assistant USB')+
    ' installation · waiting for verified Builder log —\n');
}
function rfFlashHostUpdate(deployment,log){
  if(!rfFlashHostActive)return;
  const state=deployment?.status||'idle';
  if(typeof log==='string')rfFlashReplace(log);
  if(state==='running'){
    browserUsbMessage(deployment.message||'Installing firmware…');
    rfFlashSetProgress(null,'Flashing…');
  }else if(state==='success'){
    browserUsbMessage('Installation command finished. Verify actual ESP32 boot.');
    rfFlashSetProgress(100,'Command completed · hardware boot not verified');
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
  rfFlashAppend('\n— Firmware written. Attaching ESP32 serial monitor at 115200 baud… —\n');
  try{
    await port.open({baudRate:115200});
    browserUsbMessage('Flash completed · live ESP32 serial monitor connected');
    const decoder=new TextDecoder();
    while(rfFlashMonitorActive && port.readable){
      const reader=port.readable.getReader();rfFlashMonitorReader=reader;
      try{
        while(rfFlashMonitorActive){
          const {value,done}=await reader.read();
          if(done)break;
          if(value)rfFlashAppend(decoder.decode(value,{stream:true}));
        }
      }finally{
        try{reader.releaseLock();}catch{}
        if(rfFlashMonitorReader===reader)rfFlashMonitorReader=null;
      }
      break;
    }
    const end=decoder.decode();if(end)rfFlashAppend(end);
  }catch(error){
    rfFlashAppend('USB monitor unavailable after reboot: '+error.message+
      '\nUse Open diagnosis to reconnect if the ESP32 port changes.\n');
  }finally{
    rfFlashMonitorActive=false;
    try{await port.close();}catch{}
    if(rfFlashMonitorPort===port)rfFlashMonitorPort=null;
    rfFlashStop.hidden=true;
  }
}
async function rfNativeFlashClick(){
  // The first awaited browser operation MUST be requestPort(), initiated by
  // a real user click. No intermediate fetch/promise before the permission UI.
  if(rfFlashBusy||!browserPrepared)return;
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
  let transport=null,flashed=false;
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
  if(flashed)rfFlashSerialAfterInstall(port); // Read-only, user can stop/close.
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
