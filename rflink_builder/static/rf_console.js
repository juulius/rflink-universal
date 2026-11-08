/* Independent RFLink diagnosis. Never requires a build, OTA key or flash image.
 * USB uses browser Web Serial; Wi-Fi reads native bounded RFLink logbooks through
 * the same-origin, device-identity-checked HA Builder proxy. No raw LAN CORS.
 */
const rfDiagDialog=document.getElementById('rf-diagnostics-dialog');
const rfDiagSource=document.getElementById('rf-diagnostics-source');
const rfDiagStream=document.getElementById('rf-diagnostics-stream');
const rfDiagConnect=document.getElementById('rf-diagnostics-connect');
const rfDiagStop=document.getElementById('rf-diagnostics-stop');
const rfDiagStatus=document.getElementById('rf-diagnostics-status');
const rfDiagLog=document.getElementById('rf-diagnostics-log');
const rfDiagPause=document.getElementById('rf-diagnostics-pause');
const rfDiagFollow=document.getElementById('rf-diagnostics-follow');
let rfDiagDevice=null,rfDiagPort=null,rfDiagReader=null,rfDiagPolling=null;
let rfDiagStopped=true,rfDiagPaused=false,rfDiagAutoscroll=true,rfDiagReading=false;
let rfDiagText='',rfDiagSequence=-1,rfDiagPollActive=false;
const RF_LOG_LIMIT=180000;

