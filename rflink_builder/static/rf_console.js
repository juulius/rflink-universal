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
let rfDiagCrashLoaded=false,rfDiagCrashBusy=false;
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
  rfDiagCrashLoaded=false;
  document.getElementById('rf-diagnostics-crash-summary').hidden=true;
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

/* One read-only monitor for both Diagnosis and post-flash output. It retries
 * for up to 120 seconds after lost USB serial. No auto-flash or control-signal
 * changes. A new port is selected silently only if exactly one already-granted
 * port has the same VID/PID; identical adapters require a fresh user choice.
 */
async function rfSerialMonitorWithRecovery(firstPort, running, bytes, state, readerChanged, portChanged){
  let port=firstPort,attempts=0,connections=0,hadLoss=false;
  const firstInfo=typeof port.getInfo==='function'?port.getInfo():null;
  while(running()&&attempts<120){
    let decoder=new TextDecoder('utf-8');
    try{
      await port.open({baudRate:115200});
      connections++;
      attempts=0;
      hadLoss=false;
      state('connected',connections);
      if(!port.readable)throw new Error('The USB serial stream is not readable');
      const reader=port.readable.getReader();
      readerChanged(reader);
      try{
        while(running()){
          const {value,done}=await reader.read();
          if(done)break;
          if(value)bytes(decoder.decode(value,{stream:true}));
        }
      }finally{
        try{reader.releaseLock();}catch{}
        readerChanged(null);
      }
    }catch(error){
      if(running()&&!hadLoss)state('error',String(error.message||error).slice(0,180));
    }finally{
      const tail=decoder.decode();if(tail)bytes(tail);
      readerChanged(null);
      try{await port.close();}catch{}
    }
    if(!running())break;
    attempts++;
    if(!hadLoss)state('lost',connections);
    hadLoss=true;
    if(attempts===1||attempts%10===0)state('retry',attempts);
    await new Promise(resolve=>setTimeout(resolve,1000));
    if(!running())break;
    try{
      const granted=await navigator.serial.getPorts();
      if(granted.includes(port))continue;
      if(!firstInfo||!Number.isInteger(firstInfo.usbVendorId)||
         !Number.isInteger(firstInfo.usbProductId))continue;
      const matches=granted.filter(candidate=>{
        const info=typeof candidate.getInfo==='function'?candidate.getInfo():null;
        return info?.usbVendorId===firstInfo.usbVendorId &&
          info?.usbProductId===firstInfo.usbProductId;
      });
      // Do not conflate two USB boards with the same vendor/product ID.
      if(matches.length===1){
        port=matches[0];portChanged(port);
      }
    }catch{
      // Browser/permission or temporary re-enumeration failures are retried.
    }
  }
  if(running()&&attempts>=120)state('timeout',attempts);
}

async function rfDiagFetchCrashHistory(){
  if(rfDiagStopped||!rfDiagDevice||rfDiagCrashLoaded||rfDiagCrashBusy)return;
  rfDiagCrashBusy=true;
  const device=rfDiagDevice;
  try{
    const report=await api('api/diagnostics/device/crashes?device_id='+
      encodeURIComponent(device.id));
    if(rfDiagStopped||rfDiagDevice!==device)return;
    rfLogAdd('\n— Persistent ESP32 reset history (not live serial) —\n');
    rfLogAdd('Last reset: '+(report.last_reset||'Unknown')+'\n');
    document.getElementById('rf-diag-last-reset').textContent=report.last_reset||'Unknown';
    const dump=report.core_dump||{};
    const stored=dump.integrity==='valid'?
      (dump.summary_available?'Available · '+(dump.task||'unknown task'):'Saved · no summary'):
      (dump.integrity==='invalid_or_unreadable'?'Damaged / unreadable':'No stored crash');
    document.getElementById('rf-diag-dump-state').textContent=stored;
    document.getElementById('rf-diagnostics-crash-summary').hidden=false;
    if(dump.integrity==='valid'){
      rfLogAdd('Stored crash: '+(dump.summary_available?
        (dump.task||'unknown task')+' · PC '+(dump.pc||'unknown'):'summary unavailable')+
        ' · build '+(dump.build_comparison||'unknown')+'\n');
      rfLogAdd('Note: a stored crash can predate the latest normal restart.\n');
    }else if(dump.integrity==='invalid_or_unreadable'){
      rfLogAdd('Stored crash: invalid/unreadable; reset reason may still be available.\n');
    }else{
      rfLogAdd('Stored crash: none available.\n');
    }
    if(report.history_saved!==true)rfLogAdd('Warning: reset history was not saved to flash.\n');
    const history=Array.isArray(report.history)?report.history:[];
    for(const entry of history.slice(0,12)){
      rfLogAdd('#'+entry.boot+' · '+entry.reason+
        (entry.has_core_dump?' · '+entry.task+' PC '+entry.pc:'')+'\n');
    }
    if(history.length===0)rfLogAdd('No previous boots stored.\n');
    rfDiagCrashLoaded=true;
  }catch(error){
    if(!rfDiagStopped&&rfDiagDevice===device){
      rfLogAdd('— Saved crash history unavailable: '+String(error.message||error).slice(0,150)+
        '. Live logs continue; use USB for pre-network crashes. —\n');
      rfDiagCrashLoaded=true;
    }
  }finally{rfDiagCrashBusy=false;}
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
      rfDiagCrashLoaded=false;
    }
    for(const event of fresh){
      if(event.sequence<=rfDiagSequence)continue;
      rfLogAdd(rfDiagFormatEvent(event));
      rfDiagSequence=event.sequence;
    }
    rfDiagFetchCrashHistory();
    rfDiagStatusSet('Wi-Fi connected · '+(response.ip||'')+
      ' · '+(response.firmware||'RFLink')+' · stored events (not raw serial)');
  }catch(error){
    if(!rfDiagStopped)rfDiagStatusSet('Wi-Fi diagnosis unavailable: '+error.message);
  }finally{rfDiagPollActive=false;}
}
async function rfDiagConnectSession(){
  if(!rfDiagStopped||rfDiagReading)return;
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
  rfDiagConnect.disabled=true;
  rfDiagStatusSet('Opening 115200 baud serial monitor…');
  rfLogAdd('— USB selected. Press the ESP32 reset button to capture the complete boot. —\n');
  try{
    await rfSerialMonitorWithRecovery(port,()=>!rfDiagStopped,rfLogAdd,
      (event,detail)=>{
        if(event==='connected'){
          rfDiagStatusSet('Reading ESP32 serial · 115200 baud'+
            (detail>1?' · reconnected after reset':''));
          rfLogAdd('— USB monitor '+(detail>1?'reconnected':'connected')+
            ' (read-only) —\n');
        }else if(event==='lost'){
          rfLogAdd('— USB stream disconnected; waiting for the same approved port —\n');
        }else if(event==='retry'){
          rfDiagStatusSet('ESP32 USB disconnected · reconnecting ('+detail+'s / 120s)…');
        }else if(event==='error'){
          rfLogAdd('— USB serial error: '+detail+' —\n');
        }else if(event==='timeout'){
          rfDiagStatusSet('USB reconnect timed out. Choose Connect again to resume.');
          rfLogAdd('— No approved USB serial connection after 120 seconds —\n');
        }
      },reader=>{rfDiagReader=reader;},nextPort=>{rfDiagPort=nextPort;});
  }finally{
    rfDiagReading=false;
    rfDiagPort=null;
    rfDiagStopped=true;
    rfDiagConnect.disabled=false;
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
  document.getElementById('rf-diagnostics-crash-summary').hidden=rfDiagSource.value==='usb';
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
