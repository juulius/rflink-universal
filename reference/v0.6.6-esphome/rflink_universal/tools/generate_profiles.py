#!/usr/bin/env python3
"""Validate built-in device profiles and generate the immutable C++ catalog."""
from __future__ import annotations
import json
from pathlib import Path
import re

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'components'/'rflink_universal'/'profile_catalog_generated.h'
PROFILE_DIR=ROOT/'profiles'
ID=re.compile(r'^[a-z][a-z0-9_]{0,31}$')
LOCALES=('en','nl','fr','de')

def cstr(value:str)->str:
    return '"'+value.replace('\\','\\\\').replace('"','\\"').replace('\n','\\n')+'"'

def protocol_map():
    out={}
    for folder in sorted((ROOT/'decoders').iterdir()):
        manifest_path=folder/'manifest.json'
        if not folder.is_dir() or folder.name.startswith('_') or not manifest_path.exists():
            continue
        m=json.loads(manifest_path.read_text(encoding='utf-8'))
        if not m.get('enabled',True):
            continue
        for cap in m.get('capabilities',[]):
            name=cap['protocol']
            if name in out: raise SystemExit(f'duplicate decoder protocol: {name}')
            out[name]=cap.get('rx_actions',[])
    return out

def action_allowed(tokens:list[str], action:str)->bool:
    for token in tokens:
        if token==action:return True
        if token.endswith('*') and action.startswith(token[:-1]) and len(action)>len(token)-1:return True
    return False

def load_profiles():
    protocols=protocol_map(); result=[]; ids=set()
    for path in sorted(PROFILE_DIR.glob('*.json')):
        p=json.loads(path.read_text(encoding='utf-8'))
        if p.get('schema')!=1: raise SystemExit(f'{path}: schema must be 1')
        pid=p.get('id','')
        if not ID.fullmatch(pid): raise SystemExit(f'{path}: invalid profile id {pid!r}')
        if path.stem!=pid: raise SystemExit(f'{path}: filename must equal profile id')
        if pid in ids: raise SystemExit(f'{path}: duplicate profile id')
        ids.add(pid)
        protocol=p.get('protocol','')
        if protocol not in protocols: raise SystemExit(f'{path}: unknown/disabled protocol {protocol!r}')
        for action in p.get('expected_actions',[]):
            if not ID.fullmatch(action): raise SystemExit(f'{path}: invalid expected action {action!r}')
            if not action_allowed(protocols[protocol],action): raise SystemExit(f'{path}: action {action!r} absent from {protocol}')
        policy=p.get('suggestion_policy','protocol_candidate')
        if policy not in ('protocol_candidate','manual_select'):
            raise SystemExit(f'{path}: unsupported suggestion policy {policy!r}')
        setup=p.get('setup_steps',{})
        if setup:
            if set(setup)!=set(LOCALES) or any(not isinstance(setup[x],list) or len(setup[x])>8 or not setup[x] or any(not isinstance(v,str) or not v.strip() or '|' in v for v in setup[x]) for x in LOCALES):
                raise SystemExit(f'{path}: setup_steps must contain up to 8 nonempty localized steps')
        wizard=p.get('pairing',{})
        if not isinstance(wizard,dict) or wizard.get('method','none') not in (
            'none','manual','receiver_learn','virtual_remote','unsupported'):
            raise SystemExit(f'{path}: invalid pairing method')
        if wizard and not isinstance(wizard.get('required_tx',False),bool):
            raise SystemExit(f'{path}: pairing required_tx must be boolean')
        buttons=p.get('controls', [])
        if not isinstance(buttons, list) or len(buttons)>24 or any(not isinstance(x,str) or not ID.fullmatch(x) for x in buttons) or len(set(buttons))!=len(buttons):
            raise SystemExit(f'{path}: controls must be unique, valid action IDs')
        if p.get('device_type') in ('weather_sensor', 'temperature_sensor') and buttons:
            raise SystemExit(f'{path}: sensor must not expose transmit controls')
        models=p.get('models',[]); sources=p.get('sources',[])
        if not isinstance(models,list) or not models or any(not isinstance(x,str) or not x.strip() for x in models):
            raise SystemExit(f'{path}: models must be a non-empty string list')
        if not isinstance(sources,list) or not sources: raise SystemExit(f'{path}: sources are required')
        for src in sources:
            if not isinstance(src,dict) or not src.get('label') or not str(src.get('url','')).startswith('https://') or not src.get('kind'):
                raise SystemExit(f'{path}: invalid source entry')
        for block in ('learn_help','notes'):
            value=p.get(block,{})
            if set(value)!=set(LOCALES) or any(not isinstance(value[x],str) or not value[x].strip() for x in LOCALES):
                raise SystemExit(f'{path}: {block} must contain exactly {LOCALES}')
        for key in ('name','manufacturer','family','device_type','compatibility','hardware_validation'):
            if not isinstance(p.get(key),str) or not p[key].strip(): raise SystemExit(f'{path}: missing {key}')
        result.append(p)
    if not result: raise SystemExit('no built-in device profiles found')
    return result

def generate(profiles):
    lines=['// AUTOGENERATED from profiles/*.json; DO NOT EDIT.','#pragma once','#include <cstddef>','#include <cstring>',
           'namespace esphome { namespace rflink_universal { namespace profiles {',
           'struct DeviceProfile { const char *id, *name, *protocol, *manufacturer, *family, *models, *device_type, *expected_actions, *controls, *compatibility, *hardware_validation, *learn_en, *learn_nl, *learn_fr, *learn_de, *notes_en, *notes_nl, *notes_fr, *notes_de, *sources, *suggest_policy, *setup_en, *setup_nl, *setup_fr, *setup_de, *pair_method; bool pair_tx_required; };',
           'inline const DeviceProfile *all(size_t &count) { static const DeviceProfile data[] = {']
    for p in profiles:
        vals=[p['id'],p['name'],p['protocol'],p['manufacturer'],p['family'],'|'.join(p['models']),p['device_type'],'|'.join(p.get('expected_actions',[])),'|'.join(p.get('controls',[])),p['compatibility'],p['hardware_validation'],
              p['learn_help']['en'],p['learn_help']['nl'],p['learn_help']['fr'],p['learn_help']['de'],p['notes']['en'],p['notes']['nl'],p['notes']['fr'],p['notes']['de'],
              '|'.join(f"{s['kind']}::{s['label']}::{s['url']}" for s in p['sources']),p.get('suggestion_policy','protocol_candidate'),*[('|'.join(p.get('setup_steps',{}).get(lang,[]))) for lang in LOCALES], p.get('pairing',{}).get('method','none')]
        lines.append('  {'+', '.join(cstr(v) for v in vals)+', '+('true' if p.get('pairing',{}).get('required_tx',False) else 'false')+'},')
    lines += ['}; count=sizeof(data)/sizeof(data[0]); return data; }',
              'inline const DeviceProfile *find(const char *id) { size_t count=0; const auto *items=all(count); for(size_t i=0;i<count;i++) if(std::strcmp(items[i].id,id)==0) return &items[i]; return nullptr; }',
              'inline size_t count() { size_t n=0; (void) all(n); return n; }',
              '}}} // namespace esphome::rflink_universal::profiles','']
    return '\n'.join(lines)

def main():
    profiles=load_profiles(); OUT.write_text(generate(profiles),encoding='utf-8')
    print(f'Generated {len(profiles)} built-in device profiles')

if __name__=='__main__': main()
