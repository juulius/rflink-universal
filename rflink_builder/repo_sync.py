"""Conservative, content-aware source synchronisation for RFLink Universal.

Private settings are never read from a source ZIP; source updates never delete files.
Only source files matching their previously imported hash can be overwritten.
"""
from __future__ import annotations
import hashlib
import io
import json
import os
import re
import shutil
import tempfile
import time
import zipfile
from pathlib import Path, PurePosixPath
from urllib.request import Request, urlopen

MAX_REMOTE=32*1024*1024
REPO_PATTERN=re.compile(r'^[A-Za-z0-9_.-]{1,100}/[A-Za-z0-9_.-]{1,100}$')
IGNORED_NAMES={'sdkconfig','sdkconfig.old','secrets.yaml','.env','mqtt_settings.json','local_settings.json'}
IGNORED_PARTS={'build','release','managed_components','__pycache__','.git','local_settings','private','.idf_component_manager'}
IGNORED_SUFFIXES={'.key','.pem','.p12','.pfx'}

def is_private(path: str)->bool:
    parts=PurePosixPath(path).parts
    return bool(not parts or any(x in IGNORED_PARTS for x in parts) or parts[-1] in IGNORED_NAMES or
                (parts[-1].startswith('.env')) or PurePosixPath(path).suffix in IGNORED_SUFFIXES)

def digest(raw:bytes)->str: return hashlib.sha256(raw).hexdigest()

class SourceStore:
    def __init__(self, data:Path, project:Path, zip_inspector):
        self.data=Path(data); self.project=Path(project)
        self.inspector=zip_inspector
        self.baseline=self.data/'source_baseline.json'
        self.pending=None

    def baseline_map(self):
        try: return json.loads(self.baseline.read_text('utf-8'))
        except (OSError,ValueError): return {}

    def reset_baseline(self):
        files={p.relative_to(self.project).as_posix():digest(p.read_bytes()) for p in self.project.rglob('*')
               if p.is_file() and not is_private(p.relative_to(self.project).as_posix())}
        self._write_baseline(files)

    def _write_baseline(self, manifest):
        self.data.mkdir(parents=True,exist_ok=True)
        temp=self.baseline.with_suffix('.new')
        temp.write_text(json.dumps(manifest,sort_keys=True,indent=2),encoding='utf-8')
        temp.replace(self.baseline)

    def preview(self,data:bytes, source='ZIP'):
        if len(data)>MAX_REMOTE:raise ValueError('Broncodepakket te groot')
        zf,entries=self.inspector(data)
        try:
            baseline=self.baseline_map()
            change=[]
            incoming={}
            for info,rel in entries:
                if is_private(rel): continue
                incoming[rel]=digest(zf.read(info))
                destination=self.project.joinpath(*PurePosixPath(rel).parts)
                if not destination.exists():kind='new'
                elif not destination.is_file() or destination.is_symlink():kind='conflict'
                else:
                    current=digest(destination.read_bytes())
                    if current==incoming[rel]:kind='same'
                    elif rel not in baseline or baseline[rel]!=current:kind='conflict'
                    else:kind='updated'
                change.append({'path':rel,'status':kind})
            for rel in sorted(set(baseline)-set(incoming)):
                if not is_private(rel):change.append({'path':rel,'status':'obsolete'})
        finally: zf.close()
        count={k:sum(c['status']==k for c in change) for k in ('new','updated','same','conflict','obsolete')}
        self.pending={'token':digest(data),'bytes':data,'time':time.monotonic(),'source':source}
        return {'token':digest(data), 'source':source,'counts':count,'changes':change[:250],
                'note':'Conflicting and removed upstream files are not overwritten or deleted.'}

    def apply(self, token:str):
        pending=self.pending
        if not pending or token!=pending['token'] or time.monotonic()-pending['time']>900:
            raise ValueError('No valid comparison available. Run the comparison again before updating.')
        data=pending['bytes']
        preview=self.preview(data,pending['source'])
        zf,entries=self.inspector(data)
        statuses={c['path']:c['status'] for c in preview['changes']}
        # Statuses for >250 items need recomputing, not truncated preview.
        baseline=self.baseline_map()
        applied=0
        backup=self.data/'source_backups'/str(int(time.time()))
        try:
            for info,rel in entries:
                if is_private(rel):continue
                destination=self.project.joinpath(*PurePosixPath(rel).parts)
                new=zf.read(info)
                old=digest(destination.read_bytes()) if destination.is_file() and not destination.is_symlink() else None
                newhash=digest(new)
                if old==newhash:baseline[rel]=newhash;continue
                if old is not None and (rel not in baseline or old!=baseline[rel]):continue
                if destination.exists() and (not destination.is_file() or destination.is_symlink()):continue
                if destination.exists():
                    saved=backup.joinpath(*PurePosixPath(rel).parts)
                    saved.parent.mkdir(parents=True,exist_ok=True)
                    shutil.copy2(destination,saved)
                destination.parent.mkdir(parents=True,exist_ok=True)
                # atomic replacement avoids partially written source files
                with tempfile.NamedTemporaryFile(dir=destination.parent,delete=False) as tmp:
                    tmp.write(new); tmpname=tmp.name
                os.replace(tmpname,destination)
                baseline[rel]=newhash;applied+=1
            self._write_baseline(baseline)
        finally:zf.close();self.pending=None
        return {'ok':True,'applied':applied,'counts':preview['counts'],'backup':str(backup) if backup.exists() else None}


