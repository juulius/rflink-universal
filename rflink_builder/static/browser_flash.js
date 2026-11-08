/* Browser-PC USB flashing: an in-app Web Serial dialog where permitted.
 * ESP Web Tools 10.4.0 is vendored locally under static/vendor/esp-web-tools,
 * so the private NVS factory image never needs an external JavaScript CDN.
 * On HTTP, or where Home Assistant's iframe forbids serial, explicitly offer
 * the official HTTPS ESPHome Web handoff. Never mark hardware provisioned
 * solely from a browser flash-completion notification.
 */
let browserFlashBusy=false;
let browserFlashCleanup=null;
let browserPrepared=null;
const browserUsbDialog=document.getElementById('browser-usb-dialog');
const browserUsbStatus=document.getElementById('browser-usb-status');
const browserUsbDirect=document.getElementById('browser-usb-direct');
const browserUsbExternal=document.getElementById('browser-usb-external');
const browserUsbInstall=document.getElementById('browser-usb-install-button');
const browserUsbLegacy=document.getElementById('browser-usb-legacy');
const browserUsbBlobUrls=[];

function browserUsbMessage(value){
  browserUsbStatus.textContent=value;
  installStatus.textContent=value;
}
function browserUsbAllowed(){
  if(!window.isSecureContext || !navigator.serial)return false;
  const policy=document.permissionsPolicy||document.featurePolicy;
  if(policy && typeof policy.allowsFeature==='function'){
    try{if(!policy.allowsFeature('serial'))return false;}catch{return false;}
  }
  return true;
}
function browserUsbFinish(){
  browserFlashBusy=false;
  const current=currentDetailDevice();
  if(current)renderInstallationControls(current);
}
function browserUsbFlashingInProgress(){
  return Boolean(browserFlashCleanup || document.querySelector('ewt-install-dialog'));
}
async function closeBrowserUsbDialog(){
  if(browserUsbFlashingInProgress()){
    browserUsbMessage('Finish the current flash operation before closing this console.');
    return;
  }
  if(typeof rfFlashMonitorActive!=='undefined' && rfFlashMonitorActive){
    await rfFlashStopSerialMonitor();
  }
  if(browserUsbDialog.open)browserUsbDialog.close();
}
browserUsbDialog.addEventListener('cancel',event=>{
  if(browserUsbFlashingInProgress()){
    event.preventDefault();
    browserUsbMessage('Wait until the flash operation ends before closing.');
  }else if(typeof rfFlashMonitorActive!=='undefined' && rfFlashMonitorActive){
    event.preventDefault();closeBrowserUsbDialog();
  }
});
browserUsbDialog.addEventListener('close',()=>{
  // Don't revoke blob URLs here: legacy ESP Web Tools may still need them.
  if(typeof rfFlashMonitorActive!=='undefined' && rfFlashMonitorActive)rfFlashStopSerialMonitor();
  browserPrepared=null;
  browserUsbFinish();
});
document.getElementById('browser-usb-close').addEventListener('click',closeBrowserUsbDialog);
window.addEventListener('pagehide',()=>{
  for(const url of browserUsbBlobUrls)URL.revokeObjectURL(url);
});

