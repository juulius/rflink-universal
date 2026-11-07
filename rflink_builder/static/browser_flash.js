/* Browser-PC USB flashing via ESPHome Web's versioned Web Serial handoff.
 * The firmware bytes stay in this browser; no firmware is uploaded to an
 * external server. A trusted HTTPS flasher tab receives the bytes via
 * postMessage, and can read the embedded unencrypted first-boot credentials.
 * Never mark the gateway provisioned before independently verifying its boot.
 */
let browserFlashBusy=false;
let browserFlashCleanup=null;

async function installBrowserFirmware(d){
  if(browserFlashBusy)return;
  const build=savedBuildForDevice(d);
  if(!build || inlineInstall.dataset.jobId!==build.job_id){
    installStatus.textContent='Build compatible firmware for this RFLink first.';
    return;
  }
  if(installConfirm.value.trim()!=='TESTBOARD'){
    installStatus.textContent='Enter TESTBOARD before replacing the flash on the ESP32-S3.';
    return;
  }
  // Must open synchronously from the click so the browser does not block it.
  const tab=window.open('about:blank','_blank');
  if(!tab){
    installStatus.textContent='Pop-up blocked. Allow the USB installer tab and try again.';
    return;
  }
  browserFlashBusy=true;
  renderInstallationControls(d);
  installStatus.textContent='Verifying private ESP32-S3 firmware for browser USB…';
  try{
    await activateDeviceFirmware(d);
    const response=await fetch('api/browser-flash/image',{
      method:'POST',cache:'no-store',credentials:'same-origin',
      headers:{'Content-Type':'application/json','X-RFLink-CSRF':csrf},
      body:JSON.stringify({device_id:d.id,job_id:build.job_id,confirm:'TESTBOARD'})
    });
    if(!response.ok){
      let message='Could not prepare browser USB image';
      try{const error=await response.json();message=error.error||message;}catch{}
      throw new Error(message);
    }
    if(response.headers.get('X-RFLink-Factory-Image')!=='ESP32-S3-PRIVATE-TESTBOARD'){
      throw new Error('Unexpected firmware package type; USB install blocked.');
    }
    let image=await response.arrayBuffer();
    if(image.byteLength<0x20000||image.byteLength>16*1024*1024||
       new Uint8Array(image)[0]!==0xe9){
      throw new Error('The ESP32-S3 first-install image is invalid.');
    }
    const flasherUrl='https://web.esphome.io/';
    const origin=new URL(flasherUrl).origin;
    const nonce=Array.from(crypto.getRandomValues(new Uint8Array(16)),
      value=>value.toString(16).padStart(2,'0')).join('');
    let sent=false,finished=false;
    let readyTimer=null,closePoll=null;
    const cleanup=()=>{
      if(readyTimer!==null)clearTimeout(readyTimer);
      if(closePoll!==null)clearInterval(closePoll);
      window.removeEventListener('message',onMessage);
      if(browserFlashCleanup===cleanup)browserFlashCleanup=null;
      browserFlashBusy=false;
      const current=currentDetailDevice();
      if(current)renderInstallationControls(current);
    };
    const onMessage=event=>{
      if(event.origin!==origin||event.source!==tab)return;
      const msg=event.data;
      if(!msg||typeof msg!=='object')return;
      if(msg.type==='esphome-web-flash:ready'&&!sent){
        if(msg.webSerial===false||
           (Array.isArray(msg.flashers)&&!msg.flashers.includes('esp'))){
          installStatus.textContent='Web Serial is unavailable. Open the installer in Chrome or Edge.';
          cleanup();
          return;
        }
        sent=true;
        try{
          tab.postMessage({
            type:'esphome-web-flash:firmware',version:1,nonce,
            name:'RFLink Universal Radioactive',deviceName:d.name,
            erase:true,flasher:'esp',logs:'flash-port',
            parts:[{address:0,data:image}]
          },origin,[image]);
          image=null; // transferred to the official browser flasher
          if(readyTimer!==null){clearTimeout(readyTimer);readyTimer=null;}
          installStatus.textContent='Select the ESP32-S3 COM port in the new tab and confirm flashing.';
        }catch(error){
          installStatus.textContent='Browser USB handoff failed: '+error.message;
          cleanup();
        }
        return;
      }
      if(!sent)return;
      if(msg.type==='esphome-web-flash:progress'&&Number.isFinite(msg.pct)){
        const percent=Math.min(100,Math.max(0,Math.round(msg.pct)));
        installStatus.textContent='Browser USB flash '+percent+'% · Do not disconnect the board.';
      }
      if(msg.type==='esphome-web-flash:state'){
        if(msg.state==='done'){
          finished=true;
          installStatus.textContent='USB write finished. Confirm a successful ESP32 boot and radio detection.';
          cleanup();
        }else if(msg.state==='error'){
          installStatus.textContent='USB flasher error: '+(msg.detail||'See the new tab; you may retry there.');
          // The user may retry in the same flasher tab, so remain connected.
        }else if(msg.note||msg.detail){
          installStatus.textContent='USB: '+(msg.note||msg.detail);
        }
      }
    };
    window.addEventListener('message',onMessage);
    browserFlashCleanup=cleanup;
    readyTimer=setTimeout(()=>{
      if(!sent&&!finished){
        installStatus.textContent='USB flasher did not respond. Check the separate tab.';
        cleanup();
      }
    },60000);
    closePoll=setInterval(()=>{
      if(tab.closed&&!finished){
        installStatus.textContent=sent?
          'USB installer closed. Verify whether the ESP32 started successfully.':
          'USB installer closed before firmware transfer.';
        cleanup();
      }
    },1000);
    tab.location.href=flasherUrl+'#nonce='+encodeURIComponent(nonce)+
      '&origin='+encodeURIComponent(window.location.origin);
  }catch(error){
    installStatus.textContent='Browser USB blocked: '+error.message;
    try{tab.close();}catch{}
    if(browserFlashCleanup)browserFlashCleanup();
    browserFlashBusy=false;
    renderInstallationControls(d);
  }
}
