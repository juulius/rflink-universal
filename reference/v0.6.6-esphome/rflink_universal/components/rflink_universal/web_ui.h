#pragma once

#include <cstddef>
#include <cstdint>

namespace esphome {
namespace rflink_universal {

static constexpr char RFLINK_UI_HTML[] = R"RFHTML(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<title data-i18n="static.000">RFLink Universal</title>
<link rel="icon" type="image/svg+xml" href="/rflink/favicon.svg">
<link rel="alternate icon" type="image/x-icon" href="/favicon.ico">
<style>
:root{color-scheme:light dark;--bg:#f5f7fa;--card:#fff;--text:#111827;--muted:#667085;--line:#e4e9f0;--nav:#0b1b33;--green:#159447;--green-bg:#eaf8ef;--blue:#2563eb;--amber:#c97a08;--amber-bg:#fff7e8;--shadow:0 1px 2px rgba(16,24,40,.04)}
@media(prefers-color-scheme:dark){:root{--bg:#0f141a;--card:#151e28;--text:#f8fafc;--muted:#9eabbc;--line:#293544;--nav:#081321;--green-bg:#102b1c;--amber-bg:#33220d;--shadow:none}}
[hidden]{display:none!important}*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--text);font:15px/1.45 system-ui,-apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif}button,input,textarea,select{font:inherit}.top{background:var(--nav);color:#fff;padding:17px 20px;display:flex;align-items:center;justify-content:space-between;gap:12px}.brand{font-size:22px;font-weight:800}.topstate{display:flex;gap:8px;flex-wrap:wrap}.tag{border:1px solid rgba(255,255,255,.16);border-radius:10px;padding:6px 9px;font-size:12px}.good{color:#21c45a}.shell{display:grid;grid-template-columns:205px minmax(0,1fr)}.side{min-height:calc(100vh - 62px);background:var(--card);border-right:1px solid var(--line);padding:18px 12px}.nav{display:grid;gap:6px}.nav button,.bottom button{border:0;background:transparent;color:inherit;cursor:pointer}.nav button{padding:11px 13px;border-radius:9px;text-align:left}.nav button.active{background:var(--green-bg);color:var(--green);font-weight:750}.main{padding:26px clamp(18px,2vw,34px);min-width:0;max-width:none;width:100%;margin:0}.page{display:none}.page.active{display:block}.title{font-size:32px;line-height:1.15;margin:0 0 4px}.sub{color:var(--muted);margin-bottom:20px}.grid3{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:13px}.card{background:var(--card);border:1px solid var(--line);border-radius:13px;padding:17px;box-shadow:var(--shadow);min-width:0}.radiohead,.row,.split{display:flex;align-items:center;justify-content:space-between;gap:12px}.freq{font-size:21px;font-weight:800}.muted{color:var(--muted)}.rows{margin-top:12px;border-top:1px solid var(--line)}.row{padding:9px 0;border-bottom:1px solid var(--line)}.row:last-child{border-bottom:0}.actions{display:flex;gap:9px;flex-wrap:wrap;margin:16px 0}.btn{border:1px solid var(--blue);background:var(--blue);color:#fff;border-radius:9px;padding:10px 14px;cursor:pointer;font-weight:700;min-height:42px}.btn.green{background:var(--green);border-color:var(--green)}.btn.out{background:transparent;color:var(--blue)}.btn.soft{background:var(--card);border-color:var(--line);color:var(--text)}.btn:disabled{opacity:.45;cursor:not-allowed}button:focus-visible,input:focus-visible,textarea:focus-visible,select:focus-visible,summary:focus-visible{outline:3px solid color-mix(in srgb,var(--blue) 45%,transparent);outline-offset:2px}.choice:disabled,.modebtn:disabled{opacity:.45;cursor:not-allowed;background:var(--bg)}.taskgrid{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:10px;margin:18px 0}.taskgrid.simple{grid-template-columns:repeat(2,minmax(220px,360px));justify-content:start}.task{border:1px solid var(--line);background:var(--card);color:inherit;border-radius:12px;padding:15px;text-align:left;cursor:pointer;min-height:112px}.task b{display:block;font-size:16px;margin-bottom:5px}.task span{color:var(--muted);font-size:13px}.sectiontitle{font-size:18px;margin:21px 0 10px}.list{display:grid;gap:10px}.device-finder{margin:0 0 12px}.device-finder>label{display:block;font-weight:700;margin-bottom:8px}.device-find-row{display:grid;grid-template-columns:minmax(140px,1fr) minmax(130px,190px) auto;gap:8px}.device-finder .field{min-width:0}.device{display:grid;grid-template-columns:minmax(0,1fr) auto;gap:14px;align-items:start}.device h3{margin:0 0 3px}.pill{display:inline-block;padding:4px 8px;border-radius:999px;background:var(--green-bg);color:var(--green);font-size:12px;font-weight:700}.pill.warn{background:var(--amber-bg);color:var(--amber)}.candidate{border-color:#e6a23c}.empty{text-align:center;color:var(--muted);padding:26px}.steps{display:flex;gap:7px;margin:10px 0 18px}.step{height:6px;border-radius:10px;background:var(--line);flex:1}.step.done{background:var(--green)}.modepick{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-bottom:14px}.choice,.modebtn{border:1px solid var(--line);background:var(--card);color:inherit;border-radius:11px;padding:13px;cursor:pointer}.choice.active,.modebtn.active{border-color:var(--green);background:var(--green-bg)}.choices{display:grid;grid-template-columns:repeat(3,1fr);gap:9px;margin:12px 0}.meter{height:12px;background:var(--line);border-radius:99px;overflow:hidden;margin:10px 0}.meter span{display:block;height:100%;width:0;background:var(--green);transition:width .2s}.codebox{width:100%;min-height:150px;border:1px solid var(--line);border-radius:9px;padding:11px;background:var(--bg);color:var(--text);font:12px/1.45 ui-monospace,SFMono-Regular,Consolas,monospace;resize:vertical}.notice{border-left:4px solid var(--green);padding:11px 13px;background:var(--green-bg);border-radius:8px;margin:12px 0}.notice.warn{border-left-color:var(--amber);background:var(--amber-bg)}details summary{cursor:pointer;font-weight:700}.settings{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:13px}.settings-primary{grid-column:1/-1;display:grid;grid-template-columns:minmax(230px,.75fr) minmax(280px,1fr) minmax(320px,1.15fr);gap:13px;align-items:start}.settings-primary>.card{height:auto;align-self:start}.settings-primary .actions{margin-bottom:0}.settings-compact details{margin-top:10px}.settings-compact details p:last-child{margin-bottom:0}.decoderlist{display:flex;flex-wrap:wrap;gap:7px;margin-top:10px}.decodertag{background:var(--bg);border:1px solid var(--line);border-radius:8px;padding:6px 9px;font-size:12px}.decoder-filters{display:flex;flex-wrap:wrap;gap:7px;margin:10px 0}.decoder-filter-btn{border:1px solid var(--line);background:var(--bg);color:var(--text);border-radius:9px;padding:8px 11px;font-weight:700;cursor:pointer}.decoder-filter-btn.active{border-color:var(--green);background:var(--green-bg)}.decoder-library-card{grid-column:1/-1}.decoder-browser{display:grid;grid-template-columns:minmax(260px,340px) minmax(0,1fr);gap:18px;margin-top:10px;align-items:start}.decoder-selector{display:flex;flex-direction:column;gap:7px}.decoder-select{width:100%;text-align:left;border:1px solid var(--line);background:var(--bg);color:var(--text);border-radius:10px;padding:10px 11px;cursor:pointer}.decoder-select b{display:block}.decoder-select span{display:block;margin-top:3px}.decoder-select.active{border-color:var(--green);background:var(--green-bg)}.decoder-detail{border:1px solid var(--line);border-radius:11px;padding:16px;min-height:190px;position:sticky;top:14px;align-self:start}.decoder-detail h3{margin:0 0 4px;font-size:19px}.decoder-info-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(240px,1fr));gap:10px;margin:12px 0}.decoder-info-block{border:1px solid var(--line);border-radius:9px;padding:10px}.decoder-info-block.control{grid-column:span 2}.pagehead{display:flex;align-items:flex-start;justify-content:space-between;gap:14px;margin-bottom:20px}.pagehead .sub{margin-bottom:0}.settings-wide{grid-column:1/-1}.advanced-section{border-style:dashed}.advanced-section>h2{display:flex;align-items:center;gap:8px}.advanced-badge{font-size:11px;font-weight:800;text-transform:uppercase;letter-spacing:.05em;padding:3px 7px;border-radius:999px;background:var(--bg);color:var(--muted);border:1px solid var(--line)}.candidate-section[hidden]{display:none}.compat-details{margin-top:8px}.compat-details summary{font-size:12px;color:var(--muted)}.decoder-info-block b{display:block;margin-bottom:4px}.known-profiles{margin:14px 0}.known-profile-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(280px,1fr));gap:10px;margin-top:8px}.known-profile{border:1px solid var(--line);border-radius:10px;padding:12px;background:var(--bg)}.known-profile h4{margin:0 0 3px;font-size:15px}.known-profile .profile-models{margin:5px 0}.known-profile .profile-sources{display:flex;gap:7px;flex-wrap:wrap;margin-top:8px}.known-profile .profile-sources a{font-size:12px;color:var(--blue);text-decoration:none}.known-profile .profile-sources a:hover{text-decoration:underline}.bottom{display:none}.toast{position:fixed;right:18px;bottom:18px;background:#101828;color:#fff;padding:10px 13px;border-radius:9px;opacity:0;pointer-events:none;transition:.2s;z-index:30}.toast.show{opacity:1}.modalback{position:fixed;inset:0;background:rgba(0,0,0,.45);display:none;align-items:center;justify-content:center;padding:16px;z-index:40}.modalback.show{display:flex}.modal{width:min(440px,100%);background:var(--card);border-radius:14px;border:1px solid var(--line);padding:18px}.modal h3{margin-top:0}.field{width:100%;border:1px solid var(--line);border-radius:9px;padding:11px;background:var(--bg);color:var(--text);font-size:16px}.small{font-size:13px}.mono{font-family:ui-monospace,SFMono-Regular,Consolas,monospace;overflow-wrap:anywhere}
.advanced-summary{display:flex;align-items:center;gap:8px;font-size:18px;font-weight:800;list-style-position:outside}.advanced-summary .advanced-badge{margin-left:2px}.user-library-layout{display:grid;grid-template-columns:minmax(280px,.85fr) minmax(420px,1.15fr);gap:20px;align-items:start}.user-library-pane,.user-profile-pane{min-width:0}.user-profile-pane{border-left:1px solid var(--line);padding-left:20px}.user-profile-pane h3{margin-top:0}.profile-form-grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:11px 13px}.profile-form-grid label,.user-basics-grid label,.rule-grid label{display:grid;gap:6px;font-weight:650}.profile-form-grid .full{grid-column:1/-1}.user-basics-grid{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:12px;margin:14px 0}.user-basics-grid.simple-basics{grid-template-columns:repeat(2,minmax(0,1fr));max-width:900px}.rule-grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:12px;margin-top:10px}.rule-grid>div{min-width:0}.rule-grid .codebox{min-height:240px}.settings>#libraryCard{grid-column:1/-1}.settings>.decoder-library-card{grid-column:1/-1}.settings>.card{align-self:stretch}.decoder-selector{min-width:0}.decoder-info-block{min-width:0}.row>*{min-width:0}
@media(max-width:1100px){.settings-primary{grid-template-columns:1fr 1fr}.settings-primary>.card:first-child{grid-column:1/-1}.user-library-layout{grid-template-columns:1fr}.user-profile-pane{border-left:0;border-top:1px solid var(--line);padding-left:0;padding-top:16px}.rule-grid{grid-template-columns:1fr}}
@media(max-width:850px){.settings-primary{grid-template-columns:1fr}.settings-primary>.card:first-child{grid-column:auto}.taskgrid{grid-template-columns:repeat(2,1fr)}.decoder-info-block.control{grid-column:auto}.user-basics-grid.simple-basics{grid-template-columns:1fr}.grid3{grid-template-columns:1fr}.settings{grid-template-columns:1fr}.decoder-browser{grid-template-columns:1fr}.decoder-selector{display:grid;grid-template-columns:repeat(2,minmax(0,1fr))}.decoder-detail{position:static}.decoder-info-grid{grid-template-columns:1fr}.user-basics-grid{grid-template-columns:1fr}.profile-form-grid{grid-template-columns:1fr}.profile-form-grid .full{grid-column:auto}}
@media(max-width:680px){.shell{display:block}.pagehead{display:block}.pagehead>.btn{width:100%;margin-top:10px}.taskgrid.simple{grid-template-columns:1fr;max-width:none}.side{display:none}.main{padding:17px 13px 84px}.top{padding:14px 14px}.brand{font-size:19px}.topstate .tag:nth-child(3){display:none}.title{font-size:27px}.taskgrid{grid-template-columns:1fr 1fr}.task{min-height:104px}.device{grid-template-columns:1fr}.choices,.modepick{grid-template-columns:1fr}.bottom{position:fixed;left:0;right:0;bottom:0;display:grid;grid-template-columns:repeat(5,1fr);background:var(--card);border-top:1px solid var(--line);padding:7px max(7px,env(safe-area-inset-right)) calc(7px + env(safe-area-inset-bottom)) max(7px,env(safe-area-inset-left));z-index:20}.bottom button{padding:8px 2px;font-size:12px}.bottom button.active{color:var(--green);font-weight:750}.toast{bottom:76px;left:14px;right:14px;text-align:center}}
.health{border-left:5px solid var(--amber);margin:0 0 16px}.health.ok{border-left-color:var(--green)}.health.error{border-left-color:#d92d20}.health h2{margin:2px 0 7px;font-size:20px}.health p{margin:6px 0}.wizard{border:2px solid var(--green);margin:16px 0}.wizard h2{margin:0 0 8px}.wizard input{max-width:520px}.wizard .choices{grid-template-columns:repeat(auto-fit,minmax(150px,1fr))}.wizard button:disabled{cursor:not-allowed;opacity:.5}.device-controls{display:flex;gap:8px;flex-wrap:wrap;margin-top:12px}.signal-panel{grid-column:1/-1;border-top:1px solid var(--line);padding-top:12px}.signal-panel summary{cursor:pointer;font-weight:750;padding:7px 0}.signal-entry{border:1px solid var(--line);border-radius:9px;padding:12px;margin-top:9px;display:grid;gap:9px}.signal-entry .mono{font-size:12px}.signal-entry select{max-width:100%}.signal-entry .btn{white-space:normal}.signal-controls{display:flex;gap:8px;flex-wrap:wrap;align-items:center}.signal-controls select{flex:1;min-width:150px}.signal-controls .btn{flex:0 1 auto}.storage{font-weight:700}.decoder-summary{display:grid;grid-template-columns:minmax(0,1fr) auto;align-items:center;gap:9px}.decoder-card{border:1px solid var(--line);border-radius:11px;padding:13px;margin-top:10px}.decoder-card h3{margin:0 0 3px;font-size:17px}.decoder-status{font-size:12px;font-weight:750;border-radius:8px;padding:5px 8px;white-space:normal;text-align:right;background:var(--bg);border:1px solid var(--line)}.decoder-status.pending{background:var(--amber-bg);color:var(--amber)}.decoder-status.ready{background:var(--green-bg);color:var(--green)}.decoder-card p{margin:8px 0}.decoder-tech{margin-top:11px;border-top:1px solid var(--line);padding-top:11px}.review-options{display:grid;gap:6px;margin:9px 0}.review-choice{display:flex;gap:10px;align-items:center;border:1px solid var(--line);border-radius:9px;padding:9px;min-height:42px}.review-choice input{width:18px;height:18px;flex-shrink:0}.review-choice span{min-width:0}.review-warning{font-size:13px;color:var(--muted)}.decoder-filter{max-width:240px}.device-more{grid-column:1/-1}.device-summary{display:flex;align-items:center;gap:8px;flex-wrap:wrap}.device-primary{grid-column:1/-1}.device-primary details summary{padding:8px 0}.device-controls .btn{font-size:13px}.device-more{grid-column:1/-1;border-top:1px solid var(--line);padding-top:10px}.device-more>summary{padding:7px 0}.device-more .signal-panel{margin:12px 0}.device-summary .pill{line-height:1.5}.decoder-card button[data-decoder-learn]{margin-top:9px}.review-choice:focus-within{border-color:var(--green)}@media(max-width:680px){.decoder-summary{grid-template-columns:1fr}.decoder-status{text-align:left;justify-self:start}.device-controls{display:grid;grid-template-columns:1fr}.device-controls .btn{width:100%}}

/* LOGBOOK1: compact, scannable rows; controls remain usable on 390px mobile. */
#logbookModes{display:flex;gap:8px;flex-wrap:wrap}
#logbookTimeline{height:clamp(340px,56vh,570px);max-height:none!important;min-height:340px;align-content:start;grid-auto-rows:max-content;overflow-y:auto;overscroll-behavior:contain;scrollbar-gutter:stable;background:var(--card);border:1px solid var(--line);border-radius:9px;padding:2px 9px}
.logbook-radio-tools{display:grid;gap:10px;margin:12px 0;padding:10px 12px;background:var(--bg);border:1px solid var(--line);border-radius:10px}
.logbook-radio-tools .choices{grid-template-columns:repeat(4,minmax(0,1fr));margin:4px 0}
.logbook-radio-tools .choice{min-height:48px;padding:8px}
.settings-log-group{padding:10px 0 12px;border-top:1px solid var(--line)}
.settings-log-group h3{margin:0 0 5px;font-size:15px}
.settings-log-group>p{margin:0 0 10px}
#firmwareRevision,#firmwareRawVersion{overflow-wrap:anywhere;font-size:12px;line-height:1.5}
.version-main{font-size:17px;font-weight:800}
#firmwareNotes{margin:7px 0 0;padding-left:18px}
@media(max-width:700px){#logbookTimeline{height:440px;min-height:300px}.logbook-radio-tools .choices{grid-template-columns:repeat(2,minmax(0,1fr))}}
#logbookToolbar{justify-content:flex-start}
#logbookToolbar>.field{min-height:38px}
.logbook-entry{border-bottom:1px solid var(--line);padding:1px 0}
.logbook-entry summary{display:grid;grid-template-columns:75px 92px minmax(0,1fr);align-items:start;gap:10px;padding:9px 8px;cursor:pointer;border-radius:6px;font-size:12px;list-style:none}
.logbook-entry summary::-webkit-details-marker{display:none}
.logbook-entry summary:hover{background:var(--bg)}
.logbook-entry summary>span:last-child{overflow-wrap:anywhere;line-height:1.45}
.logbook-entry summary>b{font-size:12px;font-weight:700}
.logbook-entry>div{padding:2px 10px 12px 105px;overflow-wrap:anywhere}
@media(max-width:600px){#logbookToolbar{display:grid;grid-template-columns:1fr 1fr;align-items:center}#logbookToolbar label[for=logbookSearch],#logbookSearch{grid-column:1/-1}#logbookSearch{max-width:none!important}#logbookToolbar button{width:100%}.logbook-entry summary{grid-template-columns:54px 75px minmax(0,1fr);gap:5px;padding:9px 3px;font-size:11px}.logbook-entry summary>b{font-size:11px}.logbook-entry>div{padding-left:63px}#logbookToolbar label{font-size:12px}}

/* Library/update UX foundation */
.help-btn{display:inline-flex;align-items:center;justify-content:center;width:22px;height:22px;border-radius:50%;border:1px solid var(--line);background:var(--card);font-weight:800;font-size:12px;cursor:pointer;vertical-align:middle;margin-left:5px;color:var(--muted)}
.help-btn:hover,.help-btn:focus{border-color:var(--green);color:var(--text);outline:none}
.required-wrap{border:1px solid #b78313;border-radius:12px;padding:9px;background:rgba(183,131,19,.08)}
.required-wrap .required-badge{font-size:11px;font-weight:800;margin-left:6px}
.field.required-missing{border-color:#b78313;box-shadow:0 0 0 2px rgba(183,131,19,.14)}
.library-summary{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:8px;margin-top:10px}
.library-stat{padding:10px;border:1px solid var(--line);border-radius:12px;display:flex;justify-content:space-between;gap:12px;align-items:center}
.library-attention{border-color:#b78313;background:rgba(183,131,19,.08)}
.advanced-panel{margin-top:12px;border-top:1px solid var(--line);padding-top:10px}
.advanced-kv{display:grid;grid-template-columns:minmax(110px,.65fr) minmax(0,1.35fr);gap:6px 12px;font-size:12px}
.advanced-kv span:nth-child(odd){color:var(--muted)}
.modal-backdrop[hidden]{display:none}.modal-backdrop{position:fixed;inset:0;background:rgba(0,0,0,.58);z-index:999;display:flex;align-items:center;justify-content:center;padding:16px}
.modal-card{width:min(520px,100%);max-height:82vh;overflow:auto;background:var(--card);border:1px solid var(--line);border-radius:16px;padding:18px;box-shadow:0 20px 60px rgba(0,0,0,.35)}
.modal-head{display:flex;justify-content:space-between;align-items:center;gap:12px}.modal-head h2{margin:0;font-size:19px}.modal-close{border:0;background:transparent;color:var(--text);font-size:24px;cursor:pointer}
@media(max-width:680px){.library-summary{grid-template-columns:1fr}.advanced-kv{grid-template-columns:1fr}.advanced-kv span:nth-child(even){margin-bottom:5px}}

/* UI4: simple normal flow; technical choices stay behind Advanced. */
.learn-advanced-entry{margin:12px 0 0}.learn-advanced-entry>summary{color:var(--muted);font-size:13px}.learn-advanced-entry .modebtn{margin-top:10px;width:min(420px,100%);text-align:left}.wizard-tech{margin-top:10px}.wizard-tech summary{font-size:13px;color:var(--muted)}.wizard-result-actions{align-items:center}.monitor-advanced{margin-top:12px}.monitor-advanced>summary{color:var(--muted);font-size:13px}.frequency-hint{display:block;margin-top:4px;font-size:12px;color:var(--muted);font-weight:500}.wizard-step-note{font-size:13px;color:var(--muted);margin:8px 0 0}.normal-only[hidden]{display:none!important}


/* Settings UX compact dashboard: combines related controls without hiding everyday settings. */
.page[data-view="settings"]>.sub{margin:0 0 12px}
.page[data-view="settings"] .settings{display:block}
.page[data-view="settings"] .settings-dashboard{display:grid;grid-template-columns:minmax(295px,.96fr) minmax(345px,1.08fr) minmax(300px,.96fr);gap:12px;align-items:stretch}
.page[data-view="settings"] .settings-card{padding:15px 16px;align-self:stretch;min-width:0}
.page[data-view="settings"] .settings-card-title{font-size:17px;line-height:1.28;margin:0 0 12px}
.page[data-view="settings"] .settings-general-groups{display:grid;gap:12px}
.page[data-view="settings"] .settings-subgroup{min-width:0}
.page[data-view="settings"] .settings-subgroup+.settings-subgroup{border-top:1px solid var(--line);padding-top:11px}
.page[data-view="settings"] .settings-group-title{display:block;font-weight:700;font-size:13px;margin-bottom:7px}
.page[data-view="settings"] .settings-general select{padding:8px 10px;min-height:40px;font-size:14px;max-width:none;width:100%}
.page[data-view="settings"] .settings-inline-feedback:empty{display:none}
.page[data-view="settings"] .settings-inline-feedback{margin:3px 0 0}
.page[data-view="settings"] .settings-tight-row{display:flex;align-items:center;justify-content:space-between;gap:12px;padding:5px 0;font-size:13px;min-width:0}
.page[data-view="settings"] .settings-tight-row>*{min-width:0}
.page[data-view="settings"] .settings-tight-row b{overflow-wrap:anywhere;text-align:right}
.page[data-view="settings"] .settings-subgroup-led input[type=range]{width:100%;height:17px;margin:4px 0 5px;accent-color:var(--blue)}
.page[data-view="settings"] .settings-check input{width:17px;height:17px;flex-shrink:0;accent-color:var(--green)}
.page[data-view="settings"] .settings-status{color:var(--muted);border-top:1px solid var(--line)}
.page[data-view="settings"] .settings-footer{display:flex;align-items:center;gap:8px;flex-wrap:wrap;margin-top:9px}
.page[data-view="settings"] .settings-footer .btn{min-height:36px;padding:7px 10px;font-size:12px;line-height:1.25}
.page[data-view="settings"] .settings-inline-more{font-size:12px;min-width:0}
.page[data-view="settings"] .settings-inline-more summary{color:var(--blue);font-weight:650}
.page[data-view="settings"] .settings-inline-more p{margin:6px 0}
.page[data-view="settings"] .settings-short-help{font-size:12px;line-height:1.35;margin:0 0 10px}
.page[data-view="settings"] .settings-log-fields{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:9px}
.page[data-view="settings"] .settings-log-fields label.row{display:flex;flex-direction:column;align-items:stretch;gap:5px;border:1px solid var(--line);border-radius:9px;padding:7px 9px;font-size:12px;line-height:1.3;min-width:0;background:var(--bg)}
.page[data-view="settings"] .settings-log-fields label.row span{min-height:31px}
.page[data-view="settings"] .settings-log-fields .field{width:100%;max-width:none;padding:6px 9px;min-height:36px;font-size:14px;background:var(--card)}
.page[data-view="settings"] .settings-inline-stat{font-size:12px;line-height:1.4;margin:9px 0 0}
.page[data-view="settings"] .settings-footer #rxLogSettingStatus:empty{display:none}
.page[data-view="settings"] .settings-system-shortcut .settings-tight-row{font-size:12px}
.page[data-view="settings"] .settings-system-shortcut .settings-tight-row .mono{font-size:11px}
.page[data-view="settings"] .settings-pickers{display:flex;align-items:center;gap:7px;flex-wrap:wrap;margin-top:12px;padding:9px 12px;background:var(--card);border:1px solid var(--line);border-radius:11px}
.page[data-view="settings"] .settings-pickers-label{color:var(--muted);font-size:12px;font-weight:750;margin-right:5px}
.page[data-view="settings"] .settings-picker{border:1px solid var(--line);background:var(--bg);color:var(--text);cursor:pointer;border-radius:8px;padding:7px 11px;font-size:12px;font-weight:650;min-height:36px;white-space:normal;text-align:center}
.page[data-view="settings"] .settings-picker:hover,.page[data-view="settings"] .settings-picker[aria-expanded="true"]{border-color:var(--blue);color:var(--blue)}
.page[data-view="settings"] .settings-detail-panel{margin-top:12px}
.page[data-view="settings"] .settings-detail-panel[hidden]{display:none!important}
.page[data-view="settings"] .settings-detail-panel>.card{width:100%}
@media(max-width:1390px){.page[data-view="settings"] .settings-dashboard{grid-template-columns:repeat(2,minmax(0,1fr))}.page[data-view="settings"] .settings-assistant{grid-column:1/-1}.page[data-view="settings"] .settings-assistant{display:grid;grid-template-columns:1fr 1fr;column-gap:18px}.page[data-view="settings"] .settings-assistant>.settings-card-title{grid-column:1/-1}.page[data-view="settings"] .settings-assistant>.settings-subgroup+.settings-subgroup{border-top:0;border-left:1px solid var(--line);padding-top:0;padding-left:18px}}
@media(max-width:840px){.page[data-view="settings"] .settings-dashboard{grid-template-columns:1fr}.page[data-view="settings"] .settings-assistant{grid-column:auto;display:block}.page[data-view="settings"] .settings-assistant>.settings-subgroup+.settings-subgroup{border-left:0;border-top:1px solid var(--line);padding:11px 0 0}.page[data-view="settings"] .settings-general-groups{grid-template-columns:1fr 1fr;gap:16px}.page[data-view="settings"] .settings-general .settings-subgroup+.settings-subgroup{border-top:0;border-left:1px solid var(--line);padding:0 0 0 16px}}
@media(max-width:580px){.page[data-view="settings"] .settings-general-groups{grid-template-columns:1fr}.page[data-view="settings"] .settings-general .settings-subgroup+.settings-subgroup{border-left:0;border-top:1px solid var(--line);padding:11px 0 0}.page[data-view="settings"] .settings-pickers{display:grid;grid-template-columns:repeat(2,minmax(0,1fr))}.page[data-view="settings"] .settings-pickers-label{grid-column:1/-1}.page[data-view="settings"] .settings-picker{white-space:normal}.page[data-view="settings"] .settings-card{padding:13px}}


/* NAV3: in-panel return action and direct decoder creation without extra disclosure. */
.page[data-view="settings"] .settings-detail-toolbar{display:flex;align-items:center;margin:0 0 8px}
.page[data-view="settings"] .settings-detail-toolbar .btn{min-height:35px;font-size:12px}
.page[data-view="settings"] .settings-tool-action{float:right;margin:1px 0 8px 8px}
.page[data-view="settings"] .settings-tool-action .btn{font-size:12px;min-height:34px}
.page[data-view="settings"] .settings-custom-actions{margin:6px 0 10px}
@media(max-width:580px){.page[data-view="settings"] .settings-tool-action{float:none;margin:0 0 10px}}
@media(max-width:560px){.device-find-row{grid-template-columns:1fr 1fr}.device-find-row input{grid-column:1/-1}.device-find-row button{min-width:0}}

/* Device frequency selector uses the same cards and active state as Activity. */
.page[data-view="devices"] .device-radio-choices{grid-template-columns:repeat(auto-fit,minmax(145px,1fr));gap:9px;margin:10px 0 13px}
.page[data-view="devices"] .device-radio-choices .choice{min-height:70px;text-align:center;display:flex;align-items:center;justify-content:center;flex-direction:column;gap:3px;padding:10px 7px}
.page[data-view="devices"] .device-radio-choices .choice b{font-size:14px}
.page[data-view="devices"] .device-radio-choices .choice span{display:block;font-size:12px}
.page[data-view="devices"] .device-radio-choices [hidden]{display:none}
.page[data-view="devices"] .device-finder>label{display:block;font-weight:700;margin:0 0 7px}
.page[data-view="devices"] .device-find-row{grid-template-columns:minmax(0,1fr) auto}
.page[data-view="devices"] #deviceSearchCount{margin:9px 0 0}
@media(max-width:560px){.page[data-view="devices"] .device-radio-choices{grid-template-columns:repeat(2,minmax(0,1fr))}.page[data-view="devices"] .device-find-row{grid-template-columns:minmax(0,1fr) auto}.page[data-view="devices"] .device-find-row input{grid-column:auto}}

/* Shared profile-first UI for ALL device families, built-in and user profiles. */
.profile-picker{margin:12px 0;padding:12px;border:1px solid var(--line);border-radius:10px;background:var(--bg);min-width:0}
.profile-picker label{display:block;font-weight:750;margin-bottom:6px}
.profile-picker select{width:100%;max-width:100%;min-height:40px}
.profile-picker p{margin:7px 0 0}
.device-operation{margin:12px 0 6px}
.device-operation h4{font-size:15px;margin:0 0 7px}
.device-command-list{display:flex;flex-wrap:wrap;gap:8px;margin:10px 0}
.device-command-list button{min-width:78px}
.device-operation .small{overflow-wrap:anywhere}
.profile-preview{margin-top:7px;font-size:13px;color:var(--muted)}
@media(max-width:540px){.device-command-list{display:grid;grid-template-columns:repeat(2,minmax(0,1fr))}.device-command-list .btn{width:100%}}

/* CATALOG2: browse protocols and device families with the same compact library. */
.catalog-modes{display:flex;flex-wrap:wrap;gap:8px;margin:12px 0 8px}
.catalog-modes .decoder-filter-btn{min-height:38px}
.profile-browser-tools{display:grid;grid-template-columns:minmax(190px,1fr) repeat(3,minmax(130px,190px));gap:8px;margin:10px 0;align-items:center}
.profile-browser-tools .field{min-width:0;width:100%;font-size:13px;padding:9px}
.profile-browser-tools[hidden],.decoder-filters[hidden]{display:none!important}
.profile-selector-group{font-size:12px;color:var(--muted);font-weight:750;padding:9px 2px 3px;border-top:1px solid var(--line);margin-top:5px}
.profile-selector-group:first-child{border-top:0;margin-top:0}
.profile-breadcrumb{display:flex;gap:5px;flex-wrap:wrap;align-items:center;color:var(--muted);font-size:12px;margin:4px 0 10px}
.profile-breadcrumb button{padding:2px 5px;color:var(--blue);border:0;background:none;cursor:pointer;font-weight:700}
.profile-controls{display:flex;flex-wrap:wrap;gap:6px;margin:9px 0}
.profile-controls span{border:1px solid var(--line);border-radius:7px;background:var(--bg);padding:5px 8px;font-size:12px}
.profile-summary-row{display:flex;gap:8px;align-items:baseline;flex-wrap:wrap;margin:10px 0}
.profile-family-link{background:var(--bg);border:1px solid var(--line);border-radius:8px;padding:6px 9px;cursor:pointer;color:var(--blue);font-size:12px}
@media(max-width:960px){.profile-browser-tools{grid-template-columns:repeat(2,minmax(0,1fr))}}
@media(max-width:580px){.profile-browser-tools{grid-template-columns:1fr 1fr}.profile-browser-tools input{grid-column:1/-1}.profile-selector-group{margin-top:8px}}

/* Declarative device onboarding: independent of decoder/radio implementation. */
.pair-dialog{border:1px solid var(--line,#405064);background:#131e2b;color:#f2f5fb;border-radius:14px;padding:22px;max-width:540px;width:calc(100% - 32px);box-shadow:0 18px 80px #0009}
.pair-dialog::backdrop{background:#050b13b8}.pair-dialog h3{margin:0 0 10px}.pair-dialog p{line-height:1.5}
.pair-dialog .pair-progress{height:5px;border-radius:6px;background:#334155;overflow:hidden;margin:14px 0}
.pair-dialog .pair-progress span{display:block;height:100%;background:#169b61;transition:width .15s}
.pair-dialog .pair-row{display:flex;gap:8px;flex-wrap:wrap;justify-content:flex-end;margin-top:20px}
.pair-dialog .pair-warning{border-left:3px solid #d38a24;padding:10px;background:#5b3b141c;margin-top:12px}
.device-pair-panel{display:flex;flex-direction:column;gap:7px}.device-pair-panel .btn{align-self:flex-start}

/* DeviceUX4: everyday overview is separate from setup/repair; no wizard inside a second dialog. */
#deviceList .device-overview{padding:15px 17px}
#deviceList .device-overview-actions{display:flex;flex-wrap:wrap;align-items:center;gap:10px;margin-top:12px}
#deviceList .device-overview-actions .device-operation{margin:0;flex:1 1 260px}
#deviceList .device-overview-actions .btn[data-device-open]{margin-left:auto}
#deviceList .device-overview-actions .device-command-list{margin:4px 0}
#deviceList .device-overview .device-unavailable{max-width:600px}
#deviceList .device-unconfigured{grid-template-columns:minmax(0,720px)}
#deviceList .device-unconfigured .device-setup-card{background:transparent;border:0;padding:0}
#deviceList .device-unconfigured .profile-picker{border:0;padding:0;margin:10px 0 0;background:transparent}
#deviceList .device-unconfigured .device-next-step{margin:0 0 9px;color:var(--muted)}
.device-detail-header{display:flex;align-items:center;gap:14px;margin:0 0 16px;flex-wrap:wrap}
.device-detail-header[hidden]{display:none}
.device-detail-header .sectiontitle{margin:0 0 4px}
.page[data-view="devices"].device-detail-mode .device-finder,.page[data-view="devices"].device-detail-mode .pagehead{display:none}
#deviceList .device-detail-card .device-main{grid-template-columns:minmax(0,1fr) minmax(240px,.9fr)}
@media(max-width:760px){#deviceList .device-detail-card .device-main{grid-template-columns:minmax(0,1fr)}#deviceList .device-overview-actions .btn[data-device-open]{margin-left:0}}
/* UX3: prioritize actual device status and everyday action, not configuration. */
#deviceList .card.device{display:block;padding:15px 17px;min-width:0}
#deviceList .device-top{display:flex;flex-wrap:wrap;gap:8px 14px;justify-content:space-between;align-items:flex-start}
#deviceList .device-top h3{font-size:18px;margin:0 0 4px}
#deviceList .device-top p{margin:0}
#deviceList .device-top .device-summary{display:flex;gap:5px;flex-wrap:wrap;align-items:center}
#deviceList .device-main{display:grid;grid-template-columns:minmax(0,1.25fr) minmax(230px,.7fr);gap:12px 22px;align-items:start;margin-top:13px}
#deviceList .device-operation{margin:0;min-width:0}
#deviceList .device-operation h4{font-size:14px;margin:0 0 4px}
#deviceList .device-command-list{margin:7px 0;gap:6px}
#deviceList .device-command-list button{min-width:70px}
#deviceList .device-operation p{margin:7px 0 0}
#deviceList .device-setup-card{padding:11px 13px;border-radius:10px;background:var(--bg);border:1px solid var(--line);min-width:0}
#deviceList .device-pair-panel{gap:5px}
#deviceList .device-pair-panel .btn{margin-top:4px;align-self:flex-start}
#deviceList .device-profile-label{color:var(--muted);font-size:12px;line-height:1.3;overflow-wrap:anywhere}
#deviceList .profile-picker{background:transparent;border:0;border-top:1px solid var(--line);border-radius:0;margin:10px 0 0;padding:9px 0 0}
#deviceList .profile-picker>summary{font-size:12px;color:var(--blue);font-weight:700;list-style-position:inside;overflow-wrap:anywhere}
#deviceList .profile-picker label{font-size:13px;margin:9px 0 5px}
#deviceList .profile-picker p{font-size:12px}
#deviceList .profile-picker select{min-height:39px}
#deviceList .device-secondary{display:flex;flex-wrap:wrap;gap:9px 18px;margin-top:12px;border-top:1px solid var(--line);padding-top:10px;align-items:flex-start}
#deviceList .device-secondary>details{min-width:min(100%,290px);flex:0 1 auto}
#deviceList .device-secondary>details[open]{flex:1 1 100%}
#deviceList .device-secondary>details>summary{font-size:13px;color:var(--muted);padding:6px 2px}
#deviceList .device-secondary>details[open]>summary{color:var(--text)}
#deviceList .device-more .device-controls{margin:8px 0 12px}
#deviceList .device-more .signal-panel,#deviceList .device-more .decoder-tech{margin-top:10px}
@media(max-width:560px){
 .page[data-view="devices"] .device-finder{padding:10px 11px;margin-bottom:8px}
 .page[data-view="devices"] .device-radio-choices{gap:5px;margin:6px 0}
 .page[data-view="devices"] .device-radio-choices .choice{padding:7px 4px;font-size:12px;min-height:48px}
 .page[data-view="devices"] #deviceSearchCount{display:none}
 .page[data-view="devices"] .pagehead{margin-bottom:9px}
 .page[data-view="devices"] .pagehead .title{font-size:27px}
}
#deviceList .pill.neutral{background:var(--bg);color:var(--muted);font-weight:600}
#deviceList .device-operation .device-unavailable{padding:9px 11px;border-radius:8px;background:var(--bg);border:1px solid var(--line);color:var(--muted);font-size:13px}
#deviceList .device-operation .device-unavailable p{margin:3px 0 0}
@media(max-width:760px){#deviceList .device-main{grid-template-columns:minmax(0,1fr)}#deviceList .device-top .device-summary{justify-content:flex-start}}
@media(max-width:540px){#deviceList .device-command-list{grid-template-columns:repeat(2,minmax(0,1fr))}#deviceList .device-command-list .btn{width:100%}#deviceList .device-secondary>details{min-width:0;flex:1 1 100%}}

/* UI55: same restrained panel language for the dashboard and settings. */
#logbookDiagnostics[hidden],#logbookDiagnostics [hidden],#logbookTimeline[hidden],#logbookColumns[hidden],#logbookToolbar[hidden],#logbookFooter[hidden],#logbookNotes[hidden],#radioLogView[hidden]{display:none!important}
#logbookDiagnostics{padding:14px 0 6px}
#logbookDiagnostics .diag-five-intro{margin:0 0 14px}
.diag-five-grid{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:10px;margin:4px 0 13px}
.diag-five-card{display:flex;text-align:left;align-items:center;gap:10px;padding:15px 12px;min-width:0;border:1px solid var(--line);border-radius:10px;background:var(--bg);color:var(--text);cursor:pointer}
.diag-five-card:hover,.diag-five-card[aria-expanded="true"]{border-color:var(--blue)}
.diag-five-copy{display:flex;flex-direction:column;gap:5px;min-width:0;flex:1}
.diag-five-copy b{font-size:14px}.diag-five-copy small{font-size:12px;color:var(--muted);overflow-wrap:anywhere;line-height:1.35}
.diag-five-icon{display:grid;place-items:center;flex:0 0 29px;height:29px;color:var(--green);border-radius:8px;background:color-mix(in srgb,var(--green) 13%,transparent);font-size:20px}
.diag-five-chevron{font-size:22px;color:var(--muted)}.diag-five-actions{display:flex;gap:8px;flex-wrap:wrap;padding:0 0 11px}
.diag-five-details{padding:12px 14px;border-radius:9px;border:1px solid var(--line);margin:5px 0 10px;background:var(--bg)}
.diag-five-details>summary{font-weight:700;cursor:pointer}.diag-five-details>p{margin:10px 0}
.diag-five-detail-stats{display:grid;gap:7px;margin:12px 0 0}.diag-five-detail-stats .row{align-items:flex-start;gap:12px;padding:7px 0;border-bottom:1px solid var(--line);font-size:13px}
.diag-five-detail-stats .row span{color:var(--muted)}.diag-five-detail-stats .row b{text-align:right;overflow-wrap:anywhere;max-width:72%}
.diag-five-detail-stats pre{overflow-x:auto;max-width:100%;font-size:11px;white-space:pre-wrap;overflow-wrap:anywhere}
/* Keep the existing TX controls, but only behind the Lab disclosure. */
#logbookDiagnostics #diagFiveLegacy:not([hidden]){display:block}
#logbookDiagnostics #diagFiveLegacy[hidden]{display:none!important}
#logbookDiagnostics #diagFiveLegacy .settings-detail-toolbar{display:none}
#logbookDiagnostics #settingsPanelDiagnostics{display:none!important}
#logbookDiagnostics > .diag-five-details:not([open]){display:none!important}
#logbookDiagnostics > .diag-five-details>summary{display:none}
#logbookDiagnostics #diagFiveLegacy .diagnostics-grid{display:block}

@media(max-width:1050px){.diag-five-grid{grid-template-columns:repeat(2,minmax(0,1fr))}}
@media(max-width:560px){.diag-five-grid{grid-template-columns:1fr 1fr;gap:7px}.diag-five-card{padding:10px 8px;gap:6px}.diag-five-copy b{font-size:12px}.diag-five-copy small{font-size:11px}.diag-five-icon{display:none}.diag-five-actions .btn{flex:1 1 auto}}
/* Settings cards: a single coherent visual system; no empty panel columns. */
.page[data-view="settings"] .settings-card{border-radius:12px;border-color:var(--line);box-shadow:none;min-width:0}
.page[data-view="settings"] .settings-card-title{display:flex;align-items:center;gap:9px;font-size:17px;line-height:1.25;margin-bottom:13px}
.page[data-view="settings"] .settings-card-title::before{display:grid;place-items:center;width:28px;height:28px;border-radius:8px;flex:none;background:color-mix(in srgb,var(--green) 12%,transparent);color:var(--green);font-size:17px}
.page[data-view="settings"] .settings-general .settings-card-title::before{content:'⚙'}
.page[data-view="settings"] .settings-logging .settings-card-title::before{content:'⌁'}
.page[data-view="settings"] .settings-assistant .settings-card-title::before{content:'▤'}
.page[data-view="settings"] .settings-log-group{padding-top:12px;margin-top:12px;border-top:1px solid var(--line)}
.page[data-view="settings"] .settings-log-group h3{margin-bottom:3px;font-size:14px}
.page[data-view="settings"] .settings-log-fields{gap:8px;margin-top:8px}
.page[data-view="settings"] .settings-pickers{gap:7px;flex-wrap:wrap}
.page[data-view="settings"] .settings-picker{white-space:normal;min-height:38px}
.page[data-view="settings"] .settings-inline-more>summary{color:var(--blue)}
.page[data-view="settings"] #firmwareAboutCard{padding-top:14px}
.page[data-view="settings"] .version-main{font-size:19px;font-weight:750;overflow-wrap:anywhere}
@media(max-width:760px){.page[data-view="settings"] .settings-grid{grid-template-columns:1fr}.page[data-view="settings"] .settings-card{min-height:0}}

/* DIAG55: Diagnose is a dashboard IN Logboeken, not another separate log buffer. */
#logbookDiagnostics{padding:2px 0 8px}
#logbookDiagnostics [hidden]{display:none!important}
#logbookDiagnostics .settings-detail-toolbar{display:none!important}
#logbookDiagnostics .settings-detail-panel{display:block;max-width:none;margin:0;padding:0}
#logbookDiagnostics .settings-wide{max-width:none;margin:0;padding:14px}
#logbookDiagnostics .settings-detail-panel .card{box-shadow:none}
#logbookDiagnostics .advanced-summary{font-size:14px}
#logbookDiagnostics .diagnostics-grid{display:grid;grid-template-columns:minmax(0,1.05fr) minmax(0,.95fr);gap:16px;align-items:start}
#logbookDiagnostics .diagnostics-col{min-width:0}
#logbookDiagnostics .diagnostics-col>h3{margin:14px 0 9px}
#logbookDiagnostics .diagnostics-intro h2{margin:0 0 4px}
#logbookDiagnostics .diagnostics-intro p{margin:0 0 12px}
#logbookDiagnostics .diagnostics-actions{display:flex;flex-wrap:wrap;gap:8px;margin:0 0 9px}
#logbookDiagnostics #radioDiagnostics{display:grid;gap:9px}
#logbookDiagnostics #radioDiagnostics>.card{margin:0;padding:12px}
#logbookDiagnostics #radioDiagnostics .rows{margin-top:6px}
#logbookDiagnostics #systemTraceCard{height:auto}
#logbookDiagnostics #diagnosticsDetails{display:block}
#logbookDiagnostics #diagnosticsDetails>summary{display:none}
#logbookDiagnostics #bootLog{max-height:245px;overflow:auto}
@media(max-width:950px){#logbookDiagnostics .diagnostics-grid{grid-template-columns:1fr}}

/* UXUNIFIED6: ONE logbook, single noise toggle, compact radio filter and stable row density. */
#logbookPanel{padding:18px;max-width:100%}
#logbookModes{display:flex;flex-wrap:wrap;gap:7px;margin-bottom:12px}
#logbookToolbar{display:flex;flex-wrap:wrap;align-items:center;justify-content:flex-start;gap:8px;margin:0 0 12px!important;padding:0 0 12px;border-bottom:1px solid var(--line)}
#logbookToolbar .field{width:auto;min-width:115px;max-width:220px;flex:0 1 150px;font-size:13px;min-height:38px;padding:7px 10px}
#logbookSearch{flex:1 1 230px!important;max-width:460px!important}
#logbookToolbar label.row{display:inline-flex;align-items:center;gap:5px;margin:0;padding:0;border:0;white-space:nowrap}
#logbookToolbar label.row input{accent-color:var(--green)}
#logbookToolbar .btn{min-height:38px;padding:7px 10px;font-size:13px}
#radioLogView{background:transparent;border:0;border-radius:0;padding:0;margin:0 0 12px;gap:5px}
.logbook-frequency{display:flex;align-items:center;gap:14px;flex-wrap:wrap}
.logbook-frequency>b{font-size:13px}
.logbook-frequency .choices{display:flex;flex-wrap:wrap;gap:7px;margin:0}
.logbook-frequency .choice{min-height:38px!important;padding:6px 12px!important;flex:0 0 auto;font-size:13px;border-radius:9px}
.logbook-frequency .choice span{display:none}
#radioLogView .monitor-advanced{margin:2px 0 0;font-size:12px}
#radioLogView .monitor-advanced>summary{font-size:12px}
.logbook-direction{display:flex;align-items:center;gap:10px;margin-top:9px;max-width:320px}
.logbook-direction .field{min-height:36px;font-size:13px}
.logbook-columns{display:grid;grid-template-columns:75px 105px 90px minmax(0,1fr);gap:11px;padding:10px 12px;background:var(--bg);border:1px solid var(--line);border-radius:9px 9px 0 0;color:var(--muted);font-size:12px;font-weight:700}
#logbookTimeline{height:clamp(340px,54vh,550px);min-height:340px;border-radius:0 0 9px 9px;border-top:0;gap:0!important;align-content:start;grid-auto-rows:max-content;padding:0 10px}
.logbook-entry{padding:0;border-bottom:1px solid var(--line)}
.logbook-entry summary{display:grid;grid-template-columns:75px 105px 90px minmax(0,1fr);gap:11px;align-items:center;padding:9px 2px;min-height:41px;font-size:13px}
.logbook-source{display:inline-flex;gap:6px;align-items:center;font-size:12px}
.logbook-source:before{content:'';display:inline-block;flex:none;width:7px;height:7px;border-radius:50%;background:#a970e0}
.logbook-source.src-radio:before{background:#26bb66}.logbook-source.src-web:before{background:#409ef4}
.logbook-severity{justify-self:start;padding:2px 7px;font-size:11px;border-radius:20px;background:var(--bg);border:1px solid var(--line)}
.logbook-severity.lv-warn{color:#e8ad38;border-color:#b77d18}.logbook-severity.lv-error{color:#f2757b;border-color:#d54b52}
.logbook-message{line-height:1.35;overflow-wrap:anywhere}
.logbook-entry>div{padding:6px 12px 12px 100px;font-size:12px}
.logbook-empty{padding:16px 7px;margin:0}
.logbook-footer{display:flex;align-items:center;justify-content:space-between;flex-wrap:wrap;gap:8px;margin-top:8px}
.logbook-footer p{margin:5px 0}
@media(max-width:720px){#logbookToolbar{display:grid;grid-template-columns:auto 1fr;gap:8px}#logbookToolbar #logbookSearch{grid-column:1/-1;width:100%;max-width:none!important;flex:none!important}#logbookToolbar .btn{width:auto}#logbookTimeline{height:430px;min-height:300px}.logbook-frequency{display:block}.logbook-frequency .choices{margin-top:7px;display:grid;grid-template-columns:repeat(2,minmax(0,1fr))}.logbook-frequency .choice{white-space:normal;flex:auto}.logbook-columns,.logbook-entry summary{grid-template-columns:53px 65px 35px minmax(0,1fr);gap:6px;font-size:11px}.logbook-source{font-size:11px}.logbook-entry>div{padding:6px 8px 12px 65px}}
/* UXUNIFIED6: configuration as a readable process, not another crowded device card. */
#deviceList .device-detail-card{max-width:1100px;margin:0 auto;padding:20px 22px}
#deviceList .device-detail-card>.device-top{padding-bottom:12px;border-bottom:1px solid var(--line)}
#deviceList .device-setup-flow{display:grid;grid-template-columns:minmax(0,1fr) minmax(0,.82fr);gap:12px;align-items:start;margin-top:16px}
#deviceList .device-setup-flow.device-unconfigured{grid-template-columns:minmax(0,780px);justify-content:center}
#deviceList .device-step-panel,#deviceList .device-setup-status{border:1px solid var(--line);background:var(--bg);border-radius:11px;padding:16px;min-width:0}
#deviceList .device-step-heading{display:flex;gap:11px;align-items:flex-start;margin-bottom:12px}
#deviceList .device-step-number{display:grid;place-items:center;background:var(--green-bg);color:var(--green);border-radius:50%;font-size:13px;font-weight:800;flex:0 0 26px;height:26px}
#deviceList .device-step-heading h4,#deviceList .device-setup-status h4{margin:0 0 3px;font-size:16px}
#deviceList .device-step-heading p{margin:0;font-size:13px;line-height:1.4}
#deviceList .device-current-profile{font-weight:650;margin:5px 0 7px}
#deviceList .device-step-panel .profile-picker{border:0;padding:0;margin:5px 0 0}
#deviceList .device-step-panel .profile-picker:not(details) label{display:none}
#deviceList .device-step-panel .profile-picker select{width:100%;max-width:none}
#deviceList .device-step-panel .profile-picker>summary{font-size:13px}
#deviceList .device-step-panel .device-pair-panel strong{display:none}
#deviceList .device-setup-status p{margin:5px 0}
#deviceList .device-secondary{margin-top:16px;padding:14px 4px 0;border-top:1px solid var(--line);align-items:start;gap:10px}
#deviceList .device-secondary-title{font-size:14px;width:100%;margin:0 0 3px}
#deviceDetailHeader{max-width:1100px;margin:0 auto 14px;gap:12px}
#deviceDetailHeader .sectiontitle{font-size:19px}
@media(max-width:760px){#deviceList .device-detail-card{padding:14px}#deviceList .device-setup-flow{grid-template-columns:minmax(0,1fr);gap:10px}#deviceList .device-step-panel,#deviceList .device-setup-status{padding:13px}#deviceDetailHeader{align-items:flex-start}}


/* UXUNIFIED6 refinement: no duplicate device name, no empty status card before onboarding. */
#deviceList .device-detail-card>.device-top{display:none}
#deviceList .device-setup-flow.device-unconfigured .device-setup-status{display:none}
#deviceList .device-setup-flow.device-unconfigured{max-width:750px;margin:12px auto 0}
#deviceList .device-secondary{display:block;border:1px solid var(--line);border-radius:10px;padding:12px 14px;margin-top:13px;background:var(--bg)}
#deviceList .device-secondary>summary{list-style:revert;cursor:pointer;font-size:13px;font-weight:700;line-height:1.4;margin:0}
#deviceList .device-secondary:not([open]){padding:10px 15px}
#deviceList .device-secondary-body{display:flex;flex-wrap:wrap;gap:10px 18px;align-items:flex-start;margin-top:12px;border-top:1px solid var(--line);padding-top:12px}
#deviceList .device-secondary-body>details{flex:1 1 260px}
#deviceList .device-current-profile{font-size:14px}
@media(max-width:760px){#deviceList .device-setup-flow.device-unconfigured{margin:10px 0 0}#deviceList .device-secondary-body{display:block}#deviceList .device-secondary-body>details{margin-top:10px}}


/* Keep paired logging toggles together, including on small screens. */
#logbookToolbar .logbook-checks{display:flex;align-items:center;gap:12px;flex-wrap:wrap}
@media(max-width:720px){#logbookToolbar .logbook-checks{grid-column:1/-1;justify-content:flex-start;gap:22px}#logbookToolbar .logbook-checks label{min-width:max-content}.logbook-columns,.logbook-entry summary{grid-template-columns:48px 61px minmax(0,1fr);gap:8px}.logbook-columns span:nth-child(3),.logbook-entry summary .logbook-severity{display:none}.logbook-entry:has(.lv-warn) summary{border-left:3px solid #b77d18}.logbook-entry:has(.lv-error) summary{border-left:3px solid #d54b52}}

/* DIAG7: stable, straightforward tool layout and obvious crash indicator. */
#logbookToolbar #logbookSeverity{flex:0 0 104px;min-width:104px;max-width:104px;width:104px}
#logbookToolbar #logbookEventType{flex:0 0 126px;min-width:126px;max-width:126px;width:126px}
#logbookToolbar #logbookSearch{flex:1 1 155px;min-width:145px;max-width:230px}

.crash-status-notice{display:flex;align-items:center;justify-content:space-between;gap:12px;border-left:3px solid #d68b27}
.crash-status-notice[hidden]{display:none!important}
.crash-status-notice p{margin:5px 0 0}
#logbookTimeline .logbook-entry summary{grid-template-columns:120px 100px 90px minmax(0,1fr)}
#logbookToolbar #logbookEventType{min-width:138px;max-width:190px}
#deviceList .device-detail-card .device-secondary-body{display:grid;grid-template-columns:minmax(0,1fr) minmax(0,1fr);gap:12px;align-items:start}
#deviceList .device-detail-card .device-secondary-body > details{background:var(--card);border:1px solid var(--line);border-radius:10px;padding:10px 13px;margin:0;min-width:0}
#deviceList .device-detail-card .device-secondary-body > details[open]{padding-bottom:14px}
#deviceList .device-detail-card .device-secondary-body > details summary{font-weight:700;cursor:pointer}
#deviceList .device-detail-card .device-secondary-body > details .row{gap:10px}
#deviceList .device-detail-card .device-secondary-body [data-feature^="remove"]{opacity:.8;font-size:11px}
@media(max-width:850px){#deviceList .device-detail-card .device-secondary-body{grid-template-columns:1fr}}
@media(max-width:720px){#logbookTimeline .logbook-entry summary{grid-template-columns:105px 65px minmax(0,1fr)}#logbookToolbar #logbookEventType{min-width:100px;max-width:none}.crash-status-notice{flex-wrap:wrap}}

#deviceList .rf60-remove-footer{margin-top:12px;padding:14px 16px;border:1px solid color-mix(in srgb,#d64b55 50%,var(--line));background:color-mix(in srgb,#d64b55 5%,var(--card));border-radius:10px;display:flex;align-items:center;justify-content:space-between;gap:12px;flex-wrap:wrap}
#deviceList .rf60-remove-footer b{font-size:14px}
#deviceList .rf60-remove-footer p{margin:4px 0 0;max-width:660px}
#deviceList .rf60-remove-footer .btn{border-color:#d64b55;color:#ed8085}
/* RADIO GAGA v0.6 — design system: the primary action is visible; advanced data is opt-in. */
:root{--rf60-soft:color-mix(in srgb,var(--card) 86%,var(--bg));--rf60-accent:color-mix(in srgb,var(--green) 14%,transparent)}
.main{max-width:1700px;padding:24px clamp(16px,2.1vw,32px)}
.card{border-radius:13px;box-shadow:none}
.title{font-size:clamp(26px,2.1vw,32px);letter-spacing:-.03em}
.sub{font-size:14px;margin-bottom:16px}
.btn{min-height:38px;padding:8px 12px}
.rf60-status-page .rf60-metrics{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:12px;margin:8px 0 14px}
.rf60-metric{min-height:102px;display:flex;flex-direction:column;justify-content:center;gap:3px}
.rf60-metric>span{font-weight:650;font-size:13px;color:var(--muted)}
.rf60-metric strong{font-size:25px;font-weight:780;line-height:1.2}
.rf60-metric small{font-size:12px}
.rf60-attention-metric:has(strong:not(:empty)){border-color:var(--line)}
.rf60-status-main,.rf60-status-bottom{display:grid;grid-template-columns:1fr 1fr;gap:14px;margin:0 0 14px;align-items:stretch}
.rf60-status-main>.card,.rf60-status-bottom>.card{min-height:200px}
.rf60-panel-title{margin:0 0 14px}
.rf60-panel-title h2{font-size:17px;line-height:1.3;margin:0 0 4px}
.rf60-radio-rows{border:1px solid var(--line);border-radius:11px;overflow:hidden}
.rf60-radio-row{width:100%;display:grid;grid-template-columns:12px minmax(90px,1fr) auto 16px;align-items:center;gap:12px;text-align:left;padding:12px 14px;border:0;border-bottom:1px solid var(--line);background:transparent;color:var(--text);cursor:pointer}
.rf60-radio-row:last-child{border-bottom:0}.rf60-radio-row:hover{background:var(--rf60-accent)}
.rf60-radio-row:disabled{opacity:.65;cursor:default}.rf60-radio-row strong{font-size:14px}
.rf60-dot{width:9px;height:9px;border-radius:50%;background:var(--amber)}.rf60-dot.is-ready{background:var(--green)}
.rf60-radio-arrow{color:var(--muted);font-size:23px;line-height:1}
.rf60-activity-panel #activity{margin-top:6px}.rf60-activity-panel #activity .row{font-size:13px;gap:10px;overflow-wrap:anywhere}
.rf60-textlink{background:none;border:0;color:var(--blue);padding:12px 0 0;cursor:pointer;font-weight:700}
.rf60-status-bottom{grid-template-columns:minmax(0,1fr) minmax(0,1fr)}
.rf60-attention-panel #crashStatusNotice{padding:10px 12px;margin:12px 0}.rf60-health-details{margin-top:10px;border-top:1px solid var(--line);padding-top:12px}
.rf60-health-details>summary{color:var(--muted);font-weight:600;font-size:13px}
.rf60-health-details>.health{margin:12px 0 0;padding:13px}.rf60-health-details .health h2{font-size:16px}
.rf60-quick-links{display:grid;grid-template-columns:1fr;gap:9px}
.rf60-action{border:1px solid var(--line);border-radius:9px;color:var(--text);background:var(--rf60-soft);padding:10px 14px;text-align:left;cursor:pointer;font-weight:650}
.rf60-action:hover{border-color:var(--green)}
/* Device listing: each card has one task and one expandable help area. */
.page[data-view="devices"]:not(.device-detail-mode) #deviceList{display:grid;grid-template-columns:repeat(auto-fit,minmax(min(100%,315px),1fr));gap:13px;align-items:stretch}
.page[data-view="devices"] .device-finder{display:flex;align-items:center;gap:14px;padding:12px 14px;margin-bottom:14px;flex-wrap:wrap}
.page[data-view="devices"] #deviceFrequencyLabel,.page[data-view="devices"] .device-finder>label{position:absolute;width:1px;height:1px;overflow:hidden;clip:rect(0,0,0,0)}
.page[data-view="devices"] .device-radio-choices{flex:1 1 auto;display:flex;gap:6px;min-width:280px;margin:0}
.page[data-view="devices"] .device-radio-choices .choice{min-height:40px;flex:0 1 auto;padding:8px 11px;font-size:12px}
.page[data-view="devices"] .device-radio-choices .choice span{display:none}
.page[data-view="devices"] .device-find-row{flex:2 1 240px;display:grid;grid-template-columns:minmax(0,1fr) auto}
.page[data-view="devices"] #deviceClearFilter{padding:7px 9px}
.page[data-view="devices"] #deviceList .rf60-device{display:flex;flex-direction:column;gap:12px;padding:17px 17px 15px;min-height:228px}
.rf60-device-head{display:flex;align-items:flex-start;gap:10px;min-width:0}
.rf60-device-icon{border-radius:11px;background:var(--green-bg);color:var(--green);width:40px;height:40px;flex:none;display:grid;place-items:center;font-size:23px}
.rf60-device-title{min-width:0;flex:1}.rf60-device-title h3{font-size:17px!important;line-height:1.25;margin:2px 0 5px!important;overflow-wrap:anywhere}
.rf60-device-title p{font-size:12px;line-height:1.3;overflow-wrap:anywhere}
.rf60-device-head .pill{font-size:11px;max-width:136px;text-align:center;flex:0 0 auto}
.rf60-device-last{display:flex;flex-direction:column;gap:3px;border-top:1px solid var(--line);padding-top:12px}
.rf60-device-last b{font-size:13px}.rf60-device-hint,.rf60-pending{margin:0;font-size:12px}.rf60-pending{color:var(--amber)}
.rf60-device-foot{display:flex;gap:8px;align-items:center;flex-wrap:wrap;margin-top:auto}
.rf60-device-foot>.btn,.rf60-device-foot>.rf60-quick>.btn{min-height:36px}
.rf60-quick{position:relative}.rf60-quick>summary{list-style:none;display:inline-block;user-select:none;cursor:pointer}
.rf60-quick>summary::-webkit-details-marker{display:none}
.rf60-quick[open]{width:100%;order:3}.rf60-quick[open]>.rf60-quick-body{margin:9px 0;padding:12px;border:1px solid var(--line);border-radius:10px;background:var(--bg)}
.rf60-quick .device-command-list{display:flex;flex-wrap:wrap}
.rf60-more{margin-left:auto;order:2;font-size:12px}.rf60-more>summary{color:var(--muted);font-weight:650;cursor:pointer}
.rf60-more[open]{width:100%;order:4;border-top:1px solid var(--line);padding-top:8px}
/* Device details: one clear profile/pairing path, one controls panel, one optional technical section. */
.page[data-view="devices"].device-detail-mode #deviceDetailHeader{max-width:1160px;width:100%;margin:0 auto 14px}
#deviceList .device-detail-card{max-width:1160px;width:100%;padding:16px;display:block}
#deviceList .device-setup-flow{grid-template-columns:repeat(2,minmax(0,1fr));gap:12px;margin-top:0}
#deviceList .device-setup-flow.device-unconfigured{grid-template-columns:minmax(0,1fr);max-width:720px}
#deviceList .device-step-panel{background:var(--rf60-soft);padding:18px;border-radius:12px}
#deviceList .device-step-panel .device-step-heading{margin-bottom:11px}
#deviceList .device-step-number{width:26px;height:26px}
#deviceList .rf60-device-control-panel{margin-top:12px;padding:15px 17px}
#deviceList .rf60-device-control-panel h4{margin:0 0 6px;font-size:16px}
#deviceList .rf60-device-control-panel .device-operation h4{display:none}
#deviceList .rf60-device-control-panel .device-unavailable{background:var(--rf60-soft)}
#deviceList .device-secondary{padding:0;background:transparent;margin-top:12px;border:1px solid var(--line)}
#deviceList .device-secondary>summary.rf60-secondary-heading{padding:13px 16px;font-size:14px;display:flex;align-items:center;gap:14px;flex-wrap:wrap;list-style:revert}
#deviceList .device-secondary>summary .muted{font-weight:400;font-size:12px}
#deviceList .device-secondary:not([open]){padding:0}
#deviceList .device-secondary-body{padding:14px;margin:0;display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:12px}
#deviceList .device-secondary-body>details{padding:12px;background:var(--rf60-soft)}
#deviceList .device-secondary-body>details>summary{font-size:13px}
.page[data-view="learn"] .card,.page[data-view="settings"] .card,.page[data-view="monitor"] .card{box-shadow:none}
.page[data-view="learn"] .taskgrid{gap:12px}
.page[data-view="learn"] .task{min-height:95px}
.page[data-view="settings"] .settings-card{border-radius:13px}
.page[data-view="settings"] .settings-card-title{font-size:16px}
.page[data-view="settings"] .settings-pickers{gap:8px}
.page[data-view="settings"] .settings-picker{border-radius:10px}
#deviceList .rf60-device .rf60-device-head{display:grid;grid-template-columns:40px minmax(0,1fr);gap:4px 10px;align-items:start}
#deviceList .rf60-device .rf60-device-icon{grid-row:1/span 2}
#deviceList .rf60-device .rf60-device-title{grid-column:2;min-width:0}
#deviceList .rf60-device .rf60-device-head .pill{grid-column:2;justify-self:start;max-width:100%;margin:0 0 2px}
#deviceList .rf60-device .rf60-device-last{width:100%}
#deviceList .rf60-device .rf60-device-title h3{overflow-wrap:normal;word-break:normal}
@media(max-width:1100px){.rf60-status-page .rf60-metrics{grid-template-columns:repeat(2,minmax(0,1fr))}.page[data-view="devices"] .device-radio-choices{flex:1 1 100%}}
@media(max-width:800px){.rf60-status-main,.rf60-status-bottom{grid-template-columns:1fr}#deviceList .device-setup-flow{grid-template-columns:1fr!important}#deviceList .device-secondary-body{grid-template-columns:1fr!important}}
@media(max-width:540px){.rf60-status-page .rf60-metrics{gap:8px}.rf60-metric{min-height:85px;padding:12px}.rf60-metric strong{font-size:21px}.page[data-view="devices"] .device-finder{padding:11px}.page[data-view="devices"] .device-radio-choices{display:grid;grid-template-columns:repeat(2,minmax(0,1fr))}.page[data-view="devices"] .device-radio-choices .choice{min-width:0}.rf60-device-head{flex-wrap:wrap}.rf60-device-head .pill{margin-left:49px}.rf60-device-foot{gap:7px}.rf60-device-foot .btn{font-size:12px}}


/* Radio Gaga 0.6.1: device details without the old accordion-inside-accordion.
   Primary actions are immediately visible; potentially large RF data remains
   opt-in, with clear context before each technical expander. */
#deviceList .device-secondary.rf61-device-secondary{padding:15px 16px;border:1px solid var(--line);border-radius:12px;background:transparent}
#deviceList .rf61-section-heading{display:flex;align-items:baseline;gap:12px;flex-wrap:wrap;margin-bottom:12px}
#deviceList .rf61-section-heading h4{font-size:15px;margin:0}
#deviceList .rf61-section-heading p{margin:0}
#deviceList .rf61-section-grid{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:10px}
#deviceList .rf61-info-card{padding:13px 15px;border:1px solid var(--line);border-radius:11px;background:var(--rf60-soft);min-width:0}
#deviceList .rf61-full{grid-column:1/-1}
#deviceList .rf61-card-heading{display:flex;gap:11px;align-items:center;min-width:0}
#deviceList .rf61-card-heading>div{min-width:0}
#deviceList .rf61-icon{width:32px;height:32px;flex:none;display:grid;place-items:center;border-radius:9px;background:var(--green-bg);color:var(--green);font-size:18px}
#deviceList .rf61-card-heading h4{font-size:14px;margin:0 0 4px}
#deviceList .rf61-card-heading p{margin:0;font-size:12px}
#deviceList .rf61-card-controls{margin:9px 0 0 43px}
#deviceList .rf61-info-card>.device-controls{margin:9px 0 0 43px;display:flex;flex-wrap:wrap;gap:8px}
#deviceList .rf61-info-card>.signal-panel,#deviceList .rf61-info-card>.decoder-tech{margin:10px 0 0 43px;padding:0;border:0;background:transparent}
#deviceList .rf61-info-card>.signal-panel>summary,#deviceList .rf61-info-card>.decoder-tech>summary{font-size:13px;color:var(--blue);cursor:pointer}
#deviceList .rf61-small-grid{grid-column:1/-1;display:grid;grid-template-columns:repeat(auto-fit,minmax(min(100%,275px),1fr));gap:10px}
#deviceList .rf61-compact-card{display:flex;flex-direction:column;justify-content:space-between;gap:5px}
@media(max-width:1000px){#deviceList .rf61-section-grid{grid-template-columns:repeat(2,minmax(0,1fr))}}
@media(max-width:760px){#deviceList .rf61-section-grid{grid-template-columns:minmax(0,1fr)}#deviceList .rf61-card-controls,#deviceList .rf61-info-card>.device-controls,#deviceList .rf61-info-card>.signal-panel,#deviceList .rf61-info-card>.decoder-tech{margin-left:0}}

</style>
</head>
<body>
<header class="top"><div class="brand" data-i18n="static.000">RFLink Universal</div><div class="topstate"><span class="tag good" id="onlineTag">● Online</span><span class="tag" id="frameTag">0 signals</span><span class="tag" id="uptimeTag">Uptime —</span></div></header>
<div class="shell">
<aside class="side"><nav class="nav"><button data-page="status" class="active" data-i18n="static.004">◉ Status</button><button data-page="devices" data-i18n="static.005">▣ Devices</button><button data-page="monitor" data-i18n="logbook.title_nav">◷ Logs</button><button data-page="learn" data-i18n="static.007">◇ Add device</button><button data-page="settings" data-i18n="static.008">⚙ Settings</button></nav></aside>
<main class="main">
<section class="page active rf60-status-page" data-view="status">
<h1 class="title" data-i18n="static.009">Status</h1><div class="sub" data-i18n="rf60.status_intro">Een rustig overzicht van je systeem, radio's en aandachtspunten.</div>
<div class="rf60-metrics" aria-label="RFLink status">
 <div class="card rf60-metric"><span data-i18n="rf60.system">Systeem</span><strong id="rf60System">—</strong><small id="rf60Firmware" class="muted"></small></div>
 <div class="card rf60-metric"><span data-i18n="rf60.radios">Radio's</span><strong id="rf60Radios">—</strong><small data-i18n="rf60.radios_help" class="muted">Beschikbare radio's</small></div>
 <div class="card rf60-metric"><span data-i18n="rf60.devices">Apparaten</span><strong id="rf60Devices">—</strong><small data-i18n="rf60.saved" class="muted">Opgeslagen apparaten</small></div>
 <div class="card rf60-metric rf60-attention-metric"><span data-i18n="rf60.attention">Aandachtspunten</span><strong id="rf60Attention">—</strong><small id="rf60AttentionDetail" class="muted"></small></div>
</div>
<div class="rf60-status-main">
 <section class="card rf60-radio-panel"><div class="rf60-panel-title"><h2 data-i18n="rf60.radio_overview">Radio-overzicht</h2><span class="muted small" data-i18n="rf60.radio_help">Open een radio voor ontvangst- en zendgegevens.</span></div><div id="radioCards" class="rf60-radio-rows"><p class="muted" data-i18n="static.014">Radiostatus laden…</p></div></section>
 <section class="card rf60-activity-panel"><div class="rf60-panel-title"><h2 data-i18n="static.029">Recente gebeurtenissen</h2><span class="muted small" data-i18n="rf60.activity_help">Laatste activiteiten; het volledige logboek staat onder Logboeken.</span></div><div id="activity" class="rows"><div class="row muted"><span data-i18n="static.030">Nog geen activiteiten.</span></div></div><button class="rf60-textlink" data-go="monitor" type="button" data-i18n="rf60.open_logs">Volledig logboek openen</button></section>
</div>
<div class="rf60-status-bottom">
 <section class="card rf60-attention-panel"><div class="rf60-panel-title"><h2 data-i18n="rf60.attention_title">Aandacht nodig</h2><span class="muted small" data-i18n="rf60.attention_help">Alleen problemen die je aandacht vragen.</span></div><p id="rf60NoProblems" class="muted small" data-i18n="rf60.no_problems">Geen aandachtspunten.</p><button id="rf60AttentionOpen" class="rf60-textlink" type="button" data-go="monitor" data-log-target="diagnostics" data-i18n="rf60.inspect">Diagnose bekijken</button>
 <div id="crashStatusNotice" class="notice warn crash-status-notice" hidden role="status"><div><b id="crashStatusTitle"></b><p id="crashStatusHelp" class="muted small"></p></div><button class="btn out small" id="crashStatusOpen" type="button" data-i18n="diag7.view_log">Bekijk crashlog</button></div>
 <div class="library-attention" id="libraryAttentionCard" hidden><div class="row"><div><b data-i18n="library.attention_title">Bibliotheek vraagt aandacht</b><p id="libraryAttentionText" class="muted small"></p></div><button class="btn out small" type="button" id="libraryAttentionOpen" data-i18n="library.review">Wijzigingen bekijken</button></div></div>
 <details class="rf60-health-details"><summary data-i18n="rf60.system_details">Uitgebreide systeemstatus</summary><div class="card health" id="healthCard" role="status" aria-live="polite"><h2 id="healthTitle">Status laden…</h2><p id="healthExplanation" class="muted">Status laden.</p><div class="rows" id="healthDetails"></div><button class="btn out" type="button" data-go="monitor" data-log-target="diagnostics" style="margin-top:10px" data-i18n="static.013">Diagnose bekijken</button></div></details></section>
 <section class="card rf60-quick-actions"><div class="rf60-panel-title"><h2 data-i18n="rf60.quick_actions">Snelle acties</h2><span class="muted small" data-i18n="rf60.quick_help">Veelgebruikte taken, direct bereikbaar.</span></div><div class="rf60-quick-links"><button type="button" class="rf60-action" data-go="learn" data-learn-mode="device" data-i18n="static.021">+ Apparaat toevoegen</button><button type="button" class="rf60-action" data-go="devices" data-i18n="ui.manage_devices">Apparaten beheren</button><button type="button" class="rf60-action" data-go="settings" data-i18n="static.008">Instellingen</button></div></section>
</div>
</section>

<section class="page" data-view="devices">
<div class="pagehead"><div><h1 class="title" data-i18n="static.031">Devices</h1><div class="sub" id="deviceSub">Your saved RF devices. Open a device only when you want to change something.</div></div><button class="btn out" type="button" id="pairDevicesToggle" data-i18n="devices.pairing_filter">Show pairing devices</button></div>
<div id="deviceDetailHeader" class="device-detail-header" hidden><button class="btn out" type="button" id="deviceDetailBack" data-i18n="deviceux.back">← Back to devices</button><div><h2 id="deviceDetailTitle" class="sectiontitle"></h2><p class="muted small" data-i18n="deviceux6.intro">Kies een profiel, koppel waar nodig en beheer extra opties.</p></div></div>
<div class="device-finder card" role="search"><b id="deviceFrequencyLabel" data-i18n="static.040">Filter by frequency</b><div class="choices device-radio-choices" id="deviceRadioChoices" role="group" aria-labelledby="deviceFrequencyLabel"><button class="choice active" type="button" data-dev-radio="all" aria-pressed="true"><b data-i18n="rxlog.all_radios">All radios</b><span class="muted small" data-radio-device-count="all">—</span></button><button class="choice" type="button" data-dev-radio="0" aria-pressed="false"><b>433.42 MHz</b><span class="muted small" data-radio-device-count="0">—</span></button><button class="choice" type="button" data-dev-radio="1" aria-pressed="false"><b>433.92 MHz</b><span class="muted small" data-radio-device-count="1">—</span></button><button class="choice" type="button" data-dev-radio="2" aria-pressed="false"><b>868.30 MHz</b><span class="muted small" data-radio-device-count="2">—</span></button><button class="choice" type="button" data-dev-radio="unknown" aria-pressed="false" id="deviceUnknownRadioChoice" hidden><b data-i18n="nav4.unknown_radio">Unknown radio</b><span class="muted small" data-radio-device-count="unknown">—</span></button></div><label for="deviceSearch" data-i18n="nav4.search_devices">Search saved devices</label><div class="device-find-row"><input class="field" type="search" id="deviceSearch" autocomplete="off" data-i18n-placeholder="nav4.search_placeholder" placeholder="Name, protocol or radio…"><button class="btn out small" type="button" id="deviceClearFilter" data-i18n="nav4.clear">Clear</button></div><p class="muted small" id="deviceSearchCount" aria-live="polite"></p></div>
<div id="pairNotice" class="notice" hidden data-i18n="static.033">Only devices that support pairing appear here. Follow the pairing steps shown on the device.</div>
<div class="notice" id="deviceStorageNotice">Checking device storage…</div><div class="list" id="deviceList"><div class="card muted" data-i18n="static.035">Loading devices…</div></div>
<div id="candidateSection" class="candidate-section" hidden><h2 class="sectiontitle" data-i18n="devices.waiting_signals">Signals waiting for review</h2><p class="muted small" data-i18n="devices.waiting_signals_help">These signals were recognized outside the guided Add device flow. Review only the ones you want to keep.</p><div class="list" id="candidateList"><div class="card muted" data-i18n="static.037">Loading candidates…</div></div></div>
</section>

<section class="page" data-view="monitor">
<h1 class="title" data-i18n="logbook.title">Logbooks</h1><div class="sub" data-i18n="logbook.subtitle">Follow radio events, firmware decisions and browser actions in one place.</div>
<div class="card" id="logbookPanel">
 <div class="catalog-modes" role="group" aria-label="Logbook source" id="logbookModes">
  <button type="button" class="decoder-filter-btn active" data-log-view="all" data-i18n="logbook.all">All</button>
  <button type="button" class="decoder-filter-btn" data-log-view="radio" data-i18n="logbook.radio">Radio</button>
  <button type="button" class="decoder-filter-btn" data-log-view="system" data-i18n="logbook.system">System</button>
  <button type="button" class="decoder-filter-btn" data-log-view="web" data-i18n="logbook.web">Web</button>
  <button type="button" class="decoder-filter-btn" data-log-view="diagnostics" data-i18n="diag55.tab">Diagnose</button>
 </div>
 <div id="logbookDiagnostics" hidden aria-label="Diagnose">
  <p class="muted small diag-five-intro" data-i18n="diag55.intro">Boots, crashes, radiostatus en experimenteel zenden — open alleen wat je wilt onderzoeken.</p>
  <div class="diag-five-grid" aria-label="Diagnoseonderdelen">
    <button type="button" class="diag-five-card" data-diag-show="diagFiveBoot"><span class="diag-five-icon">↻</span><span class="diag-five-copy"><b data-i18n="diag55.boots">Crashes & herstarts</b><small id="diagFiveBootSummary">—</small></span><span class="diag-five-chevron">›</span></button>
    <button type="button" class="diag-five-card" data-diag-show="diagFiveRadio"><span class="diag-five-icon">⌁</span><span class="diag-five-copy"><b data-i18n="diag55.radio">Radio's & SPI</b><small id="diagFiveRadioSummary">—</small></span><span class="diag-five-chevron">›</span></button>
    <button type="button" class="diag-five-card" data-diag-show="diagFiveMemory"><span class="diag-five-icon">▤</span><span class="diag-five-copy"><b data-i18n="diag55.memory">Geheugen</b><small id="diagFiveMemorySummary">—</small></span><span class="diag-five-chevron">›</span></button>
    <button type="button" class="diag-five-card" data-diag-show="diagFiveLab"><span class="diag-five-icon">⚗</span><span class="diag-five-copy"><b data-i18n="diag55.lab">Experimenteel</b><small id="diagFiveLabSummary">—</small></span><span class="diag-five-chevron">›</span></button>
  </div>
  <div class="diag-five-actions"><button type="button" class="btn out small" id="diagnosticOpenCrashes" data-i18n="diag55.open_crashes">Crashes in tijdlijn</button><button type="button" class="btn out small" id="diagnosticOpenRadio" data-i18n="diag55.open_radio">Radiolog</button><button type="button" class="btn out small" id="diagnosticRefresh" data-i18n="logbook.refresh">Verversen</button><button type="button" class="btn out small" id="diagnosticReport" data-i18n="diag55.export">Diagnosepakket (JSON)</button></div>
  <details class="diag-five-details" id="diagFiveBoot"><summary data-i18n="diag55.boots">Crashes & herstarts</summary><p class="muted small" data-i18n="diag55.boot_hint">Recente starts, resetredenen en technische crashgegevens zonder verzonnen tijdstippen.</p><div class="diag-five-detail-stats" id="diagFiveBootDetails"></div></details>
  <details class="diag-five-details" id="diagFiveRadio"><summary data-i18n="diag55.radio">Radio's & SPI</summary><div class="diag-five-detail-stats" id="diagFiveRadioDetails"></div></details>
  <details class="diag-five-details" id="diagFiveMemory"><summary data-i18n="diag55.memory">Geheugen</summary><div class="diag-five-detail-stats" id="diagFiveMemoryDetails"></div></details>
  <details class="diag-five-details" id="diagFiveLab"><summary data-i18n="diag55.lab">Experimenteel</summary><p class="muted small" data-i18n="diag55.lab_hint">TX, UART/GDO, radioherstel en uitgebreide systeemdiagnose. Testuitzendingen worden nooit automatisch gestart.</p>
  <div class="diagnostics-grid" id="diagFiveLegacy" hidden>
  <div class="diagnostics-col"><h3 data-i18n="diag55.radio_heading">Radio's en SPI-controles</h3><div class="settings-detail-panel" id="settingsPanelDiagnostics">
<div class="settings-detail-toolbar"><button type="button" class="btn out small settings-back" data-settings-back data-i18n="nav.back_settings">← Back to settings overview</button></div>
<div class="card settings-wide advanced-section"><details id="diagnosticsDetails" open><summary class="advanced-summary" data-i18n="static.125">Diagnostics</summary><p id="diagOverview" class="muted small">Loading radio and decoder status.</p><div class="rows" id="bootDiagnostics" style="margin:10px 0"></div>
<details style="margin:12px 0" open><summary data-i18n="diag.boot_log">Recent starts</summary>
<p class="muted small" data-i18n="diag.boot_log_hint">Stored on the device without timestamps. A restart entry is saved once on each boot.</p>
<div id="bootLog" class="rows"></div>
<div id="bootLiveEvents" class="small muted" style="margin:10px 0"></div>
<button id="bootLogExport" hidden type="button" class="btn out small" data-i18n="diag.log_download">Export diagnostics</button></details>
<div id="radioDiagnostics" class="list" style="margin:12px 0"></div><div class="rows"><div class="row"><span data-i18n="static.127">Saved devices</span><b id="diagStored">—</b></div><div class="row"><span data-i18n="static.128">Decoder registry</span><b id="diagDecoders">—</b></div><div class="row"><span data-i18n="static.129">Storage</span><b id="diagStorage">—</b></div><div class="row"><span data-i18n="static.130">Received</span><b id="diagTotal">—</b></div><div class="row"><span data-i18n="static.131">Decoded</span><b id="diagDecoded">—</b></div><div class="row"><span data-i18n="static.132">Unknown</span><b id="diagUnknown">—</b></div><div class="row"><span data-i18n="diag.noise_frames">Suspected noise</span><b id="diagNoise">—</b></div></div><details style="margin-top:12px"><summary data-i18n="settings.developer_info">Developer information</summary><p class="muted small" data-i18n="static.134">A pulse is a period when the received RF signal is HIGH or LOW. One bit can contain several pulses. Developer Mode uses their timing to compare protocols.</p><b data-i18n="static.121">Maintenance rule</b><p class="small" data-i18n="static.122">A decoder should have only one job:</p><div class="notice mono" data-i18n="static.123">RFFrame → DecodeResult</div><p class="muted small" data-i18n="static.124">No web UI, Home Assistant, GPIO settings or logic from other decoders inside a protocol module.</p></details></details></div>
</div></div>
  <div class="diagnostics-col"><h3 data-i18n="diag55.experimental_heading">Experimenteel zenden en systeemtrace</h3><div class="settings-detail-panel" id="settingsPanelSystemtrace">
  <div class="settings-detail-toolbar"><button type="button" class="btn out small settings-back" data-settings-back data-i18n="nav.back_settings">← Back to settings overview</button></div>
  <div class="card settings-wide" id="systemTraceCard">
    <div class="row" style="align-items:flex-start;flex-wrap:wrap"><div><h2 class="sectiontitle" style="margin:0" data-i18n="version.lab_tools">TX-lab en systeemdiagnose</h2>
      <p class="muted small" style="margin:4px 0" data-i18n="debug.help">What RFLink does after a signal or button: recognition, learning, profile, pairing, TX and radio restoration. Separate from the RF log.</p></div>
      <span class="tag" id="systemTraceHealth" role="status">—</span>
    </div>
    <div class="rows" style="margin:10px 0"><div class="row"><span data-i18n="debug.lab_gate">GDO TX lab</span><b id="traceGdoLab">—</b></div><div class="row"><span data-i18n="debug.probe_gate">RAMSES TX probe</span><b id="traceProbeLab">—</b></div><div class="row"><span data-i18n="debug.radio">868 MHz radio state</span><b id="traceRadioState">—</b></div></div>
    <p class="muted small" data-i18n="version.lab_help">Deze pagina is voor experimentele radio- en TX-diagnose. Voor normale systeemgebeurtenissen gebruik je Logboeken → Systeem.</p>
    <div class="actions"><button class="btn out small" type="button" id="openSystemLogbook" data-i18n="version.open_logs">Open systeemlogboek</button></div>
    <div class="systemtrace-technical-wrap" hidden><details id="systemTraceTechnical" class="decoder-tech"><summary data-i18n="diag7.raw_trace">Technical trace (optional)</summary>
    <div class="actions" style="align-items:center;margin:8px 0">
      <label style="display:flex;align-items:center;gap:6px"><span data-i18n="debug.filter">Event type</span><select id="systemTraceFilter" class="field" style="width:auto"><option value="all" data-i18n="debug.all">All</option><option value="tx">TX</option><option value="radio">Radio</option><option value="lab">Lab</option><option value="decoder">Decoder</option><option value="learn">Learn</option><option value="device">Device</option><option value="boot">Boot</option></select></label>
      <label style="display:flex;align-items:center;gap:7px"><input type="checkbox" id="systemTraceLive" checked><span data-i18n="debug.live">Live</span></label>
      <button class="btn out small" type="button" id="systemTraceRefresh" data-i18n="debug.refresh">Refresh</button>
      <button class="btn out small" type="button" id="systemTraceExport" data-i18n="debug.export">Export trace</button>
    </div>
    <p id="systemTraceCount" class="muted small" role="status"></p>
    <div id="systemTraceEvents" aria-live="off" class="list" style="max-height:460px;overflow:auto;gap:5px"><p class="muted small" data-i18n="debug.empty">No system events yet.</p></div></details></div>
    <details class="decoder-tech" style="margin-top:14px"><summary data-i18n="debug.experiment">Advanced: physical RAMSES transmission test</summary>
      <p class="muted small" data-i18n="debug.experiment_help">Only sends a 10E0 self-description test frame, NOT an Orcon command or pairing. Enable both YAML flags first. Use only on your own equipment and observe with another receiver or RF instrument.</p>
      <div class="actions"><button class="btn out small" type="button" id="systemTraceProbe" disabled data-i18n="debug.send">Send one test frame</button></div>
      <p id="systemTraceProbeStatus" class="muted small" role="status"></p>
    </details>
    <p class="muted small" data-i18n="debug.privacy">128 events in RAM, lost on restart. May contain device IDs; review before sharing. No raw radio frames or passwords are stored here.</p>
  </div>
</div></div>
  </div>
  </details>
 </div>
 <div class="row" id="logbookToolbar">
  <label for="logbookSeverity" data-i18n="logbook.level">Level</label><select class="field" id="logbookSeverity" ><option value="all" data-i18n="logbook.any">All</option><option value="warn" data-i18n="logbook.warnings">Warnings &amp; errors</option><option value="error" data-i18n="logbook.errors">Errors</option></select>
  <label for="logbookEventType" data-i18n="diag7.event_type">Type</label><select class="field" id="logbookEventType"><option value="all" data-i18n="logbook.any">All</option><option value="boot" data-i18n="diag7.boot_filter">Crashes / starts</option><option value="tx" data-i18n="diag7.tx_filter">Sending / pairing</option><option value="spi" data-i18n="diag55.spi_filter">SPI / radioherstel</option><option value="api" data-i18n="diag7.api_filter">API / browser</option></select>
  <label for="logbookSearch" data-i18n="logbook.search">Search</label><input class="field" id="logbookSearch" type="search" autocomplete="off"  data-i18n-placeholder="logbook.search_hint" placeholder="Device, ID, action…">
  <div class="logbook-checks"><label class="row"><input type="checkbox" id="logbookNoise"><span data-i18n="logbook.noise">Include noise</span></label><label class="row"><input type="checkbox" id="logbookLive" checked><span data-i18n="logbook.live">Live</span></label></div>
  <button type="button" class="btn out small" id="logbookRefresh" data-i18n="logbook.refresh">Refresh</button>
  <button type="button" class="btn out small" id="logbookExport" data-i18n="logbook.export">Export view</button>
 </div>
 <div class="logbook-radio-tools" id="radioLogView" hidden>
  <div class="logbook-frequency"><b data-i18n="logbook6.frequency">Frequentie</b><div class="choices" id="monitorRadios" role="group" aria-label="Radiofrequentie"><span class="muted" data-i18n="static.041">Radio's laden…</span></div></div>
  <details class="monitor-advanced"><summary data-i18n="ui.advanced_filters">Geavanceerde filters</summary><div class="logbook-direction"><label for="monitorDirection" data-i18n="static.042">Richting</label><select class="field" id="monitorDirection"><option value="all" data-i18n="static.043">RX en TX</option><option value="RX" data-i18n="static.044">Alleen RX</option><option value="TX" data-i18n="static.045">Alleen TX</option></select></div><p class="muted small" data-i18n="static.046">TX betekent aangeboden aan de zender, niet bevestigd door het apparaat.</p></details>
 </div>
 <!-- Legacy APIs retain their IDs for backwards-compatible host tests, but are never shown twice. -->
 <div hidden aria-hidden="true"><select id="monitorLogType"><option value="all" selected>All</option><option value="signal">Signals</option><option value="noise">Noise</option></select><button id="monitorCopy" type="button">Copy</button><button id="monitorExport" type="button">Export</button><div id="monitorHeading"></div><span id="rxLogStats"></span><div id="monitorActivity"></div><button id="rxLogOlder" type="button">Older</button></div>
 <div class="logbook-columns" id="logbookColumns" aria-hidden="true"><span data-i18n="logbook6.time">Tijd</span><span data-i18n="logbook6.source">Bron</span><span data-i18n="logbook.level">Niveau</span><span data-i18n="logbook6.event">Gebeurtenis</span></div>

 <div id="logbookTimeline" class="list" aria-live="off" style="gap:4px"></div>
 <div class="logbook-footer" id="logbookFooter"><p class="muted small" id="logbookCount" role="status"></p><button class="btn out small" type="button" id="logbookDiagnosticExport" data-i18n="diag7.export">Download diagnostic report</button><button class="btn out small" type="button" id="logbookOlder" hidden data-i18n="rxlog.older">Load older events</button></div>
 <div id="logbookNotes"><p class="muted small" data-i18n="diag7.uptime_note">Time is shown as hh:mm:ss,mmm since this boot; browser times are approximate.</p>
 <p class="muted small" data-i18n="logbook.note">Web events are recorded in this browser session only. ESP32 timestamps share its uptime; browser timestamps are approximate. Sensitive API payloads are never logged.</p></div>
</div>

</section>

<section class="page" data-view="learn">
<h1 class="title" data-i18n="static.049">Add device</h1><div class="sub" id="learnIntro" data-i18n="static.050">Choose the frequency, press Start and use the device.</div>
<div class="card" id="radioSelectCard"><b data-i18n="static.055">Choose a frequency</b><div class="choices" id="learnRadios"><button class="choice" disabled data-i18n="static.017">Loading…</button></div><p id="learnRadioHelp" class="muted small" data-i18n="static.056">Checking radio status…</p></div>
<div id="learnProtocolGuide" class="notice" hidden></div><div class="card wizard" id="deviceWizard"><h2 id="wizardTitle" data-i18n="static.057">Add device</h2><p id="wizardHelp" data-i18n="static.058">Press Start, then use the device once or a few times.</p><div class="actions"><button class="btn green" id="wizardStart" type="button" disabled data-i18n="static.059">Start</button><button class="btn out" id="wizardCancel" type="button" hidden data-i18n="static.060">Stop</button></div><div id="wizardResult" hidden><label for="wizardName" data-i18n="static.061">Device name</label><input class="field" id="wizardName" placeholder="e.g. Living room remote" data-i18n-placeholder="placeholder.wizard_device_name" maxlength="80"><div class="profile-picker" id="wizardProfileWrap"><label for="wizardProfile" data-i18n="profileflow.choose">Apparaatprofiel</label><select class="field" id="wizardProfile"></select><p class="muted small" id="wizardProfileAdvice"></p><div class="profile-preview" id="wizardProfilePreview"></div></div><div id="wizardLearnAdvice" class="notice" hidden></div><div id="wizardFeatures" hidden><b data-i18n="static.062">Functions</b><p class="muted small" data-i18n="static.063">Choose the functions you want to use.</p><div id="wizardFeatureOptions" class="review-options"></div></div><label class="row" id="wizardHaWrap" hidden><span data-i18n="static.064">Add to Home Assistant</span><input type="checkbox" id="wizardHa" checked></label><details class="wizard-tech" id="wizardTechnicalDetails"><summary data-i18n="ui.advanced_details">Advanced details</summary><p id="wizardProtocol" class="muted small"></p><p class="muted small" data-i18n="static.065">Technical radio details are stored automatically.</p></details><div class="actions wizard-result-actions"><button class="btn green" id="wizardConfirm" type="button" data-i18n="static.066">Save</button><button class="btn out" id="wizardRetry" type="button" data-i18n="ui.try_again">Try again</button><button class="btn out" id="wizardLink" type="button" hidden data-i18n="static.067">Link to existing device</button></div></div><div id="wizardDone" class="notice" hidden></div></div>
<details class="learn-advanced-entry" id="learnAdvancedMode"><summary><span data-i18n="ui.advanced">Advanced</span></summary><p class="muted small" data-i18n="static.054">Only needed when RFLink does not know the protocol yet.</p><button class="modebtn" data-learn="decoder" type="button"><b data-i18n="static.053">Create decoder</b><br><span class="muted small" data-i18n="ui.decoder_advanced_help">Record and compare raw radio data.</span></button></details>
<div id="expertLearn" hidden><div class="actions"><button class="btn out" type="button" data-learn="device" data-i18n="ui.back_add_device">← Back to Add device</button></div><div class="steps"><span class="step done"></span><span class="step" id="step2"></span><span class="step" id="step3"></span><span class="step" id="step4"></span></div>
<div class="card">
<b data-i18n="static.068">Technical measurement</b>
<div class="notice" id="learnInstruction">Keep the device about 1 metre from the correct antenna. Do not press a button yet.</div>
<div class="actions"><button class="btn soft" id="baselineBtn" data-i18n="static.070">Measure background (5 sec)</button><button class="btn green" id="listenBtn" data-i18n="static.071">Listen for device</button><button class="btn out" id="stopListenBtn" data-i18n="static.072">Stop and clear measurement</button></div>
<div class="rows"><div class="row"><span data-i18n="static.073">Background</span><b id="baselineState" data-i18n="static.074">Not measured yet</b></div><div class="row"><span data-i18n="static.075">Repeats</span><b id="learnRepeats">—</b></div><div class="row"><span data-i18n="static.077">Protocol</span><b id="learnProtocol">—</b></div><div class="row"><span data-i18n="static.078">Identity</span><b class="mono" id="learnIdentity">—</b></div></div>
<div style="margin-top:15px"><b data-i18n="static.079">Signal</b><div class="meter"><span id="signalBar"></span></div><div class="split"><span id="signalLabel" class="muted" data-i18n="static.080">Waiting for capture</span><b id="signalRssi">—</b></div></div>
<div id="deviceLearnHelp" class="notice"><span data-i18n="static.081">Once RFLink has reliably detected the device several times, it appears under</span> <b data-i18n="static.082">Devices → New signals</b><span data-i18n="static.083">. There you can name it or link it to an existing device.</span></div>
<div id="decoderTools" hidden>
<hr style="border:0;border-top:1px solid var(--line);margin:18px 0"><b data-i18n="user.heading">My own protocol</b><p class="muted small" data-i18n="user.guide">Record each button at least three times. Fixed codes can be tested, saved and activated here. For rolling codes, describe the changing fields under Advanced.</p>
<div class="user-basics-grid simple-basics"><label for="userDecoderName"><span data-i18n="user.name">Decoder name</span><input id="userDecoderName" class="field" maxlength="64" placeholder="Living room remote" data-i18n-placeholder="placeholder.decoder_name"></label><label for="userAction"><span data-i18n="user.action">Button/action</span><input id="userAction" class="field" maxlength="32" value="on" placeholder="on"></label></div>
<div class="actions"><button class="btn green" type="button" id="userCapture" data-i18n="user.capture">Record this action</button><button class="btn soft" type="button" id="userClear" data-i18n="user.clear">Clear recordings</button><button class="btn out" type="button" id="userCreate" data-i18n="user.create">Test and save decoder</button></div><p class="small muted" id="userCaptureCount" aria-live="polite"></p><p class="small" id="userResult" role="status"></p>
<details id="userAdvanced" class="advanced-panel"><summary data-i18n="user.advanced">Advanced: edit RX and TX rules</summary><p class="small muted" data-i18n="user.advanced_help">RX rules recognize signals. TX rules produce a signal preview. Saving is transactional: a working active decoder stays active if a changed rule fails validation.</p><label for="userDecoderId" style="display:grid;gap:6px;max-width:420px"><span data-i18n="user.id">Stable ID</span><input id="userDecoderId" class="field mono" maxlength="32" placeholder="user_living_room"></label><p class="small muted" data-i18n="user.id_auto_help">Leave this empty when creating a decoder; RFLink will make a stable ID from the name. Change it only when you know why you need a specific ID.</p><div class="notice small" id="userAnalysis" data-i18n="user.analysis_waiting">Record examples to compare stable and changing RF fields.</div><p class="small muted"><span data-i18n="user.radio_profile">Radio profile</span>: <b class="mono" id="userRadioProfile">—</b> · <span data-i18n="user.radio_profile_help">This release uses the board's validated radio setup; custom CC1101 modulation/bitrate is not enabled from user recipes yet.</span></p><label for="userActions" data-i18n="user.actions">Actions, separated by commas</label><input class="field mono" id="userActions" placeholder="on,off"><div class="rule-grid"><div><label for="userRxSource">RX</label><textarea class="codebox" id="userRxSource" spellcheck="false"></textarea></div><div><label for="userTxSource">TX</label><textarea class="codebox" id="userTxSource" spellcheck="false"></textarea></div></div><div class="actions"><button class="btn out" type="button" id="userSaveAdvanced" data-i18n="user.save_rules">Save rules</button></div><p class="small muted" data-i18n="user.vm_help">Rules run in a bounded instruction interpreter, without native code. Consult USER_DECODER_GUIDE.md for syntax.</p></details>
<details style="margin-top:13px"><summary data-i18n="user.firmware">Convert to firmware decoder</summary><p class="small muted" data-i18n="user.firmware_help">Export your library and run the bundled evaluation and merge tool on your computer. Review the report before building firmware.</p><div class="actions"><button class="btn out" type="button" id="userExport" data-i18n="user.export">Download evaluation export</button><button class="btn out" type="button" id="userImport" data-i18n="user.import">Restore library backup</button><input id="userImportFile" type="file" accept="application/json,.json" hidden></div><p class="small muted" data-i18n="user.import_help">Restore validates every decoder before replacing it. TX permission is never restored automatically; protocol counters only move forward.</p></details>
<details style="margin-top:13px"><summary data-i18n="static.090">View code suggestion</summary><textarea id="snippet" class="codebox" readonly aria-label="Code suggestion" data-i18n-aria-label="static.090">Make a capture first.</textarea></details>
</div>
</div>
</div>
</section>

<section class="page" data-view="settings">
<h1 class="title" data-i18n="static.092">Settings &amp; help</h1><div class="sub" data-i18n="settings.dashboard_hint">Common settings are ready to use. Open a tool below only when needed.</div>
<div class="settings">
<div class="settings-primary settings-dashboard" id="settingsDashboard">
<div class="card settings-card settings-general" id="settingsGeneralCard">
  <h2 class="settings-card-title" data-i18n="settings.general">General &amp; status light</h2>
  <div class="settings-general-groups">
    <div class="settings-subgroup">
      <label class="settings-group-title" for="uiLanguage" data-i18n="language.title">Language &amp; region</label>
      <select class="field" id="uiLanguage" aria-label="Interface language" data-i18n-aria-label="language.label">
        <option value="en">English</option><option value="nl">Nederlands</option><option value="fr">Français</option><option value="de">Deutsch</option>
      </select>
      <p id="languageSaveStatus" class="settings-inline-feedback muted small" role="status" aria-live="polite"></p>
    </div>
    <div class="settings-subgroup settings-subgroup-led">
      <b class="settings-group-title" data-i18n="static.015">RGB status light</b>
      <div class="settings-tight-row"><label for="ledBrightness" data-i18n="static.095">Brightness</label><b id="ledBrightnessValue">20%</b></div>
      <input id="ledBrightness" type="range" min="1" max="100" value="20" aria-label="RGB brightness" data-i18n-aria-label="accessibility.led_brightness">
      <label class="settings-tight-row settings-check" for="ledHeartbeat"><span data-i18n="static.097">Green heartbeat when ready</span><input id="ledHeartbeat" type="checkbox"></label>
      <div class="settings-tight-row settings-status"><span data-i18n="static.099">Current status</span><b id="ledStatusSettings">—</b></div>
      <div class="settings-footer"><button class="btn green" id="ledSave" type="button" data-i18n="static.100">Save LED settings</button>
      <details class="settings-inline-more"><summary data-i18n="settings.led_more">LED colours &amp; behaviour</summary>
        <p class="muted small" data-i18n="static.094">WS2812 on GPIO48. Warning colours always remain active.</p>
        <p class="muted small" data-i18n="static.098">Ready heartbeat is optional.</p>
        <p class="muted small" data-i18n="static.101">Settings are stored on the ESP32 and survive restarts.</p>
      </details></div>
    </div>
  </div>
</div>
<div class="card settings-card settings-logging" id="rxLogSettingsCard">
  <h2 class="settings-card-title" data-i18n="rxlog.settings">Logging settings</h2>
  <p class="muted settings-short-help" data-i18n="rxlog.settings_help">Limits apply immediately and persist after reboot. Noise is removed first.</p>
  <section class="settings-log-group" aria-labelledby="logSectionRadio"><h3 id="logSectionRadio" data-i18n="logs.group_radio">Radio's</h3><p class="muted small" data-i18n="logs.radio_help">Ontvangen signalen en gedeelde ruis van alle drie de radio's.</p>
  <div class="settings-log-fields">
    <label class="row" for="rxLogLimit0"><span data-i18n="rxlog.radio0">433.42 MHz signals</span><input class="field" type="number" id="rxLogLimit0" min="0" max="4096" value="250"></label>
    <label class="row" for="rxLogLimit1"><span data-i18n="rxlog.radio1">433.92 MHz signals</span><input class="field" type="number" id="rxLogLimit1" min="0" max="4096" value="250"></label>
    <label class="row" for="rxLogLimit2"><span data-i18n="rxlog.radio2">868.30 MHz signals</span><input class="field" type="number" id="rxLogLimit2" min="0" max="4096" value="250"></label>
    <label class="row" for="rxLogNoiseLimit"><span data-i18n="rxlog.noise_limit">Shared noise limit</span><input class="field" type="number" id="rxLogNoiseLimit" min="0" max="4096" value="2000"></label>
  </div>
  </section><section class="settings-log-group" aria-labelledby="logSectionSystem"><h3 id="logSectionSystem" data-i18n="logs.group_system">Systeem</h3><p class="muted small" data-i18n="logs.system_help">Beslissingen van de ESP32 over apparaten, decoders, koppelen en zenden.</p>
  <div class="settings-log-fields">
    <label class="row" for="systemLogLimit"><span data-i18n="logbook.system_limit">System log events (ESP32)</span><input class="field" type="number" id="systemLogLimit" min="0" max="256" value="128"></label>
  </div></section><section class="settings-log-group" aria-labelledby="logSectionWeb"><h3 id="logSectionWeb" data-i18n="logs.group_web">Web</h3><p class="muted small" data-i18n="logs.web_help">Navigatie, klikken en API-fouten in deze browsersessie.</p>
  <div class="settings-log-fields"><label class="row" for="webLogLimit"><span data-i18n="logbook.web_limit">Web log events (this browser)</span><input class="field" type="number" id="webLogLimit" min="0" max="2000" value="300"></label></div>
  <label class="row" style="margin-top:10px;gap:7px"><input type="checkbox" id="webLogVerbose"><span class="small" data-i18n="logbook.verbose">Web debug: include successful automatic API refreshes</span></label>
  <p class="muted small" data-i18n="logbook.settings_note">System log size applies until restart; browser preferences are saved locally. Radio limits remain stored on the ESP32.</p></section>
  <p class="muted settings-inline-stat" id="rxLogMemory"></p>
  <div class="settings-footer"><button class="btn green" id="rxLogSave" type="button" data-i18n="rxlog.save">Save logging settings</button><span class="muted small" id="rxLogSettingStatus" role="status" aria-live="polite"></span></div>
</div>
<div class="card settings-card settings-assistant">
  <h2 class="settings-card-title" data-i18n="settings.integrations">Home Assistant &amp; system</h2>
  <div class="settings-subgroup">
    <b class="settings-group-title" data-i18n="static.106">Home Assistant (MQTT Discovery)</b>
    <div class="settings-tight-row"><span data-i18n="static.108">MQTT broker</span><b id="haMqttStatus">Checking…</b></div>
    <div class="settings-footer"><button class="btn out" data-go="devices" data-focus-devices="ha" data-i18n="static.111">Manage HA devices</button>
      <details class="settings-inline-more"><summary data-i18n="settings.mqtt_setup">MQTT setup</summary>
        <p class="muted small" data-i18n="static.107">Add RF devices individually under Devices. The ESP32 itself remains connected via the ESPHome API.</p>
        <div class="mqtt-runtime-form" id="mqttRuntimeWrap"><p class="muted small" id="mqttRuntimeInfo" data-i18n="nav4.mqtt_not_built">MQTT is not compiled in. Enable the optional package once.</p>
          <div id="mqttRuntimeFields" hidden><label class="small" for="mqttHost" data-i18n="nav4.mqtt_host">Broker address</label><input class="field" id="mqttHost" maxlength="127" autocomplete="off" placeholder="192.168.178.10" spellcheck="false">
          <div class="device-find-row"><div><label class="small" for="mqttPort" data-i18n="nav4.mqtt_port">Port</label><input type="number" class="field" id="mqttPort" min="1" max="65535" value="1883"></div><div><label class="small" for="mqttUser" data-i18n="nav4.mqtt_user">Username</label><input class="field" id="mqttUser" maxlength="63" autocomplete="off"></div></div>
          <label class="small" for="mqttPassword" data-i18n="nav4.mqtt_pass">Password (leave empty to keep)</label><input class="field" id="mqttPassword" type="password" maxlength="127" autocomplete="new-password">
          <label class="row"><span data-i18n="nav4.mqtt_enable">Enable MQTT</span><input id="mqttEnabled" type="checkbox"></label>
          <p class="muted small" data-i18n="nav4.mqtt_security">Stored on the device; use a trusted network.</p><button class="btn green" type="button" id="mqttSave" data-i18n="nav4.mqtt_save">Save &amp; connect</button><span class="muted small" id="mqttSaveState" role="status"></span></div></div>
      </details>
    </div>
  </div>
  <div class="settings-subgroup settings-system-shortcut" id="firmwareAboutCard">
    <b class="settings-group-title" data-i18n="about.heading">About RFLink Universal</b>
    <div id="firmwareVersion" class="version-main">—</div>
    <p class="muted small" id="firmwareSubtitle" data-i18n="version.subtitle">Bediening, logboeken en experimentele TX in ontwikkeling.</p>
    <details class="settings-inline-more"><summary data-i18n="version.notes">Wat is nieuw en technische details?</summary>
      <b data-i18n="version.changes">Veranderingen in deze versie</b><ul class="small" id="firmwareNotes"></ul>
      <div class="settings-tight-row"><span data-i18n="version.raw">Firmwareversie</span><b id="firmwareRawVersion" class="mono small">—</b></div>
      <div class="settings-tight-row"><span data-i18n="version.source_hash">Bronhash (SHA-256, verkort)</span><b id="firmwareRevision" class="mono small">—</b></div>
      <div class="settings-tight-row"><span data-i18n="about.built">Compiled</span><b id="firmwareBuilt" class="mono small">—</b></div>
    </details>
  </div>
</div>
</div>
<div class="settings-pickers" role="group" aria-label="More RFLink settings" data-i18n-aria-label="settings.additional">
  <span class="settings-pickers-label" data-i18n="settings.additional">More settings &amp; tools</span>
  <button type="button" class="settings-picker" data-settings-open="library" aria-controls="settingsPanelLibrary" aria-expanded="false" data-i18n="library.title">Library &amp; updates</button>
  <button type="button" class="settings-picker" data-settings-open="decoders" aria-controls="settingsPanelDecoders" aria-expanded="false" data-i18n="static.112">Decoder library</button>
  <button type="button" class="settings-picker" data-settings-open="custom" aria-controls="settingsPanelCustom" aria-expanded="false" data-i18n="user.saved">Custom protocols &amp; device profiles</button>
</div>
<div class="settings-detail-panel" id="settingsPanelLibrary" hidden>
<div class="settings-detail-toolbar"><button type="button" class="btn out small settings-back" data-settings-back data-i18n="nav.back_settings">← Back to settings overview</button></div>
<div class="card settings-wide" id="libraryCard"><div class="row"><div><h2 class="sectiontitle" style="margin:0 0 5px" data-i18n="library.title">Library &amp; updates</h2><p id="libraryStatus" class="muted small" style="margin:3px 0 0"></p></div><button type="button" class="help-btn" data-help="library" aria-label="Help">?</button></div><div class="actions"><button class="btn out small" type="button" id="libraryReview" hidden data-i18n="library.review">Review changes</button><button class="btn out small" type="button" id="libraryAdvanced" data-i18n="version.technical">Technische details</button></div><div class="advanced-panel" id="libraryAdvancedPanel"><p class="muted small" data-i18n="library.help">Official firmware items and your own items stay separate. RFLink never replaces your changes without confirmation.</p><div class="library-summary"><div class="library-stat"><span data-i18n="library.official_decoders">Official decoders</span><b id="libOfficialDecoders">—</b></div><div class="library-stat"><span data-i18n="library.official_profiles">Official device profiles</span><b id="libOfficialProfiles">—</b></div><div class="library-stat" id="libUserStat" hidden><span data-i18n="library.my_decoders">My decoders</span><b id="libUserDecoders">—</b></div><div class="library-stat" id="libProfileStat" hidden><span data-i18n="library.my_profiles">My device profiles</span><b id="libUserProfiles">—</b></div><div class="library-stat" id="libAttentionStat" hidden><span data-i18n="library.needs_attention">Needs attention</span><b id="libAttention">—</b></div></div><div id="libraryTechnicalPanel" hidden><p class="muted small" data-i18n="library.advanced_help">Technical library information for debugging and development.</p><div class="advanced-kv"><span data-i18n="library.schema">Library schema</span><b id="libSchema">—</b><span data-i18n="library.user_storage">Persistent user library</span><b id="libPersistent">—</b><span data-i18n="library.safe_resolutions">Safe automatic resolutions</span><b id="libSafe">—</b><span data-i18n="library.archive">Archived local items</span><b id="libArchive">—</b></div><div id="libraryAdvancedItems" class="mono muted small" style="margin-top:10px;overflow-wrap:anywhere"></div></div></div></div>
</div>
<div class="settings-detail-panel" id="settingsPanelDecoders" hidden>
<div class="settings-detail-toolbar"><button type="button" class="btn out small settings-back" data-settings-back data-i18n="nav.back_settings">← Back to settings overview</button></div>
<div class="card decoder-library-card settings-wide" id="decoderLibraryCard"><div class="settings-tool-action"><button type="button" class="btn out small" data-launch-decoder data-i18n="static.053">Create decoder</button></div><h2 class="sectiontitle" style="margin:0 0 5px" data-i18n="static.112">Decoder library</h2><p class="muted small" data-i18n="static.113">What can RFLink recognize, what reaches Home Assistant and what can you control? Software tests are not radio tests.</p><div class="catalog-modes" role="group" aria-label="Library view"><button type="button" class="decoder-filter-btn active" data-catalog-mode="protocol" id="catalogProtocols" data-i18n="catalog.protocols">Protocols</button><button type="button" class="decoder-filter-btn" data-catalog-mode="profile" id="catalogProfiles" data-i18n="catalog.profiles">Device profiles</button></div>
<div class="profile-browser-tools" id="profileBrowserTools" hidden><input type="search" id="profileCatalogSearch" class="field" data-i18n-placeholder="catalog.search_placeholder" placeholder="Search brand, model or protocol…" autocomplete="off"><select class="field" id="profileCatalogBrand" aria-label="Brand"></select><select class="field" id="profileCatalogFamily" aria-label="Family"></select><select class="field" id="profileCatalogProtocol" aria-label="Protocol"></select></div>
<div class="decoder-filters" id="decoderFilters"><button type="button" class="decoder-filter-btn active" data-decoder-filter="all" data-i18n="static.115">All protocols</button><button type="button" class="decoder-filter-btn" data-decoder-filter="control" data-i18n="static.116">Remotes and controls</button><button type="button" class="decoder-filter-btn" data-decoder-filter="sensor" data-i18n="static.117">Sensors</button><button type="button" class="decoder-filter-btn" data-decoder-filter="development" data-i18n="static.118">Development examples</button></div><p id="decoderCount" class="muted small"></p><div class="decoder-browser"><div class="decoder-selector" id="decoderSelector"><span class="muted" data-i18n="static.119">Loading library…</span></div><div class="decoder-detail" id="decoderDetail"><p class="muted" data-i18n="decoder.select_hint">Select a decoder to view all information.</p></div></div></div>
</div>
<div class="settings-detail-panel" id="settingsPanelCustom" hidden>
<div class="settings-detail-toolbar"><button type="button" class="btn out small settings-back" data-settings-back data-i18n="nav.back_settings">← Back to settings overview</button></div>
<div class="card settings-wide advanced-section" id="userLibraryCard"><details id="userLibraryDetails"><summary class="advanced-summary"><span data-i18n="user.saved">Custom protocols &amp; device profiles</span><span class="advanced-badge" data-i18n="library.advanced">Advanced</span></summary><p class="muted small" data-i18n="user.saved_help">Only use this when RFLink does not already know your device or when you want a custom model profile.</p><div class="actions settings-custom-actions"><button type="button" class="btn out small" data-launch-decoder data-i18n="static.053">Create decoder</button></div><div class="user-library-layout"><div class="user-library-pane"><div id="userDecoderList" class="list"></div></div><div class="user-profile-pane"><h3 data-i18n="user.profile">Device profile</h3><p class="muted small" data-i18n="user.profile_help">Give a decoder a friendly model name and instructions. Select that profile on a saved device under Devices.</p><div class="profile-form-grid"><label for="userProfileName"><span data-i18n="user.profile_name">Profile name</span><input class="field" id="userProfileName" maxlength="64" placeholder="Living room remote" data-i18n-placeholder="placeholder.profile_name"></label><label for="userProfileDecoder"><span data-i18n="user.decoder">Decoder</span><select class="field" id="userProfileDecoder"></select></label><label class="full" for="userProfileHelp"><span data-i18n="user.instructions">Learning instructions</span><input class="field" id="userProfileHelp" maxlength="255" placeholder="Press each button three times" data-i18n-placeholder="placeholder.profile_help"></label></div><details class="decoder-tech"><summary data-i18n="user.profile_advanced">Advanced profile options</summary><div class="profile-form-grid" style="margin-top:10px"><label for="userProfileId"><span data-i18n="user.profile_id">Profile ID</span><input class="field mono" id="userProfileId" maxlength="32" placeholder="living_room_remote"></label><label for="userProfileActions"><span data-i18n="user.actions">Actions, separated by commas</span><input class="field mono" id="userProfileActions" placeholder="on,off"></label><label for="userProfileControls"><span>Bedieningsknoppen (actie-ID's, komma's)</span><input class="field mono" id="userProfileControls" placeholder="on,off"></label><label for="userProfilePair"><span>Koppelmethode</span><select class="field" id="userProfilePair"><option value="none">Geen koppeling (alleen ontvangen)</option><option value="manual">Handmatige instructies</option><option value="receiver_learn">Ontvanger leert nieuwe code</option><option value="unsupported">Nog niet ondersteund</option></select></label><label class="full" for="userProfileSteps"><span>Wizardstappen (één korte instructie per regel)</span><textarea class="field" id="userProfileSteps" rows="4" maxlength="1900" placeholder="Druk op de knop ...
Controleer de reactie ..."></textarea></label></div><p class="muted small" data-i18n="user.profile_auto_help">Leave the ID empty to create it automatically from the profile name. Actions are prefilled from the selected decoder when possible.</p></details><div class="actions"><button class="btn green" id="userSaveProfile" type="button" data-i18n="user.save_profile">Save profile</button></div><div id="userProfileList" class="list"></div></div></div></details></div>
</div>


</div>
</section>
</main>
</div>
<nav class="bottom"><button data-page="status" class="active" data-i18n="static.009">Status</button><button data-page="devices" data-i18n="static.031">Devices</button><button data-page="monitor" data-i18n="logbook.title_nav">Logs</button><button data-page="learn" data-i18n="static.049">Learn</button><button data-page="settings" data-i18n="static.136">Settings</button></nav>
<div class="modal-backdrop" id="helpModal" hidden><div class="modal-card" role="dialog" aria-modal="true" aria-labelledby="helpModalTitle"><div class="modal-head"><h2 id="helpModalTitle"></h2><button type="button" class="modal-close" id="helpModalClose" aria-label="Close" data-i18n-aria-label="accessibility.close">×</button></div><p id="helpModalText"></p><p id="helpModalMore" class="muted small"></p></div></div>
<div class="toast" id="toast"></div>
<div class="modalback" id="nameModal"><div class="modal"><h3 data-i18n="static.057">Add device</h3><p class="muted" data-i18n="static.137">Give it a recognizable name. The RF identity is stored internally.</p><div id="modalLearnAdvice" class="notice" hidden></div><label for="deviceName" data-i18n="static.061">Device name</label><input class="field" id="deviceName" placeholder="e.g. Garden temperature sensor" data-i18n-placeholder="placeholder.device_name"><div class="profile-picker" id="modalProfileWrap"><label for="modalProfile" data-i18n="profileflow.choose">Apparaatprofiel</label><select class="field" id="modalProfile"></select><p class="muted small" id="modalProfileAdvice"></p><div class="profile-preview" id="modalProfilePreview"></div></div><div id="modalFeatures" hidden><b data-i18n="static.138">Review features</b><div id="modalFeatureOptions" class="review-options"></div></div><label class="row" id="modalHaWrap" hidden><span data-i18n="static.064">Add to Home Assistant</span><input type="checkbox" id="modalHa" checked></label><div class="actions"><button class="btn soft" id="modalCancel" data-i18n="static.139">Cancel</button><button class="btn green" id="modalSave" data-i18n="static.140">Save</button></div></div></div>
<script>
// No external dependencies: standards-compliant ZIP (stored/no compression) + export helpers.
(function (root, factory) {
  const api = factory();
  if (typeof module !== 'undefined' && module.exports) module.exports = api;
  root.RFLogTools = api;
}(typeof globalThis !== 'undefined' ? globalThis : this, function () {
  const encoder = new TextEncoder();
  const table = new Uint32Array(256);
  for (let n = 0; n < 256; n++) {
    let c=n;
    for (let k=0;k<8;k++) c=(c&1)?(0xedb88320^(c>>>1)):(c>>>1);
    table[n]=c>>>0;
  }
  function crc32(data) {
    let c = 0xffffffff;
    for (const b of data) c = table[(c ^ b)&255] ^ (c >>> 8);
    return (c ^ 0xffffffff)>>>0;
  }
  function u16(b, off, n) { b[off]=n&255; b[off+1]=(n>>>8)&255; }
  function u32(b, off, n) { u16(b,off,n&65535); u16(b,off+2,Math.floor(n/65536)); }
  function dosTimestamp(date) {
    const y=Math.max(1980,date.getFullYear());
    return {t:(date.getHours()<<11)|(date.getMinutes()<<5)|Math.floor(date.getSeconds()/2),
      d:((y-1980)<<9)|((date.getMonth()+1)<<5)|date.getDate()};
  }
  function zipStore(files, date=new Date()) {
    const output=[]; const central=[]; let offset=0; const dos=dosTimestamp(date);
    for (const [name, contents] of Object.entries(files)) {
      const filename=encoder.encode(name);
      const data=contents instanceof Uint8Array ? contents : encoder.encode(String(contents));
      const crc=crc32(data);
      if (offset+30+filename.length+data.length>=0xffffffff) throw new Error('ZIP size limit');
      const local=new Uint8Array(30+filename.length);
      u32(local,0,0x04034b50);u16(local,4,20);u16(local,6,0x800);u16(local,8,0);
      u16(local,10,dos.t);u16(local,12,dos.d);u32(local,14,crc);
      u32(local,18,data.length);u32(local,22,data.length);u16(local,26,filename.length);
      local.set(filename,30);
      output.push(local,data);
      const header=new Uint8Array(46+filename.length);
      u32(header,0,0x02014b50);u16(header,4,20);u16(header,6,20);
      u16(header,8,0x800);u16(header,10,0);u16(header,12,dos.t);u16(header,14,dos.d);
      u32(header,16,crc);u32(header,20,data.length);u32(header,24,data.length);
      u16(header,28,filename.length);u32(header,42,offset);header.set(filename,46);
      central.push(header);
      offset+=local.length+data.length;
    }
    const centralSize=central.reduce((a,c)=>a+c.length,0);
    const end=new Uint8Array(22);
    u32(end,0,0x06054b50);u16(end,8,central.length);u16(end,10,central.length);
    u32(end,12,centralSize);u32(end,16,offset);
    const total=offset+centralSize+end.length;
    const all=new Uint8Array(total);let p=0;
    for(const chunk of [...output,...central,end]) {all.set(chunk,p);p+=chunk.length;}
    return all;
  }
  function csvEscape(s){return '"'+String(s??'').replace(/"/g,'""')+'"';}
  function csv(events) {
    const headings=['sequence','time_utc','uptime_ms','radio','category','protocol','raw','rssi_dbm','lqi','freqest_hz','reason','extra_json'];
    const lines=[headings.join(',')];
    for (const e of events) {
      const cells=[e.seq,e.utc||'',e.uptime_ms,e.radio,e.category,e.protocol||'',
        e.raw||'',e.rssi_dbm??'',e.lqi??'',e.freqest_hz??'',e.reason||'',JSON.stringify(e.extra||{})];
      lines.push(cells.map(csvEscape).join(','));
    }
    return '\ufeff'+lines.join('\r\n')+'\r\n';
  }
  function exportFiles(events, diagnostics={}) {
    const sorted=[...events].sort((a,b)=>a.seq-b.seq);
    const radioIDs=['433.42','433.92','868.30'];
    const files={};
    for(const r of radioIDs) {
      files[`${r}_signals.csv`]=csv(sorted.filter(e=>e.radio===r&&e.category==='signal'));
      files[`${r}_noise.csv`]=csv(sorted.filter(e=>e.radio===r&&e.category==='noise'));
    }
    files['all_events.csv']=csv(sorted);
    files['diagnostics.json']=JSON.stringify(diagnostics,null,2)+'\n';
    return files;
  }
  return { crc32, zipStore, csv, exportFiles };
}));

// Build-time generated locale dictionary; source files: ui/locales/{en,nl,fr,de}.json.
const RF_UI_LOCALES = {"en":{"static.000":"RFLink Universal","static.001":"● Online","static.002":"0 signals","static.003":"Uptime —","static.004":"◉ Status","static.005":"▣ Devices","static.006":"◷ Activity","static.007":"◇ Add device","static.008":"⚙ Settings","static.009":"Status","static.010":"Everything you need daily. Technical details are one level deeper.","static.011":"Checking system…","static.012":"Loading status.","static.013":"View diagnostics","static.014":"Loading radio status…","static.015":"RGB status light","static.016":"Current status","static.017":"Loading…","static.018":"LED","static.019":"WS2812 · GPIO48","static.020":"Quick actions","static.021":"+ Add device","static.022":"Find and save a remote, sensor or switch.","static.023":"Pairing","static.024":"View devices whose decoder supports pairing.","static.025":"Unknown protocol","static.026":"Developer Mode helps compare captures and suggests code.","static.027":"Troubleshoot","static.028":"Diagnostics, decoder library and technical details.","static.029":"Recent activity","static.030":"No activity yet.","static.031":"Devices","static.032":"RFLink creates a device only after it has observed a signal sufficiently often and you confirm it. You can view, unlink or safely replace RF codes per device.","static.033":"Only devices with supported setup instructions are shown; some may not support RF sending yet.","static.034":"Checking device storage…","static.035":"Loading devices…","static.036":"New signals","static.037":"Loading candidates…","static.038":"Activity","static.039":"Recent signals seen by RFLink.","static.040":"Filter by frequency","static.041":"Loading status…","static.042":"Direction","static.043":"RX and TX","static.044":"RX only","static.045":"TX only","static.046":"TX entries show commands handed to the radio driver, not confirmation from the target device. Raw radio details are intended for troubleshooting.","static.047":"Activity across all radios","static.048":"Loading activity…","static.049":"Add device","static.050":"Choose the frequency, press Start and use the device.","static.051":"Add device","static.052":"For everyday use","static.053":"Create decoder","static.054":"Only needed when RFLink does not know the protocol yet.","static.055":"Choose a frequency","static.056":"Checking radio status…","static.057":"Add device","static.058":"Press Start, then use the device once or a few times.","static.059":"Start","static.060":"Stop","static.061":"Device name","static.062":"Functions","static.063":"Choose the functions you want to use.","static.064":"Add to Home Assistant","static.065":"Technical radio details are stored automatically.","static.066":"Save","static.067":"Link to existing device","static.068":"Technical measurement","static.069":"Keep the device about 1 metre from the correct antenna. Do not press a button yet.","static.070":"Measure background (5 sec)","static.071":"Listen for device","static.072":"Stop and clear measurement","static.073":"Background","static.074":"Not measured yet","static.075":"Repeats","static.076":"—","static.077":"Protocol","static.078":"Identity","static.079":"Signal","static.080":"Waiting for capture","static.081":"Once RFLink has reliably detected the device several times, it appears under","static.082":"Devices → New signals","static.083":". There you can name it or link it to an existing device.","static.084":"Decoder assistant","static.085":"RFLink does not generate firmware code on the ESP32 itself. It provides a safe starting point you can paste into a new decoder folder.","static.086":"Create new folder","static.087":"python tools/new_decoder.py my_protocol \"My Protocol\"","static.088":"Then modify","static.089":"decoder.cpp + manifest.json only","static.090":"View code suggestion","static.091":"Make a capture first.","static.092":"Settings & help","static.093":"Clear guidance first; technical details only when needed.","static.094":"WS2812 on GPIO48. Blinking orange means one or more radios are missing or not ready. White means OTA, red means error; green only when fully ready. You can disable the green heartbeat without hiding warnings.","static.095":"Brightness","static.096":"20%","static.097":"Green heartbeat when ready","static.098":"Checked: green flashes when ready (two short flashes every 6 seconds). Unchecked: no green ready indicator. For the first test, set brightness to 100% and click Save.","static.099":"Current status","static.100":"Save LED settings","static.101":"Settings are stored on the ESP32 and survive restarts. The website is intended for trusted networks only.","static.102":"What do I want to do?","static.103":"New device","static.104":"Open Learn","static.105":"Pairing devices","static.106":"Home Assistant (MQTT Discovery)","static.107":"Add RF devices individually under Devices. The ESP32 itself remains connected via the ESPHome API.","static.108":"MQTT broker","static.109":"Checking…","static.110":"Optionally enable the MQTT package in YAML and add the broker credentials to secrets.yaml. Removing devices from Home Assistant requires a working broker connection.","static.111":"Manage HA devices","static.112":"Decoder library","static.113":"What can RFLink recognize, what reaches Home Assistant and what can you control? Software tests are not radio tests.","static.114":"Show","static.115":"All protocols","static.116":"Remotes and controls","static.117":"Sensors","static.118":"Development examples","static.119":"Loading library…","static.120":"Technical decoder registry","static.121":"Maintenance rule","static.122":"A decoder should have only one job:","static.123":"RFFrame → DecodeResult","static.124":"No web UI, Home Assistant, GPIO settings or logic from other decoders inside a protocol module.","static.125":"Diagnostics","static.126":"Loading radio and decoder status.","static.127":"Saved devices","static.128":"Decoder registry","static.129":"Storage","static.130":"Received","static.131":"Decoded","static.132":"Unknown","static.133":"Technical explanation","static.134":"A pulse is a period when the received RF signal is HIGH or LOW. One bit can contain several pulses. Developer Mode uses their timing to compare protocols.","static.135":"Monitor","static.136":"Settings","static.137":"Give it a recognizable name. The RF identity is stored internally.","static.138":"Review features","static.139":"Cancel","static.140":"Save","language.title":"Language & region","language.help":"Choose the interface language. Your preference is saved on the ESP32.","language.label":"Interface language","language.saving":"Saving language…","language.saved":"Language saved.","language.failed":"Could not save language on the ESP32. Previous language restored.","dyn.now":"now","dyn.sec":"{n} sec","dyn.min":"{n} min","dyn.hour":"{n} hr","dyn.dayhour":"{d}d {h}h","dyn.uptime":"Uptime {value}","dyn.frames":"{n} signals","action.on":"On","action.off":"Off","action.up":"Up","action.down":"Down","action.my":"Favourite (My)","action.prog":"Program","action.low":"Low","action.medium":"Medium","action.high":"High","action.standby":"Standby","action.status":"Status","action.measurement":"Measurement","action.rain_update":"Rain measurement","action.weather_update":"Weather measurement","action.code":"Fixed code","action.button":"Button {n}","action.command":"Doorbell code {n}","action.timer":"Timer {n} minutes","dyn.decoder_review":"Decoder {protocol}: review recognized features before saving.","dyn.decoder_missing":"Decoder information is missing. RFLink cannot safely review the features. Refresh the page and try again.","dyn.dev_instruction":"Keep the test device within about 1 metre. Measure the background first, then perform one physical action several times.","dyn.device_instruction":"Keep the device about 1 metre from the correct antenna. Do not press a button yet.","dyn.rx_ready":"● RX ready","dyn.rx_unavailable":"● RX unavailable","dyn.disconnected":"● Not connected","dyn.checking":"● Checking","dyn.rx_ready_short":"RX ready","dyn.rx_fault":"RX unavailable","dyn.not_connected":"Not connected","dyn.checking_short":"Checking","dyn.radio_unavailable_title":"Radio or receiver unavailable","dyn.no_radios":"No radios configured","dyn.only_working_receivers":"Only working receivers can be selected.","dyn.radio_checking":"Checking radio status…","dyn.no_working_receivers":"No working radio receivers: connect the modules and restart the ESP32.","dyn.all_radios":"All radios","dyn.overview":"Overview","dyn.radio_disconnected_title":"Radio module not connected","dyn.activity_radio":"Activity · {name}","dyn.activity_all":"Activity across all radios","dyn.unknown":"Unknown","dyn.no_monitor_activity":"No recorded activity for this filter. Missing radios do not produce new RX or TX records.","dyn.tx":"Transmit","dyn.available":"Available","dyn.unavailable":"Unavailable","dyn.last_signal":"Last signal","dyn.view_activity":"View activity","dyn.no_activity":"No activity yet.","dyn.offline":"● Offline","dyn.esp_unreachable":"ESP32 unreachable","dyn.esp_unreachable_help":"Check power, network and IP address. Device changes are not performed while offline.","dyn.api_unreachable":"API unreachable; starting is disabled.","dyn.connection_lost":"Connection to the ESP32 was lost.","dyn.manage_signals":"Manage signals ({n})","dyn.signal_identity_help":"The internal device ID {uid} remains unchanged. Replace an RF code only with a new code the receiver itself has recognized several times.","dyn.rf_signal":"RF signal {n}","dyn.replace_with":"Replace with","dyn.choose_learned":"Choose a newly learned signal","dyn.replace":"Replace","dyn.unlink":"Unlink","dyn.last_code_title":"Last code: delete the device or replace the code","dyn.no_matching_signal":"No matching new signal found. Go to Learn, receive the new signal at least three times, then return without saving it as a new device.","dyn.learn_new_signal":"Learn new signal","dyn.signal_history_help":"Recognized actions are stored at device level. For protocols where one RF identity represents several buttons, RFLink cannot yet reliably unlink a single button. Unlinking an RF code does not erase shared action history.","dyn.mqtt_remove_unavailable":"MQTT offline: removal unavailable","dyn.approve":"Approve","dyn.remove":"Remove","dyn.approve_technical":"Approve technical value","dyn.review_new_functions":"● Review {n} new feature(s)","dyn.view_functions":"View and edit features","dyn.approved_only_help":"Only features you approve are stored and published to Home Assistant. Value changes within existing features are handled automatically.","dyn.found_not_approved":"Found, not yet approved","dyn.no_new_visible":"No new visible features to review.","dyn.approved_features":"Approved features","dyn.none_approved":"No visible features approved yet.","dyn.technical_radio_data":"Technical radio data ({saved} saved{pending})","dyn.pending_suffix":", {n} to review","dyn.technical_radio_help":"These values may be required for an exact transmit code. Change them only when you know the protocol. RFLink does not expose them as separate Home Assistant sensors.","dyn.saved":"Saved","dyn.mqtt_connected":"Connected","dyn.mqtt_disconnected":"Not connected","dyn.mqtt_not_configured":"Not configured","dyn.storage_ok":"Saved devices survive a restart. Changing RF codes requires a confirmed new reception.","dyn.storage_uncertain":"Storage status is uncertain. Devices cannot be changed safely.","dyn.pairing_only":"Devices with guided setup steps","dyn.device_sub":"Manage name, RF codes and Home Assistant publication per device.","dyn.mqtt_no_features_title":"MQTT not connected or no approved features","dyn.remove_from_ha":"Remove from HA","dyn.add_to_ha":"Add to HA","dyn.receive_sensor":"Receive sensor","dyn.rf_device":"RF device","dyn.rf_signals":"{n} RF signal(s)","dyn.in_ha":"In Home Assistant","dyn.only_rflink":"RFLink only","dyn.ha_control_allowed":"HA control allowed","dyn.features_to_review":"{n} feature(s) to review","dyn.review_needed":"Review required","dyn.manage_more":"Manage more: name, Home Assistant and RF codes","dyn.changes_manual":"Changes are saved only after your action. RFLink remains authoritative.","dyn.mqtt_offline":"MQTT offline","dyn.disable_ha_control":"Disable HA control","dyn.allow_ha_control":"Allow HA control","dyn.rename":"Rename","dyn.dangerous_actions":"Dangerous actions","dyn.dangerous_help":"This removes the complete device from RFLink and, when linked, from Home Assistant.","dyn.delete_device":"Permanently delete device","dyn.technical_device_id":"Technical device ID","dyn.no_pair_devices":"No confirmed pairing devices yet.","dyn.no_devices":"No confirmed devices yet. Add one via Learn.","dyn.new_signal":"New signal","dyn.received_times":"Received {n} times","dyn.ready_review":"Ready for review","dyn.still_listening":"Still listening","dyn.check_signal":"Check the received signal before saving it.","dyn.looks_like":"Looks like {name}. Link only if this really is the same device.","dyn.review_add":"Review and add","dyn.more_technical":"More options and technical details","dyn.rf_identity":"RF identity: {id}","dyn.link_to":"Link to {name}","dyn.not_saved":"Not saved yet","dyn.pairing_view":"Pairing view active.","dyn.no_new_signals":"No new signals need attention.","dyn.learn_replacement_toast":"Learn a new signal; do not save it as a separate device.","dyn.devices_load_failed":"Could not load devices","dyn.confirm_unlink":"Unlink RF code {identity}? Other codes for the device will keep working.","dyn.unlinked":"RF code unlinked","dyn.unlink_failed":"Unlink failed; nothing was changed","dyn.choose_confirmed_signal":"Choose a confirmed new signal first","dyn.confirm_replace":"Replace RF code?\nOld: {old}\nNew: {new}\nThe device name and other RF codes will be preserved.","dyn.replaced":"RF code replaced and saved","dyn.replace_failed":"Replacement failed; old RF code preserved","dyn.storage_attention":"Storage needs attention","dyn.decoder_incomplete":"Decoder library incomplete","dyn.hardware_incomplete":"Hardware incomplete","dyn.all_ready":"Everything ready","dyn.storage_problem_help":"The connection works, but device changes cannot be stored safely. Check the ESPHome logs.","dyn.decoder_problem_help":"Not all expected decoders are loaded. Received signals may not be recognized.","dyn.hardware_problem_help":"ESP32 and website are working. {missing} of {total} radios are not connected or not ready.","dyn.ready_help":"ESP32, decoder library and all configured radios are ready.","dyn.esp_web":"ESP32 and web interface","dyn.radios":"Radios","dyn.ready_count":"{ready}/{total} ready","dyn.decoders":"Decoders","dyn.loaded":"{value} loaded","dyn.device_storage":"Device storage","dyn.check_required":"Check required","dyn.saved_count":"{n} saved","dyn.storage_error":"Storage error","dyn.confirm_remove_feature":"Remove feature \"{key}\" from RFLink and, if linked, from Home Assistant as well?","dyn.feature_removed":"Feature removed and saved","dyn.feature_approved":"Feature approved and saved","dyn.feature_failed":"Feature was not changed. Check MQTT and device storage.","dyn.confirm_remove_ha":"Remove \"{name}\" from Home Assistant? The RF device remains stored in RFLink.","dyn.ha_removed":"HA discovery removed; device remains stored","dyn.ha_sent":"HA discovery sent to broker","dyn.ha_failed":"HA change failed: check MQTT and storage","dyn.confirm_disable_tx":"Disable control of \"{name}\" in Home Assistant?","dyn.confirm_enable_tx":"Allow RF transmission for \"{name}\"? Only supported and approved buttons from this decoder appear in HA. The actual receiver state is not confirmed.","dyn.tx_saved":"Transmit permission saved","dyn.tx_failed":"Transmit permission unchanged; check MQTT and TX configuration","dyn.new_device_name":"New device name:","dyn.name_length":"Name must contain 1–80 characters","dyn.name_saved":"Name saved","dyn.rename_failed":"Rename was not saved","dyn.confirm_delete":"Delete device \"{name}\" from RFLink and, when active, from Home Assistant too? This cannot be undone; learning is required again.","dyn.device_removed":"Device removed","dyn.delete_failed":"Delete failed","dyn.add_device":"Add device","dyn.wizard_start_help":"Choose a radio and click Start. Background measurement and listening happen automatically.","dyn.storage_unavailable":"Device storage unavailable","dyn.receiver_unavailable":"This receiver is unavailable","dyn.step1":"Step 1/3 · Measure background","dyn.step1_help":"Wait five seconds. Do not press your device yet.","dyn.step2":"Step 2/3 · Operate your device","dyn.step2_help":"Press the same button several times. Once the signal is recognized, you can enter the name here.","dyn.measure_failed":"Measurement failed","dyn.measure_failed_help":"Check whether the radio is available and try again.","dyn.start_failed":"Could not start","dyn.start_failed_help":"Check the radio and try again.","dyn.step3":"Step 3/3 · Review and save","dyn.candidate_received":"{protocol} was received {n} times. Name the device and review the detected features.","dyn.observations":"{n} observations","dyn.link_to_name":"Link to {name}","dyn.temp_connection":"Connection temporarily unavailable","dyn.temp_connection_help":"Try again when ESPHome is reachable.","dyn.decoder_safe_disabled":"Decoder information is missing; safe saving is disabled","dyn.enter_name":"Enter a name of 1–80 characters","dyn.confirm_no_features":"You approved no features. The device will be stored only in RFLink. Continue?","dyn.signal_linked":"Signal linked. New features will be shown as proposals on the existing device; choices on this screen were not applied to it.","dyn.saved_ha_failed":"Device saved, but HA publication failed. You can enable it later under Devices.","dyn.device_saved":"Device saved. It remains stored after a restart.","dyn.done":"Done","dyn.done_help":"You can add another device or view your devices.","dyn.save_failed":"Save failed; try again","dyn.confirm_link":"Link this signal to {name}?","dyn.modal_invalid":"Check decoder information and enter a device name of 1–80 characters","dyn.confirm_store_only":"No features approved. Store only the device?","dyn.device_added":"Device added","dyn.add_failed":"Could not add device","dyn.linked_existing":"Linked to existing device","dyn.link_failed":"Link failed","dyn.radio_status_unreachable":"Radio status unavailable.","dyn.measuring_background":"Measuring background…","dyn.waiting_capture":"Waiting for capture","dyn.capture_first":"Make a capture first.","dyn.protocol_unknown":"not yet known","dyn.identity_unknown":"not yet determined","dyn.no_code_suggestion":"No code suggestion yet.","dyn.signal_strong":"Strong / possibly too close","dyn.good":"GOOD","dyn.weak":"Weak","dyn.raw_rssi":"raw: RSSI unavailable","dyn.capture_found":"Capture found","dyn.measuring":"Measuring…","dyn.ready":"Ready","dyn.background_ready":"Background measurement complete","dyn.background_failed":"Background measurement failed","dyn.listen_started":"Listening started","dyn.listen_failed":"Could not start listening","dyn.not_measured":"Not measured yet","dyn.listen_stopped":"Listening stopped; temporary measurement cleared","dyn.stop_failed":"Could not stop","dyn.invalid_brightness":"Invalid brightness","dyn.led_saved":"LED settings saved","dyn.save_failed_short":"Save failed","dyn.protocol_count":"{n} protocols · details only on request","dyn.receive_only":"Receive only","dyn.example":"Example","dyn.tx_hw_tested":"Transmit: hardware tested","dyn.tx_sw_tested":"Transmit: software tested","dyn.itho_separate":"Native control","dyn.tx_blocked":"Transmission blocked","dyn.tx_unavailable":"Transmission unavailable","dyn.rx_decoder_present":"Receive decoder present (hardware not confirmed)","dyn.in_ha_label":"In Home Assistant:","dyn.what_to_know":"What should I know?","dyn.software_prefix":"Software: ","dyn.encoder_registered":"transmit encoder registered; ","dyn.manifest_verified":"hardware status in manifest: verified","dyn.host_only":"host tested, physical radio test missing","dyn.no_generic_encoder":"no generic transmit encoder","dyn.only_approved":"RFLink shows only approved features.","dyn.technical_data":"Technical data","dyn.actions":"Actions","dyn.fields":"Fields","dyn.none":"none","dyn.learn":"Learn","dyn.no_protocols":"No protocols in this filter.","dyn.protocol_detection_note":"RFLink identifies the actual protocol only after receiving the signal. This is an instruction, not a protocol filter.","dyn.no_decoders":"No decoders registered.","dyn.decoder_registry_unavailable":"Decoder registry unavailable.","dyn.decoder_library_unavailable":"Decoder library unavailable. Learning and approval are temporarily disabled.","placeholder.device_name":"e.g. Garden temperature sensor","field.temperature":"Temperature","field.humidity":"Humidity","field.battery_low":"Battery low","field.rain":"Rain","field.fan_speed":"Fan speed","protocol.ExampleSwitch.band":"No real radio","protocol.ExampleSwitch.instruction":"This is a code example and cannot learn real signals.","protocol.ExampleSwitch.ha_result":"No Home Assistant device.","protocol.ExampleSwitch.warning":"Visible only in technical details; not intended for everyday use.","protocol.IthoRFT.band":"868.30 MHz","protocol.IthoRFT.instruction":"Repeatedly press one speed or timer button on your Itho remote.","protocol.IthoRFT.ha_result":"Confirmed speed/timer events and supported measurements where available.","protocol.IthoRFT.warning":"RFLink transmits with its own virtual Itho remote. Pair it with the ventilation unit once before using control.","protocol.Acurite609TXC.band":"433.92 MHz","protocol.Acurite609TXC.instruction":"Wait until the same sensor message has been received several times.","protocol.Acurite609TXC.ha_result":"Confirmed temperature, humidity and battery warning where available.","protocol.Acurite609TXC.warning":"After a battery change the RF identity may change; do not link a new identity automatically.","protocol.FineOffsetWH2.band":"433.92 MHz","protocol.FineOffsetWH2.instruction":"Wait for several messages from the weather sensor.","protocol.FineOffsetWH2.ha_result":"Confirmed temperature and humidity.","protocol.FineOffsetWH2.warning":"A missing measurement does not mean that feature should be removed.","protocol.Nexa.band":"433 MHz","protocol.Nexa.instruction":"Press On several times and then Off on the same channel.","protocol.Nexa.ha_result":"Confirmed On/Off events. Control buttons appear only after separate transmit permission.","protocol.Nexa.warning":"A remote and the lamp are not automatically the same device; learn multiple channels separately.","protocol.ByronSX.band":"433 MHz","protocol.ByronSX.instruction":"Press the doorbell button several times and wait until RFLink recognizes the same signal.","protocol.ByronSX.ha_result":"A bell event; a transmit button only after separate permission.","protocol.ByronSX.warning":"A transmit command does not confirm that the bell actually rang.","protocol.Dooya.band":"433 MHz","protocol.Dooya.instruction":"Press the same button repeatedly. Learn other buttons separately afterwards as feature proposals.","protocol.Dooya.ha_result":"Approved button events; transmit buttons only after separate permission.","protocol.Dooya.warning":"Button numbers do not yet define meanings such as Up or Stop. Do not assign them automatically.","protocol.KeeLoq.band":"433 MHz","protocol.KeeLoq.instruction":"Press the same button several times and check that the same remote is recognized.","protocol.KeeLoq.ha_result":"Received button events and battery warning when decoded.","protocol.KeeLoq.warning":"Rolling code is protected; received messages must not be retransmitted.","protocol.RCSwitch.band":"433 MHz","protocol.RCSwitch.instruction":"Press one button several times. Learn another fixed code separately and link it yourself to the desired device.","protocol.RCSwitch.ha_result":"One event for the recognized code; optionally a transmit button after permission.","protocol.RCSwitch.warning":"Code, protocol and bit length must match. On and off codes are not linked automatically.","protocol.AlectoV1.band":"433.92 MHz","protocol.AlectoV1.instruction":"Wait for several transmissions from the weather sensor; do not press random buttons.","protocol.AlectoV1.ha_result":"Only measurements you confirm, such as temperature, humidity and possibly rainfall.","protocol.AlectoV1.warning":"Not every variant transmits all measurements. A missing value is not automatically a fault.","protocol.SomfyRTS56.band":"433.42 MHz","protocol.SomfyRTS56.instruction":"Repeatedly press the same button on your Somfy remote. Do not use the programming button for this receive test.","protocol.SomfyRTS56.ha_result":"Received actions can be monitored. After pairing RFLink, approved Up, Down and My controls can also be used.","protocol.SomfyRTS56.warning":"Transmit uses a separate virtual RFLink remote with its own rolling counter. Pair RFLink with the motor before normal control; physical motor validation is still pending.","protocol.RubicsonTemp.band":"433.92 MHz","protocol.RubicsonTemp.instruction":"Wait for several messages from the temperature sensor.","protocol.RubicsonTemp.ha_result":"Confirmed temperature and battery warning where available.","protocol.RubicsonTemp.warning":"Channel and checksum are technical data and not separate Home Assistant sensors.","decoder.select_hint":"Select a decoder to view all information.","decoder.how_to_learn":"How to learn","decoder.why_tx":"Why transmit behaves this way","decoder.what_needed":"What is still needed","decoder.reason_sensor":"This protocol is a receive-only sensor. Transmit is intentionally disabled.","decoder.reason_example":"This is a development example, not a live RF protocol.","decoder.reason_tx_ready":"A reviewed transmitter encoder is available for this protocol.","decoder.reason_tx_host":"The transmitter encoder exists and passes software tests, but has not yet been verified with real RF hardware.","decoder.reason_itho":"Receive and native packet transmit are integrated. RFLink uses a separate virtual transmitter identity per learned Itho device.","decoder.reason_somfy":"Receive and transmit are implemented. RFLink controls Somfy as a separate virtual remote with its own rolling counter.","decoder.reason_keeloq":"Receive can identify supported frames, but generic transmit is blocked by KeeLoq rolling-code encryption.","decoder.reason_no_encoder":"Receive support exists, but no reviewed transmitter encoder is registered.","decoder.needed_none":"Nothing; receive-only is the intended behavior.","decoder.needed_hw_test":"A real-device RF test and capture comparison before marking transmit hardware-verified.","decoder.needed_itho":"Pair RFLink once with the ventilation unit, then validate the controls on real hardware.","decoder.needed_somfy":"Pair the RFLink virtual remote with the motor. Physical motor validation is still pending.","decoder.needed_keeloq":"A supported product plus its valid cryptographic key/pairing method. This cannot be derived safely from normal RF captures alone.","decoder.needed_encoder":"A protocol-specific transmit encoder plus software and hardware validation.","library.title":"Library & updates","library.help":"Official firmware items and your own items stay separate. RFLink never replaces your changes without confirmation.","library.official_decoders":"Official decoders","library.my_decoders":"My decoders","library.my_profiles":"My device profiles","library.needs_attention":"Needs attention","library.review":"Review changes","library.advanced":"Advanced","library.hide_advanced":"Hide advanced","library.advanced_help":"Technical library information for debugging and development.","library.schema":"Library schema","library.user_storage":"Persistent user library","library.safe_resolutions":"Safe automatic resolutions","library.archive":"Archived local items","library.enabled":"Enabled","library.scaffold":"Framework ready; persistent user library not enabled in this build","library.status_attention":"One or more local items need a decision. Your current setup stays active until you confirm a change.","library.status_ok":"Everything is up to date. No action is needed.","library.status_scaffold":"The update/conflict framework is ready. Persistent user decoders and profiles are the next storage layer.","library.attention_title":"Library update needs your attention","library.attention_count":"{n} local item(s) need a decision. Nothing has been replaced automatically.","library.unavailable":"Library status is temporarily unavailable.","library.review_pending":"Review details will become actionable when persistent user-library storage is enabled.","library.no_items":"No library items reported.","library.source":"Source","library.source_builtin":"Built-in","library.source_user":"User","library.stable_id":"Stable ID","library.version":"Version","library.pack":"Decoder pack","help.library_title":"What is the library?","help.library_text":"The library combines official firmware decoders with optional user-created decoders and device profiles. Official and local items remain separate.","help.library_more":"After an update, RFLink may recommend an official version. Local changes are never silently overwritten.","help.source_title":"What does Source mean?","help.source_text":"Built-in means the item ships with the firmware. User means it was learned or created on this RFLink device.","help.source_more":"Source helps RFLink decide what may be updated automatically and what needs your confirmation.","help.id_title":"What is a Stable ID?","help.id_text":"A Stable ID is the permanent identity of a decoder or profile across firmware versions.","help.id_more":"RFLink uses it together with versions and content fingerprints to detect duplicates, updates and local modifications.","user.heading":"My own protocol","user.guide":"Give the protocol a name, name the button or action you are recording, and press it at least three times. Technical rules stay under Advanced.","user.name":"Decoder name","user.id":"Stable ID","user.action":"Button/action","user.capture":"Record this action","user.clear":"Clear recordings","user.create":"Test and activate decoder","user.advanced":"Advanced: edit RX and TX rules","user.advanced_help":"RX rules recognize signals. TX rules create a preview. Saving is transactional: a working active decoder stays active if changed rules fail validation.","user.actions":"Actions, separated by commas","user.save_rules":"Save rules","user.vm_help":"Rules run in a bounded interpreter without native code. See USER_DECODER_GUIDE.md for syntax.","user.firmware":"Convert to firmware decoder","user.firmware_help":"Export the library and run the bundled evaluation and merge tool on your computer.","user.export":"Download evaluation export","user.saved":"Custom protocols & device profiles","user.profile":"Device profile","user.profile_help":"Give a decoder a friendly model name and learning instructions. Technical fields are optional.","user.profile_name":"Profile name","user.profile_id":"Profile ID","user.decoder":"Decoder","user.instructions":"Learning instructions","user.save_profile":"Save profile","user.samples":"samples","user.active":"active","user.inactive":"inactive","user.preview_ready":"TX preview available","user.rx_only":"Receive only","user.edit":"Edit","user.disable":"Disable","user.activate":"Activate","user.remove":"Remove","user.empty":"No user decoders saved yet.","user.unavailable":"User library unavailable.","user.activated":"Decoder activated.","user.disabled":"Decoder disabled.","user.confirm_remove":"Remove this item?","user.no_samples":"No recordings yet.","user.check_radio_action":"Choose a working radio and a valid action name.","user.press_three":"Press this button at least three times, then record the next action.","user.cleared":"Recordings cleared.","user.check_id_name":"Enter a decoder name. If you changed the stable ID manually, it must start with user_.","user.created_tx":"Decoder active. TX preview is ready; approve each device before sending.","user.created_rx":"Decoder active for receiving. Add a TX rule under Advanced if needed.","user.advanced_saved":"Rules saved, tested and activated.","user.profile_saved":"Profile saved.","user.exported":"Evaluation export downloaded.","user.device_controls":"User decoder and device profile","user.tx_notice":"Preview produces no radio signal. Sending needs approval for this device; the command is no RF acknowledgement.","user.no_profile":"No profile","user.revoke_tx":"Stop sending","user.allow_tx":"Allow sending","user.preview":"Preview TX","user.send":"Send","user.confirm_allow":"Allow this device to send signals using its user decoder?","user.confirm_send":"Send this RF action","user.sent_request":"TX command queued; no confirmation from the target.","user.import":"Restore library backup","user.import_help":"Every decoder is validated before replacement. TX permission is never restored automatically; protocol counters only move forward.","user.import_invalid":"This is not a valid RFLink user-library backup.","user.import_confirm":"Restore {decoders} decoder(s) and {profiles} profile(s)? Existing IDs are replaced only after validation.","user.imported":"Restore complete: {decoders} decoder(s), {profiles} profile(s), {bindings} existing device profile binding(s). TX permissions were not restored.","user.analysis_waiting":"Record examples to compare stable and changing RF fields.","user.analysis_need_more":"{action}: record at least one more example for comparison.","user.analysis_length":"{action}: recorded frames have different lengths; inspect this protocol manually.","user.analysis_dynamic":"{action}: {count} symbols; changing positions: {positions}. These may contain a counter, checksum, ID or button-dependent data.","user.analysis_fixed":"{action}: {count} symbols; stable in the recorded examples.","user.radio_profile":"Radio profile","user.radio_profile_help":"This release uses the board's validated radio setup; custom CC1101 modulation/bitrate is not enabled from user recipes yet.","user.analysis_counter":"{action}: position(s) {positions} look like an incrementing counter. Automatic replay stays disabled when values change.","user.analysis_checksum":"{action}: position(s) {positions} look like a simple checksum candidate. Verify this before writing an Advanced rule.","library.official_profiles":"Official device profiles","decoder.known_profiles":"Known device profiles","decoder.known_profiles_help":"Model families found in public vendor/protocol sources for this decoder. This is source-based compatibility information, not a physical hardware certification.","decoder.models":"Models / aliases","decoder.profile_learn":"How to learn this profile","decoder.hardware_pending":"Hardware validation on this RFLink board pending","ui.manage_devices":"Manage devices","ui.manage_devices_help":"Rename devices, review new functions and manage Home Assistant.","ui.settings_sub":"Everyday settings first. Library and technical tools stay lower on the page.","devices.pairing_filter":"Show setup guides","devices.show_all":"Show all devices","devices.waiting_signals":"Signals waiting for review","devices.waiting_signals_help":"These signals were recognized outside the guided Add device flow. Review only the ones you want to keep.","placeholder.wizard_device_name":"e.g. Living room remote","placeholder.profile_name":"e.g. Living room remote","placeholder.profile_help":"e.g. Press each button three times","accessibility.led_brightness":"RGB brightness","accessibility.close":"Close","dyn.online":"Online","dyn.frames_label":"Signals","dyn.device_sub_simple":"Your saved RF devices. Open a device only when you want to change something.","led.boot":"Starting","led.connecting":"Connecting to Wi-Fi","led.ready":"Ready","led.offline":"Connection lost","led.radio_missing":"Radio missing or not ready","led.learning":"Learning","led.capture":"Signal received","led.learned":"Device confirmed","led.itho_pair":"Itho pairing command","led.tx":"Sending RF","led.ota":"Firmware update","led.error":"Error","led.unknown":"Unknown","decoder.compatibility_details":"Compatibility details & sources","decoder.control":"Control","decoder.next_step":"Still needed","user.saved_help":"Only use this when RFLink does not already know your device or when you want a custom model profile.","user.profile_advanced":"Advanced profile options","user.profile_auto_help":"Leave the ID empty to create it automatically from the profile name. Actions are prefilled from the selected decoder when possible.","settings.developer_info":"Developer information","placeholder.decoder_name":"e.g. Living room remote","user.id_auto_help":"Leave this empty when creating a decoder; RFLink will make a stable ID from the name. Change it only when you need a specific ID.","settings.mqtt_setup":"MQTT setup","settings.led_simple_help":"Adjust brightness or disable the green ready heartbeat. Warning colours stay active.","settings.led_more":"LED colours & behaviour","ui.advanced":"Advanced","ui.advanced_details":"Advanced details","ui.advanced_filters":"Advanced filters","ui.try_again":"Try again","ui.decoder_advanced_help":"Record and compare raw radio data.","ui.freq_43392":"Most common for remotes, sensors and switches","ui.freq_43342":"Often used by Somfy RTS","ui.freq_868":"Often used by Itho and other 868 MHz devices","ui.freq_other":"Use this when your device works on this frequency","ui.frequency_help":"Not sure? 433.92 MHz is the most common starting point.","ui.device_found_help":"RFLink found the device. Give it a name and save it.","ui.protocol_label":"Protocol","ui.signal_seen":"Signal received","ui.back_add_device":"← Back to Add device","somfy.control":"Somfy control","somfy.tx_disabled":"Transmit is disabled in the firmware settings. Enable TX controls before pairing.","somfy.pair_help":"Put the motor in programming mode with an existing remote, then pair RFLink.","somfy.pair":"Pair RFLink","somfy.control_ready":"Somfy control ready to test","somfy.up":"Up","somfy.my":"My / Stop","somfy.down":"Down","somfy.no_ack":"RFLink cannot confirm the motor response over the air. If the motor does not react, pair again.","somfy.pair_again":"Pair again","somfy.pair_confirm":"Put the motor in programming mode first. Send the RFLink PROG command now?","somfy.pair_sent":"Pairing command sent","somfy.pair_failed":"Pairing command failed","somfy.command_sent":"Command sent","somfy.command_failed":"Command failed","somfy.state":"Virtual remote {address} · next rolling code {counter}","led.pairing":"Pairing","itho.control":"Itho control","itho.tx_disabled":"Transmit is disabled in the firmware settings. Enable TX controls before pairing.","itho.pair_help":"Switch the ventilation unit off for about 15 seconds and on again. Pair RFLink within 2 minutes.","itho.pair":"Pair RFLink","itho.control_ready":"Itho control ready to test","itho.low":"Low","itho.medium":"Medium","itho.high":"High","itho.timer10":"Timer 10 min","itho.timer20":"Timer 20 min","itho.timer30":"Timer 30 min","itho.standby":"Standby","itho.no_ack":"RFLink cannot directly confirm that the unit stored the pairing. If it does not react, pair again or try compatibility pairing.","itho.compat_help":"If normal pairing does not work, try compatibility pairing.","itho.pair_again":"Pair again","itho.compat_pair":"Compatibility pairing","itho.unpair":"Unpair RFLink","itho.pair_confirm":"Switch the unit off for about 15 seconds and on again first. Send the RFLink pairing command now?","itho.compat_confirm":"Switch the unit off for about 15 seconds and on again first. Send compatibility pairing now?","itho.unpair_confirm":"Put the unit in registration mode again first. Unpair RFLink now?","itho.pair_sent":"Pairing command sent","itho.pair_failed":"Pairing failed","itho.unpair_sent":"Unpair command sent","itho.unpair_failed":"Unpair failed","itho.command_sent":"Command sent","itho.command_failed":"Command failed","itho.state":"Virtual remote {address} · next counter {sequence} · type {type}","diag.checking":"Checking","diag.cc1101_failed":"CC1101 not detected","diag.rx_failed":"CC1101 OK · RX fault","diag.tx_failed":"CC1101 OK · TX fault","diag.ok":"Ready","diag.spi_ok":"Detected / initialized","diag.spi_failed":"Initialization failed","diag.cc1101_spi":"CC1101 / SPI","diag.rx_path":"Receive path","diag.tx_path":"Transmit path","diag.ready":"Ready","diag.failed":"Failed","diag.configured_pins":"Configured pins","diag.hardware":"Hardware","diag.cc1101_hint":"The CC1101 itself failed before RF receive/transmit. Check 3.3 V, GND, CSN and the shared SCK/MOSI/MISO wiring. GDO wiring is not the first suspect.","diag.spi_probe":"Read-only SPI probe","diag.probe_response":"Valid response","diag.probe_no_response":"No valid response","diag.spi_checks":"SPI checks","diag.spi_failures":"SPI errors","diag.spi_consecutive":"Consecutive errors","diag.spi_recovery_attempts":"Recovery attempts","diag.spi_recovery_successes":"Successful recoveries","diag.spi_health":"Digital SPI health","diag.spi_health_note":"Software can detect invalid SPI responses, but not electrical reflections, ringing or overshoot. Use an oscilloscope or logic analyzer for signal integrity.","monitor.show_noise":"Show suspected RF noise","monitor.noise_help":"Suspected noise is hidden by default, but remains available for diagnostics and export.","monitor.copy":"Copy","monitor.export":"Export","monitor.copied":"Visible activity copied.","monitor.exported":"Activity export downloaded.","monitor.noise_badge":"Suspected RF noise","monitor.noise_reason":"Reason","diag.noise_frames":"Suspected noise","diag.freq_offset":"Frequency offset","diag.uptime":"Uptime","diag.boot_count":"Recorded starts","diag.reset_reason":"Last reset reason","diag.boot_log":"Recent starts","diag.boot_log_hint":"Saved on this ESP32, without timestamps (the board has no guaranteed clock at boot). Newest first.","diag.log_download":"Export diagnostics","diag.heap_free":"Free RAM","diag.heap_min":"Lowest free RAM","diag.boot_unavailable":"Boot history not saved in flash","diag.reboot_observed":"Restart detected while this page was open","diag.counter_reset":"Signal counter decreased without an ESP32 restart","diag.since_boot":"Signal counters are since the last boot","diag.no_events":"No events observed while this page was open","about.heading":"About RFLink Universal","about.version":"Firmware version","about.revision":"Installed revision","about.build_more":"Build information","about.built":"Compiled","rxlog.title":"Reception log","rxlog.signals":"Signals","rxlog.noise":"Noise","rxlog.all":"All","rxlog.radio":"Radio","rxlog.all_radios":"All radios","rxlog.settings":"Logging settings","rxlog.signal_limit":"Maximum signals","rxlog.noise_limit":"Maximum noise (all radios)","rxlog.noise_floor":"Soft noise retention per radio","rxlog.memory":"Log memory budget","rxlog.download":"Download logs (ZIP)","rxlog.saved":"Saved without restarting","rxlog.memory_eviction":"Noise is removed first when memory is low","rxlog.type":"Log type","rxlog.settings_help":"Change limits while running. Settings are saved. Shared noise is removed first when memory is full.","rxlog.radio0":"433.42 MHz signals","rxlog.radio1":"433.92 MHz signals","rxlog.radio2":"868.30 MHz signals","rxlog.save":"Save log settings","rxlog.older":"Load older events","rxlog.stored":"Stored","rxlog.noise_info":"Noise is stored together but can be filtered by radio.","rxlog.invalid":"Enter valid limits (combined maximum 4096).","rxlog.exporting":"Preparing complete log export…","rxlog.exported":"ZIP downloaded","rxlog.token_error":"Write authorization not available","settings.general":"General & status light","settings.integrations":"Home Assistant & system","settings.additional":"More settings & tools","settings.dashboard_hint":"Common settings are ready to use. Open a tool below only when needed.","nav.back_settings":"← Back to settings overview","nav4.search_devices":"Search saved devices","nav4.search_placeholder":"Name, protocol or radio…","nav4.unknown_radio":"Unknown radio","nav4.clear":"Clear","nav4.found":"Showing {visible} of {total} devices","nav4.no_results":"No matching devices. Adjust or clear the search.","nav4.mqtt_host":"Broker address","nav4.mqtt_port":"Port","nav4.mqtt_user":"Username","nav4.mqtt_pass":"Password (leave empty to keep)","nav4.mqtt_enable":"Enable MQTT","nav4.mqtt_save":"Save & connect","nav4.mqtt_not_built":"MQTT is not compiled in. Enable the optional package once.","nav4.mqtt_security":"Stored on the device. Use only on a trusted network; default NVS is not encrypted.","nav4.mqtt_saved":"MQTT settings saved. Connecting…","nav4.mqtt_bad":"Invalid MQTT settings or storage error","devices.radio_count":"{n} devices","profileflow.choose":"Device profile","profileflow.none":"Select a profile","profileflow.suggested":"Suggested","profileflow.select_help":"Choose a profile to show its known functions. No RF identity has been verified.","profileflow.manual_warning":"Possible family match, but the radio message does not prove the brand or exact model. Please confirm.","profileflow.proposal_warning":"Suggested from recognized protocol/actions; confirm the actual product model.","profileflow.user_choice":"Profile selected manually.","profileflow.after_profile":"Known controls","profileflow.sensor_profile":"Sensor profile: measurements are shown when received.","profileflow.choose_first":"Choose a device profile to see its functions.","profileflow.measurements":"Measurements","profileflow.sensor_info":"Sensor values appear automatically when received. No transmit buttons are needed.","profileflow.operation":"Control","profileflow.no_commands":"This profile has no known remote-control commands.","profileflow.tx_disabled":"Buttons shown for reference; RF transmission is disabled for this protocol.","profileflow.pair_first":"Pair the RFLink transmitter before using these controls.","profileflow.not_ready":"Controls are not yet available. Firmware TX/encoder and radio state must be ready; learning RX does not enable sending.","profileflow.tx_ready":"Controls available. A sent command is not a confirmation from the device.","profileflow.functions_advanced":"Received functions","profileflow.saved":"Profile saved","profileflow.save_error":"Could not save profile","profileflow.send_confirm":"Send {command}?","profileflow.sent":"Transmit requested","profileflow.send_error":"Command not sent","profileflow.command.auto":"Auto","profileflow.command.low":"Low","profileflow.command.medium":"Medium","profileflow.command.high":"High","profileflow.remove_tx_locked":"Disable transmission for this device before removing technical fields.","profileflow.dismiss_candidate":"Remove suggestion","profileflow.dismiss_confirm":"Remove this not-yet-saved signal? It may appear again if transmitted.","profileflow.dismissed":"Suggestion removed","profileflow.dismiss_failed":"Could not remove suggestion","profileflow.setup":"Guided setup for this model","profileflow.setup_rx_only":"These steps guide identification and reception. RFLink pairing and TX remain unavailable until physically validated.","profileflow.command.away":"Away","profileflow.command.timer_15":"15 min","profileflow.command.timer_30":"30 min","profileflow.command.timer_60":"60 min","profileflow.command.bypass_auto":"Bypass auto","profileflow.command.bypass_open":"Bypass open","profileflow.command.bypass_close":"Bypass closed","catalog.protocols":"Protocols","catalog.profiles":"Device profiles","catalog.search_placeholder":"Search brand, model or protocol…","catalog.all_brands":"All brands","catalog.all_families":"All families","catalog.all_protocols":"All protocols","catalog.profile_uses_protocol":"Decoder","catalog.show_related":"View device profiles","catalog.select_profile":"Select a device profile to view its details.","catalog.custom_brand":"Custom","catalog.custom_family":"My device profiles","catalog.protocol":"Protocol","catalog.no_help":"No learning instructions provided.","catalog.operation":"Control status","catalog.tx_profile_warning":"Commands depend on the actual encoder, pairing and radio validation.","catalog.tx_not_ready":"Control functions are shown, but transmitting is not yet available for this profile.","catalog.rx_only":"Receive only; no transmit buttons.","catalog.profile_functions":"Profile functions","catalog.received_actions":"Received actions","catalog.setup_steps":"Setup steps","catalog.profile_source":"Profile source","catalog.profile_protocol_note":"Profiles describe devices; decoders process radio signals.","catalog.profile_count":"{shown} of {total} device profiles","catalog.no_results":"No device profiles match these filters.","action.auto":"Auto","action.away":"Away","wizard.setup_title":"Set up device","wizard.start":"Step-by-step setup","wizard.step":"Step","wizard.close":"Close","wizard.back":"Back","wizard.next":"Next","wizard.finish":"Done (receive only)","wizard.check":"Check result","wizard.rx_only":"Continue receive-only","wizard.transmit":"Send pairing command","wizard.identity_help":"Check that the received RF identity belongs to this device or existing remote. Choose the exact model before proceeding.","wizard.finish_help":"Check the result. Sending a signal does not prove the device paired or changed state.","wizard.observe_help":"Receive a signal from the device and check its identity.","wizard.choose_profile":"Choose a device profile first.","wizard.rx_ready":"Reception is ready. Pairing is not needed for this profile.","wizard.sent_not_verified":"Command transmitted; no receiver acknowledgement has been established.","wizard.not_supported":"Pairing is not supported for this device yet.","wizard.requires_setup":"Setup steps are available; pairing not verified.","wizard.orcon_tx_missing":"Orcon: the RF transmitter and RAMSES-II 1FC9 handshake have not yet been hardware validated. This wizard cannot pair or enable the controls.","wizard.tx_unavailable":"The reviewed encoder and physical TX are not available. RFLink stays receive-only.","wizard.verify_observed":"I saw the device respond","wizard.verify_question":"Did you personally observe the expected device response? This is a manual confirmation, not a radio ACK.","wizard.transmit_confirm":"Is the receiver in its pairing window? Send one approved pairing command now?","ux3.change_profile":"Change device profile","ux3.select_profile":"Choose a device profile","ux3.no_profile":"Not selected","ux3.not_in_ha":"Not published to HA","ux3.manage_details":"Manage device and RF signals","ux3.not_yet_available":"Control is not yet available.","ux3.finish_rx_only":"Close (receive only)","ux3.orcon_setup_short":"Receive now; RF pairing is still being hardware tested.","ux3.rx_only_short":"Signals can be received, but RF control is not ready yet.","ux3.receive_only_status":"Receiving only","ux3.control_ready_status":"Controls available","ux3.sensor_status":"Receiving measurements","ux3.profile_missing_status":"Choose device profile","debug.menu":"System trace","debug.title":"System trace","debug.help":"What RFLink does with a signal or command: decoder, learned device, profile, pairing, TX, and RX restoration. Separate from radio logs.","debug.lab_gate":"GDO TX lab","debug.probe_gate":"RAMSES TX test","debug.radio":"868 MHz radio state","debug.filter":"Show","debug.all":"All","debug.live":"Live","debug.refresh":"Refresh","debug.export":"Export trace","debug.empty":"No system events yet.","debug.experiment":"Advanced: physical RAMSES TX test","debug.experiment_help":"Sends one 10E0 self-description frame, not a fan command or pairing. Requires both YAML lab flags. Use only with your own equipment and verify RF with a separate receiver.","debug.send":"Send test frame","debug.privacy":"128 events in RAM (cleared on restart). Device IDs may be present: review before sharing. Does not contain raw RF or passwords.","debug.fetch_failed":"Cannot retrieve system events:","debug.enabled":"Enabled","debug.disabled":"Disabled","debug.ready":"Radio ready","debug.not_ready":"Not ready","debug.fault":"RX recovery fault","debug.count":"{shown} of {total} events","debug.dropped":"Dropped","debug.confirm":"Send ONE experimental 868 MHz RAMSES 10E0 test frame? This does not pair or control the fan.","debug.sending":"Sending experimental test frame…","debug.sent_unverified":"Hardware adapter reported sent and RX restored. RF transmission and fan response NOT confirmed.","debug.failed":"Test failed","logbook.title_nav":"▤ Logs","logbook.title":"Logbooks","logbook.subtitle":"Radio events, firmware decisions and browser actions together.","logbook.all":"All","logbook.radio":"Radio","logbook.system":"System","logbook.web":"Web","logbook.level":"Level","logbook.any":"Any","logbook.warnings":"Warnings & errors","logbook.errors":"Errors","logbook.search":"Search","logbook.search_hint":"Device, ID, action…","logbook.noise":"Include noise","logbook.live":"Live","logbook.refresh":"Refresh","logbook.export":"Export view","logbook.note":"Web events only exist in this browser session. ESP32 uses uptime; browser times are approximate. API payloads are never logged.","logbook.system_limit":"System events (ESP32)","logbook.web_limit":"Web events (browser)","logbook.verbose":"Web debug: include normal API refreshes","logbook.settings_note":"System limits reset on restart; browser preferences remain here. Radio limits persist on the ESP32.","logbook.empty":"No events yet.","logbook.events":"events","logbook.invalid":"Invalid limits.","logbook.saved":"Settings saved.","logbook.save_failed":"Could not save:","orcon.experimental_status":"Experimental pairing and control; requires explicit lab enable.","orcon.enable_lab":"Enable TX controls, GDO UART lab and RAMSES probe lab in YAML before attempting a real 1FC9 pairing.","orcon.wait_ack":"Wait for the matching fan acknowledgement before confirming.","orcon.status_waiting_ack":"1FC9 offer sent; waiting for matching fan acknowledgement (up to 3 minutes).","orcon.status_confirming":"Fan ACK received; finishing confirmation after radio returns to RX.","orcon.status_awaiting_user":"Matching fan ACK and follow-up transmission recorded. Confirm only if you observed a device reaction.","orcon.status_failed":"Pairing failed/timed out. No normal controls have been activated. Check system and radio logs.","orcon.status_paired":"Pairing confirmed by user; experimental controls can be tested.","orcon.status_not_paired":"No pairing established.","orcon.test_auto":"Test Auto (once)","orcon.confirm_test_auto":"Send one experimental Auto command now? Observe the fan response yourself.","orcon.sent_unconfirmed":"Auto transmitted; fan response not confirmed.","deviceux.back":"← Back to devices","deviceux.detail_help":"Set up, pair or maintain this device. Everyday controls remain on the overview.","deviceux.setup_device":"Set up device","deviceux.view_details":"View details","deviceux.settings":"Settings & pairing","deviceux.no_profile_help":"Choose the device model to see the available functions.","deviceux.profile_unavailable":"The saved profile could not be loaded. Choose a profile again; the received RF identity is retained.","logs.group_radio":"Radio","logs.radio_help":"Signals per radio and shared noise.","logs.group_system":"System","logs.system_help":"Decisions about decoding, devices, pairing and transmitting on the ESP32.","logs.group_web":"Web","logs.web_help":"Navigation, clicks and API issues in this browser session.","version.subtitle":"Device controls, unified logbooks and experimental TX.","version.notes":"What changed and technical details","version.changes":"What is new","version.raw":"Internal firmware version","version.technical":"Technical details","version.legacy":"Installed firmware (legacy naming).","version.legacy_note":"For this older build, review its original release notes.","version.note1":"A simpler device overview with separate settings and pairing.","version.note2":"Radio, system and web events in one fixed-height log viewer.","version.note3":"Grouped logging settings and a directly visible library summary.","version.note4":"Instrumented device and library APIs for crash diagnosis.","version.lab_tools":"TX lab & system diagnostics","version.lab_help":"This page is for experimental radio/TX diagnostics. For normal system events, use Logbooks → System.","version.open_logs":"Open system logbook","deviceux.remove_short":"Remove","logbook6.frequency":"Frequency","logbook6.time":"Time","logbook6.source":"Source","logbook6.event":"Event","logbook6.filtered_empty":"No matching events. Enable Show noise to include noise packets.","deviceux6.profile_title":"Choose device profile","deviceux6.profile_help":"Select the correct device type. This determines its controls and pairing steps.","deviceux6.pair_title":"Pair and test","deviceux6.pair_help":"Connect this device to RFLink if pairing is supported.","deviceux6.status_title":"Current status","deviceux6.no_profile_yet":"Controls will appear after selecting a profile.","deviceux6.extra_title":"Additional management","deviceux6.intro":"Select a profile, pair when needed and manage optional settings.","logbook6.info":"Info","logbook6.warn":"Warning","logbook6.error":"Error","logbook6.noise":"Noise","diag7.event_type":"Type","diag7.boot_filter":"Crashes and restarts","diag7.tx_filter":"Transmission and pairing","diag7.api_filter":"API and browser","diag7.view_log":"View crash log","diag7.crashes":"{n} crashes in the last 8 boots","diag7.crash_help":"RFLink restarted after a fault. The reset reason alone does not reveal the exact cause.","diag7.export":"Diagnostic package (JSON)","diag7.uptime_note":"Times show hh:mm:ss,mmm since ESP32 boot; browser times are approximate.","diag7.health_attention":"Attention: previous crashes","diag7.health_detail":"The radios and decoders are reachable, but RFLink has recently restarted after errors.","diag7.raw_trace":"Technical trace (optional)","version.source_hash":"Source hash (SHA-256, shortened)","version.history_time":"Earlier boot","version.history_event":"Boot #{n} · reset reason: {reason}","version.history_note":"Historical NVS record; exact time, root cause and backtrace were not saved.","version.note5":"v0.6.1 Radio Gaga: improved experimental CC1101/GDO-UART transmit preparation.","version.note6":"UART1 is now installed before its 8N1 configuration, with specific errors for each step.","version.note7":"The radio log shows TX requests, modem/UART stages, failures and RX restoration.","version.note8":"Device details are simpler: functions, Home Assistant and RF management without a nested main accordion.","crashtrace.reason":"Reason","crashtrace.core":"Core","crashtrace.firmware":"Crash firmware build","crashtrace.other_build":"Older/different build (original ELF required)","crashtrace.unknown_build":"Unknown build","crashtrace.backtrace":"Backtrace","crashtrace.other_core":"Other core","crashtrace.elf_note":"Raw addresses: use the exact crash-build firmware.elf to decode. No exact historical timestamp.","crashtrace.available":"Backtrace available","crashtrace.boot_event":"Crash backtrace for boot #{n}","crashtrace.unknown_boot":"Older crash backtrace (boot number unknown)","diag55.tab":"Diagnostics","diag55.title":"All diagnostics in one place","diag55.intro":"Radio and SPI health, restarts, crash details and experimental TX. Log events appear in the shared Radio and System timeline.","diag55.radio_heading":"Radios, SPI and boot history","diag55.experimental_heading":"Experimental TX and system decisions","diag55.open_crashes":"View crash events","diag55.open_radio":"View radio log","diag55.spi_filter":"SPI / radio recovery","diag55.boots":"Crashes & restarts","diag55.radio":"Radios & SPI","diag55.memory":"Memory","diag55.lab":"Experimental","diag55.export":"Diagnostic report (JSON)","diag55.boot_hint":"Previous boots and available traces. Boot numbers are shown when no timestamps exist.","diag55.lab_hint":"TX, UART/GDO and extended system diagnosis. A test transmission never starts automatically.","diag55.ready":"receive ready","diag55.crashes":"crashes","diag55.internal":"Internal free","diag55.trace":"Technical crash","diag55.nohistory":"No boot history available.","diag55.online":"online","diag55.offline":"offline","diag55.errors":"errors","diag55.restores":"recoveries","diag55.no_radio":"No radios available.","diag55.lab_enabled":"Lab enabled","diag55.lab_disabled":"Lab disabled","diag55.adapter_ready":"adapter ready","diag55.adapter_unready":"adapter not ready","pair55.queued":"Pairing operation queued. Check Radio and System logs for the result.","pair55.auto_queued":"Auto test queued. RF transmission has not been confirmed.","orcon.status_queued":"Pairing operation queued. Check Radio and System logs for the result.","rf60.status_intro":"A calm overview of your system, radios and anything needing attention.","rf60.system":"System","rf60.radios":"Radios","rf60.radios_help":"Available radios","rf60.devices":"Devices","rf60.saved":"Saved devices","rf60.attention":"Attention","rf60.attention_title":"Needs attention","rf60.attention_help":"Only items needing action.","rf60.no_problems":"No issues found.","rf60.needs_attention":"Check needed","rf60.radio_overview":"Radio overview","rf60.radio_help":"Open a radio to inspect RX and TX activity.","rf60.activity_help":"Latest events; the full history is in Logbooks.","rf60.open_logs":"Open full logbook","rf60.system_details":"Technical system status","rf60.quick_actions":"Quick actions","rf60.quick_help":"Common tasks, one click away.","rf60.last_seen":"Last signal","rf60.not_seen":"Not yet observed","rf60.more_details":"More details","rf60.control":"Control","rf60.values":"Values","rf60.no_values":"No measured values available yet.","rf60.protocol":"Protocol","rf60.controls_title":"Control & functions","rf60.advanced_details":"Signals and technical details","rf60.advanced_help":"Received signals, RF identifiers, Home Assistant and maintenance.","rf60.remove_help":"Removes this device and its RFLink configuration. Confirmation is required.","rf60.inspect":"View diagnostics","rf60.history_issues":"Previous crashes: {n}","rf61.functions_title":"Received functions","rf61.functions_help":"{n} recognized functions. Review new functions and what RFLink learned.","rf61.ha_title":"Management and Home Assistant","rf61.ha_help":"Add this device to Home Assistant or give it a familiar name.","rf61.signals_title":"Manage RF signals","rf61.signals_help":"Review or replace linked RF codes.","rf61.guide_title":"Setup instructions","rf61.guide_help":"Guidance for the chosen device profile.","rf61.id_title":"Technical device ID","rf61.id_help":"Internal identifier for support and debugging.","version.note9":"v0.6.2 Radio Gaga: one launcher for updating, clean releases and tests; guides and historical documents are organized in docs/."},"nl":{"static.000":"RFLink Universal","static.001":"● Online","static.002":"0 signalen","static.003":"Uptime —","static.004":"◉ Status","static.005":"▣ Apparaten","static.006":"◷ Activiteit","static.007":"◇ Apparaat toevoegen","static.008":"⚙ Instellingen","static.009":"Status","static.010":"Alleen wat je dagelijks nodig hebt. Technische details zitten een laag dieper.","static.011":"Systeem controleren…","static.012":"Status ophalen.","static.013":"Bekijk diagnose","static.014":"Radio-status laden…","static.015":"RGB-statuslamp","static.016":"Actuele status","static.017":"Laden…","static.018":"LED","static.019":"WS2812 · GPIO48","static.020":"Snelle acties","static.021":"+ Apparaat toevoegen","static.022":"Zoek en bewaar een afstandsbediening, sensor of schakelaar.","static.023":"Pairing","static.024":"Bekijk apparaten waarvan de decoder pairing ondersteunt.","static.025":"Onbekend protocol","static.026":"Developer Mode helpt captures vergelijken en code voorstellen.","static.027":"Probleem oplossen","static.028":"Diagnose, decoderbibliotheek en technische informatie.","static.029":"Recente activiteit","static.030":"Nog geen activiteit.","static.031":"Apparaten","static.032":"RFLink maakt pas een apparaat nadat een signaal voldoende is geobserveerd en jij het bevestigt. Per apparaat kun je RF-codes bekijken, ontkoppelen of veilig vervangen.","static.033":"Hier staan apparaten met instelinstructies. Niet elk apparaat kan al via RFLink zenden.","static.034":"Apparaatopslag controleren…","static.035":"Apparaten laden…","static.036":"Nieuwe signalen","static.037":"Candidates laden…","static.038":"Activiteit","static.039":"Recente signalen die RFLink heeft gezien.","static.040":"Filter op frequentie","static.041":"Status laden…","static.042":"Richting","static.043":"RX en TX","static.044":"Alleen RX","static.045":"Alleen TX","static.046":"TX-regels tonen opdrachten die aan de radio zijn gegeven, niet een bevestiging van het doelapparaat. Ruwe radiogegevens zijn alleen voor foutzoeken.","static.047":"Activiteit alle radio's","static.048":"Activiteit laden…","static.049":"Apparaat toevoegen","static.050":"Kies de frequentie, druk op Start en gebruik het apparaat.","static.051":"Apparaat toevoegen","static.052":"Voor dagelijks gebruik","static.053":"Decoder maken","static.054":"Alleen nodig als RFLink het protocol nog niet kent.","static.055":"Kies een frequentie","static.056":"Radiostatus controleren…","static.057":"Apparaat toevoegen","static.058":"Druk op Start en gebruik het apparaat daarna één of een paar keer.","static.059":"Start","static.060":"Stoppen","static.061":"Naam van het apparaat","static.062":"Functies","static.063":"Kies de functies die je wilt gebruiken.","static.064":"Toevoegen aan Home Assistant","static.065":"Technische radiogegevens worden automatisch opgeslagen.","static.066":"Opslaan","static.067":"Als bestaand apparaat koppelen","static.068":"Technische meting","static.069":"Houd het apparaat binnen ongeveer 1 meter van de juiste antenne. Druk nog niet op een knop.","static.070":"Achtergrond meten (5 sec)","static.071":"Luisteren naar apparaat","static.072":"Stoppen en meting wissen","static.073":"Achtergrond","static.074":"Nog niet gemeten","static.075":"Herhalingen","static.076":"—","static.077":"Protocol","static.078":"Identiteit","static.079":"Signaal","static.080":"Wachten op capture","static.081":"Wanneer RFLink het apparaat meerdere keren betrouwbaar ziet, verschijnt het onder","static.082":"Apparaten → Nieuwe signalen","static.083":". Daar geef je het een naam of koppel je het aan een bestaand apparaat.","static.084":"Decoder-assistent","static.085":"RFLink maakt geen firmwarecode op de ESP32 zelf. Het geeft je een veilige start die je in één nieuwe decodermap kunt plakken.","static.086":"Nieuwe map maken","static.087":"python tools/new_decoder.py mijn_protocol \"Mijn Protocol\"","static.088":"Daarna wijzigen","static.089":"alleen decoder.cpp + manifest.json","static.090":"Codevoorstel bekijken","static.091":"Maak eerst een capture.","static.092":"Instellingen & hulp","static.093":"Eerst duidelijke routes; technische informatie alleen wanneer je die nodig hebt.","static.094":"WS2812 op GPIO48. Oranje knipperen = een of meer radio’s ontbreken of zijn niet gereed. Wit = OTA, rood = fout; groen alleen bij volledige gereedheid. De groene hartslag kan worden uitgezet zonder waarschuwingen te verbergen.","static.095":"Helderheid","static.096":"20%","static.097":"Groene hartslag bij gereed","static.098":"Aangevinkt = groen knippert als de status \"gereed\" is (twee korte flitsen per 6 seconden). Uitgevinkt = geen groene gereed-indicatie. Stel voor de eerste test de helderheid op 100% en klik op Opslaan.","static.099":"Huidige status","static.100":"LED-instellingen opslaan","static.101":"Instellingen worden op de ESP32 opgeslagen en overleven een herstart. De webpagina is alleen bedoeld voor een vertrouwd netwerk.","static.102":"Wat wil ik doen?","static.103":"Nieuw apparaat","static.104":"Open Leren","static.105":"Pairing-apparaten","static.106":"Home Assistant (MQTT Discovery)","static.107":"RF-apparaten voeg je per stuk toe onder Apparaten. De ESP32 zelf blijft via ESPHome API gekoppeld.","static.108":"MQTT-broker","static.109":"Controleren…","static.110":"Schakel optioneel het MQTT-pakket in de YAML in en voeg de brokergegevens aan secrets.yaml toe. Apparaten verwijderen uit Home Assistant vereist een werkende brokerverbinding.","static.111":"Beheer HA-apparaten","static.112":"Decoderbibliotheek","static.113":"Wat kan RFLink herkennen, wat komt er in Home Assistant en wat kun je al bedienen? Softwaretests zijn geen radiotests.","static.114":"Toon","static.115":"Alle protocollen","static.116":"Afstandsbedieningen en bediening","static.117":"Sensoren","static.118":"Ontwikkelvoorbeelden","static.119":"Bibliotheek laden…","static.120":"Technische decoderregistratie","static.121":"Onderhoudsregel","static.122":"Een decoder hoort maar één taak te hebben:","static.123":"RFFrame → DecodeResult","static.124":"Geen web-UI, Home Assistant, GPIO-configuratie of andere decoderlogica in een protocolmodule.","static.125":"Diagnose","static.126":"De radio- en decoderstatus wordt geladen.","static.127":"Opgeslagen apparaten","static.128":"Decoderregistratie","static.129":"Opslag","static.130":"Ontvangen","static.131":"Gedecodeerd","static.132":"Onbekend","static.133":"Technische uitleg","static.134":"Een puls (pulse) is een periode waarin het ontvangen RF-signaal HIGH of LOW is. Een bit kan uit meerdere pulsen bestaan. Developer Mode gebruikt deze timings om protocollen te vergelijken.","static.135":"Monitor","static.136":"Instellingen","static.137":"Geef een herkenbare naam. De RF-identiteit blijft intern bewaard.","static.138":"Controleer de functies","static.139":"Annuleren","static.140":"Opslaan","language.title":"Taal en regio","language.help":"Kies de taal van de interface. Je voorkeur wordt op de ESP32 opgeslagen.","language.label":"Interfacetaal","language.saving":"Taal opslaan…","language.saved":"Taal opgeslagen.","language.failed":"Taal kon niet op de ESP32 worden opgeslagen. Vorige taal hersteld.","dyn.now":"nu","dyn.sec":"{n} sec","dyn.min":"{n} min","dyn.hour":"{n} uur","dyn.dayhour":"{d}d {h}u","dyn.uptime":"Uptime {value}","dyn.frames":"{n} signalen","action.on":"Aan","action.off":"Uit","action.up":"Omhoog","action.down":"Omlaag","action.my":"Favoriet (My)","action.prog":"Programmeren","action.low":"Laag","action.medium":"Midden","action.high":"Hoog","action.standby":"Stand-by","action.status":"Status","action.measurement":"Meting","action.rain_update":"Neerslagmeting","action.weather_update":"Weermeting","action.code":"Vaste code","action.button":"Knop {n}","action.command":"Deurbelcode {n}","action.timer":"Timer {n} minuten","dyn.decoder_review":"Decoder {protocol}: herkende functies controleren voor opslaan.","dyn.decoder_missing":"Decoderinformatie ontbreekt. RFLink kan de functies niet veilig beoordelen. Vernieuw de pagina en probeer opnieuw.","dyn.dev_instruction":"Houd het testapparaat binnen ongeveer 1 meter. Meet eerst de achtergrond; voer daarna één fysieke actie meerdere keren uit.","dyn.device_instruction":"Houd het apparaat binnen ongeveer 1 meter van de juiste antenne. Druk nog niet op een knop.","dyn.rx_ready":"● RX gereed","dyn.rx_unavailable":"● RX niet beschikbaar","dyn.disconnected":"● Niet aangesloten","dyn.checking":"● Controleren","dyn.rx_ready_short":"RX gereed","dyn.rx_fault":"RX defect","dyn.not_connected":"Niet aangesloten","dyn.checking_short":"Controleren","dyn.radio_unavailable_title":"Radio of ontvanger niet beschikbaar","dyn.no_radios":"Geen radio’s geconfigureerd","dyn.only_working_receivers":"Alleen werkende ontvangers kunnen worden geselecteerd.","dyn.radio_checking":"Radiostatus controleren…","dyn.no_working_receivers":"Geen werkende radio-ontvangers: sluit modules aan en herstart de ESP32.","dyn.all_radios":"Alle radio’s","dyn.overview":"Totaaloverzicht","dyn.radio_disconnected_title":"Radiomodule niet aangesloten","dyn.activity_radio":"Activiteit · {name}","dyn.activity_all":"Activiteit alle radio’s","dyn.unknown":"Onbekend","dyn.no_monitor_activity":"Geen geregistreerde activiteit voor dit filter. Bij ontbrekende radio’s verschijnen geen nieuwe RX- of TX-registraties.","dyn.tx":"Zenden","dyn.available":"Beschikbaar","dyn.unavailable":"Niet beschikbaar","dyn.last_signal":"Laatste signaal","dyn.view_activity":"Bekijk activiteit","dyn.no_activity":"Nog geen activiteit.","dyn.offline":"● Offline","dyn.esp_unreachable":"ESP32 niet bereikbaar","dyn.esp_unreachable_help":"Controleer voeding, netwerk en IP-adres. Er worden nu geen apparaatwijzigingen uitgevoerd.","dyn.api_unreachable":"API niet bereikbaar; starten uitgeschakeld.","dyn.connection_lost":"Verbinding met de ESP32 verbroken.","dyn.manage_signals":"Signalen beheren ({n})","dyn.signal_identity_help":"De interne apparaat-ID {uid} blijft behouden. Vervang een RF-code uitsluitend door een nieuwe code die de ontvanger zelf meerdere keren heeft herkend.","dyn.rf_signal":"RF-signaal {n}","dyn.replace_with":"Vervangen door","dyn.choose_learned":"Kies nieuw ingeleerd signaal","dyn.replace":"Vervang","dyn.unlink":"Ontkoppel","dyn.last_code_title":"Laatste code: verwijder het apparaat of vervang de code","dyn.no_matching_signal":"Geen passend nieuw signaal gevonden. Ga naar Leren, ontvang het nieuwe signaal minimaal drie keer en ga terug zonder het als een nieuw apparaat op te slaan.","dyn.learn_new_signal":"Nieuw signaal inleren","dyn.signal_history_help":"Herkende acties zijn apparaatbreed geregistreerd. Bij protocollen met één RF-identiteit voor meerdere knoppen kan RFLink nog niet betrouwbaar één afzonderlijke knop ontkoppelen. Ontkoppelen van een RF-code wist geen gedeelde actiegeschiedenis.","dyn.mqtt_remove_unavailable":"MQTT offline: verwijderen niet mogelijk","dyn.approve":"Goedkeuren","dyn.remove":"Verwijderen","dyn.approve_technical":"Technische waarde goedkeuren","dyn.review_new_functions":"● {n} nieuwe functie(s) beoordelen","dyn.view_functions":"Functies bekijken en aanpassen","dyn.approved_only_help":"Alleen door jou goedgekeurde functies worden opgeslagen en naar Home Assistant doorgestuurd. Waardeveranderingen binnen bestaande functies gaan wel automatisch.","dyn.found_not_approved":"Gevonden, nog niet goedgekeurd","dyn.no_new_visible":"Geen nieuwe zichtbare functies ter beoordeling.","dyn.approved_features":"Goedgekeurde functies","dyn.none_approved":"Nog geen zichtbare functies goedgekeurd.","dyn.technical_radio_data":"Technische radiogegevens ({saved} bewaard{pending})","dyn.pending_suffix":", {n} ter beoordeling","dyn.technical_radio_help":"Deze gegevens kunnen nodig zijn voor een exacte zendcode. Pas ze alleen aan wanneer je het protocol kent. RFLink gebruikt ze niet als losse sensoren in Home Assistant.","dyn.saved":"Bewaard","dyn.mqtt_connected":"Verbonden","dyn.mqtt_disconnected":"Niet verbonden","dyn.mqtt_not_configured":"Niet ingesteld","dyn.storage_ok":"Opgeslagen apparaten blijven na een herstart behouden. RF-codes wijzigen vereist een bevestigde nieuwe ontvangst.","dyn.storage_uncertain":"Opslagstatus onzeker. Apparaten kunnen niet veilig worden gewijzigd.","dyn.pairing_only":"Apparaten met instelstappen","dyn.device_sub":"Beheer naam, RF-codes en Home Assistant-publicatie per apparaat.","dyn.mqtt_no_features_title":"MQTT niet verbonden of geen goedgekeurde functies","dyn.remove_from_ha":"Uit HA verwijderen","dyn.add_to_ha":"Aan HA toevoegen","dyn.receive_sensor":"Ontvangstsensor","dyn.rf_device":"RF-apparaat","dyn.rf_signals":"{n} RF-signaal(en)","dyn.in_ha":"In Home Assistant","dyn.only_rflink":"Alleen RFLink","dyn.ha_control_allowed":"HA-bediening toegestaan","dyn.features_to_review":"{n} functie(s) te beoordelen","dyn.review_needed":"Beoordeling nodig","dyn.manage_more":"Meer beheren: naam, Home Assistant en RF-codes","dyn.changes_manual":"Wijzigingen worden pas na jouw actie opgeslagen. RFLink blijft leidend.","dyn.mqtt_offline":"MQTT offline","dyn.disable_ha_control":"HA-bediening uitschakelen","dyn.allow_ha_control":"HA-bediening toestaan","dyn.rename":"Naam wijzigen","dyn.dangerous_actions":"Gevaarlijke acties","dyn.dangerous_help":"Hiermee verwijder je het volledige apparaat uit RFLink en indien gekoppeld uit Home Assistant.","dyn.delete_device":"Apparaat definitief verwijderen","dyn.technical_device_id":"Technische apparaat-ID","dyn.no_pair_devices":"Nog geen bevestigde pairing-apparaten.","dyn.no_devices":"Nog geen bevestigde apparaten. Voeg er één toe via Leren.","dyn.new_signal":"Nieuw signaal","dyn.received_times":"{n} keer ontvangen","dyn.ready_review":"Klaar voor beoordeling","dyn.still_listening":"Nog aan het luisteren","dyn.check_signal":"Controleer het ontvangen signaal voordat je het bewaart.","dyn.looks_like":"Lijkt op {name}. Koppel alleen als dit echt hetzelfde apparaat is.","dyn.review_add":"Controleren en toevoegen","dyn.more_technical":"Meer opties en technische gegevens","dyn.rf_identity":"RF-identiteit: {id}","dyn.link_to":"Koppelen aan {name}","dyn.not_saved":"Nog niet opgeslagen","dyn.pairing_view":"Pairing-weergave actief.","dyn.no_new_signals":"Geen nieuwe signalen die aandacht nodig hebben.","dyn.learn_replacement_toast":"Leer een nieuw signaal in; sla het niet als apart apparaat op.","dyn.devices_load_failed":"Apparaten laden mislukt","dyn.confirm_unlink":"RF-code {identity} ontkoppelen? Andere codes van het apparaat blijven werken.","dyn.unlinked":"RF-code ontkoppeld","dyn.unlink_failed":"Ontkoppelen mislukt; er is niets gewijzigd","dyn.choose_confirmed_signal":"Kies eerst een bevestigd nieuw signaal","dyn.confirm_replace":"RF-code vervangen?\nOud: {old}\nNieuw: {new}\nDe apparaatnaam en overige RF-codes blijven behouden.","dyn.replaced":"RF-code vervangen en opgeslagen","dyn.replace_failed":"Vervangen mislukt; oude RF-code behouden","dyn.storage_attention":"Opslag heeft aandacht nodig","dyn.decoder_incomplete":"Decoderbibliotheek niet compleet","dyn.hardware_incomplete":"Hardware niet compleet","dyn.all_ready":"Alles gereed","dyn.storage_problem_help":"De verbinding werkt, maar apparaatwijzigingen kunnen niet veilig worden opgeslagen. Controleer ESPHome logs.","dyn.decoder_problem_help":"Niet alle verwachte decoders zijn geladen. Ontvangen signalen worden mogelijk niet herkend.","dyn.hardware_problem_help":"ESP32 en website werken. {missing} van {total} radio’s zijn niet aangesloten of niet gereed.","dyn.ready_help":"ESP32, decoderbibliotheek en alle geconfigureerde radio’s zijn gereed.","dyn.esp_web":"ESP32 en webinterface","dyn.radios":"Radio’s","dyn.ready_count":"{ready}/{total} gereed","dyn.decoders":"Decoders","dyn.loaded":"{value} geladen","dyn.device_storage":"Apparaatopslag","dyn.check_required":"Controle nodig","dyn.saved_count":"{n} bewaard","dyn.storage_error":"Opslagfout","dyn.confirm_remove_feature":"Functie \"{key}\" verwijderen uit RFLink en, indien gekoppeld, ook uit Home Assistant?","dyn.feature_removed":"Functie verwijderd en opgeslagen","dyn.feature_approved":"Functie goedgekeurd en opgeslagen","dyn.feature_failed":"Functie niet gewijzigd. Controleer MQTT en apparaatopslag.","dyn.confirm_remove_ha":"\"{name}\" uit Home Assistant verwijderen? Het RF-apparaat blijft in RFLink bewaard.","dyn.ha_removed":"HA-discovery verwijderd; apparaat blijft bewaard","dyn.ha_sent":"HA-discovery naar broker verzonden","dyn.ha_failed":"HA-wijziging mislukt: controleer MQTT en opslag","dyn.confirm_disable_tx":"Bediening van \"{name}\" in Home Assistant uitschakelen?","dyn.confirm_enable_tx":"RF-zenden voor \"{name}\" toestaan? Alleen door deze decoder ondersteunde en goedgekeurde knoppen verschijnen in HA. De werkelijke ontvangerstatus wordt niet bevestigd.","dyn.tx_saved":"Zendtoestemming opgeslagen","dyn.tx_failed":"Zendtoestemming niet gewijzigd; controleer MQTT en TX-configuratie","dyn.new_device_name":"Nieuwe apparaatnaam:","dyn.name_length":"Naam moet 1-80 tekens bevatten","dyn.name_saved":"Naam opgeslagen","dyn.rename_failed":"Hernoemen niet opgeslagen","dyn.confirm_delete":"Apparaat \"{name}\" verwijderen uit RFLink en indien actief ook uit Home Assistant? Dit kun je niet ongedaan maken; opnieuw inleren is nodig.","dyn.device_removed":"Apparaat verwijderd","dyn.delete_failed":"Verwijderen mislukt","dyn.add_device":"Apparaat toevoegen","dyn.wizard_start_help":"Kies een radio en klik op Start. Achtergrond meten en luisteren gaan automatisch.","dyn.storage_unavailable":"Apparaatopslag niet beschikbaar","dyn.receiver_unavailable":"Deze ontvanger is niet beschikbaar","dyn.step1":"Stap 1/3 · Achtergrond meten","dyn.step1_help":"Wacht vijf seconden. Druk nog niet op je apparaat.","dyn.step2":"Stap 2/3 · Bedien je apparaat","dyn.step2_help":"Druk enkele keren op dezelfde knop. Zodra het signaal herkend is, kun je hier direct de naam invullen.","dyn.measure_failed":"Meting mislukt","dyn.measure_failed_help":"Controleer of de radio beschikbaar is en probeer opnieuw.","dyn.start_failed":"Starten mislukt","dyn.start_failed_help":"Controleer de radio en probeer opnieuw.","dyn.step3":"Stap 3/3 · Controleer en bewaar","dyn.candidate_received":"{protocol} is {n} keer ontvangen. Geef het apparaat een naam en controleer de gevonden functies.","dyn.observations":"{n} waarnemingen","dyn.link_to_name":"Koppelen aan {name}","dyn.temp_connection":"Verbinding tijdelijk weg","dyn.temp_connection_help":"Opnieuw proberen zodra ESPHome bereikbaar is.","dyn.decoder_safe_disabled":"Decoderinformatie ontbreekt; veilig opslaan is uitgeschakeld","dyn.enter_name":"Geef een naam van 1-80 tekens","dyn.confirm_no_features":"Je hebt geen functies goedgekeurd. Het apparaat wordt alleen in RFLink bewaard. Doorgaan?","dyn.signal_linked":"Signaal gekoppeld. Nieuwe functies worden bij het bestaande apparaat als voorstellen getoond; keuzes uit dit scherm zijn daarop niet toegepast.","dyn.saved_ha_failed":"Apparaat opgeslagen, maar HA-publicatie is mislukt. Je kunt deze later onder Apparaten opnieuw inschakelen.","dyn.device_saved":"Apparaat opgeslagen. Het blijft na herstart bewaard.","dyn.done":"Gereed","dyn.done_help":"Je kunt nog een apparaat toevoegen of je apparaten bekijken.","dyn.save_failed":"Opslaan mislukt; probeer opnieuw","dyn.confirm_link":"Dit signaal koppelen aan {name}?","dyn.modal_invalid":"Controleer decoderinformatie en voer een apparaatnaam van 1-80 tekens in","dyn.confirm_store_only":"Geen functies goedgekeurd. Alleen apparaat bewaren?","dyn.device_added":"Apparaat toegevoegd","dyn.add_failed":"Toevoegen mislukt","dyn.linked_existing":"Aan bestaand apparaat gekoppeld","dyn.link_failed":"Koppelen mislukt","dyn.radio_status_unreachable":"Radiostatus niet bereikbaar.","dyn.measuring_background":"Achtergrond meten…","dyn.waiting_capture":"Wachten op capture","dyn.capture_first":"Maak eerst een capture.","dyn.protocol_unknown":"nog onbekend","dyn.identity_unknown":"nog niet bepaald","dyn.no_code_suggestion":"Nog geen codevoorstel.","dyn.signal_strong":"Sterk / mogelijk te dichtbij","dyn.good":"GOED","dyn.weak":"Zwak","dyn.raw_rssi":"raw: RSSI niet beschikbaar","dyn.capture_found":"Capture gevonden","dyn.measuring":"Meten…","dyn.ready":"Gereed","dyn.background_ready":"Achtergrondmeting gereed","dyn.background_failed":"Achtergrondmeting mislukt","dyn.listen_started":"Luisteren gestart","dyn.listen_failed":"Luisteren starten mislukt","dyn.not_measured":"Nog niet gemeten","dyn.listen_stopped":"Luisteren gestopt; tijdelijke meting gewist","dyn.stop_failed":"Stoppen mislukt","dyn.invalid_brightness":"Ongeldige helderheid","dyn.led_saved":"LED-instellingen opgeslagen","dyn.save_failed_short":"Opslaan mislukt","dyn.protocol_count":"{n} protocollen · details alleen op verzoek","dyn.receive_only":"Alleen ontvangen","dyn.example":"Voorbeeld","dyn.tx_hw_tested":"Zenden: hardwaregetest","dyn.tx_sw_tested":"Zenden: softwaretest","dyn.itho_separate":"Native bediening","dyn.tx_blocked":"Zenden geblokkeerd","dyn.tx_unavailable":"Zenden niet beschikbaar","dyn.rx_decoder_present":"Ontvangstdecoder aanwezig (hardware niet bevestigd)","dyn.in_ha_label":"In Home Assistant:","dyn.what_to_know":"Wat moet ik weten?","dyn.software_prefix":"Software: ","dyn.encoder_registered":"zendencoder geregistreerd; ","dyn.manifest_verified":"hardwarestatus in manifest: geverifieerd","dyn.host_only":"hostgetest, fysieke radiotest ontbreekt","dyn.no_generic_encoder":"geen generieke zendencoder","dyn.only_approved":"RFLink toont alleen goedgekeurde functies.","dyn.technical_data":"Technische gegevens","dyn.actions":"Acties","dyn.fields":"Velden","dyn.none":"geen","dyn.learn":"Inleren","dyn.no_protocols":"Geen protocollen in dit filter.","dyn.protocol_detection_note":"RFLink herkent het daadwerkelijke protocol pas wanneer het signaal wordt ontvangen. Dit is een instructie, geen protocolfilter.","dyn.no_decoders":"Geen decoders geregistreerd.","dyn.decoder_registry_unavailable":"Decoderregistratie niet beschikbaar.","dyn.decoder_library_unavailable":"Decoderbibliotheek niet beschikbaar. Inleren en goedkeuren is tijdelijk uitgeschakeld.","placeholder.device_name":"Bijv. Buitensensor tuin","field.temperature":"Temperatuur","field.humidity":"Luchtvochtigheid","field.battery_low":"Batterij bijna leeg","field.rain":"Neerslag","field.fan_speed":"Ventilatorsnelheid","protocol.ExampleSwitch.band":"Geen echte radio","protocol.ExampleSwitch.instruction":"Dit is een codevoorbeeld en kan geen echte signalen inleren.","protocol.ExampleSwitch.ha_result":"Geen Home Assistant-apparaat.","protocol.ExampleSwitch.warning":"Alleen zichtbaar bij technische details; niet bedoeld voor dagelijks gebruik.","protocol.IthoRFT.band":"868,30 MHz","protocol.IthoRFT.instruction":"Druk herhaaldelijk op één stand- of timerknop van je Itho-afstandsbediening.","protocol.IthoRFT.ha_result":"Bevestigde stand-/timergebeurtenissen en eventueel ondersteunde metingen.","protocol.IthoRFT.warning":"RFLink zendt met een eigen virtuele Itho-afstandsbediening. Koppel die één keer met de ventilatie-unit voordat je de bediening gebruikt.","protocol.Acurite609TXC.band":"433,92 MHz","protocol.Acurite609TXC.instruction":"Wacht tot dezelfde sensormelding meerdere keren is ontvangen.","protocol.Acurite609TXC.ha_result":"Bevestigde temperatuur, luchtvochtigheid en eventueel batterijmelding.","protocol.Acurite609TXC.warning":"Na batterijwissel kan de RF-identiteit veranderen; een nieuwe identiteit niet automatisch koppelen.","protocol.FineOffsetWH2.band":"433,92 MHz","protocol.FineOffsetWH2.instruction":"Wacht op meerdere berichten van de weersensor.","protocol.FineOffsetWH2.ha_result":"Bevestigde temperatuur en luchtvochtigheid.","protocol.FineOffsetWH2.warning":"Een ontbrekende meetwaarde betekent niet dat die functie moet worden verwijderd.","protocol.Nexa.band":"433 MHz","protocol.Nexa.instruction":"Druk meerdere keren op Aan en daarna op Uit van hetzelfde kanaal.","protocol.Nexa.ha_result":"Je ziet de bevestigde Aan/Uit-gebeurtenissen. Bedieningsknoppen verschijnen pas na aparte zendtoestemming.","protocol.Nexa.warning":"Een afstandsbediening en de lamp zijn niet automatisch hetzelfde apparaat; meerdere kanalen nog apart inleren.","protocol.ByronSX.band":"433 MHz","protocol.ByronSX.instruction":"Druk enkele keren op de deurbelknop en wacht tot RFLink hetzelfde signaal herkent.","protocol.ByronSX.ha_result":"Een belgebeurtenis; een zendknop pas na afzonderlijke toestemming.","protocol.ByronSX.warning":"Een uitzendopdracht bevestigt niet dat de bel werkelijk is afgegaan.","protocol.Dooya.band":"433 MHz","protocol.Dooya.instruction":"Druk dezelfde knop herhaaldelijk in. Leer andere knoppen daarna apart aan als functievoorstel.","protocol.Dooya.ha_result":"Goedgekeurde knopgebeurtenissen; zendknoppen alleen na aparte toestemming.","protocol.Dooya.warning":"Knopnummers zijn nog geen vastgelegde betekenis zoals Omhoog of Stop. Wijs die niet automatisch toe.","protocol.KeeLoq.band":"433 MHz","protocol.KeeLoq.instruction":"Druk dezelfde knop meerdere keren in en controleer of dezelfde afstandsbediening wordt herkend.","protocol.KeeLoq.ha_result":"Ontvangen knopgebeurtenissen en batterijmelding als die wordt gedecodeerd.","protocol.KeeLoq.warning":"Rolling-code is beveiligd; ontvangen berichten mogen niet worden heruitgezonden.","protocol.RCSwitch.band":"433 MHz","protocol.RCSwitch.instruction":"Druk één knop meerdere keren in. Leer een andere vaste code apart in en koppel hem zelf aan het gewenste apparaat.","protocol.RCSwitch.ha_result":"Eén gebeurtenis voor de herkende code; na toestemming eventueel een zendknop.","protocol.RCSwitch.warning":"Code, protocol en bitlengte moeten overeenkomen. Aan- en uitcodes zijn niet automatisch gekoppeld.","protocol.AlectoV1.band":"433,92 MHz","protocol.AlectoV1.instruction":"Wacht op meerdere uitzendingen van de weersensor; druk niet op willekeurige knoppen.","protocol.AlectoV1.ha_result":"Alleen door jou bevestigde metingen zoals temperatuur, vocht en eventueel neerslag.","protocol.AlectoV1.warning":"Niet iedere variant zendt alle metingen. Een ontbrekende waarde is niet automatisch een defect.","protocol.SomfyRTS56.band":"433,42 MHz","protocol.SomfyRTS56.instruction":"Druk herhaaldelijk op dezelfde knop van je Somfy-afstandsbediening. Gebruik de programmeerknop niet voor deze ontvangsttest.","protocol.SomfyRTS56.ha_result":"Ontvangen acties kunnen worden gevolgd. Na het koppelen van RFLink zijn ook goedgekeurde Omhoog-, Omlaag- en My-bedieningen beschikbaar.","protocol.SomfyRTS56.warning":"Zenden gebruikt een aparte virtuele RFLink-afstandsbediening met een eigen rolling counter. Koppel RFLink eerst met de motor; fysieke motortest staat nog open.","protocol.RubicsonTemp.band":"433,92 MHz","protocol.RubicsonTemp.instruction":"Wacht op meerdere berichten van de temperatuursensor.","protocol.RubicsonTemp.ha_result":"Bevestigde temperatuur en eventueel batterijmelding.","protocol.RubicsonTemp.warning":"Kanaal en controlesom zijn technische gegevens en geen losse Home Assistant-sensoren.","decoder.select_hint":"Klik op een decoder om alle informatie te bekijken.","decoder.how_to_learn":"Hoe inleren","decoder.why_tx":"Waarom verzenden zo werkt","decoder.what_needed":"Wat is nog nodig","decoder.reason_sensor":"Dit protocol is een alleen-ontvangen sensor. Verzenden staat bewust uit.","decoder.reason_example":"Dit is een ontwikkelvoorbeeld en geen echt RF-protocol.","decoder.reason_tx_ready":"Voor dit protocol is een gecontroleerde zend-encoder beschikbaar.","decoder.reason_tx_host":"De zend-encoder bestaat en slaagt voor softwaretests, maar is nog niet met echte RF-hardware gevalideerd.","decoder.reason_itho":"Ontvangst en native packet-TX zijn geïntegreerd. RFLink gebruikt per ingeleerd Itho-apparaat een aparte virtuele zenderidentiteit.","decoder.reason_somfy":"Ontvangen en zenden zijn geïmplementeerd. RFLink bedient Somfy als een aparte virtuele afstandsbediening met een eigen rolling counter.","decoder.reason_keeloq":"Ontvangen kan ondersteunde frames herkennen, maar generiek verzenden is geblokkeerd door KeeLoq rolling-codeversleuteling.","decoder.reason_no_encoder":"Ontvangstondersteuning bestaat, maar er is nog geen gecontroleerde zend-encoder geregistreerd.","decoder.needed_none":"Niets; alleen ontvangen is hier het bedoelde gedrag.","decoder.needed_hw_test":"Een echte RF-test met het apparaat en vergelijking van captures voordat TX hardware-gevalideerd is.","decoder.needed_itho":"Koppel RFLink één keer met de ventilatie-unit en test daarna de bediening op echte hardware.","decoder.needed_somfy":"Koppel de virtuele RFLink-afstandsbediening met de motor. Fysieke validatie met een echte motor staat nog open.","decoder.needed_keeloq":"Een ondersteund product plus geldige cryptografische sleutel/pairingmethode. Dit is niet veilig uit normale RF-captures af te leiden.","decoder.needed_encoder":"Een protocolspecifieke zend-encoder plus software- en hardwarevalidatie.","library.title":"Bibliotheek & updates","library.help":"Officiële firmware-items en je eigen items blijven gescheiden. RFLink vervangt je wijzigingen nooit zonder bevestiging.","library.official_decoders":"Officiële decoders","library.my_decoders":"Mijn decoders","library.my_profiles":"Mijn deviceprofielen","library.needs_attention":"Aandacht nodig","library.review":"Wijzigingen bekijken","library.advanced":"Geavanceerd","library.hide_advanced":"Geavanceerd verbergen","library.advanced_help":"Technische bibliotheekinformatie voor diagnose en ontwikkeling.","library.schema":"Bibliotheekschema","library.user_storage":"Persistente gebruikersbibliotheek","library.safe_resolutions":"Veilig automatisch op te lossen","library.archive":"Gearchiveerde lokale items","library.enabled":"Ingeschakeld","library.scaffold":"Fundering gereed; persistente gebruikersbibliotheek nog niet ingeschakeld in deze build","library.status_attention":"Eén of meer lokale items vragen om een keuze. Je huidige configuratie blijft actief totdat je een wijziging bevestigt.","library.status_ok":"Alles is bijgewerkt. Er is geen actie nodig.","library.status_scaffold":"De update- en conflictlogica is gereed. Persistente eigen decoders en profielen zijn de volgende opslaglaag.","library.attention_title":"Bibliotheekupdate vraagt aandacht","library.attention_count":"{n} lokaal item(s) vragen om een keuze. Er is niets automatisch vervangen.","library.unavailable":"De bibliotheekstatus is tijdelijk niet beschikbaar.","library.review_pending":"Keuzes worden actief zodra de persistente gebruikersbibliotheek is ingeschakeld.","library.no_items":"Geen bibliotheekitems gemeld.","library.source":"Bron","library.source_builtin":"In firmware","library.source_user":"Gebruiker","library.stable_id":"Vaste ID","library.version":"Versie","library.pack":"Decoderpakket","help.library_title":"Wat is de bibliotheek?","help.library_text":"De bibliotheek combineert officiële firmwaredecoders met optionele zelfgemaakte decoders en deviceprofielen. Officiële en lokale items blijven gescheiden.","help.library_more":"Na een update kan RFLink een officiële versie aanraden. Lokale wijzigingen worden nooit stilletjes overschreven.","help.source_title":"Wat betekent Bron?","help.source_text":"In firmware betekent dat het item met de firmware wordt geleverd. Gebruiker betekent dat het op dit RFLink-apparaat is geleerd of gemaakt.","help.source_more":"De bron helpt RFLink bepalen wat veilig kan worden bijgewerkt en wanneer jouw bevestiging nodig is.","help.id_title":"Wat is een Vaste ID?","help.id_text":"Een Vaste ID is de blijvende identiteit van een decoder of profiel over firmwareversies heen.","help.id_more":"RFLink gebruikt deze samen met versies en inhoudsvingerafdrukken om duplicaten, updates en lokale wijzigingen te herkennen.","user.heading":"Eigen protocol","user.guide":"Geef het protocol een naam, benoem de knop of actie die je opneemt en druk die minimaal drie keer in. Technische regels staan onder Geavanceerd.","user.name":"Naam decoder","user.id":"Vaste ID","user.action":"Knop/actie","user.capture":"Neem deze actie op","user.clear":"Wis opnames","user.create":"Test en activeer decoder","user.advanced":"Geavanceerd: RX- en TX-regels bewerken","user.advanced_help":"RX-regels herkennen signalen. TX-regels maken een voorbeeld. Opslaan is transactioneel: een werkende actieve decoder blijft actief als gewijzigde regels de controle niet halen.","user.actions":"Acties, gescheiden door komma's","user.save_rules":"Regels opslaan","user.vm_help":"Regels draaien in een begrensde interpreter zonder eigen code. Zie USER_DECODER_GUIDE.md voor de syntax.","user.firmware":"Omzetten naar firmwaredecoder","user.firmware_help":"Exporteer de bibliotheek en voer het meegeleverde evaluatie- en mergescript uit op je computer.","user.export":"Download evaluatie-export","user.saved":"Eigen protocollen en apparaatprofielen","user.profile":"Apparaatprofiel","user.profile_help":"Geef een decoder een herkenbare modelnaam en inleerinstructie. Technische velden zijn optioneel.","user.profile_name":"Profielnaam","user.profile_id":"Profiel-ID","user.decoder":"Decoder","user.instructions":"Inleerhulp","user.save_profile":"Profiel opslaan","user.samples":"opnames","user.active":"actief","user.inactive":"inactief","user.preview_ready":"TX-voorbeeld beschikbaar","user.rx_only":"Alleen ontvangen","user.edit":"Bewerken","user.disable":"Uitschakelen","user.activate":"Activeren","user.remove":"Verwijderen","user.empty":"Nog geen eigen decoders opgeslagen.","user.unavailable":"Eigen bibliotheek niet beschikbaar.","user.activated":"Decoder geactiveerd.","user.disabled":"Decoder uitgeschakeld.","user.confirm_remove":"Dit item verwijderen?","user.no_samples":"Nog geen opnames.","user.check_radio_action":"Kies een werkende radio en geldige actienaam.","user.press_three":"Druk minstens drie keer op deze knop en neem daarna de volgende actie op.","user.cleared":"Opnames gewist.","user.check_id_name":"Vul een decodernaam in. Als je de stabiele ID handmatig hebt gewijzigd, moet die met user_ beginnen.","user.created_tx":"Decoder actief. TX-voorbeeld is klaar; keur elk apparaat goed vóór zenden.","user.created_rx":"Decoder actief voor ontvangst. Voeg zo nodig een TX-regel toe onder Geavanceerd.","user.advanced_saved":"Regels opgeslagen, getest en geactiveerd.","user.profile_saved":"Profiel opgeslagen.","user.exported":"Evaluatie-export gedownload.","user.device_controls":"Eigen decoder en apparaatprofiel","user.tx_notice":"Een voorbeeld zendt niets uit. Zenden vereist toestemming voor dit apparaat; een opdracht is geen ontvangstbevestiging.","user.no_profile":"Geen profiel","user.revoke_tx":"Zenden stoppen","user.allow_tx":"Zenden toestaan","user.preview":"TX-voorbeeld","user.send":"Zenden","user.confirm_allow":"Mag dit apparaat signalen sturen met de eigen decoder?","user.confirm_send":"Deze RF-actie zenden","user.sent_request":"TX-opdracht aangeboden; geen bevestiging van het doelapparaat.","user.import":"Bibliotheekback-up herstellen","user.import_help":"Elke decoder wordt vóór vervanging gecontroleerd. TX-toestemming wordt nooit automatisch hersteld; protocoltellers kunnen alleen vooruit.","user.import_invalid":"Dit is geen geldige RFLink-back-up van de gebruikersbibliotheek.","user.import_confirm":"{decoders} decoder(s) en {profiles} profiel(en) herstellen? Bestaande ID’s worden alleen na succesvolle controle vervangen.","user.imported":"Herstel gereed: {decoders} decoder(s), {profiles} profiel(en), {bindings} koppeling(en) met bestaande apparaten. TX-toestemmingen zijn niet hersteld.","user.analysis_waiting":"Neem voorbeelden op om stabiele en veranderende RF-velden te vergelijken.","user.analysis_need_more":"{action}: neem nog minimaal één voorbeeld op om te vergelijken.","user.analysis_length":"{action}: de opgenomen frames hebben verschillende lengtes; controleer dit protocol handmatig.","user.analysis_dynamic":"{action}: {count} symbolen; veranderende posities: {positions}. Dit kan een teller, checksum, ID of knopafhankelijke data zijn.","user.analysis_fixed":"{action}: {count} symbolen; stabiel in de opgenomen voorbeelden.","user.radio_profile":"Radioprofiel","user.radio_profile_help":"Deze release gebruikt de gevalideerde radio-instellingen van het board; eigen CC1101-modulatie/bitrate kan nog niet vanuit user recipes worden ingesteld.","user.analysis_counter":"{action}: positie(s) {positions} lijken op een oplopende teller. Automatisch herhalen blijft uitgeschakeld wanneer waarden veranderen.","user.analysis_checksum":"{action}: positie(s) {positions} lijken op een eenvoudige checksum-kandidaat. Controleer dit voordat je een Advanced-regel maakt.","library.official_profiles":"Officiële apparaatprofielen","decoder.known_profiles":"Bekende apparaatprofielen","decoder.known_profiles_help":"Modelfamilies die in openbare fabrikant-/protocolbronnen bij deze decoder horen. Dit is compatibiliteitsinformatie uit bronnen, geen fysieke hardwarecertificering.","decoder.models":"Modellen / aliassen","decoder.profile_learn":"Zo leer je dit profiel in","decoder.hardware_pending":"Hardwarevalidatie op dit RFLink-board nog nodig","ui.manage_devices":"Apparaten beheren","ui.manage_devices_help":"Hernoem apparaten, beoordeel nieuwe functies en beheer Home Assistant.","ui.settings_sub":"Eerst de gewone instellingen. Bibliotheek en technische hulpmiddelen staan verder naar beneden.","devices.pairing_filter":"Apparaten met instelstappen","devices.show_all":"Alle apparaten tonen","devices.waiting_signals":"Signalen die nog beoordeeld moeten worden","devices.waiting_signals_help":"Deze signalen zijn buiten de begeleide flow Apparaat toevoegen herkend. Beoordeel alleen wat je wilt bewaren.","placeholder.wizard_device_name":"bijv. Afstandsbediening woonkamer","placeholder.profile_name":"bijv. Afstandsbediening woonkamer","placeholder.profile_help":"bijv. Druk elke knop drie keer in","accessibility.led_brightness":"RGB-helderheid","accessibility.close":"Sluiten","dyn.online":"Online","dyn.frames_label":"Signalen","dyn.device_sub_simple":"Je opgeslagen RF-apparaten. Open een apparaat alleen als je iets wilt wijzigen.","led.boot":"Opstarten","led.connecting":"Verbinden met wifi","led.ready":"Gereed","led.offline":"Verbinding verloren","led.radio_missing":"Radio ontbreekt of is niet gereed","led.learning":"Inleren","led.capture":"Signaal ontvangen","led.learned":"Apparaat bevestigd","led.itho_pair":"Itho-koppelopdracht","led.tx":"RF-signaal verzenden","led.ota":"Firmware-update","led.error":"Fout","led.unknown":"Onbekend","decoder.compatibility_details":"Compatibiliteit en bronnen","decoder.control":"Bediening","decoder.next_step":"Nog nodig","user.saved_help":"Alleen nodig als RFLink je apparaat nog niet kent of als je zelf een modelprofiel wilt maken.","user.profile_advanced":"Geavanceerde profielopties","user.profile_auto_help":"Laat de ID leeg om hem automatisch uit de profielnaam te maken. Acties worden waar mogelijk vanuit de decoder ingevuld.","settings.developer_info":"Ontwikkelaarsinformatie","placeholder.decoder_name":"bijv. Afstandsbediening woonkamer","user.id_auto_help":"Laat dit leeg bij een nieuwe decoder; RFLink maakt automatisch een stabiele ID uit de naam. Wijzig hem alleen als je bewust een specifieke ID nodig hebt.","settings.mqtt_setup":"MQTT instellen","settings.led_simple_help":"Pas de helderheid aan of zet de groene gereed-puls uit. Waarschuwingskleuren blijven actief.","settings.led_more":"LED-kleuren en gedrag","ui.advanced":"Geavanceerd","ui.advanced_details":"Geavanceerde details","ui.advanced_filters":"Geavanceerde filters","ui.try_again":"Opnieuw proberen","ui.decoder_advanced_help":"Neem ruwe radiogegevens op en vergelijk ze.","ui.freq_43392":"Meest gebruikt voor afstandsbedieningen, sensoren en schakelaars","ui.freq_43342":"Vaak gebruikt door Somfy RTS","ui.freq_868":"Vaak gebruikt door Itho en andere 868 MHz-apparaten","ui.freq_other":"Gebruik deze als je apparaat op deze frequentie werkt","ui.frequency_help":"Niet zeker? 433,92 MHz is meestal de beste eerste keuze.","ui.device_found_help":"RFLink heeft het apparaat gevonden. Geef het een naam en sla het op.","ui.protocol_label":"Protocol","ui.signal_seen":"Signaal ontvangen","ui.back_add_device":"← Terug naar Apparaat toevoegen","somfy.control":"Somfy-bediening","somfy.tx_disabled":"Zenden staat uit in de firmware-instellingen. Schakel TX-bediening in voordat je koppelt.","somfy.pair_help":"Zet de motor met een bestaande afstandsbediening in programmeermodus en koppel daarna RFLink.","somfy.pair":"RFLink koppelen","somfy.control_ready":"Somfy-bediening klaar om te testen","somfy.up":"Omhoog","somfy.my":"My / Stop","somfy.down":"Omlaag","somfy.no_ack":"RFLink kan de reactie van de motor niet via de ether bevestigen. Reageert de motor niet, koppel dan opnieuw.","somfy.pair_again":"Opnieuw koppelen","somfy.pair_confirm":"Zet de motor eerst in programmeermodus. Nu de RFLink PROG-opdracht versturen?","somfy.pair_sent":"Koppelopdracht verstuurd","somfy.pair_failed":"Koppelen mislukt","somfy.command_sent":"Opdracht verstuurd","somfy.command_failed":"Opdracht mislukt","somfy.state":"Virtuele afstandsbediening {address} · volgende rolling code {counter}","led.pairing":"Koppelen","itho.control":"Itho-bediening","itho.tx_disabled":"Zenden staat uit in de firmware-instellingen. Schakel TX-bediening in voordat je koppelt.","itho.pair_help":"Schakel de ventilatie-unit ongeveer 15 seconden uit en weer in. Koppel RFLink daarna binnen 2 minuten.","itho.pair":"RFLink koppelen","itho.control_ready":"Itho-bediening klaar om te testen","itho.low":"Laag","itho.medium":"Midden","itho.high":"Hoog","itho.timer10":"Timer 10 min","itho.timer20":"Timer 20 min","itho.timer30":"Timer 30 min","itho.standby":"Stand-by","itho.no_ack":"RFLink kan niet rechtstreeks bevestigen dat de unit de koppeling heeft opgeslagen. Reageert hij niet, probeer opnieuw of gebruik de compatibiliteitskoppeling.","itho.compat_help":"Werkt normaal koppelen niet? Probeer dan de compatibiliteitskoppeling.","itho.pair_again":"Opnieuw koppelen","itho.compat_pair":"Compatibiliteitskoppeling","itho.unpair":"RFLink afmelden","itho.pair_confirm":"Schakel de unit eerst ongeveer 15 seconden uit en weer in. Nu de RFLink-koppelopdracht versturen?","itho.compat_confirm":"Schakel de unit eerst ongeveer 15 seconden uit en weer in. Nu de compatibiliteitskoppeling versturen?","itho.unpair_confirm":"Zet de unit eerst opnieuw in de aanmeldmodus. RFLink nu afmelden?","itho.pair_sent":"Koppelopdracht verstuurd","itho.pair_failed":"Koppelen mislukt","itho.unpair_sent":"Afmeldopdracht verstuurd","itho.unpair_failed":"Afmelden mislukt","itho.command_sent":"Opdracht verstuurd","itho.command_failed":"Opdracht mislukt","itho.state":"Virtuele afstandsbediening {address} · volgende teller {sequence} · type {type}","diag.checking":"Controleren","diag.cc1101_failed":"CC1101 niet gevonden","diag.rx_failed":"CC1101 OK · RX-fout","diag.tx_failed":"CC1101 OK · TX-fout","diag.ok":"Gereed","diag.spi_ok":"Gevonden / geïnitialiseerd","diag.spi_failed":"Initialisatie mislukt","diag.cc1101_spi":"CC1101 / SPI","diag.rx_path":"Ontvangstpad","diag.tx_path":"Zendpad","diag.ready":"Gereed","diag.failed":"Mislukt","diag.configured_pins":"Ingestelde pinnen","diag.hardware":"Hardware","diag.cc1101_hint":"De CC1101 zelf faalt vóór ontvangen of zenden. Controleer 3,3 V, GND, CSN en de gedeelde SCK/MOSI/MISO-bedrading. GDO is hierbij niet de eerste verdachte.","diag.spi_probe":"Alleen-lezen SPI-test","diag.probe_response":"Geldige respons","diag.probe_no_response":"Geen geldige respons","diag.spi_checks":"SPI-controles","diag.spi_failures":"SPI-fouten","diag.spi_consecutive":"Opeenvolgende fouten","diag.spi_recovery_attempts":"Herstelpogingen","diag.spi_recovery_successes":"Geslaagde herstellingen","diag.spi_health":"Digitale SPI-status","diag.spi_health_note":"Software kan ongeldige SPI-antwoorden detecteren, maar geen elektrische reflecties, ringing of overshoot. Gebruik daarvoor een oscilloscoop of logic analyzer.","monitor.show_noise":"Toon vermoedelijke RF-ruis","monitor.noise_help":"Vermoedelijke ruis is standaard verborgen, maar blijft beschikbaar voor diagnose en export.","monitor.copy":"Kopiëren","monitor.export":"Exporteren","monitor.copied":"Zichtbare activiteit gekopieerd.","monitor.exported":"Activiteitsexport gedownload.","monitor.noise_badge":"Vermoedelijke RF-ruis","monitor.noise_reason":"Reden","diag.noise_frames":"Vermoedelijke ruis","diag.freq_offset":"Frequentie-afwijking","diag.uptime":"Uptime","diag.boot_count":"Geregistreerde starts","diag.reset_reason":"Laatste resetreden","diag.boot_log":"Recente herstarts","diag.boot_log_hint":"Bewaard op de ESP32, zonder tijdstempels (bij opstart is geen klok gegarandeerd). Nieuwste eerst.","diag.log_download":"Diagnostiek exporteren","diag.heap_free":"Vrij RAM","diag.heap_min":"Laagste vrije RAM","diag.boot_unavailable":"Herstartgeschiedenis niet opgeslagen in flash","diag.reboot_observed":"Herstart gezien terwijl deze pagina openstond","diag.counter_reset":"Signaalteller gedaald zonder ESP32-herstart","diag.since_boot":"Signaaltellers gelden sinds de laatste start","diag.no_events":"Geen gebeurtenissen gezien terwijl deze pagina openstond","about.heading":"Over RFLink Universal","about.version":"Firmwareversie","about.revision":"Geïnstalleerde revisie","about.build_more":"Buildinformatie","about.built":"Gecompileerd","rxlog.title":"Ontvangstlog","rxlog.signals":"Signalen","rxlog.noise":"Ruis","rxlog.all":"Alles","rxlog.radio":"Radio","rxlog.all_radios":"Alle radio's","rxlog.settings":"Logginginstellingen","rxlog.signal_limit":"Maximaal signalen","rxlog.noise_limit":"Maximaal ruis (alle radio's)","rxlog.noise_floor":"Ruisbescherming per radio (zacht minimum)","rxlog.memory":"Loggeheugen","rxlog.download":"Download logs (ZIP)","rxlog.saved":"Opgeslagen zonder herstart","rxlog.memory_eviction":"Bij geheugentekort wordt eerst ruis verwijderd","rxlog.type":"Logtype","rxlog.settings_help":"Wijzig de limieten tijdens gebruik. Instellingen blijven bewaard. Bij vol geheugen verdwijnt ruis als eerste.","rxlog.radio0":"433,42 MHz signalen","rxlog.radio1":"433,92 MHz signalen","rxlog.radio2":"868,30 MHz signalen","rxlog.save":"Loginstellingen opslaan","rxlog.older":"Oudere gebeurtenissen laden","rxlog.stored":"Opgeslagen","rxlog.noise_info":"Ruis wordt gezamenlijk opgeslagen, maar kan per radio gefilterd worden.","rxlog.invalid":"Ongeldige limieten (maximaal 4096 samen).","rxlog.exporting":"Volledige logexport voorbereiden…","rxlog.exported":"ZIP gedownload","rxlog.token_error":"Geen schrijfrechten beschikbaar","settings.general":"Algemeen & statuslamp","settings.integrations":"Home Assistant & systeem","settings.additional":"Meer instellingen & hulpmiddelen","settings.dashboard_hint":"Veelgebruikte instellingen direct in beeld. Open alleen wat je nodig hebt.","nav.back_settings":"← Terug naar instellingen","nav4.search_devices":"Zoek opgeslagen apparaten","nav4.search_placeholder":"Naam, protocol of radio…","nav4.unknown_radio":"Onbekende radio","nav4.clear":"Wissen","nav4.found":"{visible} van {total} apparaten zichtbaar","nav4.no_results":"Geen apparaten gevonden. Pas de filters aan of wis ze.","nav4.mqtt_host":"Brokeradres","nav4.mqtt_port":"Poort","nav4.mqtt_user":"Gebruikersnaam","nav4.mqtt_pass":"Wachtwoord (leeg = behouden)","nav4.mqtt_enable":"MQTT inschakelen","nav4.mqtt_save":"Opslaan en verbinden","nav4.mqtt_not_built":"MQTT zit niet in deze firmware. Activeer de optionele package één keer.","nav4.mqtt_security":"Opgeslagen op het apparaat. Alleen via een vertrouwd netwerk; standaard NVS is niet versleuteld.","nav4.mqtt_saved":"MQTT-instellingen opgeslagen. Verbinden…","nav4.mqtt_bad":"Ongeldige MQTT-instellingen of opslagfout","devices.radio_count":"{n} apparaten","profileflow.choose":"Apparaatprofiel","profileflow.none":"Kies een profiel","profileflow.suggested":"Voorgesteld","profileflow.select_help":"Kies een profiel om de bijbehorende functies te zien. De exacte RF-identiteit is nog niet bevestigd.","profileflow.manual_warning":"Mogelijke overeenkomst, maar het radiosignaal bewijst het merk of exacte model niet. Controleer het profiel.","profileflow.proposal_warning":"Voorgesteld op basis van protocol en ontvangen acties; bevestig zelf het apparaatmodel.","profileflow.user_choice":"Profiel handmatig gekozen.","profileflow.after_profile":"Bekende bedieningen","profileflow.sensor_profile":"Sensorprofiel: meetwaarden verschijnen wanneer ze worden ontvangen.","profileflow.choose_first":"Kies eerst een apparaatprofiel om de functies te zien.","profileflow.measurements":"Meetwaarden","profileflow.sensor_info":"Meetwaarden verschijnen automatisch zodra ze binnenkomen. Er zijn geen zendknoppen nodig.","profileflow.operation":"Bedienen","profileflow.no_commands":"Voor dit profiel zijn geen bedieningscommando’s bekend.","profileflow.tx_disabled":"Knoppen als voorbeeld: RF-verzending voor dit protocol is uitgeschakeld.","profileflow.pair_first":"Koppel eerst de RFLink-zender om deze knoppen te gebruiken.","profileflow.not_ready":"Bediening nog niet beschikbaar. De TX-encoder en radio moeten gereed zijn; alleen inleren schakelt zenden niet in.","profileflow.tx_ready":"Bediening beschikbaar. Een verzonden commando is nog geen bevestiging van het apparaat.","profileflow.functions_advanced":"Ontvangen functies","profileflow.saved":"Profiel opgeslagen","profileflow.save_error":"Profiel opslaan mislukt","profileflow.send_confirm":"{command} verzenden?","profileflow.sent":"Zendopdracht aangeboden","profileflow.send_error":"Verzenden mislukt","profileflow.command.auto":"Auto","profileflow.command.low":"Laag","profileflow.command.medium":"Midden","profileflow.command.high":"Hoog","profileflow.remove_tx_locked":"Schakel de zendfunctie van dit apparaat uit voordat je technische gegevens verwijdert.","profileflow.dismiss_candidate":"Voorstel verwijderen","profileflow.dismiss_confirm":"Dit nog niet opgeslagen signaal verwijderen? Het kan opnieuw verschijnen wanneer het wordt uitgezonden.","profileflow.dismissed":"Voorstel verwijderd","profileflow.dismiss_failed":"Voorstel verwijderen mislukt","profileflow.setup":"Stap voor stap instellen","profileflow.setup_rx_only":"Dit is begeleiding voor herkenning en ontvangst. Koppelen en zenden via RFLink zijn pas beschikbaar na hardwarevalidatie.","profileflow.command.away":"Afwezig","profileflow.command.timer_15":"15 min","profileflow.command.timer_30":"30 min","profileflow.command.timer_60":"60 min","profileflow.command.bypass_auto":"Bypass auto","profileflow.command.bypass_open":"Bypass openen","profileflow.command.bypass_close":"Bypass sluiten","catalog.protocols":"Protocollen","catalog.profiles":"Apparaatprofielen","catalog.search_placeholder":"Zoek merk, model of protocol…","catalog.all_brands":"Alle merken","catalog.all_families":"Alle families","catalog.all_protocols":"Alle protocollen","catalog.profile_uses_protocol":"Decoder","catalog.show_related":"Bekijk apparaatprofielen","catalog.select_profile":"Kies een apparaatprofiel om de details te bekijken.","catalog.custom_brand":"Eigen","catalog.custom_family":"Eigen apparaatprofielen","catalog.protocol":"Protocol","catalog.no_help":"Geen inleerinstructies opgegeven.","catalog.operation":"Bedieningsstatus","catalog.tx_profile_warning":"Opdrachten vereisen een geschikte encoder, koppeling en geteste radio.","catalog.tx_not_ready":"De functies zijn bekend, maar verzenden is voor dit profiel nog niet beschikbaar.","catalog.rx_only":"Alleen ontvangen; geen zendknoppen.","catalog.profile_functions":"Functies van dit profiel","catalog.received_actions":"Ontvangen acties","catalog.setup_steps":"Installatiestappen","catalog.profile_source":"Profielbron","catalog.profile_protocol_note":"Profielen beschrijven apparaten; decoders verwerken RF-signalen.","catalog.profile_count":"{shown} van {total} apparaatprofielen","catalog.no_results":"Geen apparaatprofielen gevonden met deze filters.","action.auto":"Auto","action.away":"Afwezig","wizard.setup_title":"Apparaat instellen","wizard.start":"Stap voor stap instellen","wizard.step":"Stap","wizard.close":"Sluiten","wizard.back":"Vorige","wizard.next":"Verder","wizard.finish":"Klaar (ontvangst)","wizard.check":"Reactie controleren","wizard.rx_only":"Verder zonder zenden","wizard.transmit":"Koppelcommando verzenden","wizard.identity_help":"Controleer of de ontvangen RF-identiteit bij dit apparaat of de bestaande afstandsbediening hoort. Kies het juiste model.","wizard.finish_help":"Controleer het resultaat. Dat RFLink iets verzendt, bewijst niet dat het apparaat gekoppeld is of reageerde.","wizard.observe_help":"Ontvang een signaal en controleer de identiteit.","wizard.choose_profile":"Kies eerst een apparaatprofiel.","wizard.rx_ready":"Ontvangst is gereed. Dit profiel heeft geen koppeling nodig.","wizard.sent_not_verified":"Koppelcommando uitgezonden; geen antwoord van de ontvanger bevestigd.","wizard.not_supported":"Koppelen wordt nog niet ondersteund voor dit apparaat.","wizard.requires_setup":"Instructies beschikbaar; koppeling nog niet bevestigd.","wizard.orcon_tx_missing":"Orcon: de RF-zender en RAMSES-II 1FC9-handshake zijn nog niet op de hardware gevalideerd. De wizard kan daarom nog niet koppelen of de bediening activeren.","wizard.tx_unavailable":"De geteste encoder of fysieke zender is nog niet beschikbaar. RFLink blijft alleen ontvangen.","wizard.verify_observed":"Ik zag het apparaat reageren","wizard.verify_question":"Heb je zelf de verwachte reactie gezien of gehoord? Dit is jouw bevestiging, geen radio-ACK.","wizard.transmit_confirm":"Staat de ontvanger in de koppelmodus? Nu één goedgekeurd koppelcommando uitzenden?","ux3.change_profile":"Apparaatprofiel wijzigen","ux3.select_profile":"Apparaatprofiel kiezen","ux3.no_profile":"Nog niet gekozen","ux3.not_in_ha":"Niet in Home Assistant","ux3.manage_details":"Apparaat en RF-signalen beheren","ux3.not_yet_available":"Bediening is nog niet beschikbaar.","ux3.finish_rx_only":"Afronden (alleen ontvangst)","ux3.orcon_setup_short":"Ontvangen werkt; RF-koppeling wordt nog getest.","ux3.rx_only_short":"Signalen ontvangen kan al. Bedienen via RFLink nog niet.","ux3.receive_only_status":"Alleen ontvangst","ux3.control_ready_status":"Bediening mogelijk","ux3.sensor_status":"Ontvangt metingen","ux3.profile_missing_status":"Profiel kiezen","debug.menu":"Systeemdebug","debug.title":"Systeemdebug","debug.help":"Wat RFLink met een signaal of opdracht doet: decoder, aangeleerd apparaat, profiel, koppelen, TX en herstel van ontvangst. Los van de radiologs.","debug.lab_gate":"GDO TX-lab","debug.probe_gate":"RAMSES zendtest","debug.radio":"868 MHz-radiostatus","debug.filter":"Toon","debug.all":"Alles","debug.live":"Live","debug.refresh":"Verversen","debug.export":"Debuglog exporteren","debug.empty":"Nog geen systeemgebeurtenissen.","debug.experiment":"Advanced: fysieke RAMSES-zendtest","debug.experiment_help":"Verzendt één 10E0-testtelegram, geen ventilatiestand of koppeling. Vereist beide YAML-labopties. Test alleen met je eigen apparatuur en controleer RF met een tweede ontvanger.","debug.send":"Eén testtelegram verzenden","debug.privacy":"128 gebeurtenissen in RAM (weg na herstart). Apparaat-ID’s kunnen erin staan: controleer vóór delen. Geen ruwe RF-signalen of wachtwoorden.","debug.fetch_failed":"Kan systeemgebeurtenissen niet ophalen:","debug.enabled":"Ingeschakeld","debug.disabled":"Uitgeschakeld","debug.ready":"Radio gereed","debug.not_ready":"Niet gereed","debug.fault":"Fout bij RX-herstel","debug.count":"{shown} van {total} gebeurtenissen","debug.dropped":"Gemist","debug.confirm":"EEN experimenteel RAMSES 10E0-testtelegram op 868 MHz verzenden? Dit koppelt of bedient de box niet.","debug.sending":"Experimenteel telegram verzenden…","debug.sent_unverified":"Zendadapter meldt verzonden en ontvangst hersteld. RF-signaal en reactie van de box NIET bevestigd.","debug.failed":"Test mislukt","logbook.title_nav":"▤ Logboeken","logbook.title":"Logboeken","logbook.subtitle":"Radio, firmwarebeslissingen en websiteacties in één overzicht.","logbook.all":"Alles","logbook.radio":"Radio","logbook.system":"Systeem","logbook.web":"Web","logbook.level":"Niveau","logbook.any":"Alle","logbook.warnings":"Waarschuwingen en fouten","logbook.errors":"Fouten","logbook.search":"Zoeken","logbook.search_hint":"Apparaat, ID, actie…","logbook.noise":"Ruis tonen","logbook.live":"Live","logbook.refresh":"Verversen","logbook.export":"Weergave exporteren","logbook.note":"Webacties bestaan alleen in deze browsersessie. ESP32 gebruikt uptime; browsertijden zijn bij benadering. API-inhoud wordt niet gelogd.","logbook.system_limit":"Systeemgebeurtenissen (ESP32)","logbook.web_limit":"Webgebeurtenissen (browser)","logbook.verbose":"Webdebug: ook geslaagde automatische API-verversingen","logbook.settings_note":"Systeemlimieten vervallen bij herstart; browserinstellingen blijven lokaal. Radiolimieten staan op ESP32.","logbook.empty":"Nog geen gebeurtenissen.","logbook.events":"gebeurtenissen","logbook.invalid":"Ongeldige limieten.","logbook.saved":"Instellingen opgeslagen.","logbook.save_failed":"Opslaan mislukt:","orcon.experimental_status":"Experimenteel koppelen en bedienen; eerst labopties inschakelen.","orcon.enable_lab":"Zet TX-bediening, GDO UART-lab en RAMSES-testlab aan in je YAML voordat je 1FC9-koppeling probeert.","orcon.wait_ack":"Wacht op het bijpassende antwoord van de ventilatiebox voordat je bevestigt.","orcon.status_waiting_ack":"1FC9-koppelverzoek verzonden; wachten op antwoord van de box (max. 3 minuten).","orcon.status_confirming":"Antwoord ontvangen; bevestigingscommando wordt verzonden zodra de radio weer beschikbaar is.","orcon.status_awaiting_user":"Passend antwoord ontvangen en bevestiging verzonden. Alleen bevestigen als je de box echt zag reageren.","orcon.status_failed":"Koppelen mislukt of verlopen. Bediening blijft geblokkeerd. Bekijk Systeemlog en Radiolog.","orcon.status_paired":"Koppeling door gebruiker bevestigd; experimentele bediening beschikbaar.","orcon.status_not_paired":"Nog niet gekoppeld.","orcon.test_auto":"Test Auto (1×)","orcon.confirm_test_auto":"Nu één experimenteel Auto-commando verzenden? Observeer zelf of de ventilatiebox reageert.","orcon.sent_unconfirmed":"Auto verzonden; reactie van ventilatiebox niet bevestigd.","deviceux.back":"← Terug naar apparaten","deviceux.detail_help":"Hier stel je dit apparaat in, koppel je het en beheer je RF-signalen. Bedienen kan vanuit het overzicht.","deviceux.setup_device":"Apparaat instellen","deviceux.view_details":"Details bekijken","deviceux.settings":"Instellen en koppelen","deviceux.no_profile_help":"Kies het apparaatmodel om de bijbehorende functies te zien.","deviceux.profile_unavailable":"Het opgeslagen profiel kon niet geladen worden. Kies opnieuw een profiel; het ontvangen RF-signaal blijft bewaard.","logs.group_radio":"Radio's","logs.radio_help":"Signalen per radio en de gezamenlijke ruislog.","logs.group_system":"Systeem","logs.system_help":"Beslissingen over decoderen, apparaten, koppelen en zenden op de ESP32.","logs.group_web":"Web","logs.web_help":"Navigatie, klikken en API-problemen in deze browsersessie.","version.subtitle":"Apparaatbediening, gecombineerde logboeken en experimenteel zenden.","version.notes":"Wat is nieuw en technische details","version.changes":"Nieuw in deze versie","version.raw":"Interne firmwareversie","version.technical":"Technische details","version.legacy":"Geïnstalleerde firmware (oude versiebenaming).","version.legacy_note":"Bekijk voor deze oudere build de oorspronkelijke release-notities.","version.note1":"Eenvoudig apparatenoverzicht met apart instellen en koppelen.","version.note2":"Radio-, systeem- en webevents in één logviewer met vaste hoogte.","version.note3":"Logginginstellingen gegroepeerd en bibliotheek direct zichtbaar.","version.note4":"Apparaat- en bibliotheek-API voorzien van timingdiagnose.","version.lab_tools":"TX-lab & systeemdiagnose","version.lab_help":"Dit scherm is voor experimentele radio- en TX-diagnose. Voor normale systeemgebeurtenissen ga je naar Logboeken → Systeem.","version.open_logs":"Open systeemlogboek","deviceux.remove_short":"Verwijderen","logbook6.frequency":"Frequentie","logbook6.time":"Tijd","logbook6.source":"Bron","logbook6.event":"Gebeurtenis","logbook6.filtered_empty":"Geen passende gebeurtenissen. Schakel ‘Ruis tonen’ in om ook ruismeldingen te zien.","deviceux6.profile_title":"Apparaatprofiel kiezen","deviceux6.profile_help":"Kies het apparaatmodel. RFLink toont daarna de juiste functies en eventuele koppelstappen.","deviceux6.pair_title":"Koppelen en testen","deviceux6.pair_help":"Koppel dit apparaat aan RFLink wanneer het profiel dat ondersteunt.","deviceux6.status_title":"Huidige status","deviceux6.no_profile_yet":"Na de profielkeuze zie je hier de beschikbare functies.","deviceux6.extra_title":"Overige apparaatinstellingen","deviceux6.intro":"Kies het juiste profiel, koppel waar nodig en beheer extra opties.","logbook6.info":"Info","logbook6.warn":"Let op","logbook6.error":"Fout","logbook6.noise":"Ruis","diag7.event_type":"Type","diag7.boot_filter":"Crashes en herstarts","diag7.tx_filter":"Zenden en koppelen","diag7.api_filter":"API en browser","diag7.view_log":"Bekijk crashlog","diag7.crashes":"{n} crashes in de laatste 8 starts","diag7.crash_help":"RFLink is opnieuw gestart na een fout. De oorzaak is niet uit de resetreden alleen af te leiden.","diag7.export":"Diagnosepakket (JSON)","diag7.uptime_note":"Tijden zijn uu:mm:ss,mmm sinds de start van de ESP32; browsertijden zijn bij benadering.","diag7.health_attention":"Aandacht nodig: eerdere crashes","diag7.health_detail":"De radio’s en decoders zijn bereikbaar, maar RFLink is recent na fouten herstart.","diag7.raw_trace":"Technische trace (optioneel)","version.source_hash":"Bronhash (SHA-256, verkort)","version.history_time":"Eerdere start","version.history_event":"Start #{n} · resetreden: {reason}","version.history_note":"Historische NVS-registratie; exacte tijd, oorzaak en backtrace zijn niet bewaard.","version.note5":"v0.6.1 Radio Gaga: verbeterde experimentele CC1101/GDO-UART-zendvoorbereiding.","version.note6":"UART1 wordt nu vóór de 8N1-instellingen geïnstalleerd, met foutcodes per stap.","version.note7":"Radiolog toont TX-aanvragen, radio- en UART-stappen, afwijzingen en herstel naar RX.","version.note8":"Apparaatdetails overzichtelijker: ontvangen functies, Home Assistant en RF-beheer zonder geneste hoofd-uitklapper.","crashtrace.reason":"Crashreden","crashtrace.core":"Core","crashtrace.firmware":"Firmwarebuild van crash","crashtrace.other_build":"Andere/oude build (oorspronkelijke ELF vereist)","crashtrace.unknown_build":"Onbekende build","crashtrace.backtrace":"Backtrace","crashtrace.other_core":"Andere core","crashtrace.elf_note":"Ruwe adressen: decodeer met de firmware.elf van de oorspronkelijke crashbuild. Geen exacte historische tijd.","crashtrace.available":"Backtrace beschikbaar","crashtrace.boot_event":"Crashbacktrace bij start #{n}","crashtrace.unknown_boot":"Oudere crashbacktrace (startnummer onbekend)","diag55.tab":"Diagnose","diag55.title":"Alle diagnose op één plek","diag55.intro":"Radio- en SPI-status, herstarts, crashdetails en experimentele TX. De gebeurtenissen staan in dezelfde Radio- en Systeemtijdlijn.","diag55.radio_heading":"Radio’s, SPI en opstartgeschiedenis","diag55.experimental_heading":"Experimentele TX en systeemdiagnose","diag55.open_crashes":"Bekijk crashmeldingen","diag55.open_radio":"Bekijk radiolog","diag55.spi_filter":"SPI / radioherstel","diag55.boots":"Crashes & herstarts","diag55.radio":"Radio’s & SPI","diag55.memory":"Geheugen","diag55.lab":"Experimenteel","diag55.export":"Diagnosepakket (JSON)","diag55.boot_hint":"Eerdere starts en beschikbare backtraces. Zonder tijdstempel wordt alleen het bootnummer getoond.","diag55.lab_hint":"TX, UART/GDO en uitgebreide systeemdiagnose. Een testuitzending start nooit automatisch.","diag55.ready":"ontvangst gereed","diag55.crashes":"crashes","diag55.internal":"Intern vrij","diag55.trace":"Technische crash","diag55.nohistory":"Geen bootgeschiedenis beschikbaar.","diag55.online":"online","diag55.offline":"offline","diag55.errors":"fouten","diag55.restores":"herstelpogingen","diag55.no_radio":"Geen radios beschikbaar.","diag55.lab_enabled":"Lab ingeschakeld","diag55.lab_disabled":"Lab uitgeschakeld","diag55.adapter_ready":"adapter gereed","diag55.adapter_unready":"adapter niet gereed","pair55.queued":"Koppelopdracht ingepland. Bekijk Radio- en Systeemlog voor het resultaat.","pair55.auto_queued":"Auto-test ingepland. Nog geen RF-uitzending bevestigd.","orcon.status_queued":"Koppelopdracht ingepland. Bekijk Radio- en Systeemlog voor het resultaat.","rf60.status_intro":"Een rustig overzicht van je systeem, radio’s en aandachtspunten.","rf60.system":"Systeem","rf60.radios":"Radio’s","rf60.radios_help":"Beschikbare radio’s","rf60.devices":"Apparaten","rf60.saved":"Opgeslagen apparaten","rf60.attention":"Aandachtspunten","rf60.attention_title":"Aandacht nodig","rf60.attention_help":"Alleen zaken die aandacht vragen.","rf60.no_problems":"Geen aandachtspunten.","rf60.needs_attention":"Aandacht nodig","rf60.radio_overview":"Radio-overzicht","rf60.radio_help":"Open een radio voor ontvangst- en zendgegevens.","rf60.activity_help":"Laatste activiteiten; het volledige overzicht staat in Logboeken.","rf60.open_logs":"Volledig logboek openen","rf60.system_details":"Uitgebreide systeemstatus","rf60.quick_actions":"Snelle acties","rf60.quick_help":"Veelgebruikte taken direct bereikbaar.","rf60.last_seen":"Laatst ontvangen","rf60.not_seen":"Nog niet waargenomen","rf60.more_details":"Meer details","rf60.control":"Bedienen","rf60.values":"Waarden","rf60.no_values":"Nog geen meetwaarden beschikbaar.","rf60.protocol":"Protocol","rf60.controls_title":"Bediening en functies","rf60.advanced_details":"Signalen en technische details","rf60.advanced_help":"Ontvangen signalen, RF-identiteiten, Home Assistant en beheer.","rf60.remove_help":"Verwijdert dit apparaat en de RFLink-instellingen. Je moet dit bevestigen.","rf60.inspect":"Diagnose bekijken","rf60.history_issues":"Eerdere crashes: {n}","rf61.functions_title":"Ontvangen functies","rf61.functions_help":"{n} herkende functies. Beoordeel nieuwe functies en bekijk wat RFLink heeft geleerd.","rf61.ha_title":"Beheer en Home Assistant","rf61.ha_help":"Voeg dit apparaat toe aan Home Assistant of geef het een herkenbare naam.","rf61.signals_title":"RF-signalen beheren","rf61.signals_help":"Bekijk of vervang gekoppelde RF-codes.","rf61.guide_title":"Hulp bij instellen","rf61.guide_help":"Aanwijzingen voor het gekozen apparaatprofiel.","rf61.id_title":"Technische apparaat-ID","rf61.id_help":"Interne identificatie voor ondersteuning en debugging.","version.note9":"v0.6.2 Radio Gaga: één startmenu voor bijwerken, een schone release en tests; handleidingen en historische documenten staan in docs/."},"fr":{"static.000":"RFLink Universal","static.001":"● En ligne","static.002":"0 signaux","static.003":"Durée de fonctionnement —","static.004":"◉ État","static.005":"▣ Appareils","static.006":"◷ Activité","static.007":"◇ Ajouter un appareil","static.008":"⚙ Paramètres","static.009":"Status","static.010":"L’essentiel au quotidien. Les détails techniques sont accessibles plus loin.","static.011":"Vérification du système…","static.012":"Chargement de l’état.","static.013":"Voir les diagnostics","static.014":"Chargement de l’état radio…","static.015":"Voyant d’état RVB","static.016":"État actuel","static.017":"Chargement…","static.018":"LED","static.019":"WS2812 · GPIO48","static.020":"Actions rapides","static.021":"+ Ajouter un appareil","static.022":"Trouvez et enregistrez une télécommande, un capteur ou un interrupteur.","static.023":"Pairing","static.024":"Voir les appareils dont le décodeur prend en charge l’appairage.","static.025":"Protocole inconnu","static.026":"Le mode développeur compare les captures et propose du code.","static.027":"Résoudre un problème","static.028":"Diagnostics, bibliothèque des décodeurs et informations techniques.","static.029":"Activité récente","static.030":"Aucune activité pour le moment.","static.031":"Appareils","static.032":"RFLink ne crée un appareil qu’après avoir observé suffisamment son signal et obtenu votre confirmation. Vous pouvez consulter, dissocier ou remplacer ses codes RF.","static.033":"Seuls les appareils avec des étapes de configuration sont affichés. La transmission RF peut être indisponible.","static.034":"Vérification du stockage des appareils…","static.035":"Chargement des appareils…","static.036":"Nouveaux signaux","static.037":"Chargement des candidats…","static.038":"Activité","static.039":"Signaux récents détectés par RFLink.","static.040":"Filtrer par fréquence","static.041":"Chargement de l’état…","static.042":"Direction","static.043":"RX et TX","static.044":"RX uniquement","static.045":"TX uniquement","static.046":"Les entrées TX indiquent les commandes transmises au pilote radio, pas une confirmation de l’appareil cible. Les données radio brutes servent au dépannage.","static.047":"Activité de toutes les radios","static.048":"Chargement de l’activité…","static.049":"Ajouter un appareil","static.050":"Choisissez la fréquence, appuyez sur Démarrer puis utilisez l’appareil.","static.051":"Ajouter un appareil","static.052":"Pour l’usage quotidien","static.053":"Créer un décodeur","static.054":"Nécessaire uniquement si RFLink ne connaît pas encore le protocole.","static.055":"Choisissez une fréquence","static.056":"Vérification de l’état des radios…","static.057":"Ajouter un appareil","static.058":"Appuyez sur Démarrer puis utilisez l’appareil une ou plusieurs fois.","static.059":"Démarrer","static.060":"Arrêter","static.061":"Nom de l’appareil","static.062":"Fonctions","static.063":"Choisissez les fonctions que vous souhaitez utiliser.","static.064":"Ajouter à Home Assistant","static.065":"Les détails radio techniques sont enregistrés automatiquement.","static.066":"Enregistrer","static.067":"Associer à un appareil existant","static.068":"Mesure technique","static.069":"Placez l’appareil à environ 1 mètre de la bonne antenne. N’appuyez pas encore sur un bouton.","static.070":"Mesurer le bruit de fond (5 s)","static.071":"Écouter l’appareil","static.072":"Arrêter et effacer la mesure","static.073":"Bruit de fond","static.074":"Pas encore mesuré","static.075":"Répétitions","static.076":"—","static.077":"Protocole","static.078":"Identité","static.079":"Signal","static.080":"En attente d’une capture","static.081":"Lorsque RFLink a détecté l’appareil plusieurs fois de manière fiable, il apparaît sous","static.082":"Appareils → Nouveaux signaux","static.083":". Vous pouvez alors le nommer ou l’associer à un appareil existant.","static.084":"Assistant de décodage","static.085":"RFLink ne génère pas de firmware sur l’ESP32. Il propose un point de départ à copier dans un nouveau dossier de décodeur.","static.086":"Créer un dossier","static.087":"python tools/new_decoder.py mon_protocole \"Mon protocole\"","static.088":"Puis modifier","static.089":"uniquement decoder.cpp + manifest.json","static.090":"Voir la proposition de code","static.091":"Effectuez d’abord une capture.","static.092":"Paramètres et aide","static.093":"Des instructions claires d’abord, les détails techniques si nécessaire.","static.094":"WS2812 sur GPIO48. Orange clignotant : radio absente ou non prête. Blanc : OTA ; rouge : erreur ; vert : tout est prêt. Le battement vert peut être désactivé sans masquer les avertissements.","static.095":"Luminosité","static.096":"20%","static.097":"Battement vert si prêt","static.098":"Coché : le vert clignote lorsque tout est prêt (deux éclairs toutes les 6 secondes). Décoché : aucun voyant vert. Pour le premier essai, réglez la luminosité à 100 % puis enregistrez.","static.099":"État actuel","static.100":"Enregistrer les paramètres LED","static.101":"Les paramètres sont enregistrés sur l’ESP32 et conservés après un redémarrage. Cette page est réservée aux réseaux de confiance.","static.102":"Que souhaitez-vous faire ?","static.103":"Nouvel appareil","static.104":"Ouvrir l’apprentissage","static.105":"Appareils à appairer","static.106":"Home Assistant (MQTT Discovery)","static.107":"Ajoutez les appareils RF un par un sous Appareils. L’ESP32 reste connecté via l’API ESPHome.","static.108":"Courtier MQTT","static.109":"Vérification…","static.110":"Activez éventuellement le paquet MQTT dans YAML et ajoutez les identifiants du courtier à secrets.yaml. La suppression d’appareils de Home Assistant nécessite une connexion au courtier.","static.111":"Gérer les appareils HA","static.112":"Bibliothèque des décodeurs","static.113":"Que peut reconnaître RFLink, que reçoit Home Assistant et que peut-on piloter ? Les tests logiciels ne sont pas des essais radio.","static.114":"Afficher","static.115":"Tous les protocoles","static.116":"Télécommandes et commandes","static.117":"Capteurs","static.118":"Exemples de développement","static.119":"Chargement de la bibliothèque…","static.120":"Registre technique des décodeurs","static.121":"Règle de maintenance","static.122":"Un décodeur doit avoir une seule tâche :","static.123":"RFFrame → DecodeResult","static.124":"Aucune interface web, Home Assistant, configuration GPIO ou logique d’autres décodeurs dans un module de protocole.","static.125":"Diagnostics","static.126":"Chargement de l’état radio et des décodeurs.","static.127":"Appareils enregistrés","static.128":"Registre des décodeurs","static.129":"Stockage","static.130":"Reçu","static.131":"Décodé","static.132":"Inconnu","static.133":"Explication technique","static.134":"Une impulsion correspond à une période pendant laquelle le signal RF reçu est HIGH ou LOW. Un bit peut contenir plusieurs impulsions. Le mode développeur compare les protocoles grâce à ces durées.","static.135":"Monitor","static.136":"Paramètres","static.137":"Donnez-lui un nom reconnaissable. L’identité RF est conservée en interne.","static.138":"Vérifier les fonctions","static.139":"Annuler","static.140":"Enregistrer","language.title":"Langue et région","language.help":"Choisissez la langue de l’interface. Votre choix est enregistré sur l’ESP32.","language.label":"Langue de l’interface","language.saving":"Enregistrement de la langue…","language.saved":"Langue enregistrée.","language.failed":"Impossible d’enregistrer la langue sur l’ESP32. Langue précédente rétablie.","dyn.now":"maintenant","dyn.sec":"{n} s","dyn.min":"{n} min","dyn.hour":"{n} h","dyn.dayhour":"{d} j {h} h","dyn.uptime":"Disponibilité {value}","dyn.frames":"{n} signaux","action.on":"Marche","action.off":"Arrêt","action.up":"Monter","action.down":"Descendre","action.my":"Favori (My)","action.prog":"Programmer","action.low":"Faible","action.medium":"Moyen","action.high":"Élevé","action.standby":"Veille","action.status":"État","action.measurement":"Mesure","action.rain_update":"Mesure de pluie","action.weather_update":"Mesure météo","action.code":"Code fixe","action.button":"Bouton {n}","action.command":"Code sonnette {n}","action.timer":"Minuterie {n} minutes","dyn.decoder_review":"Décodeur {protocol} : vérifiez les fonctions reconnues avant l’enregistrement.","dyn.decoder_missing":"Les informations du décodeur sont manquantes. RFLink ne peut pas vérifier les fonctions en toute sécurité. Actualisez la page et réessayez.","dyn.dev_instruction":"Gardez l’appareil de test à environ 1 mètre. Mesurez d’abord le bruit de fond, puis effectuez plusieurs fois une seule action physique.","dyn.device_instruction":"Gardez l’appareil à environ 1 mètre de l’antenne correcte. N’appuyez pas encore sur un bouton.","dyn.rx_ready":"● RX prêt","dyn.rx_unavailable":"● RX indisponible","dyn.disconnected":"● Non connecté","dyn.checking":"● Vérification","dyn.rx_ready_short":"RX prêt","dyn.rx_fault":"RX indisponible","dyn.not_connected":"Non connecté","dyn.checking_short":"Vérification","dyn.radio_unavailable_title":"Radio ou récepteur indisponible","dyn.no_radios":"Aucune radio configurée","dyn.only_working_receivers":"Seuls les récepteurs fonctionnels peuvent être sélectionnés.","dyn.radio_checking":"Vérification de l’état radio…","dyn.no_working_receivers":"Aucun récepteur radio fonctionnel : connectez les modules et redémarrez l’ESP32.","dyn.all_radios":"Toutes les radios","dyn.overview":"Vue d’ensemble","dyn.radio_disconnected_title":"Module radio non connecté","dyn.activity_radio":"Activité · {name}","dyn.activity_all":"Activité de toutes les radios","dyn.unknown":"Inconnu","dyn.no_monitor_activity":"Aucune activité enregistrée pour ce filtre. Les radios absentes ne produisent pas de nouveaux enregistrements RX ou TX.","dyn.tx":"Émission","dyn.available":"Disponible","dyn.unavailable":"Indisponible","dyn.last_signal":"Dernier signal","dyn.view_activity":"Voir l’activité","dyn.no_activity":"Aucune activité pour le moment.","dyn.offline":"● Hors ligne","dyn.esp_unreachable":"ESP32 inaccessible","dyn.esp_unreachable_help":"Vérifiez l’alimentation, le réseau et l’adresse IP. Aucune modification d’appareil n’est effectuée hors ligne.","dyn.api_unreachable":"API inaccessible ; démarrage désactivé.","dyn.connection_lost":"Connexion à l’ESP32 perdue.","dyn.manage_signals":"Gérer les signaux ({n})","dyn.signal_identity_help":"L’ID interne de l’appareil {uid} reste inchangé. Remplacez un code RF uniquement par un nouveau code que le récepteur a lui-même reconnu plusieurs fois.","dyn.rf_signal":"Signal RF {n}","dyn.replace_with":"Remplacer par","dyn.choose_learned":"Choisir un nouveau signal appris","dyn.replace":"Remplacer","dyn.unlink":"Dissocier","dyn.last_code_title":"Dernier code : supprimez l’appareil ou remplacez le code","dyn.no_matching_signal":"Aucun nouveau signal correspondant trouvé. Allez dans Apprendre, recevez le nouveau signal au moins trois fois, puis revenez sans l’enregistrer comme nouvel appareil.","dyn.learn_new_signal":"Apprendre un nouveau signal","dyn.signal_history_help":"Les actions reconnues sont enregistrées au niveau de l’appareil. Pour les protocoles où une seule identité RF représente plusieurs boutons, RFLink ne peut pas encore dissocier de façon fiable un bouton individuel. Dissocier un code RF n’efface pas l’historique d’actions partagé.","dyn.mqtt_remove_unavailable":"MQTT hors ligne : suppression indisponible","dyn.approve":"Approuver","dyn.remove":"Supprimer","dyn.approve_technical":"Approuver la valeur technique","dyn.review_new_functions":"● Vérifier {n} nouvelle(s) fonction(s)","dyn.view_functions":"Voir et modifier les fonctions","dyn.approved_only_help":"Seules les fonctions que vous approuvez sont enregistrées et publiées dans Home Assistant. Les changements de valeur des fonctions existantes sont traités automatiquement.","dyn.found_not_approved":"Trouvé, pas encore approuvé","dyn.no_new_visible":"Aucune nouvelle fonction visible à vérifier.","dyn.approved_features":"Fonctions approuvées","dyn.none_approved":"Aucune fonction visible approuvée pour le moment.","dyn.technical_radio_data":"Données radio techniques ({saved} enregistrées{pending})","dyn.pending_suffix":", {n} à vérifier","dyn.technical_radio_help":"Ces valeurs peuvent être nécessaires pour un code d’émission exact. Ne les modifiez que si vous connaissez le protocole. RFLink ne les expose pas comme capteurs Home Assistant distincts.","dyn.saved":"Enregistré","dyn.mqtt_connected":"Connecté","dyn.mqtt_disconnected":"Non connecté","dyn.mqtt_not_configured":"Non configuré","dyn.storage_ok":"Les appareils enregistrés sont conservés après redémarrage. La modification des codes RF nécessite une nouvelle réception confirmée.","dyn.storage_uncertain":"L’état du stockage est incertain. Les appareils ne peuvent pas être modifiés en toute sécurité.","dyn.pairing_only":"Appareils avec configuration guidée","dyn.device_sub":"Gérez le nom, les codes RF et la publication Home Assistant par appareil.","dyn.mqtt_no_features_title":"MQTT non connecté ou aucune fonction approuvée","dyn.remove_from_ha":"Retirer de HA","dyn.add_to_ha":"Ajouter à HA","dyn.receive_sensor":"Capteur en réception","dyn.rf_device":"Appareil RF","dyn.rf_signals":"{n} signal(aux) RF","dyn.in_ha":"Dans Home Assistant","dyn.only_rflink":"RFLink uniquement","dyn.ha_control_allowed":"Commande HA autorisée","dyn.features_to_review":"{n} fonction(s) à vérifier","dyn.review_needed":"Vérification requise","dyn.manage_more":"Gérer davantage : nom, Home Assistant et codes RF","dyn.changes_manual":"Les modifications ne sont enregistrées qu’après votre action. RFLink reste la source de référence.","dyn.mqtt_offline":"MQTT hors ligne","dyn.disable_ha_control":"Désactiver la commande HA","dyn.allow_ha_control":"Autoriser la commande HA","dyn.rename":"Renommer","dyn.dangerous_actions":"Actions dangereuses","dyn.dangerous_help":"Cela supprime complètement l’appareil de RFLink et, s’il est lié, de Home Assistant.","dyn.delete_device":"Supprimer définitivement l’appareil","dyn.technical_device_id":"ID technique de l’appareil","dyn.no_pair_devices":"Aucun appareil d’appairage confirmé.","dyn.no_devices":"Aucun appareil confirmé. Ajoutez-en un via Apprendre.","dyn.new_signal":"Nouveau signal","dyn.received_times":"Reçu {n} fois","dyn.ready_review":"Prêt pour vérification","dyn.still_listening":"Toujours à l’écoute","dyn.check_signal":"Vérifiez le signal reçu avant de l’enregistrer.","dyn.looks_like":"Ressemble à {name}. Ne liez que s’il s’agit réellement du même appareil.","dyn.review_add":"Vérifier et ajouter","dyn.more_technical":"Plus d’options et de détails techniques","dyn.rf_identity":"Identité RF : {id}","dyn.link_to":"Lier à {name}","dyn.not_saved":"Pas encore enregistré","dyn.pairing_view":"Vue d’appairage active.","dyn.no_new_signals":"Aucun nouveau signal ne nécessite votre attention.","dyn.learn_replacement_toast":"Apprenez un nouveau signal ; ne l’enregistrez pas comme appareil séparé.","dyn.devices_load_failed":"Impossible de charger les appareils","dyn.confirm_unlink":"Dissocier le code RF {identity} ? Les autres codes de l’appareil continueront de fonctionner.","dyn.unlinked":"Code RF dissocié","dyn.unlink_failed":"Échec de la dissociation ; rien n’a été modifié","dyn.choose_confirmed_signal":"Choisissez d’abord un nouveau signal confirmé","dyn.confirm_replace":"Remplacer le code RF ?\nAncien : {old}\nNouveau : {new}\nLe nom de l’appareil et les autres codes RF seront conservés.","dyn.replaced":"Code RF remplacé et enregistré","dyn.replace_failed":"Échec du remplacement ; ancien code RF conservé","dyn.storage_attention":"Le stockage nécessite votre attention","dyn.decoder_incomplete":"Bibliothèque de décodeurs incomplète","dyn.hardware_incomplete":"Matériel incomplet","dyn.all_ready":"Tout est prêt","dyn.storage_problem_help":"La connexion fonctionne, mais les modifications d’appareils ne peuvent pas être enregistrées en toute sécurité. Consultez les journaux ESPHome.","dyn.decoder_problem_help":"Tous les décodeurs attendus ne sont pas chargés. Certains signaux reçus peuvent ne pas être reconnus.","dyn.hardware_problem_help":"L’ESP32 et le site fonctionnent. {missing} radio(s) sur {total} ne sont pas connectées ou prêtes.","dyn.ready_help":"L’ESP32, la bibliothèque de décodeurs et toutes les radios configurées sont prêts.","dyn.esp_web":"ESP32 et interface web","dyn.radios":"Radios","dyn.ready_count":"{ready}/{total} prêtes","dyn.decoders":"Décodeurs","dyn.loaded":"{value} chargés","dyn.device_storage":"Stockage des appareils","dyn.check_required":"Vérification requise","dyn.saved_count":"{n} enregistrés","dyn.storage_error":"Erreur de stockage","dyn.confirm_remove_feature":"Supprimer la fonction « {key} » de RFLink et, si elle est liée, également de Home Assistant ?","dyn.feature_removed":"Fonction supprimée et enregistrée","dyn.feature_approved":"Fonction approuvée et enregistrée","dyn.feature_failed":"La fonction n’a pas été modifiée. Vérifiez MQTT et le stockage des appareils.","dyn.confirm_remove_ha":"Retirer « {name} » de Home Assistant ? L’appareil RF reste enregistré dans RFLink.","dyn.ha_removed":"Découverte HA supprimée ; l’appareil reste enregistré","dyn.ha_sent":"Découverte HA envoyée au broker","dyn.ha_failed":"Échec de la modification HA : vérifiez MQTT et le stockage","dyn.confirm_disable_tx":"Désactiver la commande de « {name} » dans Home Assistant ?","dyn.confirm_enable_tx":"Autoriser l’émission RF pour « {name} » ? Seuls les boutons pris en charge et approuvés par ce décodeur apparaissent dans HA. L’état réel du récepteur n’est pas confirmé.","dyn.tx_saved":"Autorisation d’émission enregistrée","dyn.tx_failed":"Autorisation d’émission inchangée ; vérifiez MQTT et la configuration TX","dyn.new_device_name":"Nouveau nom de l’appareil :","dyn.name_length":"Le nom doit contenir entre 1 et 80 caractères","dyn.name_saved":"Nom enregistré","dyn.rename_failed":"Le renommage n’a pas été enregistré","dyn.confirm_delete":"Supprimer l’appareil « {name} » de RFLink et, s’il est actif, également de Home Assistant ? Cette action est irréversible ; un nouvel apprentissage sera nécessaire.","dyn.device_removed":"Appareil supprimé","dyn.delete_failed":"Échec de la suppression","dyn.add_device":"Ajouter un appareil","dyn.wizard_start_help":"Choisissez une radio et cliquez sur Démarrer. La mesure du bruit de fond et l’écoute sont automatiques.","dyn.storage_unavailable":"Stockage des appareils indisponible","dyn.receiver_unavailable":"Ce récepteur est indisponible","dyn.step1":"Étape 1/3 · Mesurer le bruit de fond","dyn.step1_help":"Attendez cinq secondes. N’actionnez pas encore votre appareil.","dyn.step2":"Étape 2/3 · Actionner votre appareil","dyn.step2_help":"Appuyez plusieurs fois sur le même bouton. Dès que le signal est reconnu, vous pouvez saisir le nom ici.","dyn.measure_failed":"Échec de la mesure","dyn.measure_failed_help":"Vérifiez que la radio est disponible puis réessayez.","dyn.start_failed":"Impossible de démarrer","dyn.start_failed_help":"Vérifiez la radio puis réessayez.","dyn.step3":"Étape 3/3 · Vérifier et enregistrer","dyn.candidate_received":"{protocol} a été reçu {n} fois. Nommez l’appareil et vérifiez les fonctions détectées.","dyn.observations":"{n} observations","dyn.link_to_name":"Lier à {name}","dyn.temp_connection":"Connexion temporairement indisponible","dyn.temp_connection_help":"Réessayez lorsque ESPHome est accessible.","dyn.decoder_safe_disabled":"Informations du décodeur manquantes ; l’enregistrement sécurisé est désactivé","dyn.enter_name":"Saisissez un nom de 1 à 80 caractères","dyn.confirm_no_features":"Vous n’avez approuvé aucune fonction. L’appareil sera enregistré uniquement dans RFLink. Continuer ?","dyn.signal_linked":"Signal lié. Les nouvelles fonctions seront proposées sur l’appareil existant ; les choix de cet écran ne lui ont pas été appliqués.","dyn.saved_ha_failed":"Appareil enregistré, mais la publication HA a échoué. Vous pourrez l’activer plus tard dans Appareils.","dyn.device_saved":"Appareil enregistré. Il reste conservé après un redémarrage.","dyn.done":"Terminé","dyn.done_help":"Vous pouvez ajouter un autre appareil ou afficher vos appareils.","dyn.save_failed":"Échec de l’enregistrement ; réessayez","dyn.confirm_link":"Lier ce signal à {name} ?","dyn.modal_invalid":"Vérifiez les informations du décodeur et saisissez un nom d’appareil de 1 à 80 caractères","dyn.confirm_store_only":"Aucune fonction approuvée. Enregistrer uniquement l’appareil ?","dyn.device_added":"Appareil ajouté","dyn.add_failed":"Impossible d’ajouter l’appareil","dyn.linked_existing":"Lié à l’appareil existant","dyn.link_failed":"Échec de la liaison","dyn.radio_status_unreachable":"État radio inaccessible.","dyn.measuring_background":"Mesure du bruit de fond…","dyn.waiting_capture":"En attente d’une capture","dyn.capture_first":"Effectuez d’abord une capture.","dyn.protocol_unknown":"encore inconnu","dyn.identity_unknown":"pas encore déterminée","dyn.no_code_suggestion":"Aucune suggestion de code.","dyn.signal_strong":"Fort / peut-être trop proche","dyn.good":"BON","dyn.weak":"Faible","dyn.raw_rssi":"brut : RSSI indisponible","dyn.capture_found":"Capture trouvée","dyn.measuring":"Mesure…","dyn.ready":"Prêt","dyn.background_ready":"Mesure du bruit de fond terminée","dyn.background_failed":"Échec de la mesure du bruit de fond","dyn.listen_started":"Écoute démarrée","dyn.listen_failed":"Impossible de démarrer l’écoute","dyn.not_measured":"Pas encore mesuré","dyn.listen_stopped":"Écoute arrêtée ; mesure temporaire effacée","dyn.stop_failed":"Impossible d’arrêter","dyn.invalid_brightness":"Luminosité invalide","dyn.led_saved":"Paramètres LED enregistrés","dyn.save_failed_short":"Échec de l’enregistrement","dyn.protocol_count":"{n} protocoles · détails sur demande","dyn.receive_only":"Réception uniquement","dyn.example":"Exemple","dyn.tx_hw_tested":"Émission : testée sur matériel","dyn.tx_sw_tested":"Émission : test logiciel","dyn.itho_separate":"Commande native","dyn.tx_blocked":"Émission bloquée","dyn.tx_unavailable":"Émission indisponible","dyn.rx_decoder_present":"Décodeur de réception présent (matériel non confirmé)","dyn.in_ha_label":"Dans Home Assistant :","dyn.what_to_know":"Que dois-je savoir ?","dyn.software_prefix":"Logiciel : ","dyn.encoder_registered":"encodeur d’émission enregistré ; ","dyn.manifest_verified":"état matériel dans le manifeste : vérifié","dyn.host_only":"testé sur hôte, test radio physique manquant","dyn.no_generic_encoder":"aucun encodeur d’émission générique","dyn.only_approved":"RFLink affiche uniquement les fonctions approuvées.","dyn.technical_data":"Données techniques","dyn.actions":"Actions","dyn.fields":"Champs","dyn.none":"aucun","dyn.learn":"Apprendre","dyn.no_protocols":"Aucun protocole dans ce filtre.","dyn.protocol_detection_note":"RFLink identifie le protocole réel uniquement après réception du signal. Il s’agit d’une instruction, pas d’un filtre de protocole.","dyn.no_decoders":"Aucun décodeur enregistré.","dyn.decoder_registry_unavailable":"Registre des décodeurs indisponible.","dyn.decoder_library_unavailable":"Bibliothèque de décodeurs indisponible. L’apprentissage et l’approbation sont temporairement désactivés.","placeholder.device_name":"ex. Capteur de jardin","field.temperature":"Température","field.humidity":"Humidité","field.battery_low":"Batterie faible","field.rain":"Pluie","field.fan_speed":"Vitesse du ventilateur","protocol.ExampleSwitch.band":"Pas de vraie radio","protocol.ExampleSwitch.instruction":"Ceci est un exemple de code et ne peut pas apprendre de vrais signaux.","protocol.ExampleSwitch.ha_result":"Aucun appareil Home Assistant.","protocol.ExampleSwitch.warning":"Visible uniquement dans les détails techniques ; non destiné à un usage quotidien.","protocol.IthoRFT.band":"868,30 MHz","protocol.IthoRFT.instruction":"Appuyez plusieurs fois sur un bouton de vitesse ou de minuterie de votre télécommande Itho.","protocol.IthoRFT.ha_result":"Événements de vitesse/minuterie confirmés et mesures prises en charge si disponibles.","protocol.IthoRFT.warning":"RFLink émet avec sa propre télécommande Itho virtuelle. Associez-la une fois à l’unité de ventilation avant d’utiliser les commandes.","protocol.Acurite609TXC.band":"433,92 MHz","protocol.Acurite609TXC.instruction":"Attendez que le même message du capteur soit reçu plusieurs fois.","protocol.Acurite609TXC.ha_result":"Température, humidité et avertissement de batterie confirmés si disponibles.","protocol.Acurite609TXC.warning":"Après un changement de pile, l’identité RF peut changer ; ne liez pas automatiquement une nouvelle identité.","protocol.FineOffsetWH2.band":"433,92 MHz","protocol.FineOffsetWH2.instruction":"Attendez plusieurs messages du capteur météo.","protocol.FineOffsetWH2.ha_result":"Température et humidité confirmées.","protocol.FineOffsetWH2.warning":"Une mesure absente ne signifie pas que cette fonction doit être supprimée.","protocol.Nexa.band":"433 MHz","protocol.Nexa.instruction":"Appuyez plusieurs fois sur Marche puis sur Arrêt pour le même canal.","protocol.Nexa.ha_result":"Événements Marche/Arrêt confirmés. Les boutons de commande n’apparaissent qu’après une autorisation d’émission distincte.","protocol.Nexa.warning":"Une télécommande et la lampe ne sont pas automatiquement le même appareil ; apprenez les différents canaux séparément.","protocol.ByronSX.band":"433 MHz","protocol.ByronSX.instruction":"Appuyez plusieurs fois sur le bouton de sonnette et attendez que RFLink reconnaisse le même signal.","protocol.ByronSX.ha_result":"Un événement de sonnette ; un bouton d’émission seulement après autorisation séparée.","protocol.ByronSX.warning":"Une commande d’émission ne confirme pas que la sonnette a réellement sonné.","protocol.Dooya.band":"433 MHz","protocol.Dooya.instruction":"Appuyez plusieurs fois sur le même bouton. Apprenez ensuite les autres boutons séparément comme propositions de fonction.","protocol.Dooya.ha_result":"Événements de bouton approuvés ; boutons d’émission uniquement après autorisation séparée.","protocol.Dooya.warning":"Les numéros de bouton ne définissent pas encore des significations comme Monter ou Stop. Ne les attribuez pas automatiquement.","protocol.KeeLoq.band":"433 MHz","protocol.KeeLoq.instruction":"Appuyez plusieurs fois sur le même bouton et vérifiez que la même télécommande est reconnue.","protocol.KeeLoq.ha_result":"Événements de bouton reçus et avertissement de batterie s’il est décodé.","protocol.KeeLoq.warning":"Le rolling code est protégé ; les messages reçus ne doivent pas être retransmis.","protocol.RCSwitch.band":"433 MHz","protocol.RCSwitch.instruction":"Appuyez plusieurs fois sur un bouton. Apprenez séparément un autre code fixe et liez-le vous-même à l’appareil souhaité.","protocol.RCSwitch.ha_result":"Un événement pour le code reconnu ; éventuellement un bouton d’émission après autorisation.","protocol.RCSwitch.warning":"Le code, le protocole et la longueur en bits doivent correspondre. Les codes marche et arrêt ne sont pas liés automatiquement.","protocol.AlectoV1.band":"433,92 MHz","protocol.AlectoV1.instruction":"Attendez plusieurs transmissions du capteur météo ; n’appuyez pas sur des boutons au hasard.","protocol.AlectoV1.ha_result":"Uniquement les mesures que vous confirmez, comme température, humidité et éventuellement pluie.","protocol.AlectoV1.warning":"Toutes les variantes ne transmettent pas toutes les mesures. Une valeur absente n’est pas automatiquement une panne.","protocol.SomfyRTS56.band":"433,42 MHz","protocol.SomfyRTS56.instruction":"Appuyez plusieurs fois sur le même bouton de votre télécommande Somfy. N’utilisez pas le bouton de programmation pour ce test de réception.","protocol.SomfyRTS56.ha_result":"Les actions reçues peuvent être suivies. Après appairage de RFLink, les commandes Monter, Descendre et My approuvées peuvent aussi être utilisées.","protocol.SomfyRTS56.warning":"L’émission utilise une télécommande RFLink virtuelle séparée avec son propre compteur tournant. Appairez d’abord RFLink au moteur ; la validation sur moteur réel reste à faire.","protocol.RubicsonTemp.band":"433,92 MHz","protocol.RubicsonTemp.instruction":"Attendez plusieurs messages du capteur de température.","protocol.RubicsonTemp.ha_result":"Température confirmée et avertissement de batterie si disponible.","protocol.RubicsonTemp.warning":"Le canal et la somme de contrôle sont des données techniques, pas des capteurs Home Assistant séparés.","decoder.select_hint":"Sélectionnez un décodeur pour afficher toutes les informations.","decoder.how_to_learn":"Comment apprendre","decoder.why_tx":"Pourquoi l’émission fonctionne ainsi","decoder.what_needed":"Ce qui manque encore","decoder.reason_sensor":"Ce protocole est un capteur en réception seule. L’émission est volontairement désactivée.","decoder.reason_example":"Ceci est un exemple de développement, pas un protocole RF réel.","decoder.reason_tx_ready":"Un encodeur d’émission validé est disponible pour ce protocole.","decoder.reason_tx_host":"L’encodeur d’émission existe et passe les tests logiciels, mais n’a pas encore été validé avec du matériel RF réel.","decoder.reason_itho":"La réception et l’émission native par paquets sont intégrées. RFLink utilise une identité d’émetteur virtuelle distincte pour chaque appareil Itho appris.","decoder.reason_somfy":"La réception et l’émission sont implémentées. RFLink commande Somfy comme une télécommande virtuelle séparée avec son propre compteur tournant.","decoder.reason_keeloq":"La réception peut reconnaître les trames prises en charge, mais l’émission générique est bloquée par le chiffrement à code tournant KeeLoq.","decoder.reason_no_encoder":"La réception est prise en charge, mais aucun encodeur d’émission validé n’est enregistré.","decoder.needed_none":"Rien ; la réception seule est le comportement prévu.","decoder.needed_hw_test":"Un test RF sur un appareil réel et une comparaison des captures avant de déclarer l’émission validée matériellement.","decoder.needed_itho":"Associez RFLink une fois à l’unité de ventilation, puis validez les commandes sur le matériel réel.","decoder.needed_somfy":"Appairez la télécommande RFLink virtuelle au moteur. La validation sur un moteur réel reste à faire.","decoder.needed_keeloq":"Un produit pris en charge avec sa clé cryptographique valide et sa méthode d’appairage. Cela ne peut pas être déduit de façon sûre de captures RF normales seules.","decoder.needed_encoder":"Un encodeur d’émission spécifique au protocole, puis validation logicielle et matérielle.","library.title":"Bibliothèque et mises à jour","library.help":"Les éléments officiels du firmware et vos éléments restent séparés. RFLink ne remplace jamais vos modifications sans confirmation.","library.official_decoders":"Décodeurs officiels","library.my_decoders":"Mes décodeurs","library.my_profiles":"Mes profils d’appareil","library.needs_attention":"Attention requise","library.review":"Examiner les modifications","library.advanced":"Avancé","library.hide_advanced":"Masquer les détails","library.advanced_help":"Informations techniques de bibliothèque pour le diagnostic et le développement.","library.schema":"Schéma de bibliothèque","library.user_storage":"Bibliothèque utilisateur persistante","library.safe_resolutions":"Résolutions automatiques sûres","library.archive":"Éléments locaux archivés","library.enabled":"Activée","library.scaffold":"Structure prête ; bibliothèque utilisateur persistante non activée dans cette version","library.status_attention":"Un ou plusieurs éléments locaux nécessitent une décision. Votre configuration actuelle reste active jusqu’à votre confirmation.","library.status_ok":"Tout est à jour. Aucune action n’est nécessaire.","library.status_scaffold":"La logique de mise à jour et de conflit est prête. Les décodeurs et profils utilisateur persistants constituent la prochaine couche de stockage.","library.attention_title":"La mise à jour de la bibliothèque nécessite votre attention","library.attention_count":"{n} élément(s) local(aux) nécessitent une décision. Rien n’a été remplacé automatiquement.","library.unavailable":"L’état de la bibliothèque est temporairement indisponible.","library.review_pending":"Les choix deviendront actifs lorsque le stockage persistant de la bibliothèque utilisateur sera activé.","library.no_items":"Aucun élément de bibliothèque signalé.","library.source":"Source","library.source_builtin":"Intégré","library.source_user":"Utilisateur","library.stable_id":"ID stable","library.version":"Version","library.pack":"Pack de décodeur","help.library_title":"Qu’est-ce que la bibliothèque ?","help.library_text":"La bibliothèque combine les décodeurs officiels du firmware avec des décodeurs et profils d’appareil créés par l’utilisateur. Les éléments officiels et locaux restent séparés.","help.library_more":"Après une mise à jour, RFLink peut recommander une version officielle. Les modifications locales ne sont jamais écrasées silencieusement.","help.source_title":"Que signifie Source ?","help.source_text":"Intégré signifie que l’élément est fourni avec le firmware. Utilisateur signifie qu’il a été appris ou créé sur cet appareil RFLink.","help.source_more":"La source aide RFLink à déterminer ce qui peut être mis à jour automatiquement et ce qui nécessite votre confirmation.","help.id_title":"Qu’est-ce qu’un ID stable ?","help.id_text":"Un ID stable est l’identité permanente d’un décodeur ou d’un profil à travers les versions du firmware.","help.id_more":"RFLink l’utilise avec les versions et les empreintes de contenu pour détecter doublons, mises à jour et modifications locales.","user.heading":"Mon protocole","user.guide":"Donnez un nom au protocole, nommez le bouton ou l’action à enregistrer et actionnez-le au moins trois fois. Les règles techniques restent dans Avancé.","user.name":"Nom du décodeur","user.id":"ID stable","user.action":"Bouton/action","user.capture":"Enregistrer cette action","user.clear":"Effacer les captures","user.create":"Tester et activer le décodeur","user.advanced":"Avancé : modifier les règles RX et TX","user.advanced_help":"Les règles RX reconnaissent les signaux. Les règles TX créent un aperçu. L’enregistrement est transactionnel : un décodeur actif fonctionnel reste actif si les nouvelles règles échouent à la validation.","user.actions":"Actions séparées par des virgules","user.save_rules":"Enregistrer les règles","user.vm_help":"Les règles tournent dans un interpréteur borné, sans code natif. Voir USER_DECODER_GUIDE.md.","user.firmware":"Convertir en décodeur firmware","user.firmware_help":"Exportez la bibliothèque puis lancez les scripts d’évaluation et de fusion fournis sur votre ordinateur.","user.export":"Télécharger l’export d’évaluation","user.saved":"Protocoles et profils d’appareil personnalisés","user.profile":"Profil d’appareil","user.profile_help":"Donnez au décodeur un nom de modèle clair et des instructions d’apprentissage. Les champs techniques sont facultatifs.","user.profile_name":"Nom du profil","user.profile_id":"ID du profil","user.decoder":"Décodeur","user.instructions":"Instructions d’apprentissage","user.save_profile":"Enregistrer le profil","user.samples":"captures","user.active":"actif","user.inactive":"inactif","user.preview_ready":"Aperçu TX disponible","user.rx_only":"Réception seule","user.edit":"Modifier","user.disable":"Désactiver","user.activate":"Activer","user.remove":"Supprimer","user.empty":"Aucun décodeur personnel enregistré.","user.unavailable":"Bibliothèque personnelle indisponible.","user.activated":"Décodeur activé.","user.disabled":"Décodeur désactivé.","user.confirm_remove":"Supprimer cet élément ?","user.no_samples":"Aucune capture pour le moment.","user.check_radio_action":"Choisissez une radio active et un nom d’action valide.","user.press_three":"Appuyez au moins trois fois sur ce bouton, puis enregistrez l’action suivante.","user.cleared":"Captures effacées.","user.check_id_name":"Saisissez un nom de décodeur. Si vous avez modifié l’ID stable manuellement, il doit commencer par user_.","user.created_tx":"Décodeur actif. Aperçu TX prêt ; autorisez chaque appareil avant l’envoi.","user.created_rx":"Décodeur actif en réception. Ajoutez une règle TX dans Avancé si nécessaire.","user.advanced_saved":"Règles enregistrées, testées et activées.","user.profile_saved":"Profil enregistré.","user.exported":"Export d’évaluation téléchargé.","user.device_controls":"Décodeur personnel et profil","user.tx_notice":"L’aperçu n’émet rien. L’envoi exige l’autorisation de cet appareil ; la commande n’est pas un accusé de réception.","user.no_profile":"Aucun profil","user.revoke_tx":"Arrêter l’envoi","user.allow_tx":"Autoriser l’envoi","user.preview":"Aperçu TX","user.send":"Envoyer","user.confirm_allow":"Autoriser cet appareil à émettre avec son décodeur personnel ?","user.confirm_send":"Envoyer cette action RF","user.sent_request":"Commande TX transmise ; aucune confirmation de l’appareil cible.","user.import":"Restaurer une sauvegarde","user.import_help":"Chaque décodeur est validé avant remplacement. L’autorisation TX n’est jamais restaurée automatiquement ; les compteurs de protocole ne peuvent qu’avancer.","user.import_invalid":"Ce fichier n’est pas une sauvegarde valide de la bibliothèque utilisateur RFLink.","user.import_confirm":"Restaurer {decoders} décodeur(s) et {profiles} profil(s) ? Les ID existants ne sont remplacés qu’après validation.","user.imported":"Restauration terminée : {decoders} décodeur(s), {profiles} profil(s), {bindings} liaison(s) de profil avec des appareils existants. Les autorisations TX n’ont pas été restaurées.","user.analysis_waiting":"Enregistrez des exemples pour comparer les champs RF stables et variables.","user.analysis_need_more":"{action} : enregistrez au moins un exemple supplémentaire.","user.analysis_length":"{action} : les trames enregistrées ont des longueurs différentes ; inspectez ce protocole manuellement.","user.analysis_dynamic":"{action} : {count} symboles ; positions variables : {positions}. Il peut s’agir d’un compteur, checksum, ID ou de données liées au bouton.","user.analysis_fixed":"{action} : {count} symboles ; stables dans les exemples enregistrés.","user.radio_profile":"Profil radio","user.radio_profile_help":"Cette version utilise la configuration radio validée de la carte ; la modulation/le débit CC1101 personnalisés ne sont pas encore configurables depuis les recettes utilisateur.","user.analysis_counter":"{action} : la/les position(s) {positions} ressemblent à un compteur incrémental. La répétition automatique reste désactivée si les valeurs changent.","user.analysis_checksum":"{action} : la/les position(s) {positions} ressemblent à un candidat de somme de contrôle simple. Vérifiez avant de créer une règle Advanced.","library.official_profiles":"Profils d’appareil officiels","decoder.known_profiles":"Profils d’appareil connus","decoder.known_profiles_help":"Familles de modèles associées à ce décodeur dans des sources publiques fabricant/protocole. Il s’agit d’une compatibilité documentée, pas d’une certification matérielle.","decoder.models":"Modèles / alias","decoder.profile_learn":"Apprentissage de ce profil","decoder.hardware_pending":"Validation matérielle sur cette carte RFLink en attente","ui.manage_devices":"Gérer les appareils","ui.manage_devices_help":"Renommez les appareils, vérifiez les nouvelles fonctions et gérez Home Assistant.","ui.settings_sub":"Les réglages courants d’abord. La bibliothèque et les outils techniques se trouvent plus bas.","devices.pairing_filter":"Appareils avec étapes de configuration","devices.show_all":"Afficher tous les appareils","devices.waiting_signals":"Signaux en attente de vérification","devices.waiting_signals_help":"Ces signaux ont été reconnus en dehors de l’assistant Ajouter un appareil. Vérifiez uniquement ceux que vous souhaitez conserver.","placeholder.wizard_device_name":"ex. Télécommande du salon","placeholder.profile_name":"ex. Télécommande du salon","placeholder.profile_help":"ex. Appuyez trois fois sur chaque bouton","accessibility.led_brightness":"Luminosité RVB","accessibility.close":"Fermer","dyn.online":"En ligne","dyn.frames_label":"Signaux","dyn.device_sub_simple":"Vos appareils RF enregistrés. Ouvrez un appareil uniquement lorsque vous souhaitez le modifier.","led.boot":"Démarrage","led.connecting":"Connexion au Wi-Fi","led.ready":"Prêt","led.offline":"Connexion perdue","led.radio_missing":"Radio absente ou non prête","led.learning":"Apprentissage","led.capture":"Signal reçu","led.learned":"Appareil confirmé","led.itho_pair":"Commande d’association Itho","led.tx":"Émission RF","led.ota":"Mise à jour du firmware","led.error":"Erreur","led.unknown":"Inconnu","decoder.compatibility_details":"Compatibilité et sources","decoder.control":"Commande","decoder.next_step":"Encore nécessaire","user.saved_help":"À utiliser seulement si RFLink ne connaît pas encore votre appareil ou si vous souhaitez créer un profil de modèle personnalisé.","user.profile_advanced":"Options avancées du profil","user.profile_auto_help":"Laissez l’ID vide pour le créer automatiquement à partir du nom du profil. Les actions sont préremplies depuis le décodeur lorsque c’est possible.","settings.developer_info":"Informations pour développeurs","placeholder.decoder_name":"ex. Télécommande du salon","user.id_auto_help":"Laissez ce champ vide pour un nouveau décodeur : RFLink créera automatiquement un ID stable à partir du nom. Modifiez-le uniquement si vous avez besoin d’un ID précis.","settings.mqtt_setup":"Configuration MQTT","settings.led_simple_help":"Réglez la luminosité ou désactivez le battement vert prêt. Les couleurs d’avertissement restent actives.","settings.led_more":"Couleurs et comportement de la LED","ui.advanced":"Avancé","ui.advanced_details":"Détails avancés","ui.advanced_filters":"Filtres avancés","ui.try_again":"Réessayer","ui.decoder_advanced_help":"Enregistrer et comparer les données radio brutes.","ui.freq_43392":"Le plus courant pour télécommandes, capteurs et interrupteurs","ui.freq_43342":"Souvent utilisé par Somfy RTS","ui.freq_868":"Souvent utilisé par Itho et d’autres appareils 868 MHz","ui.freq_other":"Utilisez ceci si votre appareil fonctionne sur cette fréquence","ui.frequency_help":"Vous hésitez ? 433,92 MHz est généralement le meilleur point de départ.","ui.device_found_help":"RFLink a trouvé l’appareil. Donnez-lui un nom puis enregistrez-le.","ui.protocol_label":"Protocole","ui.signal_seen":"Signal reçu","ui.back_add_device":"← Retour à Ajouter un appareil","somfy.control":"Commande Somfy","somfy.tx_disabled":"L’émission est désactivée dans le firmware. Activez les commandes TX avant l’appairage.","somfy.pair_help":"Mettez le moteur en mode programmation avec une télécommande existante, puis appairez RFLink.","somfy.pair":"Appairer RFLink","somfy.control_ready":"Commande Somfy prête à tester","somfy.up":"Monter","somfy.my":"My / Stop","somfy.down":"Descendre","somfy.no_ack":"RFLink ne peut pas confirmer la réaction du moteur par radio. Si le moteur ne réagit pas, recommencez l’appairage.","somfy.pair_again":"Appairer à nouveau","somfy.pair_confirm":"Mettez d’abord le moteur en mode programmation. Envoyer maintenant la commande PROG RFLink ?","somfy.pair_sent":"Commande d’appairage envoyée","somfy.pair_failed":"Échec de l’appairage","somfy.command_sent":"Commande envoyée","somfy.command_failed":"Échec de la commande","somfy.state":"Télécommande virtuelle {address} · prochain rolling code {counter}","led.pairing":"Association","itho.control":"Commande Itho","itho.tx_disabled":"L’émission est désactivée dans le firmware. Activez les commandes TX avant l’appairage.","itho.pair_help":"Coupez l’unité de ventilation pendant environ 15 secondes puis rallumez-la. Appairez RFLink dans les 2 minutes.","itho.pair":"Appairer RFLink","itho.control_ready":"Commande Itho prête à tester","itho.low":"Bas","itho.medium":"Moyen","itho.high":"Haut","itho.timer10":"Minuterie 10 min","itho.timer20":"Minuterie 20 min","itho.timer30":"Minuterie 30 min","itho.standby":"Veille","itho.no_ack":"RFLink ne peut pas confirmer directement que l’unité a enregistré l’appairage. Si elle ne réagit pas, recommencez ou utilisez l’appairage de compatibilité.","itho.compat_help":"Si l’appairage normal ne fonctionne pas, essayez l’appairage de compatibilité.","itho.pair_again":"Appairer à nouveau","itho.compat_pair":"Appairage de compatibilité","itho.unpair":"Désappairer RFLink","itho.pair_confirm":"Coupez d’abord l’unité pendant environ 15 secondes puis rallumez-la. Envoyer maintenant la commande d’appairage RFLink ?","itho.compat_confirm":"Coupez d’abord l’unité pendant environ 15 secondes puis rallumez-la. Envoyer maintenant l’appairage de compatibilité ?","itho.unpair_confirm":"Remettez d’abord l’unité en mode d’enregistrement. Désappairer RFLink maintenant ?","itho.pair_sent":"Commande d’appairage envoyée","itho.pair_failed":"Échec de l’appairage","itho.unpair_sent":"Commande de désappairage envoyée","itho.unpair_failed":"Échec du désappairage","itho.command_sent":"Commande envoyée","itho.command_failed":"Échec de la commande","itho.state":"Télécommande virtuelle {address} · prochain compteur {sequence} · type {type}","diag.checking":"Vérification","diag.cc1101_failed":"CC1101 non détecté","diag.rx_failed":"CC1101 OK · défaut RX","diag.tx_failed":"CC1101 OK · défaut TX","diag.ok":"Prêt","diag.spi_ok":"Détecté / initialisé","diag.spi_failed":"Échec d'initialisation","diag.cc1101_spi":"CC1101 / SPI","diag.rx_path":"Chaîne de réception","diag.tx_path":"Chaîne d'émission","diag.ready":"Prêt","diag.failed":"Échec","diag.configured_pins":"Broches configurées","diag.hardware":"Matériel","diag.cc1101_hint":"Le CC1101 échoue avant la réception ou l'émission. Vérifiez 3,3 V, GND, CSN et les lignes SCK/MOSI/MISO partagées. GDO n'est pas le premier suspect ici.","diag.spi_probe":"Test SPI en lecture seule","diag.probe_response":"Réponse valide","diag.probe_no_response":"Aucune réponse valide","diag.spi_checks":"Contrôles SPI","diag.spi_failures":"Erreurs SPI","diag.spi_consecutive":"Erreurs consécutives","diag.spi_recovery_attempts":"Tentatives de récupération","diag.spi_recovery_successes":"Récupérations réussies","diag.spi_health":"Santé SPI numérique","diag.spi_health_note":"Le logiciel peut détecter des réponses SPI invalides, mais pas les réflexions, oscillations ou dépassements électriques. Utilisez un oscilloscope ou un analyseur logique pour l’intégrité du signal.","monitor.show_noise":"Afficher le bruit RF suspect","monitor.noise_help":"Le bruit suspect est masqué par défaut, mais reste disponible pour le diagnostic et l’export.","monitor.copy":"Copier","monitor.export":"Exporter","monitor.copied":"Activité visible copiée.","monitor.exported":"Export d’activité téléchargé.","monitor.noise_badge":"Bruit RF suspect","monitor.noise_reason":"Raison","diag.noise_frames":"Bruit suspect","diag.freq_offset":"Décalage de fréquence","diag.uptime":"Durée de fonctionnement","diag.boot_count":"Démarrages enregistrés","diag.reset_reason":"Dernière cause de redémarrage","diag.boot_log":"Démarrages récents","diag.boot_log_hint":"Enregistrés sur l’ESP32, sans horodatage (pas d’horloge garantie au démarrage). Plus récents en premier.","diag.log_download":"Exporter le diagnostic","diag.heap_free":"RAM libre","diag.heap_min":"RAM libre minimale","diag.boot_unavailable":"Historique non sauvegardé en mémoire flash","diag.reboot_observed":"Redémarrage détecté pendant l’ouverture de cette page","diag.counter_reset":"Compteur de signaux diminué sans redémarrage de l’ESP32","diag.since_boot":"Les compteurs couvrent le démarrage actuel","diag.no_events":"Aucun événement observé pendant l’ouverture de cette page","about.heading":"À propos de RFLink Universal","about.version":"Version du firmware","about.revision":"Révision installée","about.build_more":"Informations de compilation","about.built":"Compilé le","rxlog.title":"Journal de réception","rxlog.signals":"Signaux","rxlog.noise":"Bruit","rxlog.all":"Tout","rxlog.radio":"Radio","rxlog.all_radios":"Toutes les radios","rxlog.settings":"Paramètres de journalisation","rxlog.signal_limit":"Nombre maximal de signaux","rxlog.noise_limit":"Nombre maximal d'événements de bruit (toutes radios)","rxlog.noise_floor":"Conservation minimale indicative du bruit par radio","rxlog.memory":"Mémoire du journal","rxlog.download":"Télécharger les journaux (ZIP)","rxlog.saved":"Enregistré sans redémarrage","rxlog.memory_eviction":"Le bruit est supprimé en premier si la mémoire manque","rxlog.type":"Type de journal","rxlog.settings_help":"Modifiez les limites sans redémarrer. Les réglages sont mémorisés. Le bruit est supprimé en premier.","rxlog.radio0":"Signaux 433,42 MHz","rxlog.radio1":"Signaux 433,92 MHz","rxlog.radio2":"Signaux 868,30 MHz","rxlog.save":"Enregistrer les paramètres","rxlog.older":"Charger les événements précédents","rxlog.stored":"Conservé","rxlog.noise_info":"Le bruit est enregistré ensemble, mais filtrable par radio.","rxlog.invalid":"Limites invalides (maximum total : 4096).","rxlog.exporting":"Préparation de l’export…","rxlog.exported":"ZIP téléchargé","rxlog.token_error":"Autorisation d’écriture indisponible","settings.general":"Général et voyant d’état","settings.integrations":"Home Assistant et système","settings.additional":"Autres réglages et outils","settings.dashboard_hint":"Réglages courants visibles. Ouvrez uniquement les outils nécessaires.","nav.back_settings":"← Retour aux paramètres","nav4.search_devices":"Rechercher des appareils","nav4.search_placeholder":"Nom, protocole ou radio…","nav4.unknown_radio":"Radio inconnue","nav4.clear":"Effacer","nav4.found":"{visible} appareils affichés sur {total}","nav4.no_results":"Aucun appareil trouvé. Modifiez les filtres.","nav4.mqtt_host":"Adresse du broker","nav4.mqtt_port":"Port","nav4.mqtt_user":"Utilisateur","nav4.mqtt_pass":"Mot de passe (vide = conserver)","nav4.mqtt_enable":"Activer MQTT","nav4.mqtt_save":"Enregistrer et connecter","nav4.mqtt_not_built":"MQTT absent du firmware. Activez le paquet optionnel une fois.","nav4.mqtt_security":"Stocké sur l’appareil ; le NVS standard n’est pas chiffré. Réseau fiable requis.","nav4.mqtt_saved":"Paramètres enregistrés. Connexion…","nav4.mqtt_bad":"Paramètres MQTT invalides ou erreur de stockage","devices.radio_count":"{n} appareils","profileflow.choose":"Profil de l’appareil","profileflow.none":"Choisir un profil","profileflow.suggested":"Suggéré","profileflow.select_help":"Choisissez un profil pour afficher ses fonctions. Le modèle exact n’est pas vérifié.","profileflow.manual_warning":"Correspondance possible ; le signal ne confirme pas la marque ni le modèle.","profileflow.proposal_warning":"Suggestion fondée sur le protocole et les actions détectées ; vérifiez le modèle.","profileflow.user_choice":"Profil choisi manuellement.","profileflow.after_profile":"Commandes connues","profileflow.sensor_profile":"Profil de capteur : valeurs affichées dès réception.","profileflow.choose_first":"Choisissez un profil pour voir les fonctions.","profileflow.measurements":"Mesures","profileflow.sensor_info":"Les valeurs apparaissent à leur réception ; aucun bouton TX nécessaire.","profileflow.operation":"Commande","profileflow.no_commands":"Aucune commande connue pour ce profil.","profileflow.tx_disabled":"Boutons indicatifs : émission RF désactivée.","profileflow.pair_first":"Associez d’abord l’émetteur RFLink.","profileflow.not_ready":"Commande indisponible. TX et radio doivent être prêts.","profileflow.tx_ready":"Commande disponible ; aucun accusé de réception garanti.","profileflow.functions_advanced":"Fonctions reçues","profileflow.saved":"Profil enregistré","profileflow.save_error":"Échec de l’enregistrement du profil","profileflow.send_confirm":"Envoyer {command} ?","profileflow.sent":"Commande d’émission demandée","profileflow.send_error":"Échec de la commande","profileflow.command.auto":"Auto","profileflow.command.low":"Bas","profileflow.command.medium":"Moyen","profileflow.command.high":"Haut","profileflow.remove_tx_locked":"Désactivez la transmission avant de supprimer les données techniques.","profileflow.dismiss_candidate":"Supprimer la suggestion","profileflow.dismiss_confirm":"Supprimer ce signal non enregistré ? Il pourra réapparaître.","profileflow.dismissed":"Suggestion supprimée","profileflow.dismiss_failed":"Suppression impossible","profileflow.setup":"Installation guidée","profileflow.setup_rx_only":"Ce guide concerne l'identification et la réception. L'appairage et l'émission RFLink attendent une validation matérielle.","profileflow.command.away":"Absence","profileflow.command.timer_15":"15 min","profileflow.command.timer_30":"30 min","profileflow.command.timer_60":"60 min","profileflow.command.bypass_auto":"Bypass auto","profileflow.command.bypass_open":"Ouvrir bypass","profileflow.command.bypass_close":"Fermer bypass","catalog.protocols":"Protocoles","catalog.profiles":"Profils des appareils","catalog.search_placeholder":"Rechercher marque, modèle ou protocole…","catalog.all_brands":"Toutes les marques","catalog.all_families":"Toutes les familles","catalog.all_protocols":"Tous les protocoles","catalog.profile_uses_protocol":"Décodeur","catalog.show_related":"Voir les profils des appareils","catalog.select_profile":"Sélectionnez un profil pour afficher ses détails.","catalog.custom_brand":"Personnel","catalog.custom_family":"Mes profils","catalog.protocol":"Protocole","catalog.no_help":"Aucune instruction d’apprentissage.","catalog.operation":"État des commandes","catalog.tx_profile_warning":"Les commandes nécessitent un encodeur adapté, un appairage et une radio vérifiée.","catalog.tx_not_ready":"Les fonctions sont répertoriées, mais la transmission est indisponible.","catalog.rx_only":"Réception uniquement ; aucune commande radio.","catalog.profile_functions":"Fonctions du profil","catalog.received_actions":"Actions reçues","catalog.setup_steps":"Étapes de configuration","catalog.profile_source":"Source du profil","catalog.profile_protocol_note":"Les profils décrivent les appareils ; les décodeurs traitent les signaux radio.","catalog.profile_count":"{shown} profils sur {total}","catalog.no_results":"Aucun profil correspondant aux filtres.","action.auto":"Auto","action.away":"Absent","wizard.setup_title":"Configurer appareil","wizard.start":"Configuration guidée","wizard.step":"Étape","wizard.close":"Fermer","wizard.back":"Retour","wizard.next":"Suivant","wizard.finish":"Terminé (réception)","wizard.check":"Vérifier la réponse","wizard.rx_only":"Continuer sans émission","wizard.transmit":"Envoyer la commande association","wizard.identity_help":"Vérifiez que l’identité RF appartient à cet appareil ou à sa télécommande. Confirmez le modèle.","wizard.finish_help":"Vérifiez le résultat. Émettre ne prouve pas que l’appareil est associé.","wizard.observe_help":"Recevez un signal et vérifiez son identité.","wizard.choose_profile":"Choisissez d’abord un profil.","wizard.rx_ready":"Réception prête; association non requise.","wizard.sent_not_verified":"Commande envoyée, sans accusé de réception confirmé.","wizard.not_supported":"Association non disponible.","wizard.requires_setup":"Instructions présentes, association non confirmée.","wizard.orcon_tx_missing":"Orcon : émission et échange 1FC9 RAMSES-II non validés physiquement. Aucune association possible.","wizard.tx_unavailable":"Émetteur ou encodeur approuvé non disponible ; réception seule.","wizard.verify_observed":"J’ai observé une réaction","wizard.verify_question":"Avez-vous observé la réaction de l’appareil ? Ce n’est pas un accusé RF.","wizard.transmit_confirm":"Le récepteur est-il en mode association ? Envoyer la commande approuvée ?","ux3.change_profile":"Modifier le profil de l’appareil","ux3.select_profile":"Choisir un profil d’appareil","ux3.no_profile":"Non sélectionné","ux3.not_in_ha":"Non publié sur Home Assistant","ux3.manage_details":"Gérer l’appareil et les signaux RF","ux3.not_yet_available":"La commande n’est pas encore disponible.","ux3.finish_rx_only":"Terminer (réception seule)","ux3.orcon_setup_short":"La réception fonctionne; l’appairage RF est encore en cours de test.","ux3.rx_only_short":"Réception possible, commande RF pas encore prête.","ux3.receive_only_status":"Réception uniquement","ux3.control_ready_status":"Commande disponible","ux3.sensor_status":"Mesures reçues","ux3.profile_missing_status":"Choisir un profil","debug.menu":"Journal système","debug.title":"Journal système","debug.help":"Décisions de RFLink: décodage, appareil, profil, association, émission et retour en réception. Indépendant des logs RF.","debug.lab_gate":"Laboratoire GDO TX","debug.probe_gate":"Test RAMSES TX","debug.radio":"État radio 868 MHz","debug.filter":"Afficher","debug.all":"Tout","debug.live":"Direct","debug.refresh":"Actualiser","debug.export":"Exporter le journal","debug.empty":"Aucun événement système.","debug.experiment":"Avancé : test de transmission RAMSES","debug.experiment_help":"Envoie une trame 10E0 de test, sans commande ni association. Activez les deux options YAML. Vérifiez avec un autre récepteur.","debug.send":"Émettre une trame test","debug.privacy":"128 événements en RAM (effacés au redémarrage). Les ID peuvent figurer dans le journal. Aucun mot de passe ni trame RF brute.","debug.fetch_failed":"Impossible de lire les événements :","debug.enabled":"Activé","debug.disabled":"Désactivé","debug.ready":"Radio prête","debug.not_ready":"Non prête","debug.fault":"Défaut de restauration RX","debug.count":"{shown} sur {total} événements","debug.dropped":"Perdus","debug.confirm":"Envoyer UNE trame expérimentale RAMSES 10E0 sur 868 MHz ? Aucun appairage ni commande.","debug.sending":"Émission du test…","debug.sent_unverified":"Adaptateur : envoi et reprise RX signalés. RF et réponse de l’appareil NON confirmés.","debug.failed":"Échec du test","logbook.title_nav":"▤ Journaux","logbook.title":"Journaux","logbook.subtitle":"Radio, décisions du firmware et actions du navigateur.","logbook.all":"Tout","logbook.radio":"Radio","logbook.system":"Système","logbook.web":"Web","logbook.level":"Niveau","logbook.any":"Tous","logbook.warnings":"Alertes et erreurs","logbook.errors":"Erreurs","logbook.search":"Rechercher","logbook.search_hint":"Appareil, ID, action…","logbook.noise":"Inclure bruit","logbook.live":"Direct","logbook.refresh":"Actualiser","logbook.export":"Exporter","logbook.note":"Événements web limités à ce navigateur. Les heures du navigateur sont approximatives. Les données API ne sont pas journalisées.","logbook.system_limit":"Événements système (ESP32)","logbook.web_limit":"Événements web (navigateur)","logbook.verbose":"Débogage web : inclure actualisations API réussies","logbook.settings_note":"Limites système réinitialisées au redémarrage ; préférences du navigateur locales.","logbook.empty":"Aucun événement.","logbook.events":"événements","logbook.invalid":"Limites incorrectes.","logbook.saved":"Paramètres enregistrés.","logbook.save_failed":"Échec de sauvegarde :","orcon.experimental_status":"Appairage et commandes expérimentaux; options labo requises.","orcon.enable_lab":"Activer les options TX, GDO UART labo et RAMSES labo dans YAML.","orcon.wait_ack":"Attendre la réponse correspondant au ventilateur.","orcon.status_waiting_ack":"Offre 1FC9 envoyée; attente de l’ACK (3 minutes max).","orcon.status_confirming":"ACK reçu; confirmation après retour en réception.","orcon.status_awaiting_user":"ACK et confirmation envoyés. Confirmer uniquement après réaction réelle du ventilateur.","orcon.status_failed":"Appairage échoué ou expiré; commandes désactivées.","orcon.status_paired":"Appairage confirmé par utilisateur; commandes expérimentales disponibles.","orcon.status_not_paired":"Pas encore appairé.","orcon.test_auto":"Tester Auto (1 fois)","orcon.confirm_test_auto":"Envoyer une seule commande Auto expérimentale ? Observez la réaction du ventilateur.","orcon.sent_unconfirmed":"Auto envoyé ; réponse du ventilateur non confirmée.","deviceux.back":"← Retour aux appareils","deviceux.detail_help":"Configurez, associez ou gérez cet appareil ici. Les commandes restent accessibles dans la liste.","deviceux.setup_device":"Configurer l’appareil","deviceux.view_details":"Voir les détails","deviceux.settings":"Réglages et association","deviceux.no_profile_help":"Choisissez le modèle de l’appareil pour voir ses fonctions.","deviceux.profile_unavailable":"Le profil enregistré n’a pas pu être chargé. Sélectionnez un profil ; l’identité radio est conservée.","logs.group_radio":"Radio","logs.radio_help":"Signaux par radio et bruit partagé.","logs.group_system":"Système","logs.system_help":"Décisions sur les décodeurs, appareils, appairage et émission.","logs.group_web":"Web","logs.web_help":"Navigation, clics et erreurs API de cette session.","version.subtitle":"Commande des appareils, journaux unifiés et émission expérimentale.","version.notes":"Nouveautés et détails techniques","version.changes":"Nouveautés","version.raw":"Version interne du micrologiciel","version.technical":"Détails techniques","version.legacy":"Micrologiciel installé (ancien nom de version).","version.legacy_note":"Consultez les notes d’origine pour cette ancienne version.","version.note1":"Vue des appareils simplifiée et réglages/appairage séparés.","version.note2":"Événements radio, système et web dans un journal à hauteur fixe.","version.note3":"Réglages des journaux regroupés et bibliothèque visible directement.","version.note4":"Mesures des temps API pour diagnostiquer les problèmes.","version.lab_tools":"Laboratoire TX et diagnostic système","version.lab_help":"Cette page sert au diagnostic expérimental radio/TX. Pour les événements habituels, ouvrez Journaux → Système.","version.open_logs":"Ouvrir le journal système","deviceux.remove_short":"Supprimer","logbook6.frequency":"Fréquence","logbook6.time":"Heure","logbook6.source":"Source","logbook6.event":"Événement","logbook6.filtered_empty":"Aucun événement correspondant. Activez Afficher le bruit pour inclure le bruit radio.","deviceux6.profile_title":"Choisir le profil d’appareil","deviceux6.profile_help":"Choisissez le modèle pour afficher les commandes et les étapes d’appairage.","deviceux6.pair_title":"Associer et tester","deviceux6.pair_help":"Associez cet appareil à RFLink si le profil le permet.","deviceux6.status_title":"État actuel","deviceux6.no_profile_yet":"Les commandes apparaîtront après le choix d’un profil.","deviceux6.extra_title":"Gestion supplémentaire","deviceux6.intro":"Choisissez un profil, associez l’appareil si nécessaire et gérez les options.","logbook6.info":"Info","logbook6.warn":"Alerte","logbook6.error":"Erreur","logbook6.noise":"Bruit","diag7.event_type":"Type","diag7.boot_filter":"Plantages et redémarrages","diag7.tx_filter":"Émission et association","diag7.api_filter":"API et navigateur","diag7.view_log":"Voir le journal de plantage","diag7.crashes":"{n} plantages sur les 8 derniers démarrages","diag7.crash_help":"RFLink a redémarré après une erreur. La raison exacte nécessite un diagnostic supplémentaire.","diag7.export":"Rapport de diagnostic (JSON)","diag7.uptime_note":"Durées hh:mm:ss,mmm depuis le démarrage de l’ESP32 ; heures navigateur approximatives.","diag7.health_attention":"Attention : plantages précédents","diag7.health_detail":"Les radios et décodeurs sont accessibles, mais RFLink a redémarré après des erreurs.","diag7.raw_trace":"Trace technique (optionnel)","version.source_hash":"Empreinte source (SHA-256 abrégé)","version.history_time":"Démarrage antérieur","version.history_event":"Démarrage n°{n} · cause du redémarrage : {reason}","version.history_note":"Historique NVS sans heure exacte, cause profonde ni pile d’appels.","version.note5":"v0.6.1 Radio Gaga : préparation expérimentale CC1101/GDO-UART améliorée.","version.note6":"UART1 est installé avant sa configuration 8N1, avec des erreurs détaillées par étape.","version.note7":"Le journal radio montre les demandes TX, les étapes modem/UART, les échecs et le retour en RX.","version.note8":"Les détails des appareils sont simplifiés : fonctions, Home Assistant et gestion RF sans panneau imbriqué.","crashtrace.reason":"Cause du plantage","crashtrace.core":"Cœur CPU","crashtrace.firmware":"Version du firmware du plantage","crashtrace.other_build":"Ancienne/autre compilation (ELF originale requise)","crashtrace.unknown_build":"Compilation inconnue","crashtrace.backtrace":"Pile d’appels","crashtrace.other_core":"Autre cœur","crashtrace.elf_note":"Adresses brutes : décoder avec la firmware.elf exacte de la compilation ayant planté. Heure historique inconnue.","crashtrace.available":"Pile disponible","crashtrace.boot_event":"Pile du plantage au démarrage n°{n}","crashtrace.unknown_boot":"Ancienne pile de plantage (numéro inconnu)","diag55.tab":"Diagnostic","diag55.title":"Tous les diagnostics au même endroit","diag55.intro":"État radio et SPI, redémarrages, détails des plantages et TX expérimental. Les événements figurent dans les journaux Radio et Système.","diag55.radio_heading":"Radios, SPI et historique de démarrage","diag55.experimental_heading":"TX expérimental et décisions système","diag55.open_crashes":"Voir les incidents","diag55.open_radio":"Voir le journal radio","diag55.spi_filter":"SPI / récupération radio","diag55.boots":"Plantages et redémarrages","diag55.radio":"Radios et SPI","diag55.memory":"Mémoire","diag55.lab":"Expérimental","diag55.export":"Rapport diagnostic (JSON)","diag55.boot_hint":"Redémarrages antérieurs et traces disponibles, avec numéros de démarrage si la date manque.","diag55.lab_hint":"TX, UART/GDO et diagnostic avancé. Aucun envoi de test automatique.","diag55.ready":"réception prête","diag55.crashes":"plantages","diag55.internal":"Mémoire interne libre","diag55.trace":"Détails du plantage","diag55.nohistory":"Aucun historique.","diag55.online":"en ligne","diag55.offline":"hors ligne","diag55.errors":"erreurs","diag55.restores":"récupérations","diag55.no_radio":"Aucune radio.","diag55.lab_enabled":"Lab activé","diag55.lab_disabled":"Lab désactivé","diag55.adapter_ready":"adaptateur prêt","diag55.adapter_unready":"adaptateur non prêt","pair55.queued":"Appairage mis en file. Consultez les journaux Radio et Système.","pair55.auto_queued":"Test Auto mis en file. Aucune émission RF confirmée.","orcon.status_queued":"Appairage mis en file. Consultez les journaux Radio et Système.","rf60.status_intro":"Vue claire de votre système, de vos radios et des points à surveiller.","rf60.system":"Système","rf60.radios":"Radios","rf60.radios_help":"Radios disponibles","rf60.devices":"Appareils","rf60.saved":"Appareils enregistrés","rf60.attention":"Points d’attention","rf60.attention_title":"À vérifier","rf60.attention_help":"Uniquement les éléments nécessitant une action.","rf60.no_problems":"Aucun problème détecté.","rf60.needs_attention":"À vérifier","rf60.radio_overview":"Aperçu des radios","rf60.radio_help":"Ouvrez une radio pour examiner la réception et l’émission.","rf60.activity_help":"Derniers événements ; l’historique complet est dans les journaux.","rf60.open_logs":"Ouvrir tous les journaux","rf60.system_details":"État technique du système","rf60.quick_actions":"Actions rapides","rf60.quick_help":"Tâches courantes, directement accessibles.","rf60.last_seen":"Dernier signal","rf60.not_seen":"Pas encore observé","rf60.more_details":"Plus de détails","rf60.control":"Commander","rf60.values":"Valeurs","rf60.no_values":"Aucune valeur mesurée disponible.","rf60.protocol":"Protocole","rf60.controls_title":"Commandes et fonctions","rf60.advanced_details":"Signaux et détails techniques","rf60.advanced_help":"Signaux reçus, identifiants RF, Home Assistant et maintenance.","rf60.remove_help":"Supprime cet appareil et sa configuration RFLink. Une confirmation est nécessaire.","rf60.inspect":"Voir le diagnostic","rf60.history_issues":"Plantages précédents : {n}","rf61.functions_title":"Fonctions reçues","rf61.functions_help":"{n} fonctions reconnues. Vérifiez les nouvelles fonctions détectées.","rf61.ha_title":"Gestion et Home Assistant","rf61.ha_help":"Ajoutez cet appareil à Home Assistant ou renommez-le.","rf61.signals_title":"Gérer les signaux RF","rf61.signals_help":"Consultez ou remplacez les codes RF associés.","rf61.guide_title":"Aide à la configuration","rf61.guide_help":"Instructions pour le profil choisi.","rf61.id_title":"Identifiant technique","rf61.id_help":"Identifiant interne pour l’assistance et le débogage.","version.note9":"v0.6.2 Radio Gaga : un seul lanceur pour les mises à jour, les versions propres et les tests ; documentation regroupée dans docs/."},"de":{"static.000":"RFLink Universal","static.001":"● Online","static.002":"0 Signale","static.003":"Laufzeit —","static.004":"◉ Status","static.005":"▣ Geräte","static.006":"◷ Aktivität","static.007":"◇ Gerät hinzufügen","static.008":"⚙ Einstellungen","static.009":"Status","static.010":"Alles für den Alltag. Technische Details findest du eine Ebene tiefer.","static.011":"System wird geprüft …","static.012":"Status wird geladen.","static.013":"Diagnose anzeigen","static.014":"Funkstatus wird geladen …","static.015":"RGB-Statusleuchte","static.016":"Aktueller Status","static.017":"Wird geladen …","static.018":"LED","static.019":"WS2812 · GPIO48","static.020":"Schnellaktionen","static.021":"+ Gerät hinzufügen","static.022":"Fernbedienung, Sensor oder Schalter finden und speichern.","static.023":"Pairing","static.024":"Geräte mit Pairing-Unterstützung anzeigen.","static.025":"Unbekanntes Protokoll","static.026":"Der Entwicklermodus vergleicht Aufzeichnungen und schlägt Code vor.","static.027":"Fehler beheben","static.028":"Diagnose, Decoderbibliothek und technische Informationen.","static.029":"Letzte Aktivität","static.030":"Noch keine Aktivität.","static.031":"Geräte","static.032":"RFLink erstellt ein Gerät erst, nachdem ein Signal oft genug erkannt und von dir bestätigt wurde. RF-Codes lassen sich pro Gerät anzeigen, lösen oder sicher ersetzen.","static.033":"Hier stehen Geräte mit Einrichtungsanleitung. RF-Senden ist möglicherweise noch nicht verfügbar.","static.034":"Gerätespeicher wird geprüft …","static.035":"Geräte werden geladen …","static.036":"Neue Signale","static.037":"Kandidaten werden geladen …","static.038":"Aktivität","static.039":"Kürzlich von RFLink erkannte Signale.","static.040":"Nach Frequenz filtern","static.041":"Status wird geladen …","static.042":"Richtung","static.043":"RX und TX","static.044":"Nur RX","static.045":"Nur TX","static.046":"TX-Einträge zeigen Befehle an den Funk-Treiber, nicht die Bestätigung des Zielgeräts. Rohe Funkdaten sind für die Fehlersuche gedacht.","static.047":"Aktivität aller Funkmodule","static.048":"Aktivität wird geladen…","static.049":"Gerät hinzufügen","static.050":"Frequenz wählen, Start drücken und das Gerät benutzen.","static.051":"Gerät hinzufügen","static.052":"Für den Alltag","static.053":"Decoder erstellen","static.054":"Nur nötig, wenn RFLink das Protokoll noch nicht kennt.","static.055":"Frequenz wählen","static.056":"Funkstatus wird geprüft …","static.057":"Gerät hinzufügen","static.058":"Start drücken und das Gerät danach ein- oder mehrmals benutzen.","static.059":"Start","static.060":"Stoppen","static.061":"Gerätename","static.062":"Funktionen","static.063":"Wähle die Funktionen aus, die du verwenden möchtest.","static.064":"Zu Home Assistant hinzufügen","static.065":"Technische Funkdaten werden automatisch gespeichert.","static.066":"Speichern","static.067":"Mit vorhandenem Gerät verknüpfen","static.068":"Technische Messung","static.069":"Halte das Gerät etwa 1 Meter von der richtigen Antenne entfernt. Drücke noch keine Taste.","static.070":"Hintergrund messen (5 Sek.)","static.071":"Auf Gerät lauschen","static.072":"Stoppen und Messung löschen","static.073":"Hintergrund","static.074":"Noch nicht gemessen","static.075":"Wiederholungen","static.076":"—","static.077":"Protokoll","static.078":"Identität","static.079":"Signal","static.080":"Warte auf Aufzeichnung","static.081":"Wenn RFLink das Gerät mehrfach zuverlässig erkannt hat, erscheint es unter","static.082":"Geräte → Neue Signale","static.083":". Dort kannst du es benennen oder einem vorhandenen Gerät zuordnen.","static.084":"Decoder-Assistent","static.085":"RFLink erzeugt keine Firmware auf dem ESP32 selbst. Es bietet eine sichere Vorlage für einen neuen Decoderordner.","static.086":"Neuen Ordner erstellen","static.087":"python tools/new_decoder.py mein_protokoll \"Mein Protokoll\"","static.088":"Anschließend ändern","static.089":"nur decoder.cpp + manifest.json","static.090":"Codevorschlag anzeigen","static.091":"Erstelle zuerst eine Aufzeichnung.","static.092":"Einstellungen & Hilfe","static.093":"Erst klare Anleitungen, technische Details nur bei Bedarf.","static.094":"WS2812 an GPIO48. Orange blinkend: mindestens ein Funkmodul fehlt oder ist nicht bereit. Weiß: OTA, Rot: Fehler, Grün: alles bereit. Der grüne Herzschlag lässt sich ohne Ausblenden von Warnungen deaktivieren.","static.095":"Helligkeit","static.096":"20%","static.097":"Grüner Herzschlag bei Bereitschaft","static.098":"Aktiviert: Grün blinkt bei Bereitschaft (zwei kurze Blitze alle 6 Sekunden). Deaktiviert: keine grüne Bereitschaftsanzeige. Für den ersten Test 100 % Helligkeit einstellen und speichern.","static.099":"Aktueller Status","static.100":"LED-Einstellungen speichern","static.101":"Einstellungen werden auf dem ESP32 gespeichert und überstehen Neustarts. Die Webseite ist nur für vertrauenswürdige Netze vorgesehen.","static.102":"Was möchte ich tun?","static.103":"Neues Gerät","static.104":"Anlernen öffnen","static.105":"Pairing-Geräte","static.106":"Home Assistant (MQTT Discovery)","static.107":"RF-Geräte werden einzeln unter Geräte hinzugefügt. Der ESP32 bleibt über die ESPHome-API verbunden.","static.108":"MQTT-Broker","static.109":"Wird geprüft …","static.110":"Aktiviere optional das MQTT-Paket in YAML und trage die Broker-Zugangsdaten in secrets.yaml ein. Das Entfernen aus Home Assistant erfordert eine funktionierende Brokerverbindung.","static.111":"HA-Geräte verwalten","static.112":"Decoderbibliothek","static.113":"Was erkennt RFLink, was kommt bei Home Assistant an und was lässt sich steuern? Softwaretests sind keine Funktests.","static.114":"Anzeigen","static.115":"Alle Protokolle","static.116":"Fernbedienungen und Steuerung","static.117":"Sensoren","static.118":"Entwicklungsbeispiele","static.119":"Bibliothek wird geladen …","static.120":"Technisches Decoderregister","static.121":"Wartungsregel","static.122":"Ein Decoder sollte nur eine Aufgabe haben:","static.123":"RFFrame → DecodeResult","static.124":"Keine Web-UI, Home Assistant, GPIO-Einstellungen oder Logik anderer Decoder in einem Protokollmodul.","static.125":"Diagnose","static.126":"Funk- und Decoderstatus wird geladen.","static.127":"Gespeicherte Geräte","static.128":"Decoderregister","static.129":"Speicher","static.130":"Empfangen","static.131":"Decodiert","static.132":"Unbekannt","static.133":"Technische Erklärung","static.134":"Ein Puls ist eine Zeitspanne, in der das empfangene RF-Signal HIGH oder LOW ist. Ein Bit kann mehrere Pulse enthalten. Der Entwicklermodus vergleicht anhand der Zeitwerte Protokolle.","static.135":"Monitor","static.136":"Einstellungen","static.137":"Gib einen erkennbaren Namen ein. Die RF-Identität wird intern gespeichert.","static.138":"Funktionen prüfen","static.139":"Abbrechen","static.140":"Speichern","language.title":"Sprache und Region","language.help":"Wähle die Sprache der Oberfläche. Die Einstellung wird auf dem ESP32 gespeichert.","language.label":"Oberflächensprache","language.saving":"Sprache wird gespeichert …","language.saved":"Sprache gespeichert.","language.failed":"Die Sprache konnte nicht auf dem ESP32 gespeichert werden. Vorherige Sprache wiederhergestellt.","dyn.now":"jetzt","dyn.sec":"{n} Sek.","dyn.min":"{n} Min.","dyn.hour":"{n} Std.","dyn.dayhour":"{d} T {h} Std.","dyn.uptime":"Laufzeit {value}","dyn.frames":"{n} Signale","action.on":"Ein","action.off":"Aus","action.up":"Hoch","action.down":"Runter","action.my":"Favorit (My)","action.prog":"Programmieren","action.low":"Niedrig","action.medium":"Mittel","action.high":"Hoch","action.standby":"Standby","action.status":"Status","action.measurement":"Messung","action.rain_update":"Regenmessung","action.weather_update":"Wettermessung","action.code":"Fester Code","action.button":"Taste {n}","action.command":"Türklingelcode {n}","action.timer":"Timer {n} Minuten","dyn.decoder_review":"Decoder {protocol}: erkannte Funktionen vor dem Speichern prüfen.","dyn.decoder_missing":"Decoderinformationen fehlen. RFLink kann die Funktionen nicht sicher prüfen. Seite aktualisieren und erneut versuchen.","dyn.dev_instruction":"Testgerät etwa 1 Meter entfernt halten. Zuerst den Hintergrund messen, danach eine physische Aktion mehrmals ausführen.","dyn.device_instruction":"Gerät etwa 1 Meter von der richtigen Antenne entfernt halten. Noch keine Taste drücken.","dyn.rx_ready":"● RX bereit","dyn.rx_unavailable":"● RX nicht verfügbar","dyn.disconnected":"● Nicht verbunden","dyn.checking":"● Prüfen","dyn.rx_ready_short":"RX bereit","dyn.rx_fault":"RX nicht verfügbar","dyn.not_connected":"Nicht verbunden","dyn.checking_short":"Prüfen","dyn.radio_unavailable_title":"Radio oder Empfänger nicht verfügbar","dyn.no_radios":"Keine Radios konfiguriert","dyn.only_working_receivers":"Nur funktionierende Empfänger können ausgewählt werden.","dyn.radio_checking":"Radiostatus wird geprüft…","dyn.no_working_receivers":"Keine funktionierenden Funkempfänger: Module anschließen und ESP32 neu starten.","dyn.all_radios":"Alle Radios","dyn.overview":"Gesamtübersicht","dyn.radio_disconnected_title":"Funkmodul nicht verbunden","dyn.activity_radio":"Aktivität · {name}","dyn.activity_all":"Aktivität aller Radios","dyn.unknown":"Unbekannt","dyn.no_monitor_activity":"Keine aufgezeichnete Aktivität für diesen Filter. Fehlende Radios erzeugen keine neuen RX- oder TX-Einträge.","dyn.tx":"Senden","dyn.available":"Verfügbar","dyn.unavailable":"Nicht verfügbar","dyn.last_signal":"Letztes Signal","dyn.view_activity":"Aktivität ansehen","dyn.no_activity":"Noch keine Aktivität.","dyn.offline":"● Offline","dyn.esp_unreachable":"ESP32 nicht erreichbar","dyn.esp_unreachable_help":"Stromversorgung, Netzwerk und IP-Adresse prüfen. Offline werden keine Geräteänderungen ausgeführt.","dyn.api_unreachable":"API nicht erreichbar; Start deaktiviert.","dyn.connection_lost":"Verbindung zum ESP32 unterbrochen.","dyn.manage_signals":"Signale verwalten ({n})","dyn.signal_identity_help":"Die interne Geräte-ID {uid} bleibt unverändert. Einen RF-Code nur durch einen neuen Code ersetzen, den der Empfänger selbst mehrmals erkannt hat.","dyn.rf_signal":"RF-Signal {n}","dyn.replace_with":"Ersetzen durch","dyn.choose_learned":"Neu angelerntes Signal auswählen","dyn.replace":"Ersetzen","dyn.unlink":"Trennen","dyn.last_code_title":"Letzter Code: Gerät löschen oder Code ersetzen","dyn.no_matching_signal":"Kein passendes neues Signal gefunden. Zu Lernen wechseln, das neue Signal mindestens dreimal empfangen und zurückkehren, ohne es als neues Gerät zu speichern.","dyn.learn_new_signal":"Neues Signal anlernen","dyn.signal_history_help":"Erkannte Aktionen werden geräteweit gespeichert. Bei Protokollen, bei denen eine RF-Identität mehrere Tasten repräsentiert, kann RFLink eine einzelne Taste noch nicht zuverlässig trennen. Das Trennen eines RF-Codes löscht keine gemeinsame Aktionshistorie.","dyn.mqtt_remove_unavailable":"MQTT offline: Entfernen nicht verfügbar","dyn.approve":"Freigeben","dyn.remove":"Entfernen","dyn.approve_technical":"Technischen Wert freigeben","dyn.review_new_functions":"● {n} neue Funktion(en) prüfen","dyn.view_functions":"Funktionen ansehen und bearbeiten","dyn.approved_only_help":"Nur von Ihnen freigegebene Funktionen werden gespeichert und an Home Assistant veröffentlicht. Wertänderungen innerhalb bestehender Funktionen werden automatisch verarbeitet.","dyn.found_not_approved":"Gefunden, noch nicht freigegeben","dyn.no_new_visible":"Keine neuen sichtbaren Funktionen zur Prüfung.","dyn.approved_features":"Freigegebene Funktionen","dyn.none_approved":"Noch keine sichtbaren Funktionen freigegeben.","dyn.technical_radio_data":"Technische Funkdaten ({saved} gespeichert{pending})","dyn.pending_suffix":", {n} zu prüfen","dyn.technical_radio_help":"Diese Werte können für einen exakten Sendecode erforderlich sein. Nur ändern, wenn das Protokoll bekannt ist. RFLink stellt sie nicht als separate Home-Assistant-Sensoren bereit.","dyn.saved":"Gespeichert","dyn.mqtt_connected":"Verbunden","dyn.mqtt_disconnected":"Nicht verbunden","dyn.mqtt_not_configured":"Nicht konfiguriert","dyn.storage_ok":"Gespeicherte Geräte bleiben nach einem Neustart erhalten. Änderungen an RF-Codes erfordern einen bestätigten neuen Empfang.","dyn.storage_uncertain":"Speicherstatus ist unsicher. Geräte können nicht sicher geändert werden.","dyn.pairing_only":"Geräte mit Einrichtungsanleitung","dyn.device_sub":"Name, RF-Codes und Home-Assistant-Veröffentlichung pro Gerät verwalten.","dyn.mqtt_no_features_title":"MQTT nicht verbunden oder keine freigegebenen Funktionen","dyn.remove_from_ha":"Aus HA entfernen","dyn.add_to_ha":"Zu HA hinzufügen","dyn.receive_sensor":"Empfangssensor","dyn.rf_device":"RF-Gerät","dyn.rf_signals":"{n} RF-Signal(e)","dyn.in_ha":"In Home Assistant","dyn.only_rflink":"Nur RFLink","dyn.ha_control_allowed":"HA-Steuerung erlaubt","dyn.features_to_review":"{n} Funktion(en) zu prüfen","dyn.review_needed":"Prüfung erforderlich","dyn.manage_more":"Mehr verwalten: Name, Home Assistant und RF-Codes","dyn.changes_manual":"Änderungen werden erst nach Ihrer Aktion gespeichert. RFLink bleibt maßgeblich.","dyn.mqtt_offline":"MQTT offline","dyn.disable_ha_control":"HA-Steuerung deaktivieren","dyn.allow_ha_control":"HA-Steuerung erlauben","dyn.rename":"Umbenennen","dyn.dangerous_actions":"Gefährliche Aktionen","dyn.dangerous_help":"Damit wird das vollständige Gerät aus RFLink und, falls verknüpft, aus Home Assistant entfernt.","dyn.delete_device":"Gerät endgültig löschen","dyn.technical_device_id":"Technische Geräte-ID","dyn.no_pair_devices":"Noch keine bestätigten Pairing-Geräte.","dyn.no_devices":"Noch keine bestätigten Geräte. Über Lernen hinzufügen.","dyn.new_signal":"Neues Signal","dyn.received_times":"{n}-mal empfangen","dyn.ready_review":"Bereit zur Prüfung","dyn.still_listening":"Empfang läuft noch","dyn.check_signal":"Empfangenes Signal vor dem Speichern prüfen.","dyn.looks_like":"Ähnelt {name}. Nur verknüpfen, wenn es wirklich dasselbe Gerät ist.","dyn.review_add":"Prüfen und hinzufügen","dyn.more_technical":"Weitere Optionen und technische Details","dyn.rf_identity":"RF-Identität: {id}","dyn.link_to":"Mit {name} verknüpfen","dyn.not_saved":"Noch nicht gespeichert","dyn.pairing_view":"Pairing-Ansicht aktiv.","dyn.no_new_signals":"Keine neuen Signale erfordern Aufmerksamkeit.","dyn.learn_replacement_toast":"Neues Signal anlernen; nicht als separates Gerät speichern.","dyn.devices_load_failed":"Geräte konnten nicht geladen werden","dyn.confirm_unlink":"RF-Code {identity} trennen? Andere Codes des Geräts funktionieren weiter.","dyn.unlinked":"RF-Code getrennt","dyn.unlink_failed":"Trennen fehlgeschlagen; es wurde nichts geändert","dyn.choose_confirmed_signal":"Zuerst ein bestätigtes neues Signal auswählen","dyn.confirm_replace":"RF-Code ersetzen?\nAlt: {old}\nNeu: {new}\nGerätename und andere RF-Codes bleiben erhalten.","dyn.replaced":"RF-Code ersetzt und gespeichert","dyn.replace_failed":"Ersetzen fehlgeschlagen; alter RF-Code bleibt erhalten","dyn.storage_attention":"Speicher benötigt Aufmerksamkeit","dyn.decoder_incomplete":"Decoderbibliothek unvollständig","dyn.hardware_incomplete":"Hardware unvollständig","dyn.all_ready":"Alles bereit","dyn.storage_problem_help":"Die Verbindung funktioniert, Geräteänderungen können jedoch nicht sicher gespeichert werden. ESPHome-Logs prüfen.","dyn.decoder_problem_help":"Nicht alle erwarteten Decoder sind geladen. Empfangene Signale werden möglicherweise nicht erkannt.","dyn.hardware_problem_help":"ESP32 und Website funktionieren. {missing} von {total} Radios sind nicht verbunden oder nicht bereit.","dyn.ready_help":"ESP32, Decoderbibliothek und alle konfigurierten Radios sind bereit.","dyn.esp_web":"ESP32 und Weboberfläche","dyn.radios":"Radios","dyn.ready_count":"{ready}/{total} bereit","dyn.decoders":"Decoder","dyn.loaded":"{value} geladen","dyn.device_storage":"Gerätespeicher","dyn.check_required":"Prüfung erforderlich","dyn.saved_count":"{n} gespeichert","dyn.storage_error":"Speicherfehler","dyn.confirm_remove_feature":"Funktion „{key}“ aus RFLink und, falls verknüpft, auch aus Home Assistant entfernen?","dyn.feature_removed":"Funktion entfernt und gespeichert","dyn.feature_approved":"Funktion freigegeben und gespeichert","dyn.feature_failed":"Funktion wurde nicht geändert. MQTT und Gerätespeicher prüfen.","dyn.confirm_remove_ha":"„{name}“ aus Home Assistant entfernen? Das RF-Gerät bleibt in RFLink gespeichert.","dyn.ha_removed":"HA-Discovery entfernt; Gerät bleibt gespeichert","dyn.ha_sent":"HA-Discovery an Broker gesendet","dyn.ha_failed":"HA-Änderung fehlgeschlagen: MQTT und Speicher prüfen","dyn.confirm_disable_tx":"Steuerung von „{name}“ in Home Assistant deaktivieren?","dyn.confirm_enable_tx":"RF-Senden für „{name}“ erlauben? Nur von diesem Decoder unterstützte und freigegebene Tasten erscheinen in HA. Der tatsächliche Empfängerstatus wird nicht bestätigt.","dyn.tx_saved":"Sendefreigabe gespeichert","dyn.tx_failed":"Sendefreigabe unverändert; MQTT und TX-Konfiguration prüfen","dyn.new_device_name":"Neuer Gerätename:","dyn.name_length":"Name muss 1–80 Zeichen enthalten","dyn.name_saved":"Name gespeichert","dyn.rename_failed":"Umbenennen wurde nicht gespeichert","dyn.confirm_delete":"Gerät „{name}“ aus RFLink und, wenn aktiv, auch aus Home Assistant löschen? Dies kann nicht rückgängig gemacht werden; erneutes Anlernen ist erforderlich.","dyn.device_removed":"Gerät entfernt","dyn.delete_failed":"Löschen fehlgeschlagen","dyn.add_device":"Gerät hinzufügen","dyn.wizard_start_help":"Radio auswählen und Start klicken. Hintergrundmessung und Empfang erfolgen automatisch.","dyn.storage_unavailable":"Gerätespeicher nicht verfügbar","dyn.receiver_unavailable":"Dieser Empfänger ist nicht verfügbar","dyn.step1":"Schritt 1/3 · Hintergrund messen","dyn.step1_help":"Fünf Sekunden warten. Gerät noch nicht betätigen.","dyn.step2":"Schritt 2/3 · Gerät bedienen","dyn.step2_help":"Dieselbe Taste mehrmals drücken. Sobald das Signal erkannt wurde, kann hier direkt der Name eingegeben werden.","dyn.measure_failed":"Messung fehlgeschlagen","dyn.measure_failed_help":"Prüfen, ob das Radio verfügbar ist, und erneut versuchen.","dyn.start_failed":"Start fehlgeschlagen","dyn.start_failed_help":"Radio prüfen und erneut versuchen.","dyn.step3":"Schritt 3/3 · Prüfen und speichern","dyn.candidate_received":"{protocol} wurde {n}-mal empfangen. Gerät benennen und erkannte Funktionen prüfen.","dyn.observations":"{n} Beobachtungen","dyn.link_to_name":"Mit {name} verknüpfen","dyn.temp_connection":"Verbindung vorübergehend nicht verfügbar","dyn.temp_connection_help":"Erneut versuchen, sobald ESPHome erreichbar ist.","dyn.decoder_safe_disabled":"Decoderinformationen fehlen; sicheres Speichern ist deaktiviert","dyn.enter_name":"Namen mit 1–80 Zeichen eingeben","dyn.confirm_no_features":"Sie haben keine Funktionen freigegeben. Das Gerät wird nur in RFLink gespeichert. Fortfahren?","dyn.signal_linked":"Signal verknüpft. Neue Funktionen werden beim vorhandenen Gerät als Vorschläge angezeigt; die Auswahl auf diesem Bildschirm wurde darauf nicht angewendet.","dyn.saved_ha_failed":"Gerät gespeichert, aber HA-Veröffentlichung fehlgeschlagen. Sie können sie später unter Geräte aktivieren.","dyn.device_saved":"Gerät gespeichert. Es bleibt nach einem Neustart erhalten.","dyn.done":"Fertig","dyn.done_help":"Sie können ein weiteres Gerät hinzufügen oder Ihre Geräte ansehen.","dyn.save_failed":"Speichern fehlgeschlagen; erneut versuchen","dyn.confirm_link":"Dieses Signal mit {name} verknüpfen?","dyn.modal_invalid":"Decoderinformationen prüfen und einen Gerätenamen mit 1–80 Zeichen eingeben","dyn.confirm_store_only":"Keine Funktionen freigegeben. Nur das Gerät speichern?","dyn.device_added":"Gerät hinzugefügt","dyn.add_failed":"Gerät konnte nicht hinzugefügt werden","dyn.linked_existing":"Mit vorhandenem Gerät verknüpft","dyn.link_failed":"Verknüpfen fehlgeschlagen","dyn.radio_status_unreachable":"Radiostatus nicht erreichbar.","dyn.measuring_background":"Hintergrund wird gemessen…","dyn.waiting_capture":"Warte auf Aufnahme","dyn.capture_first":"Zuerst eine Aufnahme erstellen.","dyn.protocol_unknown":"noch unbekannt","dyn.identity_unknown":"noch nicht bestimmt","dyn.no_code_suggestion":"Noch kein Codevorschlag.","dyn.signal_strong":"Stark / möglicherweise zu nah","dyn.good":"GUT","dyn.weak":"Schwach","dyn.raw_rssi":"raw: RSSI nicht verfügbar","dyn.capture_found":"Aufnahme gefunden","dyn.measuring":"Messen…","dyn.ready":"Bereit","dyn.background_ready":"Hintergrundmessung abgeschlossen","dyn.background_failed":"Hintergrundmessung fehlgeschlagen","dyn.listen_started":"Empfang gestartet","dyn.listen_failed":"Empfang konnte nicht gestartet werden","dyn.not_measured":"Noch nicht gemessen","dyn.listen_stopped":"Empfang gestoppt; temporäre Messung gelöscht","dyn.stop_failed":"Stoppen fehlgeschlagen","dyn.invalid_brightness":"Ungültige Helligkeit","dyn.led_saved":"LED-Einstellungen gespeichert","dyn.save_failed_short":"Speichern fehlgeschlagen","dyn.protocol_count":"{n} Protokolle · Details nur auf Anfrage","dyn.receive_only":"Nur Empfang","dyn.example":"Beispiel","dyn.tx_hw_tested":"Senden: hardwaregetestet","dyn.tx_sw_tested":"Senden: Softwaretest","dyn.itho_separate":"Native Steuerung","dyn.tx_blocked":"Senden blockiert","dyn.tx_unavailable":"Senden nicht verfügbar","dyn.rx_decoder_present":"Empfangsdecoder vorhanden (Hardware nicht bestätigt)","dyn.in_ha_label":"In Home Assistant:","dyn.what_to_know":"Was muss ich wissen?","dyn.software_prefix":"Software: ","dyn.encoder_registered":"Sende-Encoder registriert; ","dyn.manifest_verified":"Hardwarestatus im Manifest: verifiziert","dyn.host_only":"Host-getestet, physischer Funktest fehlt","dyn.no_generic_encoder":"kein allgemeiner Sende-Encoder","dyn.only_approved":"RFLink zeigt nur freigegebene Funktionen.","dyn.technical_data":"Technische Daten","dyn.actions":"Aktionen","dyn.fields":"Felder","dyn.none":"keine","dyn.learn":"Anlernen","dyn.no_protocols":"Keine Protokolle in diesem Filter.","dyn.protocol_detection_note":"RFLink erkennt das tatsächliche Protokoll erst, wenn das Signal empfangen wird. Dies ist eine Anleitung, kein Protokollfilter.","dyn.no_decoders":"Keine Decoder registriert.","dyn.decoder_registry_unavailable":"Decoderregister nicht verfügbar.","dyn.decoder_library_unavailable":"Decoderbibliothek nicht verfügbar. Anlernen und Freigabe sind vorübergehend deaktiviert.","placeholder.device_name":"z. B. Gartensensor","field.temperature":"Temperatur","field.humidity":"Luftfeuchtigkeit","field.battery_low":"Batterie schwach","field.rain":"Niederschlag","field.fan_speed":"Lüftergeschwindigkeit","protocol.ExampleSwitch.band":"Kein echtes Funkgerät","protocol.ExampleSwitch.instruction":"Dies ist ein Codebeispiel und kann keine echten Signale anlernen.","protocol.ExampleSwitch.ha_result":"Kein Home-Assistant-Gerät.","protocol.ExampleSwitch.warning":"Nur in technischen Details sichtbar; nicht für den täglichen Gebrauch gedacht.","protocol.IthoRFT.band":"868,30 MHz","protocol.IthoRFT.instruction":"Drücken Sie wiederholt eine Stufen- oder Timer-Taste Ihrer Itho-Fernbedienung.","protocol.IthoRFT.ha_result":"Bestätigte Stufen-/Timer-Ereignisse und gegebenenfalls unterstützte Messwerte.","protocol.IthoRFT.warning":"RFLink sendet mit einer eigenen virtuellen Itho-Fernbedienung. Kopple sie einmal mit dem Lüftungsgerät, bevor du die Steuerung verwendest.","protocol.Acurite609TXC.band":"433,92 MHz","protocol.Acurite609TXC.instruction":"Warten Sie, bis dieselbe Sensormeldung mehrmals empfangen wurde.","protocol.Acurite609TXC.ha_result":"Bestätigte Temperatur, Luftfeuchtigkeit und gegebenenfalls Batteriewarnung.","protocol.Acurite609TXC.warning":"Nach einem Batteriewechsel kann sich die RF-Identität ändern; eine neue Identität nicht automatisch verknüpfen.","protocol.FineOffsetWH2.band":"433,92 MHz","protocol.FineOffsetWH2.instruction":"Warten Sie auf mehrere Meldungen des Wettersensors.","protocol.FineOffsetWH2.ha_result":"Bestätigte Temperatur und Luftfeuchtigkeit.","protocol.FineOffsetWH2.warning":"Ein fehlender Messwert bedeutet nicht, dass diese Funktion entfernt werden sollte.","protocol.Nexa.band":"433 MHz","protocol.Nexa.instruction":"Mehrmals Ein und danach Aus auf demselben Kanal drücken.","protocol.Nexa.ha_result":"Bestätigte Ein/Aus-Ereignisse. Bedientasten erscheinen erst nach separater Sendefreigabe.","protocol.Nexa.warning":"Fernbedienung und Lampe sind nicht automatisch dasselbe Gerät; mehrere Kanäle separat anlernen.","protocol.ByronSX.band":"433 MHz","protocol.ByronSX.instruction":"Türklingeltaste mehrmals drücken und warten, bis RFLink dasselbe Signal erkennt.","protocol.ByronSX.ha_result":"Ein Klingelereignis; eine Sendetaste erst nach separater Freigabe.","protocol.ByronSX.warning":"Ein Sendebefehl bestätigt nicht, dass die Klingel tatsächlich ausgelöst wurde.","protocol.Dooya.band":"433 MHz","protocol.Dooya.instruction":"Dieselbe Taste wiederholt drücken. Andere Tasten anschließend separat als Funktionsvorschläge anlernen.","protocol.Dooya.ha_result":"Freigegebene Tastenereignisse; Sendetasten nur nach separater Freigabe.","protocol.Dooya.warning":"Tastennummern haben noch keine feste Bedeutung wie Hoch oder Stop. Nicht automatisch zuordnen.","protocol.KeeLoq.band":"433 MHz","protocol.KeeLoq.instruction":"Dieselbe Taste mehrmals drücken und prüfen, ob dieselbe Fernbedienung erkannt wird.","protocol.KeeLoq.ha_result":"Empfangene Tastenereignisse und Batteriewarnung, sofern decodiert.","protocol.KeeLoq.warning":"Rolling Code ist geschützt; empfangene Nachrichten dürfen nicht erneut gesendet werden.","protocol.RCSwitch.band":"433 MHz","protocol.RCSwitch.instruction":"Eine Taste mehrmals drücken. Einen weiteren festen Code separat anlernen und selbst dem gewünschten Gerät zuordnen.","protocol.RCSwitch.ha_result":"Ein Ereignis für den erkannten Code; nach Freigabe optional eine Sendetaste.","protocol.RCSwitch.warning":"Code, Protokoll und Bitlänge müssen übereinstimmen. Ein- und Aus-Codes werden nicht automatisch verknüpft.","protocol.AlectoV1.band":"433,92 MHz","protocol.AlectoV1.instruction":"Auf mehrere Aussendungen des Wettersensors warten; keine zufälligen Tasten drücken.","protocol.AlectoV1.ha_result":"Nur von Ihnen bestätigte Messwerte wie Temperatur, Feuchtigkeit und gegebenenfalls Niederschlag.","protocol.AlectoV1.warning":"Nicht jede Variante sendet alle Messwerte. Ein fehlender Wert ist nicht automatisch ein Fehler.","protocol.SomfyRTS56.band":"433,42 MHz","protocol.SomfyRTS56.instruction":"Dieselbe Taste der Somfy-Fernbedienung wiederholt drücken. Für diesen Empfangstest nicht die Programmiertaste verwenden.","protocol.SomfyRTS56.ha_result":"Empfangene Aktionen können überwacht werden. Nach dem Koppeln von RFLink sind auch freigegebene Auf-, Ab- und My-Steuerungen verfügbar.","protocol.SomfyRTS56.warning":"Zum Senden verwendet RFLink eine eigene virtuelle Fernbedienung mit eigenem Rolling Counter. RFLink zuerst mit dem Motor koppeln; der Test an echter Hardware steht noch aus.","protocol.RubicsonTemp.band":"433,92 MHz","protocol.RubicsonTemp.instruction":"Auf mehrere Meldungen des Temperatursensors warten.","protocol.RubicsonTemp.ha_result":"Bestätigte Temperatur und gegebenenfalls Batteriewarnung.","protocol.RubicsonTemp.warning":"Kanal und Prüfsumme sind technische Daten und keine separaten Home-Assistant-Sensoren.","decoder.select_hint":"Wähle einen Decoder aus, um alle Informationen anzuzeigen.","decoder.how_to_learn":"So wird angelernt","decoder.why_tx":"Warum Senden so funktioniert","decoder.what_needed":"Was noch benötigt wird","decoder.reason_sensor":"Dieses Protokoll ist ein reiner Empfangssensor. Senden ist absichtlich deaktiviert.","decoder.reason_example":"Dies ist ein Entwicklungsbeispiel und kein echtes RF-Protokoll.","decoder.reason_tx_ready":"Für dieses Protokoll ist ein geprüfter Sende-Encoder verfügbar.","decoder.reason_tx_host":"Der Sende-Encoder ist vorhanden und besteht Softwaretests, wurde aber noch nicht mit echter RF-Hardware verifiziert.","decoder.reason_itho":"Empfang und native Paketübertragung sind integriert. RFLink verwendet für jedes eingelernte Itho-Gerät eine eigene virtuelle Senderidentität.","decoder.reason_somfy":"Empfang und Senden sind implementiert. RFLink steuert Somfy als separate virtuelle Fernbedienung mit eigenem Rolling Counter.","decoder.reason_keeloq":"Unterstützte Frames können empfangen werden, aber generisches Senden ist durch die KeeLoq-Rolling-Code-Verschlüsselung gesperrt.","decoder.reason_no_encoder":"Empfang wird unterstützt, aber es ist noch kein geprüfter Sende-Encoder registriert.","decoder.needed_none":"Nichts; reiner Empfang ist hier vorgesehen.","decoder.needed_hw_test":"Ein RF-Test mit echter Hardware und Capture-Vergleich, bevor Senden als hardwareverifiziert gilt.","decoder.needed_itho":"RFLink einmal mit dem Lüftungsgerät koppeln und danach die Steuerung an echter Hardware prüfen.","decoder.needed_somfy":"Die virtuelle RFLink-Fernbedienung mit dem Motor koppeln. Die Validierung an einem echten Motor steht noch aus.","decoder.needed_keeloq":"Ein unterstütztes Produkt mit gültigem kryptografischem Schlüssel und Pairing-Verfahren. Dies lässt sich nicht sicher allein aus normalen RF-Captures ableiten.","decoder.needed_encoder":"Ein protokollspezifischer Sende-Encoder plus Software- und Hardwarevalidierung.","library.title":"Bibliothek & Updates","library.help":"Offizielle Firmware-Inhalte und deine eigenen Inhalte bleiben getrennt. RFLink ersetzt deine Änderungen nie ohne Bestätigung.","library.official_decoders":"Offizielle Decoder","library.my_decoders":"Meine Decoder","library.my_profiles":"Meine Geräteprofile","library.needs_attention":"Aufmerksamkeit nötig","library.review":"Änderungen prüfen","library.advanced":"Erweitert","library.hide_advanced":"Erweitert ausblenden","library.advanced_help":"Technische Bibliotheksinformationen für Diagnose und Entwicklung.","library.schema":"Bibliotheksschema","library.user_storage":"Persistente Benutzerbibliothek","library.safe_resolutions":"Sicher automatisch lösbar","library.archive":"Archivierte lokale Einträge","library.enabled":"Aktiviert","library.scaffold":"Grundlage bereit; persistente Benutzerbibliothek in diesem Build noch nicht aktiviert","library.status_attention":"Ein oder mehrere lokale Einträge benötigen eine Entscheidung. Deine aktuelle Konfiguration bleibt aktiv, bis du eine Änderung bestätigst.","library.status_ok":"Alles ist aktuell. Keine Aktion erforderlich.","library.status_scaffold":"Die Update- und Konfliktlogik ist bereit. Persistente eigene Decoder und Profile sind die nächste Speicherschicht.","library.attention_title":"Bibliotheksupdate benötigt deine Aufmerksamkeit","library.attention_count":"{n} lokale Einträge benötigen eine Entscheidung. Nichts wurde automatisch ersetzt.","library.unavailable":"Der Bibliotheksstatus ist vorübergehend nicht verfügbar.","library.review_pending":"Entscheidungen werden verfügbar, sobald der persistente Benutzerspeicher aktiviert ist.","library.no_items":"Keine Bibliothekseinträge gemeldet.","library.source":"Quelle","library.source_builtin":"Integriert","library.source_user":"Benutzer","library.stable_id":"Stabile ID","library.version":"Version","library.pack":"Decoder-Paket","help.library_title":"Was ist die Bibliothek?","help.library_text":"Die Bibliothek kombiniert offizielle Firmware-Decoder mit optionalen, vom Benutzer erstellten Decodern und Geräteprofilen. Offizielle und lokale Einträge bleiben getrennt.","help.library_more":"Nach einem Update kann RFLink eine offizielle Version empfehlen. Lokale Änderungen werden nie stillschweigend überschrieben.","help.source_title":"Was bedeutet Quelle?","help.source_text":"Integriert bedeutet, dass der Eintrag mit der Firmware geliefert wird. Benutzer bedeutet, dass er auf diesem RFLink-Gerät gelernt oder erstellt wurde.","help.source_more":"Die Quelle hilft RFLink zu entscheiden, was automatisch aktualisiert werden kann und wann deine Bestätigung erforderlich ist.","help.id_title":"Was ist eine stabile ID?","help.id_text":"Eine stabile ID ist die dauerhafte Identität eines Decoders oder Profils über Firmware-Versionen hinweg.","help.id_more":"RFLink verwendet sie zusammen mit Versionen und Inhalts-Fingerabdrücken, um Duplikate, Updates und lokale Änderungen zu erkennen.","user.heading":"Eigenes Protokoll","user.guide":"Gib dem Protokoll einen Namen, benenne die aufzuzeichnende Taste oder Aktion und betätige sie mindestens dreimal. Technische Regeln bleiben unter Erweitert.","user.name":"Decodername","user.id":"Stabile ID","user.action":"Taste/Aktion","user.capture":"Aktion aufnehmen","user.clear":"Aufnahmen löschen","user.create":"Decoder testen und aktivieren","user.advanced":"Erweitert: RX- und TX-Regeln bearbeiten","user.advanced_help":"RX-Regeln erkennen Signale. TX-Regeln erzeugen eine Vorschau. Speichern ist transaktional: Ein funktionierender aktiver Decoder bleibt aktiv, wenn geänderte Regeln die Prüfung nicht bestehen.","user.actions":"Aktionen, durch Kommas getrennt","user.save_rules":"Regeln speichern","user.vm_help":"Regeln laufen in einem begrenzten Interpreter ohne nativen Code. Syntax in USER_DECODER_GUIDE.md.","user.firmware":"In Firmwaredecoder umwandeln","user.firmware_help":"Bibliothek exportieren und die mitgelieferten Prüf- und Zusammenführungsskripte am Computer ausführen.","user.export":"Prüfexport herunterladen","user.saved":"Eigene Protokolle und Geräteprofile","user.profile":"Geräteprofil","user.profile_help":"Gib dem Decoder einen verständlichen Modellnamen und eine Lernanleitung. Technische Felder sind optional.","user.profile_name":"Profilname","user.profile_id":"Profil-ID","user.decoder":"Decoder","user.instructions":"Lernanleitung","user.save_profile":"Profil speichern","user.samples":"Aufnahmen","user.active":"aktiv","user.inactive":"inaktiv","user.preview_ready":"TX-Vorschau verfügbar","user.rx_only":"Nur Empfang","user.edit":"Bearbeiten","user.disable":"Deaktivieren","user.activate":"Aktivieren","user.remove":"Entfernen","user.empty":"Noch keine eigenen Decoder gespeichert.","user.unavailable":"Eigene Bibliothek nicht verfügbar.","user.activated":"Decoder aktiviert.","user.disabled":"Decoder deaktiviert.","user.confirm_remove":"Dieses Element entfernen?","user.no_samples":"Noch keine Aufnahmen.","user.check_radio_action":"Funktionierende Funkverbindung und gültigen Aktionsnamen wählen.","user.press_three":"Diese Taste mindestens dreimal drücken, dann nächste Aktion aufnehmen.","user.cleared":"Aufnahmen gelöscht.","user.check_id_name":"Gib einen Decodernamen ein. Wenn du die stabile ID manuell geändert hast, muss sie mit user_ beginnen.","user.created_tx":"Decoder aktiv. TX-Vorschau bereit; jedes Gerät vor dem Senden freigeben.","user.created_rx":"Decoder für Empfang aktiv. Bei Bedarf TX-Regel unter Erweitert hinzufügen.","user.advanced_saved":"Regeln gespeichert, geprüft und aktiviert.","user.profile_saved":"Profil gespeichert.","user.exported":"Prüfexport heruntergeladen.","user.device_controls":"Eigener Decoder und Geräteprofil","user.tx_notice":"Eine Vorschau sendet nicht. Senden erfordert die Freigabe für dieses Gerät; ein Befehl ist keine Empfangsbestätigung.","user.no_profile":"Kein Profil","user.revoke_tx":"Senden beenden","user.allow_tx":"Senden erlauben","user.preview":"TX-Vorschau","user.send":"Senden","user.confirm_allow":"Darf dieses Gerät mit dem eigenen Decoder Funksignale senden?","user.confirm_send":"Diese Funkaktion senden","user.sent_request":"TX-Befehl übergeben; keine Bestätigung des Zielgeräts.","user.import":"Bibliothekssicherung wiederherstellen","user.import_help":"Jeder Decoder wird vor dem Ersetzen geprüft. TX-Freigaben werden nie automatisch wiederhergestellt; Protokollzähler können nur vorwärts gesetzt werden.","user.import_invalid":"Dies ist keine gültige RFLink-Sicherung der Benutzerbibliothek.","user.import_confirm":"{decoders} Decoder und {profiles} Profil(e) wiederherstellen? Bestehende IDs werden erst nach erfolgreicher Prüfung ersetzt.","user.imported":"Wiederherstellung abgeschlossen: {decoders} Decoder, {profiles} Profil(e), {bindings} Profilbindung(en) an bestehende Geräte. TX-Freigaben wurden nicht wiederhergestellt.","user.analysis_waiting":"Beispiele aufnehmen, um stabile und veränderliche RF-Felder zu vergleichen.","user.analysis_need_more":"{action}: Mindestens ein weiteres Beispiel zum Vergleichen aufnehmen.","user.analysis_length":"{action}: Die aufgenommenen Frames haben unterschiedliche Längen; dieses Protokoll manuell prüfen.","user.analysis_dynamic":"{action}: {count} Symbole; veränderliche Positionen: {positions}. Das können Zähler, Prüfsumme, ID oder tastenabhängige Daten sein.","user.analysis_fixed":"{action}: {count} Symbole; in den aufgenommenen Beispielen stabil.","user.radio_profile":"Funkprofil","user.radio_profile_help":"Diese Version verwendet die validierte Funkkonfiguration des Boards; eigene CC1101-Modulation/Bitrate kann noch nicht aus Benutzerrezepten gesetzt werden.","user.analysis_counter":"{action}: Position(en) {positions} sehen wie ein hochzählender Zähler aus. Automatisches Wiederholen bleibt bei wechselnden Werten deaktiviert.","user.analysis_checksum":"{action}: Position(en) {positions} sehen wie ein einfacher Prüfsummen-Kandidat aus. Vor einer Advanced-Regel bitte prüfen.","library.official_profiles":"Offizielle Geräteprofile","decoder.known_profiles":"Bekannte Geräteprofile","decoder.known_profiles_help":"Modellfamilien, die in öffentlichen Hersteller-/Protokollquellen diesem Decoder zugeordnet sind. Das ist quellenbasierte Kompatibilitätsinformation, keine Hardwarezertifizierung.","decoder.models":"Modelle / Aliase","decoder.profile_learn":"So wird dieses Profil angelernt","decoder.hardware_pending":"Hardwarevalidierung auf diesem RFLink-Board steht aus","ui.manage_devices":"Geräte verwalten","ui.manage_devices_help":"Geräte umbenennen, neue Funktionen prüfen und Home Assistant verwalten.","ui.settings_sub":"Zuerst die alltäglichen Einstellungen. Bibliothek und technische Werkzeuge stehen weiter unten.","devices.pairing_filter":"Geräte mit Einrichtungsanleitung","devices.show_all":"Alle Geräte anzeigen","devices.waiting_signals":"Signale zur Prüfung","devices.waiting_signals_help":"Diese Signale wurden außerhalb des geführten Ablaufs Gerät hinzufügen erkannt. Prüfe nur die Signale, die du behalten möchtest.","placeholder.wizard_device_name":"z. B. Wohnzimmer-Fernbedienung","placeholder.profile_name":"z. B. Wohnzimmer-Fernbedienung","placeholder.profile_help":"z. B. Jede Taste dreimal drücken","accessibility.led_brightness":"RGB-Helligkeit","accessibility.close":"Schließen","dyn.online":"Online","dyn.frames_label":"Signale","dyn.device_sub_simple":"Deine gespeicherten RF-Geräte. Öffne ein Gerät nur, wenn du etwas ändern möchtest.","led.boot":"Startet","led.connecting":"WLAN wird verbunden","led.ready":"Bereit","led.offline":"Verbindung verloren","led.radio_missing":"Funkmodul fehlt oder ist nicht bereit","led.learning":"Lernen","led.capture":"Signal empfangen","led.learned":"Gerät bestätigt","led.itho_pair":"Itho-Koppelbefehl","led.tx":"RF wird gesendet","led.ota":"Firmware-Update","led.error":"Fehler","led.unknown":"Unbekannt","decoder.compatibility_details":"Kompatibilität und Quellen","decoder.control":"Bedienung","decoder.next_step":"Noch erforderlich","user.saved_help":"Nur verwenden, wenn RFLink dein Gerät noch nicht kennt oder wenn du ein eigenes Modellprofil erstellen möchtest.","user.profile_advanced":"Erweiterte Profiloptionen","user.profile_auto_help":"Lass die ID leer, damit sie automatisch aus dem Profilnamen erstellt wird. Aktionen werden nach Möglichkeit aus dem Decoder übernommen.","settings.developer_info":"Entwicklerinformationen","placeholder.decoder_name":"z. B. Wohnzimmer-Fernbedienung","user.id_auto_help":"Lass dieses Feld bei einem neuen Decoder leer; RFLink erzeugt automatisch eine stabile ID aus dem Namen. Ändere sie nur, wenn du bewusst eine bestimmte ID brauchst.","settings.mqtt_setup":"MQTT einrichten","settings.led_simple_help":"Helligkeit anpassen oder den grünen Bereitschaftsimpuls ausschalten. Warnfarben bleiben aktiv.","settings.led_more":"LED-Farben und Verhalten","ui.advanced":"Erweitert","ui.advanced_details":"Erweiterte Details","ui.advanced_filters":"Erweiterte Filter","ui.try_again":"Erneut versuchen","ui.decoder_advanced_help":"Rohe Funkdaten aufnehmen und vergleichen.","ui.freq_43392":"Am häufigsten für Fernbedienungen, Sensoren und Schalter","ui.freq_43342":"Häufig für Somfy RTS","ui.freq_868":"Häufig für Itho und andere 868-MHz-Geräte","ui.freq_other":"Verwende dies, wenn dein Gerät auf dieser Frequenz arbeitet","ui.frequency_help":"Unsicher? 433,92 MHz ist meist der beste Startpunkt.","ui.device_found_help":"RFLink hat das Gerät gefunden. Gib ihm einen Namen und speichere es.","ui.protocol_label":"Protokoll","ui.signal_seen":"Signal empfangen","ui.back_add_device":"← Zurück zu Gerät hinzufügen","somfy.control":"Somfy-Steuerung","somfy.tx_disabled":"Senden ist in den Firmware-Einstellungen deaktiviert. Aktiviere TX-Steuerung vor dem Pairing.","somfy.pair_help":"Versetze den Motor mit einer vorhandenen Fernbedienung in den Programmiermodus und kopple danach RFLink.","somfy.pair":"RFLink koppeln","somfy.control_ready":"Somfy-Steuerung bereit zum Testen","somfy.up":"Hoch","somfy.my":"My / Stop","somfy.down":"Runter","somfy.no_ack":"RFLink kann die Reaktion des Motors per Funk nicht bestätigen. Reagiert der Motor nicht, erneut koppeln.","somfy.pair_again":"Erneut koppeln","somfy.pair_confirm":"Versetze den Motor zuerst in den Programmiermodus. Jetzt den RFLink-PROG-Befehl senden?","somfy.pair_sent":"Pairing-Befehl gesendet","somfy.pair_failed":"Pairing fehlgeschlagen","somfy.command_sent":"Befehl gesendet","somfy.command_failed":"Befehl fehlgeschlagen","somfy.state":"Virtuelle Fernbedienung {address} · nächster Rolling Code {counter}","led.pairing":"Koppeln","itho.control":"Itho-Steuerung","itho.tx_disabled":"Senden ist in den Firmware-Einstellungen deaktiviert. Aktiviere TX-Steuerung vor dem Pairing.","itho.pair_help":"Schalte das Lüftungsgerät etwa 15 Sekunden aus und wieder ein. Kopple RFLink innerhalb von 2 Minuten.","itho.pair":"RFLink koppeln","itho.control_ready":"Itho-Steuerung bereit zum Testen","itho.low":"Niedrig","itho.medium":"Mittel","itho.high":"Hoch","itho.timer10":"Timer 10 Min","itho.timer20":"Timer 20 Min","itho.timer30":"Timer 30 Min","itho.standby":"Standby","itho.no_ack":"RFLink kann nicht direkt bestätigen, dass das Gerät die Kopplung gespeichert hat. Reagiert es nicht, erneut koppeln oder Kompatibilitätskopplung verwenden.","itho.compat_help":"Wenn normales Koppeln nicht funktioniert, versuche die Kompatibilitätskopplung.","itho.pair_again":"Erneut koppeln","itho.compat_pair":"Kompatibilitätskopplung","itho.unpair":"RFLink abmelden","itho.pair_confirm":"Schalte das Gerät zuerst etwa 15 Sekunden aus und wieder ein. Jetzt den RFLink-Kopplungsbefehl senden?","itho.compat_confirm":"Schalte das Gerät zuerst etwa 15 Sekunden aus und wieder ein. Jetzt die Kompatibilitätskopplung senden?","itho.unpair_confirm":"Versetze das Gerät zuerst erneut in den Anmeldemodus. RFLink jetzt abmelden?","itho.pair_sent":"Kopplungsbefehl gesendet","itho.pair_failed":"Koppeln fehlgeschlagen","itho.unpair_sent":"Abmeldebefehl gesendet","itho.unpair_failed":"Abmelden fehlgeschlagen","itho.command_sent":"Befehl gesendet","itho.command_failed":"Befehl fehlgeschlagen","itho.state":"Virtuelle Fernbedienung {address} · nächster Zähler {sequence} · Typ {type}","diag.checking":"Prüfen","diag.cc1101_failed":"CC1101 nicht erkannt","diag.rx_failed":"CC1101 OK · RX-Fehler","diag.tx_failed":"CC1101 OK · TX-Fehler","diag.ok":"Bereit","diag.spi_ok":"Erkannt / initialisiert","diag.spi_failed":"Initialisierung fehlgeschlagen","diag.cc1101_spi":"CC1101 / SPI","diag.rx_path":"Empfangspfad","diag.tx_path":"Sendepfad","diag.ready":"Bereit","diag.failed":"Fehler","diag.configured_pins":"Konfigurierte Pins","diag.hardware":"Hardware","diag.cc1101_hint":"Der CC1101 selbst fällt vor Empfang oder Senden aus. 3,3 V, GND, CSN sowie die gemeinsamen SCK/MOSI/MISO-Leitungen prüfen. GDO ist hier nicht der erste Verdacht.","diag.spi_probe":"Nur-Lese-SPI-Test","diag.probe_response":"Gültige Antwort","diag.probe_no_response":"Keine gültige Antwort","diag.spi_checks":"SPI-Prüfungen","diag.spi_failures":"SPI-Fehler","diag.spi_consecutive":"Aufeinanderfolgende Fehler","diag.spi_recovery_attempts":"Wiederherstellungsversuche","diag.spi_recovery_successes":"Erfolgreiche Wiederherstellungen","diag.spi_health":"Digitaler SPI-Zustand","diag.spi_health_note":"Software kann ungültige SPI-Antworten erkennen, aber keine elektrischen Reflexionen, Ringing oder Overshoot. Dafür ein Oszilloskop oder einen Logikanalysator verwenden.","monitor.show_noise":"Vermutetes HF-Rauschen anzeigen","monitor.noise_help":"Vermutetes Rauschen ist standardmäßig ausgeblendet, bleibt aber für Diagnose und Export verfügbar.","monitor.copy":"Kopieren","monitor.export":"Exportieren","monitor.copied":"Sichtbare Aktivität kopiert.","monitor.exported":"Aktivitätsexport heruntergeladen.","monitor.noise_badge":"Vermutetes HF-Rauschen","monitor.noise_reason":"Grund","diag.noise_frames":"Vermutetes Rauschen","diag.freq_offset":"Frequenzabweichung","diag.uptime":"Laufzeit","diag.boot_count":"Erfasste Starts","diag.reset_reason":"Letzter Reset-Grund","diag.boot_log":"Letzte Starts","diag.boot_log_hint":"Auf dem ESP32 gespeichert, ohne Zeitstempel (beim Start ist keine Uhrzeit garantiert). Neueste zuerst.","diag.log_download":"Diagnose exportieren","diag.heap_free":"Freier RAM","diag.heap_min":"Kleinster freier RAM","diag.boot_unavailable":"Startverlauf nicht im Flash gespeichert","diag.reboot_observed":"Neustart bei geöffneter Seite erkannt","diag.counter_reset":"Signalzähler sank ohne ESP32-Neustart","diag.since_boot":"Signalzähler gelten seit dem letzten Start","diag.no_events":"Keine Ereignisse bei geöffneter Seite erkannt","about.heading":"Über RFLink Universal","about.version":"Firmwareversion","about.revision":"Installierte Revision","about.build_more":"Build-Informationen","about.built":"Kompiliert am","rxlog.title":"Empfangsprotokoll","rxlog.signals":"Signale","rxlog.noise":"Rauschen","rxlog.all":"Alle","rxlog.radio":"Funkmodul","rxlog.all_radios":"Alle Funkmodule","rxlog.settings":"Protokolleinstellungen","rxlog.signal_limit":"Maximale Signalanzahl","rxlog.noise_limit":"Maximale Rauschereignisse (alle Funkmodule)","rxlog.noise_floor":"Weiche Rauschreserve pro Funkmodul","rxlog.memory":"Protokollspeicher","rxlog.download":"Protokolle herunterladen (ZIP)","rxlog.saved":"Ohne Neustart gespeichert","rxlog.memory_eviction":"Bei Speichermangel wird zuerst Rauschen entfernt","rxlog.type":"Protokolltyp","rxlog.settings_help":"Grenzen zur Laufzeit ändern. Einstellungen bleiben gespeichert. Rauschen wird zuerst entfernt.","rxlog.radio0":"433,42-MHz-Signale","rxlog.radio1":"433,92-MHz-Signale","rxlog.radio2":"868,30-MHz-Signale","rxlog.save":"Protokolleinstellungen speichern","rxlog.older":"Ältere Ereignisse laden","rxlog.stored":"Gespeichert","rxlog.noise_info":"Rauschen liegt in einem gemeinsamen Speicher, ist aber je Funkmodul filterbar.","rxlog.invalid":"Ungültige Grenzen (gesamt maximal 4096).","rxlog.exporting":"Vollständigen Export vorbereiten…","rxlog.exported":"ZIP heruntergeladen","rxlog.token_error":"Schreibberechtigung nicht verfügbar","settings.general":"Allgemein & Statusleuchte","settings.integrations":"Home Assistant & System","settings.additional":"Weitere Einstellungen & Werkzeuge","settings.dashboard_hint":"Häufig genutzte Einstellungen direkt im Blick. Weitere Werkzeuge bei Bedarf öffnen.","nav.back_settings":"← Zurück zu den Einstellungen","nav4.search_devices":"Gespeicherte Geräte suchen","nav4.search_placeholder":"Name, Protokoll oder Funkmodul…","nav4.unknown_radio":"Unbekanntes Funkmodul","nav4.clear":"Löschen","nav4.found":"{visible} von {total} Geräten angezeigt","nav4.no_results":"Keine Treffer. Filter ändern oder löschen.","nav4.mqtt_host":"Brokeradresse","nav4.mqtt_port":"Port","nav4.mqtt_user":"Benutzername","nav4.mqtt_pass":"Passwort (leer = behalten)","nav4.mqtt_enable":"MQTT aktivieren","nav4.mqtt_save":"Speichern und verbinden","nav4.mqtt_not_built":"MQTT nicht einkompiliert. Optionales Paket einmalig aktivieren.","nav4.mqtt_security":"Auf dem Gerät gespeichert. Standard-NVS nicht verschlüsselt; nur vertrauenswürdiges Netzwerk.","nav4.mqtt_saved":"Einstellungen gespeichert. Verbinden…","nav4.mqtt_bad":"Ungültige MQTT-Werte oder Speicherfehler","devices.radio_count":"{n} Geräte","profileflow.choose":"Geräteprofil","profileflow.none":"Profil auswählen","profileflow.suggested":"Vorgeschlagen","profileflow.select_help":"Profil wählen, um Funktionen anzuzeigen. Das genaue Modell wurde nicht bestätigt.","profileflow.manual_warning":"Möglicher Treffer; das Funksignal bestätigt weder Hersteller noch Modell.","profileflow.proposal_warning":"Vorschlag anhand von Protokoll und Aktionen; Modell selbst prüfen.","profileflow.user_choice":"Profil manuell gewählt.","profileflow.after_profile":"Bekannte Befehle","profileflow.sensor_profile":"Sensorprofil: Messwerte erscheinen nach Empfang.","profileflow.choose_first":"Geräteprofil auswählen, um die Funktionen zu sehen.","profileflow.measurements":"Messwerte","profileflow.sensor_info":"Messwerte erscheinen beim Empfang; keine Sendetasten erforderlich.","profileflow.operation":"Steuerung","profileflow.no_commands":"Keine bekannten Steuerbefehle für dieses Profil.","profileflow.tx_disabled":"Tasten nur zur Information: RF-Senden ist deaktiviert.","profileflow.pair_first":"RFLink-Sender vor der Verwendung koppeln.","profileflow.not_ready":"Steuerung noch nicht möglich. TX-Encoder und Funk müssen bereit sein.","profileflow.tx_ready":"Steuerung verfügbar; keine Gerätebestätigung garantiert.","profileflow.functions_advanced":"Empfangene Funktionen","profileflow.saved":"Profil gespeichert","profileflow.save_error":"Profil konnte nicht gespeichert werden","profileflow.send_confirm":"{command} senden?","profileflow.sent":"Sendeauftrag angefordert","profileflow.send_error":"Senden fehlgeschlagen","profileflow.command.auto":"Auto","profileflow.command.low":"Niedrig","profileflow.command.medium":"Mittel","profileflow.command.high":"Hoch","profileflow.remove_tx_locked":"Deaktivieren Sie die Sendefunktion, bevor Sie technische Daten entfernen.","profileflow.dismiss_candidate":"Vorschlag entfernen","profileflow.dismiss_confirm":"Dieses noch nicht gespeicherte Signal entfernen? Es kann bei erneutem Empfang wieder erscheinen.","profileflow.dismissed":"Vorschlag entfernt","profileflow.dismiss_failed":"Vorschlag konnte nicht entfernt werden","profileflow.setup":"Schrittweise Einrichtung","profileflow.setup_rx_only":"Diese Schritte dienen Erkennung und Empfang. RFLink-Kopplung und Senden benötigen zuvor Hardwarevalidierung.","profileflow.command.away":"Abwesend","profileflow.command.timer_15":"15 Min.","profileflow.command.timer_30":"30 Min.","profileflow.command.timer_60":"60 Min.","profileflow.command.bypass_auto":"Bypass Auto","profileflow.command.bypass_open":"Bypass öffnen","profileflow.command.bypass_close":"Bypass schließen","catalog.protocols":"Protokolle","catalog.profiles":"Geräteprofile","catalog.search_placeholder":"Marke, Modell oder Protokoll suchen…","catalog.all_brands":"Alle Marken","catalog.all_families":"Alle Familien","catalog.all_protocols":"Alle Protokolle","catalog.profile_uses_protocol":"Decoder","catalog.show_related":"Geräteprofile ansehen","catalog.select_profile":"Wähle ein Geräteprofil für die Details.","catalog.custom_brand":"Benutzerdefiniert","catalog.custom_family":"Eigene Geräteprofile","catalog.protocol":"Protokoll","catalog.no_help":"Keine Anlernanleitung vorhanden.","catalog.operation":"Steuerungsstatus","catalog.tx_profile_warning":"Befehle erfordern einen passenden Encoder, Kopplung und geprüften Funkbetrieb.","catalog.tx_not_ready":"Funktionen sind bekannt, aber Senden ist für dieses Profil nicht verfügbar.","catalog.rx_only":"Nur Empfang; keine Sendetasten.","catalog.profile_functions":"Profilfunktionen","catalog.received_actions":"Empfangene Aktionen","catalog.setup_steps":"Einrichtungsschritte","catalog.profile_source":"Profilquelle","catalog.profile_protocol_note":"Profile beschreiben Geräte; Decoder verarbeiten Funksignale.","catalog.profile_count":"{shown} von {total} Geräteprofilen","catalog.no_results":"Keine passenden Geräteprofile gefunden.","action.auto":"Auto","action.away":"Abwesend","wizard.setup_title":"Gerät einrichten","wizard.start":"Schrittweise einrichten","wizard.step":"Schritt","wizard.close":"Schließen","wizard.back":"Zurück","wizard.next":"Weiter","wizard.finish":"Fertig (Empfang)","wizard.check":"Antwort überprüfen","wizard.rx_only":"Ohne Senden fortfahren","wizard.transmit":"Koppelbefehl senden","wizard.identity_help":"Prüfe, ob die RF-Kennung zu diesem Gerät oder zur vorhandenen Fernbedienung gehört, und bestätige das Modell.","wizard.finish_help":"Prüfe das Ergebnis. Senden allein bestätigt keine erfolgreiche Kopplung.","wizard.observe_help":"Signal empfangen und Kennung prüfen.","wizard.choose_profile":"Zuerst Geräteprofil wählen.","wizard.rx_ready":"Empfang bereit; keine Kopplung erforderlich.","wizard.sent_not_verified":"Befehl gesendet; keine Gerätebestätigung erfasst.","wizard.not_supported":"Kopplung noch nicht unterstützt.","wizard.requires_setup":"Anleitung verfügbar; Kopplung unbestätigt.","wizard.orcon_tx_missing":"Orcon: Sender und RAMSES-II-1FC9-Austausch wurden noch nicht auf Hardware validiert. Kopplung bleibt deaktiviert.","wizard.tx_unavailable":"Freigegebener Encoder oder Sender nicht verfügbar; nur Empfang.","wizard.verify_observed":"Ich habe eine Reaktion gesehen","wizard.verify_question":"Hast du die erwartete Reaktion beobachtet? Dies ist keine RF-Bestätigung.","wizard.transmit_confirm":"Ist der Empfänger im Anlernmodus? Einen freigegebenen Koppelbefehl senden?","ux3.change_profile":"Geräteprofil ändern","ux3.select_profile":"Geräteprofil auswählen","ux3.no_profile":"Noch nicht ausgewählt","ux3.not_in_ha":"Nicht in Home Assistant veröffentlicht","ux3.manage_details":"Gerät und Funksignale verwalten","ux3.not_yet_available":"Steuerung ist noch nicht verfügbar.","ux3.finish_rx_only":"Beenden (nur Empfang)","ux3.orcon_setup_short":"Empfang funktioniert; die RF-Kopplung wird noch getestet.","ux3.rx_only_short":"Empfang ist möglich, RF-Steuerung noch nicht bereit.","ux3.receive_only_status":"Nur Empfang","ux3.control_ready_status":"Steuerung verfügbar","ux3.sensor_status":"Messwerte empfangen","ux3.profile_missing_status":"Profil auswählen","debug.menu":"Systemdiagnose","debug.title":"Systemdiagnose","debug.help":"RFLink-Abläufe: Decoder, Gerät, Profil, Kopplung, TX und RX-Wiederherstellung. Getrennt von Funkprotokollen.","debug.lab_gate":"GDO-TX-Labor","debug.probe_gate":"RAMSES-TX-Test","debug.radio":"868-MHz-Funkstatus","debug.filter":"Anzeigen","debug.all":"Alle","debug.live":"Live","debug.refresh":"Aktualisieren","debug.export":"Diagnose exportieren","debug.empty":"Noch keine Systemereignisse.","debug.experiment":"Erweitert: RAMSES-Sendetest","debug.experiment_help":"Sendet ein 10E0-Testtelegramm, ohne Steuerung oder Kopplung. Beide YAML-Lab-Optionen erforderlich. Mit separatem Empfänger prüfen.","debug.send":"Ein Testtelegramm senden","debug.privacy":"128 RAM-Ereignisse (nach Neustart gelöscht). Geräte-IDs können enthalten sein. Keine Rohfunkdaten oder Passwörter.","debug.fetch_failed":"Systemereignisse nicht erreichbar:","debug.enabled":"Aktiv","debug.disabled":"Deaktiviert","debug.ready":"Funk bereit","debug.not_ready":"Nicht bereit","debug.fault":"RX-Wiederherstellungsfehler","debug.count":"{shown} von {total} Ereignissen","debug.dropped":"Verworfen","debug.confirm":"EIN experimentelles RAMSES-10E0-Telegramm auf 868 MHz senden? Keine Kopplung oder Steuerung.","debug.sending":"Testtelegramm wird gesendet…","debug.sent_unverified":"Adapter meldet TX und RX-Wiederherstellung. Funksignal und Geräteantwort NICHT bestätigt.","debug.failed":"Test fehlgeschlagen","logbook.title_nav":"▤ Protokolle","logbook.title":"Protokolle","logbook.subtitle":"Funk, Firmware-Entscheidungen und Browser-Aktionen zusammen.","logbook.all":"Alle","logbook.radio":"Funk","logbook.system":"System","logbook.web":"Web","logbook.level":"Stufe","logbook.any":"Alle","logbook.warnings":"Warnungen & Fehler","logbook.errors":"Fehler","logbook.search":"Suchen","logbook.search_hint":"Gerät, ID, Aktion…","logbook.noise":"Rauschen zeigen","logbook.live":"Live","logbook.refresh":"Aktualisieren","logbook.export":"Exportieren","logbook.note":"Webereignisse nur in diesem Browser. Browserzeiten sind angenähert. API-Nutzdaten werden nicht protokolliert.","logbook.system_limit":"Systemereignisse (ESP32)","logbook.web_limit":"Webereignisse (Browser)","logbook.verbose":"Webdebug: erfolgreiche API-Aktualisierungen erfassen","logbook.settings_note":"Systemgrenzen gelten bis Neustart, Browsereinstellungen lokal; Funkgrenzen auf ESP32.","logbook.empty":"Noch keine Ereignisse.","logbook.events":"Ereignisse","logbook.invalid":"Ungültige Werte.","logbook.saved":"Einstellungen gespeichert.","logbook.save_failed":"Speichern fehlgeschlagen:","orcon.experimental_status":"Experimentelle Kopplung und Steuerung; Laboreinstellungen erforderlich.","orcon.enable_lab":"TX, GDO-UART-Labor und RAMSES-Testlabor in YAML aktivieren.","orcon.wait_ack":"Auf die passende Antwort des Lüftungsgeräts warten.","orcon.status_waiting_ack":"1FC9 gesendet; warte auf ACK des Lüftungsgeräts (max. 3 Minuten).","orcon.status_confirming":"ACK empfangen; Bestätigung nach Rückkehr zum Empfang.","orcon.status_awaiting_user":"ACK und Bestätigung gesendet. Nur bei tatsächlicher Geräteantwort bestätigen.","orcon.status_failed":"Kopplung fehlgeschlagen oder abgelaufen; Steuerung bleibt gesperrt.","orcon.status_paired":"Vom Benutzer bestätigte Kopplung; experimentelle Steuerung verfügbar.","orcon.status_not_paired":"Noch nicht gekoppelt.","orcon.test_auto":"Auto einmal testen","orcon.confirm_test_auto":"Jetzt einen experimentellen Auto-Befehl senden? Reaktion des Lüfters selbst prüfen.","orcon.sent_unconfirmed":"Auto gesendet; Geräteantwort nicht bestätigt.","deviceux.back":"← Zurück zu Geräten","deviceux.detail_help":"Hier Gerät einrichten, koppeln und Funksignale verwalten. Bedienung bleibt in der Übersicht.","deviceux.setup_device":"Gerät einrichten","deviceux.view_details":"Details ansehen","deviceux.settings":"Einrichten und koppeln","deviceux.no_profile_help":"Wähle das Gerätemodell, um seine Funktionen zu sehen.","deviceux.profile_unavailable":"Das gespeicherte Profil konnte nicht geladen werden. Wähle erneut ein Profil; die Funkkennung bleibt erhalten.","logs.group_radio":"Funk","logs.radio_help":"Signale je Funkmodul und gemeinsames Rauschprotokoll.","logs.group_system":"System","logs.system_help":"Entscheidungen über Decoder, Geräte, Kopplung und Senden.","logs.group_web":"Web","logs.web_help":"Navigation, Klicks und API-Fehler dieser Browsersitzung.","version.subtitle":"Gerätesteuerung, gemeinsame Protokolle und experimentelles Senden.","version.notes":"Neuigkeiten und technische Details","version.changes":"Neu in dieser Version","version.raw":"Interne Firmwareversion","version.technical":"Technische Details","version.legacy":"Installierte Firmware (alte Versionsbezeichnung).","version.legacy_note":"Für diesen älteren Build die ursprünglichen Versionshinweise lesen.","version.note1":"Kompakte Geräteübersicht mit getrennten Einstellungen und Koppeln.","version.note2":"Funk-, System- und Webereignisse im gemeinsamen Protokollfenster.","version.note3":"Gruppierte Protokolleinstellungen und direkt sichtbare Bibliothek.","version.note4":"API-Zeitmessungen für Geräte und Bibliothek zur Fehlerdiagnose.","version.lab_tools":"TX-Labor und Systemdiagnose","version.lab_help":"Diese Seite dient der experimentellen Funk-/TX-Diagnose. Normale Systemereignisse: Protokolle → System.","version.open_logs":"Systemprotokoll öffnen","deviceux.remove_short":"Entfernen","logbook6.frequency":"Frequenz","logbook6.time":"Zeit","logbook6.source":"Quelle","logbook6.event":"Ereignis","logbook6.filtered_empty":"Keine passenden Ereignisse. Aktivieren Sie Rauschen anzeigen, um Rauschmeldungen einzuschließen.","deviceux6.profile_title":"Geräteprofil wählen","deviceux6.profile_help":"Wählen Sie das Gerät. Danach erscheinen passende Funktionen und Kopplungsschritte.","deviceux6.pair_title":"Koppeln und testen","deviceux6.pair_help":"Koppeln Sie das Gerät mit RFLink, falls das Profil dies unterstützt.","deviceux6.status_title":"Aktueller Status","deviceux6.no_profile_yet":"Nach der Profilauswahl erscheinen hier die verfügbaren Funktionen.","deviceux6.extra_title":"Weitere Geräteverwaltung","deviceux6.intro":"Profil wählen, bei Bedarf koppeln und weitere Optionen verwalten.","logbook6.info":"Info","logbook6.warn":"Warnung","logbook6.error":"Fehler","logbook6.noise":"Rauschen","diag7.event_type":"Typ","diag7.boot_filter":"Abstürze und Neustarts","diag7.tx_filter":"Senden und Koppeln","diag7.api_filter":"API und Browser","diag7.view_log":"Absturzprotokoll ansehen","diag7.crashes":"{n} Abstürze in den letzten 8 Starts","diag7.crash_help":"RFLink wurde nach einem Fehler neu gestartet. Der genaue Grund erfordert weitere Diagnose.","diag7.export":"Diagnosepaket (JSON)","diag7.uptime_note":"Zeitangaben hh:mm:ss,mmm seit ESP32-Start; Browserzeiten sind Näherungen.","diag7.health_attention":"Achtung: frühere Abstürze","diag7.health_detail":"Funkmodule und Decoder sind erreichbar, aber RFLink wurde zuletzt nach Fehlern neu gestartet.","diag7.raw_trace":"Technischer Trace (optional)","version.source_hash":"Quellhash (SHA-256, gekürzt)","version.history_time":"Früherer Start","version.history_event":"Start #{n} · Resetgrund: {reason}","version.history_note":"Gespeicherter NVS-Eintrag ohne genaue Zeit, Ursache oder Stacktrace.","version.note5":"v0.6.1 Radio Gaga: verbesserte experimentelle CC1101/GDO-UART-Sendevorbereitung.","version.note6":"UART1 wird vor der 8N1-Konfiguration installiert, mit gezielten Fehlercodes je Schritt.","version.note7":"Das Funklog zeigt TX-Anfragen, Modem-/UART-Schritte, Fehler und die RX-Wiederherstellung.","version.note8":"Gerätedetails vereinfacht: Funktionen, Home Assistant und Funkverwaltung ohne verschachteltes Hauptmenü.","crashtrace.reason":"Absturzgrund","crashtrace.core":"Prozessorkern","crashtrace.firmware":"Firmware-Build des Absturzes","crashtrace.other_build":"Anderer/älterer Build (ursprüngliche ELF nötig)","crashtrace.unknown_build":"Unbekannter Build","crashtrace.backtrace":"Stacktrace","crashtrace.other_core":"Anderer Kern","crashtrace.elf_note":"Rohadressen: nur mit der originalen firmware.elf des Absturz-Builds auswerten. Keine genaue historische Uhrzeit.","crashtrace.available":"Stacktrace verfügbar","crashtrace.boot_event":"Absturz-Stacktrace bei Start #{n}","crashtrace.unknown_boot":"Älterer Stacktrace (Startnummer unbekannt)","diag55.tab":"Diagnose","diag55.title":"Alle Diagnosen an einem Ort","diag55.intro":"Funk- und SPI-Status, Neustarts, Absturzdetails und experimentelles Senden. Ereignisse stehen im gemeinsamen Radio- und Systemprotokoll.","diag55.radio_heading":"Funkgeräte, SPI und Startverlauf","diag55.experimental_heading":"Experimentelles TX und Systementscheidungen","diag55.open_crashes":"Abstürze anzeigen","diag55.open_radio":"Funkprotokoll anzeigen","diag55.spi_filter":"SPI / Funkwiederherstellung","diag55.boots":"Abstürze & Neustarts","diag55.radio":"Funk & SPI","diag55.memory":"Speicher","diag55.lab":"Experimentell","diag55.export":"Diagnosebericht (JSON)","diag55.boot_hint":"Frühere Starts und vorhandene Traces ohne erfundene Zeitangaben.","diag55.lab_hint":"TX, UART/GDO und erweiterte Systemdiagnose. Keine automatische Testsendung.","diag55.ready":"Empfang bereit","diag55.crashes":"Abstürze","diag55.internal":"Interner Speicher frei","diag55.trace":"Technischer Absturz","diag55.nohistory":"Kein Startverlauf verfügbar.","diag55.online":"online","diag55.offline":"offline","diag55.errors":"Fehler","diag55.restores":"Wiederherstellungen","diag55.no_radio":"Keine Funkmodule verfügbar.","diag55.lab_enabled":"Lab aktiviert","diag55.lab_disabled":"Lab deaktiviert","diag55.adapter_ready":"Adapter bereit","diag55.adapter_unready":"Adapter nicht bereit","pair55.queued":"Kopplungsauftrag eingereiht. Ergebnis im Radio- und Systemprotokoll prüfen.","pair55.auto_queued":"Auto-Test eingereiht. Funksendung noch nicht bestätigt.","orcon.status_queued":"Kopplungsauftrag eingereiht. Ergebnis im Radio- und Systemprotokoll prüfen.","rf60.status_intro":"Ruhige Übersicht über System, Funkgeräte und wichtige Hinweise.","rf60.system":"System","rf60.radios":"Funkgeräte","rf60.radios_help":"Verfügbare Funkgeräte","rf60.devices":"Geräte","rf60.saved":"Gespeicherte Geräte","rf60.attention":"Hinweise","rf60.attention_title":"Handlungsbedarf","rf60.attention_help":"Nur Punkte, die eine Aktion erfordern.","rf60.no_problems":"Keine Probleme erkannt.","rf60.needs_attention":"Prüfung nötig","rf60.radio_overview":"Funkübersicht","rf60.radio_help":"Funkgerät öffnen, um Empfang und Senden zu prüfen.","rf60.activity_help":"Letzte Ereignisse; alle Details stehen im Protokoll.","rf60.open_logs":"Gesamtes Protokoll öffnen","rf60.system_details":"Technischer Systemstatus","rf60.quick_actions":"Schnellaktionen","rf60.quick_help":"Häufige Aufgaben direkt erreichbar.","rf60.last_seen":"Letztes Signal","rf60.not_seen":"Noch nicht empfangen","rf60.more_details":"Mehr Details","rf60.control":"Bedienen","rf60.values":"Messwerte","rf60.no_values":"Noch keine Messwerte verfügbar.","rf60.protocol":"Protokoll","rf60.controls_title":"Steuerung und Funktionen","rf60.advanced_details":"Signale und technische Details","rf60.advanced_help":"Empfangene Signale, RF-Kennungen, Home Assistant und Verwaltung.","rf60.remove_help":"Entfernt dieses Gerät und seine RFLink-Konfiguration. Eine Bestätigung ist erforderlich.","rf60.inspect":"Diagnose ansehen","rf60.history_issues":"Frühere Abstürze: {n}","rf61.functions_title":"Empfangene Funktionen","rf61.functions_help":"{n} erkannte Funktionen. Neue Funktionen prüfen und verwalten.","rf61.ha_title":"Verwaltung und Home Assistant","rf61.ha_help":"Gerät zu Home Assistant hinzufügen oder umbenennen.","rf61.signals_title":"Funksignale verwalten","rf61.signals_help":"Verknüpfte RF-Codes ansehen oder ersetzen.","rf61.guide_title":"Einrichtungshilfe","rf61.guide_help":"Hinweise zum gewählten Geräteprofil.","rf61.id_title":"Technische Geräte-ID","rf61.id_help":"Interne Kennung für Support und Fehlersuche.","version.note9":"v0.6.2 Radio Gaga: ein Startmenü für Updates, saubere Releases und Tests; Anleitungen und ältere Dokumente liegen in docs/."}};
const RF_SUPPORTED_LOCALES = ['en', 'nl', 'fr', 'de'];
let rfLanguage = 'en';
function rfTranslate(key) { return (RF_UI_LOCALES[rfLanguage] && RF_UI_LOCALES[rfLanguage][key]) || RF_UI_LOCALES.en[key] || key; }
function rfFormat(key, values={}) { return rfTranslate(key).replace(/\{([A-Za-z0-9_]+)\}/g, (m,k)=>Object.prototype.hasOwnProperty.call(values,k)?String(values[k]):m); }
function rfProtocolText(p, part) { const key='protocol.'+p.name+'.'+part; const translated=rfTranslate(key); if(translated!==key)return translated; const prop=part==='band'?'learn_band':part==='instruction'?'learn_instruction':part==='ha_result'?'learn_ha_result':'learn_warning'; return p[prop]||''; }
function rfFieldLabel(key, fallback) { const k='field.'+key, translated=rfTranslate(k); return translated===k?(fallback||key):translated; }
function rfApplyLanguage(language) {
  rfLanguage = RF_SUPPORTED_LOCALES.includes(language) ? language : 'en';
  document.documentElement.lang = rfLanguage;
  document.querySelectorAll('[data-i18n]').forEach(el => { el.textContent = rfTranslate(el.dataset.i18n); });
  document.querySelectorAll('[data-i18n-placeholder]').forEach(el => { el.placeholder = rfTranslate(el.dataset.i18nPlaceholder); });
  document.querySelectorAll('[data-i18n-aria-label]').forEach(el => { el.setAttribute('aria-label', rfTranslate(el.dataset.i18nAriaLabel)); });
  document.querySelector('#uiLanguage').value = rfLanguage;
  document.dispatchEvent(new CustomEvent('rflink-language-changed'));
}
(()=>{const $=(s)=>document.querySelector(s),$$=(s)=>[...document.querySelectorAll(s)];let page='status',selectedChannel=-1,monitorChannel=-1,monitorDirection='all',monitorLogType='signal',rxLogRows=[],rxLogEnabled=false,rxLogNextCursor=null,rxLogRequestId=0,rxLogError='',latestStatus=null,lastDeviceView=null,learnMode='device',pairOnly=false,deviceFocus='',pendingIdentity='',baselineTimer=null,ledDirty=false,wizardStage='idle',wizardStartedAt=0,wizardSelected=null,wizardTick=0,wizardToken=0;const esc=s=>String(s??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));const age=ms=>ms==null?'—':ms<1000?rfTranslate('dyn.now'):ms<60000?rfFormat('dyn.sec',{n:Math.round(ms/1000)}):ms<3600000?rfFormat('dyn.min',{n:Math.round(ms/60000)}):rfFormat('dyn.hour',{n:Math.round(ms/3600000)});const uptime=ms=>{if(ms==null)return'—';const t=Math.floor(ms/1000),d=Math.floor(t/86400),h=Math.floor(t%86400/3600),m=Math.floor(t%3600/60),sec=t%60;return (d?d+'d ':'')+(d||h?h+'h ':'')+(d||h||m?m+'m ':'')+sec+'s'};const ledStatusKeys={boot:'led.boot',opstarten:'led.boot',connecting:'led.connecting',wifi_verbinden:'led.connecting',ready:'led.ready',ok:'led.ready',gereed:'led.ready',offline:'led.offline',verbinding_verloren:'led.offline',radio_missing:'led.radio_missing',radio_ontbreekt:'led.radio_missing',learning:'led.learning',inleren:'led.learning',capture:'led.capture',signaal_ontvangen:'led.capture',learned:'led.learned',apparaat_bevestigd:'led.learned',pairing:'led.pairing',koppelopdracht:'led.pairing',itho_pair:'led.pairing',itho_koppelopdracht:'led.pairing',tx:'led.tx',rf_verzenden:'led.tx',ota:'led.ota',error:'led.error',fout:'led.error'};const ledStatusLabel=status=>rfTranslate(ledStatusKeys[String(status||'').toLowerCase()]||'led.unknown');function renderLedStatus(led){if(!led)return;const label=ledStatusLabel(led.status_code||led.status);const onStatus=$('#ledStatus');if(onStatus)onStatus.textContent=label;const settings=$('#ledStatusSettings');if(settings)settings.textContent=label;}function toast(t){const e=$('#toast');e.textContent=t;e.classList.add('show');setTimeout(()=>e.classList.remove('show'),1800)}async function req(url,opt){const r=await rfTrackedFetch(url,opt);if(!r.ok)throw new Error(await r.text());return r.json()}const get=u=>req(u);const post=u=>req(u,{method:'POST'});const expandedSignals=new Set(),expandedCapabilities=new Set(),expandedMore=new Set(),expandedDangerous=new Set(),replacementSelections=new Map();let decoderProtocols=new Map(),haProfiles=new Map(),libraryState=null;let deviceSearchText='',deviceRadioChoice='all';
// All wording and onboarding steps originate from each decoder's manifest.
const helpTopics={
 library:{title:'help.library_title',text:'help.library_text',more:'help.library_more'},
 decoder_source:{title:'help.source_title',text:'help.source_text',more:'help.source_more'},
 stable_id:{title:'help.id_title',text:'help.id_text',more:'help.id_more'}
};
function openHelp(key){const h=helpTopics[key];if(!h)return;$('#helpModalTitle').textContent=rfTranslate(h.title);$('#helpModalText').textContent=rfTranslate(h.text);$('#helpModalMore').textContent=rfTranslate(h.more);$('#helpModal').hidden=false;$('#helpModalClose').focus()}
function closeHelp(){$('#helpModal').hidden=true}
function bindHelpButtons(){$$('[data-help]').forEach(b=>b.onclick=()=>openHelp(b.dataset.help))}
$('#helpModalClose').onclick=closeHelp;$('#helpModal').onclick=e=>{if(e.target===$('#helpModal'))closeHelp()};document.addEventListener('keydown',e=>{if(e.key==='Escape'&&!$('#helpModal').hidden)closeHelp()});
bindHelpButtons();
const keys=s=>String(s||'').split('|').filter(Boolean);
// Small, independent built-in profile catalog; full custom library is lazy-loaded.
let rfProfileCatalogStatus='loading';
let rfProfilesRequest=null;
async function loadProfileCatalog(){
 if(rfProfilesRequest)return rfProfilesRequest;
 rfProfilesRequest=(async()=>{
  try{
   const response=await rfTrackedFetch('/rflink/api/profile-catalog',{cache:'no-store'});
   if(!response.ok)throw Error('profile catalog HTTP '+response.status);
   const data=await response.json();
   if(!Array.isArray(data.profiles)||!data.profiles.length)throw Error('empty profile catalog');
   const prior=window.rfUserLibraryCache||{};
   const merged=new Map((prior.profiles||[]).map(p=>[p.id,p]));
   for(const p of data.profiles)merged.set(p.id,{...merged.get(p.id),...p});
   window.rfUserLibraryCache={...prior,profiles:[...merged.values()]};
   rfProfileCatalogStatus='ready';
   document.dispatchEvent(new CustomEvent('rflink-user-library-changed'));
  }catch(error){
   rfProfileCatalogStatus='unavailable';
   if(!(window.rfUserLibraryCache?.profiles||[]).length){
    // The lightweight catalog endpoint might not exist in an older firmware.
    // A library listing is sufficient to populate basic profile choices.
    try{
     const legacy=await(await rfTrackedFetch('/rflink/api/library')).json();
     const profiles=(legacy.items||[]).filter(x=>x.kind==='profile').map(x=>({id:String(x.id).replace(/^profile:/,''),name:x.name,protocol:x.protocol||'',source:x.source})).filter(x=>x.protocol);
     if(profiles.length){window.rfUserLibraryCache={...(window.rfUserLibraryCache||{}),profiles};rfProfileCatalogStatus='partial';}
    }catch(_ignored){}
   }
   document.dispatchEvent(new CustomEvent('rflink-user-library-changed'));
  }finally{rfProfilesRequest=null;}
 })();return rfProfilesRequest;
}
async function loadProfileDetail(id){
 const profile=(window.rfUserLibraryCache?.profiles||[]).find(p=>p.id===id);
 if(!profile||profile.setup_steps_i18n)return profile;
 try{
  const response=await rfTrackedFetch('/rflink/api/profile-catalog?id='+encodeURIComponent(id));
  if(!response.ok)throw Error('profile detail HTTP '+response.status);
  const data=await response.json(),detail=data.profiles?.find(p=>p.id===id);
  if(detail){Object.assign(profile,detail);return profile;}
 }catch(_ignored){}
 return profile;
}
document.addEventListener('rflink-user-library-changed',()=>{if(page==='devices')loadDevices()});
const getProtocol=name=>decoderProtocols.get(name)||(window.rfUserLibraryCache?.decoders||[]).filter(x=>x.id===name).map(x=>({name:x.id,source:'user',learn_category:'control',learn_band:'User protocol',learn_instruction:(window.rfUserLibraryCache?.profiles||[]).find(p=>p.protocol===x.id)?.learn_help||'Press the learned button three times.',learn_ha_result:'Approved actions appear as events.',learn_warning:'Hardware TX must be verified.',rx_actions:x.actions.join('|'),fields:''}))[0]||null;
const actionLabel=(p,key)=>{
 const common={auto:'action.auto',away:'action.away',on:'action.on',off:'action.off',up:'action.up',down:'action.down',my:'action.my',prog:'action.prog',low:'action.low',medium:'action.medium',high:'action.high',standby:'action.standby',status:'action.status',measurement:'action.measurement',rain_update:'action.rain_update',weather_update:'action.weather_update',code:'action.code'};
 if(common[key])return rfTranslate(common[key]);
 return key.startsWith('button_')?rfFormat('action.button',{n:key.slice(7)}):key.startsWith('command_')?rfFormat('action.command',{n:key.slice(8)}):key.startsWith('timer_')?rfFormat('action.timer',{n:key.slice(6)}):key;
};
const publicFields=c=>(c.fields||[]).filter(f=>haProfiles.has(f.key));
const visibleActions=c=>getProtocol(c.protocol)?.learn_category==='sensor'?[]:(c.actions||[]);
function instructions(c){const p=getProtocol(c.protocol);if(!p)return rfFormat('dyn.decoder_review',{protocol:esc(c.protocol)});
 return '<b>'+esc(p.name)+' · '+esc(rfProtocolText(p,'band'))+'</b><p>'+esc(rfProtocolText(p,'instruction'))+'</p><p class="muted small">Home Assistant: '+esc(rfProtocolText(p,'ha_result'))+'</p><p class="review-warning">'+esc(rfProtocolText(p,'warning'))+'</p>';
}
function renderReview(c,prefix){
 if(!getProtocol(c.protocol)){
  const wrap=$('#'+prefix+'Features');wrap.hidden=false;
  $('#'+prefix+'FeatureOptions').innerHTML='<div class="notice warn small">'+esc(rfTranslate('dyn.decoder_missing'))+'</div>';
  return -1;
 }
 const actions=visibleActions(c),fields=publicFields(c),features=[...actions.map(key=>({type:'action',key,label:actionLabel(getProtocol(c.protocol),key)})),...fields.map(f=>({type:'field',key:f.key,label:rfFieldLabel(f.key,haProfiles.get(f.key))}))];
 const wrap=$('#'+prefix+'Features'),target=$('#'+prefix+'FeatureOptions');wrap.hidden=!features.length;
 target.innerHTML=features.map(f=>'<label class="review-choice"><input type="checkbox" data-review="'+esc(prefix)+'" data-kind="'+f.type+'" value="'+esc(f.key)+'" checked><span>'+esc(f.label)+'</span></label>').join('');
 return features.length;
}
function selectedReview(prefix,c){
 const selected=$$('[data-review="'+prefix+'"]:checked');return {
  actions:selected.filter(e=>e.dataset.kind==='action').map(e=>e.value),
  fields:selected.filter(e=>e.dataset.kind==='field').map(e=>e.value),
  count:selected.length,
  total:visibleActions(c).length+publicFields(c).length
 };
}
function approvalParams(prefix,c){const approved=selectedReview(prefix,c);
 return '&approved_actions='+encodeURIComponent(approved.actions.join(','))+'&approved_fields='+encodeURIComponent(approved.fields.join(','));
}
// NAV4: hash routes use the same /rflink page. Back and Forward restore the visible view.
const navPages=new Set(['status','devices','monitor','learn','settings']);
let navigatingFromHistory=false;
function navLocation(){
 let hash=(location.hash||'').replace(/^#/,'');
 const [view,tool]=hash.split('/');
 return {view:navPages.has(view)?view:'status',tool:view==='settings'&&['library','decoders','custom','diagnostics'].includes(tool)?tool:''};
}
function navRecord(view,tool=''){
 if(navigatingFromHistory)return;
 const next='#'+view+(view==='settings'&&tool?'/'+tool:'');
 if(location.hash!==next)history.pushState({view,tool},'',next);
}
function navRestore(){
 const target=navLocation();navigatingFromHistory=true;
 show(target.view);
 if(target.view==='settings'){
  document.querySelectorAll('.settings-detail-panel').forEach(p=>p.hidden=true);
  document.querySelectorAll('[data-settings-open]').forEach(b=>b.setAttribute('aria-expanded','false'));
  if(target.tool)showSettingsPanel(target.tool,false,true);
 }
 navigatingFromHistory=false;
}
window.addEventListener('popstate',navRestore);
window.addEventListener('hashchange',navRestore);
function showSettingsPanel(which,scrollToContent,forceOpen=false){
  if(which==='diagnostics'||which==='systemtrace'){show('monitor');window.rfLogbookSelectView?.('diagnostics');return;}
 const panel=document.getElementById('settingsPanel'+which.charAt(0).toUpperCase()+which.slice(1));
 const opening=!!panel&&(panel.hidden||forceOpen);
 document.querySelectorAll('.settings-detail-panel').forEach(p=>p.hidden=true);
 document.querySelectorAll('[data-settings-open]').forEach(b=>b.setAttribute('aria-expanded','false'));
 if(opening){
  panel.hidden=false;
  const btn=document.querySelector('[data-settings-open="'+which+'"]');
  if(btn)btn.setAttribute('aria-expanded','true');
  const details=panel.querySelector(':scope > .card > details');if(details)details.open=true;
  if(scrollToContent)panel.scrollIntoView({behavior:'smooth',block:'start'});
 }
 if(page==='settings')navRecord('settings',opening?which:'');
}
document.querySelectorAll('[data-settings-open]').forEach(b=>b.addEventListener('click',()=>{
 showSettingsPanel(b.dataset.settingsOpen,true);
 if(b.dataset.settingsOpen==='systemtrace')refreshSystemTrace();
}));
document.querySelectorAll('[data-settings-back]').forEach(b=>b.addEventListener('click',()=>{
 showSettingsPanel(b.closest('.settings-detail-panel').id.replace('settingsPanel','').toLowerCase(),false);
 document.querySelector('.page[data-view="settings"] .settings-pickers').scrollIntoView({behavior:'smooth',block:'nearest'});
}));
function show(name){
 if(!navPages.has(name))return;
 const changed=page!==name;
 page=name;
 if(changed && window.rfLog)window.rfLog('navigation','Open '+name,'info');
 $$('.page').forEach(x=>x.classList.toggle('active',x.dataset.view===name));
 $$('[data-page]').forEach(x=>x.classList.toggle('active',x.dataset.page===name));
 if(name==='devices')loadDevices();
 if(name==='monitor'){if(latestStatus)renderMonitor(latestStatus);else loadStatus();loadRxLogSettings();if(window.rfLogbookOpen)window.rfLogbookOpen()}
 if(name==='learn'){setupLearnRadios();loadDeveloper()}
 if(name==='settings'){loadDecoders();loadLibrary();loadRxLogSettings();if(window.rfLogbookSettings)window.rfLogbookSettings();if(!mqttLoaded)loadMqttSettings()}
 if(changed&&!navigatingFromHistory)navRecord(name);
 window.scrollTo({top:0,behavior:navigatingFromHistory?'instant':'smooth'});
}
$$('[data-page]').forEach(b=>b.onclick=()=>{if(b.dataset.page==='devices'){pairOnly=false;deviceFocus='';deviceDetailUid='';}show(b.dataset.page)});
$$('[data-go]').forEach(b=>b.onclick=()=>{
 if(b.dataset.learnMode)setLearnMode(b.dataset.learnMode);
 if(b.dataset.go==='learn')$('#learnProtocolGuide').hidden=true;
 if(b.dataset.go==='devices'){pairOnly=false;deviceFocus=b.dataset.focusDevices||'';deviceDetailUid='';}
 const shortcut=!!b.dataset.openDetails&&b.dataset.go==='settings';
 if(shortcut)navigatingFromHistory=true;
 show(b.dataset.go);
 if(b.dataset.logTarget==='diagnostics')window.rfLogbookSelectView?.('diagnostics');
 if(shortcut)navigatingFromHistory=false;
 if(b.dataset.openDetails){
  const d=document.getElementById(b.dataset.openDetails);
  if(d){const p=d.closest('.settings-detail-panel');if(p&&p.hidden)showSettingsPanel(p.id.replace('settingsPanel','').toLowerCase(),false);d.open=true;setTimeout(()=>d.closest('.card')?.scrollIntoView({behavior:'smooth',block:'start'}),50)}
 }
});
// MQTT configuration API uses POST form body; passwords never appear in query strings or GET responses.
let mqttLoaded=false;
async function loadMqttSettings(){
 try{
  const r=await get('/rflink/api/mqtt-settings');
  if(!r.ok)throw new Error('Unavailable');
  const fields=$('#mqttRuntimeFields'), info=$('#mqttRuntimeInfo');
  fields.hidden=!r.compiled;
  info.textContent=r.compiled?'':rfTranslate('nav4.mqtt_not_built');
  info.hidden=!!r.compiled;
  if(!mqttLoaded||!r.compiled){
   $('#mqttHost').value=r.host||'';$('#mqttPort').value=r.port||1883;
   $('#mqttUser').value=r.username||'';$('#mqttEnabled').checked=!!r.enabled;
   $('#mqttPassword').placeholder=r.saved_password?'••••••••':'';
  }
  mqttLoaded=!!r.compiled;
 }catch(e){$('#mqttRuntimeInfo').textContent=rfTranslate('nav4.mqtt_not_built')}
}
$('#mqttSave').onclick=async()=>{
 const state=$('#mqttSaveState');state.textContent='';
 const button=$('#mqttSave');button.disabled=true;
 try{
  const auth=await get('/rflink/api/mqtt-settings');
  const params=new URLSearchParams({write_token:String(auth.write_token),host:$('#mqttHost').value.trim(),port:$('#mqttPort').value,
   username:$('#mqttUser').value,password:$('#mqttPassword').value,enabled:$('#mqttEnabled').checked?'1':'0'});
  const r=await rfTrackedFetch('/rflink/api/mqtt-settings',{method:'POST',body:params,cache:'no-store'});
  const payload=await r.json();if(!r.ok||!payload.ok)throw Error(payload.error||'Save failed');
  $('#mqttPassword').value='';state.textContent=rfTranslate('nav4.mqtt_saved');mqttLoaded=false;await loadMqttSettings();
 }catch(e){state.textContent=rfTranslate('nav4.mqtt_bad')+': '+String(e.message||e)}finally{button.disabled=false}
};
// Deep link: Settings -> Decoder library/Custom -> Create decoder (max 3 clicks).
document.querySelectorAll('[data-launch-decoder]').forEach(b=>b.addEventListener('click',()=>{
 show('learn');setLearnMode('decoder');
 const target=document.querySelector('#expertLearn');
 if(target)target.scrollIntoView({behavior:'smooth',block:'start'});
}));
function setLearnMode(mode){learnMode=mode;$$('[data-learn]').forEach(b=>b.classList.toggle('active',b.dataset.learn===mode));$('#decoderTools').hidden=mode!=='decoder';$('#deviceWizard').hidden=mode==='decoder';$('#expertLearn').hidden=mode!=='decoder';$('#deviceLearnHelp').hidden=mode==='decoder';$('#radioSelectCard').hidden=mode==='device'&&wizardStage!=='idle';$('#learnIntro').hidden=mode==='decoder';$('#learnAdvancedMode').hidden=mode==='decoder';$('#learnInstruction').textContent=rfTranslate(mode==='decoder'?'dyn.dev_instruction':'dyn.device_instruction')}$$('[data-learn]').forEach(b=>b.onclick=()=>setLearnMode(b.dataset.learn));
function radioLabel(r){return r.checked?(r.available?(r.rx_ready?rfTranslate('dyn.rx_ready'):rfTranslate('dyn.rx_unavailable')):rfTranslate('dyn.disconnected')):rfTranslate('dyn.checking')}
function renderLearnRadios(channels){
 const ready=channels.filter(r=>r.checked&&r.available&&r.rx_ready);
 if(!ready.some(r=>r.index===selectedChannel)){const common=ready.find(r=>String(r.frequency_label||r.name||'').includes('433.92'));selectedChannel=common?common.index:(ready.length?ready[0].index:-1)}
 const frequencyHint=r=>{const f=String(r.frequency_label||r.name||'');return f.includes('433.92')?rfTranslate('ui.freq_43392'):f.includes('433.42')?rfTranslate('ui.freq_43342'):f.includes('868')?rfTranslate('ui.freq_868'):rfTranslate('ui.freq_other')};
 $('#learnRadios').innerHTML=channels.map(r=>`<button class="choice ${r.index===selectedChannel?'active':''}" data-ch="${r.index}" ${r.checked&&r.available&&r.rx_ready?'':'disabled title="'+esc(rfTranslate('dyn.radio_unavailable_title'))+'"'}><b>${esc(r.frequency_label)}</b><span class="frequency-hint">${esc(frequencyHint(r))}</span></button>`).join('')||'<span class="muted">'+esc(rfTranslate('dyn.no_radios'))+'</span>';
 $$('[data-ch]').forEach(b=>b.onclick=()=>{if(b.disabled)return;if(wizardStage!=='idle')return;selectedChannel=Number(b.dataset.ch);renderLearnRadios(channels)});
 const canLearn=selectedChannel>=0;
 $('#wizardStart').disabled=!canLearn||wizardStage!=='idle'||!!(latestStatus&&(!latestStatus.storage_ok||!latestStatus.storage_ready));
 $('#baselineBtn').disabled=!canLearn;$('#listenBtn').disabled=!canLearn;
 $('#learnRadioHelp').textContent=canLearn?rfTranslate('ui.frequency_help'):channels.some(r=>!r.checked)?rfTranslate('dyn.radio_checking'):rfTranslate('dyn.no_working_receivers');
}
function monitorFilteredEvents(events){return (events||[]).filter(a=>(monitorChannel===-1||a.channel_index===monitorChannel)&&(monitorDirection==='all'||a.direction===monitorDirection)&&((monitorLogType==='all')||(monitorLogType==='noise'?a.noise_suspect:!a.noise_suspect)))}
async function fetchRxLogPage(older=false){
 const requestId=++rxLogRequestId;
 if(!older){rxLogRows=[];rxLogNextCursor=null;}
 const args=new URLSearchParams({limit:'20',order:'desc'});
 if(monitorChannel>=0)args.set('radio',monitorChannel);
 if(monitorLogType!=='all')args.set('kind',monitorLogType==='noise'?'1':'0');
 if(older&&rxLogNextCursor!=null)args.set('cursor',rxLogNextCursor);
 try{
  const result=await get('/rflink/api/rx-log?'+args);
  if(requestId!==rxLogRequestId)return;
  rxLogEnabled=true;rxLogError='';
  const rows=(result.events||[]).map(e=>({...e.record,seq:e.seq,radio_id:e.radio,category:e.category,
    age_ms:latestStatus?Math.max(0,(latestStatus.uptime_ms||0)-e.uptime_ms):0}));
  rxLogRows=older?rxLogRows.concat(rows):rows;
  rxLogNextCursor=rows.length?String(rows[rows.length-1].seq):null;
  $('#rxLogOlder').hidden=!result.has_more;
  if(latestStatus)renderMonitor(latestStatus);
 }catch(e){if(requestId!==rxLogRequestId)return;rxLogEnabled=false;rxLogError=e.message||String(e);$('#rxLogOlder').hidden=true;if(latestStatus)renderMonitor(latestStatus)}
}
async function loadRxLogSettings(){
 try{
  const c=await get('/rflink/api/rx-log-config');
  (c.signal_limits||[]).forEach((v,i)=>{$('#rxLogLimit'+i).value=v});
  $('#rxLogNoiseLimit').value=c.noise_limit;
  const fmtBytes=v=>Number.isFinite(Number(v))&&v!==null&&v!==undefined?(Number(v)/1024).toFixed(0):'—';
  const signals=Array.isArray(c.signals)?c.signals.join(' / '):'—';
  const noise=Number.isFinite(Number(c.noise_total))&&c.noise_total!==null&&c.noise_total!==undefined?String(c.noise_total):'—';
  const info=`${rfTranslate('rxlog.stored')}: ${signals} ${rfTranslate('rxlog.signals').toLowerCase()} · ${noise} ${rfTranslate('rxlog.noise').toLowerCase()} | ${fmtBytes(c.log_bytes_used)} / ${fmtBytes(c.arena_bytes)} KB`;
  $('#rxLogMemory').textContent=info;
  $('#rxLogStats').textContent=info;
 }catch(e){$('#rxLogMemory').textContent=e.message;}
}
async function saveRxLogSettings(){
 const vals=[0,1,2].map(i=>$('#rxLogLimit'+i).value),noise=$('#rxLogNoiseLimit').value;
 if(![...vals,noise].every(x=>/^\d+$/.test(x)&&Number(x)<=4096)||vals.reduce((a,b)=>a+Number(b),Number(noise))>4096){
  $('#rxLogSettingStatus').textContent=rfTranslate('rxlog.invalid');return;
 }
 try{
  const config=await get('/rflink/api/rx-log-config');
  const token=config.write_token;
  if(!token)throw new Error(rfTranslate('rxlog.token_error'));
  const body=new URLSearchParams({write_token:String(token),signal_0:vals[0],signal_1:vals[1],signal_2:vals[2],noise});
  await req('/rflink/api/rx-log-config',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});
  if(window.rfSaveLogbookSettings)await window.rfSaveLogbookSettings();
  $('#rxLogSettingStatus').textContent=rfTranslate('rxlog.saved');
  await loadRxLogSettings();await fetchRxLogPage();
 }catch(e){$('#rxLogSettingStatus').textContent=e.message;}
}
async function exportAllRxLogs(){
 try{
  $('#monitorExport').disabled=true;
  $('#rxLogStats').textContent=rfTranslate('rxlog.exporting');
  const events=[];let cursor='0';
  for(let i=0;i<100;i++){
    const result=await get('/rflink/api/rx-log?order=asc&limit=50&cursor='+cursor);
    const batch=result.events||[];
    for(const e of batch){
      if(!e.record)continue;
      const x=e.record||{};
      const radio=['433.42','433.92','868.30'][e.radio]||String(e.radio);
      events.push({seq:e.seq,radio,category:e.category,uptime_ms:e.uptime_ms,
        utc:'',protocol:x.protocol,raw:x.full_detail||'',rssi_dbm:x.rssi_dbm??'',lqi:x.lqi??'',freqest_hz:x.freqest_hz??'',reason:x.noise_reason||'',
        extra:{channel:x.channel,summary:x.summary,detail:x.detail,direction:x.direction,record:x}});
    }
    if(!result.has_more)break;
    const next=String(result.next_cursor);
    if(next===cursor)throw new Error('Non advancing log cursor');
    cursor=next;
  }
  const status=await get('/rflink/api/status');
  const config=await get('/rflink/api/rx-log-config');
  const zip=RFLogTools.zipStore(RFLogTools.exportFiles(events,{firmware_version:status.firmware_version,
      firmware_revision:status.firmware_revision,boot_session_id:status.boot_session_id,
      uptime_ms:status.uptime_ms,logging:config,exported_at:new Date().toISOString(),note:'UTC event timestamps unavailable; uptime is exact within current boot'}));
  const name='rflink-logs-'+new Date().toISOString().slice(0,19).replace(/[:T]/g,'-')+'.zip';
  const url=URL.createObjectURL(new Blob([zip],{type:'application/zip'}));const a=document.createElement('a');
  a.href=url;a.download=name;document.body.appendChild(a);a.click();a.remove();setTimeout(()=>URL.revokeObjectURL(url),3000);
  toast(rfTranslate('rxlog.exported'));await loadRxLogSettings();
 }catch(e){toast(e.message)}finally{$('#monitorExport').disabled=false}
}

// Keep activity cards mounted across automatic refreshes. Replacing the whole
// list with innerHTML every three seconds closes every expanded <details>.
// RX events have a stable sequence number; legacy TX has a stable timestamp.
let monitorRenderedView = '';
function monitorEventKey(a, seen){
 const base=a.seq!=null?'rx:'+String(a.seq):
  'legacy:'+String(a.direction||'RX')+':'+String(a.timestamp_ms??'')+':'+
  String(a.channel_index??'')+':'+String(a.protocol||'')+':'+String(a.summary||'');
 const n=seen.get(base)||0;seen.set(base,n+1);
 return base+':'+n;
}
function monitorCreateCard(a){
 const card=document.createElement('div');
 card.className='card'+(a.noise_suspect?' notice':'');
 card.innerHTML=`<div class="radiohead"><b>${esc(a.channel)} · ${esc(a.noise_suspect?rfTranslate('monitor.noise_badge'):(a.summary||rfTranslate('ui.signal_seen')))}</b><span class="muted small" data-monitor-age>${age(a.age_ms)}</span></div><details class="decoder-tech"><summary>${esc(rfTranslate('ui.advanced_details'))}</summary><p class="muted small">${esc(a.direction||'RX')} · ${esc(a.protocol||rfTranslate('dyn.unknown'))}</p>${a.noise_suspect&&a.noise_reason?`<p class="muted small">${esc(rfTranslate('monitor.noise_reason'))}: ${esc(a.noise_reason)}</p>`:''}${a.detail?`<div class="muted small mono" style="margin-top:7px">${esc(a.detail)}</div>`:''}</details>`;
 return card;
}
function renderMonitorActivity(events,d){
 const root=$('#monitorActivity');
 // Changing filters or boot sessions should discard pinned records. Ordinary
 // polling, loading newer pages and temporarily failing requests should not.
 const view=[monitorChannel,monitorDirection,monitorLogType,d.boot_session_id??'',rfLanguage].join('|');
 if(view!==monitorRenderedView){root.replaceChildren();monitorRenderedView=view;}
 const previous=new Map([...root.children].filter(el=>el.dataset.monitorKey).map(el=>[el.dataset.monitorKey,el]));
 const expanded=root.querySelector('[data-monitor-key] details[open]');
 const anchor=expanded?.closest('[data-monitor-key]');
 const anchorTop=anchor?anchor.getBoundingClientRect().top:null;
 const wanted=[];const seen=new Map();const visibleKeys=new Set();
 if(rxLogError){const error=document.createElement('div');error.className='card notice';error.textContent='RX-log: '+rxLogError;wanted.push(error);}
 for(const a of events){
  const key=monitorEventKey(a,seen);
  let card=previous.get(key);
  if(!card)card=monitorCreateCard(a);
  else {const ageElement=card.querySelector('[data-monitor-age]');if(ageElement)ageElement.textContent=age(a.age_ms);}
  card.dataset.monitorKey=key;visibleKeys.add(key);
  wanted.push(card);
 }
 // If an opened record ages off the newest 20 entries, keep that record
 // visible until the user closes it. Do not pin records across filter changes.
 for(const [key,card] of previous){
  if(!visibleKeys.has(key) && card.querySelector('details[open]'))wanted.push(card);
 }
 if(!wanted.length){const empty=document.createElement('div');empty.className='card empty';empty.textContent=rfTranslate('dyn.no_monitor_activity');wanted.push(empty);}
 const keep=new Set(wanted);
 for(const child of [...root.children])if(!keep.has(child))child.remove();
 wanted.forEach((child,i)=>{if(root.children[i]!==child)root.insertBefore(child,root.children[i]||null);});
 // Preserve the reader's viewport when newer records are inserted above it.
 if(anchor?.isConnected && anchorTop!=null && page==='monitor'){
  const shift=anchor.getBoundingClientRect().top-anchorTop;
  if(Math.abs(shift)>1 && typeof window.scrollBy==='function')window.scrollBy(0,shift);
 }
}
function renderMonitor(d){
 const channels=d.channels||[];
 if(monitorChannel>=0&&!channels.some(r=>r.index===monitorChannel))monitorChannel=-1;
 $('#monitorRadios').innerHTML=`<button class="choice ${monitorChannel===-1?'active':''}" data-mon="-1"><b>${esc(rfTranslate('dyn.all_radios'))}</b><br><span class="muted small">${esc(rfTranslate('dyn.overview'))}</span></button>`+channels.map(r=>`<button class="choice ${monitorChannel===r.index?'active':''}" data-mon="${r.index}" ><b>${esc(r.frequency_label)}</b><br><span class="muted small">${esc(r.name)} · ${esc(r.checked?(r.available?(r.rx_ready?rfTranslate('dyn.rx_ready_short'):rfTranslate('dyn.rx_fault')):rfTranslate('dyn.not_connected')):rfTranslate('dyn.checking_short'))}</span></button>`).join('');
 $$('[data-mon]').forEach(b=>b.onclick=()=>{monitorChannel=Number(b.dataset.mon);renderMonitor(d);if(window.rfLogbookRefreshRadio)window.rfLogbookRefreshRadio()});
 const selected=channels.find(r=>r.index===monitorChannel);
 $('#monitorHeading').textContent=selected?rfFormat('dyn.activity_radio',{name:selected.name}):rfTranslate('dyn.activity_all');
 const liveTx=(d.activity||[]).filter(a=>a.direction==='TX');
 const events=monitorFilteredEvents(rxLogEnabled?rxLogRows.concat(liveTx):d.activity||[]);
 renderMonitorActivity(events,d);
}

function monitorExportLine(a){
 const ageMs=Number(a.age_ms||0),when=new Date(Date.now()-ageMs),stamp=when.toLocaleTimeString([], {hour12:false,hour:'2-digit',minute:'2-digit',second:'2-digit',fractionalSecondDigits:3});
 const noise=a.noise_suspect?` | ${rfTranslate('monitor.noise_badge')}${a.noise_reason?': '+a.noise_reason:''}`:'';
 return `${stamp} | ${a.channel||'?'} | ${a.direction||'RX'} | ${a.protocol||rfTranslate('dyn.unknown')} | ${a.full_detail||a.detail||''}${noise}`;
}
async function monitorExportData(){if(rxLogEnabled)return monitorFilteredEvents(rxLogRows);const d=await get('/rflink/api/activity-export');return monitorFilteredEvents(d.activity||[])}
async function copyMonitor(){
 try{const events=await monitorExportData(),text=events.map(monitorExportLine).join('\n');if(!text){toast(rfTranslate('dyn.no_monitor_activity'));return}if(navigator.clipboard&&navigator.clipboard.writeText)await navigator.clipboard.writeText(text);else{const ta=document.createElement('textarea');ta.value=text;ta.style.position='fixed';ta.style.opacity='0';document.body.appendChild(ta);ta.select();document.execCommand('copy');ta.remove()}toast(rfTranslate('monitor.copied'))}catch(e){toast(e.message)}
}
async function exportMonitor(){
 try{const events=await monitorExportData(),text=events.map(monitorExportLine).join('\n');if(!text){toast(rfTranslate('dyn.no_monitor_activity'));return}const now=new Date(),pad=n=>String(n).padStart(2,'0'),name=`rflink-rx-${now.getFullYear()}-${pad(now.getMonth()+1)}-${pad(now.getDate())}_${pad(now.getHours())}-${pad(now.getMinutes())}-${pad(now.getSeconds())}.txt`;const url=URL.createObjectURL(new Blob([text+'\n'],{type:'text/plain;charset=utf-8'})),a=document.createElement('a');a.href=url;a.download=name;document.body.appendChild(a);a.click();a.remove();setTimeout(()=>URL.revokeObjectURL(url),1000);toast(rfTranslate('monitor.exported'))}catch(e){toast(e.message)}
}
function radioDiagnosisText(r){
 if(!r.checked)return rfTranslate('diag.checking');
 if(!r.available)return rfTranslate('diag.cc1101_failed');
 if(!r.rx_ready)return rfTranslate('diag.rx_failed');
 if(!r.tx_ready)return rfTranslate('diag.tx_failed');
 return rfTranslate('diag.ok');
}
function renderRadioDiagnostics(channels){
 const box=$('#radioDiagnostics');if(!box)return;
 box.innerHTML=(channels||[]).map(r=>{
   const ok=r.checked&&r.available&&r.rx_ready&&r.tx_ready;
   const wiring=[`CSN ${r.cs_pin||'—'}`,`GDO0 ${r.gdo0_pin||'—'}`,`GDO2 ${r.gdo2_pin||'—'}`].join(' · ');
   const spi=!r.checked?rfTranslate('diag.checking'):(r.available?rfTranslate('diag.spi_ok'):rfTranslate('diag.spi_failed'));
   const hex=v=>'0x'+Number(v==null?255:v).toString(16).toUpperCase().padStart(2,'0');
   const probe=r.self_test_done?(r.self_test_spi_ok?rfTranslate('diag.probe_response'):rfTranslate('diag.probe_no_response')):rfTranslate('diag.checking');
   const checks=Number(r.spi_checks||0), failures=Number(r.spi_failures||0);
   const health=checks?Math.max(0,100-(failures/checks*100)):100;
   return `<div class="card"><div class="radiohead"><b>${esc(r.frequency_label)} · ${esc(r.name)}</b><span class="${ok?'good':'pill warn'}">${esc(radioDiagnosisText(r))}</span></div><div class="rows"><div class="row"><span>${esc(rfTranslate('diag.cc1101_spi'))}</span><b>${esc(spi)}</b></div><div class="row"><span>${esc(rfTranslate('diag.spi_probe'))}</span><b>${esc(probe)}</b></div><div class="row"><span>PARTNUM</span><b class="mono">${esc(hex(r.partnum))}</b></div><div class="row"><span>VERSION</span><b class="mono">${esc(hex(r.version))}</b></div><div class="row"><span>${esc(rfTranslate('diag.spi_health'))}</span><b>${health.toFixed(1)}%</b></div><div class="row"><span>${esc(rfTranslate('diag.spi_checks'))}</span><b>${checks}</b></div><div class="row"><span>${esc(rfTranslate('diag.spi_failures'))}</span><b>${failures}</b></div><div class="row"><span>${esc(rfTranslate('diag.spi_consecutive'))}</span><b>${Number(r.spi_consecutive_failures||0)}</b></div><div class="row"><span>${esc(rfTranslate('diag.spi_recovery_attempts'))}</span><b>${Number(r.spi_recovery_attempts||0)}</b></div><div class="row"><span>${esc(rfTranslate('diag.spi_recovery_successes'))}</span><b>${Number(r.spi_recovery_successes||0)}</b></div><div class="row"><span>${esc(rfTranslate('diag.noise_frames'))}</span><b>${Number(r.noise_frames||0)}</b></div>${r.lqi!=null?`<div class="row"><span>LQI</span><b>${Number(r.lqi)}</b></div>`:''}${r.frequency_offset_hz!=null?`<div class="row"><span>${esc(rfTranslate('diag.freq_offset'))}</span><b>${Number(r.frequency_offset_hz).toFixed(0)} Hz</b></div>`:''}<div class="row"><span>${esc(rfTranslate('diag.rx_path'))}</span><b>${esc(r.rx_ready?rfTranslate('diag.ready'):rfTranslate('diag.failed'))}</b></div><div class="row"><span>${esc(rfTranslate('diag.tx_path'))}</span><b>${esc(r.tx_ready?rfTranslate('diag.ready'):rfTranslate('diag.failed'))}</b></div><div class="row"><span>${esc(rfTranslate('diag.configured_pins'))}</span><b class="mono">${esc(wiring)}</b></div></div><p class="muted small">${esc(rfTranslate('diag.spi_health_note'))}</p>${!r.available?`<p class="notice small">${esc(rfTranslate('diag.cc1101_hint'))}</p>`:''}</div>`;
 }).join('');
}
let lastBootSession=null,lastBootFrames=null,lastDiagnosticStatus=null;
const observedEvents=[];
const rebootLabel=(reason)=>({power_on:'Power-on',external_reset:'EN / RESET',software:'Software',panic:'Crash / panic',interrupt_watchdog:'Interrupt watchdog',task_watchdog:'Task watchdog',watchdog:'Watchdog',deep_sleep:'Deep sleep',brownout:'Brownout (supply)',sdio:'SDIO',unknown:'Unknown'})[reason]||reason||'Unknown';
function addBootObservation(message){observedEvents.unshift(new Date().toLocaleString()+' — '+message);if(observedEvents.length>10)observedEvents.pop()}
function renderBootDiagnostics(d){
 const box=$('#bootDiagnostics');if(!box)return;
 box.innerHTML=`<div class="row"><span>${esc(rfTranslate('diag.uptime'))}</span><b class="mono">${esc(uptime(d.uptime_ms))}</b></div>`+
 `<div class="row"><span>${esc(rfTranslate('diag.boot_count'))}</span><b>${Number(d.boot_count||0)}</b></div>`+
 `<div class="row"><span>${esc(rfTranslate('diag.reset_reason'))}</span><b>${esc(rebootLabel(d.reset_reason))}</b></div>`+
 `<div class="row"><span>${esc(rfTranslate('diag.heap_free'))}</span><b>${d.free_heap_bytes!=null?Math.round(d.free_heap_bytes/1024)+' KiB':'—'}</b></div>`+
 `<div class="row"><span>${esc(rfTranslate('diag.heap_min'))}</span><b>${d.min_free_heap_bytes!=null?Math.round(d.min_free_heap_bytes/1024)+' KiB':'—'}</b></div>`+
 `<div class="row muted small"><span>${esc(rfTranslate('diag.since_boot'))}</span></div>`;
 $('#bootLog').innerHTML=(d.boot_history||[]).map(x=>`<div class="row"><span>#${Number(x.boot_number)}</span><b>${esc(rebootLabel(x.reason))}</b></div>`).join('')||'—';
 if(!d.boot_history_persisted)$('#bootLog').innerHTML+='<div class="row muted small">'+esc(rfTranslate('diag.boot_unavailable'))+'</div>';
 $('#bootLiveEvents').textContent=observedEvents.length?observedEvents.join(' | '):rfTranslate('diag.no_events');
}
function checkBootChanges(d){
 if(lastBootSession!==null&&d.boot_session_id!==lastBootSession){addBootObservation(rfTranslate('diag.reboot_observed')+' (#'+(d.boot_count||'?')+', '+rebootLabel(d.reset_reason)+')')}
 else if(lastBootSession!==null&&lastBootFrames!==null&&d.total_frames<lastBootFrames){addBootObservation(rfTranslate('diag.counter_reset')+' ('+lastBootFrames+' → '+d.total_frames+')')}
 window.rfCurrentBootSession=d.boot_session_id||null;
 if(d.boot_session_id&&lastBootSession!==null&&d.boot_session_id!==lastBootSession){window.rfLogbookBootChanged?.(d.boot_session_id);}
 lastBootSession=d.boot_session_id;lastBootFrames=d.total_frames;
}
function updateCrashNotice(d){
 const notice=$('#crashStatusNotice');if(!notice)return;
 const recent=(Array.isArray(d.boot_history)?d.boot_history:[]).slice(0,8);
 const isCrash=r=>['panic','task_watchdog','interrupt_watchdog','watchdog','brownout'].includes(String(r||''));
 const failures=recent.filter(x=>isCrash(x.reason)).length;
 // Past boots have no timestamps: do not claim a rate or a crash loop in minutes.
 const show=failures>0 && (isCrash(d.reset_reason)||recent.slice(0,3).filter(x=>isCrash(x.reason)).length>=2);
 notice.hidden=!show;
 if(show){
  $('#crashStatusTitle').textContent=rfFormat('diag7.crashes',{n:failures});
  $('#crashStatusHelp').textContent=rfTranslate('diag7.crash_help');
 }
}
$('#crashStatusOpen').onclick=()=>{
 show('monitor');rfLogbookSetView('system');
 const filter=$('#logbookEventType');filter.value='boot';rfLogbookRender();
};
function rfDiag5Render(d){
 const text=(v,fallback='—')=>v===null||v===undefined?fallback:String(v);
 const safe=x=>systemTraceEscape(text(x));
 const radios=Array.isArray(d.channels)?d.channels:[];
 const previous=Array.isArray(d.boot_history)?d.boot_history:[];
 const crashes=previous.filter(h=>/panic|watchdog|brownout/i.test(String(h.reason||'')));
 const latest=previous.length?previous[0]:null;
 const rr=latest?rebootLabel(latest.reason):text(d.reset_reason);
 const labels={boot:document.getElementById('diagFiveBootSummary'),radio:document.getElementById('diagFiveRadioSummary'),memory:document.getElementById('diagFiveMemorySummary')};
 labels.boot.textContent=(crashes.length?crashes.length+' '+rfTranslate('diag55.crashes')+' · ':'')+rr;
 labels.radio.textContent=radios.filter(r=>r.available&&r.rx_ready).length+'/'+radios.length+' '+rfTranslate('diag55.ready');
 labels.memory.textContent=rfTranslate('diag55.internal')+': '+(Number.isFinite(Number(d.internal_free_bytes))?Math.round(d.internal_free_bytes/1024)+' KiB':'—');
 const bootHtml=previous.slice(0,8).map(h=>'<div class="row"><span>#'+Number(h.boot_number)+'</span><b>'+safe(rebootLabel(h.reason))+'</b></div>').join('');
 const traces=(d.crash_traces||[]).slice(0,4).map(t=>'<details class="diag-five-details"><summary>'+safe(rfTranslate('diag55.trace'))+' #'+Number(t.boot_number||0)+'</summary><pre>'+safe(JSON.stringify(t,null,2))+'</pre></details>').join('');
 document.getElementById('diagFiveBootDetails').innerHTML=(bootHtml||'<p class="muted">'+safe(rfTranslate('diag55.nohistory'))+'</p>')+traces;
 document.getElementById('diagFiveRadioDetails').innerHTML=radios.map(r=>'<div class="row"><span>'+safe(r.name||r.frequency_label||r.frequency_hz)+'</span><b>'+safe(r.available?rfTranslate('diag55.online'):rfTranslate('diag55.offline'))+' · SPI '+Number(r.spi_failures||0)+' '+safe(rfTranslate('diag55.errors'))+' · '+Number(r.spi_recovery_attempts||0)+' '+safe(rfTranslate('diag55.restores'))+'</b></div>').join('')||'<p class="muted">'+safe(rfTranslate('diag55.no_radio'))+'</p>';
 document.getElementById('diagFiveMemoryDetails').innerHTML=[['Internal free',d.internal_free_bytes],['Largest internal block',d.largest_internal_block_bytes],['PSRAM free',d.psram_free_bytes],['Heap free',d.free_heap_bytes]].map(([k,v])=>'<div class="row"><span>'+safe(k)+'</span><b>'+(Number.isFinite(Number(v))?Math.round(Number(v)/1024)+' KiB':'—')+'</b></div>').join('');
}

window.rfDiag5Render=rfDiag5Render;
function downloadBootDiagnostics(){if(!lastDiagnosticStatus)return;const d=lastDiagnosticStatus;const lines=[
 'RFLink Universal boot diagnostics',new Date().toISOString(),'Firmware '+(d.firmware_version||'unknown'),'Revision '+(d.firmware_revision||'unknown'),'Built '+(d.firmware_built||'unknown'),'Boot #'+(d.boot_count||'?'),'Uptime '+uptime(d.uptime_ms),
 'Reset reason '+rebootLabel(d.reset_reason)+' ('+d.reset_reason_code+')','NVS saved '+String(d.boot_history_persisted),
 'Received '+d.total_frames+' | decoded '+d.decoded_frames+' | unknown '+d.unknown_frames+' | noise '+d.noise_frames,
 'Free RAM '+d.free_heap_bytes+' | minimum '+d.min_free_heap_bytes,'',
 'Recent boots (no timestamps):',...(d.boot_history||[]).map(x=>'#'+x.boot_number+' '+rebootLabel(x.reason)+' ('+x.reason_code+')'),
 '', 'Events observed in this browser session:',...observedEvents];
 const blob=new Blob([lines.join('\n')+'\n'],{type:'text/plain;charset=utf-8'});const url=URL.createObjectURL(blob);
 const link=document.createElement('a');link.href=url;link.download='rflink-boot-diagnostics.txt';document.body.appendChild(link);link.click();link.remove();setTimeout(()=>URL.revokeObjectURL(url),1000);
}
$('#bootLogExport').addEventListener('click',downloadBootDiagnostics);
let statusLoadBusy=false;
async function loadStatus(){if(statusLoadBusy)return;statusLoadBusy=true;try{
 const d=await get('/rflink/api/status');latestStatus=d;lastDiagnosticStatus=d;window.rfDiagnosticStatus={firmware_version:d.firmware_version,firmware_revision:d.firmware_revision,firmware_built:d.firmware_built,boot_count:d.boot_count,boot_session_id:d.boot_session_id,boot_history:d.boot_history,boot_history_persisted:d.boot_history_persisted,crash_traces:d.crash_traces||[],crash_trace_persisted:d.crash_trace_persisted,reset_reason:d.reset_reason,free_heap_bytes:d.free_heap_bytes,internal_free_bytes:d.internal_free_bytes,largest_internal_block_bytes:d.largest_internal_block_bytes,psram_free_bytes:d.psram_free_bytes,channels:(d.channels||[]).map(r=>({name:r.name,frequency_hz:r.frequency_hz,available:r.available,rx_ready:r.rx_ready,tx_ready:r.tx_ready,spi_failures:r.spi_failures,spi_recovery_attempts:r.spi_recovery_attempts}))};checkBootChanges(d);renderBootDiagnostics(d);updateCrashNotice(d);rfDiag5Render(d);
 // Values come from the running ESP32 image, not the browser's embedded UI bundle.
 const revision=String(d.firmware_revision||'');
 // Always use the release identity actually reported by the ESP32, never guess from UI age.
 const releaseVersion=String(d.firmware_version||'');
 const releaseName=String(d.firmware_name||'');
 const thisRelease=Boolean(d.firmware_build_hash && /^v\d+\.\d+(?:\.\d+)?$/.test(releaseVersion));
 $('#firmwareVersion').textContent=thisRelease?(releaseVersion+(releaseName?' '+releaseName:'')):(releaseVersion||'—');
 $('#firmwareSubtitle').textContent=thisRelease?rfTranslate('version.subtitle'):rfTranslate('version.legacy');
 $('#firmwareRawVersion').textContent=releaseVersion||'—';
 $('#firmwareRevision').textContent=d.firmware_build_hash||revision||'—';
 const noteKeys=thisRelease?['version.note5','version.note6','version.note7','version.note8','version.note9']:['version.legacy_note'];
 $('#firmwareNotes').innerHTML=noteKeys.map(k=>'<li>'+esc(rfTranslate(k))+'</li>').join('');
 $('#firmwareBuilt').textContent=d.firmware_built||'—';
 $('#onlineTag').textContent=rfTranslate('static.001');$('#onlineTag').classList.add('good');$('#frameTag').textContent=rfFormat('dyn.frames',{n:d.total_frames});$('#uptimeTag').textContent=rfFormat('dyn.uptime',{value:uptime(d.uptime_ms)});
 $('#diagTotal').textContent=d.total_frames;$('#diagDecoded').textContent=d.decoded_frames;$('#diagUnknown').textContent=d.unknown_frames;$('#diagNoise').textContent=Number(d.noise_frames||0);
 window.rfLogbookRenderBootHistory?.();renderHealth(d);if(!$('#crashStatusNotice').hidden&&$('#healthCard').classList.contains('ok')){$('#healthCard').classList.remove('ok');$('#healthTitle').textContent=rfTranslate('diag7.health_attention');$('#healthExplanation').textContent=rfTranslate('diag7.health_detail');}renderRadioDiagnostics(d.channels||[]);
 $('#radioCards').innerHTML=(d.channels||[]).map(r=>`<button class="rf60-radio-row" type="button" data-monitor-go="${Number(r.index)}" ${r.available&&r.checked?'':'disabled'}><span class="rf60-dot ${r.available&&r.rx_ready?'is-ready':'is-off'}" aria-hidden="true"></span><strong>${esc(r.frequency_label||r.name||'—')}</strong><span class="muted small">${esc(r.available&&r.rx_ready?rfTranslate('diag55.online'):rfTranslate('diag55.offline'))}</span><span class="rf60-radio-arrow" aria-hidden="true">›</span></button>`).join('');
 $$('[data-monitor-go]').forEach(b=>b.onclick=()=>{if(b.disabled)return;monitorChannel=Number(b.dataset.monitorGo);show('monitor');rfLogbookSetView('radio')});
 $('#activity').innerHTML=(d.activity||[]).length?d.activity.slice(0,4).map(a=>`<div class="row"><span>${esc(a.channel)} · ${esc(a.summary||rfTranslate('ui.signal_seen'))}</span><span class="muted small">${age(a.age_ms)}</span></div>`).join(''):'<div class="row muted"><span>'+esc(rfTranslate('dyn.no_activity'))+'</span></div>';
 const rf60Radios=d.channels||[],rf60Missing=rf60Radios.filter(r=>!r.checked||!r.available||!r.rx_ready).length;
 const rf60History=Array.isArray(d.boot_history)?d.boot_history:[];
 const rf60Crashes=rf60History.filter(x=>/panic|watchdog|brownout/i.test(String(x.reason||''))).length;
 const rf60Attention=(rf60Missing>0?1:0)+(rf60Crashes>0?1:0)+(d.storage_ready===false||d.storage_ok===false?1:0)+(Number(d.library_attention_count||0)>0?1:0);
 $('#rf60System').textContent=rf60Missing||d.storage_ready===false||d.storage_ok===false?rfTranslate('rf60.needs_attention'):rfTranslate('dyn.online');
 $('#rf60Firmware').textContent=[d.firmware_version,d.firmware_name].filter(Boolean).join(' ');
 $('#rf60Radios').textContent=(rf60Radios.length-rf60Missing)+' / '+rf60Radios.length;
 $('#rf60Devices').textContent=Number.isFinite(Number(d.saved_devices))?String(d.saved_devices):'—';
 $('#rf60Attention').textContent=String(rf60Attention);
 $('#rf60AttentionDetail').textContent=rf60Crashes?rf60Crashes+' '+rfTranslate('diag55.crashes'):rf60Attention?rfTranslate('rf60.needs_attention'):rfTranslate('rf60.no_problems');
 $('#rf60NoProblems').hidden=!$('#crashStatusNotice').hidden;$('#rf60NoProblems').textContent=rf60Attention?(rf60Crashes?rfFormat('rf60.history_issues',{n:rf60Crashes}):rfTranslate('rf60.needs_attention')):rfTranslate('rf60.no_problems');$('#rf60AttentionOpen').hidden=!rf60Attention||!$('#crashStatusNotice').hidden;
 if(d.led){renderLedStatus(d.led);if(!ledDirty){$('#ledBrightness').value=d.led.brightness;$('#ledBrightnessValue').textContent=d.led.brightness+'%';$('#ledHeartbeat').checked=!d.led.quiet}}
 if(page==='learn')renderLearnRadios(d.channels);if(page==='monitor')renderMonitor(d);
}catch(e){$('#onlineTag').textContent=rfTranslate('dyn.offline');$('#onlineTag').classList.remove('good');$('#healthCard').className='card health error';$('#healthTitle').textContent=rfTranslate('dyn.esp_unreachable');$('#healthExplanation').textContent=rfTranslate('dyn.esp_unreachable_help');if(page==='learn'){$('#baselineBtn').disabled=true;$('#listenBtn').disabled=true;$('#learnRadioHelp').textContent=rfTranslate('dyn.api_unreachable')}if(page==='monitor')$('#monitorActivity').innerHTML='<div class="card empty">'+esc(rfTranslate('dyn.connection_lost'))+'</div>'}finally{statusLoadBusy=false}}

function deviceSignalsHtml(x,candidates){
 const ids=Array.isArray(x.rf_identities)?x.rf_identities:[], options=(candidates||[]).filter(c=>c.ready&&c.protocol===x.protocol&&c.device_type===x.device_type&&!ids.includes(c.identity));
 return `<details class="signal-panel" data-device-signals="${esc(x.uid)}" ${expandedSignals.has(x.uid)?'open':''}><summary>${esc(rfFormat('dyn.manage_signals',{n:ids.length}))}</summary>
 <p class="muted small">${esc(rfFormat('dyn.signal_identity_help',{uid:x.uid}))}</p>
 ${ids.map((id,i)=>`<div class="signal-entry"><div><b>${esc(rfFormat('dyn.rf_signal',{n:i+1}))}</b><div class="mono">${esc(id)}</div></div>
 <div class="signal-controls"><label class="small" for="rf-replacement-${esc(x.uid)}-${i}">${esc(rfTranslate('dyn.replace_with'))}</label><select class="field" id="rf-replacement-${esc(x.uid)}-${i}" data-replace-select="${esc(x.uid)}" data-identity="${esc(id)}"><option value="">${esc(rfTranslate('dyn.choose_learned'))}</option>${options.map(c=>`<option value="${esc(c.identity)}" ${replacementSelections.get(x.uid+'\u0000'+id)===c.identity?'selected':''}>${esc(c.identity)} (${c.observations}×)</option>`).join('')}</select><button class="btn out small" data-replace-id="${esc(x.uid)}" data-identity="${esc(id)}" ${options.length?'':'disabled'}>${esc(rfTranslate('dyn.replace'))}</button><button class="btn out small" data-unlink-id="${esc(x.uid)}" data-identity="${esc(id)}" ${ids.length>1?'':'disabled title="'+esc(rfTranslate('dyn.last_code_title'))+'"'}>${esc(rfTranslate('dyn.unlink'))}</button></div>
 </div>`).join('')}
 ${options.length?'':`<div class="notice small">${esc(rfTranslate('dyn.no_matching_signal'))}</div>`}
 <button class="btn out small" data-learn-replacement="${esc(x.uid)}">${esc(rfTranslate('dyn.learn_new_signal'))}</button>
 <p class="muted small">${esc(rfTranslate('dyn.signal_history_help'))}</p></details>`;
}
// Device profiles drive the visible controls; the TX backend alone decides if a click is allowed.
function profilesFor(x){return (window.rfUserLibraryCache?.profiles||[]).filter(p=>p.protocol===x.protocol)}
function officialSuggestion(x,p){return (x.profile_suggestions||[]).find(s=>s.id===p.id)}
function suggestedProfile(x){
 const ps=profilesFor(x),ranked=ps.filter(p=>officialSuggestion(x,p));
 // Only preselect when there is one candidate with non-manual compatibility.
 return ranked.length===1&&ranked[0].compatibility!=='manual_only' ? ranked[0].id : '';
}
function profileChoices(x,selected=''){
 const ps=profilesFor(x),current=selected||x.profile_id||'';
 const suggested=p=>!!officialSuggestion(x,p);
 const options=items=>items.sort((a,b)=>Number(suggested(b))-Number(suggested(a))||a.name.localeCompare(b.name)).map(p=>{
   const mark=suggested(p)?' · '+rfTranslate('profileflow.suggested'):'';
   return '<option value="'+esc(p.id)+'" '+(p.id===current?'selected':'')+'>'+esc(p.name+mark)+'</option>';
 }).join('');
 const manufacturers=[...new Set(ps.map(p=>p.manufacturer||p.source||'Other'))].sort();
 return '<option value="">'+esc(rfTranslate('profileflow.none'))+'</option>'+manufacturers.map(brand=>
    '<optgroup label="'+esc(brand)+'">'+options(ps.filter(p=>(p.manufacturer||p.source||'Other')===brand))+'</optgroup>').join('');
}
function profileAdvice(x,selected){
 const p=profilesFor(x).find(p=>p.id===selected),possible=p&&officialSuggestion(x,p);
 if(!p) return rfTranslate('profileflow.select_help');
 if(p.compatibility==='manual_only')return rfTranslate('profileflow.manual_warning');
 if(possible)return rfTranslate('profileflow.proposal_warning');
 return rfTranslate('profileflow.user_choice');
}
function controlKeys(p){
 if(!p)return [];
 if(Array.isArray(p.controls))return p.controls;
 // User profiles remain usable: only commands declared by a decoder can be enabled.
 const proto=getProtocol(p.protocol),whitelist=keys(proto?.tx_actions);
 return (p.expected_actions||[]).filter(a=>whitelist.includes(a));
}
function displayCommand(p,key){
 const trans=rfTranslate('profileflow.command.'+key);
 return trans!=='profileflow.command.'+key?trans:actionLabel(getProtocol(p.protocol),key);
}
function profilePreviewHtml(x,p){
 if(!p)return '';
 const actions=controlKeys(p);
 return actions.length?rfTranslate('profileflow.after_profile')+': '+actions.map(a=>displayCommand(p,a)).join(' · '):rfTranslate('profileflow.sensor_profile');
}
function setCandidateProfilePicker(prefix,x){
 const select=$('#'+prefix+'Profile');if(!select)return;
 select.innerHTML=profileChoices(x,suggestedProfile(x));
 // A profile can be recommended from protocol evidence, not verified as a model.
 const update=()=>{
  const p=profilesFor(x).find(p=>p.id===select.value);
  $('#'+prefix+'ProfileAdvice').textContent=profileAdvice(x,select.value);
  $('#'+prefix+'ProfilePreview').textContent=profilePreviewHtml(x,p);
 };
 select.onchange=update;update();
}
function profilePickerHtml(x){
 const current=profilesFor(x).find(p=>p.id===x.profile_id);
 const label=current?rfTranslate('ux3.change_profile'):rfTranslate('ux3.select_profile');
 const field='<label for="device-profile-'+esc(x.uid)+'">'+esc(rfTranslate('profileflow.choose'))+'</label><select class="field" id="device-profile-'+esc(x.uid)+'" data-device-profile="'+esc(x.uid)+'">'+profileChoices(x)+'</select><p class="muted small">'+esc(profileAdvice(x,x.profile_id))+'</p>';
 if(!current)return '<div class="profile-picker" data-profile-panel="'+esc(x.uid)+'">'+field+'</div>';
 return '<details class="profile-picker" data-profile-panel="'+esc(x.uid)+'"><summary>'+esc(label)+'</summary>'+field+'</details>';
}
// Universal profile-driven onboarding: no profile is allowed to write RF directly.
// All TX requests go through the pre-existing reviewed Somfy/Itho endpoints.
const pairWizardState={device:null,profile:null,step:0,sent:false,ack:false,verified:false,busy:false};
const pairTxt=(key)=>rfTranslate('wizard.'+key);
function pairProfile(d){return profilesFor(d).find(p=>p.id===d.profile_id)||null}
function pairSteps(p){
 const localized=p?.setup_steps_i18n?.[rfLanguage]||p?.setup_steps_i18n?.en||[];
 const instructions=Array.isArray(localized)&&localized.length?localized:
   (p?.learn_help_i18n?.[rfLanguage]||p?.learn_help||pairTxt('observe_help')).split(/\n+/).filter(Boolean);
 return [pairTxt('identity_help'), ...instructions.slice(0,8), pairTxt('finish_help')];
}
function devicePairStatus(d){
 const p=pairProfile(d);if(!p)return pairTxt('choose_profile');
 const mode=p.pair_method||'none';
 if(mode==='none')return pairTxt('rx_ready');
 if(d.protocol==='RamsesII')return d.ramses?.pair_stage==='paired'?pairTxt('sent_not_verified'):rfTranslate('orcon.experimental_status');
 if(d.protocol==='IthoRFT'&&d.itho?.paired || d.protocol==='SomfyRTS56'&&d.somfy?.paired)
   return pairTxt('sent_not_verified');
 if(mode==='unsupported')return pairTxt('not_supported');
 return pairTxt('requires_setup');
}
function pairButtonHtml(d){
 const p=pairProfile(d);
 if(!p)return '<div class="device-pair-panel"><strong>'+esc(pairTxt('setup_title'))+'</strong><span class="muted small">'+esc(pairTxt('choose_profile'))+'</span><button class="btn out small" type="button" data-profile-open="'+esc(d.uid)+'">'+esc(rfTranslate('ux3.select_profile'))+'</button></div>';
 return '<div class="device-pair-panel"><strong>'+esc(pairTxt('setup_title'))+'</strong><span class="muted small">'+esc(devicePairStatus(d))+'</span><button class="btn out small" type="button" data-pair-open="'+esc(d.uid)+'">'+esc(pairTxt('start'))+'</button></div>';
}
// The API body can be JSON (e.g. {ok:false,error:"..."}). Present only the
// approved error string; never log raw request bodies, URL parameters or keys.
function pairErrorText(err){
 const raw=String(err?.message||err||'Unknown pairing error');
 try{const data=JSON.parse(raw);if(typeof data.error==='string')return data.error.slice(0,155)}catch(_){}
 return raw.slice(0,155);
}
function recordPairResult(stage,err,uid){
 const message=pairErrorText(err);
 rfWebPush('pair',stage+': '+message,'error',uid);
 return message;
}
function pairModal(){
 let modal=document.querySelector('#universalPairModal');
 if(modal)return modal;
 modal=document.createElement('dialog');modal.id='universalPairModal';modal.className='pair-dialog';
 modal.setAttribute('aria-label',pairTxt('setup_title'));document.body.appendChild(modal);
 modal.addEventListener('close',()=>{pairWizardState.device=null;pairWizardState.profile=null;});
 return modal;
}
function pairClose(){const d=pairModal();if(d.open)d.close()}
async function pairShow(uid){
 const d=(lastDeviceView?.devices||[]).find(x=>x.uid===uid);if(!d)return;
 let p=pairProfile(d);if(!p){toast(pairTxt('choose_profile'));return}
 p=await loadProfileDetail(p.id)
 const ongoing=d.protocol==='RamsesII'&&['queued','waiting_ack','confirming','awaiting_user'].includes(d.ramses?.pair_stage);
 Object.assign(pairWizardState,{device:d,profile:p,step:ongoing?pairSteps(p).length-1:0,sent:ongoing,ack:false,verified:false,busy:false});
 const dialog=pairModal();pairRender();if(!dialog.open)dialog.showModal();
}
function pairRender(){
 const st=pairWizardState,d=st.device,p=st.profile;if(!d||!p)return;
 const steps=pairSteps(p),last=steps.length-1,idx=Math.min(st.step,last),mode=p.pair_method||'none';
 const txAction=mode==='virtual_remote'&&d.protocol==='SomfyRTS56'?'somfy_pair':
   mode==='receiver_learn'&&d.protocol==='IthoRFT'?'itho_pair':
   mode==='virtual_remote'&&d.protocol==='RamsesII'?'ramses_pair':null;
 const needsTx=mode==='virtual_remote'||mode==='receiver_learn'||!!p.pair_tx_required;
 const unavailable=mode==='unsupported'||(needsTx&&(!txAction||!lastDeviceView?.tx_controls_enabled))||
  (d.protocol==='RamsesII'&&!d.ramses?.lab_enabled);
 let info='';
 if(unavailable)info='<div class="pair-warning">'+esc(d.protocol==='RamsesII'?rfTranslate('orcon.enable_lab'):pairTxt('tx_unavailable'))+'</div>';
 if(d.protocol==='RamsesII'&&st.sent)info+='<div class="pair-warning">'+esc(rfTranslate('orcon.status_'+(d.ramses?.pair_stage||'waiting_ack')))+'</div>';
 if(idx===last&&mode!=='none'&&st.sent&&!st.verified)
   info+='<div class="pair-warning">'+esc(pairTxt('sent_not_verified'))+'</div>';
 const progress=Math.round(((idx+1)/steps.length)*100);
 let actionButton='';
 if(idx===last-1 && txAction&&!unavailable&&!st.sent)
   actionButton='<button class="btn green" data-pair-action="'+txAction+'">'+esc(pairTxt('transmit'))+'</button>';
 if(idx===last-1&&st.sent)actionButton='<button class="btn out" data-pair-next="1">'+esc(pairTxt('check'))+'</button>';
 const atEnd=idx===last;
 const complete=mode==='none'||unavailable||(st.sent&&st.verified)||mode==='manual';
 const endButton=atEnd&&complete?'<button class="btn green" data-pair-end="1">'+esc(unavailable?rfTranslate('ux3.finish_rx_only'):pairTxt('finish'))+'</button>':'';
 const verify=atEnd&&st.sent&&!st.verified&&(d.protocol!=='RamsesII'||(d.ramses?.pair_stage==='awaiting_user'&&d.ramses?.test_sent))?'<button class="btn green" data-pair-verify="1">'+esc(pairTxt('verify_observed'))+'</button>':'';
 pairModal().innerHTML='<h3>'+esc(p.name)+'</h3><p class="muted small">'+esc(d.name+' · '+d.protocol+' · '+pairTxt('step')+' '+(idx+1)+' / '+steps.length)+'</p><div class="pair-progress"><span style="width:'+progress+'%"></span></div><p>'+esc(steps[idx])+'</p>'+info+
  '<div class="pair-row"><button class="btn out" data-pair-cancel="1">'+esc(pairTxt('close'))+'</button>'+
  (idx?'<button class="btn out" data-pair-back="1">'+esc(pairTxt('back'))+'</button>':'')+
  (idx<last&&idx!==last-1?'<button class="btn green" data-pair-next="1">'+esc(pairTxt('next'))+'</button>':'')+
  (idx===last-1&&!actionButton?'<button class="btn out" data-pair-next="1">'+esc(pairTxt(unavailable?'rx_only':'next'))+'</button>':'')+
  actionButton+(d.protocol==='RamsesII'&&d.ramses?.pair_stage==='awaiting_user'&&!d.ramses?.test_sent?'<button class="btn green" data-pair-test="1">'+esc(rfTranslate('orcon.test_auto'))+'</button>':'')+(d.protocol==='RamsesII'&&st.sent&&!st.verified?'<button class="btn out" data-pair-check="1">'+esc(pairTxt('check'))+'</button>':'')+verify+endButton+'</div>';
}
async function pairWizardAction(action){
 const st=pairWizardState;if(!st.device||st.busy)return;
 if(action==='close'){pairClose();return}
 if(action==='back'){st.step=Math.max(0,st.step-1);pairRender();return}
 if(action==='next'){st.step=Math.min(pairSteps(st.profile).length-1,st.step+1);pairRender();return}
 if(action==='end'){pairClose();toast(pairTxt('rx_ready'));return}
 if(action==='test'&&st.device.protocol==='RamsesII'){
  if(!confirm(rfTranslate('orcon.confirm_test_auto')))return;
  st.busy=true;
  try{await post('/rflink/api/device?action=ramses_pair_test&uid='+encodeURIComponent(st.device.uid)+'&confirm=TEST_AUTO_ONE_SHOT');
      const data=await get('/rflink/api/devices');st.device=(data.devices||[]).find(x=>x.uid===st.device.uid)||st.device;
      toast(rfTranslate('pair55.auto_queued'));}
  catch(e){toast(recordPairResult('Auto-test mislukt',e,st.device.uid));}
  finally{st.busy=false;pairRender()}return;
 }
 if(action==='check'&&st.device.protocol==='RamsesII'){
  st.busy=true;
  try {const data=await get('/rflink/api/devices');const latest=(data.devices||[]).find(x=>x.uid===st.device.uid);if(latest)st.device=latest;}
  catch(e){toast(recordPairResult('Koppelstatus ophalen mislukt',e,st.device.uid))}
  finally{st.busy=false;pairRender()}
  return;
 }
 if(action==='verify'){
  if(!confirm(pairTxt('verify_question')))return;
  if(st.device.protocol==='RamsesII'){
    if(st.device.ramses?.pair_stage!=='awaiting_user'){toast(rfTranslate('orcon.wait_ack'));return}
    st.busy=true;
    try{await post('/rflink/api/device?action=ramses_pair_verify&uid='+encodeURIComponent(st.device.uid)+'&confirm=DEVICE_REACTION_CONFIRMED');
      st.verified=true;await loadDevices();}
    catch(e){toast(recordPairResult('Koppeling bevestigen mislukt',e,st.device.uid));}
    finally{st.busy=false;pairRender()}
    return;
  }
  st.verified=true;pairRender();return;
 }
 if(action==='somfy_pair'||action==='itho_pair'||action==='ramses_pair'){
  if(!confirm(pairTxt('transmit_confirm')))return;
  st.busy=true;
  try{
   const query='/rflink/api/device?action='+action+'&uid='+encodeURIComponent(st.device.uid)+(action==='itho_pair'?'&mode=standard':action==='ramses_pair'?'&confirm=START_1FC9_EXPERIMENT':'');
   await post(query);st.sent=true;st.step=pairSteps(st.profile).length-1;
   rfWebPush('pair',action==='ramses_pair'?'1FC9 koppelverzoek ingepland in TX-router; RF-uitzending nog niet bevestigd':action+' koppelverzoek geaccepteerd','info',st.device.uid);
   if(action==='ramses_pair'){const data=await get('/rflink/api/devices');st.device=(data.devices||[]).find(x=>x.uid===st.device.uid)||st.device;}
   toast(action==='ramses_pair'?rfTranslate('pair55.queued'):pairTxt('sent_not_verified'));
  }catch(e){toast(recordPairResult('Koppelverzoek mislukt',e,st.device.uid));}
  finally{st.busy=false;pairRender()}
 }
}
document.addEventListener('click',e=>{
 const profile=e.target.closest('[data-profile-open]');if(profile){
  const panel=[...document.querySelectorAll('[data-profile-panel]')].find(p=>p.dataset.profilePanel===profile.dataset.profileOpen);
  if(panel){panel.open=true;panel.querySelector('select')?.focus();}return;
 }
 const launch=e.target.closest('[data-pair-open]');if(launch){pairShow(launch.dataset.pairOpen);return}
 const cmd=e.target.closest('[data-pair-action],[data-pair-next],[data-pair-back],[data-pair-cancel],[data-pair-end],[data-pair-verify],[data-pair-check],[data-pair-test]');
 if(!cmd)return;
 pairWizardAction(cmd.dataset.pairAction||cmd.hasAttribute('data-pair-next')&&'next'||cmd.hasAttribute('data-pair-back')&&'back'||cmd.hasAttribute('data-pair-cancel')&&'close'||cmd.hasAttribute('data-pair-end')&&'end'||cmd.hasAttribute('data-pair-verify')&&'verify'||cmd.hasAttribute('data-pair-check')&&'check'||cmd.hasAttribute('data-pair-test')&&'test');
});

function profileSetupHtml(x){
 const p=profilesFor(x).find(p=>p.id===x.profile_id), steps=p?.setup_steps_i18n?.[rfLanguage]||p?.setup_steps_i18n?.en||[];
 if(!steps.length)return '';
 return '<details class="decoder-tech profile-setup"><summary>'+esc(rfTranslate('profileflow.setup'))+'</summary><ol>'+steps.map(step=>'<li>'+esc(step)+'</li>').join('')+'</ol><p class="muted small">'+esc(rfTranslate('profileflow.setup_rx_only'))+'</p></details>';
}
function deviceOperationsHtml(x,view,compact=false){
 const p=profilesFor(x).find(p=>p.id===x.profile_id),commands=controlKeys(p),sensor=getProtocol(x.protocol)?.learn_category==='sensor'||['co2_sensor','humidity_sensor','motion_sensor'].includes(p?.device_type);
 if(!p)return '<div class="device-operation notice small">'+esc(rfTranslate('profileflow.choose_first'))+'</div>';
 if(sensor)return '<div class="device-operation"><h4>'+esc(rfTranslate('profileflow.measurements'))+'</h4><p class="muted small">'+esc(rfTranslate('profileflow.sensor_info'))+'</p></div>';
 if(!commands.length)return '<div class="device-operation"><h4>'+esc(rfTranslate('profileflow.operation'))+'</h4><p class="muted small">'+esc(rfTranslate('profileflow.no_commands'))+'</p></div>';
 const paired=x.protocol==='SomfyRTS56'?!!x.somfy?.paired:x.protocol==='IthoRFT'?!!x.itho?.paired:x.protocol==='RamsesII'?x.ramses?.pair_stage==='paired':true;
 const isUser=(window.rfUserLibraryCache?.decoders||[]).some(d=>d.id===x.protocol&&d.tx_source&&d.enabled);
 const nativeSupported=!!view.tx_controls_enabled;
 const nativeReady=nativeSupported&&paired&&(x.protocol!=='RamsesII'||x.ramses?.lab_enabled);
 const userReady=isUser&&!!x.local_tx_enabled;
 const ready=commands.some(a=>(nativeReady&&(x.tx_ready_actions||[]).includes(a)) || (userReady&&(x.actions||[]).includes(a)));
 const reason=!ready?(x.protocol==='RamsesII'?rfTranslate('ux3.rx_only_short'):
    !paired?rfTranslate('profileflow.pair_first'):rfTranslate('ux3.rx_only_short')):rfTranslate('profileflow.tx_ready');
 const buttons=commands.filter(a=>!compact||(nativeReady&&(x.tx_ready_actions||[]).includes(a))||(userReady&&(x.actions||[]).includes(a))).map(a=>{
   const canNative=nativeReady&&(x.tx_ready_actions||[]).includes(a);
   const canUser=userReady&&(x.actions||[]).includes(a);
   const canSend=canNative||canUser;
   return '<button class="btn '+(canSend?'out':'soft')+' small" data-profile-command="'+esc(x.uid)+'" data-command="'+esc(a)+'" data-transport="'+(canUser?'user':'native')+'" '+(!canSend?'disabled':'')+'>'+esc(displayCommand(p,a))+'</button>';
 }).join('');
 const pair=(x.protocol==='SomfyRTS56'&&!paired&&view.tx_controls_enabled)?'<button class="btn green small" data-somfy-pair="'+esc(x.uid)+'">'+esc(rfTranslate('somfy.pair'))+'</button>':
    (x.protocol==='IthoRFT'&&!paired&&view.tx_controls_enabled)?'<button class="btn green small" data-itho-pair="'+esc(x.uid)+'" data-mode="standard">'+esc(rfTranslate('itho.pair'))+'</button>':'';
 return '<div class="device-operation"><h4>'+esc(rfTranslate('profileflow.operation'))+'</h4>'+(ready?'<div class="device-command-list">'+buttons+'</div><p class="muted small">'+esc(reason)+'</p>':'<div class="device-unavailable">'+esc(rfTranslate('ux3.not_yet_available'))+'<p>'+esc(reason)+'</p></div>')+pair+'</div>';
}
function ordinaryDeviceAction(x,key){return !(x.protocol==='SomfyRTS56'&&key==='prog')}
function somfyControlsHtml(x,view){
 if(x.protocol!=='SomfyRTS56')return '';
 const enabled=!!view.tx_controls_enabled, configured=!!x.somfy?.configured, paired=!!x.somfy?.paired;
 if(!enabled)return `<div class="notice small"><b>${esc(rfTranslate('somfy.control'))}</b><br>${esc(rfTranslate('somfy.tx_disabled'))}</div>`;
 if(!paired)return `<div class="notice small somfy-control"><b>${esc(rfTranslate('somfy.control'))}</b><br>${esc(rfTranslate('somfy.pair_help'))}<div class="actions"><button class="btn green small" data-somfy-pair="${esc(x.uid)}">${esc(rfTranslate('somfy.pair'))}</button></div>${configured?`<details class="decoder-tech"><summary>${esc(rfTranslate('ui.advanced_details'))}</summary><p class="mono small">${esc(rfFormat('somfy.state',{address:Number(x.somfy.address).toString(16).toUpperCase().padStart(6,'0'),counter:x.somfy.next_rolling_code}))}</p></details>`:''}</div>`;
 return `<div class="notice small somfy-control"><b>${esc(rfTranslate('somfy.control_ready'))}</b><div class="actions"><button class="btn out small" data-native-command="up" data-native-uid="${esc(x.uid)}">${esc(rfTranslate('somfy.up'))}</button><button class="btn out small" data-native-command="my" data-native-uid="${esc(x.uid)}">${esc(rfTranslate('somfy.my'))}</button><button class="btn out small" data-native-command="down" data-native-uid="${esc(x.uid)}">${esc(rfTranslate('somfy.down'))}</button></div><details class="decoder-tech"><summary>${esc(rfTranslate('ui.advanced_details'))}</summary><p class="muted small">${esc(rfTranslate('somfy.no_ack'))}</p><p class="mono small">${esc(rfFormat('somfy.state',{address:Number(x.somfy.address).toString(16).toUpperCase().padStart(6,'0'),counter:x.somfy.next_rolling_code}))}</p><button class="btn out small" data-somfy-pair="${esc(x.uid)}">${esc(rfTranslate('somfy.pair_again'))}</button></details></div>`;
}
function ithoControlsHtml(x,view){
 if(x.protocol!=='IthoRFT')return '';
 const enabled=!!view.tx_controls_enabled, configured=!!x.itho?.configured, paired=!!x.itho?.paired;
 const state=configured?`<p class="mono small">${esc(rfFormat('itho.state',{address:Number(x.itho.address).toString(16).toUpperCase().padStart(6,'0'),sequence:x.itho.next_sequence,type:Number(x.itho.device_type).toString(16).toUpperCase().padStart(2,'0')}))}</p>`:'';
 if(!enabled)return `<div class="notice small"><b>${esc(rfTranslate('itho.control'))}</b><br>${esc(rfTranslate('itho.tx_disabled'))}</div>`;
 if(!paired)return `<div class="notice small itho-control"><b>${esc(rfTranslate('itho.control'))}</b><br>${esc(rfTranslate('itho.pair_help'))}<div class="actions"><button class="btn green small" data-itho-pair="${esc(x.uid)}" data-mode="standard">${esc(rfTranslate('itho.pair'))}</button></div><details class="decoder-tech"><summary>${esc(rfTranslate('ui.advanced_details'))}</summary>${state}<p class="muted small">${esc(rfTranslate('itho.compat_help'))}</p><button class="btn out small" data-itho-pair="${esc(x.uid)}" data-mode="compat">${esc(rfTranslate('itho.compat_pair'))}</button></details></div>`;
 return `<div class="notice small itho-control"><b>${esc(rfTranslate('itho.control_ready'))}</b><div class="actions"><button class="btn out small" data-native-command="low" data-native-uid="${esc(x.uid)}">${esc(rfTranslate('itho.low'))}</button><button class="btn out small" data-native-command="medium" data-native-uid="${esc(x.uid)}">${esc(rfTranslate('itho.medium'))}</button><button class="btn out small" data-native-command="high" data-native-uid="${esc(x.uid)}">${esc(rfTranslate('itho.high'))}</button></div><div class="actions"><button class="btn out small" data-native-command="timer_10" data-native-uid="${esc(x.uid)}">${esc(rfTranslate('itho.timer10'))}</button><button class="btn out small" data-native-command="timer_20" data-native-uid="${esc(x.uid)}">${esc(rfTranslate('itho.timer20'))}</button><button class="btn out small" data-native-command="timer_30" data-native-uid="${esc(x.uid)}">${esc(rfTranslate('itho.timer30'))}</button></div><details class="decoder-tech"><summary>${esc(rfTranslate('ui.advanced_details'))}</summary><p class="muted small">${esc(rfTranslate('itho.no_ack'))}</p>${state}<div class="actions"><button class="btn out small" data-native-command="standby" data-native-uid="${esc(x.uid)}">${esc(rfTranslate('itho.standby'))}</button><button class="btn out small" data-itho-pair="${esc(x.uid)}" data-mode="standard">${esc(rfTranslate('itho.pair_again'))}</button><button class="btn out small" data-itho-pair="${esc(x.uid)}" data-mode="compat">${esc(rfTranslate('itho.compat_pair'))}</button><button class="btn out small" data-itho-unpair="${esc(x.uid)}">${esc(rfTranslate('itho.unpair'))}</button></div></details></div>`;
}
function deviceCapabilitiesHtml(x){
 const p=getProtocol(x.protocol), sensor=p?.learn_category==='sensor';
 const actions=sensor?[]:(x.actions||[]).filter(key=>ordinaryDeviceAction(x,key)).map(key=>({kind:'action',key}));
 const fields=(x.fields||[]).filter(f=>haProfiles.has(f.key)).map(f=>({kind:'field',key:f.key}));
 const pendingActions=(x.pending_actions||[]).filter(key=>!sensor&&ordinaryDeviceAction(x,key)).map(key=>({kind:'action',key}));
 const pendingFields=(x.pending_fields||[]).filter(f=>haProfiles.has(f.key)).map(f=>({kind:'field',key:f.key}));
 const technical=(x.fields||[]).filter(f=>!haProfiles.has(f.key)&&!f.key.startsWith('tx_lock_'));
 const pendingTechnical=(x.pending_fields||[]).filter(f=>!haProfiles.has(f.key));
 const publicLabel=item=>item.kind==='action'?actionLabel(p,item.key):rfFieldLabel(item.key,haProfiles.get(item.key));
 const publicRow=(item,approve)=>`<div class="row"><span>${esc(publicLabel(item))}</span><button class="btn out small" data-feature="${approve?'approve':'remove'}_${item.kind}" data-feature-uid="${esc(x.uid)}" data-feature-key="${esc(item.key)}" ${!approve&&x.ha_enabled&&!lastDeviceView?.mqtt_connected?'disabled title="'+esc(rfTranslate('dyn.mqtt_remove_unavailable'))+'"':''}>${esc(rfTranslate(approve?'dyn.approve':'dyn.remove'))}</button></div>`;
 const technicalRow=f=>`<div class="row"><span class="mono small">${esc(f.key)}</span><button class="btn out small" data-feature="approve_field" data-feature-uid="${esc(x.uid)}" data-feature-key="${esc(f.key)}">${esc(rfTranslate('dyn.approve_technical'))}</button></div>`;
 const pendingCount=pendingActions.length+pendingFields.length+pendingTechnical.length;
 const open=expandedCapabilities.has(x.uid)||(!expandedCapabilities.has('closed:'+x.uid)&&pendingCount>0);
 return `<details class="signal-panel" data-device-capabilities="${esc(x.uid)}" ${open?'open':''}><summary>${esc(pendingCount?rfFormat('dyn.review_new_functions',{n:pendingCount}):rfTranslate('dyn.view_functions'))}${!pendingCount?' ('+(actions.length+fields.length)+')':''}</summary>
 <p class="muted small">${esc(rfTranslate('dyn.approved_only_help'))}</p>
 ${pendingActions.length||pendingFields.length?'<div class="notice warn small"><b>'+esc(rfTranslate('dyn.found_not_approved'))+'</b></div>'+[...pendingActions,...pendingFields].map(x=>publicRow(x,true)).join(''):'<p class="muted small">'+esc(rfTranslate('dyn.no_new_visible'))+'</p>'}
 <details class="device-approved-list"><summary>${esc(rfTranslate('dyn.approved_features'))} (${actions.length+fields.length})</summary>${actions.length+fields.length?[...actions,...fields].map(x=>publicRow(x,false)).join('') :'<p class="muted small">'+esc(rfTranslate('dyn.none_approved'))+'</p>'}</details>
 ${technical.length||pendingTechnical.length?`<details class="decoder-tech"><summary>${esc(rfFormat('dyn.technical_radio_data',{saved:technical.length,pending:pendingTechnical.length?rfFormat('dyn.pending_suffix',{n:pendingTechnical.length}):''}))}</summary><p class="muted small">${esc(rfTranslate('dyn.technical_radio_help'))}</p>${technical.map(f=>'<div class="row"><span class="small mono">'+esc(f.key)+'</span><button class="btn out small" data-feature="remove_field" data-feature-uid="'+esc(x.uid)+'" data-feature-key="'+esc(f.key)+'" '+(x.tx_enabled||x.ha_enabled&&!lastDeviceView?.mqtt_connected?'disabled title="'+esc(x.tx_enabled?rfTranslate('profileflow.remove_tx_locked'):rfTranslate('dyn.mqtt_remove_unavailable'))+'"':'')+'>'+esc(rfTranslate('dyn.remove'))+'</button></div>').join('')}${pendingTechnical.map(technicalRow).join('')}</details>`:''}
 </details>`;
}
function deviceChannelKey(x){return x.channel===0?'0':x.channel===1?'1':x.channel===2?'2':'unknown'}
function renderDeviceRadioChoices(devices){
 const counts={all:devices.length,'0':0,'1':0,'2':0,unknown:0};
 devices.forEach(x=>counts[deviceChannelKey(x)]++);
 const unknown=$('#deviceUnknownRadioChoice');unknown.hidden=counts.unknown===0&&deviceRadioChoice!=='unknown';
 $$('[data-dev-radio]').forEach(b=>{
  const key=b.dataset.devRadio,selected=key===deviceRadioChoice;
  b.classList.toggle('active',selected);b.setAttribute('aria-pressed',selected?'true':'false');
  const counter=b.querySelector('[data-radio-device-count]');
  if(counter)counter.textContent=rfFormat('devices.radio_count',{n:counts[key]});
 });
}
$('#deviceSearch').addEventListener('input',e=>{deviceSearchText=e.target.value;clearTimeout(window.rfSearchDebounce);window.rfSearchDebounce=setTimeout(()=>{if(lastDeviceView){/* repaint from cached data through render path */renderDeviceCache()}else loadDevices()},120)});
$$('[data-dev-radio]').forEach(b=>b.addEventListener('click',()=>{
 deviceRadioChoice=b.dataset.devRadio;
 $$('[data-dev-radio]').forEach(c=>{const active=c===b;c.classList.toggle('active',active);c.setAttribute('aria-pressed',active?'true':'false')});
 loadDevices();
}));
$('#deviceClearFilter').addEventListener('click',()=>{deviceSearchText='';deviceRadioChoice='all';$('#deviceSearch').value='';loadDevices()});
// Details are persistent UI state. The device list is rebuilt on each refresh;
// remember every opened nested panel per device rather than letting it close.
const openedDevicePanels=new Map(),expandedFeatureShell=new Set(),closedFeatureShell=new Set();
function rememberDevicePanels(){
 const list=document.querySelector('#deviceList');if(!list)return;
 list.querySelectorAll('.card.device[data-device-uid]').forEach(card=>{
  const open=[];card.querySelectorAll('details').forEach((panel,index)=>{if(panel.open)open.push(index)});
  openedDevicePanels.set(card.dataset.deviceUid,open);
 });
}
function restoreDevicePanels(){
 const list=document.querySelector('#deviceList');if(!list)return;
 list.querySelectorAll('.card.device[data-device-uid]').forEach(card=>{
  const saved=openedDevicePanels.get(card.dataset.deviceUid);
  if(!saved)return;
  const sections=card.querySelectorAll('details');
  sections.forEach((panel,index)=>{
   const uid=card.dataset.deviceUid;
   if(panel.hasAttribute('data-device-capabilities')){
    if(expandedCapabilities.has('closed:'+uid))panel.open=false;
    else if(expandedCapabilities.has(uid))panel.open=true;
    // Otherwise retain the new pending-review default from deviceCapabilitiesHtml.
   }else if(panel.hasAttribute('data-feature-shell')){
    if(closedFeatureShell.has(uid))panel.open=false;
    else if(expandedFeatureShell.has(uid))panel.open=true;
    // Otherwise retain the new pending-review default.
   }else if(panel.hasAttribute('data-device-more')&&deviceFocus==='ha'){
    // Settings > Manage HA devices must land on the actual HA control.
    panel.open=true;
   }else panel.open=saved.includes(index);
  });
 });
}
let deviceDetailUid='';
const deviceSection=document.querySelector('.page[data-view="devices"]');
function openDeviceDetail(uid){deviceDetailUid=uid;loadDevices();window.scrollTo({top:0,behavior:'smooth'})}
function closeDeviceDetail(){deviceDetailUid='';deviceFocus='';loadDevices();window.scrollTo({top:0,behavior:'smooth'})}
$('#deviceDetailBack').onclick=closeDeviceDetail;
let deviceLoadBusy=false;
async function loadDevices(){
 if(deviceLoadBusy)return;
 deviceLoadBusy=true;
 try {
  const d=await get('/rflink/api/devices');lastDeviceView=d;
  renderDeviceCache(d);
 } catch(e){toast(rfTranslate('dyn.devices_load_failed'))}
 finally{deviceLoadBusy=false}
}
function renderDeviceCache(d=lastDeviceView){
 if(!d)return;
 try {
  let devs=pairOnly?d.devices.filter(x=>x.pairing_supported||(pairProfile(x)?.pair_method||'none')!=='none'):d.devices;
  const search=deviceSearchText.toLocaleLowerCase().trim();
  if(search)devs=devs.filter(x=>[x.name,x.protocol,x.device_type,x.profile_id,x.channel===0?'433.42 433,42':x.channel===1?'433.92 433,92':x.channel===2?'868.30 868,30':'',...(x.rf_identities||[])].some(part=>String(part||'').toLocaleLowerCase().includes(search)));
  renderDeviceRadioChoices(devs);
  if(deviceRadioChoice!=='all')devs=devs.filter(x=>deviceChannelKey(x)===deviceRadioChoice);
  const searchCount=$('#deviceSearchCount');if(searchCount)searchCount.textContent=rfFormat('nav4.found',{visible:devs.length,total:d.devices.length});
  $('#haMqttStatus').textContent=d.mqtt_connected?rfTranslate('dyn.mqtt_connected'):d.mqtt_configured?rfTranslate('dyn.mqtt_disconnected'):rfTranslate('dyn.mqtt_not_configured');
  $('#deviceStorageNotice').textContent=d.storage_ok?rfTranslate('dyn.storage_ok'):rfTranslate('dyn.storage_uncertain');
  $('#deviceStorageNotice').hidden=d.storage_ok===true;
  $('#pairNotice').hidden=!pairOnly;
  $('#deviceSub').textContent=pairOnly?rfTranslate('dyn.pairing_only'):rfTranslate('dyn.device_sub_simple');
  const pairButton=$('#pairDevicesToggle');if(pairButton)pairButton.textContent=rfTranslate(pairOnly?'devices.show_all':'devices.pairing_filter');
  const pendingCandidates=!pairOnly?(d.candidates||[]):[];const candidateSection=$('#candidateSection');if(candidateSection)candidateSection.hidden=pendingCandidates.length===0;
  // The device overview is intentionally separate from setup and RF maintenance.
  const currentDetail=(d.devices||[]).find(x=>x.uid===deviceDetailUid);
  if(deviceDetailUid&&!currentDetail)deviceDetailUid='';
  deviceSection.classList.toggle('device-detail-mode',!!deviceDetailUid);
  $('#deviceDetailHeader').hidden=!deviceDetailUid;
  $('#deviceDetailTitle').textContent=currentDetail?.name||'';
  if(deviceDetailUid){devs=(d.devices||[]).filter(x=>x.uid===deviceDetailUid);if(candidateSection)candidateSection.hidden=true;}
  if(deviceFocus==='ha' && !deviceDetailUid && d.devices?.length===1){deviceDetailUid=d.devices[0].uid;devs=[d.devices[0]];deviceSection.classList.add('device-detail-mode');$('#deviceDetailHeader').hidden=false;$('#deviceDetailTitle').textContent=d.devices[0].name;}
  // Do not replace a live dropdown while its user is choosing an option.
  const focused=document.activeElement;
  if(focused?.matches?.('#deviceList select'))return;
  rememberDevicePanels();
  $('#deviceList').innerHTML=devs.length?devs.map(x=>{
   const p=getProtocol(x.protocol),selected=profilesFor(x).find(y=>y.id===x.profile_id),sensor=p?.learn_category==='sensor'||['co2_sensor','humidity_sensor','motion_sensor'].includes(selected?.device_type);
   const pending=(sensor?0:(x.pending_actions||[]).filter(k=>ordinaryDeviceAction(x,k)).length)+(x.pending_fields||[]).length;
   const visible=(sensor?[]:(x.actions||[]).filter(k=>ordinaryDeviceAction(x,k)).map(k=>actionLabel(p,k))).concat((x.fields||[]).filter(f=>haProfiles.has(f.key)).map(f=>rfFieldLabel(f.key,haProfiles.get(f.key))));
   const mqttAllowed=d.mqtt_connected&&(x.ha_enabled||visible.length);
   const haButton=`<button class="btn out small" data-ha-toggle="${esc(x.uid)}" data-ha-enabled="${x.ha_enabled?'1':'0'}" data-name="${esc(x.name)}" ${mqttAllowed?'':'disabled title="'+esc(rfTranslate('dyn.mqtt_no_features_title'))+'"'}>${esc(rfTranslate(x.ha_enabled?'dyn.remove_from_ha':'dyn.add_to_ha'))}</button>`;
   const chosen=selected?selected.name:rfTranslate('ux3.no_profile');
   const readyActions=controlKeys(selected);
   const nativePaired=x.protocol==='SomfyRTS56'?!!x.somfy?.paired:x.protocol==='IthoRFT'?!!x.itho?.paired:true;
   const userDecoderReady=!!x.local_tx_enabled&&(window.rfUserLibraryCache?.decoders||[]).some(p=>p.id===x.protocol&&p.tx_source&&p.enabled);
   const controlReady=!sensor&&!!selected&&readyActions.some(a=>(d.tx_controls_enabled&&nativePaired&&(x.tx_ready_actions||[]).includes(a))||(userDecoderReady&&(x.actions||[]).includes(a)));
   const controlState=sensor?'ux3.sensor_status':!selected?'ux3.profile_missing_status':controlReady?'ux3.control_ready_status':'ux3.receive_only_status';
   if(!deviceDetailUid){
    const next=!selected?rfTranslate('deviceux.setup_device'):rfTranslate('deviceux.settings');
    const step=!selected?(x.profile_id?rfTranslate('deviceux.profile_unavailable'):rfTranslate('deviceux.no_profile_help')):'';
    const last=x.age_ms==null?rfTranslate('rf60.not_seen'):age(x.age_ms);
    const band=x.channel===0?'433,42 MHz':x.channel===1?'433,92 MHz':x.channel===2?'868,30 MHz':rfTranslate('nav4.unknown_radio');
    const moreId='rf60-more-'+esc(x.uid);
    return `<article class="card device device-overview rf60-device" data-device-uid="${esc(x.uid)}">
      <div class="rf60-device-head"><div class="rf60-device-icon" aria-hidden="true">${sensor?'◉':x.protocol==='RamsesII'?'✿':'⌁'}</div><div class="rf60-device-title"><h3>${esc(x.name)}</h3><p class="muted small">${esc(band)} · ${esc(chosen||x.protocol)}</p></div><span class="pill ${controlReady?'':'warn'}">${esc(rfTranslate(controlState))}</span></div>
      <div class="rf60-device-last"><span class="muted small">${esc(rfTranslate('rf60.last_seen'))}</span><b>${esc(last)}</b></div>
      ${pending?'<p class="rf60-pending">'+esc(rfFormat('dyn.features_to_review',{n:pending}))+'</p>':''}
      ${step?'<p class="muted small rf60-device-hint">'+esc(step)+'</p>':''}
      <div class="rf60-device-foot">
       ${controlReady?`<details class="rf60-quick"><summary class="btn green">${esc(rfTranslate('rf60.control'))}</summary><div class="rf60-quick-body">${deviceOperationsHtml(x,d,true)}</div></details>`:''}
       ${sensor&&selected?`<details class="rf60-quick"><summary class="btn out">${esc(rfTranslate('rf60.values'))}</summary><div class="rf60-quick-body">${(x.fields||[]).length?(x.fields||[]).slice(0,8).map(f=>'<div class="row"><span>'+esc(rfFieldLabel(f.key,f.key))+'</span><b>'+esc(f.value??'—')+'</b></div>').join(''):'<p class="muted small">'+esc(rfTranslate('rf60.no_values'))+'</p>'}</div></details>`:''}
       <button class="btn out" type="button" data-device-open="${esc(x.uid)}">${esc(next)}</button>
       <details class="rf60-more"><summary>${esc(rfTranslate('rf60.more_details'))}</summary><p class="muted small">${esc(rfTranslate('rf60.protocol'))}: ${esc(x.protocol)}<br>${esc(rfTranslate(x.ha_enabled?'dyn.in_ha':'ux3.not_in_ha'))}<br>${esc(rfTranslate('rf60.last_seen'))}: ${esc(last)}</p></details>
      </div>
    </article>`;
   }
   return `<div class="card device device-detail-card" data-device-uid="${esc(x.uid)}">
    <div class="device-top"><div><h3>${esc(x.name)}</h3><p class="muted small">${esc(x.protocol)} · ${x.channel===0?'433,42 MHz':x.channel===1?'433,92 MHz':x.channel===2?'868,30 MHz':''} · ${esc(rfTranslate(sensor?'dyn.receive_sensor':'dyn.rf_device'))}</p></div>
    <div class="device-summary"><span class="pill ${controlReady?'':'warn'}">${esc(rfTranslate(controlState))}</span><span class="pill neutral">${esc(rfTranslate(x.ha_enabled?'dyn.in_ha':'ux3.not_in_ha'))}</span></div></div>
    <div class="device-setup-flow ${!selected?'device-unconfigured':''}">
      <section class="device-step-panel">
        <div class="device-step-heading"><span class="device-step-number">1</span><div><h4>${esc(rfTranslate('deviceux6.profile_title'))}</h4><p class="muted small">${esc(rfTranslate('deviceux6.profile_help'))}</p></div></div>
        ${selected?'<p class="device-current-profile">'+esc(chosen)+'</p>':''}
        ${profilePickerHtml(x)}
      </section>
      ${selected?`<section class="device-step-panel"><div class="device-step-heading"><span class="device-step-number">2</span><div><h4>${esc(rfTranslate('deviceux6.pair_title'))}</h4><p class="muted small">${esc(rfTranslate('deviceux6.pair_help'))}</p></div></div>${pairButtonHtml(x)}</section>`:''}
    </div>
    ${selected?`<section class="card rf60-device-control-panel"><h4>${esc(rfTranslate('rf60.controls_title'))}</h4><p class="muted small">${esc(rfTranslate(controlState))}</p>${deviceOperationsHtml(x,d,true)}</section>`:''}
    <section class="device-secondary rf61-device-secondary" aria-label="${esc(rfTranslate('rf60.advanced_details'))}">
      <div class="rf61-section-heading"><h4>${esc(rfTranslate('rf60.advanced_details'))}</h4><p class="muted small">${esc(rfTranslate('rf60.advanced_help'))}</p></div>
      <div class="rf61-section-grid">
        <div class="rf61-info-card rf61-full">
          <div class="rf61-card-heading"><span class="rf61-icon" aria-hidden="true">◉</span><div><h4>${esc(rfTranslate('rf61.functions_title'))}</h4><p class="muted small">${esc(rfFormat('rf61.functions_help',{n:(x.actions||[]).length+(x.fields||[]).length}))}</p></div></div>
          ${pending?'<p class="rf60-pending">'+esc(rfTranslate('dyn.review_needed'))+'</p>':''}
          <div class="rf61-card-controls">${deviceCapabilitiesHtml(x)}</div>
        </div>
        <div class="rf61-info-card rf61-full rf61-manage-card">
          <div class="rf61-card-heading"><span class="rf61-icon" aria-hidden="true">⌂</span><div><h4>${esc(rfTranslate('rf61.ha_title'))}</h4><p class="muted small">${esc(rfTranslate('rf61.ha_help'))}</p></div></div>
          <div class="device-controls">${haButton}${x.tx_eligible&&x.ha_enabled?`<button class="btn out small" data-tx-toggle="${esc(x.uid)}" data-tx-enabled="${x.tx_enabled?'1':'0'}" data-name="${esc(x.name)}" ${d.mqtt_connected?'':'disabled title="'+esc(rfTranslate('dyn.mqtt_offline'))+'"'}>${esc(rfTranslate(x.tx_enabled?'dyn.disable_ha_control':'dyn.allow_ha_control'))}</button>`:''}<button class="btn out small" data-rename="${esc(x.uid)}" data-name="${esc(x.name)}">${esc(rfTranslate('dyn.rename'))}</button></div>
        </div>
        <div class="rf61-small-grid"><div class="rf61-info-card rf61-compact-card">
          <div class="rf61-card-heading"><span class="rf61-icon" aria-hidden="true">⌁</span><div><h4>${esc(rfTranslate('rf61.signals_title'))}</h4><p class="muted small">${esc(rfTranslate('rf61.signals_help'))}</p></div></div>
          ${deviceSignalsHtml(x,d.candidates)}
        </div>
        ${profileSetupHtml(x)?`<div class="rf61-info-card rf61-compact-card"><div class="rf61-card-heading"><span class="rf61-icon" aria-hidden="true">☷</span><div><h4>${esc(rfTranslate('rf61.guide_title'))}</h4><p class="muted small">${esc(rfTranslate('rf61.guide_help'))}</p></div></div>${profileSetupHtml(x)}</div>`:''}
        <div class="rf61-info-card rf61-compact-card">
          <div class="rf61-card-heading"><span class="rf61-icon" aria-hidden="true">#</span><div><h4>${esc(rfTranslate('rf61.id_title'))}</h4><p class="muted small">${esc(rfTranslate('rf61.id_help'))}</p></div></div>
          <details class="decoder-tech"><summary>${esc(rfTranslate('dyn.technical_device_id'))}</summary><p class="mono small">${esc(x.uid)}</p></details>
        </div></div>
      </div>
    </section>
    <div class="rf60-remove-footer"><div><b>${esc(rfTranslate('dyn.delete_device'))}</b><p class="muted small">${esc(rfTranslate('rf60.remove_help'))}</p></div><button type="button" class="btn out" data-delete="${esc(x.uid)}" data-name="${esc(x.name)}">${esc(rfTranslate('dyn.delete_device'))}</button></div>
   </div>`;
  }).join(''):'<div class="card empty">'+esc(rfTranslate(deviceSearchText||deviceRadioChoice!=='all'?'nav4.no_results':pairOnly?'dyn.no_pair_devices':'dyn.no_devices'))+'</div>';
  restoreDevicePanels();
  document.dispatchEvent(new CustomEvent('rflink-devices-rendered',{detail:d}));
  $('#candidateList').innerHTML=pendingCandidates.length?pendingCandidates.map(x=>`<div class="card device candidate"><div><h3>${esc(x.protocol||rfTranslate('dyn.new_signal'))}</h3><p class="muted small">${esc(rfFormat('dyn.received_times',{n:x.observations}))} · ${esc(rfTranslate(x.ready?'dyn.ready_review':'dyn.still_listening'))}</p><p class="small">${getProtocol(x.protocol)?esc(rfProtocolText(getProtocol(x.protocol),'instruction')):esc(rfTranslate('dyn.check_signal'))}</p>${x.suggest_uid?`<div class="notice small">${esc(rfFormat('dyn.looks_like',{name:x.suggest_name}))}</div>`:''}<div class="actions">${x.ready?`<button class="btn green" data-confirm="${esc(x.identity)}">${esc(rfTranslate('dyn.review_add'))}</button>`:''}<button class="btn out small" data-dismiss-candidate="${esc(x.identity)}">${esc(rfTranslate('profileflow.dismiss_candidate'))}</button></div><details class="decoder-tech"><summary>${esc(rfTranslate('dyn.more_technical'))}</summary><p class="mono small">${esc(rfFormat('dyn.rf_identity',{id:x.identity}))}</p>${x.suggest_uid&&x.ready?`<button class="btn out small" data-link-id="${esc(x.identity)}" data-link-uid="${esc(x.suggest_uid)}">${esc(rfFormat('dyn.link_to',{name:x.suggest_name}))}</button>`:''}</details></div><span class="pill warn">${esc(rfTranslate('dyn.not_saved'))}</span></div>`).join(''):'<div class="card empty">'+esc(rfTranslate(pairOnly?'dyn.pairing_view':'dyn.no_new_signals'))+'</div>';
  $$('[data-device-open]').forEach(b=>b.onclick=()=>openDeviceDetail(b.dataset.deviceOpen));
  $$('[data-ha-toggle]').forEach(b=>b.onclick=()=>toggleHA(b.dataset.haToggle,b.dataset.haEnabled==='1',b.dataset.name));
  $$('[data-device-profile]').forEach(e=>e.onchange=()=>saveDeviceProfile(e.dataset.deviceProfile,e.value));
  $$('[data-profile-command]').forEach(b=>b.onclick=()=>sendProfileCommand(b.dataset.profileCommand,b.dataset.command,b.dataset.transport));
  $$('[data-somfy-pair]').forEach(b=>b.onclick=()=>pairSomfy(b.dataset.somfyPair));
  $$('[data-itho-pair]').forEach(b=>b.onclick=()=>pairItho(b.dataset.ithoPair,b.dataset.mode||'standard'));
  $$('[data-itho-unpair]').forEach(b=>b.onclick=()=>unpairItho(b.dataset.ithoUnpair));
  $$('[data-native-command]').forEach(b=>b.onclick=()=>sendNative(b.dataset.nativeUid,b.dataset.nativeCommand));
  $$('[data-tx-toggle]').forEach(b=>b.onclick=()=>toggleTX(b.dataset.txToggle,b.dataset.txEnabled==='1',b.dataset.name));
  $$('[data-dismiss-candidate]').forEach(b=>b.onclick=()=>dismissCandidate(b.dataset.dismissCandidate));$$('[data-confirm]').forEach(b=>b.onclick=()=>openName(b.dataset.confirm));$$('[data-link-id]').forEach(b=>b.onclick=()=>linkCandidate(b.dataset.linkId,b.dataset.linkUid));$$('[data-rename]').forEach(b=>b.onclick=()=>renameDevice(b.dataset.rename,b.dataset.name));$$('[data-delete]').forEach(b=>b.onclick=()=>removeDevice(b.dataset.delete,b.dataset.name));
  $$('[data-device-more]').forEach(e=>e.ontoggle=()=>{if(!e.isConnected)return;if(e.open){if(deviceFocus!=='ha')expandedMore.add(e.dataset.deviceMore)}else expandedMore.delete(e.dataset.deviceMore)});$$('[data-device-danger]').forEach(e=>e.ontoggle=()=>{if(!e.isConnected)return;if(e.open)expandedDangerous.add(e.dataset.deviceDanger);else expandedDangerous.delete(e.dataset.deviceDanger)});
  $$('[data-feature-shell]').forEach(e=>e.ontoggle=()=>{if(!e.isConnected)return;const uid=e.dataset.featureShell;if(e.open){expandedFeatureShell.add(uid);closedFeatureShell.delete(uid)}else{closedFeatureShell.add(uid);expandedFeatureShell.delete(uid)}});
  $$('[data-device-capabilities]').forEach(e=>e.ontoggle=()=>{if(!e.isConnected)return;if(e.open){expandedCapabilities.add(e.dataset.deviceCapabilities);expandedCapabilities.delete('closed:'+e.dataset.deviceCapabilities)}else{expandedCapabilities.delete(e.dataset.deviceCapabilities);expandedCapabilities.add('closed:'+e.dataset.deviceCapabilities)}});
  $$('[data-feature]').forEach(b=>b.onclick=()=>changeFeature(b.dataset.featureUid,b.dataset.feature,b.dataset.featureKey));
  $$('[data-device-signals]').forEach(e=>e.ontoggle=()=>{if(!e.isConnected)return;if(e.open)expandedSignals.add(e.dataset.deviceSignals);else expandedSignals.delete(e.dataset.deviceSignals)});
  $$('[data-replace-select]').forEach(e=>e.onchange=()=>replacementSelections.set(e.dataset.replaceSelect+'\u0000'+e.dataset.identity,e.value));
  $$('[data-replace-id]').forEach(b=>b.onclick=()=>replaceIdentity(b.dataset.replaceId,b.dataset.identity));
  $$('[data-unlink-id]').forEach(b=>b.onclick=()=>unlinkIdentity(b.dataset.unlinkId,b.dataset.identity));
  $$('[data-learn-replacement]').forEach(b=>b.onclick=()=>{expandedSignals.add(b.dataset.learnReplacement);setLearnMode('device');show('learn');toast(rfTranslate('dyn.learn_replacement_toast'))});
 } catch(e){toast(rfTranslate('dyn.devices_load_failed'))}
}
async function unlinkIdentity(uid,identity){
 if(!confirm(rfFormat('dyn.confirm_unlink',{identity})))return;
 try{await post('/rflink/api/device?action=unlink_identity&uid='+encodeURIComponent(uid)+'&identity='+encodeURIComponent(identity));toast(rfTranslate('dyn.unlinked'));await loadDevices()}catch(e){toast(rfTranslate('dyn.unlink_failed'))}
}
async function replaceIdentity(uid,identity){
 const candidate=replacementSelections.get(uid+'\0'+identity)||$$('[data-replace-select]').find(e=>e.dataset.replaceSelect===uid&&e.dataset.identity===identity)?.value;
 if(!candidate){toast(rfTranslate('dyn.choose_confirmed_signal'));return}
 if(!confirm(rfFormat('dyn.confirm_replace',{old:identity,new:candidate})))return;
 try{await post('/rflink/api/device?action=replace_identity&uid='+encodeURIComponent(uid)+'&identity='+encodeURIComponent(identity)+'&replacement='+encodeURIComponent(candidate));replacementSelections.delete(uid+'\0'+identity);toast(rfTranslate('dyn.replaced'));await loadDevices()}catch(e){toast(rfTranslate('dyn.replace_failed'))}
}
function renderHealth(d){
 const radios=d.channels||[],missing=radios.filter(x=>!x.checked||!x.available||!x.rx_ready||!x.tx_ready),decodeMismatch=d.expected_decoders!=null&&d.decoder_count!==d.expected_decoders,storageProblem=!d.storage_ready||!d.storage_ok||d.user_library_ready===false;
 const card=$('#healthCard');card.className='card health'+(storageProblem||decodeMismatch?' error':missing.length?'':' ok');
 $('#healthTitle').textContent=rfTranslate(storageProblem?'dyn.storage_attention':decodeMismatch?'dyn.decoder_incomplete':missing.length?'dyn.hardware_incomplete':'dyn.all_ready');
 $('#healthExplanation').textContent=storageProblem?rfTranslate('dyn.storage_problem_help'):decodeMismatch?rfTranslate('dyn.decoder_problem_help'):missing.length?rfFormat('dyn.hardware_problem_help',{missing:missing.length,total:radios.length}):rfTranslate('dyn.ready_help');
 $('#healthDetails').innerHTML=`<div class="row"><span>${esc(rfTranslate('diag.uptime'))}</span><b class="mono">${esc(uptime(d.uptime_ms))}</b></div><div class="row"><span>${esc(rfTranslate('dyn.esp_web'))}</span><b>${esc(rfTranslate('dyn.online'))}</b></div><div class="row"><span>${esc(rfTranslate('dyn.radios'))}</span><b>${esc(rfFormat('dyn.ready_count',{ready:radios.length-missing.length,total:radios.length}))}</b></div><div class="row"><span>${esc(rfTranslate('dyn.decoders'))}</span><b>${esc(rfFormat('dyn.loaded',{value:(d.decoder_count??'—')+(d.expected_decoders!=null?'/'+d.expected_decoders:'')}))}</b></div><div class="row"><span>${esc(rfTranslate('dyn.device_storage'))}</span><b>${esc(storageProblem?rfTranslate('dyn.check_required'):rfFormat('dyn.saved_count',{n:d.saved_devices||0}))}</b></div>${d.led?`<div class="row"><span>${esc(rfTranslate('static.015'))}</span><b id="ledStatus">${esc(ledStatusLabel(d.led.status_code||d.led.status))}</b></div>`:''}`;
 $('#diagStored').textContent=d.saved_devices??0;$('#diagDecoders').textContent=(d.decoder_count??'—')+(d.expected_decoders!=null?'/'+d.expected_decoders:'');$('#diagStorage').textContent=storageProblem?rfTranslate('dyn.storage_error'):rfTranslate('dyn.available');$('#diagOverview').textContent=$('#healthExplanation').textContent;
}
async function changeFeature(uid,action,key){
 const removing=action.startsWith('remove_');
 if(removing&&!confirm(rfFormat('dyn.confirm_remove_feature',{key})))return;
 try {await post('/rflink/api/device?action='+action+'&uid='+encodeURIComponent(uid)+'&key='+encodeURIComponent(key));toast(rfTranslate(removing?'dyn.feature_removed':'dyn.feature_approved'));await loadDevices();}
 catch(e){toast(rfTranslate('dyn.feature_failed'));await loadDevices()}
}
async function toggleHA(uid,enabled,name){
 if(enabled&&!confirm(rfFormat('dyn.confirm_remove_ha',{name})))return;
 try{await post('/rflink/api/device?action='+(enabled?'ha_disable':'ha_enable')+'&uid='+encodeURIComponent(uid));toast(rfTranslate(enabled?'dyn.ha_removed':'dyn.ha_sent'));await loadDevices();}
 catch(e){toast(rfTranslate('dyn.ha_failed'));await loadDevices();}
}
async function pairSomfy(uid){
 if(!confirm(rfTranslate('somfy.pair_confirm')))return;
 try{await post('/rflink/api/device?action=somfy_pair&uid='+encodeURIComponent(uid));toast(rfTranslate('somfy.pair_sent'));await loadDevices()}
 catch(e){toast(rfTranslate('somfy.pair_failed'))}
}
async function pairItho(uid,mode='standard'){
 if(!confirm(rfTranslate(mode==='compat'?'itho.compat_confirm':'itho.pair_confirm')))return;
 try{await post('/rflink/api/device?action=itho_pair&uid='+encodeURIComponent(uid)+'&mode='+encodeURIComponent(mode));toast(rfTranslate('itho.pair_sent'));await loadDevices()}
 catch(e){toast(rfTranslate('itho.pair_failed'))}
}
async function unpairItho(uid){
 if(!confirm(rfTranslate('itho.unpair_confirm')))return;
 try{await post('/rflink/api/device?action=itho_unpair&uid='+encodeURIComponent(uid));toast(rfTranslate('itho.unpair_sent'));await loadDevices()}
 catch(e){toast(rfTranslate('itho.unpair_failed'))}
}
async function saveDeviceProfile(uid,id){
 const p=profilesFor((lastDeviceView?.devices||[]).find(d=>d.uid===uid)||{}).find(p=>p.id===id);
 try{
  await postWithWriteToken('/rflink/api/user-tx',{action:'profile',uid,id});
  const picker=[...document.querySelectorAll('[data-profile-panel]')].find(e=>e.dataset.profilePanel===uid);
  if(picker)picker.open=false;
  if(document.activeElement?.matches?.('[data-device-profile]'))document.activeElement.blur();
  toast(rfTranslate('profileflow.saved'));await loadDevices();
 }catch(e){if(document.activeElement?.matches?.('[data-device-profile]'))document.activeElement.blur();toast(rfTranslate('profileflow.save_error'));await loadDevices()}
}
async function postWithWriteToken(url,data){
 let token=window.rfUserLibraryCache?.write_token;
 if(!token){const response=await rfTrackedFetch('/rflink/api/user-library');if(!response.ok)throw Error('User library unavailable');const data=await response.json();token=data.write_token;window.rfUserLibraryCache={...data,profiles:[...new Map([...(window.rfUserLibraryCache?.profiles||[]).map(p=>[p.id,p]),...(data.profiles||[]).map(p=>[p.id,p])]).values()]};}
 if(!token)throw Error('User library unavailable');
 const body=new URLSearchParams({...data,write_token:String(token)});
 return req(url,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});
}
async function sendProfileCommand(uid,command,transport){
 const d=(lastDeviceView?.devices||[]).find(x=>x.uid===uid);
 const p=d&&profilesFor(d).find(p=>p.id===d.profile_id);
 if(!d||!p||!controlKeys(p).includes(command))return;
 if(!confirm(rfFormat('profileflow.send_confirm',{command:displayCommand(p,command)})))return;
 try{
  if(transport==='user')await postWithWriteToken('/rflink/api/user-tx',{action:'send',uid,command});
  else await post('/rflink/api/device?action=native_send&uid='+encodeURIComponent(uid)+'&command='+encodeURIComponent(command));
  toast(rfTranslate('profileflow.sent'));await loadDevices();
 }catch(e){toast(rfTranslate('profileflow.send_error'))}
}
async function sendNative(uid,command){
 const protocol=lastDeviceView?.devices?.find(d=>d.uid===uid)?.protocol;
 const prefix=protocol==='IthoRFT'?'itho':'somfy';
 try{await post('/rflink/api/device?action=native_send&uid='+encodeURIComponent(uid)+'&command='+encodeURIComponent(command));toast(rfTranslate(prefix+'.command_sent'));await loadDevices()}
 catch(e){toast(rfTranslate(prefix+'.command_failed'))}
}
async function toggleTX(uid,enabled,name){
 if(!confirm(rfFormat(enabled?'dyn.confirm_disable_tx':'dyn.confirm_enable_tx',{name})))return;
 try{await post('/rflink/api/device?action='+(enabled?'tx_disable':'tx_enable')+'&uid='+encodeURIComponent(uid));toast(rfTranslate('dyn.tx_saved'));await loadDevices()}catch(e){toast(rfTranslate('dyn.tx_failed'))}
}
async function renameDevice(uid,current){const next=prompt(rfTranslate('dyn.new_device_name'),current);if(next===null)return;const name=next.trim();if(!name||name.length>80){toast(rfTranslate('dyn.name_length'));return}try{await post('/rflink/api/device?action=rename&uid='+encodeURIComponent(uid)+'&name='+encodeURIComponent(name));toast(rfTranslate('dyn.name_saved'));loadDevices()}catch(e){toast(rfTranslate('dyn.rename_failed'))}}
async function dismissCandidate(identity){
 if(!confirm(rfTranslate('profileflow.dismiss_confirm')))return;
 try{await post('/rflink/api/candidate?action=dismiss&identity='+encodeURIComponent(identity));toast(rfTranslate('profileflow.dismissed'));await loadDevices()}
 catch(e){toast(rfTranslate('profileflow.dismiss_failed'))}
}
async function removeDevice(uid,name){if(!confirm(rfFormat('dyn.confirm_delete',{name})))return;try{await post('/rflink/api/device?action=delete&uid='+encodeURIComponent(uid));toast(rfTranslate('dyn.device_removed'));loadDevices()}catch(e){toast(rfTranslate('dyn.delete_failed'))}}
function wizardMessage(title,help){$('#wizardTitle').textContent=title;$('#wizardHelp').textContent=help}
function resetWizardUi(){wizardStage='idle';wizardSelected=null;$('#radioSelectCard').hidden=false;$('#learnIntro').hidden=false;$('#wizardStart').hidden=false;$('#wizardCancel').hidden=true;$('#wizardResult').hidden=true;$('#wizardTechnicalDetails').open=false;$('#wizardStart').disabled=selectedChannel<0||!!(latestStatus&&(!latestStatus.storage_ok||!latestStatus.storage_ready));}
async function cancelWizard(){const token=++wizardToken;clearTimeout(baselineTimer);baselineTimer=null;try{await post('/rflink/api/developer?action=reset')}catch(e){}if(token!==wizardToken)return;resetWizardUi();wizardMessage(rfTranslate('dyn.add_device'),rfTranslate('dyn.wizard_start_help'));}
$('#wizardCancel').onclick=cancelWizard;$('#wizardRetry').onclick=cancelWizard;
$('#wizardStart').onclick=async()=>{
 if(selectedChannel<0||wizardStage!=='idle'||!latestStatus)return;
 if(!latestStatus.storage_ok||!latestStatus.storage_ready){toast(rfTranslate('dyn.storage_unavailable'));return;}
 const channel=latestStatus.channels.find(x=>x.index===selectedChannel);if(!channel||!channel.rx_ready){toast(rfTranslate('dyn.receiver_unavailable'));return}
 const token=++wizardToken;wizardStage='baseline';wizardStartedAt=latestStatus.uptime_ms;wizardSelected=null;$('#wizardDone').hidden=true;$('#wizardStart').hidden=true;$('#wizardCancel').hidden=false;$('#radioSelectCard').hidden=true;$('#learnIntro').hidden=true;wizardMessage(rfTranslate('dyn.step1'),rfTranslate('dyn.step1_help'));
 try{await post('/rflink/api/developer?action=baseline&channel='+selectedChannel);baselineTimer=setTimeout(async()=>{
 if(token!==wizardToken)return;try{await post('/rflink/api/developer?action=finish_baseline');if(token!==wizardToken)return;await post('/rflink/api/developer?action=listen&channel='+selectedChannel+'&label=device');if(token!==wizardToken)return;wizardStage='listening';wizardMessage(rfTranslate('dyn.step2'),rfTranslate('dyn.step2_help'));}catch(e){wizardStage='error';wizardMessage(rfTranslate('dyn.measure_failed'),rfTranslate('dyn.measure_failed_help'));$('#wizardCancel').hidden=false;}},5000)}catch(e){resetWizardUi();wizardMessage(rfTranslate('dyn.start_failed'),rfTranslate('dyn.start_failed_help'))}
};
async function pollWizard(){if(wizardStage!=='listening'||!latestStatus)return;try{const d=await get('/rflink/api/devices');const since=Math.max(0,latestStatus.uptime_ms-wizardStartedAt);const ready=(d.candidates||[]).filter(x=>x.ready&&x.channel===selectedChannel&&x.age_ms!==null&&x.age_ms<=since);if(!ready.length)return;const c=ready[0];wizardSelected=c;wizardStage='confirm';wizardMessage(rfTranslate('dyn.step3'),rfTranslate('ui.device_found_help'));$('#wizardProtocol').textContent=rfTranslate('ui.protocol_label')+': '+c.protocol+' · '+rfFormat('dyn.observations',{n:c.observations});$('#wizardLearnAdvice').innerHTML=instructions(c);$('#wizardLearnAdvice').hidden=false;setCandidateProfilePicker('wizard',c);renderReview(c,'wizard');$('#wizardName').value='';const haReady=!!d.mqtt_connected&&!!c.ha_available;$('#wizardHaWrap').hidden=!haReady;$('#wizardHa').checked=haReady;$('#wizardResult').hidden=false;$('#wizardConfirm').disabled=!getProtocol(c.protocol);$('#wizardLink').hidden=!c.suggest_uid;if(c.suggest_uid)$('#wizardLink').textContent=rfFormat('dyn.link_to_name',{name:c.suggest_name});}catch(e){wizardMessage(rfTranslate('dyn.temp_connection'),rfTranslate('dyn.temp_connection_help'))}}
async function finishWizard(link){if(!wizardSelected)return;const c=wizardSelected;if(!getProtocol(c.protocol)){toast(rfTranslate('dyn.decoder_safe_disabled'));return}const name=$('#wizardName').value.trim();if(!link&&(!name||name.length>80)){toast(rfTranslate('dyn.enter_name'));return}const approval=selectedReview('wizard',c);if(!link&&approval.total&&!approval.count&&!confirm(rfTranslate('dyn.confirm_no_features')))return;$('#wizardConfirm').disabled=true;try{let url='/rflink/api/candidate?action='+(link?'link':'confirm')+'&identity='+encodeURIComponent(c.identity);url+=link?'&uid='+encodeURIComponent(c.suggest_uid):'&name='+encodeURIComponent(name)+'&ha='+($('#wizardHaWrap').hidden?'0':$('#wizardHa').checked&&approval.count?'1':'0')+'&profile_id='+encodeURIComponent($('#wizardProfile').value)+approvalParams('wizard',c);const saved=await post(url);await post('/rflink/api/developer?action=reset');resetWizardUi();$('#wizardDone').hidden=false;$('#wizardDone').textContent=link?rfTranslate('dyn.signal_linked'):saved.ha_requested&&!saved.ha_enabled?rfTranslate('dyn.saved_ha_failed'):rfTranslate('dyn.device_saved');wizardMessage(rfTranslate('dyn.done'),rfTranslate('dyn.done_help'));toast(rfTranslate('dyn.device_saved'));}catch(e){$('#wizardConfirm').disabled=false;toast(rfTranslate('dyn.save_failed'))}}
$('#wizardConfirm').onclick=()=>finishWizard(false);$('#wizardLink').onclick=()=>{if(wizardSelected&&confirm(rfFormat('dyn.confirm_link',{name:wizardSelected.suggest_name})))finishWizard(true)};
function openName(id){pendingIdentity=id;$('#deviceName').value='';const candidate=(lastDeviceView?.candidates||[]).find(c=>c.identity===id);if(!candidate)return;$('#modalLearnAdvice').innerHTML=instructions(candidate);$('#modalLearnAdvice').hidden=false;setCandidateProfilePicker('modal',candidate);renderReview(candidate,'modal');$('#modalSave').disabled=!getProtocol(candidate.protocol);const ready=!!lastDeviceView?.mqtt_connected&&!!candidate?.ha_available;$('#modalHaWrap').hidden=!ready;$('#modalHa').checked=ready;$('#nameModal').classList.add('show');setTimeout(()=>$('#deviceName').focus(),50)}$('#modalCancel').onclick=()=>$('#nameModal').classList.remove('show');$('#modalSave').onclick=async()=>{if(!pendingIdentity)return;const name=$('#deviceName').value.trim();const candidate=(lastDeviceView?.candidates||[]).find(c=>c.identity===pendingIdentity);if(!candidate||!getProtocol(candidate.protocol)||!name||name.length>80){toast(rfTranslate('dyn.modal_invalid'));return}const approval=selectedReview('modal',candidate);if(approval.total&&!approval.count&&!confirm(rfTranslate('dyn.confirm_store_only')))return;$('#modalSave').disabled=true;try{await post('/rflink/api/candidate?action=confirm&identity='+encodeURIComponent(pendingIdentity)+'&name='+encodeURIComponent(name)+'&ha='+($('#modalHaWrap').hidden?'0':$('#modalHa').checked&&approval.count?'1':'0')+'&profile_id='+encodeURIComponent($('#modalProfile').value)+approvalParams('modal',candidate));$('#nameModal').classList.remove('show');toast(rfTranslate('dyn.device_added'));loadDevices()}catch(e){toast(rfTranslate('dyn.add_failed'))}finally{$('#modalSave').disabled=false}};async function linkCandidate(id,uid){try{await post('/rflink/api/candidate?action=link&identity='+encodeURIComponent(id)+'&uid='+encodeURIComponent(uid));toast(rfTranslate('dyn.linked_existing'));loadDevices()}catch(e){toast(rfTranslate('dyn.link_failed'))}}
function openPairing(){pairOnly=true;show('devices')}const pairToggle=$('#pairDevicesToggle');if(pairToggle)pairToggle.onclick=()=>{pairOnly=!pairOnly;loadDevices()};
async function setupLearnRadios(){try{const d=await get('/rflink/api/status');renderLearnRadios(d.channels)}catch(e){$('#baselineBtn').disabled=true;$('#listenBtn').disabled=true;$('#learnRadioHelp').textContent=rfTranslate('dyn.radio_status_unreachable')}}
async function loadDeveloper(){try{const d=await get('/rflink/api/developer');const c=d.best;if(!c){$('#learnRepeats').textContent='—';$('#learnProtocol').textContent='—';$('#learnIdentity').textContent='—';$('#signalRssi').textContent='—';$('#signalLabel').textContent=d.mode==='baseline'?rfTranslate('dyn.measuring_background'):rfTranslate('dyn.waiting_capture');$('#signalBar').style.width='0';$('#snippet').value=rfTranslate('dyn.capture_first');return}$('#step4').classList.add('done');$('#learnRepeats').textContent=c.observations;$('#learnProtocol').textContent=c.protocol||rfTranslate('dyn.protocol_unknown');$('#learnIdentity').textContent=c.identity||rfTranslate('dyn.identity_unknown');$('#snippet').value=c.snippet||rfTranslate('dyn.no_code_suggestion');if(c.rssi_dbm!=null){$('#signalRssi').textContent=Number(c.rssi_dbm).toFixed(0)+' dBm';const p=Math.max(8,Math.min(100,(c.rssi_dbm+100)*1.6));$('#signalBar').style.width=p+'%';$('#signalLabel').textContent=c.rssi_dbm>-25?rfTranslate('dyn.signal_strong'):c.rssi_dbm>-70?rfTranslate('dyn.good'):rfTranslate('dyn.weak')}else{$('#signalRssi').textContent=c.kind==='raw'?rfTranslate('dyn.raw_rssi'):'—';$('#signalBar').style.width='48%';$('#signalLabel').textContent=rfTranslate('dyn.capture_found')}}catch(e){}}
$('#baselineBtn').onclick=async()=>{if(selectedChannel<0||$('#baselineBtn').disabled)return;clearTimeout(baselineTimer);try{await post('/rflink/api/developer?action=baseline&channel='+selectedChannel);$('#baselineState').textContent=rfTranslate('dyn.measuring');$('#step2').classList.add('done');baselineTimer=setTimeout(async()=>{try{await post('/rflink/api/developer?action=finish_baseline');$('#baselineState').textContent=rfTranslate('dyn.ready');$('#step3').classList.add('done');toast(rfTranslate('dyn.background_ready'))}catch(e){}},5000)}catch(e){toast(rfTranslate('dyn.background_failed'))}};$('#listenBtn').onclick=async()=>{if(selectedChannel<0||$('#listenBtn').disabled)return;try{await post('/rflink/api/developer?action=listen&channel='+selectedChannel+'&label='+encodeURIComponent(learnMode==='decoder'?'decoder_test':'device'));toast(rfTranslate('dyn.listen_started'))}catch(e){toast(rfTranslate('dyn.listen_failed'))}};
$('#stopListenBtn').onclick=async()=>{clearTimeout(baselineTimer);baselineTimer=null;try{await post('/rflink/api/developer?action=reset');$('#baselineState').textContent=rfTranslate('dyn.not_measured');['step2','step3','step4'].forEach(id=>$('#'+id).classList.remove('done'));await loadDeveloper();toast(rfTranslate('dyn.listen_stopped'))}catch(e){toast(rfTranslate('dyn.stop_failed'))}};
$('#ledBrightness').oninput=()=>{ledDirty=true;$('#ledBrightnessValue').textContent=$('#ledBrightness').value+'%'};
$('#ledHeartbeat').onchange=()=>{ledDirty=true};
$('#ledSave').onclick=async()=>{const b=$('#ledSave');b.disabled=true;try{const brightness=Number($('#ledBrightness').value);if(!Number.isInteger(brightness)||brightness<1||brightness>100)throw Error(rfTranslate('dyn.invalid_brightness'));await post('/rflink/api/led?brightness='+brightness+'&quiet='+($('#ledHeartbeat').checked?'0':'1'));ledDirty=false;toast(rfTranslate('dyn.led_saved'));await loadStatus()}catch(e){toast(rfTranslate('dyn.save_failed_short'))}finally{b.disabled=false}};
let decoderFilterValue='all',selectedDecoderName='';
function renderLibrary(d){
 libraryState=d;
 $('#libOfficialDecoders').textContent=d.official_decoders??0;$('#libOfficialProfiles').textContent=d.official_profiles??0;$('#libAttention').textContent=d.attention_count??0;$('#libSchema').textContent=d.schema??'—';$('#libSafe').textContent=d.safe_resolution_count??0;$('#libArchive').textContent=d.archive_count??0;
 const enabled=!!d.persistent_user_library;$('#libPersistent').textContent=rfTranslate(enabled?'library.enabled':'library.scaffold');
 const userDecoders=d.user_decoders??0,userProfiles=d.user_profiles??0;$('#libUserStat').hidden=!enabled||userDecoders<=0;$('#libProfileStat').hidden=!enabled||userProfiles<=0;if(enabled){$('#libUserDecoders').textContent=userDecoders;$('#libUserProfiles').textContent=userProfiles}
 const attention=(d.attention_count||0)>0;$('#libAttentionStat').hidden=!attention;$('#libraryReview').hidden=!attention;$('#libraryStatus').textContent=rfTranslate(attention?'library.status_attention':enabled?'library.status_ok':'library.status_scaffold');
 $('#libraryAttentionCard').hidden=!attention;if(attention)$('#libraryAttentionText').textContent=rfFormat('library.attention_count',{n:d.attention_count});
 $('#libraryAdvancedItems').textContent=(d.items||[]).map(x=>`${x.id} · ${x.source} · v${x.version} · ${x.pack||''}`).join('\n')||rfTranslate('library.no_items');
}
async function loadLibrary(){try{renderLibrary(await get('/rflink/api/library'))}catch(e){$('#libraryStatus').textContent=rfTranslate('library.unavailable');$('#libraryAttentionCard').hidden=true}}
function openLibraryReview(){show('settings');showSettingsPanel('library',true,true);if(libraryState&&(libraryState.attention_count||0)>0)toast(rfTranslate('library.review_pending'))}
$('#libraryAttentionOpen').onclick=openLibraryReview;$('#libraryReview').onclick=openLibraryReview;$('#libraryAdvanced').onclick=()=>{$('#libraryTechnicalPanel').hidden=!$('#libraryTechnicalPanel').hidden;$('#libraryAdvanced').textContent=rfTranslate($('#libraryTechnicalPanel').hidden?'version.technical':'library.hide_advanced')};
function decoderTxStatus(p){
 const registered=p.tx_encoder!=='none'&&p.tx_registered;
 if(p.learn_category==='sensor')return {label:rfTranslate('dyn.receive_only'),style:'',reason:rfTranslate('decoder.reason_sensor'),needed:rfTranslate('decoder.needed_none')};
 if(p.learn_category==='development')return {label:rfTranslate('dyn.example'),style:'',reason:rfTranslate('decoder.reason_example'),needed:rfTranslate('decoder.needed_none')};
 if(registered)return {label:p.tx_status==='hardware_verified'?rfTranslate('dyn.tx_hw_tested'):rfTranslate('dyn.tx_sw_tested'),style:p.tx_status==='hardware_verified'?'ready':'pending',reason:p.tx_status==='hardware_verified'?rfTranslate('decoder.reason_tx_ready'):rfTranslate('decoder.reason_tx_host'),needed:p.tx_status==='hardware_verified'?rfTranslate('decoder.needed_none'):rfTranslate('decoder.needed_hw_test')};
 if(p.name==='SomfyRTS56')return {label:rfTranslate('dyn.tx_blocked'),style:'',reason:rfTranslate('decoder.reason_somfy'),needed:rfTranslate('decoder.needed_somfy')};
 if(p.name==='KeeLoq')return {label:rfTranslate('dyn.tx_blocked'),style:'',reason:rfTranslate('decoder.reason_keeloq'),needed:rfTranslate('decoder.needed_keeloq')};
 return {label:rfTranslate('dyn.tx_unavailable'),style:'',reason:rfTranslate('decoder.reason_no_encoder'),needed:rfTranslate('decoder.needed_encoder')};
}
function decoderEntries(){return [...decoderProtocols.values()].filter(p=>decoderFilterValue==='all'?p.learn_category!=='development':p.learn_category===decoderFilterValue)}
function profileText(p,key){const block=p?.[key+'_i18n'];return block?.[rfLanguage]||block?.en||p?.[key]||''}
function knownProfiles(protocol){return (window.rfUserLibraryCache?.profiles||[]).filter(p=>p.protocol===protocol&&p.source==='builtin')}
function knownProfilesHtml(protocol){
 const profiles=knownProfiles(protocol);if(!profiles.length)return '';
 const families=new Map();for(const p of profiles){const group=(p.manufacturer||'')+' · '+(p.family||'');families.set(group,(families.get(group)||0)+1)}
 return '<div class="known-profiles"><div class="profile-summary-row"><b>'+esc(rfTranslate('decoder.known_profiles'))+' ('+profiles.length+')</b><span class="muted small">'+esc(rfTranslate('catalog.profile_uses_protocol'))+': '+esc(protocol)+'</span></div><div class="profile-summary-row">'+[...families].map(([label,n])=>'<span class="muted small">'+esc(label)+' ('+n+')</span>').join('')+'</div><button type="button" class="btn out small" id="openProtocolProfiles">'+esc(rfTranslate('catalog.show_related'))+' ('+profiles.length+')</button></div>';
}

function renderDecoderDetail(p){
 const detail=$('#decoderDetail');if(!p){detail.hidden=window.matchMedia('(max-width:850px)').matches;detail.innerHTML='<p class="muted">'+esc(rfTranslate('decoder.select_hint'))+'</p>';return;}detail.hidden=false;
 const tx=decoderTxStatus(p),registered=p.tx_encoder!=='none'&&p.tx_registered;
 const transportHint=p.rx_transport&&p.rx_transport!=='not_specified'?'<p class="muted small">RX: '+esc(p.rx_transport)+(p.rx_fallback&&p.rx_fallback!=='none'?' · fallback: '+esc(p.rx_fallback):'')+' (decoder manifest)</p>':'';
 const needed=tx.needed&&tx.needed!==rfTranslate('decoder.needed_none')?'<br><span class="muted">'+esc(rfTranslate('decoder.next_step'))+': '+esc(tx.needed)+'</span>':'';
 $('#decoderDetail').innerHTML='<div class="decoder-summary"><div><h3>'+esc(p.name)+'</h3><span class="muted small">'+esc(rfProtocolText(p,'band'))+' · '+esc(rfTranslate('dyn.rx_decoder_present'))+'</span></div><span class="decoder-status '+tx.style+'">'+esc(tx.label)+'</span></div>'+
 '<div class="decoder-info-grid"><div class="decoder-info-block"><b>'+esc(rfTranslate('decoder.how_to_learn'))+'</b><span class="small">'+esc(rfProtocolText(p,'instruction'))+'</span></div><div class="decoder-info-block"><b>'+esc(rfTranslate('dyn.in_ha_label'))+'</b><span class="small">'+esc(rfProtocolText(p,'ha_result'))+'</span></div><div class="decoder-info-block control"><b>'+esc(rfTranslate('decoder.control'))+'</b><span class="small">'+esc(tx.reason)+needed+'</span></div></div>'+
 (rfProtocolText(p,'warning')?'<p class="small"><b>'+esc(rfTranslate('dyn.what_to_know'))+':</b> '+esc(rfProtocolText(p,'warning'))+'</p>':'')+
 knownProfilesHtml(p.name)+
 '<div class="actions">'+(p.learn_category==='development'?'':'<button type="button" class="btn green small" id="selectedDecoderLearn">'+esc(rfTranslate('dyn.learn'))+'</button>')+'<button type="button" class="btn out small" id="selectedDecoderAdvanced">'+esc(rfTranslate('library.advanced'))+'</button></div>'+
 '<div class="advanced-panel" id="selectedDecoderAdvancedPanel" hidden>'+transportHint+'<p class="muted small">'+esc(rfTranslate('dyn.software_prefix'))+(registered?esc(rfTranslate('dyn.encoder_registered'))+esc(p.tx_status==='hardware_verified'?rfTranslate('dyn.manifest_verified'):rfTranslate('dyn.host_only')):esc(rfTranslate('dyn.no_generic_encoder'))) + '. '+esc(rfTranslate('dyn.only_approved'))+'</p><div class="advanced-kv"><span>'+esc(rfTranslate('library.source'))+' <button type="button" class="help-btn" data-help="decoder_source">?</button></span><b>'+esc(rfTranslate(p.source==='builtin'?'library.source_builtin':'library.source_user'))+'</b><span>'+esc(rfTranslate('library.stable_id'))+' <button type="button" class="help-btn" data-help="stable_id">?</button></span><b class="mono">'+esc(p.stable_id||('decoder:'+p.name))+'</b><span>'+esc(rfTranslate('library.version'))+'</span><b>'+esc(p.library_version||1)+'</b><span>'+esc(rfTranslate('library.pack'))+'</span><b class="mono">'+esc(p.pack)+'</b><span>'+esc(rfTranslate('dyn.actions'))+'</span><b class="mono">'+esc(keys(p.rx_actions).join(', ')||rfTranslate('dyn.none'))+'</b><span>'+esc(rfTranslate('dyn.fields'))+'</span><b class="mono">'+esc(keys(p.fields).join(', ')||rfTranslate('dyn.none'))+'</b><span>TX</span><b class="mono">'+esc(keys(p.tx_actions).join(', ')||rfTranslate('dyn.none'))+'</b></div></div>';
 const related=$('#openProtocolProfiles');if(related)related.onclick=()=>openProfilesForProtocol(p.name);const learn=$('#selectedDecoderLearn');if(learn)learn.onclick=()=>openDecoderLearn(p);const adv=$('#selectedDecoderAdvanced');if(adv)adv.onclick=()=>{$('#selectedDecoderAdvancedPanel').hidden=!$('#selectedDecoderAdvancedPanel').hidden;adv.textContent=rfTranslate($('#selectedDecoderAdvancedPanel').hidden?'library.advanced':'library.hide_advanced')};bindHelpButtons();
}

function openDecoderLearn(proto){
 if(!proto||proto.learn_category==='development')return;
 setLearnMode('device');show('learn');$('#learnProtocolGuide').innerHTML=instructions({protocol:proto.name})+'<p class="small">'+esc(rfTranslate('dyn.protocol_detection_note'))+'</p>';$('#learnProtocolGuide').hidden=false;
 if(latestStatus&&rfProtocolText(proto,'band')!=='433 MHz'){
  const prefix=rfProtocolText(proto,'band').replace(',','.').split(' ')[0];const match=(latestStatus.channels||[]).find(r=>r.frequency_label.startsWith(prefix)&&r.checked&&r.available&&r.rx_ready);
  if(match&&wizardStage==='idle'){selectedChannel=match.index;renderLearnRadios(latestStatus.channels)}
 }
}
let catalogMode='protocol', selectedProfileId='',profileCatalogBrand='all',profileCatalogFamily='all',profileCatalogProtocol='all';
function allCatalogProfiles(){return (window.rfUserLibraryCache?.profiles||[]).filter(p=>p&&p.id&&p.protocol).sort((a,b)=>String(a.manufacturer||'').localeCompare(String(b.manufacturer||''))||String(a.family||'').localeCompare(String(b.family||''))||String(a.name||'').localeCompare(String(b.name||'')))}
function catalogBrand(p){return p.manufacturer||rfTranslate('catalog.custom_brand')}
function catalogFamily(p){return p.family||rfTranslate('catalog.custom_family')}
function refreshProfileCatalogFilters(profiles){
 const choices=(id,values,selected,anyLabel)=>{const element=$(id),last=element.value;const all=[...new Set(values.filter(Boolean))].sort((a,b)=>a.localeCompare(b));if(selected!=='all'&&!all.includes(selected))selected='all';element.innerHTML='<option value="all">'+esc(anyLabel)+'</option>'+all.map(v=>'<option value="'+esc(v)+'">'+esc(v)+'</option>').join('');element.value=selected;return selected};
 profileCatalogBrand=choices('#profileCatalogBrand',profiles.map(catalogBrand),profileCatalogBrand,rfTranslate('catalog.all_brands'));
 const brandItems=profiles.filter(p=>profileCatalogBrand==='all'||catalogBrand(p)===profileCatalogBrand);
 profileCatalogFamily=choices('#profileCatalogFamily',brandItems.map(catalogFamily),profileCatalogFamily,rfTranslate('catalog.all_families'));
 const familyItems=brandItems.filter(p=>profileCatalogFamily==='all'||catalogFamily(p)===profileCatalogFamily);
 profileCatalogProtocol=choices('#profileCatalogProtocol',familyItems.map(p=>p.protocol),profileCatalogProtocol,rfTranslate('catalog.all_protocols'));
}
function renderProfileDetail(profile){
 const detail=$('#decoderDetail');detail.hidden=false;
 if(!profile){detail.innerHTML='<p class="muted">'+esc(rfTranslate('catalog.select_profile'))+'</p>';return}
 const protocol=getProtocol(profile.protocol),models=Array.isArray(profile.models)?profile.models:[],controls=Array.isArray(profile.controls)?profile.controls:[],steps=profile.setup_steps_i18n?.[rfLanguage]||profile.setup_steps_i18n?.en||[],actions=profile.expected_actions||[];
 const help=profileText(profile,'learn_help'),notes=profileText(profile,'notes');
 const availability=protocol?decoderTxStatus(protocol):null;
 const hasControls=controls.length>0,txReady=Boolean(protocol&&protocol.tx_registered&&protocol.tx_status==='hardware_verified');
 let html='<h3>'+esc(profile.name)+'</h3><div class="profile-breadcrumb"><b>'+esc(catalogBrand(profile))+'</b> · '+esc(catalogFamily(profile))+' · '+esc(rfTranslate('catalog.protocol'))+': <button type="button" id="profileOpenProtocol">'+esc(profile.protocol)+'</button></div>';
 if(models.length)html+='<div class="small"><b>'+esc(rfTranslate('decoder.models'))+':</b> '+esc(models.join(' · '))+'</div>';
 html+='<div class="decoder-info-grid"><div class="decoder-info-block"><b>'+esc(rfTranslate('decoder.how_to_learn'))+'</b><span class="small">'+esc(help||rfTranslate('catalog.no_help'))+'</span></div><div class="decoder-info-block"><b>'+esc(rfTranslate('catalog.operation'))+'</b><span class="small">'+esc(hasControls?(txReady?rfTranslate('catalog.tx_profile_warning'):rfTranslate('catalog.tx_not_ready')):rfTranslate('catalog.rx_only'))+'</span></div></div>';
 if(hasControls)html+='<b>'+esc(rfTranslate('catalog.profile_functions'))+'</b><div class="profile-controls">'+controls.map(a=>'<span>'+esc(actionLabel(protocol,a))+'</span>').join('')+'</div>';
 if(!hasControls&&actions.length)html+='<div class="small"><b>'+esc(rfTranslate('catalog.received_actions'))+':</b> '+esc(actions.map(a=>actionLabel(protocol,a)).join(' · '))+'</div>';
 if(steps.length)html+='<details class="compat-details"><summary>'+esc(rfTranslate('catalog.setup_steps'))+' ('+steps.length+')</summary><ol class="small">'+steps.map(a=>'<li>'+esc(a)+'</li>').join('')+'</ol></details>';
 if(notes||profile.sources?.length){const sources=(profile.sources||[]).filter(x=>typeof x.url==='string'&&x.url.startsWith('https://')).map(x=>'<a href="'+esc(x.url)+'" rel="noopener noreferrer" target="_blank">'+esc(x.label||x.url)+'</a>').join(' · ');html+='<details class="compat-details"><summary>'+esc(rfTranslate('decoder.compatibility_details'))+'</summary>'+(notes?'<p class="small muted">'+esc(notes)+'</p>':'')+(sources?'<div class="profile-sources">'+sources+'</div>':'')+'</details>';}
 html+='<div class="small muted" style="margin-top:12px">'+esc(rfTranslate('catalog.profile_source'))+': '+esc(profile.source==='builtin'?rfTranslate('library.source_builtin'):rfTranslate('library.source_user'))+' · '+esc(rfTranslate('catalog.profile_protocol_note'))+'</div>';
 detail.innerHTML=html;
 const link=$('#profileOpenProtocol');if(link)link.onclick=()=>{catalogMode='protocol';selectedDecoderName=profile.protocol;decoderFilterValue='all';renderDecoderLibrary()};
}
function renderProfileBrowser(){
 const all=allCatalogProfiles();refreshProfileCatalogFilters(all);
 const query=$('#profileCatalogSearch').value.trim().toLocaleLowerCase();
 const matches=all.filter(p=>(profileCatalogBrand==='all'||catalogBrand(p)===profileCatalogBrand)&&(profileCatalogFamily==='all'||catalogFamily(p)===profileCatalogFamily)&&(profileCatalogProtocol==='all'||p.protocol===profileCatalogProtocol)&&(!query||[p.name,p.manufacturer,p.family,p.protocol,p.id,...(p.models||[])].join(' ').toLocaleLowerCase().includes(query)));
 const orconCount=all.filter(p=>p.manufacturer==='Orcon').length;
 $('#decoderCount').textContent=rfFormat('catalog.profile_count',{shown:matches.length,total:all.length})+(orconCount?' · Orcon: '+orconCount:'');
 const groups=new Map();for(const p of matches){const key=catalogBrand(p)+' · '+catalogFamily(p);if(!groups.has(key))groups.set(key,[]);groups.get(key).push(p)}
 if(!matches.some(p=>p.id===selectedProfileId))selectedProfileId=matches.length&& !window.matchMedia('(max-width:850px)').matches?matches[0].id:'';
 $('#decoderSelector').innerHTML=[...groups].map(([family,items])=>'<div class="profile-selector-group">'+esc(family)+' ('+items.length+')</div>'+items.map(p=>'<button type="button" class="decoder-select '+(selectedProfileId===p.id?'active':'')+'" data-catalog-profile="'+esc(p.id)+'"><b>'+esc(p.name)+'</b><span class="muted small">'+esc(rfTranslate('catalog.protocol'))+': '+esc(p.protocol)+'</span></button>').join('')).join('')||'<span class="muted">'+esc(!all.length&&rfProfileCatalogStatus!=='ready'?'Apparaatprofielen konden niet worden geladen. Probeer opnieuw.':rfTranslate('catalog.no_results'))+'</span>';
 $$('[data-catalog-profile]').forEach(b=>b.onclick=async()=>{selectedProfileId=b.dataset.catalogProfile;renderProfileBrowser();await loadProfileDetail(selectedProfileId);if(catalogMode==='profile')renderProfileBrowser()});
 renderProfileDetail(all.find(p=>p.id===selectedProfileId&&matches.includes(p)));
}
function openProfilesForProtocol(protocol){catalogMode='profile';profileCatalogBrand='all';profileCatalogFamily='all';profileCatalogProtocol=protocol;$('#profileCatalogSearch').value='';renderDecoderLibrary()}
$$('[data-catalog-mode]').forEach(b=>b.onclick=()=>{catalogMode=b.dataset.catalogMode;profileCatalogBrand='all';profileCatalogFamily='all';profileCatalogProtocol='all';$('#profileCatalogSearch').value='';renderDecoderLibrary()});
$('#profileCatalogSearch').oninput=()=>renderProfileBrowser();
for(const [id,key] of [['#profileCatalogBrand','brand'],['#profileCatalogFamily','family'],['#profileCatalogProtocol','protocol']]){$(id).onchange=()=>{if(key==='brand'){profileCatalogBrand=$(id).value;profileCatalogFamily='all';profileCatalogProtocol='all'}else if(key==='family'){profileCatalogFamily=$(id).value;profileCatalogProtocol='all'}else profileCatalogProtocol=$(id).value;renderProfileBrowser()}}
function renderDecoderLibrary(){
 const browsingProfiles=catalogMode==='profile';
 $('#decoderFilters').hidden=browsingProfiles;
 $('#profileBrowserTools').hidden=!browsingProfiles;
 $$('[data-catalog-mode]').forEach(b=>{b.classList.toggle('active',b.dataset.catalogMode===catalogMode);b.setAttribute('aria-pressed',b.dataset.catalogMode===catalogMode?'true':'false')});
 if(browsingProfiles){renderProfileBrowser();return}
 const entries=decoderEntries();
 $('#decoderCount').textContent=rfFormat('dyn.protocol_count',{n:entries.length});
 if(!entries.some(p=>p.name===selectedDecoderName)){const compact=window.matchMedia('(max-width:850px)').matches&&decoderFilterValue==='all';selectedDecoderName=entries.length&&!compact?entries[0].name:'';}
 $('#decoderSelector').innerHTML=entries.map(p=>{const tx=decoderTxStatus(p);return '<button type="button" class="decoder-select '+(p.name===selectedDecoderName?'active':'')+'" data-decoder-select="'+esc(p.name)+'"><b>'+esc(p.name)+'</b><span class="muted small">'+esc(rfProtocolText(p,'band'))+' · '+esc(tx.label)+'</span></button>'}).join('')||'<span class="muted">'+esc(rfTranslate('dyn.no_protocols'))+'</span>';
 $$('[data-decoder-select]').forEach(b=>b.onclick=()=>{selectedDecoderName=b.dataset.decoderSelect;renderDecoderLibrary()});
 renderDecoderDetail(selectedDecoderName?decoderProtocols.get(selectedDecoderName):null);
 $$('[data-decoder-filter]').forEach(b=>b.classList.toggle('active',b.dataset.decoderFilter===decoderFilterValue));
}
async function loadDecoders(){try{
 const d=await get('/rflink/api/decoders');
 decoderProtocols=new Map((d.protocols||[]).map(p=>[p.name,p]));haProfiles=new Map((d.ha_fields||[]).map(f=>[f.key,rfFieldLabel(f.key,f.label)]));
 renderDecoderLibrary();
 }catch(e){$('#decoderSelector').textContent=rfTranslate('dyn.decoder_registry_unavailable');$('#decoderDetail').textContent=rfTranslate('dyn.decoder_library_unavailable')}}
$$('[data-decoder-filter]').forEach(b=>b.onclick=()=>{decoderFilterValue=b.dataset.decoderFilter;renderDecoderLibrary()});

$('#monitorDirection').onchange=()=>{monitorDirection=$('#monitorDirection').value;if(window.rfLogbookRefreshRadio)window.rfLogbookRefreshRadio()};
window.rfLogbookRadioFilters=()=>({channel:monitorChannel,type:monitorLogType,direction:monitorDirection});
$('#monitorLogType').onchange=()=>{monitorLogType=$('#monitorLogType').value;if(window.rfLogbookRefreshRadio)window.rfLogbookRefreshRadio()};
$('#monitorCopy').onclick=copyMonitor;$('#monitorExport').onclick=exportAllRxLogs;
$('#rxLogOlder').onclick=()=>fetchRxLogPage(true);$('#rxLogSave').onclick=saveRxLogSettings;
async function initializeLanguage() {
  try {
    const result = await get('/rflink/api/language');
    rfApplyLanguage(result.language);
    try { localStorage.setItem('rflink.ui.language', rfLanguage); } catch (ignored) {}
  } catch (e) {
    // Offline display only; the device is always authoritative when reachable.
    try { rfApplyLanguage(localStorage.getItem('rflink.ui.language') || 'en'); }
    catch (ignored) { rfApplyLanguage('en'); }
  }
}
$('#uiLanguage').onchange = async () => {
  const selected = $('#uiLanguage').value;
  const prior = rfLanguage;
  rfApplyLanguage(selected);
  $('#languageSaveStatus').textContent = rfTranslate('language.saving');
  try {
    const response = await post('/rflink/api/language?language=' + encodeURIComponent(selected));
    rfApplyLanguage(response.language);
    try { localStorage.setItem('rflink.ui.language', rfLanguage); } catch (ignored) {}
    $('#languageSaveStatus').textContent = rfTranslate('language.saved');
    // Refresh server-derived panels. Never discard unsaved names or RF learning.
    loadStatus();
    if (page === 'devices') loadDevices();
    if (page === 'settings'){loadDecoders();loadLibrary();}
  } catch (e) {
    rfApplyLanguage(prior);
    $('#languageSaveStatus').textContent = rfTranslate('language.failed');
  }
};

let decoderCompactMode=window.matchMedia('(max-width:850px)').matches;window.addEventListener('resize',()=>{const compact=window.matchMedia('(max-width:850px)').matches;if(compact!==decoderCompactMode){decoderCompactMode=compact;if(decoderProtocols.size)renderDecoderLibrary()}});
document.addEventListener('rflink-user-library-changed',()=>{if(decoderProtocols.size)renderDecoderLibrary()});
document.addEventListener('rflink-language-changed',()=>{
 setLearnMode(learnMode);
 if(latestStatus){renderHealth(latestStatus);renderLedStatus(latestStatus.led);renderLearnRadios(latestStatus.channels||[]);if(page==='monitor')renderMonitor(latestStatus);}
 if(lastDeviceView||page==='devices')loadDevices();
 if(decoderProtocols.size)renderDecoderLibrary();
 if(libraryState)renderLibrary(libraryState);
 if(page==='learn')loadDeveloper();
});
rfApplyLanguage('en');
initializeLanguage();
navRestore();
window.rfLogbookLoadRadio=()=>fetchRxLogPage();
setInterval(()=>{loadStatus();if(page==='devices')loadDevices();if(page==='learn'){loadDeveloper();pollWizard()}},3000);setLearnMode('device');loadStatus();loadProfileCatalog();loadDevices();setupLearnRadios();loadDecoders();loadLibrary()})();
// User recipe controls intentionally use POST bodies: programs and recorded RF
// frames do not belong in URLs, browser history or access logs.
(()=>{
 const $=s=>document.querySelector(s), esc=s=>String(s??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
 const tr=k=>rfTranslate(k), setResult=t=>{$('#userResult').textContent=t};
 const slug=s=>String(s||'').trim().toLowerCase().normalize('NFKD').replace(/[^a-z0-9]+/g,'_').replace(/^_+|_+$/g,'').slice(0,32);
 let library={decoders:[],profiles:[]}, devices={devices:[]};
 async function api(path,data){const response=await rfTrackedFetch('/rflink/api/'+path,{method:'POST',body:new URLSearchParams({...data,write_token:library.write_token||''})});const value=await response.json();if(!response.ok||!value.ok)throw Error(value.error||response.statusText);return value}
 async function fetchDecoderDetail(decoder){
  if(!decoder)return decoder;
  const url='/rflink/api/user-decoder-detail?id='+encodeURIComponent(decoder.id);
  const response=await rfTrackedFetch(url);
  if(!response.ok)throw Error('Decoder detail HTTP '+response.status);
  const detail=await response.json();
  const samples=[];
  for(let i=0;i<(detail.sample_count||0);i++){
    const next=await rfTrackedFetch(url+'&sample='+i);
    if(!next.ok)throw Error('Decoder sample '+i+' HTTP '+next.status);
    const piece=await next.json();
    if(!Array.isArray(piece.samples)||piece.samples.length!==1)throw Error('Invalid decoder sample '+i);
    samples.push(piece.samples[0]);
  }
  return {...decoder,...detail,samples};
 }
 async function refresh(){try{const response=await rfTrackedFetch('/rflink/api/user-library');if(!response.ok)throw Error('user-library HTTP '+response.status);const next=await response.json();if(!Array.isArray(next.decoders)||!Array.isArray(next.profiles))throw Error('Invalid user-library response');library=next;window.rfUserLibraryCache={...library,profiles:[...new Map([...(library.profiles||[]).map(p=>[p.id,p]),...(window.rfUserLibraryCache?.profiles||[]).filter(p=>p.source==='builtin').map(p=>[p.id,{...(library.profiles||[]).find(d=>d.id===p.id),...p}])]).values()]};renderLibrary();document.dispatchEvent(new CustomEvent('rflink-user-library-changed'));}catch(e){$('#userDecoderList').textContent=tr('user.unavailable')}}
 function renderCaptureAnalysis(samples){
  const box=$('#userAnalysis');if(!box)return;
  if(!samples||samples.length<2){box.textContent=tr('user.analysis_waiting');return}
  const groups={};for(const sample of samples)(groups[sample.label]??=[]).push(sample);
  const lines=[];
  for(const [label,group] of Object.entries(groups)){
   if(group.length<2){lines.push(rfFormat('user.analysis_need_more',{action:label}));continue}
   const packet=group[0].kind==='packet',base=packet?group[0].packet:group[0].raw,count=base.length,vary=[];
   if(group.some(x=>(packet?x.packet:x.raw).length!==count)){lines.push(rfFormat('user.analysis_length',{action:label}));continue}
   for(let i=0;i<count;i++){
    const first=base[i];
    const changed=group.slice(1).some(x=>{const value=(packet?x.packet:x.raw)[i];if(packet)return value!==first;const tolerance=Math.max(100,Math.abs(first)/4);return Math.sign(value)!==Math.sign(first)||Math.abs(value-first)>tolerance});
    if(changed)vary.push(i);
   }
   lines.push(vary.length?rfFormat('user.analysis_dynamic',{action:label,count,positions:vary.slice(0,16).join(', ')+(vary.length>16?' …':'')}):rfFormat('user.analysis_fixed',{action:label,count}));
   if(packet&&group.length>=3&&vary.length){
    const counter=vary.filter(i=>group.slice(1).every((x,n)=>x.packet[i]===((group[n].packet[i]+1)&255)));
    if(counter.length)lines.push(rfFormat('user.analysis_counter',{action:label,positions:counter.join(', ')}));
    const checksum=[];
    for(const i of vary){
     const xorOk=group.every(x=>x.packet[i]===x.packet.reduce((v,b,j)=>j===i?v:(v^b),0));
     const sumOk=group.every(x=>x.packet[i]===x.packet.reduce((v,b,j)=>j===i?v:((v+b)&255),0));
     if(xorOk||sumOk)checksum.push(i+(xorOk?' XOR':' SUM'));
    }
    if(checksum.length)lines.push(rfFormat('user.analysis_checksum',{action:label,positions:checksum.join(', ')}));
   }
  }
  box.textContent=lines.join('\n');box.style.whiteSpace='pre-wrap';
 }
 function renderLibrary(){
  const locals=library.decoders.filter(d=>d.source!=='firmware');
  $('#userDecoderList').innerHTML=locals.length?locals.map(d=>`<div class="notice"><b>${esc(d.name)}</b> <span class="mono small">${esc(d.id)} · v${d.version}</span><p class="small">${esc(d.actions.join(', '))} · ${d.sample_count??d.samples?.length??0} ${esc(tr('user.samples'))} · RX ${esc(d.enabled?tr('user.active'):tr('user.inactive'))} · TX ${esc(d.tx_source?tr('user.preview_ready'):tr('user.rx_only'))} · ${esc(d.radio_profile||'board')}</p><div class="actions"><button class="btn out small" data-user-edit="${esc(d.id)}">${esc(tr('user.edit'))}</button><button class="btn out small" data-user-enable="${esc(d.id)}" data-state="${d.enabled?'disable':'enable'}">${esc(tr(d.enabled?'user.disable':'user.activate'))}</button><button class="btn soft small" data-user-remove="${esc(d.id)}">${esc(tr('user.remove'))}</button></div></div>`).join(''):`<p class="muted">${esc(tr('user.empty'))}</p>`;
  const protocolOptions=[...(library.protocols||[]),...library.decoders.map(d=>({id:d.id,name:d.name,source:d.source,actions:d.actions}))];
  const seen=new Set();$('#userProfileDecoder').innerHTML=protocolOptions.filter(p=>p.id&&!seen.has(p.id)&&seen.add(p.id)).map(p=>`<option value="${esc(p.id)}">${esc(p.name||p.id)}${p.source==='builtin'?' · builtin':''}</option>`).join('');
  $('#userProfileDecoder').onchange=()=>{const id=$('#userProfileDecoder').value,p=protocolOptions.find(x=>x.id===id),fixed=(p?.actions||[]).filter(a=>!a.includes('*'));if(fixed.length)$('#userProfileActions').value=fixed.join(',')};
  $('#userProfileList').innerHTML=library.profiles.filter(p=>p.source==='user').map(p=>`<div class="row"><div><b>${esc(p.name)}</b> <span class="mono small">${esc(p.id)}</span><p class="small muted">${esc(p.protocol)} · ${esc(p.expected_actions.join(', '))}</p></div><span><button class="btn out small" data-profile-edit="${esc(p.id)}">Bewerken</button> <button class="btn out small" data-profile-remove="${esc(p.id)}">${esc(tr('user.remove'))}</button></span></div>`).join('');
  document.querySelectorAll('[data-user-edit]').forEach(b=>b.onclick=async()=>{let d=library.decoders.find(x=>x.id===b.dataset.userEdit);if(!d)return;try{d=await fetchDecoderDetail(d)}catch(e){setResult(e.message);return} document.querySelector('[data-learn="decoder"]').click();document.querySelector('[data-page="learn"]').click();$('#userDecoderName').value=d.name;$('#userDecoderId').value=d.id;$('#userActions').value=d.actions.join(',');$('#userRxSource').value=d.rx_source;$('#userTxSource').value=d.tx_source;$('#userRadioProfile').textContent=d.radio_profile||'board';renderCaptureAnalysis(d.samples);$('#userAdvanced').open=true;$('#userAdvanced').scrollIntoView({block:'center'})});
  document.querySelectorAll('[data-user-enable]').forEach(b=>b.onclick=async()=>{try{await api('user-library',{action:b.dataset.state,id:b.dataset.userEnable});setResult(tr(b.dataset.state==='enable'?'user.activated':'user.disabled'));await refresh()}catch(e){setResult(e.message)}});
  document.querySelectorAll('[data-user-remove]').forEach(b=>b.onclick=async()=>{if(!confirm(tr('user.confirm_remove')))return;try{await api('user-library',{action:'remove_decoder',id:b.dataset.userRemove});await refresh()}catch(e){setResult(e.message)}});
  document.querySelectorAll('[data-profile-edit]').forEach(b=>b.onclick=()=>{
    const p=library.profiles.find(x=>x.id===b.dataset.profileEdit);if(!p)return;
    $('#userProfileId').value=p.id;$('#userProfileName').value=p.name;
    $('#userProfileDecoder').value=p.protocol;$('#userProfileHelp').value=p.learn_help||'';
    $('#userProfileActions').value=(p.expected_actions||[]).join(',');
    $('#userProfileControls').value=(p.controls||[]).join(',');
    $('#userProfilePair').value=p.pair_method||'none';
    $('#userProfileSteps').value=(p.setup_steps_i18n?.[rfLanguage]||p.setup_steps_i18n?.en||[]).join('\n');
    $('#userProfileId').scrollIntoView({block:'center'});
  });
  document.querySelectorAll('[data-profile-remove]').forEach(b=>b.onclick=async()=>{if(!confirm(tr('user.confirm_remove')))return;try{await api('user-library',{action:'remove_profile',id:b.dataset.profileRemove});await refresh()}catch(e){setResult(e.message)}});
 }
 function activeRadio(){const e=document.querySelector('#learnRadios [data-ch].active');return e&&!e.disabled?Number(e.dataset.ch):-1}
 async function updateCaptures(){if(!$('#userCaptureCount').offsetParent)return;try{const d=await(await rfTrackedFetch('/rflink/api/developer')).json();const counts={};for(const sample of d.captures||[])counts[sample.label]=(counts[sample.label]||0)+1;$('#userCaptureCount').textContent=Object.entries(counts).map(([label,n])=>label+': '+n+'/3').join(' · ')||tr('user.no_samples');renderCaptureAnalysis(d.captures||[]);const radio=activeRadio();if(radio>=0)$('#userRadioProfile').textContent=radio===2?'board_packet_63':'board_raw'}catch(e){}}
 $('#userCapture').onclick=async()=>{const radio=activeRadio(),action=$('#userAction').value.trim();if(radio<0||!/^[a-z][a-z0-9_]{0,31}$/.test(action)){setResult(tr('user.check_radio_action'));return}try{await api('developer',{action:'listen',channel:radio,label:action});setResult(tr('user.press_three'));updateCaptures()}catch(e){setResult(e.message)}};
 $('#userClear').onclick=async()=>{try{await api('developer',{action:'reset'});await updateCaptures();setResult(tr('user.cleared'))}catch(e){setResult(e.message)}};
 $('#userCreate').onclick=async()=>{const name=$('#userDecoderName').value.trim();let id=$('#userDecoderId').value.trim();if(!id&&name){const base=slug(name).replace(/^user_?/,'').slice(0,27);id='user_'+(base||'decoder');$('#userDecoderId').value=id}if(!/^user_[a-z0-9_]{1,27}$/.test(id)||!name){setResult(tr('user.check_id_name'));return}try{await api('user-library',{action:'create',id,name});await api('user-library',{action:'enable',id});await refresh();const d=await fetchDecoderDetail(library.decoders.find(x=>x.id===id));$('#userActions').value=d.actions.join(',');$('#userRxSource').value=d.rx_source;$('#userTxSource').value=d.tx_source;$('#userRadioProfile').textContent=d.radio_profile||'board';renderCaptureAnalysis(d.samples);setResult(tr(d.tx_source?'user.created_tx':'user.created_rx'))}catch(e){setResult(e.message)}};
 $('#userSaveAdvanced').onclick=async()=>{const id=$('#userDecoderId').value.trim(),name=$('#userDecoderName').value.trim(),radio=activeRadio(),existing=library.decoders.find(d=>d.id===id);if(radio<0&&!existing){setResult(tr('user.check_radio_action'));return}try{await api('user-library',{action:'save_decoder',id,name,radio:existing?existing.radio:radio,kind:existing?existing.kind:radio===2?'packet':'raw',actions:$('#userActions').value.split(',').map(x=>x.trim()).filter(Boolean).join(','),rx_source:$('#userRxSource').value,tx_source:$('#userTxSource').value});if(!existing)await api('user-library',{action:'enable',id});await refresh();setResult(tr('user.advanced_saved'))}catch(e){setResult(e.message)}};
 $('#userSaveProfile').onclick=async()=>{try{const name=$('#userProfileName').value.trim();let id=$('#userProfileId').value.trim();if(!id){id=slug(name);$('#userProfileId').value=id}const protocol=$('#userProfileDecoder').value;let actions=$('#userProfileActions').value.split(',').map(x=>x.trim()).filter(Boolean);if(!actions.length){const p=(library.protocols||[]).concat(library.decoders||[]).find(x=>(x.id||x.name)===protocol);actions=(p?.actions||[]).filter(a=>!String(a).includes('*'));$('#userProfileActions').value=actions.join(',')}await api('user-library',{action:'save_profile',id,name,protocol,actions:actions.join(','),learn_help:$('#userProfileHelp').value.trim(),controls:$('#userProfileControls').value.trim(),pair_method:$('#userProfilePair').value,setup_steps:$('#userProfileSteps').value.trim()});setResult(tr('user.profile_saved'));await refresh();await refreshDevices()}catch(e){setResult(e.message)}};
 async function exportBackup(){const current=await(await rfTrackedFetch('/rflink/api/user-library')).json(),saved=await(await rfTrackedFetch('/rflink/api/devices')).json();for(let i=0;i<(current.decoders||[]).length;i++){const d=current.decoders[i];if(d.source==='user'||!d.source)current.decoders[i]=await fetchDecoderDetail(d)}const {write_token,...exportedLibrary}=current;const backup={format:'rflink-user-library',schema:1,exported_at:new Date().toISOString(),library:exportedLibrary,devices:saved.devices};const file=new Blob([JSON.stringify(backup,null,2)],{type:'application/json'}),link=document.createElement('a');link.href=URL.createObjectURL(file);link.download='rflink-user-library-export.json';link.click();setTimeout(()=>URL.revokeObjectURL(link.href),1000);setResult(tr('user.exported'))}
 $('#userExport').onclick=async()=>{try{await exportBackup()}catch(e){setResult(e.message)}};
 async function restoreBackup(backup){
  if(!backup||backup.format!=='rflink-user-library'||backup.schema!==1||backup.library?.schema!==1||!Array.isArray(backup.library?.decoders)||!Array.isArray(backup.library?.profiles))throw Error(tr('user.import_invalid'));
  const decoders=backup.library.decoders.filter(d=>d.source==='user'||!d.source),profiles=backup.library.profiles.filter(p=>p.source==='user'||!p.source);
  if(!confirm(rfFormat('user.import_confirm',{decoders:decoders.length,profiles:profiles.length})))return;
  for(const d of decoders){
   const data={action:'import_decoder',id:d.id,name:d.name,radio:String(d.radio),kind:d.kind,actions:(d.actions||[]).join(','),rx_source:d.rx_source||'',tx_source:d.tx_source||'',sample_count:String((d.samples||[]).length)};
   (d.samples||[]).forEach((sample,n)=>{data[`s${n}_label`]=sample.label;data[`s${n}_channel`]=String(sample.channel);data[`s${n}_kind`]=sample.kind;data[`s${n}_raw`]=(sample.raw||[]).join(',');data[`s${n}_packet`]=(sample.packet||[]).join(',')});
   await api('user-library',data);await api('user-library',{action:d.enabled?'enable':'disable',id:d.id});
  }
  for(const p of profiles)await api('user-library',{action:'save_profile',id:p.id,name:p.name,protocol:p.protocol,actions:(p.expected_actions||[]).join(','),learn_help:p.learn_help||'',controls:(p.controls||[]).join(','),pair_method:p.pair_method||'none',setup_steps:(p.setup_steps_i18n?.nl||[]).join('\n')});
  for(const [key,value] of Object.entries(backup.library.state||{}))if(Number.isInteger(value)&&value>0)await api('user-library',{action:'restore_state',id:key,value:String(value)});
  await refresh();await refreshDevices();
  const currentByUid=new Map((devices.devices||[]).map(d=>[d.uid,d]));let bindings=0;
  for(const saved of backup.devices||[]){
   const current=currentByUid.get(saved.uid);if(!current||current.protocol!==saved.protocol)continue;
   if(saved.protocol==='SomfyRTS56'&&saved.somfy?.configured){await api('device',{action:'somfy_restore',uid:saved.uid,address:String(saved.somfy.address),rolling:String(saved.somfy.next_rolling_code),paired:saved.somfy.paired?'1':'0'})}
   if(saved.protocol==='IthoRFT'&&saved.itho?.configured){await api('device',{action:'itho_restore',uid:saved.uid,address:String(saved.itho.address),sequence:String(saved.itho.next_sequence),device_type:String(saved.itho.device_type),paired:saved.itho.paired?'1':'0'})}
   if(saved.profile_id){try{await api('user-tx',{action:'profile',uid:saved.uid,id:saved.profile_id});bindings++}catch(e){}}
  }
  await refreshDevices();setResult(rfFormat('user.imported',{decoders:decoders.length,profiles:profiles.length,bindings}));
 }
 $('#userImport').onclick=()=>$('#userImportFile').click();
 $('#userImportFile').onchange=async e=>{const file=e.target.files?.[0];e.target.value='';if(!file)return;try{await restoreBackup(JSON.parse(await file.text()))}catch(err){setResult(err.message||tr('user.import_invalid'))}};
 function deviceControls(){
  const list=$('#deviceList');
  for(const card of list.querySelectorAll('.card.device')){
   const uid=card.querySelector('[data-rename]')?.dataset.rename;if(!uid||card.querySelector('[data-local-tx]'))continue;
   const d=devices.devices.find(x=>x.uid===uid);if(!d)continue;
   const decoder=library.decoders.find(x=>x.id===d.protocol),profiles=library.profiles.filter(p=>p.protocol===d.protocol);
   if(!decoder&&!profiles.length)continue;
   const parent=card.querySelector('.device-more')||card,section=document.createElement('div');section.setAttribute('data-local-tx',uid);section.className='notice';
   const tx=decoder?.tx_source?`<p class="small muted">${esc(tr('user.tx_notice'))}</p><div class="actions"><button class="btn out small" data-toggle-local>${esc(tr(d.local_tx_enabled?'user.revoke_tx':'user.allow_tx'))}</button><select class="field" data-select-action>${(d.actions||[]).filter(a=>decoder.actions.includes(a)).map(a=>`<option value="${esc(a)}">${esc(a)}</option>`).join('')}</select><button class="btn out small" data-preview-tx>${esc(tr('user.preview'))}</button><button class="btn green small" data-send-tx ${d.local_tx_enabled?'':'disabled'}>${esc(tr('user.send'))}</button></div><pre class="mono small" data-tx-result style="white-space:pre-wrap;overflow-wrap:anywhere"></pre>`:(decoder?`<p class="muted small">${esc(tr('user.rx_only'))}</p>`:'');
   if(!decoder?.tx_source)continue; // RX-only profiles are configured by the main device card.
   section.innerHTML=`<details class="decoder-tech"><summary>${esc(tr('user.device_controls'))} (Advanced)</summary>${tx}</details>`;
   parent.appendChild(section);
   const toggle=section.querySelector('[data-toggle-local]');if(toggle)toggle.onclick=async()=>{if(!d.local_tx_enabled&&!confirm(tr('user.confirm_allow')))return;try{await api('user-tx',{action:d.local_tx_enabled?'revoke':'approve',uid});d.local_tx_enabled=!d.local_tx_enabled;toggle.textContent=tr(d.local_tx_enabled?'user.revoke_tx':'user.allow_tx');section.querySelector('[data-send-tx]').disabled=!d.local_tx_enabled}catch(err){section.querySelector('[data-tx-result]').textContent=err.message}};
   const preview=section.querySelector('[data-preview-tx]');if(preview)preview.onclick=async()=>{try{const frame=await api('user-tx',{action:'preview',id:decoder.id,uid,command:section.querySelector('[data-select-action]').value});section.querySelector('[data-tx-result]').textContent=`radio ${frame.radio} × ${frame.repeats}\n${frame.raw.length?'RAW: '+frame.raw.join(' '):'PACKET: '+frame.packet.map(n=>n.toString(16).padStart(2,'0').toUpperCase()).join(' ')}${Object.keys(frame.next_state||{}).length?'\nSTATE: '+JSON.stringify(frame.next_state):''}`}catch(err){section.querySelector('[data-tx-result]').textContent=err.message}};
   const send=section.querySelector('[data-send-tx]');if(send)send.onclick=async()=>{const command=section.querySelector('[data-select-action]').value;if(!command||!confirm(tr('user.confirm_send')+' '+command+'?'))return;try{await api('user-tx',{action:'send',uid,command});section.querySelector('[data-tx-result]').textContent=tr('user.sent_request')}catch(err){section.querySelector('[data-tx-result]').textContent=err.message}};
  }
 }
 async function refreshDevices(){try{devices=await(await rfTrackedFetch('/rflink/api/devices')).json();deviceControls()}catch(e){}}
 const observer=new MutationObserver(()=>deviceControls());observer.observe($('#deviceList'),{childList:true});
 document.querySelectorAll('[data-settings-open="custom"]').forEach(b=>b.addEventListener('click',refresh));
 document.addEventListener('rflink-devices-rendered',e=>{devices=e.detail;deviceControls()});
 setInterval(()=>{updateCaptures()},4000);
 document.addEventListener('rflink-language-changed',()=>{if(library.decoders.length)renderLibrary()});
 // Compact library summaries are safe at startup; captures load one decoder at a time.
 refresh();
})();

// RFLink system trace is separate from the signal/noise activity log.
let systemTraceLast=null;
const systemTraceText=id=>rfTranslate('debug.'+id);
const systemTraceEscape=s=>String(s??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
let systemTraceLoadBusy=false;
async function refreshSystemTrace(){
 if(systemTraceLoadBusy)return;
 const panel=document.getElementById('settingsPanelSystemtrace');
 if(!panel||panel.hidden||!panel.closest('.page').classList.contains('active'))return;
 systemTraceLoadBusy=true;
 try{
  const response=await rfTrackedFetch('/rflink/api/system-trace',{cache:'no-store'});
  if(!response.ok)throw Error('HTTP '+response.status);
  systemTraceLast=await response.json();
  renderSystemTrace();
 }catch(e){document.getElementById('systemTraceCount').textContent=systemTraceText('fetch_failed')+' '+e.message;}finally{systemTraceLoadBusy=false;}
}
function renderSystemTrace(){
 if(!systemTraceLast)return;
 const d=systemTraceLast;
 const labels=['Itho RX','Acquiring','RAMSES RX','Restoring','Radio fault'];
 document.getElementById('traceRadioState').textContent=labels[d.radio_868_mode]||'?';
 document.getElementById('traceGdoLab').textContent=d.gdo_lab?systemTraceText('enabled'):systemTraceText('disabled');
 document.getElementById('traceProbeLab').textContent=d.probe_lab?systemTraceText('enabled'):systemTraceText('disabled');
 document.getElementById('diagFiveLabSummary').textContent=(d.gdo_lab&&d.probe_lab?rfTranslate('diag55.lab_enabled'):rfTranslate('diag55.lab_disabled'))+' · '+(d.adapter_ready?rfTranslate('diag55.adapter_ready'):rfTranslate('diag55.adapter_unready'));
 const health=document.getElementById('systemTraceHealth');
 health.textContent=d.sticky_fault?systemTraceText('fault'):(d.adapter_ready?systemTraceText('ready'):systemTraceText('not_ready'));
 const probe=document.getElementById('systemTraceProbe');probe.disabled=!d.probe_lab||!d.adapter_ready||d.sticky_fault;
 const choice=document.getElementById('systemTraceFilter').value;
 const events=(d.events||[]).filter(e=>choice==='all'||e.subsystem===choice);
 const count=document.getElementById('systemTraceCount');count.textContent=rfFormat('debug.count',{shown:events.length,total:(d.events||[]).length})+' · '+systemTraceText('dropped')+': '+(d.dropped||0);
 document.getElementById('systemTraceEvents').innerHTML=events.length?events.slice().reverse().map(e=>
  '<div class="row" style="align-items:flex-start;gap:12px"><span class="mono muted small" style="flex:0 0 86px">'+systemTraceEscape((e.ms/1000).toFixed(1))+' s</span><div style="min-width:0;flex:1"><b class="small">'+systemTraceEscape(e.subsystem)+' · '+systemTraceEscape(e.level)+'</b>'+(e.uid?' <span class="mono muted small">'+systemTraceEscape(e.uid)+'</span>':'')+'<div class="small" style="overflow-wrap:anywhere">'+systemTraceEscape(e.message)+'</div></div></div>'
 ).join(''):'<p class="muted small">'+systemTraceEscape(systemTraceText('empty'))+'</p>';
}
document.getElementById('systemTraceFilter').addEventListener('change',renderSystemTrace);
document.getElementById('systemTraceRefresh').onclick=refreshSystemTrace;
document.getElementById('openSystemLogbook').onclick=()=>{window.rfLogbookSelectView?.('system');document.getElementById('logbookEventType').value='tx';window.rfLogbookRenderBootHistory?.();};
document.getElementById('systemTraceExport').onclick=()=>{
 if(!systemTraceLast)return;
 const data={exported_at:new Date().toISOString(),firmware_revision:document.getElementById('firmwareRevision').textContent,system_trace:systemTraceLast};
 const url=URL.createObjectURL(new Blob([JSON.stringify(data,null,2)],{type:'application/json'}));
 const a=document.createElement('a');a.href=url;a.download='rflink-system-trace.json';a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);
};
document.getElementById('systemTraceProbe').onclick=async()=>{
 if(!confirm(systemTraceText('confirm')))return;
 const button=document.getElementById('systemTraceProbe');button.disabled=true;
 const status=document.getElementById('systemTraceProbeStatus');status.textContent=systemTraceText('sending');
 try{
  const r=await rfTrackedFetch('/rflink/api/lab-tx-probe?confirm=LAB_10E0_ONE_SHOT',{method:'POST'});
  const data=await r.json();
  if(!r.ok||!data.ok)throw Error(data.error||('HTTP '+r.status));
  status.textContent=systemTraceText('sent_unverified');
 }catch(e){status.textContent=systemTraceText('failed')+': '+e.message;}
 await refreshSystemTrace();
};
setInterval(()=>{if(!document.getElementById('logbookDiagnostics').hidden&&document.getElementById('systemTraceLive').checked)refreshSystemTrace()},3000);
document.addEventListener('rflink-language-changed',()=>{if(systemTraceLast)renderSystemTrace()});
// Unified logbook: independent radio, system and browser streams; NO fourth device buffer.
const rfWebEvents=[];
let rfWebSeq=0,rfWebLimit=300,rfWebVerbose=false,rfBootOffset=null;
let rfLogbookCurrentBoot=null;
let rfLogbookView='all',rfLogbookSystem=[],rfLogbookRadio=[],rfLogbookBusy=false,rfLogbookLastError='',rfLogbookRadioCursor=null,rfLogbookRadioHasMore=false,rfLogbookRadioFilterKey='';
// Retrieve saved NVS boot history even when Status was not opened.
let rfLogbookLastStatusPoll=0;
const rfLogbookEl=id=>document.getElementById(id);
const rfLogbookT=k=>rfTranslate('logbook.'+k);
function rfWebConfigure(){
 try{const cfg=JSON.parse(localStorage.getItem('rflink_web_logging_v1')||'{}');
   if(Number.isInteger(cfg.limit)&&cfg.limit>=0&&cfg.limit<=2000)rfWebLimit=cfg.limit;
   rfWebVerbose=cfg.verbose===true;
 }catch(_){}
 rfLogbookEl('webLogLimit').value=rfWebLimit;
 rfLogbookEl('webLogVerbose').checked=rfWebVerbose;
}
function rfWebPush(area,msg,level='info',identity=''){
 if(!rfWebLimit)return;
 // Never record URL parameters, responses, tokens, passwords or packet data.
 const e={source:'web',seq:++rfWebSeq,client_ms:performance.now(),boot_session_id:window.rfCurrentBootSession||null,level,subsystem:area,message:String(msg).slice(0,175),uid:String(identity).slice(0,44)};
 rfWebEvents.push(e);if(rfWebEvents.length>rfWebLimit)rfWebEvents.splice(0,rfWebEvents.length-rfWebLimit);
}
window.rfLog=rfWebPush;
async function rfTrackedFetch(url,options){
 const start=performance.now(),path=String(url).split(/[?#]/,1)[0];
 const safePath=path.startsWith('/rflink/api/')?path:'/api';
 const method=String(options?.method||'GET').toUpperCase();
 try{
  const response=await window.fetch(url,options);
  const elapsed=Math.round(performance.now()-start);
  if(!response.ok)rfWebPush('api',method+' '+safePath+' HTTP '+response.status+' ('+elapsed+' ms)','error');
  else if(rfWebVerbose || method!=='GET' || elapsed>1500)
    rfWebPush('api',method+' '+safePath+' HTTP '+response.status+' ('+elapsed+' ms)',elapsed>1500?'warn':'info');
  return response;
 }catch(e){rfWebPush('api',method+' '+safePath+' network failure','error');throw e;}
}
document.addEventListener('click',e=>{
 const b=e.target.closest('button');if(!b||b.closest('#logbookPanel'))return;
 const id=b.id||b.dataset.page||b.dataset.go||b.dataset.settingsOpen||b.dataset.nativeCommand||b.dataset.command||b.dataset.deviceAction;
 const uid=b.dataset.nativeUid||b.dataset.profileCommand||b.closest('[data-device-uid]')?.dataset.deviceUid||'';
 if(id)rfWebPush('action','Click: '+String(id).slice(0,64),'info',uid);
});
document.addEventListener('change',e=>{
 const el=e.target;if(!el||!el.id||el.closest('#logbookPanel')||el.type==='password')return;
 rfWebPush('action','Control changed: '+String(el.id).slice(0,64));
});
window.addEventListener('error',e=>rfWebPush('browser','JavaScript error: '+String(e.message||'error').slice(0,100),'error'));
window.addEventListener('unhandledrejection',e=>rfWebPush('browser','Unhandled Promise rejection'+(e.reason?.name?': '+e.reason.name:''),'error'));
function rfLogbookDuration(ms){
 if(!Number.isFinite(ms)||ms<0)return '—';
 const total=Math.floor(ms),hours=Math.floor(total/3600000),minutes=Math.floor(total/60000)%60;
 const seconds=Math.floor(total/1000)%60,milli=total%1000;
 return String(hours).padStart(2,'0')+':'+String(minutes).padStart(2,'0')+':'+String(seconds).padStart(2,'0')+','+String(milli).padStart(3,'0');
}
function rfLogbookTime(x){
 if(x.historical)return null; // Old NVS records do not have an accurate uptime.
 if(x.boot_session_id&&rfLogbookCurrentBoot&&x.boot_session_id!==rfLogbookCurrentBoot)return null;
 return Number.isFinite(x.uptime_ms)?x.uptime_ms:rfBootOffset===null?null:Math.max(0,x.client_ms+rfBootOffset);
}
window.rfLogbookBootChanged=nextBoot=>{
 rfLogbookCurrentBoot=nextBoot;rfBootOffset=null;
 rfLogbookRadio=[];rfLogbookSystem=[];rfLogbookRadioCursor=null;rfLogbookRadioFilterKey='';rfLogbookRadioHasMore=false;
 rfWebPush('reboot','ESP32 started a new boot session','warn');
 rfLogbookRender();rfLogbookRefresh(true);
};
function rfLogbookRows(){
 const includeNoise=rfLogbookEl('logbookNoise').checked;
 const web=rfWebEvents.map(e=>({...e,uptime_ms:rfLogbookTime(e),key:'web-'+e.seq}));
 const system=rfLogbookSystem.map(e=>({source:'system',key:'system-'+e.seq,boot_session_id:rfLogbookCurrentBoot,uid:e.uid,uptime_ms:e.ms,seq:e.seq,subsystem:e.subsystem,message:e.message,level:e.level==='warn'?'warn':e.level==='error'?'error':'info'}));
 // NVS retains eight reset reasons. CRASHTRACE holds up to four technical
 // ESPHome reports, with original core/PC and raw backtrace addresses.
 // The original ELF is required to decode addresses, particularly after OTA.
 const status=window.rfDiagnosticStatus||{};
 const recorded=Array.isArray(status.boot_history)?status.boot_history:[];
 const technical=Array.isArray(status.crash_traces)?status.crash_traces:[];
 const traceByBoot=new Map(technical.filter(x=>Number.isSafeInteger(x.boot_number)&&x.boot_number>0).map(x=>[x.boot_number,x]));
 const traceDetail=trace=>{
  const lines=[rfTranslate('crashtrace.reason')+': '+String(trace.reason||'—'),
   rfTranslate('crashtrace.core')+': '+String(trace.core??'?'),
   'PC: '+String(trace.pc||'?'),
   rfTranslate('crashtrace.firmware')+': '+(trace.foreign_build?rfTranslate('crashtrace.other_build'):String(trace.source_hash||rfTranslate('crashtrace.unknown_build'))),
   rfTranslate('crashtrace.backtrace')+':',...(Array.isArray(trace.backtrace)?trace.backtrace:[]).slice(0,16).map((addr,i)=>'  BT'+i+': '+addr)];
  if(Array.isArray(trace.other_backtrace)&&trace.other_backtrace.length)
   lines.push(rfTranslate('crashtrace.other_core')+':',...trace.other_backtrace.slice(0,16).map((addr,i)=>'  BT'+i+': '+addr));
  lines.push(rfTranslate('crashtrace.elf_note'));
  return lines.join('\n');
 };
 const crashReasons=new Set(['panic','task_watchdog','interrupt_watchdog','watchdog','brownout']);
 const resetLabels={power_on:'Power-on',external_reset:'EN / RESET',software:'Software',panic:'Crash / panic',interrupt_watchdog:'Interrupt watchdog',task_watchdog:'Task watchdog',watchdog:'Watchdog',deep_sleep:'Deep sleep',brownout:'Brownout (supply)',sdio:'SDIO',unknown:'Unknown'};
 const history=recorded.filter(h=>Number.isSafeInteger(h.boot_number)&&h.boot_number>0&&h.boot_number!==Number(status.boot_count)).slice(0,8).map(h=>{
  const reason=String(h.reason||'unknown'),severity=crashReasons.has(reason)?'error':'info',trace=traceByBoot.get(h.boot_number);
  return {source:'system',key:'history-'+h.boot_number,uid:'',uptime_ms:null,historical:true,
   seq:h.boot_number,subsystem:'boot_history',level:severity,
   display_time:rfTranslate('version.history_time'),
   message:rfFormat('version.history_event',{n:h.boot_number,reason:resetLabels[reason]||reason})+(trace?' · '+rfTranslate('crashtrace.available'):''),
   detail:(trace?traceDetail(trace):rfTranslate('version.history_note'))+'\n'+rfTranslate('diag.reset_reason')+': '+reason+' ('+Number(h.reason_code||0)+')'};
 });
 // Current boot already has a live boot marker, but its previous-crash
 // backtrace should still be inspectable. Foreign-build records with unknown
 // boot number must not be silently assigned to an unrelated reset.
 const detached=technical.filter(t=>!Number.isSafeInteger(t.boot_number)||t.boot_number===0||
             Number(t.boot_number)===Number(status.boot_count)||!recorded.some(h=>h.boot_number===t.boot_number))
 .map((t,i)=>({source:'system',key:'crashtrace-'+i,uid:'',uptime_ms:null,historical:true,
   seq:Number(t.boot_number)||0,subsystem:'boot_history',level:'error',display_time:rfTranslate('version.history_time'),
   message:(t.boot_number?rfFormat('crashtrace.boot_event',{n:t.boot_number}):rfTranslate('crashtrace.unknown_boot')),
   detail:traceDetail(t)}));
 history.push(...detached);
 const radio=rfLogbookRadio.filter(e=>(includeNoise||e.category!=='noise')&&(rfLogbookView!=='radio'||(window.rfLogbookRadioFilters?.().direction||'all')==='all'||e.direction===window.rfLogbookRadioFilters?.().direction)).map(e=>({source:'radio',key:'radio-'+e.seq,seq:e.seq,uptime_ms:e.uptime_ms,subsystem:e.direction==='TX'?'tx':e.protocol||e.category,uid:e.identity||'',level:e.category==='noise'?'noise':e.level==='error'?'error':e.level==='warn'?'warn':'info',message:[e.direction||'RX',e.protocol||'',e.summary||e.reason||e.raw||''].filter(Boolean).join(' · ').slice(0,260),detail:e.full_detail||e.detail||'',category:e.category,radio:e.radio}));
 const chosen=rfLogbookView==='all'?radio.concat(system,web,history):rfLogbookView==='system'?system.concat(history):rfLogbookView==='web'?web:radio;
 const sev=rfLogbookEl('logbookSeverity').value,search=rfLogbookEl('logbookSearch').value.trim().toLowerCase(),eventType=rfLogbookEl('logbookEventType').value;
 return chosen.filter(e=>(sev==='all'||(sev==='warn'?(e.level==='warn'||e.level==='error'):e.level==='error'))&&(eventType==='all'||(eventType==='boot'&&(/boot|crash|panic|restart|watchdog/i).test(e.subsystem+' '+e.message))||(eventType==='tx'&&(/tx|pair|send|encoder|lab/i).test(e.subsystem+' '+e.message))||(eventType==='spi'&&(/spi|watchdog|radio.*recover/i).test(e.subsystem+' '+e.message))||(eventType==='api'&&(e.subsystem==='api'||e.source==='web')))&&(!search||[e.message,e.uid,e.subsystem,e.source,e.radio].join(' ').toLowerCase().includes(search))).sort((a,b)=>{if(a.historical!==b.historical)return a.historical?1:-1;if(a.historical)return b.seq-a.seq;const at=rfLogbookTime(a),bt=rfLogbookTime(b);return (bt??-1)-(at??-1)||String(b.source).localeCompare(a.source)||b.seq-a.seq;}).slice(0,2000);
}
function rfLogbookRender(){
 const host=rfLogbookEl('logbookTimeline');
 const expanded=new Set([...host.querySelectorAll('details[open]')].map(e=>e.dataset.key));
 const top=host.scrollTop;const events=rfLogbookRows();
 host.innerHTML=events.map(e=>{
   const ms=rfLogbookTime(e),kind=e.source==='system'?rfLogbookT('system'):e.source==='web'?rfLogbookT('web'):rfLogbookT('radio');
   const time=e.historical?e.display_time:rfLogbookDuration(ms);
   const state=e.level==='error'?'error':e.level==='warn'?'warn':e.level==='noise'?'noise':'info';
   const esc=systemTraceEscape;
   return '<details class="logbook-entry" data-key="'+esc(e.key)+'" '+(expanded.has(e.key)?'open':'')+'><summary><span class="mono muted small">'+time+'</span><b class="logbook-source src-'+esc(e.source)+'">'+esc(kind)+'</b><span class="logbook-severity lv-'+state+'">'+esc(rfTranslate('logbook6.'+state))+'</span><span class="logbook-message">'+esc(e.message)+'</span></summary><div class="muted small">'+esc(e.subsystem||'')+(e.uid?' · '+esc(e.uid):'')+(e.radio?' · '+esc(e.radio):'')+' · '+esc(e.level)+' · #'+esc(e.seq)+(e.detail?'<pre class="mono" style="white-space:pre-wrap">'+esc(e.detail)+'</pre>':'')+'</div></details>';
 }).join('')||'<p class="muted small logbook-empty">'+systemTraceEscape((rfLogbookView==='radio'&&!rfLogbookEl('logbookNoise').checked)?rfTranslate('logbook6.filtered_empty'):rfLogbookT('empty'))+'</p>';
 host.scrollTop=top;
 rfLogbookEl('logbookCount').textContent=events.length+' '+rfLogbookT('events')+(rfLogbookLastError?' · '+rfLogbookLastError:'');
 rfLogbookEl('logbookOlder').hidden=!(rfLogbookView==='radio'||rfLogbookView==='all')||!rfLogbookRadioHasMore;
}
function rfLogbookSetView(view){
 rfLogbookView=view;
 rfLogbookRadioFilterKey='';rfLogbookRadioCursor=null;rfLogbookRadioHasMore=false;
 document.querySelectorAll('[data-log-view]').forEach(b=>{b.classList.toggle('active',b.dataset.logView===view);b.setAttribute('aria-pressed',String(b.dataset.logView===view))});
  const diagnosis=view==='diagnostics';
  rfLogbookEl('logbookDiagnostics').hidden=!diagnosis;
  rfLogbookEl('radioLogView').hidden=view!=='radio';
  for(const id of ['logbookTimeline','logbookToolbar','logbookCount','logbookOlder','logbookExport','logbookLive','logbookNoise']){
   if(['logbookTimeline','logbookToolbar','logbookCount','logbookOlder'].includes(id))rfLogbookEl(id).hidden=diagnosis;
  }
  rfLogbookEl('logbookColumns').hidden=diagnosis;
  rfLogbookEl('logbookFooter').hidden=diagnosis;
  rfLogbookEl('logbookNotes').hidden=diagnosis;
 rfLogbookRender();rfLogbookRefresh(true);
 rfWebPush('navigation','Logbook: '+view);
}
async function rfLogbookRefresh(force=false,older=false){
 if(rfLogbookBusy||(!force&&!rfLogbookEl('logbookLive').checked))return;
 if(!rfLogbookEl('logbookPanel').closest('.page').classList.contains('active'))return;
 rfLogbookBusy=true;rfLogbookLastError='';
  try{
   if(rfLogbookView==='diagnostics'){
    const jobs=[];
    if(force || !window.rfLastDiag55Poll || Date.now()-window.rfLastDiag55Poll>10000){
     window.rfLastDiag55Poll=Date.now();
     jobs.push(rfTrackedFetch('/rflink/api/status',{cache:'no-store'}).then(async response=>{
      if(!response.ok)throw Error('Diagnose HTTP '+response.status);
      const data=await response.json();
      window.rfDiag5Render?.(data);
      window.rfDiagnosticStatus={...(window.rfDiagnosticStatus||{}),...data};
     }));
    }
    jobs.push(refreshSystemTrace());
    const outcomes=await Promise.allSettled(jobs);
    rfLogbookLastError=outcomes.filter(x=>x.status==='rejected').map(x=>String(x.reason)).join('; ');
    return;
   }
   const requests=[];
   // Historical NVS reset reasons are served by /status, not /system-trace.
  // Keep this query bounded: at most one request per 30 s while viewing logs.
  if(rfLogbookView==='all'||rfLogbookView==='system') {
    const now=Date.now();
    if(!rfLogbookLastStatusPoll||now-rfLogbookLastStatusPoll>=30000){
      rfLogbookLastStatusPoll=now;
      requests.push(rfTrackedFetch('/rflink/api/status',{cache:'no-store'}).then(async r=>{
        if(!r.ok)throw Error('Boothistorie HTTP '+r.status);
        const d=await r.json();
        if(!Array.isArray(d.boot_history))throw Error('Boothistorie ontbreekt in status-API');
        window.rfDiagnosticStatus={...(window.rfDiagnosticStatus||{}),
          boot_count:d.boot_count,boot_session_id:d.boot_session_id,
          boot_history:d.boot_history,boot_history_persisted:d.boot_history_persisted,
          crash_traces:d.crash_traces||[],crash_trace_persisted:d.crash_trace_persisted,
          reset_reason:d.reset_reason,reset_reason_code:d.reset_reason_code};
      }));
    }
  }
  if(rfLogbookView==='all'||rfLogbookView==='system') requests.push(rfTrackedFetch('/rflink/api/system-trace',{cache:'no-store'}).then(async r=>{if(!r.ok)throw Error('System HTTP '+r.status);const d=await r.json();if(Number.isFinite(d.server_uptime_ms))rfBootOffset=d.server_uptime_ms-performance.now();if(d.boot_session_id)rfLogbookCurrentBoot=d.boot_session_id;rfLogbookSystem=d.events||[];}));
  if(rfLogbookView==='all'||rfLogbookView==='radio') {
   const args=new URLSearchParams({order:'desc',limit:'100'});
   if(rfLogbookView==='radio'){
     const filters=window.rfLogbookRadioFilters?.()||{channel:-1,type:'all'};
     if(filters.channel>=0)args.set('radio',String(filters.channel));
     if(!rfLogbookEl('logbookNoise').checked)args.set('kind','0');
   }else if(!rfLogbookEl('logbookNoise').checked)args.set('kind','0');
   const queryKey=rfLogbookView+'|'+args.toString();
   if(queryKey!==rfLogbookRadioFilterKey){rfLogbookRadio=[];rfLogbookRadioFilterKey=queryKey;rfLogbookRadioCursor=null;rfLogbookRadioHasMore=false;}
   if(older && rfLogbookRadioCursor!==null)args.set('cursor',String(rfLogbookRadioCursor));
   requests.push(rfTrackedFetch('/rflink/api/rx-log?'+args,{cache:'no-store'}).then(async r=>{
    if(!r.ok)throw Error('Radio HTTP '+r.status);
    const d=await r.json();
    const rows=(d.events||[]).map(e=>({...e.record,seq:e.seq,uptime_ms:e.uptime_ms,category:e.category,radio:['433.42','433.92','868.30'][e.radio]||String(e.radio)}));
    const existing=rfLogbookRadio;
    rfLogbookRadio=(older?[...existing,...rows]:[...rows,...existing]).filter((e,i,all)=>all.findIndex(v=>v.seq===e.seq)===i).slice(0,2000);
    rfLogbookRadioCursor=rfLogbookRadio.length?rfLogbookRadio[rfLogbookRadio.length-1].seq:null;
    rfLogbookRadioHasMore=!!d.has_more;
   }));
  }
  const outcomes=await Promise.allSettled(requests);
  rfLogbookLastError=outcomes.filter(x=>x.status==='rejected').map(x=>String(x.reason.message||x.reason)).join('; ');
 }finally{rfLogbookBusy=false;rfLogbookRender();}
}
window.rfLogbookSelectView=rfLogbookSetView;
 window.rfLogbookRefreshRadio=()=>rfLogbookRefresh(true);
window.rfLogbookRenderBootHistory=()=>{if(document.getElementById('logbookPanel')?.closest('.page')?.classList.contains('active'))rfLogbookRender()};
window.rfLogbookOpen=()=>{rfLogbookRender();rfLogbookRefresh(true)};
window.rfLogbookSettings=async()=>{rfWebConfigure();try{const r=await rfTrackedFetch('/rflink/api/system-trace-config');if(r.ok){const d=await r.json();rfLogbookEl('systemLogLimit').value=d.limit;}}catch(_){}};
rfWebConfigure();rfWebPush('browser','RFLink web interface initialized');
document.querySelectorAll('[data-log-view]').forEach(b=>b.addEventListener('click',()=>rfLogbookSetView(b.dataset.logView)));
for(const id of ['logbookSeverity','logbookSearch','logbookEventType'])rfLogbookEl(id).addEventListener(id==='logbookSearch'?'input':'change',rfLogbookRender);
 rfLogbookEl('diagnosticOpenCrashes').onclick=()=>{rfLogbookSetView('system');rfLogbookEl('logbookEventType').value='boot';rfLogbookRender();};
 rfLogbookEl('diagnosticOpenRadio').onclick=()=>{rfLogbookSetView('radio');};
 rfLogbookEl('diagnosticRefresh').onclick=()=>rfLogbookRefresh(true);
 rfLogbookEl('diagnosticReport').onclick=()=>rfLogbookEl('logbookDiagnosticExport').click();
 document.querySelectorAll('[data-diag-show]').forEach(b=>b.onclick=()=>{
  const target=b.dataset.diagShow;
  const panel=rfLogbookEl(target);
  const next=!panel.open;
  document.querySelectorAll('.diag-five-details[id^="diagFive"]').forEach(d=>{d.open=d===panel&&next});
  document.querySelectorAll('[data-diag-show]').forEach(x=>x.setAttribute('aria-expanded',String(x===b&&next)));
  if(next&&target==='diagFiveLab'){
   rfLogbookEl('diagFiveLegacy').hidden=false;
   rfLogbookEl('diagFiveLegacy').style.setProperty('display','block','important');
   refreshSystemTrace();
  }else{
   rfLogbookEl('diagFiveLegacy').hidden=true;
   rfLogbookEl('diagFiveLegacy').style.removeProperty('display');
  }
 });
rfLogbookEl('logbookNoise').onchange=()=>rfLogbookRefresh(true);
rfLogbookEl('logbookRefresh').onclick=()=>rfLogbookRefresh(true);
rfLogbookEl('logbookOlder').onclick=()=>rfLogbookRefresh(true,true);
rfLogbookEl('logbookExport').onclick=()=>{
 const filtered=rfLogbookRows();
 const sanitized=filtered.map(({key,client_ms,...data})=>data);
 const data={exported_at:new Date().toISOString(),selection:rfLogbookView,boot_aligned_to_esp:rfBootOffset!==null,events:sanitized};
 const url=URL.createObjectURL(new Blob([JSON.stringify(data,null,2)],{type:'application/json'}));
 const a=document.createElement('a');a.href=url;a.download='rflink-logbook-'+rfLogbookView+'.json';a.click();setTimeout(()=>URL.revokeObjectURL(url),500);
 rfWebPush('export','Exported '+rfLogbookView+' log');
};
// DIAG7: one privacy-conscious diagnostic report. No passwords, keys, raw RF data,
// access tokens, or full unrestricted API responses. Browser can inspect before sharing.
document.getElementById('logbookDiagnosticExport').onclick=async()=>{
 const status=window.rfDiagnosticStatus||{};
 const scrub=x=>String(x??'').replace(/(password|token|secret|key)\s*[:=]\s*[^\s,;]+/gi,'$1=[redacted]').slice(0,240);
 let system=[];
 try{const r=await rfTrackedFetch('/rflink/api/system-trace',{cache:'no-store'});if(r.ok){const d=await r.json();system=(d.events||[]).slice(-128).map(e=>({seq:e.seq,uptime_ms:e.ms,level:e.level,subsystem:e.subsystem,message:scrub(e.message)}));}}catch(_){}
 const report={format:'RFLink-DIAG7',exported_at:new Date().toISOString(),warning:'Review diagnostic records before sharing. RF identities may be present in event text.',firmware:{version:status.firmware_version||null,revision:status.firmware_revision||null,built:status.firmware_built||null},boot:{number:status.boot_count||null,session_id:status.boot_session_id||null,previous_reset_reason:status.reset_reason||null,history:status.boot_history||[],persisted:!!status.boot_history_persisted,technical_traces:status.crash_traces||[],traces_persisted:!!status.crash_trace_persisted},memory:{free_heap_bytes:status.free_heap_bytes??null,internal_free_bytes:status.internal_free_bytes??null,largest_internal_block_bytes:status.largest_internal_block_bytes??null,psram_free_bytes:status.psram_free_bytes??null},radios:(status.channels||[]).map(r=>({name:r.name,frequency_hz:r.frequency_hz,available:r.available,rx_ready:r.rx_ready,tx_ready:r.tx_ready,spi_failures:r.spi_failures,spi_recovery_attempts:r.spi_recovery_attempts})),system_events:system,web_events:rfWebEvents.slice(-120).map(e=>({level:e.level,subsystem:e.subsystem,message:scrub(e.message),uptime_ms:rfLogbookTime(e)})),note:'Technical crash details may include raw addresses from ESPHome. Keep the matching crash-build firmware.elf for symbolication; older reports may be unavailable.'};
 const url=URL.createObjectURL(new Blob([JSON.stringify(report,null,2)],{type:'application/json'}));
 const a=document.createElement('a');a.href=url;a.download='rflink-diagnose-'+(status.boot_count||'unknown')+'.json';a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);
};
// Existing radio limits keep their own NVS-backed save; additionally store both new limits.
window.rfSaveLogbookSettings=async()=>{
 const systemText=rfLogbookEl('systemLogLimit').value,webText=rfLogbookEl('webLogLimit').value;
 const wanted=Number(systemText),web=Number(webText);
 if(!/^\d+$/.test(systemText)||!/^\d+$/.test(webText)||!Number.isInteger(wanted)||wanted<0||wanted>256||!Number.isInteger(web)||web<0||web>2000){rfLogbookEl('rxLogSettingStatus').textContent=rfLogbookT('invalid');return;}
 try{
  const config=await(await rfTrackedFetch('/rflink/api/system-trace-config')).json();
  const r=await rfTrackedFetch('/rflink/api/system-trace-config',{method:'POST',body:new URLSearchParams({limit:String(wanted),write_token:String(config.write_token)})});
  if(!r.ok)throw Error('System logging HTTP '+r.status);
  rfWebLimit=web;rfWebVerbose=rfLogbookEl('webLogVerbose').checked;
  if(rfWebEvents.length>rfWebLimit)rfWebEvents.splice(0,rfWebEvents.length-rfWebLimit);
  try{localStorage.setItem('rflink_web_logging_v1',JSON.stringify({limit:rfWebLimit,verbose:rfWebVerbose}));}catch(_){}
  rfWebPush('settings','Logging limits updated');
 }catch(e){throw Error(rfLogbookT('save_failed')+' '+e.message);}
};
setInterval(()=>{if(rfLogbookEl('logbookLive').checked)rfLogbookRefresh();},3000);
document.addEventListener('rflink-language-changed',rfLogbookRender);

</script>
</body>
</html>
)RFHTML";
static constexpr size_t RFLINK_UI_HTML_SIZE = sizeof(RFLINK_UI_HTML) - 1;

static constexpr char RFLINK_FAVICON_SVG[] = R"RFSVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64">
  <rect width="64" height="64" rx="15" fill="#0b1b33"/>
  <path d="M14 24 Q32 7 50 24 M20 31 Q32 20 44 31" fill="none" stroke="#47e69e" stroke-width="5" stroke-linecap="round"/>
  <circle cx="32" cy="38" r="4" fill="#47e69e"/>
  <path d="M32 43 V52 M23 52 H41" fill="none" stroke="#e8fff2" stroke-width="4" stroke-linecap="round"/>
</svg>
)RFSVG";
static constexpr size_t RFLINK_FAVICON_SVG_SIZE = sizeof(RFLINK_FAVICON_SVG) - 1;

static constexpr uint8_t RFLINK_FAVICON_ICO[] = {
    0x00, 0x00, 0x01, 0x00, 0x04, 0x00, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0xE3, 0x02,
    0x00, 0x00, 0x46, 0x00, 0x00, 0x00, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x6D, 0x06,
    0x00, 0x00, 0x29, 0x03, 0x00, 0x00, 0x30, 0x30, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x88, 0x0A,
    0x00, 0x00, 0x96, 0x09, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x7E, 0x0E,
    0x00, 0x00, 0x1E, 0x14, 0x00, 0x00, 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00,
    0x00, 0x0D, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x10, 0x08, 0x06,
    0x00, 0x00, 0x00, 0x1F, 0xF3, 0xFF, 0x61, 0x00, 0x00, 0x02, 0xAA, 0x49, 0x44, 0x41, 0x54, 0x78,
    0x9C, 0x6D, 0x92, 0x4D, 0x68, 0xD4, 0x47, 0x18, 0xC6, 0x7F, 0xEF, 0xCC, 0xEC, 0xAE, 0xBB, 0x9B,
    0x5D, 0x71, 0x15, 0xED, 0x66, 0x23, 0xB6, 0xC1, 0x1A, 0x90, 0x98, 0x0F, 0xB7, 0x68, 0x69, 0x85,
    0x42, 0x59, 0xBD, 0xD4, 0x83, 0x07, 0x5B, 0x22, 0x78, 0xD0, 0xAB, 0x87, 0x9E, 0x3D, 0xD4, 0x0F,
    0x54, 0xF0, 0xD2, 0x4B, 0x3D, 0x78, 0x13, 0x05, 0x0F, 0x8A, 0x78, 0xB2, 0x78, 0x28, 0xD1, 0x82,
    0xA5, 0xD2, 0xB4, 0xB4, 0x4D, 0x22, 0x5A, 0x30, 0x8A, 0x15, 0x8C, 0x59, 0xEB, 0x47, 0xE2, 0x6E,
    0x12, 0x93, 0xDD, 0xFD, 0xCF, 0xBC, 0x1E, 0xFE, 0xEB, 0x47, 0xC5, 0xB9, 0xCC, 0xC0, 0x3B, 0xEF,
    0xF3, 0xCE, 0xF3, 0x9B, 0x47, 0x00, 0x32, 0xC5, 0xF2, 0x36, 0xB1, 0xE6, 0x38, 0xAA, 0xBD, 0xA0,
    0x09, 0x40, 0x78, 0xFF, 0x52, 0xA0, 0x09, 0xF2, 0x87, 0x6A, 0xEB, 0xDB, 0x17, 0x53, 0xE3, 0x63,
    0x92, 0xED, 0xFC, 0xA4, 0x82, 0xF0, 0x23, 0x22, 0x4B, 0xD0, 0x00, 0x80, 0x18, 0x01, 0x11, 0x50,
    0x8D, 0xDB, 0xDA, 0x67, 0x0D, 0xDA, 0xAE, 0x5B, 0x42, 0xF0, 0x8F, 0x45, 0xA8, 0x48, 0xA6, 0xB3,
    0x7C, 0x4B, 0x8C, 0x59, 0x8F, 0xFA, 0x08, 0x11, 0x27, 0x46, 0xF0, 0xF3, 0x2D, 0xFC, 0x42, 0x0B,
    0x71, 0x26, 0x1E, 0x1B, 0x05, 0x6C, 0x3A, 0x81, 0xCD, 0x26, 0x62, 0x91, 0xA0, 0x91, 0x18, 0xE7,
    0x82, 0xFA, 0x5F, 0x25, 0x5B, 0x2A, 0x7B, 0x40, 0x10, 0x11, 0x42, 0x20, 0x9A, 0x6D, 0x92, 0x1F,
    0x2C, 0x52, 0xDC, 0xB9, 0x81, 0x5C, 0xEF, 0x2A, 0x00, 0x66, 0x6F, 0xFD, 0xC7, 0xA3, 0x8B, 0x37,
    0xA9, 0xFD, 0x3D, 0x85, 0xCB, 0x25, 0xC1, 0x18, 0x50, 0x55, 0x40, 0x25, 0x5B, 0x2A, 0x2B, 0x22,
    0x68, 0xCB, 0x83, 0xC0, 0xBA, 0xC3, 0x15, 0x4A, 0xBB, 0x07, 0xA9, 0x8F, 0x4E, 0x31, 0x7D, 0xFD,
    0x3E, 0x28, 0x14, 0xB6, 0xAC, 0x21, 0xBF, 0xB1, 0xC4, 0xC3, 0xB3, 0xA3, 0x4C, 0x1C, 0xBE, 0x0A,
    0xAA, 0x48, 0xC2, 0xC6, 0x7B, 0xB6, 0xAB, 0xAC, 0xEA, 0x15, 0x9B, 0x72, 0x0C, 0x9E, 0x1F, 0x62,
    0x49, 0x67, 0x9E, 0xB1, 0xBD, 0x17, 0x99, 0x1D, 0x7F, 0x44, 0x47, 0xCF, 0x0A, 0x00, 0xE6, 0x6E,
    0x3F, 0x25, 0xD7, 0xF7, 0x01, 0x03, 0xA7, 0x77, 0xB2, 0x58, 0xAD, 0x33, 0x3A, 0x74, 0x1E, 0xDF,
    0x88, 0x10, 0x2B, 0xB8, 0xD8, 0xA4, 0x62, 0xD2, 0x8E, 0xE7, 0x23, 0x0F, 0xB8, 0x7F, 0x72, 0x84,
    0xC2, 0x96, 0x35, 0x0C, 0x9C, 0xF9, 0x1A, 0x93, 0xB2, 0x00, 0x84, 0x86, 0x67, 0xE2, 0xD0, 0x30,
    0x23, 0x5B, 0x4F, 0xF1, 0xE1, 0xBE, 0x4F, 0x31, 0x69, 0x87, 0x5F, 0x6C, 0xC5, 0xCE, 0x63, 0x0B,
    0x00, 0xC2, 0xC2, 0xE4, 0x0C, 0xEB, 0x0E, 0x56, 0xF8, 0xF8, 0xBB, 0x2F, 0xB9, 0x7D, 0x60, 0x98,
    0xEA, 0x85, 0x1B, 0x00, 0x14, 0xBF, 0xE9, 0xA3, 0xE7, 0xE8, 0x56, 0xEE, 0x1C, 0xFB, 0x99, 0x89,
    0x23, 0x57, 0x48, 0x77, 0x2D, 0x8B, 0x7F, 0x54, 0x69, 0x0B, 0x40, 0xCC, 0x30, 0xF2, 0xE4, 0xFB,
    0x8B, 0x84, 0xA6, 0x67, 0xFA, 0xDA, 0xBF, 0x94, 0x76, 0xF5, 0x03, 0xF0, 0xF0, 0xDC, 0x38, 0x85,
    0x2F, 0x3E, 0xC2, 0x24, 0x2D, 0xF5, 0xF1, 0x2A, 0xC6, 0xD9, 0x36, 0x43, 0xDA, 0x16, 0x88, 0xA1,
    0x8A, 0xB3, 0x3C, 0xFF, 0x73, 0x12, 0x02, 0x64, 0xBA, 0x0B, 0x74, 0x1F, 0xAF, 0x80, 0xC2, 0xCC,
    0xEF, 0x0F, 0xA8, 0x8F, 0x55, 0xC1, 0x80, 0x4D, 0x27, 0x5F, 0x37, 0xFF, 0x4F, 0xE0, 0x15, 0x0B,
    0x9B, 0x4E, 0x92, 0x4C, 0x3A, 0xE6, 0x9F, 0xD4, 0xE9, 0xFD, 0x25, 0x02, 0xE0, 0xFA, 0x93, 0x3A,
    0xD9, 0xE5, 0x79, 0x9A, 0xCD, 0xE8, 0x4D, 0xB8, 0xDE, 0x2B, 0xD0, 0xCE, 0x70, 0x6D, 0x66, 0x96,
    0x50, 0x9B, 0x64, 0xD5, 0xB3, 0xB8, 0xDC, 0xAC, 0x4D, 0x12, 0x99, 0xD5, 0x64, 0xB2, 0x69, 0xF4,
    0x9D, 0xFB, 0x0E, 0x08, 0xED, 0x3E, 0x11, 0x23, 0x34, 0x16, 0x1A, 0xEC, 0xDE, 0xB3, 0x83, 0xBE,
    0xEE, 0xB5, 0x7C, 0xF6, 0xD5, 0x66, 0x00, 0xBE, 0x37, 0x3F, 0x70, 0xE3, 0xDE, 0x5D, 0xCE, 0x9D,
    0xBD, 0x44, 0x2A, 0x9D, 0x7A, 0x15, 0x69, 0x05, 0xD4, 0x29, 0xFA, 0x9B, 0x11, 0xF7, 0xB9, 0x6A,
    0x14, 0x81, 0x38, 0x00, 0x0D, 0x81, 0x54, 0x2E, 0xC3, 0x4F, 0x97, 0xAF, 0x01, 0xB0, 0x22, 0x57,
    0x40, 0x43, 0x78, 0xDB, 0x6B, 0x84, 0x58, 0xA7, 0x21, 0xFC, 0x23, 0xD9, 0xAE, 0xF2, 0x06, 0x55,
    0xAE, 0x18, 0x63, 0x57, 0x6A, 0xF0, 0x88, 0x08, 0x73, 0xB5, 0x39, 0x68, 0x34, 0xC0, 0xC6, 0x39,
    0xC0, 0x7B, 0x48, 0xA5, 0xE8, 0x58, 0xDA, 0x11, 0x03, 0x14, 0x03, 0xAA, 0x8B, 0xA8, 0xDF, 0x2E,
    0x00, 0x99, 0xCE, 0xFE, 0x01, 0x91, 0xC4, 0x09, 0x60, 0x13, 0x68, 0xD2, 0x5A, 0x23, 0x22, 0xA6,
    0xFD, 0xCA, 0x98, 0x8C, 0x6A, 0xC0, 0xFB, 0xA0, 0x20, 0x2D, 0x44, 0x6E, 0xE2, 0xC3, 0xFE, 0xF9,
    0xEA, 0x5F, 0xC3, 0x2F, 0x01, 0x7E, 0x50, 0x24, 0xB9, 0x72, 0xA7, 0x66, 0xED, 0x00, 0x00, 0x00,
    0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82, 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A,
    0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x20, 0x08, 0x06, 0x00, 0x00, 0x00, 0x73, 0x7A, 0x7A, 0xF4, 0x00, 0x00, 0x06, 0x34, 0x49, 0x44,
    0x41, 0x54, 0x78, 0x9C, 0xAD, 0x97, 0x69, 0x6C, 0x5D, 0x47, 0x15, 0xC7, 0x7F, 0x67, 0xE6, 0xDE,
    0xFB, 0xFC, 0x16, 0xC7, 0x4B, 0xEC, 0xCA, 0xA9, 0x9D, 0x34, 0xD0, 0xD4, 0x6D, 0x88, 0x80, 0x16,
    0xB7, 0x51, 0x0C, 0x09, 0x85, 0x42, 0xA1, 0x15, 0x20, 0x40, 0xD0, 0x0F, 0xA8, 0x80, 0x8A, 0x1A,
    0x16, 0x81, 0xF2, 0xA1, 0x08, 0xA9, 0x20, 0xB1, 0x88, 0x4D, 0x0D, 0x12, 0x08, 0x01, 0x65, 0x11,
    0xB4, 0x7C, 0x08, 0x54, 0xFD, 0x10, 0x21, 0x40, 0x80, 0x02, 0x22, 0x6A, 0xA0, 0xA1, 0x4D, 0xDA,
    0x2C, 0x6A, 0x11, 0x69, 0x13, 0x13, 0x22, 0x37, 0x8E, 0x9B, 0xA8, 0x76, 0x6C, 0x3F, 0xFB, 0x2D,
    0x7E, 0x77, 0x99, 0xC3, 0x87, 0x7B, 0x5F, 0xB3, 0xF8, 0xBD, 0xC4, 0x51, 0x73, 0xA4, 0x27, 0x3D,
    0xCD, 0x9D, 0x39, 0xFF, 0xFF, 0xF9, 0x9F, 0x33, 0x33, 0x67, 0x84, 0xD4, 0x0C, 0xE0, 0xE0, 0x76,
    0xAF, 0x30, 0x54, 0xB9, 0xCF, 0x20, 0xF7, 0x3A, 0xD5, 0x37, 0x0B, 0x74, 0x03, 0xC2, 0x6B, 0x33,
    0x55, 0x98, 0x16, 0xD8, 0xAF, 0xC8, 0xCF, 0x6B, 0x93, 0x07, 0x77, 0x65, 0xE3, 0x02, 0xA8, 0x34,
    0xC1, 0x4B, 0x03, 0xB7, 0xAC, 0x57, 0xCF, 0x3E, 0x2A, 0x22, 0xA3, 0xAA, 0xD9, 0x32, 0xF4, 0x35,
    0x62, 0x37, 0x4D, 0x10, 0x11, 0x14, 0x05, 0xE5, 0xD1, 0xAA, 0x78, 0xDB, 0x38, 0xB5, 0xBF, 0x0E,
    0x88, 0x00, 0x94, 0x56, 0x8D, 0xDC, 0xE4, 0xAC, 0x3C, 0x69, 0xC4, 0xF4, 0xAB, 0x8B, 0x93, 0x6C,
    0xBC, 0xF9, 0xBB, 0x1A, 0xA6, 0x69, 0x44, 0x82, 0x58, 0xCF, 0xB8, 0x38, 0x7E, 0xA2, 0xE6, 0x4F,
    0xBF, 0x8F, 0x97, 0x36, 0x46, 0x86, 0x75, 0xEB, 0x72, 0x4E, 0x74, 0x47, 0x0A, 0x9E, 0x44, 0x20,
    0x96, 0x54, 0x95, 0xAB, 0x05, 0x4E, 0xEA, 0x4B, 0x0C, 0x60, 0x34, 0x89, 0x43, 0xE3, 0xF9, 0x77,
    0x14, 0xE2, 0xBE, 0x1F, 0xC0, 0xCE, 0x44, 0x0A, 0xAB, 0x46, 0xB6, 0x1A, 0x6B, 0x7F, 0xA5, 0x1A,
    0x27, 0x19, 0xF8, 0xE5, 0x5D, 0x89, 0x34, 0xC5, 0x3B, 0x2F, 0x46, 0x45, 0x55, 0x97, 0x93, 0x35,
    0x05, 0x1C, 0x22, 0x46, 0x12, 0xDD, 0xE2, 0x61, 0xF4, 0x53, 0x9A, 0xCA, 0x73, 0xE9, 0x88, 0x45,
    0x10, 0x23, 0x68, 0x94, 0x10, 0x37, 0x22, 0x34, 0x76, 0xE0, 0x32, 0x34, 0x23, 0x88, 0x67, 0xB0,
    0x39, 0x0F, 0xF1, 0x2D, 0xEA, 0x32, 0xC5, 0xDB, 0x86, 0xA0, 0x80, 0x15, 0x95, 0xE4, 0x8B, 0x9E,
    0xA8, 0x6C, 0x00, 0xD7, 0x3E, 0xDF, 0x02, 0x62, 0x0C, 0x49, 0x3D, 0xC2, 0x2D, 0xC6, 0x04, 0x7D,
    0x05, 0x3A, 0xDF, 0x34, 0x40, 0xE1, 0xF5, 0xBD, 0xF8, 0xBD, 0x05, 0x00, 0xA2, 0x99, 0x1A, 0xB5,
    0x13, 0x33, 0x54, 0xC7, 0xA6, 0x09, 0xA7, 0x6B, 0x98, 0x0E, 0x0F, 0x9B, 0xF7, 0x51, 0xE7, 0xDA,
    0x28, 0x22, 0x06, 0x75, 0xA8, 0xF0, 0x76, 0x0F, 0x61, 0x45, 0x5B, 0xAA, 0x46, 0xD0, 0xC4, 0x11,
    0x95, 0x6B, 0x14, 0x87, 0xFB, 0xB9, 0xF6, 0x9E, 0x37, 0xB2, 0xF2, 0x5D, 0xD7, 0x93, 0x5F, 0xDD,
    0x85, 0x2D, 0x06, 0x88, 0x6F, 0x52, 0x4D, 0x23, 0x47, 0x52, 0x0D, 0xA9, 0x4F, 0x94, 0x39, 0xBB,
    0xE7, 0x04, 0xA7, 0x77, 0xFE, 0x9B, 0xCA, 0xD1, 0x29, 0xBC, 0xCE, 0x1C, 0x62, 0x4D, 0xAA, 0x48,
    0x0B, 0x15, 0x04, 0x56, 0x4A, 0x71, 0x70, 0xA4, 0x35, 0x47, 0x6B, 0x48, 0x6A, 0x21, 0x26, 0xE7,
    0xB1, 0xE6, 0xB3, 0x1B, 0x19, 0xFA, 0xF8, 0x2D, 0x04, 0xD7, 0x14, 0x11, 0x63, 0xD0, 0x38, 0x21,
    0x2A, 0x37, 0x48, 0x2A, 0x0D, 0x00, 0x6C, 0x29, 0x87, 0xDF, 0x95, 0x43, 0x3C, 0x8B, 0x3A, 0x47,
    0x38, 0x55, 0x65, 0xF2, 0xB1, 0xE7, 0x78, 0xE9, 0x17, 0xCF, 0xE0, 0x16, 0x63, 0x6C, 0x21, 0x40,
    0x13, 0xD7, 0x3A, 0xC8, 0x56, 0x04, 0xC4, 0x08, 0xD1, 0x7C, 0x83, 0xD2, 0xFA, 0x7E, 0xD6, 0x6F,
    0xBF, 0x8B, 0xAE, 0x91, 0x6B, 0x51, 0x07, 0xAE, 0x1E, 0x31, 0xBB, 0xEF, 0x24, 0xD3, 0xBB, 0x8F,
    0xB3, 0xF0, 0xC2, 0x2B, 0x44, 0x33, 0x75, 0x00, 0xFC, 0xDE, 0x3C, 0xA5, 0x9B, 0xFA, 0xE9, 0x7F,
    0xCF, 0x0D, 0xF4, 0x8C, 0xAE, 0xC1, 0xE4, 0x7D, 0xC4, 0x08, 0xE5, 0xC3, 0x93, 0xBC, 0xF8, 0xE0,
    0x5F, 0xA9, 0xBC, 0x38, 0x85, 0xBF, 0x22, 0xD7, 0x4A, 0x89, 0xA5, 0x04, 0xC4, 0x08, 0x71, 0x35,
    0xA4, 0xF7, 0x6D, 0x6B, 0xD9, 0xF0, 0xA3, 0xF7, 0xE3, 0xF7, 0xE4, 0x41, 0x60, 0xEE, 0xC0, 0x24,
    0xE3, 0x3F, 0x7E, 0x8A, 0xD9, 0x7D, 0x13, 0xC4, 0x0B, 0x0D, 0xE0, 0xBC, 0xBA, 0x55, 0x05, 0x04,
    0xAF, 0x33, 0x47, 0xCF, 0xE8, 0x6A, 0xD6, 0x6E, 0x7B, 0x2B, 0xDD, 0x1B, 0x87, 0x40, 0x21, 0x9A,
    0xAD, 0x73, 0xE4, 0x81, 0x3F, 0x33, 0xB3, 0x77, 0x1C, 0xAF, 0x18, 0x2C, 0x21, 0x71, 0x01, 0x01,
    0x31, 0x42, 0x52, 0x8F, 0x28, 0x0E, 0xF7, 0x31, 0xB2, 0xF3, 0x5E, 0x4C, 0x87, 0x87, 0xAA, 0x32,
    0xF1, 0xCB, 0x03, 0x8C, 0x3F, 0xFC, 0x34, 0xE1, 0xD9, 0x3A, 0xB6, 0x14, 0x50, 0x1A, 0xEE, 0xA3,
    0x73, 0xC3, 0x35, 0xE4, 0x06, 0x3A, 0x01, 0x68, 0x9C, 0x59, 0x60, 0xE1, 0xC8, 0x2B, 0x54, 0xC6,
    0xA6, 0x49, 0xAA, 0x21, 0x41, 0x4F, 0x9E, 0xEB, 0xB6, 0x8D, 0xB2, 0xE6, 0x33, 0x1B, 0x11, 0x11,
    0x5C, 0x23, 0xE6, 0xD0, 0x47, 0x1F, 0xA3, 0x3A, 0x36, 0x9D, 0x15, 0xE7, 0x39, 0x12, 0x5E, 0xCB,
    0xC4, 0x00, 0x9A, 0x38, 0xDC, 0x62, 0xCC, 0xD8, 0x37, 0x76, 0x33, 0xF9, 0xF8, 0xF3, 0x88, 0x15,
    0x56, 0xBE, 0xE3, 0x75, 0x0C, 0xDD, 0x37, 0x42, 0xCF, 0xA6, 0xD5, 0xF8, 0xBD, 0x05, 0x4C, 0x90,
    0x1E, 0x1B, 0x2E, 0x4C, 0x88, 0x66, 0x6A, 0xCC, 0x3D, 0x33, 0xC1, 0xC4, 0xAF, 0x0F, 0x31, 0xBB,
    0xFF, 0x24, 0xC7, 0xBF, 0xFB, 0x0F, 0xEA, 0x27, 0x66, 0x18, 0xFE, 0xE6, 0x9D, 0xE9, 0x96, 0x6D,
    0x63, 0x4B, 0x6B, 0x20, 0x63, 0x5C, 0xBC, 0x61, 0x25, 0x62, 0x84, 0xF9, 0xE7, 0x4E, 0x63, 0x4B,
    0x01, 0x6B, 0xBF, 0x30, 0xCA, 0x9A, 0x4F, 0xDF, 0x86, 0x2D, 0x06, 0x60, 0x84, 0xA4, 0x12, 0x12,
    0xCD, 0x66, 0x35, 0xD0, 0x93, 0xC7, 0x96, 0x02, 0x70, 0x4A, 0x5C, 0x0D, 0x99, 0x78, 0xE4, 0x00,
    0xE3, 0x0F, 0xEF, 0x23, 0xA9, 0x84, 0xAC, 0xB8, 0x79, 0x15, 0xEA, 0x94, 0xEA, 0x7F, 0xCF, 0x62,
    0x72, 0xDE, 0x92, 0xF3, 0xA1, 0xF5, 0x2E, 0xC8, 0x48, 0xA0, 0x8A, 0x2D, 0x05, 0xDC, 0xF8, 0xAD,
    0x3B, 0x19, 0xF8, 0xF0, 0x06, 0x5C, 0x94, 0x90, 0x54, 0x42, 0xCE, 0xFC, 0xE1, 0x05, 0xA6, 0x76,
    0x1D, 0xA3, 0x7E, 0xAA, 0x0C, 0x40, 0x7E, 0x75, 0x17, 0xFD, 0x77, 0xDD, 0xC8, 0xC0, 0x87, 0xDE,
    0x80, 0x2D, 0xE5, 0x30, 0xBE, 0xE1, 0xCC, 0xEF, 0x8F, 0x70, 0xEC, 0xEB, 0x7F, 0x27, 0xA9, 0x84,
    0x20, 0xD2, 0x12, 0xBC, 0x3D, 0x01, 0x40, 0x3C, 0x43, 0xBC, 0xD0, 0xA0, 0x67, 0xD3, 0x1A, 0x6E,
    0xFB, 0xD3, 0x27, 0x49, 0x6A, 0x11, 0xE5, 0xC3, 0x93, 0x8C, 0x7D, 0x6D, 0x37, 0xE5, 0xC3, 0x93,
    0x98, 0x0E, 0xEF, 0x82, 0x22, 0x74, 0x8B, 0x31, 0x5D, 0x6F, 0x19, 0x64, 0xF8, 0xDB, 0xEF, 0xA6,
    0x6B, 0x64, 0x10, 0x9B, 0xF7, 0x39, 0xF0, 0x81, 0x1D, 0xCC, 0xEE, 0x3F, 0x89, 0xD7, 0x99, 0x6B,
    0x9B, 0x86, 0xB6, 0x04, 0x10, 0xC0, 0x29, 0xE2, 0x5B, 0xAE, 0xFF, 0xD2, 0x16, 0x24, 0xF0, 0x38,
    0xFE, 0xD0, 0x1E, 0xE2, 0xB9, 0x45, 0xBC, 0xEE, 0x3C, 0x49, 0xA5, 0x81, 0xD7, 0x99, 0x03, 0x20,
    0x5E, 0x68, 0x60, 0x4B, 0x39, 0xE2, 0xB9, 0x3A, 0x5E, 0x77, 0x07, 0xEB, 0xBE, 0xF2, 0x4E, 0x34,
    0x8C, 0xF9, 0xDF, 0xF7, 0xF7, 0xA2, 0x51, 0x02, 0x46, 0xDA, 0xDE, 0x11, 0xED, 0x09, 0x64, 0x24,
    0x34, 0x51, 0x5C, 0x3D, 0x42, 0x15, 0x6C, 0xC1, 0xC7, 0xE6, 0x3C, 0xA2, 0xF9, 0x45, 0xFA, 0xDF,
    0x3B, 0xCC, 0xBA, 0x2F, 0xDF, 0x0E, 0xC0, 0xF1, 0xED, 0xFF, 0x64, 0xEA, 0x6F, 0x63, 0xF8, 0x2B,
    0x3A, 0x48, 0x1A, 0x31, 0x49, 0x2D, 0x42, 0x84, 0xF4, 0x3C, 0xB0, 0xED, 0xC1, 0x2F, 0x4F, 0xA0,
    0x39, 0xC9, 0x48, 0x4A, 0x46, 0x15, 0x8D, 0x15, 0x9B, 0xF3, 0xB8, 0xF5, 0x8F, 0x9F, 0xA0, 0xB8,
    0xB6, 0x17, 0x80, 0xEA, 0xF8, 0x0C, 0x07, 0x3F, 0xF8, 0x1B, 0x92, 0x46, 0x8C, 0x78, 0x69, 0xF3,
    0x81, 0xD2, 0xF2, 0xE0, 0xB9, 0xD8, 0xDA, 0x6E, 0xC3, 0xF3, 0xED, 0x55, 0x47, 0x17, 0x5D, 0x57,
    0x95, 0x46, 0x7D, 0xE9, 0xF0, 0x32, 0x81, 0x9B, 0x66, 0x96, 0x3D, 0x33, 0x73, 0x6E, 0x3C, 0x43,
    0x3C, 0xDF, 0xE0, 0xE8, 0xF6, 0x3D, 0x6C, 0x36, 0x43, 0x6C, 0x36, 0x43, 0x1C, 0x7D, 0xE8, 0x09,
    0xE2, 0xF9, 0x06, 0xC6, 0x33, 0x57, 0xDC, 0xC5, 0x2D, 0x4B, 0x81, 0x8B, 0x49, 0x68, 0xCE, 0xD0,
    0xF8, 0xD7, 0x24, 0x5B, 0xED, 0xCD, 0x00, 0xFC, 0xEC, 0xA9, 0x97, 0xD1, 0xDC, 0x95, 0x83, 0xC3,
    0x15, 0x2A, 0x20, 0x72, 0x2E, 0xBF, 0x1D, 0xDD, 0x45, 0xE2, 0x40, 0x88, 0x03, 0xA1, 0xA3, 0xBB,
    0x98, 0x5D, 0x0D, 0xD9, 0xF7, 0x2B, 0xB0, 0x65, 0x2B, 0x20, 0x22, 0xC4, 0x71, 0x02, 0x40, 0x14,
    0xC5, 0x84, 0x8B, 0x21, 0x92, 0x45, 0x1C, 0x2E, 0x86, 0x44, 0x51, 0x7C, 0xCE, 0xA9, 0x67, 0xD3,
    0xF6, 0x6C, 0x99, 0x04, 0xD2, 0xAB, 0xEC, 0xD2, 0xE8, 0x34, 0x1A, 0x21, 0x5D, 0xDD, 0x9D, 0x04,
    0xBE, 0x4F, 0x18, 0xC5, 0x74, 0xF7, 0x9C, 0xEB, 0x63, 0x9A, 0xFF, 0x03, 0xDF, 0x23, 0x8C, 0x22,
    0xCA, 0x73, 0x0B, 0x04, 0xB9, 0xE0, 0x52, 0x6D, 0x59, 0xD3, 0x54, 0x0A, 0x83, 0x23, 0x53, 0x82,
    0xF4, 0xBD, 0x7A, 0xA7, 0x5E, 0x64, 0xD6, 0x1A, 0x2A, 0x73, 0x15, 0x3E, 0xF2, 0xB1, 0xBB, 0xF9,
    0xE1, 0xF7, 0xBE, 0xCA, 0xA2, 0x09, 0x11, 0x93, 0x4A, 0xDD, 0x59, 0x28, 0x02, 0xB0, 0x50, 0xAB,
    0xA6, 0x5B, 0xD4, 0x29, 0x1D, 0x2E, 0xE0, 0x81, 0x07, 0xBF, 0xC3, 0xEF, 0x1E, 0xDF, 0x45, 0xA9,
    0xBB, 0x44, 0xD2, 0xA6, 0x11, 0x01, 0x50, 0xA5, 0x6C, 0x44, 0xD9, 0x9B, 0x76, 0xCC, 0xDA, 0x7E,
    0xE6, 0x85, 0xCB, 0xB2, 0xC5, 0xCA, 0x6C, 0xB9, 0xCC, 0x6C, 0xB9, 0x7C, 0x9E, 0xDC, 0xCB, 0xAE,
    0x42, 0x87, 0x18, 0x45, 0xF4, 0x3F, 0x52, 0x5A, 0x35, 0xB2, 0x59, 0xAD, 0x3C, 0x89, 0xAA, 0xA3,
    0xDD, 0x7B, 0x40, 0x84, 0x30, 0x4B, 0x81, 0xEF, 0x79, 0xE9, 0x0B, 0x07, 0xB9, 0xB0, 0x1F, 0x41,
    0x11, 0x84, 0x28, 0x8E, 0x97, 0x91, 0x02, 0x4D, 0x44, 0x3C, 0xEB, 0x92, 0x70, 0xAB, 0x00, 0x14,
    0x06, 0x47, 0x7E, 0x6A, 0xAC, 0xFF, 0x79, 0x4D, 0xA2, 0x10, 0x08, 0x5A, 0x2D, 0x69, 0x16, 0xE1,
    0xE5, 0x8A, 0x4B, 0x44, 0x2E, 0x57, 0x84, 0x91, 0x18, 0xEB, 0xBB, 0x24, 0x7E, 0xB6, 0xD6, 0xBD,
    0xB8, 0x45, 0xE0, 0x1E, 0xCB, 0x75, 0xCF, 0xFA, 0x85, 0xA8, 0xEF, 0x2F, 0xC6, 0xF3, 0xEE, 0xD0,
    0x24, 0xCE, 0x7A, 0x69, 0x59, 0xD2, 0xAA, 0x2F, 0x77, 0x8B, 0xB5, 0x00, 0xCF, 0x1E, 0x9A, 0xAA,
    0x62, 0x3C, 0xEB, 0xD4, 0x4D, 0x59, 0xC7, 0x96, 0x85, 0x97, 0x0F, 0x1E, 0x6B, 0x82, 0x28, 0x43,
    0x9B, 0xF2, 0x45, 0x8D, 0x7F, 0x82, 0x70, 0xBF, 0x20, 0x99, 0x93, 0xAB, 0xF7, 0x38, 0x45, 0xD2,
    0x94, 0xA9, 0x73, 0x4F, 0x8B, 0xE3, 0xFE, 0xCA, 0xE9, 0x43, 0x47, 0x49, 0xEF, 0xC9, 0xE6, 0x8C,
    0x14, 0xAD, 0x30, 0x78, 0xEB, 0xDD, 0x82, 0x7E, 0x4E, 0x61, 0x54, 0xA0, 0x8F, 0xAB, 0xF1, 0x3C,
    0x57, 0x66, 0x8D, 0x91, 0xE7, 0x1D, 0xFC, 0xB6, 0x76, 0xEA, 0xE0, 0x0E, 0x20, 0x26, 0x7B, 0x95,
    0xFF, 0x1F, 0x21, 0xBB, 0xDB, 0x18, 0x06, 0x04, 0x51, 0x08, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45,
    0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82, 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00,
    0x00, 0x0D, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x30, 0x08, 0x06,
    0x00, 0x00, 0x00, 0x57, 0x02, 0xF9, 0x87, 0x00, 0x00, 0x0A, 0x4F, 0x49, 0x44, 0x41, 0x54, 0x78,
    0x9C, 0xCD, 0x9A, 0x79, 0x8C, 0x5D, 0x55, 0x1D, 0xC7, 0x3F, 0xBF, 0x73, 0xCF, 0x7D, 0xEB, 0xCC,
    0xBC, 0xB6, 0xD3, 0x69, 0x3B, 0x9D, 0x5A, 0x9C, 0x69, 0x65, 0x2D, 0x6D, 0x69, 0x8B, 0x80, 0xAD,
    0x58, 0x08, 0x1A, 0x40, 0x10, 0x37, 0x34, 0x71, 0xC3, 0x25, 0x2E, 0x89, 0x31, 0xDA, 0x18, 0x45,
    0x8D, 0xC6, 0x3D, 0x41, 0x8D, 0x02, 0x31, 0x6A, 0x24, 0x6A, 0xE2, 0x9A, 0x28, 0x11, 0x12, 0x02,
    0x01, 0x91, 0x8A, 0x41, 0x2A, 0x88, 0x14, 0x6B, 0xA9, 0xD0, 0x16, 0xDA, 0xD2, 0x7D, 0x9D, 0xCE,
    0xBC, 0x99, 0x79, 0xCB, 0x7D, 0xF7, 0xDC, 0xF3, 0xF3, 0x8F, 0xFB, 0x66, 0x3A, 0x53, 0x85, 0xCE,
    0x1B, 0xDA, 0xD2, 0xDF, 0x1F, 0xEF, 0xBD, 0xDC, 0x7B, 0xDF, 0xEF, 0x7C, 0xBF, 0xE7, 0xFC, 0xB6,
    0xF3, 0x3B, 0x57, 0x98, 0x28, 0xD2, 0xFC, 0xD6, 0x52, 0x69, 0xC9, 0xB4, 0xB8, 0x3D, 0x7C, 0xBB,
    0xA8, 0x7F, 0x0B, 0xB0, 0x18, 0x95, 0x6E, 0x84, 0x1C, 0xA7, 0x47, 0xAA, 0x0A, 0x2F, 0x08, 0xF2,
    0x77, 0xD0, 0xDF, 0x55, 0xF6, 0xAE, 0x7F, 0x38, 0xBD, 0x7C, 0x63, 0x00, 0x77, 0x26, 0xFF, 0x0F,
    0x30, 0x80, 0x01, 0x3C, 0x40, 0x7E, 0xEE, 0xF2, 0x4F, 0x18, 0xE4, 0xB3, 0x18, 0x59, 0x28, 0x80,
    0xA2, 0xA0, 0x7A, 0x9A, 0xB0, 0x8F, 0x22, 0x13, 0x04, 0x41, 0x55, 0xBD, 0x57, 0xEE, 0x17, 0xF8,
    0x4A, 0x75, 0xDF, 0x93, 0xFF, 0x3A, 0x9E, 0x84, 0x8C, 0xFB, 0xD6, 0xF6, 0xB9, 0x17, 0x77, 0x7A,
    0x92, 0x5F, 0x49, 0x10, 0x5E, 0xAB, 0x9A, 0x80, 0xAA, 0x4F, 0x91, 0x8B, 0x61, 0x22, 0xD9, 0xD3,
    0x21, 0x0A, 0xEA, 0x41, 0x02, 0x31, 0x01, 0xEA, 0x7D, 0x5D, 0xF1, 0x9F, 0xAE, 0xEE, 0x7D, 0xEA,
    0x8E, 0xF1, 0x24, 0x64, 0x14, 0x58, 0xC7, 0xBC, 0x4B, 0xA7, 0x27, 0x1A, 0xFF, 0x49, 0x8C, 0x5D,
    0xA1, 0xDE, 0xC5, 0x40, 0x40, 0xBA, 0x2A, 0x67, 0x80, 0xA8, 0x03, 0xB1, 0x62, 0x02, 0xBC, 0x4B,
    0xBE, 0x50, 0xDD, 0xBF, 0xFE, 0x3B, 0xA3, 0x24, 0x0C, 0x7C, 0x55, 0x00, 0x49, 0x7C, 0xFC, 0xDB,
    0x71, 0xE0, 0x43, 0xCE, 0x18, 0xF0, 0x00, 0x62, 0x01, 0xAF, 0x3E, 0x49, 0x4C, 0x10, 0xDC, 0x52,
    0x9C, 0xB3, 0xE2, 0x7D, 0xE9, 0x0A, 0xDC, 0x18, 0x08, 0x40, 0xA1, 0x67, 0xC5, 0xC7, 0x8C, 0x09,
    0x7E, 0x3A, 0x0E, 0xFC, 0x99, 0x2A, 0xBE, 0x69, 0x30, 0x55, 0x67, 0x74, 0x69, 0xB4, 0x7B, 0xFD,
    0x76, 0x33, 0xBD, 0x6F, 0x79, 0x49, 0xD0, 0x2F, 0xAA, 0x7A, 0x4F, 0x6A, 0x36, 0x67, 0xB2, 0x18,
    0xF0, 0x5E, 0x4C, 0xD0, 0x16, 0x78, 0xFD, 0x36, 0xA0, 0x52, 0x98, 0x7B, 0xD1, 0x87, 0xC4, 0xD8,
    0x5F, 0x90, 0x12, 0x38, 0x39, 0x66, 0x23, 0x82, 0x1C, 0xE7, 0xF2, 0xAA, 0x9C, 0xAC, 0x48, 0x36,
    0xAA, 0xC4, 0xA9, 0xD7, 0xCB, 0xAC, 0x88, 0xBC, 0x55, 0x40, 0x35, 0x8D, 0x36, 0x53, 0x53, 0x29,
    0x20, 0x26, 0xFD, 0xAF, 0x26, 0x8A, 0x36, 0x1C, 0x89, 0xF3, 0xC7, 0x86, 0x12, 0x30, 0xD6, 0x20,
    0x61, 0x80, 0x04, 0xCD, 0xE7, 0xBC, 0x1E, 0xBB, 0xDF, 0xF2, 0x68, 0x9A, 0x88, 0xD8, 0x50, 0xC5,
    0xDD, 0x64, 0x41, 0x16, 0x2B, 0x2A, 0xCD, 0x50, 0xD9, 0xA2, 0x2A, 0x41, 0x8C, 0xA0, 0x2E, 0x21,
    0x1E, 0x6E, 0x20, 0x40, 0x50, 0xCC, 0x90, 0xED, 0xE9, 0x20, 0x33, 0xA3, 0x80, 0xC9, 0x59, 0x00,
    0x7C, 0xCD, 0xD1, 0x18, 0xA8, 0xD2, 0x38, 0x58, 0xC1, 0x95, 0x23, 0x14, 0x08, 0x0A, 0x21, 0x26,
    0x0C, 0x9A, 0x44, 0x5A, 0x65, 0x22, 0xA2, 0x78, 0x40, 0x56, 0x5B, 0x94, 0x39, 0xCD, 0xA9, 0x68,
    0x69, 0xFA, 0x25, 0x30, 0xF8, 0x86, 0xC3, 0x55, 0x62, 0xC2, 0x99, 0x05, 0x66, 0xAD, 0xEE, 0x63,
    0xC6, 0xCA, 0xB3, 0xE8, 0x58, 0xD2, 0x4D, 0x76, 0x56, 0x1B, 0x41, 0x7B, 0x16, 0x93, 0x49, 0x5D,
    0xCA, 0x37, 0x12, 0x92, 0xE1, 0x88, 0xE8, 0xE0, 0x08, 0x43, 0x1B, 0xF7, 0x73, 0x74, 0xDD, 0x4E,
    0x06, 0x1F, 0xDF, 0x4D, 0xE3, 0x70, 0x25, 0x25, 0x92, 0xB5, 0x68, 0xE2, 0x5B, 0x19, 0xDE, 0xA0,
    0x8A, 0x08, 0x7D, 0x52, 0xEC, 0x59, 0xDE, 0x12, 0x7D, 0x31, 0x82, 0xAA, 0xE2, 0x86, 0x22, 0x72,
    0xDD, 0x1D, 0x74, 0xBF, 0xFB, 0x42, 0x66, 0x5F, 0x77, 0x2E, 0xC5, 0xD7, 0xCC, 0x44, 0xAC, 0xC1,
    0x47, 0x0E, 0x1F, 0x27, 0xA8, 0xF3, 0xE0, 0x9B, 0xAA, 0x8D, 0x20, 0xD6, 0x60, 0xC2, 0x20, 0x05,
    0xEB, 0x3C, 0x95, 0xED, 0xFD, 0x1C, 0xBA, 0x67, 0x33, 0xFB, 0xEE, 0x7C, 0x9A, 0xFA, 0xEE, 0x32,
    0xB6, 0x23, 0x9B, 0xEA, 0xF6, 0xAD, 0xAD, 0x46, 0x4B, 0x04, 0x24, 0x30, 0x24, 0xD5, 0x06, 0x12,
    0x18, 0xE6, 0xBE, 0x67, 0x09, 0xF3, 0x3F, 0xBC, 0x82, 0x7C, 0xEF, 0x0C, 0x7C, 0x3D, 0x26, 0xA9,
    0xC4, 0x28, 0x4A, 0x90, 0xB5, 0x98, 0xAC, 0x45, 0xAC, 0x81, 0xA6, 0x5F, 0xE0, 0x15, 0x75, 0x1E,
    0x1F, 0x39, 0x92, 0xC8, 0x21, 0x08, 0xA6, 0x10, 0x12, 0xE4, 0x43, 0x6A, 0x3B, 0x07, 0xD8, 0xFD,
    0xF3, 0x27, 0xD9, 0xFB, 0xBB, 0x0D, 0xA8, 0xF3, 0x04, 0x85, 0x4C, 0x4B, 0xAB, 0x31, 0x69, 0x02,
    0x12, 0x08, 0xF1, 0x60, 0x9D, 0xF6, 0x0B, 0x66, 0xB3, 0xF0, 0xCB, 0x57, 0xD0, 0xF9, 0x86, 0x5E,
    0x92, 0x9A, 0x23, 0xA9, 0x34, 0x10, 0x6B, 0xB0, 0xED, 0x59, 0x00, 0xA2, 0xFD, 0x43, 0x54, 0x9E,
    0xEB, 0xA7, 0xB6, 0x73, 0x90, 0xB8, 0x5C, 0x07, 0x20, 0x2C, 0xE5, 0xC8, 0x9F, 0x35, 0x8D, 0xC2,
    0x82, 0x19, 0xE4, 0xE6, 0x76, 0x80, 0x08, 0x6E, 0x38, 0x6A, 0x02, 0x0E, 0x09, 0x0A, 0x21, 0x47,
    0x1F, 0xDD, 0xC9, 0x73, 0xDF, 0xF8, 0x0B, 0xC3, 0x9B, 0x0E, 0x10, 0x4E, 0xCB, 0xA1, 0xC9, 0xE4,
    0xE6, 0xF5, 0xC4, 0x04, 0x24, 0xFD, 0x70, 0xC3, 0x75, 0xE6, 0xBC, 0xED, 0x02, 0xCE, 0xFE, 0xDA,
    0x55, 0x84, 0x33, 0xF2, 0xB8, 0xC1, 0x14, 0x9C, 0x2D, 0xE5, 0xF0, 0x75, 0x47, 0xFF, 0xDF, 0x76,
    0x70, 0xE8, 0x9E, 0xCD, 0x94, 0x9F, 0xDA, 0x4B, 0x74, 0x70, 0x04, 0xDF, 0x48, 0x60, 0x74, 0x26,
    0x03, 0x83, 0xC9, 0x04, 0x64, 0x66, 0x16, 0x28, 0xAD, 0x98, 0xC7, 0xEC, 0x1B, 0xCE, 0xA3, 0xF3,
    0xF5, 0xBD, 0x98, 0x9C, 0xC5, 0x95, 0x8F, 0xE9, 0x89, 0x07, 0x6A, 0x6C, 0xFD, 0xDA, 0x43, 0x1C,
    0xB8, 0xFB, 0x3F, 0xD8, 0xF6, 0x1C, 0x69, 0x39, 0xF4, 0x72, 0x08, 0x08, 0xE0, 0x95, 0xA4, 0xE6,
    0xE8, 0xFD, 0xCC, 0x4A, 0x7A, 0x3F, 0xB3, 0x12, 0x5F, 0x77, 0x24, 0xB5, 0x98, 0x20, 0x6F, 0x31,
    0xF9, 0x90, 0xFE, 0x87, 0xB7, 0xB3, 0xF3, 0x27, 0xFF, 0xA0, 0xFC, 0xC4, 0x1E, 0x34, 0x49, 0x67,
    0x54, 0x32, 0xC1, 0x84, 0x5C, 0x30, 0x9A, 0x03, 0x34, 0x4E, 0x48, 0xAA, 0x31, 0x62, 0x0C, 0xA5,
    0xD7, 0xCE, 0xE3, 0xAC, 0x4F, 0x5C, 0x42, 0xE7, 0x15, 0x7D, 0xA9, 0x09, 0xD6, 0x5C, 0xAA, 0x33,
    0x17, 0xB2, 0xE3, 0xB6, 0x75, 0xEC, 0xB8, 0x7D, 0x1D, 0x41, 0xCE, 0xA6, 0x66, 0xF8, 0x12, 0x24,
    0x4E, 0x48, 0x40, 0x63, 0xCF, 0xC2, 0x2F, 0xAD, 0xE6, 0x55, 0x1F, 0xB9, 0x98, 0x78, 0xB0, 0x06,
    0x89, 0x62, 0x3B, 0xB2, 0xC4, 0x03, 0x35, 0xB6, 0x7F, 0xFF, 0x6F, 0xEC, 0xFB, 0xC3, 0x46, 0x34,
    0x51, 0xC2, 0x8E, 0x1C, 0x9A, 0x78, 0x5C, 0xA5, 0x81, 0x3A, 0x9F, 0xE6, 0x85, 0x71, 0x0C, 0xD4,
    0x6B, 0x6A, 0x6A, 0xC5, 0x0C, 0x12, 0x18, 0xE2, 0xA1, 0x08, 0x31, 0xD0, 0xFD, 0xCE, 0x0B, 0xE9,
    0xFB, 0xFC, 0xE5, 0x64, 0xA6, 0xE7, 0x71, 0x43, 0x11, 0x04, 0x42, 0x38, 0x3D, 0xCF, 0xEE, 0x5F,
    0x3C, 0xC9, 0xF3, 0xDF, 0x7A, 0x18, 0x09, 0x83, 0x97, 0x0C, 0xB3, 0x2F, 0x4A, 0x40, 0x02, 0xC1,
    0x95, 0x23, 0xBA, 0xAE, 0x3E, 0x9B, 0xC5, 0x3F, 0x7F, 0x3B, 0x71, 0x7F, 0x15, 0x10, 0xC2, 0x19,
    0x79, 0xCA, 0xFF, 0xDA, 0xC7, 0xE6, 0x9B, 0x1F, 0x60, 0xF8, 0xE9, 0x03, 0x84, 0x9D, 0x85, 0x14,
    0x78, 0x39, 0xC2, 0x96, 0x72, 0x74, 0x2C, 0x99, 0x43, 0xC7, 0x92, 0x6E, 0x0A, 0x0B, 0x3A, 0xC9,
    0x4C, 0xCF, 0x03, 0xD0, 0x18, 0xA8, 0x51, 0xDD, 0xD6, 0xCF, 0xD0, 0xBF, 0xF7, 0x33, 0xBC, 0xF1,
    0x00, 0xF1, 0x60, 0x3D, 0x8D, 0x3A, 0xD6, 0x10, 0xF7, 0x57, 0x69, 0xBF, 0x70, 0x0E, 0xE7, 0xDE,
    0x72, 0x35, 0xA5, 0x8B, 0xE6, 0x12, 0x0F, 0xD4, 0x00, 0x25, 0xEC, 0x2C, 0xB0, 0xF1, 0x23, 0x77,
    0x71, 0xF8, 0x81, 0xAD, 0xD8, 0x52, 0xF6, 0x45, 0x7D, 0xC2, 0xBE, 0xF8, 0xE2, 0x8C, 0x8A, 0x22,
    0x46, 0x08, 0x0A, 0x19, 0x82, 0x62, 0x86, 0x43, 0xF7, 0x3E, 0xCB, 0xB3, 0x9F, 0xBB, 0x1F, 0x37,
    0xD2, 0x20, 0xD3, 0x55, 0x24, 0x3E, 0x5A, 0x23, 0x9C, 0x51, 0x60, 0xFE, 0xC7, 0x17, 0x33, 0xE7,
    0x86, 0x0B, 0x28, 0x9E, 0xDB, 0x45, 0x90, 0xB7, 0xE9, 0x80, 0xE3, 0x32, 0xB1, 0x04, 0x42, 0x52,
    0x73, 0x54, 0xB7, 0x1E, 0x61, 0xFF, 0xDD, 0xFF, 0xE1, 0xC0, 0x1F, 0x37, 0x11, 0x1F, 0xAD, 0x91,
    0xE9, 0x2A, 0x52, 0xD9, 0x7A, 0x84, 0x0D, 0x1F, 0xF8, 0x03, 0xE7, 0x7F, 0xEF, 0x1A, 0x66, 0x5D,
    0x77, 0x5E, 0x1A, 0x18, 0x4C, 0xBA, 0x95, 0x3A, 0x91, 0x9C, 0xD0, 0x89, 0xD5, 0x2B, 0xAF, 0xFE,
    0xE4, 0xA5, 0xCC, 0xBA, 0xE6, 0x1C, 0x8E, 0xFC, 0xF9, 0x79, 0xB6, 0xDF, 0xFA, 0x28, 0x62, 0x24,
    0x35, 0x83, 0xE1, 0x88, 0x59, 0x6F, 0x3E, 0x87, 0xBE, 0x35, 0xAB, 0x68, 0x3B, 0xAF, 0x0B, 0x1F,
    0x25, 0x24, 0xD5, 0x06, 0x9A, 0xA4, 0xE6, 0x32, 0x56, 0x5E, 0x34, 0xC3, 0xA8, 0x04, 0xE9, 0x44,
    0x98, 0x6C, 0xC0, 0xC8, 0xB3, 0x87, 0xD9, 0x7E, 0xEB, 0xA3, 0x1C, 0xBE, 0x6F, 0x0B, 0xB6, 0x3D,
    0x8B, 0x26, 0x1E, 0xF5, 0x4A, 0xDF, 0x9A, 0x55, 0xCC, 0x7C, 0xE3, 0x42, 0x0E, 0xDD, 0xBF, 0x85,
    0x17, 0x7E, 0xF4, 0xF8, 0x98, 0x8E, 0x29, 0x13, 0x40, 0x95, 0xA4, 0xEE, 0x30, 0x59, 0x8B, 0xAF,
    0x3B, 0x82, 0x62, 0x88, 0xC6, 0x1E, 0x09, 0x0C, 0x0B, 0x6E, 0xBE, 0x9C, 0x79, 0x37, 0x2D, 0x43,
    0x13, 0xC5, 0x8D, 0x44, 0x88, 0x11, 0x6C, 0x7B, 0x16, 0x09, 0x0C, 0x6E, 0xA8, 0x4E, 0x52, 0x8D,
    0x81, 0xB4, 0x6C, 0xB0, 0xA3, 0x3E, 0x32, 0x1C, 0xA1, 0x5E, 0xB1, 0x6D, 0x59, 0x24, 0x10, 0xF6,
    0xFC, 0xEA, 0x29, 0xB6, 0xDD, 0xF2, 0x08, 0x9A, 0x78, 0x24, 0x34, 0x24, 0x95, 0x18, 0x93, 0xB3,
    0xF8, 0xC8, 0xA5, 0x4E, 0x7C, 0x7C, 0x55, 0xD8, 0x32, 0x01, 0x18, 0xCB, 0x90, 0x12, 0x08, 0xBE,
    0xE1, 0xB1, 0xA5, 0x2C, 0xE7, 0x7F, 0xFF, 0xCD, 0xCC, 0xBC, 0xB2, 0x8F, 0x46, 0x7F, 0x0D, 0xBC,
    0x4F, 0x67, 0x51, 0x61, 0xE0, 0xB1, 0x5D, 0x1C, 0x7E, 0x60, 0x0B, 0xC3, 0x9B, 0x0E, 0xD2, 0x38,
    0x5C, 0x05, 0x20, 0xD3, 0x55, 0xA0, 0x7D, 0xD1, 0x6C, 0xBA, 0xAE, 0x3E, 0x9B, 0xE9, 0x97, 0x9D,
    0x85, 0x08, 0xB8, 0xE1, 0x08, 0x8C, 0x21, 0xD3, 0x99, 0xE7, 0xC8, 0x5F, 0xB6, 0xF3, 0xCC, 0x67,
    0xEF, 0xC3, 0x95, 0x23, 0x4C, 0xC6, 0xA4, 0x2B, 0x38, 0xC9, 0xAC, 0xDC, 0x5A, 0x26, 0x36, 0x42,
    0x52, 0x8D, 0x39, 0xFF, 0xF6, 0xEB, 0xE8, 0x7E, 0xC7, 0x22, 0xA2, 0x83, 0x23, 0x88, 0x35, 0x84,
    0xA5, 0x1C, 0x43, 0xFF, 0xDE, 0xCF, 0x8E, 0x5B, 0xD7, 0xD1, 0xFF, 0xC8, 0x0E, 0x7C, 0xDD, 0x61,
    0x32, 0x41, 0x9A, 0x8D, 0x21, 0xCD, 0xC2, 0x8D, 0x04, 0x93, 0x0B, 0xE8, 0xBC, 0xBC, 0x8F, 0xDE,
    0x35, 0x2B, 0xE9, 0x58, 0xD2, 0x4D, 0x5C, 0xAE, 0xA3, 0xCE, 0x93, 0x9D, 0xDD, 0xC6, 0xFE, 0x3F,
    0x6E, 0xE2, 0x99, 0x4F, 0xDF, 0x4B, 0x50, 0x08, 0x5B, 0x2A, 0x27, 0x26, 0xE1, 0xC4, 0xC7, 0x44,
    0xBD, 0x22, 0xA1, 0xA1, 0xB8, 0xB0, 0x13, 0x1F, 0xA5, 0x66, 0x15, 0x14, 0x43, 0xF6, 0xFC, 0x72,
    0x3D, 0xDB, 0xBE, 0xF3, 0x08, 0xF1, 0x50, 0x44, 0xD8, 0x91, 0x25, 0xC8, 0x37, 0x37, 0x75, 0x63,
    0x4D, 0x1A, 0x08, 0x8A, 0x80, 0x2A, 0x87, 0x1F, 0x7A, 0x9E, 0xC1, 0x27, 0x76, 0xB3, 0xE0, 0x73,
    0x97, 0xD3, 0xF3, 0xC1, 0x65, 0x24, 0x95, 0x18, 0x1F, 0x39, 0x8A, 0x0B, 0x3B, 0x91, 0xD0, 0xB4,
    0x5C, 0x0B, 0xB5, 0x54, 0x42, 0x8B, 0x11, 0xB4, 0xE1, 0xD9, 0x75, 0xC7, 0x13, 0xB8, 0x72, 0x1D,
    0x5F, 0x8F, 0x79, 0xEE, 0xEB, 0x6B, 0xD9, 0xFC, 0xA5, 0x07, 0xF1, 0xCE, 0x13, 0x96, 0x72, 0x63,
    0xE5, 0x71, 0x52, 0x8F, 0x89, 0xFB, 0xAB, 0xC4, 0xFD, 0x55, 0x92, 0x7A, 0x3C, 0x96, 0x0B, 0xC2,
    0x52, 0x0E, 0xEF, 0x3C, 0x9B, 0xBF, 0xFC, 0x20, 0xCF, 0x7D, 0x7D, 0x2D, 0xBE, 0x1E, 0xE3, 0xCA,
    0x75, 0x76, 0xDD, 0xF1, 0x04, 0xDA, 0xF0, 0x27, 0x74, 0xDA, 0xFF, 0xC1, 0xD4, 0x6A, 0x35, 0x0A,
    0xE0, 0xEB, 0x8E, 0x5C, 0x4F, 0x07, 0x08, 0xD4, 0x76, 0x95, 0x09, 0x8A, 0x99, 0xF4, 0x86, 0x2A,
    0x18, 0xC1, 0x57, 0x62, 0x0A, 0x0B, 0x66, 0xD0, 0x79, 0xE5, 0x02, 0x40, 0x39, 0xB2, 0x76, 0x1B,
    0xB5, 0xED, 0x03, 0x98, 0x62, 0x98, 0x56, 0xA8, 0x4D, 0xC7, 0x4C, 0x2A, 0x0D, 0xF2, 0xF3, 0x4B,
    0xA0, 0x50, 0xDF, 0x3B, 0x34, 0xB6, 0x7F, 0x38, 0xE5, 0x04, 0xC4, 0x08, 0x3E, 0x4A, 0x00, 0x4D,
    0xCB, 0xE3, 0xE6, 0xB2, 0x8B, 0x11, 0x5C, 0x25, 0x66, 0xFA, 0xEB, 0xE6, 0xB3, 0xE8, 0x87, 0xD7,
    0x93, 0xE9, 0x6A, 0x03, 0xA0, 0x71, 0x68, 0x84, 0x4D, 0x9F, 0xBA, 0x87, 0x81, 0xC7, 0x76, 0x63,
    0x8B, 0xE1, 0x84, 0xE7, 0x7D, 0xE4, 0x00, 0xC1, 0x64, 0x83, 0x96, 0xCD, 0x07, 0xA6, 0xB8, 0x07,
    0x56, 0xAF, 0x48, 0x26, 0x40, 0x32, 0x76, 0xC2, 0xA0, 0xAA, 0x8A, 0xC9, 0x18, 0xFA, 0xD6, 0xAC,
    0x24, 0x33, 0xB3, 0x48, 0xE3, 0x48, 0x85, 0xC6, 0x91, 0x0A, 0x99, 0xAE, 0x22, 0xBD, 0x6B, 0x56,
    0xA5, 0x11, 0x66, 0x5C, 0x59, 0x90, 0xEA, 0xB1, 0x48, 0x66, 0x6A, 0xE0, 0xA7, 0x4C, 0xA0, 0x89,
    0x76, 0x62, 0x8D, 0x22, 0x69, 0xB4, 0xB1, 0xD3, 0xF2, 0xE4, 0x7A, 0x4A, 0xB8, 0x91, 0x06, 0x36,
    0x13, 0x60, 0x33, 0x01, 0x6E, 0xA4, 0x41, 0x7E, 0x5E, 0x09, 0x3B, 0x2D, 0x9F, 0x6E, 0x74, 0xE4,
    0x25, 0xF4, 0x9C, 0x36, 0x02, 0xC7, 0x8B, 0x82, 0x58, 0x83, 0x1B, 0xAC, 0x51, 0xDB, 0x53, 0x26,
    0x6C, 0xCF, 0x52, 0x73, 0x8E, 0x9A, 0x73, 0xE9, 0xEF, 0x3D, 0x65, 0xDC, 0x60, 0x2D, 0x0D, 0xAD,
    0x27, 0xB1, 0xCD, 0x7A, 0x52, 0xBB, 0x6F, 0x22, 0x69, 0x94, 0x7A, 0xE1, 0xB6, 0x75, 0x0C, 0xEF,
    0x1D, 0xA0, 0x2F, 0xDF, 0x49, 0x5F, 0xBE, 0x93, 0xE1, 0x3D, 0x03, 0xEC, 0xB8, 0x6D, 0x5D, 0x1A,
    0x65, 0x4E, 0x90, 0x59, 0x5B, 0x95, 0x93, 0x4A, 0x40, 0xBD, 0x12, 0xB6, 0x67, 0xD9, 0xB7, 0x76,
    0x0B, 0xED, 0x3F, 0xDA, 0xC6, 0x77, 0x67, 0x5E, 0xCB, 0x77, 0x67, 0x5E, 0x4B, 0xFB, 0x8F, 0xB7,
    0xB1, 0x7F, 0xED, 0x16, 0xC2, 0xF6, 0xEC, 0x94, 0x6D, 0xFD, 0xC5, 0xE4, 0xA4, 0xF7, 0x3F, 0xD3,
    0x0D, 0x9C, 0xB2, 0x74, 0x7E, 0x2F, 0x05, 0xB2, 0x14, 0xC8, 0xB2, 0x74, 0x7E, 0x2F, 0x88, 0x9E,
    0x92, 0xF6, 0x76, 0xEB, 0x81, 0x77, 0x32, 0xA2, 0x8A, 0x13, 0xC5, 0x37, 0x8D, 0xDD, 0xC9, 0xA9,
    0x3B, 0x5F, 0x38, 0x35, 0x04, 0x18, 0xD7, 0xB7, 0x87, 0x53, 0x7A, 0xB0, 0xF0, 0xB2, 0x09, 0x18,
    0x63, 0x30, 0xE3, 0xD2, 0xBF, 0xB5, 0x01, 0xD6, 0x5A, 0x8C, 0x31, 0x13, 0x9E, 0xB1, 0xD6, 0x62,
    0xED, 0xC4, 0xDE, 0xB1, 0xF7, 0x8A, 0xF7, 0x2D, 0x35, 0xB4, 0xFE, 0x47, 0x5E, 0x16, 0x01, 0x11,
    0x61, 0x64, 0xB8, 0x82, 0x46, 0x8D, 0x63, 0x17, 0xC3, 0x00, 0x86, 0xFB, 0xA9, 0x54, 0x6A, 0x48,
    0x73, 0xEE, 0x2B, 0x95, 0x1A, 0x6E, 0xF8, 0x08, 0xFD, 0xA2, 0x10, 0x1F, 0x3B, 0xE2, 0x92, 0x6C,
    0x86, 0x62, 0x5B, 0x61, 0x42, 0x72, 0x6B, 0x55, 0x2C, 0x50, 0x01, 0x8A, 0x53, 0x01, 0x1F, 0xD5,
    0x23, 0xDE, 0x78, 0xF5, 0x2A, 0x2E, 0xBD, 0x78, 0x29, 0x0D, 0x75, 0x88, 0xA4, 0xB3, 0x1D, 0x55,
    0x6A, 0xAC, 0xBA, 0xF2, 0x12, 0xEA, 0x44, 0x00, 0xAC, 0xBE, 0xF2, 0x12, 0xEC, 0xB7, 0x6E, 0x26,
    0x5B, 0xCC, 0xE3, 0xBD, 0x47, 0x15, 0x32, 0x62, 0x79, 0xFC, 0x9F, 0x1B, 0x78, 0xF8, 0xA1, 0xC7,
    0xC8, 0xE6, 0xB2, 0x53, 0x23, 0xA1, 0xD4, 0xA5, 0xD0, 0xB3, 0xEC, 0x69, 0x11, 0xB3, 0xA8, 0xA9,
    0x61, 0x52, 0xE6, 0x6A, 0x8C, 0xA1, 0x5A, 0xA9, 0xB1, 0x78, 0xE9, 0x39, 0x3C, 0xF8, 0xD0, 0x2F,
    0x69, 0x93, 0x22, 0x09, 0x7E, 0x9C, 0xCD, 0x1B, 0x6A, 0xD4, 0xA8, 0xBA, 0xB4, 0xE7, 0x53, 0xB0,
    0x39, 0xF2, 0xE4, 0x49, 0x1B, 0xB2, 0x69, 0x1E, 0x0B, 0x30, 0x8C, 0x68, 0x85, 0x37, 0x5D, 0x75,
    0x13, 0x1B, 0x37, 0x6C, 0xA1, 0xD0, 0x24, 0x37, 0x69, 0xE8, 0x22, 0x82, 0xEA, 0x0E, 0x8B, 0xF2,
    0x57, 0x11, 0xB3, 0x48, 0x71, 0x1E, 0xE4, 0x24, 0x1C, 0x70, 0x08, 0x8A, 0xA7, 0x40, 0x9E, 0x36,
    0x9B, 0x76, 0x25, 0x3C, 0xE0, 0xD3, 0x6E, 0x32, 0x27, 0x27, 0x0D, 0xAB, 0x17, 0x8C, 0xF1, 0xEA,
    0x37, 0x4A, 0xA1, 0x7B, 0xF9, 0x32, 0x31, 0xF2, 0x38, 0xE8, 0xA8, 0x3F, 0x4C, 0x6A, 0x15, 0x46,
    0x4D, 0xE8, 0x8A, 0xAB, 0x2E, 0x9B, 0x60, 0x42, 0x93, 0x1A, 0xFE, 0xE5, 0x9B, 0x90, 0x47, 0x8C,
    0x51, 0xEF, 0x3F, 0xD8, 0x3C, 0x23, 0x5B, 0xFE, 0x7B, 0x63, 0xEC, 0xBB, 0xD4, 0xC7, 0xAE, 0x79,
    0xA0, 0x36, 0x29, 0x11, 0x11, 0x2A, 0x23, 0xD5, 0x89, 0x4E, 0xDC, 0x82, 0x4C, 0xD1, 0x89, 0x3D,
    0x62, 0xC0, 0x27, 0xBB, 0xB2, 0x75, 0xBB, 0x44, 0x00, 0xC9, 0xBE, 0x6A, 0x79, 0x9F, 0xF5, 0xB2,
    0x01, 0x28, 0x34, 0x97, 0x78, 0xD2, 0x19, 0xFA, 0xF8, 0x30, 0xDA, 0x12, 0x92, 0xA9, 0x85, 0xD1,
    0x58, 0x8C, 0x0D, 0xBD, 0xBA, 0x8F, 0x56, 0xF7, 0xAC, 0xFF, 0x99, 0x8C, 0x9E, 0xB7, 0x16, 0x7B,
    0x56, 0xBC, 0x4F, 0xC4, 0xFC, 0x5A, 0x35, 0x49, 0x48, 0xCD, 0xE8, 0x0C, 0x3A, 0x66, 0x1D, 0x93,
    0x58, 0x8C, 0x0D, 0x93, 0xC4, 0xDD, 0x57, 0xDB, 0xB7, 0xFE, 0x7A, 0xB8, 0xD1, 0x98, 0xD1, 0xF3,
    0xD6, 0xCA, 0xDE, 0x27, 0x7F, 0xE3, 0x93, 0xE4, 0x0B, 0x62, 0x82, 0xE6, 0x01, 0xB7, 0xBA, 0x57,
    0x1A, 0xED, 0x38, 0xF1, 0x34, 0xC1, 0xAB, 0x77, 0xFF, 0xCC, 0xD8, 0xFC, 0xFB, 0xD3, 0xCB, 0x77,
    0xFA, 0x71, 0x6B, 0x9F, 0xAE, 0x44, 0xA1, 0x67, 0xD9, 0xC7, 0x04, 0x73, 0xBB, 0x18, 0x93, 0x53,
    0x9F, 0x00, 0x9A, 0xBC, 0xC2, 0xAF, 0x1A, 0x08, 0x22, 0x46, 0x24, 0x40, 0xBD, 0xBB, 0x37, 0x90,
    0xDA, 0x4D, 0x43, 0x7B, 0x9E, 0x39, 0xDA, 0xC4, 0xA3, 0xC7, 0x81, 0x6A, 0x92, 0x98, 0xBB, 0xE2,
    0x22, 0x85, 0x6F, 0x1A, 0xE1, 0x1A, 0x11, 0x31, 0xAF, 0xE8, 0xCB, 0x1E, 0x00, 0xAA, 0x5B, 0x3D,
    0xFA, 0x83, 0xDA, 0xDE, 0xF5, 0x3F, 0x6D, 0xDE, 0x1D, 0x7B, 0x31, 0xE5, 0xFF, 0xCC, 0xEA, 0xB1,
    0x17, 0x29, 0x8A, 0x3D, 0xCB, 0x57, 0x83, 0xBC, 0x57, 0xD1, 0xD7, 0x09, 0xBC, 0x1A, 0x28, 0x9C,
    0x06, 0xE8, 0xA0, 0x5A, 0x07, 0xF6, 0x21, 0xB2, 0x51, 0xC5, 0xDF, 0x93, 0xAD, 0x98, 0xBB, 0x06,
    0x06, 0xD6, 0x97, 0xC7, 0xE1, 0x1D, 0x9B, 0xCD, 0xFF, 0x02, 0xE4, 0x97, 0xFB, 0x15, 0x78, 0x98,
    0x39, 0x6E, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82, 0x89, 0x50,
    0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00,
    0x00, 0x40, 0x00, 0x00, 0x00, 0x40, 0x08, 0x06, 0x00, 0x00, 0x00, 0xAA, 0x69, 0x71, 0xDE, 0x00,
    0x00, 0x0E, 0x45, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9C, 0xDD, 0x9C, 0x79, 0x8C, 0x5D, 0xD5, 0x7D,
    0xC7, 0x3F, 0xBF, 0x73, 0xEE, 0x7D, 0xEB, 0xCC, 0x9B, 0xC1, 0xC3, 0x8C, 0xED, 0x19, 0x1B, 0xB0,
    0x41, 0x18, 0x8C, 0x31, 0x98, 0x61, 0x6B, 0xDA, 0x04, 0x50, 0xF8, 0x03, 0x08, 0xA8, 0x69, 0x09,
    0x42, 0x94, 0x22, 0x68, 0x68, 0x2B, 0x1A, 0xA9, 0x55, 0x69, 0x70, 0x5B, 0x25, 0x52, 0x11, 0xAD,
    0x54, 0x29, 0x01, 0xDA, 0x28, 0x6D, 0x05, 0x6A, 0x04, 0x4A, 0x44, 0x69, 0x48, 0x59, 0xDA, 0x34,
    0x02, 0x27, 0x69, 0x13, 0x4A, 0x9A, 0x22, 0x30, 0xDE, 0x70, 0x88, 0x17, 0x0C, 0xD8, 0x78, 0x1B,
    0xC6, 0xE3, 0x65, 0xB6, 0xB7, 0xDE, 0x7B, 0xCE, 0xAF, 0x7F, 0xDC, 0x3B, 0x9B, 0x3D, 0x0E, 0x1E,
    0xCF, 0xF3, 0x18, 0xF8, 0x8D, 0x9E, 0xE6, 0xCD, 0x3B, 0xF7, 0x9C, 0xF7, 0x3B, 0xBF, 0xF5, 0xFB,
    0x3B, 0xCB, 0x08, 0xC7, 0xA5, 0xDB, 0x2C, 0x3C, 0xEB, 0x00, 0x98, 0x77, 0x65, 0xA9, 0x98, 0xF3,
    0x37, 0x29, 0x7A, 0x9D, 0xC0, 0x2A, 0x15, 0xCE, 0x11, 0x68, 0x07, 0xC2, 0xE3, 0xF7, 0x9F, 0x53,
    0xAA, 0x03, 0x87, 0x55, 0xD9, 0xAA, 0xF8, 0xD7, 0x42, 0x63, 0xBE, 0x3F, 0xBC, 0x77, 0xFD, 0xDA,
    0xB4, 0xCD, 0x00, 0x9A, 0xBE, 0x8E, 0x21, 0x39, 0xCE, 0x67, 0x02, 0xF8, 0x62, 0xD7, 0x8A, 0xF9,
    0x1A, 0xE4, 0xEE, 0x43, 0xF4, 0x6E, 0x11, 0x59, 0x22, 0x62, 0x50, 0xF5, 0xE9, 0x50, 0xD3, 0x8E,
    0x77, 0x1A, 0x49, 0x40, 0x04, 0x41, 0x50, 0x75, 0x75, 0x54, 0xFE, 0x1B, 0xDC, 0xDF, 0x97, 0xF7,
    0x6F, 0xFC, 0x49, 0xD2, 0xFE, 0xA0, 0x81, 0x87, 0xFC, 0x34, 0xBD, 0xA6, 0xFD, 0x5B, 0x8B, 0x3D,
    0xAB, 0x7E, 0x17, 0xCC, 0xDF, 0x8A, 0x09, 0x16, 0x27, 0x93, 0xF6, 0x0A, 0xEA, 0x41, 0xC6, 0x04,
    0x34, 0x9D, 0xF0, 0x4E, 0x27, 0xA5, 0x5A, 0x56, 0x05, 0xB1, 0x62, 0x6C, 0xAA, 0x2C, 0xFD, 0xB6,
    0x78, 0x56, 0x8F, 0xF6, 0xAD, 0x3F, 0x08, 0x58, 0xC0, 0x4D, 0xEE, 0x64, 0x26, 0xBD, 0x1F, 0x9B,
    0x90, 0x69, 0xE9, 0xE9, 0xFD, 0x47, 0x4C, 0xF8, 0x14, 0x62, 0x16, 0xAB, 0x8F, 0xE2, 0x64, 0xF2,
    0x08, 0x88, 0x4D, 0xFB, 0x7C, 0xD4, 0x26, 0x0F, 0x09, 0x4F, 0x26, 0xE5, 0x51, 0xD5, 0xC7, 0x0E,
    0x55, 0x2F, 0x26, 0xB8, 0xC7, 0x1B, 0xFE, 0xA7, 0xD8, 0xB3, 0x6A, 0x25, 0xE0, 0x12, 0xD7, 0x9E,
    0xDA, 0x29, 0xFD, 0xFD, 0xA0, 0x70, 0xDB, 0x16, 0x29, 0xBC, 0xFA, 0xEE, 0x53, 0xC6, 0x84, 0x77,
    0xA8, 0x8F, 0x62, 0x10, 0xC3, 0x54, 0x21, 0x7D, 0x0C, 0x49, 0x63, 0x24, 0x08, 0x54, 0x7D, 0xBF,
    0x0A, 0xB7, 0x54, 0xF7, 0xAE, 0x7B, 0x63, 0x72, 0x7C, 0x4B, 0x05, 0x90, 0xF8, 0x47, 0xA1, 0xA7,
    0xF7, 0x9F, 0xC4, 0xD8, 0x2F, 0xE1, 0x5D, 0x0C, 0x04, 0xA7, 0x91, 0xEB, 0x26, 0x93, 0x3A, 0xC4,
    0x5A, 0x55, 0xDF, 0x1F, 0xE3, 0x3E, 0xDD, 0xD8, 0xB7, 0x69, 0x07, 0x89, 0x62, 0xBD, 0x90, 0xFA,
    0x45, 0x71, 0xE1, 0x65, 0x77, 0x4A, 0x10, 0xFC, 0x8B, 0xFA, 0x38, 0x4E, 0xCD, 0xE8, 0xA3, 0x68,
    0xE6, 0xB3, 0x20, 0x75, 0x22, 0x81, 0x55, 0xF5, 0x1B, 0xCA, 0x71, 0xE3, 0x33, 0xF4, 0x6F, 0xAE,
    0x02, 0x6A, 0x00, 0xDF, 0xB2, 0xE0, 0xD2, 0x4E, 0x0C, 0x8F, 0xA8, 0x7A, 0x4D, 0xCD, 0xFE, 0x13,
    0x36, 0x79, 0x00, 0xB1, 0xAA, 0x71, 0x2C, 0x62, 0x2F, 0x2B, 0x98, 0xF0, 0x21, 0xC0, 0x93, 0x4E,
    0x56, 0x09, 0x82, 0x2F, 0x89, 0x09, 0x17, 0xA0, 0xDE, 0xF1, 0xB1, 0xF7, 0xF9, 0x5F, 0x45, 0x62,
    0x55, 0x63, 0x2F, 0x46, 0xFE, 0xA4, 0xD0, 0x73, 0xE9, 0x25, 0x80, 0x37, 0xF3, 0xE6, 0x5D, 0x59,
    0x52, 0xD5, 0xDF, 0x53, 0xEF, 0xC6, 0xB4, 0xFF, 0x49, 0x26, 0x01, 0x55, 0x31, 0x26, 0x44, 0x65,
    0x35, 0x80, 0x14, 0xBB, 0x57, 0xDD, 0x8E, 0xB1, 0xCF, 0xA0, 0x9A, 0xA6, 0xBA, 0xB9, 0x61, 0x63,
    0x5A, 0x9A, 0x1B, 0x6C, 0xA5, 0x80, 0x28, 0x3A, 0xEA, 0x44, 0x57, 0x05, 0x1E, 0xF3, 0x59, 0x8B,
    0x41, 0x89, 0x1D, 0xC8, 0xA9, 0x89, 0xFC, 0x02, 0x62, 0x12, 0xEC, 0xA4, 0x5E, 0xC1, 0x6B, 0x22,
    0x6F, 0x9D, 0xD4, 0x2E, 0x02, 0x46, 0xD2, 0xE7, 0x34, 0xC5, 0x5D, 0xA7, 0x8A, 0x1B, 0xF5, 0xC6,
    0xD8, 0x16, 0x62, 0x7F, 0x6B, 0x60, 0xD0, 0x4B, 0x75, 0x22, 0x1E, 0x34, 0xF7, 0x9B, 0x8C, 0x24,
    0xF0, 0x2C, 0x72, 0xB8, 0x5A, 0x8C, 0xC6, 0x1E, 0x93, 0x0B, 0xB1, 0xF9, 0x00, 0x13, 0x58, 0x24,
    0x48, 0xBE, 0x52, 0x63, 0x8F, 0xC6, 0x1E, 0x57, 0x8D, 0xF1, 0xB5, 0x08, 0x09, 0x0C, 0x26, 0x17,
    0x60, 0x42, 0x9B, 0xB2, 0xDB, 0x74, 0x61, 0x28, 0x0A, 0x22, 0x5C, 0x13, 0x20, 0xB2, 0x04, 0x55,
    0x68, 0x56, 0xF0, 0x4B, 0xB5, 0xAD, 0xB1, 0x12, 0x8F, 0xD4, 0x11, 0x6B, 0xC8, 0x74, 0x15, 0x69,
    0xBB, 0xB4, 0x9B, 0x96, 0xE5, 0x5D, 0xE4, 0xCF, 0x6E, 0x27, 0xB7, 0xB8, 0x8D, 0xA0, 0x35, 0x8B,
    0x2D, 0x66, 0x00, 0x70, 0xE5, 0x06, 0xF1, 0x48, 0x9D, 0xDA, 0x9E, 0x21, 0xAA, 0xEF, 0x0F, 0x32,
    0xB2, 0xA5, 0x9F, 0xE1, 0x4D, 0x7D, 0xD4, 0x0F, 0x94, 0x51, 0xE7, 0x09, 0x0A, 0x21, 0x12, 0x98,
    0x26, 0x0A, 0x42, 0x8C, 0xAA, 0x82, 0xE8, 0x0A, 0x29, 0xF6, 0xF4, 0x36, 0x68, 0x52, 0x55, 0x27,
    0x56, 0x12, 0x4D, 0x96, 0x23, 0x82, 0x52, 0x96, 0xF6, 0x5F, 0x3B, 0x8B, 0xF9, 0xB7, 0x5C, 0x40,
    0x5B, 0x6F, 0x0F, 0xD9, 0xAE, 0x16, 0x6C, 0x3E, 0xC4, 0x3B, 0x8F, 0x46, 0x7E, 0xC2, 0x0D, 0x98,
    0x64, 0xFE, 0xA1, 0xC1, 0x58, 0x83, 0xAB, 0x46, 0xD4, 0xFB, 0x47, 0x19, 0xDA, 0xB0, 0x9F, 0x81,
    0x1F, 0x6E, 0xE7, 0xF0, 0xCF, 0x77, 0x11, 0x1D, 0xA9, 0x11, 0x14, 0x33, 0x48, 0x68, 0x50, 0xD7,
    0x34, 0x73, 0x68, 0x48, 0xB1, 0xA7, 0x77, 0xD6, 0xA3, 0x8D, 0x99, 0x7A, 0x3C, 0x5C, 0x23, 0x6C,
    0xCB, 0x33, 0xFF, 0xF3, 0x17, 0xD2, 0x7D, 0xFB, 0x4A, 0x5A, 0x96, 0x75, 0x62, 0x32, 0x36, 0x31,
    0xFF, 0xC8, 0x4D, 0x2A, 0x29, 0xD2, 0x7E, 0x69, 0xFF, 0x71, 0x06, 0xD2, 0x2A, 0x53, 0x8C, 0x41,
    0x42, 0x83, 0xCD, 0x05, 0x68, 0xEC, 0x19, 0xD9, 0x3A, 0x40, 0xDF, 0x73, 0xBF, 0xA0, 0xFF, 0x3F,
    0xB6, 0x10, 0x1D, 0xAE, 0x62, 0x5B, 0x33, 0x88, 0xA4, 0xF1, 0x64, 0xB6, 0xBC, 0xCF, 0x56, 0x00,
    0x62, 0x05, 0x57, 0x8D, 0x41, 0xA1, 0xEB, 0xE6, 0x65, 0x9C, 0x75, 0xEF, 0x15, 0xB4, 0xAE, 0x5C,
    0x80, 0xC6, 0x1E, 0x5F, 0x8B, 0x92, 0xFA, 0x31, 0x75, 0xAE, 0x04, 0x67, 0x25, 0x81, 0x4E, 0x8C,
    0x4C, 0x91, 0x80, 0xFA, 0xB1, 0xC0, 0xA7, 0x89, 0x45, 0xC8, 0x44, 0xED, 0x69, 0xF2, 0x21, 0x12,
    0x1A, 0x46, 0x7F, 0x79, 0x80, 0xDD, 0xDF, 0x5A, 0x4B, 0xFF, 0x7F, 0x6E, 0x03, 0x51, 0x6C, 0x3E,
    0x83, 0xBA, 0x63, 0x2A, 0xDC, 0x39, 0x12, 0x80, 0x08, 0x22, 0x10, 0x0F, 0xD7, 0x29, 0x9C, 0x3B,
    0x8F, 0xA5, 0x0F, 0x7C, 0x86, 0xAE, 0x9B, 0xCE, 0x47, 0x9D, 0xE2, 0x2A, 0x51, 0x5A, 0x34, 0x27,
    0x5A, 0x12, 0x03, 0x26, 0x13, 0x60, 0x72, 0x01, 0xEA, 0x15, 0x57, 0x6E, 0xE0, 0x1B, 0x0E, 0x8D,
    0x13, 0xE6, 0x25, 0x30, 0x98, 0x8C, 0xC5, 0x16, 0x33, 0x88, 0x15, 0x7C, 0x2D, 0xC6, 0xD7, 0xE3,
    0x54, 0x78, 0x02, 0xAA, 0xA8, 0x82, 0xCD, 0x87, 0x48, 0x20, 0x0C, 0xFC, 0x68, 0x07, 0xEF, 0x3D,
    0xF2, 0xBF, 0x8C, 0xBE, 0x3D, 0x40, 0x58, 0xCA, 0x25, 0x21, 0x4C, 0x4F, 0x72, 0x1A, 0x27, 0x23,
    0x00, 0x31, 0x82, 0x77, 0x8A, 0xAF, 0x34, 0x58, 0x70, 0xEB, 0x45, 0x9C, 0xF7, 0x17, 0xD7, 0x92,
    0x5D, 0xD0, 0x42, 0x34, 0x5C, 0x1F, 0x6F, 0x57, 0xA7, 0x88, 0x15, 0x82, 0x96, 0x0C, 0x3E, 0xF2,
    0x54, 0x77, 0x0F, 0x32, 0xB8, 0x76, 0x2F, 0x23, 0x9B, 0xFB, 0xA8, 0xEC, 0x3A, 0x42, 0xE3, 0x40,
    0x19, 0x57, 0x6E, 0x00, 0x60, 0x8B, 0x19, 0x32, 0x5D, 0x45, 0x0A, 0xE7, 0x9C, 0x41, 0xCB, 0xF2,
    0x2E, 0xDA, 0xAF, 0x5C, 0x4C, 0x61, 0xE9, 0x3C, 0x4C, 0x68, 0x88, 0x47, 0x1B, 0xE3, 0x63, 0x91,
    0x9A, 0xBC, 0x2D, 0x65, 0x69, 0xF4, 0x8F, 0xF2, 0xEE, 0xD7, 0x5E, 0xA1, 0xEF, 0xB9, 0xB7, 0x30,
    0x85, 0x0C, 0xC6, 0x9E, 0x9C, 0x4B, 0xCC, 0x58, 0x00, 0x89, 0x86, 0x1C, 0x04, 0x86, 0x73, 0x1F,
    0xF8, 0x0D, 0x16, 0xDF, 0xD3, 0x8B, 0x8F, 0x3D, 0xBE, 0x1E, 0x23, 0xD6, 0x4C, 0x30, 0xD9, 0x9A,
    0xC5, 0x95, 0x1B, 0x1C, 0x7A, 0x65, 0x27, 0xFD, 0xDF, 0xDF, 0xC2, 0xE0, 0x1B, 0x7B, 0x89, 0x8E,
    0x54, 0xD1, 0xC8, 0x23, 0x19, 0x8B, 0x58, 0x49, 0x4C, 0x1D, 0x50, 0x55, 0xD4, 0x29, 0xDA, 0x70,
    0x88, 0x15, 0xC2, 0x33, 0x0A, 0xB4, 0x5F, 0xB5, 0x88, 0xF9, 0xBF, 0xB9, 0x9C, 0x8E, 0x6B, 0x96,
    0x60, 0x8B, 0x19, 0xDC, 0x68, 0x3D, 0x59, 0xEA, 0x30, 0x82, 0x3A, 0x8F, 0xC9, 0x26, 0x69, 0x72,
    0xCF, 0x93, 0xEB, 0x78, 0xF7, 0xD1, 0x9F, 0x43, 0xEC, 0x31, 0x39, 0x3B, 0xE3, 0x00, 0x39, 0x33,
    0x01, 0x58, 0xC1, 0x57, 0x22, 0xC2, 0x33, 0x0A, 0x5C, 0xF8, 0xF0, 0x8D, 0x9C, 0x79, 0xFD, 0xB9,
    0xC4, 0x83, 0xB5, 0x74, 0x7D, 0x28, 0x61, 0xCC, 0xE6, 0x43, 0xC4, 0x08, 0x03, 0xFF, 0xF5, 0x0E,
    0x7B, 0x9E, 0x58, 0xC7, 0xE0, 0xFA, 0x7D, 0xE0, 0x3C, 0x26, 0x1F, 0x62, 0x02, 0x93, 0x38, 0x75,
    0x6A, 0xAE, 0x93, 0x70, 0x50, 0xFA, 0x26, 0x69, 0xF3, 0xB1, 0xC7, 0x57, 0x23, 0xB0, 0x86, 0xF6,
    0xDE, 0x1E, 0x16, 0xDF, 0x7B, 0x39, 0x67, 0x5E, 0x7F, 0x1E, 0xA8, 0xE2, 0xAA, 0x51, 0x22, 0xE8,
    0xD4, 0x2D, 0xC2, 0xF6, 0x1C, 0x07, 0x7F, 0xF2, 0x2E, 0x5B, 0x57, 0xAF, 0x21, 0x3A, 0x54, 0xC1,
    0x14, 0x43, 0x98, 0x81, 0x10, 0x4E, 0x58, 0x00, 0x62, 0x04, 0x57, 0x8D, 0xC8, 0x2F, 0x6E, 0x67,
    0xF9, 0x37, 0x3E, 0x47, 0xDB, 0xAA, 0x1E, 0xA2, 0xC1, 0x6A, 0x02, 0x66, 0x34, 0xD1, 0x62, 0x50,
    0xCA, 0x52, 0x79, 0xEF, 0x30, 0x3B, 0xFF, 0xEE, 0xFF, 0x38, 0xF0, 0xE2, 0x36, 0x54, 0x35, 0xF1,
    0xEB, 0x74, 0xCE, 0x89, 0xC6, 0x53, 0x6D, 0x4F, 0xCE, 0xE9, 0x29, 0x76, 0x10, 0x9B, 0xA2, 0x45,
    0xD5, 0xF1, 0x3E, 0xAE, 0xD2, 0x40, 0x10, 0x3A, 0x6F, 0x38, 0x9F, 0x25, 0xF7, 0xFF, 0x3A, 0x2D,
    0xE7, 0x77, 0x12, 0x0D, 0xD7, 0x26, 0x02, 0x65, 0xEC, 0x09, 0xDB, 0xF3, 0x0C, 0xBD, 0xB9, 0x9F,
    0x2D, 0xF7, 0xBF, 0x48, 0x75, 0xD7, 0x11, 0x6C, 0x3E, 0x3C, 0x61, 0x77, 0x38, 0x31, 0x01, 0x88,
    0xA0, 0xB1, 0x23, 0x73, 0x66, 0x91, 0x95, 0x4F, 0xDE, 0x4A, 0xCB, 0xB2, 0x4E, 0xE2, 0xA1, 0xDA,
    0x38, 0x38, 0x31, 0xD6, 0x60, 0x0A, 0x21, 0x1F, 0x3C, 0xFF, 0x16, 0xEF, 0x7C, 0xED, 0x67, 0xD4,
    0x3F, 0x18, 0x21, 0x28, 0x65, 0xA7, 0x40, 0x7E, 0xDF, 0x70, 0xB8, 0x5A, 0x84, 0x20, 0xD8, 0x62,
    0x06, 0x93, 0x9D, 0x8A, 0x04, 0x7D, 0xDD, 0xE1, 0xCA, 0x0D, 0x14, 0xC5, 0xE6, 0x42, 0x4C, 0x66,
    0x62, 0xE5, 0x6A, 0x2C, 0xC5, 0x66, 0xE7, 0xB7, 0x72, 0xDE, 0x5F, 0x5E, 0xC3, 0x82, 0x5B, 0x57,
    0xE0, 0x2B, 0x11, 0xDE, 0xF9, 0x14, 0x74, 0x79, 0x82, 0xB6, 0x1C, 0xE5, 0xED, 0x07, 0x79, 0xF3,
    0x8B, 0xCF, 0xD1, 0x38, 0x58, 0x46, 0x02, 0x7B, 0x42, 0x81, 0xF1, 0x84, 0xB0, 0xBF, 0x58, 0x21,
    0x1A, 0x6C, 0xB0, 0xE8, 0xAE, 0xCB, 0x28, 0xAD, 0x98, 0x4F, 0x7D, 0xA0, 0x8C, 0x09, 0x13, 0x7F,
    0x33, 0x59, 0x0B, 0x4E, 0xD9, 0xF1, 0xD0, 0x4F, 0xD9, 0xF3, 0xED, 0xF5, 0x98, 0xD0, 0x10, 0xB6,
    0xE5, 0xC6, 0x41, 0x4E, 0x5C, 0x89, 0x20, 0xF6, 0xE4, 0xCE, 0x6A, 0xA3, 0xFD, 0x8A, 0x45, 0x94,
    0x2E, 0x5D, 0x48, 0x61, 0x49, 0x07, 0xD9, 0x85, 0x2D, 0x53, 0x90, 0x60, 0xBD, 0x6F, 0x94, 0xCA,
    0xCE, 0xC3, 0x0C, 0x6F, 0xDA, 0xCF, 0xE0, 0x1B, 0xFB, 0xA8, 0xED, 0x1D, 0x04, 0x63, 0x08, 0x0A,
    0x09, 0x46, 0x0B, 0xDB, 0xF2, 0xC4, 0x43, 0x35, 0xB6, 0x3C, 0xB0, 0x86, 0x91, 0xB7, 0xFA, 0x59,
    0xBA, 0xFA, 0xD3, 0x98, 0x8C, 0xC5, 0xD7, 0x1D, 0x12, 0x18, 0xA2, 0xC1, 0x2A, 0xAD, 0x17, 0x75,
    0xD1, 0x75, 0xE3, 0x32, 0x76, 0x3D, 0xF6, 0x1A, 0x99, 0x8E, 0x02, 0x1A, 0x37, 0x49, 0x00, 0x13,
    0x94, 0x02, 0x19, 0x05, 0x8D, 0x3C, 0xB6, 0x35, 0x43, 0x3C, 0x58, 0x63, 0xEB, 0x9F, 0xAF, 0x61,
    0xE0, 0x47, 0x3B, 0x08, 0xDA, 0x73, 0xE3, 0xCF, 0x69, 0x3D, 0xC6, 0x45, 0x9E, 0xB6, 0xDE, 0x6E,
    0x16, 0xFC, 0xF6, 0x0A, 0x3A, 0xAE, 0x5D, 0x4A, 0x6E, 0x61, 0x6B, 0x82, 0xE4, 0xA2, 0x04, 0xFB,
    0x8F, 0x23, 0xC1, 0x33, 0x8B, 0x14, 0xCF, 0xED, 0xA0, 0xE3, 0xBA, 0xA5, 0x68, 0x23, 0xA6, 0x7E,
    0xA0, 0xCC, 0xA1, 0x9F, 0xED, 0xA4, 0xEF, 0xDF, 0x36, 0x33, 0xB4, 0x7E, 0x3F, 0x26, 0x34, 0x48,
    0x2E, 0x40, 0x32, 0x16, 0x9B, 0xB1, 0xEC, 0xFE, 0xD6, 0x5A, 0xAA, 0xEF, 0x1F, 0xE1, 0xC2, 0xAF,
    0xDF, 0x44, 0x70, 0x46, 0x0E, 0x37, 0xD2, 0x48, 0x58, 0x4B, 0x0B, 0xA9, 0x99, 0xD0, 0x8C, 0x5D,
    0xE0, 0xE2, 0x7F, 0xFE, 0x2D, 0xDA, 0x56, 0x75, 0xA3, 0x5E, 0x29, 0x6F, 0x1B, 0x60, 0xEB, 0xEA,
    0x35, 0x0C, 0xAE, 0xDB, 0x4B, 0xD8, 0x9E, 0x4F, 0x7C, 0x17, 0x88, 0x87, 0xEA, 0x14, 0xCE, 0xEF,
    0xE0, 0x9C, 0x3F, 0xBA, 0x9A, 0xCE, 0x1B, 0xCF, 0x27, 0x68, 0xCD, 0xE2, 0xAA, 0x11, 0xBE, 0xE1,
    0xC6, 0x82, 0x41, 0x32, 0xEC, 0x24, 0xB1, 0x26, 0x6F, 0x92, 0x36, 0x13, 0x5A, 0x6C, 0x21, 0x24,
    0x1E, 0xA9, 0x33, 0xB0, 0xE6, 0x6D, 0x76, 0x3D, 0xF6, 0x1A, 0x95, 0xB7, 0x0F, 0x11, 0x94, 0xB2,
    0x68, 0x5A, 0x39, 0x46, 0x43, 0x55, 0xDA, 0x7B, 0x17, 0x71, 0xE1, 0xC3, 0x37, 0x52, 0xBC, 0xA0,
    0x13, 0x31, 0xC2, 0xD0, 0xC6, 0xFD, 0xFC, 0xE2, 0x0F, 0xFF, 0x7D, 0x46, 0x2E, 0x70, 0xE2, 0x59,
    0x40, 0x04, 0x5F, 0x8F, 0xC9, 0x2E, 0x68, 0xA1, 0xEB, 0x86, 0x65, 0x20, 0x70, 0xE0, 0xA5, 0xED,
    0xD4, 0xF6, 0x0D, 0x13, 0xB4, 0x66, 0x93, 0xE8, 0x1D, 0x39, 0xB4, 0xE1, 0xE9, 0xBE, 0xF3, 0x12,
    0x96, 0xFC, 0xF1, 0xA7, 0xC8, 0x2E, 0x6C, 0x25, 0x1E, 0xA9, 0xA3, 0xA9, 0xAF, 0x02, 0x29, 0x30,
    0x12, 0xB0, 0x66, 0x4C, 0x0E, 0x09, 0x9F, 0xCE, 0x4F, 0xB4, 0x8D, 0x3D, 0x67, 0x0D, 0x41, 0x6B,
    0x96, 0x7A, 0xDF, 0x08, 0x3B, 0xFF, 0xE1, 0x55, 0xF6, 0xFF, 0xEB, 0x9B, 0x49, 0xBD, 0x10, 0x5A,
    0x10, 0x21, 0x1E, 0xA9, 0x93, 0xEB, 0x29, 0xD1, 0x75, 0xD3, 0x32, 0x50, 0x38, 0xF0, 0xC3, 0xED,
    0xD4, 0x3F, 0x18, 0xC5, 0x64, 0x83, 0x13, 0x06, 0x46, 0x33, 0x4B, 0x83, 0xA9, 0x25, 0xC4, 0xA3,
    0x09, 0x80, 0x09, 0x5A, 0x32, 0x49, 0x2C, 0x00, 0x7C, 0x35, 0x22, 0x28, 0xE5, 0x38, 0xEF, 0xAB,
    0xD7, 0xB2, 0xF0, 0x0B, 0x17, 0xE3, 0xEB, 0x51, 0xE2, 0x9F, 0x93, 0x52, 0x96, 0xC9, 0xDA, 0xA4,
    0x20, 0xAA, 0xC5, 0x09, 0x1A, 0x8C, 0x12, 0x24, 0x68, 0x42, 0x93, 0x04, 0xC6, 0x5C, 0x90, 0x58,
    0x4A, 0xDD, 0x4D, 0x20, 0x49, 0xE7, 0x31, 0x19, 0x8B, 0xC9, 0x85, 0x7C, 0xF0, 0xC2, 0x5B, 0xEC,
    0xF8, 0x9B, 0x97, 0x89, 0x87, 0x6B, 0x09, 0x3C, 0x06, 0x7C, 0x34, 0x95, 0x9F, 0x13, 0xD5, 0xFC,
    0xC9, 0x09, 0x20, 0x15, 0xC2, 0x84, 0x96, 0x3C, 0x82, 0xE0, 0x6A, 0x11, 0xF9, 0xB3, 0xDA, 0xB9,
    0xF0, 0x91, 0x9B, 0x68, 0xBF, 0x72, 0x11, 0xD1, 0x91, 0x5A, 0xBA, 0xB4, 0x9A, 0x62, 0x83, 0x5C,
    0x88, 0x64, 0x2D, 0x95, 0xF7, 0x0E, 0x73, 0xE8, 0xE5, 0xF7, 0x18, 0x5E, 0xBF, 0x9F, 0xF2, 0xCE,
    0xC3, 0xC4, 0x87, 0xAB, 0x09, 0xE3, 0xF3, 0xF2, 0x14, 0x97, 0xCC, 0xA3, 0xD4, 0xDB, 0x4D, 0xC7,
    0x75, 0x4B, 0x29, 0x2C, 0x9D, 0x87, 0xD6, 0xD3, 0xAC, 0x31, 0x26, 0x40, 0x0F, 0xE1, 0x19, 0x39,
    0x06, 0xD7, 0xEE, 0x65, 0xEB, 0x03, 0x2F, 0x51, 0xDD, 0x3D, 0x88, 0xCD, 0x85, 0x68, 0x5A, 0x3C,
    0x25, 0xFC, 0xE8, 0x8C, 0x21, 0xF1, 0xEC, 0x8A, 0xA1, 0xA3, 0xD3, 0xE3, 0x05, 0x9D, 0xC4, 0x83,
    0xB5, 0x14, 0x1B, 0xA4, 0x40, 0xA5, 0x94, 0xA3, 0xF2, 0xDE, 0x21, 0xF6, 0x7C, 0x67, 0x03, 0x07,
    0x5E, 0xDC, 0x4E, 0xBD, 0x7F, 0x04, 0x10, 0x4C, 0xC6, 0x4E, 0x31, 0x77, 0xDF, 0x70, 0x80, 0x92,
    0x5D, 0xD0, 0x4A, 0xD7, 0x4D, 0xCB, 0x58, 0x7C, 0xF7, 0x65, 0xE4, 0x97, 0x76, 0x10, 0x8F, 0xD4,
    0x92, 0x58, 0x21, 0x69, 0xBA, 0x6B, 0xCF, 0x31, 0xBA, 0x6D, 0x80, 0xCD, 0x5F, 0x7C, 0x7E, 0x46,
    0xBE, 0x7E, 0x3C, 0x9A, 0xD5, 0x22, 0x88, 0x58, 0x21, 0x1E, 0x6D, 0xD0, 0x75, 0xE3, 0x32, 0x4A,
    0x2B, 0xE6, 0x13, 0x1D, 0xA9, 0x8E, 0x63, 0x03, 0x09, 0x0C, 0x36, 0x1F, 0xB2, 0xF7, 0xA9, 0x0D,
    0x6C, 0xB8, 0xE3, 0x19, 0xF6, 0x3C, 0xB1, 0x8E, 0x78, 0xB8, 0x4E, 0xD8, 0x96, 0x23, 0x6C, 0xCB,
    0x62, 0xB2, 0x01, 0x12, 0x26, 0x65, 0xAF, 0xC9, 0x06, 0x84, 0x6D, 0x59, 0xC2, 0xB6, 0x1C, 0xF1,
    0x50, 0x9D, 0x3D, 0x4F, 0xAC, 0x63, 0xC3, 0x1D, 0xCF, 0xB0, 0xEF, 0xA9, 0x0D, 0x89, 0xF5, 0x4C,
    0x1A, 0x33, 0x3A, 0x52, 0xA5, 0xB4, 0x62, 0x3E, 0x5D, 0x37, 0x2E, 0x23, 0x1E, 0x6D, 0xA4, 0xE0,
    0xE9, 0xE4, 0xA9, 0x49, 0xCB, 0x60, 0x69, 0x64, 0xD7, 0x04, 0xD4, 0xD8, 0x42, 0x88, 0xAB, 0xC6,
    0x6C, 0x5D, 0xFD, 0x12, 0xDB, 0xBF, 0xFA, 0x63, 0xA2, 0xC3, 0x55, 0xC2, 0xF6, 0x3C, 0x12, 0x24,
    0x45, 0x92, 0xBA, 0xA3, 0x4C, 0x75, 0xAC, 0x16, 0x70, 0x8A, 0x04, 0x42, 0xD8, 0x9E, 0x27, 0x3A,
    0x5C, 0x65, 0xFB, 0x57, 0x7E, 0xCC, 0xD6, 0x2F, 0xBF, 0x88, 0xAB, 0xC4, 0xD8, 0x42, 0x98, 0x54,
    0x8F, 0x4A, 0x9A, 0x45, 0x9A, 0xB3, 0x28, 0x32, 0x2B, 0x01, 0xA8, 0x53, 0x82, 0x96, 0x0C, 0x07,
    0xD6, 0xBC, 0xCD, 0xD0, 0xC6, 0xFD, 0x64, 0xE7, 0xB7, 0x10, 0x76, 0x14, 0xA8, 0xEC, 0x3A, 0xC2,
    0xE6, 0xDF, 0x7F, 0x81, 0xBE, 0x67, 0xDF, 0x4A, 0x10, 0x61, 0x68, 0x93, 0xBA, 0x3D, 0xE5, 0x39,
    0x59, 0x0B, 0x48, 0x96, 0xCD, 0x34, 0xD6, 0x29, 0x71, 0x05, 0x25, 0xC9, 0x1A, 0xA1, 0x25, 0x68,
    0xCB, 0xD2, 0xF7, 0xFC, 0x2F, 0xD9, 0xFC, 0x07, 0x2F, 0x50, 0xD9, 0x75, 0x84, 0xB0, 0xA3, 0x40,
    0x76, 0x7E, 0x0B, 0x43, 0x1B, 0xF7, 0x73, 0x60, 0xCD, 0xDB, 0x04, 0x2D, 0x99, 0x59, 0xAF, 0x0E,
    0xCD, 0x7E, 0x45, 0xE8, 0x43, 0xD2, 0xE3, 0xD1, 0x0B, 0x16, 0x63, 0x35, 0x05, 0x40, 0xD8, 0x9E,
    0x07, 0x20, 0x1A, 0x4C, 0x82, 0xE1, 0x74, 0x18, 0x5E, 0xAC, 0x69, 0x4A, 0xBA, 0x3B, 0x2E, 0xFB,
    0xCD, 0x58, 0x12, 0x3B, 0x6E, 0x7A, 0x3C, 0x7A, 0x32, 0x46, 0x88, 0x2B, 0x11, 0xAD, 0x17, 0x76,
    0x72, 0xF6, 0x7D, 0x57, 0x51, 0xBA, 0x64, 0x21, 0x00, 0xC3, 0x9B, 0xFA, 0x78, 0xFF, 0xF1, 0xD7,
    0x19, 0xD9, 0x3A, 0x40, 0x50, 0x9C, 0x46, 0x08, 0x46, 0x66, 0x9D, 0xEE, 0x8E, 0xCB, 0x7A, 0x53,
    0x04, 0x00, 0x47, 0xA5, 0xC7, 0x63, 0xD3, 0xD1, 0x98, 0xE6, 0x5B, 0x2E, 0xE8, 0x64, 0xE5, 0x13,
    0x5F, 0x20, 0xD7, 0xD3, 0x8A, 0x2B, 0x27, 0x96, 0x60, 0x8B, 0x19, 0x6A, 0x7B, 0x87, 0xD8, 0x7C,
    0xEF, 0xF3, 0x8C, 0x6E, 0x1F, 0x98, 0xBE, 0x9A, 0xFB, 0x90, 0xF1, 0x4F, 0x96, 0x9A, 0xB7, 0x17,
    0xA0, 0x8A, 0x3A, 0x9F, 0xFA, 0xFA, 0xF1, 0x99, 0x3B, 0xEB, 0xBE, 0xAB, 0xC9, 0xF5, 0x94, 0x68,
    0x1C, 0xAC, 0x8C, 0xEF, 0x07, 0x34, 0x0E, 0x96, 0xC9, 0x2D, 0x6A, 0xE3, 0xAC, 0xFB, 0xAE, 0x9A,
    0xF5, 0xF8, 0x33, 0xA5, 0xB9, 0xD9, 0x0B, 0x14, 0xF0, 0x51, 0x52, 0xB7, 0x97, 0x2E, 0x59, 0x80,
    0x2B, 0x37, 0xC6, 0x37, 0x3D, 0x10, 0x30, 0xA1, 0xC5, 0x95, 0x1B, 0x94, 0x2E, 0x59, 0x48, 0xD8,
    0x96, 0x4B, 0x10, 0xE2, 0x1C, 0xED, 0x4F, 0xCF, 0xED, 0x66, 0x68, 0x13, 0x35, 0xD7, 0x2C, 0x9A,
    0x1B, 0x01, 0x68, 0x82, 0xF7, 0xA3, 0xA1, 0x1A, 0xC3, 0x6F, 0xF6, 0x25, 0xEB, 0x00, 0xCE, 0xA3,
    0xE9, 0x0F, 0xCE, 0x63, 0x8B, 0x19, 0x86, 0xDF, 0xEC, 0x23, 0x1A, 0xAA, 0x61, 0x42, 0x33, 0x67,
    0x87, 0xD0, 0xE6, 0x7C, 0x3B, 0x7C, 0xF7, 0x63, 0xAF, 0x53, 0xDF, 0x37, 0x84, 0x6B, 0x09, 0x08,
    0x31, 0x84, 0x18, 0x5C, 0x4B, 0x40, 0x6D, 0xDF, 0x10, 0xEF, 0x3F, 0xFE, 0xFA, 0x5C, 0xB3, 0x33,
    0x77, 0x02, 0x50, 0xAF, 0xD8, 0x7C, 0x48, 0x79, 0xDB, 0x00, 0x6F, 0xDC, 0xFD, 0x3D, 0x96, 0xEF,
    0x08, 0x78, 0xB4, 0xF3, 0x66, 0x1E, 0xED, 0xBC, 0x99, 0xE5, 0x3B, 0x02, 0xDE, 0xB8, 0xFB, 0x7B,
    0x54, 0xB6, 0x1D, 0x27, 0x03, 0x9C, 0x42, 0x9A, 0xFB, 0x18, 0x90, 0xB3, 0xE8, 0xEE, 0x51, 0xEE,
    0x6A, 0x5B, 0x45, 0x77, 0x50, 0xA2, 0x3B, 0x28, 0x71, 0x57, 0xDB, 0x2A, 0xD8, 0x3D, 0x0A, 0xB9,
    0xE6, 0xE4, 0xF6, 0x99, 0xD0, 0x1C, 0xBB, 0x80, 0xE0, 0x9D, 0xA7, 0xD0, 0x5E, 0xA4, 0xE3, 0xEC,
    0x4E, 0xAA, 0x34, 0xA8, 0xD2, 0xA0, 0xE3, 0xEC, 0x4E, 0x0A, 0xED, 0x45, 0xBC, 0xF3, 0xCC, 0xF5,
    0xF1, 0xA4, 0xD3, 0x72, 0x24, 0x46, 0x9D, 0xC7, 0x45, 0x0E, 0x83, 0x60, 0x10, 0x5C, 0xE4, 0x66,
    0xBD, 0xC7, 0x77, 0xB2, 0x74, 0xDA, 0xCE, 0x04, 0x8D, 0xED, 0x0A, 0x1D, 0xFD, 0x7E, 0xAE, 0xE9,
    0x93, 0x7E, 0x28, 0xEA, 0x43, 0xE9, 0x94, 0x9D, 0x06, 0x9D, 0x4E, 0xAB, 0x22, 0x32, 0xFE, 0x9A,
    0x49, 0x1B, 0x30, 0xBE, 0x84, 0xDE, 0x6C, 0x3A, 0x25, 0x02, 0x30, 0xC6, 0x10, 0xC7, 0x0E, 0xEF,
    0x3D, 0x93, 0xE7, 0x23, 0x22, 0xC4, 0x51, 0x4C, 0x1C, 0xC7, 0xC7, 0xF4, 0x89, 0xE3, 0x98, 0x38,
    0x4A, 0x3E, 0x9F, 0x3C, 0x59, 0xD5, 0x64, 0xBC, 0x20, 0xB0, 0x78, 0xDF, 0xFC, 0x38, 0x11, 0x00,
    0x0D, 0x20, 0xD3, 0xB4, 0x11, 0x05, 0x2A, 0x95, 0x2A, 0xAD, 0xA5, 0x22, 0xB9, 0x4C, 0x36, 0x41,
    0x7A, 0x63, 0x4D, 0x22, 0xC4, 0x71, 0x4C, 0xA9, 0xAD, 0xF5, 0x98, 0x18, 0x50, 0x6A, 0x6B, 0x4D,
    0x18, 0x0A, 0x82, 0x29, 0x02, 0x10, 0x84, 0x5A, 0xA3, 0xCE, 0xC8, 0x70, 0x99, 0x5C, 0x3E, 0xDB,
    0x6C, 0x84, 0x18, 0x49, 0xB1, 0xFB, 0xB2, 0x3D, 0x88, 0x59, 0x94, 0x26, 0xE0, 0x59, 0x45, 0x23,
    0x63, 0x84, 0x6A, 0xA5, 0xC6, 0xE7, 0x6E, 0xB9, 0x8E, 0x87, 0xFE, 0xEA, 0x4F, 0x69, 0xED, 0x28,
    0xA2, 0x32, 0xB1, 0x11, 0x32, 0x46, 0x02, 0x14, 0x8A, 0xF9, 0x29, 0xDB, 0xE3, 0x95, 0x72, 0xF5,
    0xD8, 0xB9, 0xA9, 0x22, 0x2A, 0x8C, 0x1C, 0x2A, 0xF3, 0xE0, 0x5F, 0x7F, 0x83, 0x17, 0x7F, 0xF0,
    0x32, 0xF9, 0x42, 0x0E, 0xDF, 0x24, 0xA0, 0x24, 0xE8, 0x01, 0x29, 0x76, 0xF7, 0xAE, 0x11, 0x63,
    0x6E, 0x50, 0x75, 0x8E, 0xE4, 0x90, 0xF4, 0xC9, 0x0D, 0x26, 0x82, 0x73, 0x8E, 0x5C, 0x36, 0xCB,
    0xCB, 0xAF, 0x3E, 0xCD, 0x79, 0x0B, 0xCF, 0xA6, 0x4C, 0x05, 0x33, 0x4D, 0x9C, 0x55, 0xC0, 0xE9,
    0x94, 0x7B, 0x0B, 0x58, 0xB1, 0xD3, 0x4A, 0xDF, 0xE3, 0x29, 0x52, 0xE0, 0x9D, 0xBE, 0xF7, 0xB9,
    0xEE, 0x53, 0x77, 0x52, 0xAB, 0xD7, 0xB1, 0xD6, 0xCE, 0x36, 0x26, 0x78, 0x44, 0x0C, 0xDE, 0xAF,
    0x35, 0x2A, 0xBC, 0xC2, 0xD4, 0x9D, 0xAA, 0x53, 0x4E, 0x02, 0x04, 0x12, 0x4C, 0x79, 0xCD, 0x6D,
    0x22, 0x54, 0x15, 0x0C, 0x8A, 0x6C, 0x92, 0xEC, 0xA2, 0x55, 0xE7, 0x59, 0x95, 0x8D, 0x82, 0xB4,
    0x30, 0xBE, 0xFB, 0x79, 0x72, 0x74, 0xA2, 0x2E, 0x30, 0x03, 0x3E, 0x4F, 0x95, 0x0B, 0x28, 0x22,
    0x22, 0xAA, 0xB7, 0x09, 0x40, 0xA1, 0xBB, 0xF7, 0x69, 0x63, 0xED, 0xEF, 0xA8, 0x8F, 0x67, 0xE5,
    0x06, 0x00, 0x08, 0xD4, 0xAA, 0xF5, 0x69, 0x83, 0xE0, 0xC9, 0x0D, 0xD7, 0xF4, 0x20, 0xE8, 0x93,
    0x33, 0x76, 0xFA, 0x7E, 0xAE, 0x96, 0x5D, 0x19, 0x24, 0xDF, 0xE1, 0xBE, 0xAE, 0x5E, 0x6E, 0x4B,
    0x27, 0x3F, 0xBB, 0x60, 0xA8, 0x50, 0x28, 0xE4, 0xA9, 0xD7, 0x22, 0xAA, 0x95, 0xFA, 0x49, 0x2B,
    0x7F, 0x7C, 0xB8, 0x34, 0x0D, 0x16, 0x0A, 0xF9, 0x26, 0xA5, 0x41, 0xF5, 0x22, 0x41, 0x80, 0x8F,
    0x9F, 0x3C, 0x74, 0xE8, 0xD5, 0x91, 0xF4, 0xA2, 0x11, 0xBE, 0xB0, 0xB0, 0xF7, 0x11, 0x63, 0x83,
    0x2F, 0xAB, 0x46, 0x31, 0x4D, 0x38, 0x34, 0xDD, 0x6C, 0x78, 0xDB, 0x24, 0x20, 0x94, 0x68, 0xDF,
    0xEB, 0x07, 0xA2, 0xAC, 0x1C, 0xED, 0x5B, 0x7F, 0x68, 0xEC, 0x52, 0xA1, 0xA9, 0xF8, 0xE8, 0x41,
    0xD5, 0x78, 0xA3, 0x48, 0x10, 0xC0, 0x51, 0x21, 0xFA, 0x24, 0x48, 0x55, 0x9B, 0xFA, 0x6A, 0x02,
    0x69, 0xA2, 0x7D, 0x11, 0xF0, 0x7F, 0x96, 0x5E, 0xA3, 0x33, 0x63, 0x02, 0x80, 0xFE, 0xCD, 0xE5,
    0x08, 0x77, 0xBB, 0x57, 0xD7, 0x8F, 0x58, 0xDB, 0x0C, 0x21, 0x7C, 0xB4, 0x48, 0x9D, 0x98, 0x20,
    0xF0, 0xCE, 0x7F, 0xB3, 0xBC, 0x7F, 0xE3, 0x33, 0xC9, 0x45, 0x31, 0xDC, 0x24, 0x3B, 0x4D, 0xAE,
    0x92, 0xE5, 0x17, 0x5D, 0x7E, 0x85, 0x28, 0x3F, 0x10, 0x31, 0xF3, 0xD1, 0xB8, 0x29, 0xEE, 0x70,
    0x9A, 0xC9, 0x03, 0x2A, 0x26, 0xB0, 0xDE, 0xC5, 0x4F, 0x57, 0xF6, 0x2F, 0xBD, 0x3B, 0xF9, 0xF8,
    0xD9, 0xE4, 0xF3, 0xA9, 0xCF, 0x26, 0x42, 0x28, 0x2E, 0xEA, 0xBD, 0x58, 0x95, 0xEF, 0x1A, 0x13,
    0x5C, 0xA4, 0x3E, 0x4E, 0x17, 0xE2, 0x3F, 0x76, 0x97, 0xA9, 0x94, 0xE4, 0xBA, 0x5C, 0x90, 0x5C,
    0x94, 0xF1, 0xDF, 0xAC, 0xEC, 0x5D, 0x7F, 0x3F, 0x13, 0x79, 0x44, 0x61, 0xFA, 0x09, 0x59, 0xC0,
    0xB5, 0x2C, 0xEC, 0x3D, 0x53, 0x0D, 0x0F, 0x23, 0x72, 0x8F, 0x88, 0x49, 0xEF, 0x53, 0xA9, 0xE3,
    0xE3, 0x71, 0x75, 0xD6, 0x20, 0x46, 0x52, 0xBE, 0x77, 0x21, 0xFA, 0x95, 0xF2, 0xDE, 0xF5, 0xDF,
    0x65, 0x9A, 0x63, 0x49, 0xC7, 0x99, 0xC4, 0xC4, 0x45, 0xE3, 0x62, 0xF7, 0xAA, 0xCF, 0x82, 0xBD,
    0x1F, 0xD1, 0xEB, 0x45, 0x6C, 0x56, 0xC7, 0xBE, 0xE3, 0x23, 0x79, 0x79, 0x1A, 0xC6, 0x2E, 0x78,
    0xAB, 0xF2, 0x2E, 0xAA, 0xDF, 0x31, 0xDE, 0x3D, 0x3E, 0xFA, 0xC1, 0xA6, 0x01, 0x8E, 0x73, 0x8B,
    0xFC, 0x57, 0x69, 0x71, 0xFC, 0x16, 0x39, 0x40, 0x69, 0xF1, 0x15, 0x97, 0x47, 0xDE, 0x7D, 0x5E,
    0x90, 0xAB, 0x05, 0x96, 0xA3, 0xCC, 0x43, 0xC8, 0x9E, 0xAA, 0xE9, 0xCC, 0x90, 0x22, 0x85, 0x41,
    0x51, 0x76, 0x29, 0xB2, 0x41, 0xF0, 0x3F, 0x2D, 0xD7, 0xCB, 0x6B, 0x38, 0xB4, 0x7D, 0x24, 0x69,
    0x9E, 0xF4, 0xAF, 0x00, 0x8E, 0xA2, 0xFF, 0x07, 0x2D, 0x6C, 0x7F, 0xC5, 0x51, 0x6F, 0x89, 0x4A,
    0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82
};
static constexpr size_t RFLINK_FAVICON_ICO_SIZE = sizeof(RFLINK_FAVICON_ICO);

}  // namespace rflink_universal
}  // namespace esphome
