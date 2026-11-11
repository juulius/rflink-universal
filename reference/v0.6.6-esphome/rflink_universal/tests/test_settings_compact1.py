"""SETTINGS_COMPACT1 DOM and localization contract; no web server/hardware needed."""
from html.parser import HTMLParser
from pathlib import Path
import json
ROOT=Path(__file__).resolve().parents[1]
class IDs(HTMLParser):
 def __init__(self):super().__init__();self.ids=set();self.buttons={};self.panel_ids=set()
 def handle_starttag(self,tag,attrs):
  a=dict(attrs)
  if 'id' in a:
   assert a['id'] not in self.ids, a['id']
   self.ids.add(a['id'])
   if 'settingsPanel' in a['id']: self.panel_ids.add(a['id']); assert 'hidden' in a
  if 'data-settings-open' in a:self.buttons[a['data-settings-open']]=a.get('aria-controls')
text=(ROOT/'ui/index.html').read_text(encoding='utf-8')
s=IDs();s.feed(text)
assert all(k in s.ids for k in ('uiLanguage','ledBrightness','ledHeartbeat','ledSave','rxLogSave','rxLogLimit0','rxLogLimit1','rxLogLimit2','rxLogNoiseLimit','firmwareVersion','firmwareRevision','firmwareBuilt','haMqttStatus','libraryCard','decoderLibraryCard','userLibraryCard','diagnosticsDetails'))
assert s.buttons=={'library':'settingsPanelLibrary','decoders':'settingsPanelDecoders','custom':'settingsPanelCustom','systemtrace':'settingsPanelSystemtrace','diagnostics':'settingsPanelDiagnostics'}
assert 'function showSettingsPanel(' in text and "showSettingsPanel('library',true,true)" in text
for lang in ('en','nl','fr','de'):
 d=json.loads((ROOT/'ui/locales'/f'{lang}.json').read_text(encoding='utf-8'))
 assert all(d.get(k) for k in ('settings.general','settings.integrations','settings.additional','settings.dashboard_hint'))
print('SETTINGS_COMPACT1 DOM, access points, retained IDs, four locales: PASS')
