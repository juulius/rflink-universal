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
let rfDiagCrashRetryAt=0,rfDiagCrashFailureNoted=false;
let rfDiagCrashSignature='';
let rfDiagFirmwareHeader='',rfDiagFirmwareSignature='',rfDiagRmtSignature='',rfDiagOokSignature='';
let rfDiagConsoleCursor=0,rfDiagConsoleUnavailable=false,rfDiagConsoleWarning=false;
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
  rfDiagCrashLoaded=false;rfDiagCrashRetryAt=0;rfDiagCrashFailureNoted=false;rfDiagCrashSignature='';
  rfDiagFirmwareSignature='';rfDiagRmtSignature='';rfDiagOokSignature='';
  rfDiagConsoleCursor=0;rfDiagConsoleUnavailable=false;rfDiagConsoleWarning=false;
  document.getElementById('rf-diagnostics-crash-summary').hidden=true;
  // Clearing the scrollback must not silently remove the running build from
  // a subsequent Copy/Export. This is verified live ESP32 data, not a selected
  // Builder release or an untrusted USB adapter's presumed firmware.
  if(rfDiagSource.value==='wifi'&&rfDiagFirmwareHeader){
    rfLogAdd(rfDiagFirmwareHeader);
    rfDiagFirmwareSignature=rfDiagFirmwareHeader;
  }
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

const RF_DIAG_RMT_STAGES=[
  'none','queue allocation','RMT channel allocation','GDO2 pull-down',
  'callback registration','RMT enable','initial RX arm','RX rearm'
];
function rfDiagCaptureRunningBuild(response){
  const version=String(response.firmware||'Unknown').slice(0,96);
  const channel=String(response.release_channel||'Unknown').slice(0,32);
  const build=Number.isInteger(response.release_build)?' · build '+response.release_build:'';
  const board=String(response.board_id||'Unknown').slice(0,64);
  const boardHash=String(response.board_hash||'').slice(0,32);
  const label='— Running ESP32 firmware (verified via live Wi-Fi status, not Builder selection) —\n'+
    'Version: '+version+'\nChannel: '+channel+build+'\nBoard: '+board+
    (boardHash?' · '+boardHash:'')+'\n';
  rfDiagFirmwareHeader=label;
  if(rfDiagFirmwareSignature!==label){
    rfLogAdd('\n'+label);
    rfDiagFirmwareSignature=label;
    rfDiagRmtSignature='';rfDiagOokSignature=''; // Re-report errors after a firmware change.
    rfDiagConsoleUnavailable=false; // New firmware may add the mirror endpoint.
  }
}
const RF_DIAG_OOK_STAGES=[
  'none','mutex allocation','invalid RF profile','CC1101 absent',
  'CC1101 SIDLE','frequency register write','modem register write',
  'modem register read','modem register mismatch','RMT receiver start',
  'CC1101 verified RX entry'
];
function rfDiagCaptureOokFailures(response){
  // RMT stage can be zero if the CC1101 modem setup failed first.
  // Never confuse the high-level ManagerError 7 with this exact stage.
  const radios=[
    {name:'433.42 MHz',stage:response.rx43342_start_error_stage,
      error:response.rx43342_start_error_code,reg:response.rx43342_start_error_reg,
      expected:response.rx43342_start_expected,observed:response.rx43342_start_observed,
      failures:response.rx43342_start_failures},
    {name:'433.92 MHz',stage:response.rx43392_start_error_stage,
      error:response.rx43392_start_error_code,reg:response.rx43392_start_error_reg,
      expected:response.rx43392_start_expected,observed:response.rx43392_start_observed,
      failures:response.rx43392_start_failures}
  ];
  const signature=JSON.stringify(radios);
  if(signature===rfDiagOokSignature)return;
  rfDiagOokSignature=signature;
  for(const r of radios){
    if(!Number.isInteger(r.stage)||r.stage<1||r.stage>=RF_DIAG_OOK_STAGES.length)continue;
    let info='— '+r.name+' CC1101 RX startup: '+RF_DIAG_OOK_STAGES[r.stage]+
      ' · ESP-IDF error '+(Number.isInteger(r.error)?r.error:'unknown');
    if(Number.isInteger(r.reg)&&r.reg>=0){
      info+=' · register 0x'+r.reg.toString(16).padStart(2,'0');
      if(Number.isInteger(r.expected)&&r.expected>=0)info+=' · expected 0x'+r.expected.toString(16);
      if(Number.isInteger(r.observed)&&r.observed>=0)info+=' · read 0x'+r.observed.toString(16);
    }
    rfLogAdd(info+' · failed attempts '+(r.failures??'?')+' —\n');
  }
}

