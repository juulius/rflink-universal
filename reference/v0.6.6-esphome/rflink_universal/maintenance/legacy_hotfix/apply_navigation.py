from pathlib import Path
import json
root=Path('/mnt/data/rflink_nav3/rflink_universal')
f=root/'ui/index.html'
s=f.read_text(encoding='utf-8')
old=""" if(opening){panel.hidden=false;const btn=document.querySelector('[data-settings-open="'+which+'"]');if(btn)btn.setAttribute('aria-expanded','true');if(scrollToContent)panel.scrollIntoView({behavior:'smooth',block:'start'});}
}
document.querySelectorAll('[data-settings-open]').forEach(b=>b.addEventListener('click',()=>showSettingsPanel(b.dataset.settingsOpen,true)));"""
new=""" if(opening){
  panel.hidden=false;
  const btn=document.querySelector('[data-settings-open="'+which+'"]');
  if(btn)btn.setAttribute('aria-expanded','true');
  // Opening a named destination must show its content immediately. The old UI
  // retained an inner <details> layer for Custom and Diagnostics, causing an
  // unnecessary third click on its heading.
  const rootDetails=panel.querySelector(':scope > .card > details');
  if(rootDetails)rootDetails.open=true;
  if(scrollToContent)panel.scrollIntoView({behavior:'smooth',block:'start'});
 }
}
document.querySelectorAll('[data-settings-open]').forEach(b=>b.addEventListener('click',()=>showSettingsPanel(b.dataset.settingsOpen,true)));
document.querySelectorAll('[data-settings-back]').forEach(b=>b.addEventListener('click',()=>{
 showSettingsPanel(b.closest('.settings-detail-panel').id.replace('settingsPanel','').toLowerCase(),false);
 document.querySelector('.page[data-view="settings"] .settings-pickers').scrollIntoView({behavior:'smooth',block:'nearest'});
}));"""
assert s.count(old)==1,s.count(old)
s=s.replace(old,new)
# Put a small in-panel back button in every tool area.
for name in ('Library','Decoders','Custom','Diagnostics'):
 needle=f'<div class="settings-detail-panel" id="settingsPanel{name}" hidden>'
 assert s.count(needle)==1, (name,s.count(needle))
 s=s.replace(needle,needle+'\n<div class="settings-detail-toolbar"><button type="button" class="btn out small settings-back" data-settings-back data-i18n="nav.back_settings">← Back to settings overview</button></div>',1)
# A direct Create decoder route from decoder library and custom protocol list.
needle='<div class="card decoder-library-card settings-wide" id="decoderLibraryCard"><h2'
assert needle in s
s=s.replace(needle,'<div class="card decoder-library-card settings-wide" id="decoderLibraryCard"><div class="settings-tool-action"><button type="button" class="btn out small" data-launch-decoder data-i18n="static.053">Create decoder</button></div><h2',1)
needle='<details id="userLibraryDetails"><summary'
assert needle in s
s=s.replace(needle,'<details id="userLibraryDetails"><summary',1) # no-op clarity
needle='<p class="muted small" data-i18n="user.saved_help">Only use this when RFLink does not already know your device or when you want a custom model profile.</p>'
assert needle in s
s=s.replace(needle,needle+'<div class="actions settings-custom-actions"><button type="button" class="btn out small" data-launch-decoder data-i18n="static.053">Create decoder</button></div>',1)
needle='function setLearnMode(mode){'
assert needle in s
s=s.replace(needle,"""// Deep link: Settings -> Decoder library/Custom -> Create decoder (max 3 clicks).
document.querySelectorAll('[data-launch-decoder]').forEach(b=>b.addEventListener('click',()=>{
 show('learn');setLearnMode('decoder');
 const target=document.querySelector('#expertLearn');
 if(target)target.scrollIntoView({behavior:'smooth',block:'start'});
}));
"""+needle,1)
css='''
/* NAV3: in-panel return action and direct decoder creation without extra disclosure. */
.page[data-view="settings"] .settings-detail-toolbar{display:flex;align-items:center;margin:0 0 8px}
.page[data-view="settings"] .settings-detail-toolbar .btn{min-height:35px;font-size:12px}
.page[data-view="settings"] .settings-tool-action{float:right;margin:1px 0 8px 8px}
.page[data-view="settings"] .settings-tool-action .btn{font-size:12px;min-height:34px}
.page[data-view="settings"] .settings-custom-actions{margin:6px 0 10px}
@media(max-width:580px){.page[data-view="settings"] .settings-tool-action{float:none;margin:0 0 10px}}
'''
assert s.count('</style>')==1
s=s.replace('</style>',css+'</style>')
f.write_text(s,encoding='utf-8')
labels={'en':'← Back to settings overview','nl':'← Terug naar instellingen','fr':'← Retour aux paramètres','de':'← Zurück zu den Einstellungen'}
for lang,label in labels.items():
 p=root/'ui/locales'/f'{lang}.json'; j=json.loads(p.read_text(encoding='utf-8'));assert 'nav.back_settings' not in j;j['nav.back_settings']=label;p.write_text(json.dumps(j,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
print('Navigation patch applied')