def valid_repo(text:str):
    if not isinstance(text,str) or not REPO_PATTERN.fullmatch(text):
        raise ValueError('GitHub-repository moet eigenaar/repository zijn')
    if any(piece.startswith('.') or piece.endswith('.') for piece in text.split('/')):
        raise ValueError('Ongeldige GitHub-repository')
    return text


def read_http(url:str, limit:int=MAX_REMOTE):
    req=Request(url,headers={'Accept':'application/vnd.github+json','User-Agent':'RFLink-Universal-Builder/0.4'})
    with urlopen(req,timeout=18) as f:
        content=f.read(limit+1)
        if len(content)>limit:raise ValueError('GitHub-download overschrijdt de limiet')
        return content


def github_head(repo:str):
    repo=valid_repo(repo)
    doc=json.loads(read_http(f'https://api.github.com/repos/{repo}/commits/main',32768))
    sha=doc.get('sha','')
    if not re.fullmatch(r'[a-f0-9]{40}',sha):raise ValueError('GitHub did not return a valid commit SHA')
    return sha


def github_firmware_zip(repo:str,sha:str):
    repo=valid_repo(repo)
    if not re.fullmatch(r'[a-f0-9]{40}',sha):raise ValueError('Ongeldige commit-ID')
    # Pin the downloaded archive to an immutable SHA, never a moving branch.
    raw=read_http(f'https://api.github.com/repos/{repo}/zipball/{sha}')
    with zipfile.ZipFile(io.BytesIO(raw)) as z:
        selected={}
        total=0
        if len(z.infolist())>2500:raise ValueError('Too many files in the GitHub archive')
        for info in z.infolist():
            if info.is_dir():continue
            parts=PurePosixPath(info.filename).parts
            if len(parts)<3 or parts[1]!='firmware':continue
            local='/'.join(parts[2:])
            if not local or local.startswith('/') or '..' in PurePosixPath(local).parts:raise ValueError('Onveilig repositorypad')
            if is_private(local):continue
            if ((info.external_attr >> 16) & 0o170000)==0o120000:
                raise ValueError('Symbolic links are not allowed in GitHub firmware source')
            if local in selected:raise ValueError('Dubbel firmwarepad in GitHub-archief')
            if info.file_size>12*1024*1024:raise ValueError('Source file is too large')
            total+=info.file_size
            if total>96*1024*1024:raise ValueError('Expanded GitHub firmware source is too large')
            selected[local]=z.read(info)
        if 'CMakeLists.txt' not in selected or 'main/CMakeLists.txt' not in selected:
            raise ValueError('Repository does not contain a complete firmware project')
    out=io.BytesIO()
    with zipfile.ZipFile(out,'w',zipfile.ZIP_DEFLATED) as f:
        for name,content in selected.items():f.writestr(name,content)
    return out.getvalue()


def github_release(repo:str):
    """Metadata only. Availability never implies an OTA image is safe to flash."""
    repo=valid_repo(repo)
    try: doc=json.loads(read_http(f'https://api.github.com/repos/{repo}/releases/latest',64*1024))
    except Exception: return {'available':False,'note':'No published stable firmware release found'}
    assets=[{'name':a.get('name',''),'size':a.get('size',0)} for a in doc.get('assets',[]) if isinstance(a,dict)]
    return {'available':True,'version':doc.get('tag_name',''), 'published_at':doc.get('published_at',''),
            'url':doc.get('html_url',''), 'assets':assets,
            'note':'For information only. USB/OTA is available only after board, hash and hardware checks.'}