async function browserUsbImage(d,build){
  await activateDeviceFirmware(d);
  const response=await fetch('api/browser-flash/image',{
    method:'POST',cache:'no-store',credentials:'same-origin',
    headers:{'Content-Type':'application/json','X-RFLink-CSRF':csrf},
    // The user's explicit checkbox is checked before this call. The server's
    // existing TESTBOARD confirmation still protects against blind requests.
    body:JSON.stringify({device_id:d.id,job_id:build.job_id,confirm:'TESTBOARD'})
  });
  if(!response.ok){
    let message='Could not prepare the USB firmware image.';
    try{const result=await response.json();message=result.error||message;}catch{}
    throw new Error(message);
  }
  if(response.headers.get('X-RFLink-Factory-Image')!=='ESP32-S3-PRIVATE-TESTBOARD'){
    throw new Error('The Builder returned an unexpected flash image type.');
  }
  const data=await response.arrayBuffer();
  if(data.byteLength<0x20000||data.byteLength>16*1024*1024||
     new Uint8Array(data)[0]!==0xe9){
    throw new Error('The ESP32-S3 factory image is incomplete or invalid.');
  }
  return data;
}
async function browserUsbLibraryLoaded(){
  if(customElements.get('esp-web-install-button'))return true;
  return await Promise.race([
    customElements.whenDefined('esp-web-install-button').then(()=>true),
    new Promise(resolve=>setTimeout(()=>resolve(false),8000))
  ]);
}
async function installBrowserFirmware(d){
  if(browserFlashBusy)return;
  const build=savedBuildForDevice(d);
  if(!build||inlineInstall.dataset.jobId!==build.job_id){
    installStatus.textContent='First build matching firmware for this RFLink.';
    return;
  }
  if(!installConfirm.checked){
    installStatus.textContent='Acknowledge that the USB installation may replace existing firmware.';
    return;
  }
  browserFlashBusy=true;
  browserPrepared=null;
  browserUsbLegacy.open=false;
  browserUsbLegacy.hidden=false;
  browserUsbInstall.removeAttribute('manifest');
  browserUsbInstall.manifest='';
  browserUsbDirect.hidden=true;
  browserUsbExternal.hidden=true;
  rfFlashNative.hidden=true;
  rfFlashOpen('browser',d.name);
  browserUsbMessage('Checking the selected device and preparing the private USB image…');
  rfFlashAppend('Checking selected RFLink firmware, GPIO profile, source commit and SHA-256…\n');
  renderInstallationControls(d);
  try{
    const image=await browserUsbImage(d,build);
    if(!browserUsbDialog.open)throw new Error('USB installer was closed.');
    if(!deviceBuildMatches(d,build) || inlineInstall.dataset.jobId!==build.job_id){
      throw new Error('Device settings changed. Rebuild before flashing.');
    }
    browserPrepared={deviceId:d.id,jobId:build.job_id,image,name:d.name,
      release:firmwareReleaseLabel(build.release_tag),version:build.version};
    browserUsbExternal.hidden=false;
    browserUsbDirect.hidden=false; // Fallback disclosure is also available if Web Serial is blocked.
    if(!browserUsbAllowed()){
      rfFlashAppend('Browser / Home Assistant Ingress does not grant Web Serial.\n');
      browserUsbLegacy.open=true;
      browserUsbMessage('Web Serial is blocked by this browser or HA Ingress. Use the secure external fallback.');
      return;
    }
    rfFlashNative.hidden=false;
    browserUsbDirect.hidden=false;
    rfFlashSetProgress(null,'Verified firmware ready · select USB port');
    browserUsbMessage('Ready. Click Choose COM port: flashing starts immediately after selection.');
    rfFlashAppend('Firmware verified · ready for ESP32-S3 USB port selection.\n');
  }catch(error){
    browserUsbMessage('USB installation not prepared: '+error.message);
    browserUsbFinish();
  }
}


// Prepare the old ESP Web Tools installer only when a user deliberately opens
// the fallback. Never create private, credential-filled Blob URLs otherwise.
browserUsbLegacy.addEventListener('toggle',()=>{
  if(!browserUsbLegacy.open || !browserPrepared)return;
  if(browserUsbFlashingInProgress()){
    browserUsbMessage('Finish the current flash before switching installers.');
    return;
  }
  const prepared=browserPrepared;
  browserUsbLibraryLoaded().then(ready=>{
    if(!ready || !browserUsbDialog.open || !browserUsbLegacy.open || browserPrepared!==prepared){
      browserUsbMessage('Legacy in-app installer unavailable; the secure external fallback remains available.');
      return;
    }
    const imageUrl=URL.createObjectURL(new Blob([prepared.image],{type:'application/octet-stream'}));
    browserUsbBlobUrls.push(imageUrl);
    const manifest={
      name:'RFLink Universal Radioactive',version:prepared.release||'Experimental',
      new_install_prompt_erase:true,new_install_improv_wait_time:0,
      builds:[{chipFamily:'ESP32-S3',improv:false,parts:[{path:imageUrl,offset:0}]}]
    };
    const manifestUrl=URL.createObjectURL(new Blob([JSON.stringify(manifest)],{type:'application/json'}));
    browserUsbBlobUrls.push(manifestUrl);
    browserUsbInstall.manifest=manifestUrl;
    browserUsbInstall.setAttribute('manifest',manifestUrl);
  });
});

