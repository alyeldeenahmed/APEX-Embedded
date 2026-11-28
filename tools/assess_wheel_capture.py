"""Summarize a decoded integration capture with the same speed selection as the Pi."""
import argparse
from collections import Counter
import json
from pathlib import Path
import sys

sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from wheel_speed import WheelSpeed


def assess(records):
    selector=WheelSpeed()
    counts=Counter()
    orphan_timings=0
    diagnostics={}
    for record in records:
        event=dict(record)
        if event['type']=='wheel_detail':
            diagnostics[str(event['boot'])]={key:event[key] for key in
                ('read_errors','timing_gaps','ambiguous_gaps')}
        event.pop('wheel_speed_output',None)
        selector.feed(event)
        if event['type']=='timing':
            output=event.get('wheel_speed_output')
            if output is None:
                orphan_timings+=1
            else:
                counts[output['state']]+=1
    total=sum(counts.values())
    return dict(paired_reports=total,orphan_timings=orphan_timings,
                states={key:counts[key] for key in ('measured','short_window','held','unavailable')},
                numeric_percent=round(100*(total-counts['unavailable'])/total,2) if total else None,
                last_encoder_counters_by_boot=diagnostics)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('capture',type=Path)
    args=parser.parse_args()
    with args.capture.open(encoding='utf-8') as f:
        print(json.dumps(assess(json.loads(line) for line in f if line.strip()),indent=2))