function rfDiagCaptureRmtFailures(response){
  // Last actual driver errors from /api/status. High-level value 7 is only
  // ManagerError::driver_failure; it does not describe an ESP-IDF failure.
  const reports=[
    {name:'433.42 MHz',stage:response.rx43342_rmt_error_stage,
      code:response.rx43342_rmt_error_code,listening:response.rx43342_listening},
    {name:'433.92 MHz',stage:response.rx43392_rmt_error_stage,
      code:response.rx43392_rmt_error_code,listening:response.rx43392_listening}
  ];
  const signature=JSON.stringify(reports);
  if(signature===rfDiagRmtSignature)return;
  const previous=rfDiagRmtSignature;
  rfDiagRmtSignature=signature;
  for(const r of reports){
    if(Number.isInteger(r.stage) && r.stage>=1 && r.stage<RF_DIAG_RMT_STAGES.length){
      rfLogAdd('— '+r.name+' RMT failure: '+RF_DIAG_RMT_STAGES[r.stage]+
        ' · ESP-IDF error '+(Number.isInteger(r.code)?r.code:'unknown')+
        ' · listening='+(r.listening===true?'yes':'no')+' —\n');
    }
  }
  if(previous && reports.every(r=>r.stage===0&&r.listening===true))
    rfLogAdd('— 433 MHz RMT receivers both reporting active (no last driver failure) —\n');
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
  // Keep checking while startup checkpoints advance, then occasionally
  // refresh after boot to discover later resets without log-sequence clues.
  if(rfDiagStopped||!rfDiagDevice||rfDiagCrashBusy||Date.now()<rfDiagCrashRetryAt)return;
  rfDiagCrashBusy=true;
  const device=rfDiagDevice;
  try{
    const report=await api('api/diagnostics/device/crashes?device_id='+
      encodeURIComponent(device.id));
    if(rfDiagStopped||rfDiagDevice!==device)return;
    const history=Array.isArray(report.history)?report.history:[];
    const phases=Array.isArray(report.boot_phases)?report.boot_phases:[];
    const phaseByBoot=new Map(phases.map(e=>[e.boot,e]));
    const latest=history.length?phaseByBoot.get(history[0].boot):null;
    const signature=JSON.stringify({
      last_reset:report.last_reset,history,phases,
      dump:report.core_dump,phase_history_saved:report.phase_history_saved
    });
    // The visual cards update on each fetch, but the console only gains
    // lines when persisted data really changed. No duplicate spam.
    document.getElementById('rf-diag-last-reset').textContent=report.last_reset||'Unknown';
    const dump=report.core_dump||{};
    const stored=dump.integrity==='valid'?
      (dump.summary_available?'Available · '+(dump.task||'unknown task'):'Saved · no summary'):
      (dump.integrity==='invalid_or_unreadable'?'Damaged / unreadable':'No stored crash');
    document.getElementById('rf-diag-dump-state').textContent=stored;
    const checkpoint=latest?
      latest.stage.replaceAll('_',' ')+(latest.startup_complete?' · startup completed':' · last reached')
      :'Not available on this firmware';
    document.getElementById('rf-diag-boot-phase').textContent=checkpoint;
    document.getElementById('rf-diagnostics-crash-summary').hidden=false;
    if(!rfDiagCrashLoaded || signature!==rfDiagCrashSignature){
      rfLogAdd('\n— Persistent ESP32 reset history (not live serial) —\n');
      rfLogAdd('Last reset: '+(report.last_reset||'Unknown')+'\n');
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
      if(report.phase_history_saved!==true)
        rfLogAdd('Warning: boot checkpoints not saved in flash (or unsupported by old firmware).\n');
      for(const entry of history.slice(0,12)){
        const phase=phaseByBoot.get(entry.boot);
        rfLogAdd('#'+entry.boot+' · '+entry.reason+
          (entry.has_core_dump?' · '+entry.task+' PC '+entry.pc:'')+
          (phase?' · '+phase.stage.replaceAll('_',' ')+
            (phase.startup_complete?' (startup completed)':' (last reached)'):'')+'\n');
      }
      if(history.length===0)rfLogAdd('No previous boots stored.\n');
      rfLogAdd('Checkpoint = last reached stage, not a proven crash cause.\n');
      rfDiagCrashSignature=signature;
    }
    rfDiagCrashLoaded=true;
    rfDiagCrashFailureNoted=false;
    // A running boot updates the flash checkpoint a few times. Poll it every
    // 5s until ready; after that an occasional 30s check detects reboots.
    rfDiagCrashRetryAt=Date.now()+(latest && !latest.startup_complete?5000:30000);
  }catch(error){
    if(!rfDiagStopped&&rfDiagDevice===device){
      if(!rfDiagCrashFailureNoted){
        rfLogAdd('— Saved crash history temporarily unavailable: '+
          String(error.message||error).slice(0,150)+
          '. Live logs continue; use USB for pre-network crashes. —\n');
        rfDiagCrashFailureNoted=true;
      }
      rfDiagCrashRetryAt=Date.now()+15000;
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
    rfDiagCaptureRunningBuild(response);
    // USB and OTA now share the firmware's ESP-IDF text logger. Older native
    // firmware has no authenticated endpoint: keep structured events useful.
    if(!rfDiagConsoleUnavailable){
      try{
        const raw=await api('api/diagnostics/device/console?device_id='+
          encodeURIComponent(queriedDevice.id)+'&since='+rfDiagConsoleCursor);
        if(rfDiagStopped||rfDiagDevice!==queriedDevice)return;
        if(raw.cursor<rfDiagConsoleCursor){
          rfLogAdd('— ESP32 restarted: console cursor reset —\n');
        }
        if(raw.truncated)rfLogAdd('— Console ring overwritten; earlier lines no longer in RAM —\n');
        rfDiagConsoleCursor=raw.cursor;
        if(raw.text)rfLogAdd(raw.text);
      }catch(error){
        rfDiagConsoleUnavailable=true;
        if(!rfDiagConsoleWarning){
          rfLogAdd('— Detailed ESP-IDF console unavailable on this firmware or '+
            'management key; structured events and saved boot history continue. '+
            String(error.message||error).slice(0,100)+' —\n');
          rfDiagConsoleWarning=true;
        }
      }
    }
    rfDiagCaptureOokFailures(response);
    rfDiagCaptureRmtFailures(response);
    const events=Array.isArray(response.events)?response.events:[];
    const fresh=events.filter(e=>e&&Number.isSafeInteger(e.sequence)).sort((a,b)=>a.sequence-b.sequence);
    const highest=fresh.length?fresh[fresh.length-1].sequence:-1;
    if(highest>=0&&rfDiagSequence>highest){
      rfLogAdd('— RFLink log sequence restarted; possible reboot —\n');
      rfDiagSequence=-1;
      rfDiagCrashLoaded=false;
      rfDiagCrashRetryAt=0;
    }
    for(const event of fresh){
      if(event.sequence<=rfDiagSequence)continue;
      rfLogAdd(rfDiagFormatEvent(event));
      rfDiagSequence=event.sequence;
    }
    rfDiagFetchCrashHistory();
    const radioState=response.radio_initialized===false?
      ' · radio initialization pending · diagnosis online':
      response.startup_complete===false?
      ' · finishing startup / OTA checks · diagnosis online':
      ' · ESP-IDF console + structured events';
    rfDiagStatusSet('Wi-Fi connected · '+(response.ip||'')+
      ' · '+(response.firmware||'RFLink')+radioState);
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
    rfLogAdd('— Wi-Fi diagnosis: ESP-IDF application console, structured events and saved boot history. Boot ROM and crashes before network access still require USB. —\n');
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
  rfLogAdd('— USB serial: firmware build is not verified until the ESP32 reports it. —\n');
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
  rfDiagFirmwareHeader=''; // Never carry firmware identity to another device.
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
  rfDiagFirmwareHeader=''; // In USB mode, never assume the Wi-Fi device is this port.
  rfDiagResetText();
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
