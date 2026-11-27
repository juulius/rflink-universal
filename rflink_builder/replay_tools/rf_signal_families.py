"""Bounded, offline timing families. No device identity or protocol claim."""
from collections import Counter, defaultdict
from statistics import median


def _modes(values):
    center = median(values)
    if all(abs(v-center) <= center*.25 for v in values):
        return (center,), None
    a, b = min(values), max(values)
    for _ in range(8):
        labels = [int(abs(v-b) < abs(v-a)) for v in values]
        groups = [[v for v, k in zip(values, labels) if k == i] for i in (0, 1)]
        if min(map(len, groups)) < 2:
            return (), None
        a, b = map(median, groups)
    if not 1.5 <= b/a <= 8 or any(abs(v-(a,b)[k]) > (a,b)[k]*.25 for v,k in zip(values,labels)):
        return (), None
    return (a,b), labels


def timing_family(pulses):
    p = list(pulses)
    if len(p) < 32 or any(not v for v in p):
        return None
    # Only an obvious trailing idle or unpaired last duration is omitted.
    if p[-1] < 0 and abs(p[-1]) >= max(10000, 4*median(map(abs,p[:-1]))):
        p.pop()
    if len(p)%2:
        p.pop()
    if len(p)<32 or any((a>0)==(b>0) for a,b in zip(p,p[1:])):
        return None
    h, l = [v for v in p if v>0], [-v for v in p if v<0]
    hm, hb = _modes(h)
    lm, lb = _modes(l)
    if len(hm)==1 and lb is not None:
        kind,bits = 'pulse_distance',lb
    elif len(lm)==1 and hb is not None:
        kind,bits = 'pulse_width',hb
    elif hb is not None and lb is not None and all(a!=b for a,b in zip(hb,lb)):
        kind,bits = 'complementary_widths',hb
    else:
        return None
    return kind, p[0]>0, tuple(hm+lm), ''.join(map(str,bits))


def analyze_signal_families(captures, outputs):
    groups = defaultdict(list)
    all_groups = []
    seen = set()
    stats = Counter()
    for cap,out in zip(captures,outputs):
        if cap.get('format')!='signed_pulses_us' or cap.get('capture_phase')!='pre_filter_rmt':
            continue
        cid=cap['capture_id']
        if cid in seen:
            stats['duplicate_rows_ignored']+=1
            continue
        seen.add(cid)
        stats['original_captures']+=1
        model=timing_family(cap['pulses_us'])
        if model is None:
            continue
        stats['timing_candidates']+=1
        kind,polarity,centers,bits=model
        key=(cap['radio'],kind,polarity,len(bits))
        bucket=groups[key]
        g=next((v for v in bucket if all(.85<=a/b<=1.18 for a,b in zip(centers,v['centers']))),None)
        if g is None:
            if len(bucket)>=16 or len(all_groups)>=256:
                stats['group_limit_skipped']+=1
                continue
            g={'id':f'F{len(all_groups)+1}', 'radio':cap['radio'],'kind':kind,
               'bit_count':len(bits),'centers':centers,'items':[]}
            bucket.append(g)
            all_groups.append(g)
        g['items'].append((cap,bits,out))
    result=[]
    for g in all_groups:
        items=g['items']
        if len(items)<2:
            continue
        payloads=Counter(bits for _,bits,_ in items)
        fixed=sum(len(set(col))==1 for col in zip(*payloads))
        # This is one predefined structural check, not a CRC or identity.
        inverse=Counter(sum(a!=b for a,b in zip(bits[:32],bits[32:64]))
                        for _,bits,_ in items if len(bits)>=64)
        by_boot=defaultdict(set)
        for cap,_,_ in items:
            try:
                boot=cap.get('boot_id')
                if boot is not None and str(boot).strip():
                    by_boot[str(boot)].add(int(cap['uptime_ms']))
            except (ValueError,TypeError,KeyError):
                pass
        intervals=[]
        bursts=0
        for times in by_boot.values():
            starts=[]
            previous=None
            for t in sorted(times):
                if previous is None or t-previous>=250:
                    starts.append(t)
                previous=t
            bursts+=len(starts)
            intervals.extend(b-a for a,b in zip(starts,starts[1:]))
        for _,_,out in items:
            out['signal_family_id']=g['id']
        row={'family_id':g['id'],'radio':g['radio'],'kind':g['kind'],
             'bit_count':g['bit_count'],'timing_centers_us':list(g['centers']),
             'captures':len(items),'distinct_patterns':len(payloads),
             'largest_exact_repeat':max(payloads.values()),'fixed_positions':fixed,
             'variable_positions':g['bit_count']-fixed,'inverse32_exact':inverse[32],
             'inverse32_one_difference':inverse[31],'inverse32_tested':sum(inverse.values()),
             'observed_bursts_with_time':bursts,'interval_count':len(intervals),
             'median_interval_ms':median(intervals) if intervals else None,
             'protocol_verified':False}
        result.append(row)
    result.sort(key=lambda g:(-g['captures'],g['family_id']))
    stats['repeated_families']=len(result)
    stats['captures_in_repeated_families']=sum(g['captures'] for g in result)
    return {'version':1,'summary':dict(stats),'families':result[:40],
            'notes':['Timing families are not devices or protocol matches.',
                     'Intervals use boot-local uptime; captures less than 250 ms apart are grouped only for cadence.',
                     'Missing captures and mixed sources can distort cadence. No reception-rate estimate.']}
