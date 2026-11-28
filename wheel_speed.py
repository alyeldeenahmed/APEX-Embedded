"""Qualified commissioning speed for each matched report, in ESP boot time."""
import math


class WheelSpeed:
    def __init__(self):
        self.boot = None
        self.data = self.detail = self.report = None
        self.source = self.value = None

    def reset(self, boot=None):
        self.__init__()
        self.boot = boot

    def feed(self, event):
        kind = event['type']
        if kind == 'status':
            if event['boot'] != self.boot:
                self.reset(event['boot'])
        elif kind == 'wheel_detail':
            self.detail = event if event['boot'] == self.boot else None
        elif kind == 'data':
            if event['flags'] & 64 and self.report is not None:
                self.reset()
            self.data = event
        elif kind == 'timing':
            data, detail = self.data, self.detail
            self.data = self.detail = None
            if not data or data['seq'] != event['seq']:
                return
            report = event['report_us']
            if self.report is not None and report < self.report:
                self.reset()
            detail = detail if (detail and detail['seq']==event['seq'] and
                                detail['report_us']==report and detail['boot']==self.boot) else None
            self.report = report
            state, valid = 'unavailable', False
            stamp, speed = event['wheel_us'], data['wheel_speed']
            suspect = bool((detail and detail['suspect']) or event.get('diagnostics', 0)&2)
            if suspect:
                self.source = self.value = None
            elif self.boot is not None:
                if (event['present'] & 2 and 0 < stamp <= report and report-stamp <= 25000
                        and stamp > (self.source or 0) and math.isfinite(speed)):
                    self.source, self.value = stamp, speed
                    state, valid = 'measured', bool(data['flags'] & 2)
                elif detail and detail['segment_present'] and detail['end_us'] > (self.source or 0):
                    self.source, self.value = detail['end_us'], detail['speed_mps']
                    state = 'short_window'
                elif self.source is not None and 0 <= report-self.source < 50000:
                    state = 'held'
            age = report-self.source if self.source is not None else None
            event['wheel_speed_output'] = dict(state=state, speed_mps=self.value if state!='unavailable' else None,
                last_known_mps=self.value, source_us=self.source, age_us=age, wheel_valid=valid)