function rfNormalizeLog(text){
  return String(text||'').replace(/\x1b\[[0-9;?]*[ -/]*[@-~]/g,'')
    .replace(/\r\n/g,'\n').replace(/\r/g,'\n')
    .replace(/[\x00-\x08\x0b\x0c\x0e-\x1f\x7f]/g,'');
}
function rfLogAdd(text){
  rfDiagText=(rfDiagText+rfNormalizeLog(text)).slice(-RF_LOG_LIMIT);
  if(!rfDiagPaused){
    const follow=rfDiagAutoscroll &&
      rfDiagLog.scrollHeight-rfDiagLog.scrollTop<rfDiagLog.clientHeight+80;
    rfDiagLog.textContent=rfDiagText;
    if(follow)rfDiagLog.scrollTop=rfDiagLog.scrollHeight;
  }
}
function rfDiagStatusSet(text){rfDiagStatus.textContent=text;}
function rfDiagResetText(){
  rfDiagText='';rfDiagLog.textContent='';
  rfDiagSequence=-1;
}
function rfBrowserSerialAvailable(){
  if(!window.isSecureContext||!navigator.serial)return false;
  const policy=document.permissionsPolicy||document.featurePolicy;
  try{
    if(policy?.allowsFeature && !policy.allowsFeature('serial'))return false;
  }catch{return false;}
  return true;
}
function rfDiagUIConnected(active){
  rfDiagConnect.hidden=active;
  rfDiagStop.hidden=!active;
  rfDiagSource.disabled=active;
}
async function rfDiagStopSession(){
  rfDiagStopped=true;
  if(rfDiagPolling!==null){
    clearInterval(rfDiagPolling);rfDiagPolling=null;
  }
  // Cancel a blocked read first, then unlock and close before another
  // operation can request this same COM port.
  const reader=rfDiagReader;
  if(reader){try{await reader.cancel();}catch{}}
  // The active read loop releases its lock and closes the port in finally.
  if(!rfDiagReading && rfDiagPort){
    try{await rfDiagPort.close();}catch{}
    rfDiagPort=null;
  }
  rfDiagUIConnected(false);
}
function rfDiagFormatEvent(e){
  const ms=Number(e.uptime_ms)||0;
  const stamp='+'+(ms/1000).toFixed(3)+'s';
  const kind=String(e.kind||'event');
  const severity=e.severity===2?'ERROR':e.severity===1?'WARN':'INFO';
  const radios=['433.42 MHz','433.92 MHz','868.30 MHz'];
  const radio=Number.isInteger(e.radio)&&e.radio>=0&&e.radio<3?' · '+radios[e.radio]:'';
  const value=Number.isFinite(e.value)&&e.value!==0?' · value '+e.value:'';
  const correlation=e.correlation_id?' · #'+e.correlation_id:'';
  return stamp+'  ['+severity+'] '+String(e.stream||'system').toUpperCase()+
    ' · '+kind+radio+value+correlation+'\n';
}
async function rfDiagFetchWifi(){
  if(rfDiagPollActive||rfDiagStopped||!rfDiagDevice)return;
  rfDiagPollActive=true;
  const queriedDevice=rfDiagDevice;
  const queriedStream=rfDiagStream.value;
  try{
    const response=await api('api/diagnostics/device?device_id='+
      encodeURIComponent(queriedDevice.id)+'&stream='+encodeURIComponent(queriedStream));
    if(rfDiagStopped||rfDiagDevice!==queriedDevice||rfDiagStream.value!==queriedStream)return;
    const events=Array.isArray(response.events)?response.events:[];
    const fresh=events.filter(e=>e&&Number.isSafeInteger(e.sequence)).sort((a,b)=>a.sequence-b.sequence);
    const highest=fresh.length?fresh[fresh.length-1].sequence:-1;
    if(highest>=0&&rfDiagSequence>highest){
      rfLogAdd('— RFLink log sequence restarted; possible reboot —\n');
      rfDiagSequence=-1;
    }
    for(const event of fresh){
      if(event.sequence<=rfDiagSequence)continue;
      rfLogAdd(rfDiagFormatEvent(event));
      rfDiagSequence=event.sequence;
    }
    rfDiagStatusSet('Wi-Fi connected · '+(response.ip||'')+
      ' · '+(response.firmware||'RFLink')+' · stored events (not raw serial)');
  }catch(error){
    if(!rfDiagStopped)rfDiagStatusSet('Wi-Fi diagnosis unavailable: '+error.message);
  }finally{rfDiagPollActive=false;}
}
async function rfDiagConnectSession(){
  if(!rfDiagStopped)return;
  const source=rfDiagSource.value;
  if(source==='wifi'){
    if(!rfDiagDevice){
      rfDiagStatusSet('Select an RFLink device first.');return;
    }
    rfDiagStopped=false;
    rfDiagUIConnected(true);
    rfDiagStatusSet('Contacting RFLink via Home Assistant…');
    rfLogAdd('— Wi-Fi diagnosis: saved RFLink events; early boot output requires USB —\n');
    await rfDiagFetchWifi();
    if(!rfDiagStopped)rfDiagPolling=setInterval(rfDiagFetchWifi,2500);
    return;
  }
  if(typeof rfFlashBusy!=='undefined' && (rfFlashBusy || browserFlashBusy)){
    rfDiagStatusSet('A USB flash session is active. Finish it before opening a serial monitor.');
    return;
  }
  if(!rfBrowserSerialAvailable()){
    rfDiagStatusSet('USB serial not allowed by this browser/HA Ingress frame. Use HTTPS Chrome/Edge or connect USB to the HA host.');
    return;
  }
  let port;
  try{
    // Must happen immediately after the real user click: browser permission
    // cannot be requested from a timer, background refresh or after a fetch.
    port=await navigator.serial.requestPort();
  }catch(error){
    rfDiagStatusSet(error.name==='NotFoundError'?'No USB port chosen.':'USB selection failed: '+error.message);
    return;
  }
  rfDiagStopped=false;rfDiagReading=true;rfDiagPort=port;rfDiagUIConnected(true);
  rfDiagStatusSet('Opening 115200 baud serial monitor…');
  try{
    await port.open({baudRate:115200});
    rfDiagStatusSet('Reading live ESP32 USB serial · 115200 baud');
    rfLogAdd('— USB monitor connected (read-only) —\n');
    const decoder=new TextDecoder('utf-8');
    while(!rfDiagStopped&&port.readable){
      const reader=port.readable.getReader();rfDiagReader=reader;
      try{
        while(!rfDiagStopped){
          const {value,done}=await reader.read();
          if(done)break;
          if(value)rfLogAdd(decoder.decode(value,{stream:true}));
        }
      }finally{
        try{reader.releaseLock();}catch{}
        if(rfDiagReader===reader)rfDiagReader=null;
      }
      break;
    }
    const tail=decoder.decode();if(tail)rfLogAdd(tail);
  }catch(error){
    if(!rfDiagStopped)rfDiagStatusSet('USB monitor stopped: '+error.message);
  }finally{
    rfDiagReading=false;
    try{await port.close();}catch{}
    if(rfDiagPort===port)rfDiagPort=null;
    rfDiagStopped=true;
    rfDiagUIConnected(false);
  }
}
function rfOpenDiagnostics(device){
  // Read-only and independent: no saved build or firmware version required.
  rfDiagDevice=device||null;
  document.getElementById('rf-diagnostics-device').textContent=device?.name||'RFLink';
  rfDiagSource.value=device?.online?'wifi':'usb';
  rfDiagResetText();
  rfDiagStream.value='all';
  rfDiagStream.disabled=rfDiagSource.value==='usb';
  rfDiagPaused=false;
  rfDiagPause.textContent='Pause';
  rfDiagFollow.checked=true;rfDiagAutoscroll=true;
  rfDiagStatusSet('Select a connection and open the console.');
  rfDiagDialog.showModal();
  if(rfDiagSource.value==='wifi')rfDiagConnectSession();
}
rfDiagConnect.addEventListener('click',rfDiagConnectSession);
rfDiagSource.addEventListener('change',()=>{
  rfDiagStream.disabled=rfDiagSource.value==='usb';
});
rfDiagStream.addEventListener('change',()=>{
  if(!rfDiagStopped&&rfDiagSource.value==='wifi'){
    rfDiagSequence=-1;
    rfLogAdd('— Filter switched to '+rfDiagStream.value+' —\n');
    rfDiagFetchWifi();
  }
});
rfDiagStop.addEventListener('click',async()=>{
  await rfDiagStopSession();
  rfDiagStatusSet('Disconnected. The console remains available.');
});
document.getElementById('rf-diagnostics-close').addEventListener('click',async()=>{
  await rfDiagStopSession();rfDiagDialog.close();
});
rfDiagDialog.addEventListener('cancel',event=>{
  if(rfDiagReading){
    event.preventDefault();rfDiagStatusSet('Disconnect the USB monitor before closing.');
    return;
  }
  rfDiagStopSession();
});
rfDiagDialog.addEventListener('close',()=>{rfDiagStopSession();});
rfDiagPause.addEventListener('click',()=>{
  rfDiagPaused=!rfDiagPaused;
  rfDiagPause.textContent=rfDiagPaused?'Resume':'Pause';
  if(!rfDiagPaused){
    rfDiagLog.textContent=rfDiagText;
    if(rfDiagAutoscroll)rfDiagLog.scrollTop=rfDiagLog.scrollHeight;
  }
});
rfDiagFollow.addEventListener('change',()=>{rfDiagAutoscroll=rfDiagFollow.checked;});
document.getElementById('rf-diagnostics-clear').addEventListener('click',rfDiagResetText);
document.getElementById('rf-diagnostics-copy').addEventListener('click',async()=>{
  try{await navigator.clipboard.writeText(rfDiagText);rfDiagStatusSet('Log copied to clipboard.');}
  catch(error){rfDiagStatusSet('Copy unavailable: '+error.message);}
});
document.getElementById('rf-diagnostics-export').addEventListener('click',()=>{
  const blob=new Blob([rfDiagText],{type:'text/plain;charset=utf-8'});
  const url=URL.createObjectURL(blob);
  const a=document.createElement('a');a.href=url;
  a.download='RFLink_diagnosis_'+new Date().toISOString().replace(/[:.]/g,'-')+'.txt';
  document.body.append(a);a.click();a.remove();
  setTimeout(()=>URL.revokeObjectURL(url),5000);
});
document.getElementById('detail-diagnostics').addEventListener('click',()=>{
  const device=currentDetailDevice();if(device)rfOpenDiagnostics(device);
});
