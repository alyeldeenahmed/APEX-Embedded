"""Standalone IMU bench CSV recorder/analyzer. Does not drive or flash hardware."""
import argparse
import csv
import json
import math
from pathlib import Path
import statistics
import time

FIELDS = 'seq,uptime_us,sample_us,state,fault,present,valid,calibrating,ax,ay,az,gx,gy,gz,attempts,recoveries,read_errors,config_errors,no_data,clipped,late,ambiguous,missed_edges,tx_drops,generation'.split(',')
FLOATS = set('ax ay az gx gy gz'.split())


def parse_line(line):
    parts=line.strip().split(',')
    if len(parts)!=len(FIELDS) or parts[0]=='seq': return None
    try:
        row={k: float(v) if k in FLOATS else int(v) for k,v in zip(FIELDS,parts)}
        if not all(math.isfinite(v) for v in row.values()): return None
        if any(row[k]<0 for k in FIELDS if k not in FLOATS): return None
        if row['state']>4 or row['fault']>4: return None
        if any(row[k] not in (0,1) for k in ('present','valid','calibrating')): return None
        if row['valid'] and (not row['present'] or row['state']!=4 or row['sample_us']>row['uptime_us']): return None
        return row
    except (ValueError,OverflowError): return None


def percentile(values,p):
    if not values: return None
    values=sorted(values)
    return values[max(0,math.ceil(p*len(values))-1)]


def summarize(rows):
    intervals=[]; resets=0; missing=0; duplicates=0; same_samples=0; sample_backwards=0
    usable=[]; ages=[]
    for i,r in enumerate(rows):
        new_sample=True
        if i:
            old=rows[i-1]
            if r['uptime_us']<old['uptime_us']:
                resets+=1
            else:
                intervals.append((r['uptime_us']-old['uptime_us'])/1000)
                delta=(r['seq']-old['seq']) & 0xffffffff
                if delta==0: duplicates+=1
                elif delta<0x80000000: missing+=delta-1
                if r['present'] and old['present']:
                    new_sample=r['sample_us']!=old['sample_us']
                    same_samples+=int(not new_sample)
                    sample_backwards+=int(r['sample_us']<old['sample_us'])
        if r['present'] and r['sample_us']<=r['uptime_us']:
            ages.append((r['uptime_us']-r['sample_us'])/1000)
        if r['valid'] and not r['calibrating'] and new_sample: usable.append(r)
    stats={}
    for k in FLOATS:
        data=[r[k] for r in usable]
        stats[k]={'mean':statistics.mean(data),'stddev':statistics.pstdev(data),'min':min(data),'max':max(data)} if data else None
    norms=[math.sqrt(sum(r[k]**2 for k in ('ax','ay','az'))) for r in usable]
    return dict(rows=len(rows),valid_rows=sum(r['valid'] for r in rows),
        invalid_rows=sum(not r['valid'] for r in rows),usable_unique_samples=len(usable),
        observed_resets=resets,missing_records=missing,duplicate_records=duplicates,
        held_sample_rows=same_samples,backwards_sample_stamps=sample_backwards,
        device_report_spacing_ms=dict(p50=percentile(intervals,.5),p999=percentile(intervals,.999),max=max(intervals) if intervals else None),
        sample_age_ms_p95=percentile(ages,.95),
        acceleration_norm_mean=statistics.mean(norms) if norms else None,
        axes=stats,last_diagnostics={k:rows[-1][k] for k in FIELDS[14:]} if rows else {},
        note='Bench diagnostics, not an acceptance certificate. Timing uses device report stamps, not physical sample accuracy or host arrival spacing. Bias statistics require an intentionally stationary capture; no bias is subtracted.')


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--list',action='store_true'); p.add_argument('--port')
    p.add_argument('--seconds',type=float,default=60)
    p.add_argument('--output',type=Path,default=Path('captures/imu.csv'))
    p.add_argument('--analyze',type=Path)
    a=p.parse_args()
    if a.analyze:
        with a.analyze.open(newline='') as f:
            reader=csv.DictReader(f); rows=[]
            for raw in reader:
                row=parse_line(','.join(raw.get(k,'') for k in FIELDS))
                if row is not None: rows.append(row)
        print(json.dumps(summarize(rows),indent=2)); return
    try:
        import serial
        from serial.tools import list_ports
    except ImportError:
        p.error('Install the bench dependency: python -m pip install -r requirements-bench.txt')
    if a.list:
        for port in list_ports.comports(): print(port.device,port.description)
        return
    if not a.port: p.error('--port is required (or use --list / --analyze)')
    if not math.isfinite(a.seconds) or a.seconds<=0: p.error('--seconds must be positive and finite')
    a.output.parent.mkdir(parents=True,exist_ok=True)
    rows=[]; ignored=0; last_print=0
    # Exclusive output prevents accidentally overwriting a previous test capture.
    with a.output.open('x',newline='') as f:
        writer=csv.DictWriter(f,fieldnames=['host_monotonic_s']+FIELDS); writer.writeheader()
        with serial.Serial(a.port,921600,timeout=.1) as stream:
            start=time.monotonic(); pending=bytearray()
            try:
                while time.monotonic()-start<a.seconds:
                    pending.extend(stream.read(stream.in_waiting or 1))
                    while b'\n' in pending:
                        line,_,pending=pending.partition(b'\n')
                        row=parse_line(line.decode('ascii',errors='replace'))
                        if row is None:
                            if not line.startswith(b'seq,'): ignored+=1
                            continue
                        now=time.monotonic(); rows.append(row)
                        writer.writerow(dict(host_monotonic_s=now,**row))
                        if now-last_print>=1:
                            print(f"state={row['state']} fault={row['fault']} valid={row['valid']} cal={row['calibrating']} accel=({row['ax']:.3f},{row['ay']:.3f},{row['az']:.3f}) gyro=({row['gx']:.3f},{row['gy']:.3f},{row['gz']:.3f})")
                            last_print=now
                    if len(pending)>8192: pending.clear(); ignored+=1
            except KeyboardInterrupt: pass
    report=summarize(rows); report['ignored_lines']=ignored
    report_path=a.output.with_suffix('.summary.json')
    with report_path.open('x') as f: json.dump(report,f,indent=2)
    print(json.dumps(report,indent=2))
    if not rows: raise SystemExit('No bench records received: check UART connector, COM port and bench firmware.')


if __name__=='__main__': main()
