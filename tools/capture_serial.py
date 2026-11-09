"""Read-only hardware capture; run only when the bridge has released the port.
No commands are written. Opening some USB adapters may still reset the board.
"""
import argparse,json,time
from datetime import datetime,timezone
from pathlib import Path

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--port',required=True)
    p.add_argument('--seconds',type=float,default=30)
    p.add_argument('--output',type=Path,required=True,help='New directory; existing paths refused')
    args=p.parse_args()
    if not 0<args.seconds<=3600: p.error('seconds must be between 0 and 3600')
    import serial
    args.output.mkdir(parents=True,exist_ok=False)
    meta=dict(source='hardware serial read',port=args.port,baud=921600,
              started_utc=datetime.now(timezone.utc).isoformat(),complete=False,
              commands_written=0,notes='Host read timestamps are not per-byte wire arrival times; USB/OS batching applies.')
    link=None;offset=0
    try:
        link=serial.Serial(port=None,baudrate=921600,timeout=.01)
        link.dtr=False;link.rts=False;link.port=args.port;link.open()
        start=time.monotonic_ns()
        with (args.output/'raw.bin').open('xb') as raw, (args.output/'host_reads.jsonl').open('x') as timing:
            while (time.monotonic_ns()-start)/1e9<args.seconds:
                before=time.monotonic_ns()
                chunk=link.read(min(4096,max(1,link.in_waiting)))
                after=time.monotonic_ns()
                if not chunk: continue
                raw.write(chunk)
                timing.write(json.dumps(dict(offset=offset,length=len(chunk),read_start_ns=before,read_end_ns=after))+'\n')
                offset+=len(chunk)
        meta['complete']=True
    finally:
        if link is not None: link.close()
        meta['bytes']=offset
        (args.output/'metadata.json').write_text(json.dumps(meta,indent=2)+'\n')
    print(str(args.output))
if __name__=='__main__': main()