// The separate HTTPS tab is a deliberate FALLBACK, started only by the user.
// Retain the official ESPHome v1 postMessage protocol, strict origin and source
// checks, timeout, progress feedback, and Transferable ArrayBuffer handoff.
async function launchExternalBrowserUsb(){
  if(!browserPrepared){
    browserUsbMessage('No verified firmware image is ready. Reopen USB installation.');
    return;
  }
  if(browserUsbFlashingInProgress()){
    browserUsbMessage('Finish or close the embedded flasher before opening another USB installer.');
    return;
  }
  const tab=window.open('about:blank','_blank');
  if(!tab){
    browserUsbMessage('The browser blocked the secure USB tab. Allow popups and retry.');
    return;
  }
  const flasherUrl='https://web.esphome.io/';
  const origin=new URL(flasherUrl).origin;
  const nonce=Array.from(crypto.getRandomValues(new Uint8Array(16)),
    value=>value.toString(16).padStart(2,'0')).join('');
  let image=browserPrepared.image.slice(0);
  let sent=false,finished=false,readyTimer=null,closePoll=null;
  const cleanup=()=>{
    if(readyTimer!==null)clearTimeout(readyTimer);
    if(closePoll!==null)clearInterval(closePoll);
    window.removeEventListener('message',onMessage);
    if(browserFlashCleanup===cleanup)browserFlashCleanup=null;
    browserUsbFinish();
  };
  const onMessage=event=>{
    if(event.origin!==origin||event.source!==tab)return;
    const msg=event.data;
    if(!msg||typeof msg!=='object')return;
    if(msg.type==='esphome-web-flash:ready'&&!sent){
      if(msg.webSerial===false ||
         (Array.isArray(msg.flashers)&&!msg.flashers.includes('esp'))){
        browserUsbMessage('The secure flasher cannot access Web Serial. Use a supported desktop browser.');
        cleanup();return;
      }
      sent=true;
      try{
        tab.postMessage({
          type:'esphome-web-flash:firmware',version:1,nonce,
          name:'RFLink Universal Radioactive',deviceName:browserPrepared?.name||'RFLink',
          erase:true,flasher:'esp',logs:'flash-port',
          parts:[{address:0,data:image}]
        },origin,[image]);
        image=null;
        if(readyTimer!==null){clearTimeout(readyTimer);readyTimer=null;}
        browserUsbMessage('ESPHome Web is ready. Select the ESP32-S3 COM port in the secure tab.');
      }catch(error){browserUsbMessage('Secure USB transfer failed: '+error.message);cleanup();}
      return;
    }
    if(!sent)return;
    if(msg.type==='esphome-web-flash:progress'&&Number.isFinite(msg.pct)){
      browserUsbMessage('USB flashing '+Math.max(0,Math.min(100,Math.round(msg.pct)))+
        '% · Do not disconnect the ESP32-S3.');
    }
    if(msg.type==='esphome-web-flash:state'){
      if(msg.state==='done'){
        finished=true;
        browserUsbMessage('Flash write complete. Check that RFLink boots and discovers its radios.');
        cleanup();
      }else if(msg.state==='error'){
        browserUsbMessage('USB flasher error: '+(msg.detail||'See the secure installer tab.'));
      }else if(msg.note||msg.detail){
        browserUsbMessage('USB: '+(msg.note||msg.detail));
      }
    }
  };
  window.addEventListener('message',onMessage);
  browserFlashCleanup=cleanup;
  readyTimer=setTimeout(()=>{
    if(!sent&&!finished){
      browserUsbMessage('The secure USB installer did not respond. Check its tab.');
      cleanup();
    }
  },60000);
  closePoll=setInterval(()=>{
    if(tab.closed&&!finished){
      browserUsbMessage(sent?'USB installer closed. Verify whether RFLink boots.':
        'The secure USB installer was closed before firmware transfer.');
      cleanup();
    }
  },1000);
  browserUsbMessage('Opening the official HTTPS ESPHome Web flasher…');
  tab.location.href=flasherUrl+'#nonce='+encodeURIComponent(nonce)+
    '&origin='+encodeURIComponent(window.location.origin);
}
browserUsbExternal.addEventListener('click',launchExternalBrowserUsb);
