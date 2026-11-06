/* Lightweight RFLink Builder memory diagnostics.
 * Updates only while Settings is visible. The backend samples independently
 * every five seconds, including while the browser is closed.
 */
const memoryPanel=document.getElementById('memory-diagnostics');
let latestMemoryReport=null;
let memoryRequestRunning=false;
const memoryMB=value=>Number.isFinite(value)
  ?(value/1048576).toLocaleString(undefined,{maximumFractionDigits:1})+' MB':'—';
const memoryPhaseLabels={
  idle:'Idle',building:'Building firmware',installing:'Installing firmware',
  toolchain_setup:'Preparing ESP-IDF',scanning:'Checking devices',before_build:'Before build',
};
const memoryText=(id,text)=>{document.getElementById(id).textContent=text;};
function paintMemoryReport(report){
  latestMemoryReport=report;
  const current=report.current;
  if(!current){
    memoryText('memory-phase','Waiting for first sample');
    return;
  }
  memoryText('memory-phase',memoryPhaseLabels[current.phase]||current.phase||'Unknown');
  memoryText('memory-sample-time',current.time?new Date(current.time).toLocaleString():'Unknown sample time');
  memoryText('memory-container',memoryMB(current.container_bytes));
  memoryText('memory-python',memoryMB(current.python_rss_bytes));
  const percent=current.percent_of_limit!=null
    ? current.percent_of_limit+'% of container limit'
    :current.percent_of_host!=null
      ? current.percent_of_host+'% of host RAM (estimate)'
      :'Cgroup limit unavailable';
  memoryText('memory-container-percent',percent);
  memoryText('memory-idle',memoryMB(report.last_idle?.container_bytes));
  memoryText('memory-cache',memoryMB(current.cache_bytes));
  memoryText('memory-anon',memoryMB(current.python_anon_bytes));
  memoryText('memory-file',memoryMB(current.python_file_bytes));
  memoryText('memory-threads',Number.isFinite(current.python_threads)?String(current.python_threads):'—');
  memoryText('memory-fds',Number.isFinite(current.python_fds)?String(current.python_fds):'—');
  memoryText('memory-kernel-peak',memoryMB(current.container_peak_bytes));
  memoryText('memory-observed-peak',memoryMB(report.observed_peak_bytes));
  memoryText('memory-source',current.source||'Unavailable');
  const build=report.last_build;
  memoryText('memory-build-peak',memoryMB(build?.peak_bytes));
  memoryText('memory-build-after',memoryMB(build?.after?.container_bytes));
  if(!build){
    memoryText('memory-before-after','No firmware build has been measured since the Builder started.');
    return;
  }
  const before=build.before?.container_bytes;
  const after=build.after?.container_bytes;
  const lines=['Before last build: '+memoryMB(before)+' · During build peak: '+memoryMB(build.peak_bytes)];
  if(after!=null){
    const delta=after-before;
    const sign=Number.isFinite(delta)&&delta>0?'+':'';
    lines.push('After build: '+memoryMB(after)+(Number.isFinite(delta)?' · Change: '+sign+memoryMB(delta):''));
  }else if(build.finished){
    lines.push('Waiting for at least 15 seconds of idle time to measure after-build RAM.');
  }else{
    lines.push('Build running. Peak updates while compilation is active.');
  }
  memoryText('memory-before-after',lines.join('  |  '));
}
async function updateMemoryDiagnostics(force=false){
  if(!force&&!memoryPanel.classList.contains('active-view'))return;
  if(memoryRequestRunning)return;
  memoryRequestRunning=true;
  try{
    const response=await api('api/diagnostics/memory');
    paintMemoryReport(response);
  }catch(e){
    memoryText('memory-sample-time','Measurement unavailable: '+e.message);
  }finally{memoryRequestRunning=false;}
}
document.getElementById('memory-refresh').addEventListener('click',()=>updateMemoryDiagnostics(true));
document.querySelector('.nav-tab[data-page="settings"]').addEventListener('click',()=>updateMemoryDiagnostics(true));
document.getElementById('memory-copy').addEventListener('click',async()=>{
  if(!latestMemoryReport)await updateMemoryDiagnostics(true);
  if(!latestMemoryReport)return;
  const current=latestMemoryReport.current||{};
  const build=latestMemoryReport.last_build||{};
  const lines=[
    'RFLink Builder memory diagnostics',
    'Sample: '+(current.time||'unknown'),
    'Phase: '+(current.phase||'unknown'),
    'Source: '+(current.source||'unknown'),
    'Container: '+memoryMB(current.container_bytes),
    'Python webserver RSS: '+memoryMB(current.python_rss_bytes),
    'Python anonymous RSS: '+memoryMB(current.python_anon_bytes),
    'Python file-backed RSS: '+memoryMB(current.python_file_bytes),
    'Python threads: '+(current.python_threads??'unknown'),
    'Python open file descriptors: '+(current.python_fds??'unknown'),
    'Cgroup file cache: '+memoryMB(current.cache_bytes),
    'Container memory limit: '+memoryMB(current.container_limit_bytes),
    'Host total RAM: '+memoryMB(current.host_total_bytes),
    'Kernel container peak: '+memoryMB(current.container_peak_bytes),
    'Idle sample: '+memoryMB(latestMemoryReport.last_idle?.container_bytes),
    'Build before: '+memoryMB(build.before?.container_bytes),
    'Build peak: '+memoryMB(build.peak_bytes),
    'Build after: '+memoryMB(build.after?.container_bytes),
    'Observational only: includes Linux cache; percentages can differ from Home Assistant.',
  ];
  try{
    await navigator.clipboard.writeText(lines.join('\n'));
    memoryText('memory-copy-feedback','Copied (no device passwords included).');
  }catch(e){memoryText('memory-copy-feedback','Copy failed: '+e.message);}
});
setInterval(()=>updateMemoryDiagnostics(),5000);
