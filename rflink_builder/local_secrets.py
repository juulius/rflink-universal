"""Simple local credential vault in the Home Assistant app's persistent /data volume.

Values never enter source archives or generated firmware by default.
They are not encrypted at rest; Home Assistant backups also require protection.
"""
import json
from pathlib import Path

class LocalSecrets:
    def __init__(self,data):
        self.root=Path(data)/'local'; self.file=self.root/'secrets.json'

    def load(self):
        try:return json.loads(self.file.read_text('utf-8'))
        except (OSError,ValueError):return {}

    def status(self):
        obj=self.load()
        return {'wifi_ssid':obj.get('wifi_ssid',''),'wifi_password_set':bool(obj.get('wifi_password')),
                'ota_key_set':bool(obj.get('ota_key')),'note':'Stored only in /data/local; never embedded in firmware or sent to GitHub.'}

    def save(self, settings):
        if not isinstance(settings,dict):raise ValueError('Invalid secret settings')
        ssid=settings.get('wifi_ssid','')
        if not isinstance(ssid,str) or len(ssid.encode('utf-8'))>32:raise ValueError('WiFi-SSID is te lang (max. 32 bytes)')
        previous=self.load()
        out={'wifi_ssid':ssid,'wifi_password':previous.get('wifi_password',''),'ota_key':previous.get('ota_key','')}
        for key,minimum,maximum in [('wifi_password',8,63),('ota_key',12,64)]:
            val=settings.get(key,'')
            if not isinstance(val,str):raise ValueError('Ongeldige toegangssleutel')
            if val and not minimum<=len(val)<=maximum:raise ValueError(f'{key}: verwacht {minimum} tot {maximum} tekens')
            if val:out[key]=val
        self.root.mkdir(parents=True,exist_ok=True)
        temp=self.file.with_suffix('.tmp')
        temp.write_text(json.dumps(out)+'\n',encoding='utf-8')
        temp.chmod(0o600)
        temp.replace(self.file)
        return self.status()

    def clear(self):
        self.file.unlink(missing_ok=True)
        return self.status()
