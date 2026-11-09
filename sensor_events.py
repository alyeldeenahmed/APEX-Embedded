"""Raw sensor-event adapter for the Pi bridge; no EKF, ROS clock mapping or trust policy.

Feed ordered records from protocol.Decoder, with the host monotonic receipt time.
Requires status boot identity and matched data/timing pairs. Invalid/held IMU data
never becomes a new observation. An emitted IMU event is only a candidate for the
Perception team's bias, model agreement and ten-sample recovery checks.
"""
import math


class SensorEvents:
    def __init__(self):
        self.boot = None
        self.pending = None
        self.last = {}
        self.report = -1
        self.rejected = 0

    def feed(self, record, received_monotonic_us):
        kind = record['type']
        if kind == 'status':
            if self.boot != record['boot']:
                self.boot = record['boot']
                self.pending = None
                self.last.clear()
                self.report = -1
            return []
        if kind == 'data':
            if self.pending is not None: self.rejected += 1
            self.pending = record if self.boot is not None else None
            return []
        if kind != 'timing':
            return []
        data, self.pending = self.pending, None
        if data is None or data['seq'] != record['seq']:
            self.rejected += 1
            return []
        report = record['report_us']
        if report < self.report:
            self.boot = None
            self.last.clear()
            self.report = -1
            self.rejected += 1
            return []  # Require status confirmation after a backwards clock.
        present = record['present']
        stamps = [record[k] for k in ('imu_us', 'wheel_us', 'steering_us')]
        values = (*data['gyro'], *data['accel'], data['wheel_speed'], data['steering'])
        if (report <= self.report or any(present & (1 << i) and stamps[i] > report for i in range(3))
                or (data['flags'] & 7) & ~present or not all(math.isfinite(x) for x in values)
                or (present & 1 and stamps[0] & 0xffffffff != data['t_imu_us'])):
            self.rejected += 1
            return []
        self.report = report
        events = []
        for bit, name, payload in (
            (0, 'imu', dict(gyro=data['gyro'], accel=data['accel'], calibrating=bool(data['flags'] & 8))),
            (1, 'wheel', dict(speed=data['wheel_speed'], ticks=data['enc_ticks'])),
            (2, 'steering', dict(angle=data['steering'], measured=bool(data['flags'] & 32))),
        ):
            stamp = stamps[bit]
            if name == 'wheel':
                diagnostics=record.get('diagnostics',0)
                if record.get('version',1)==2:
                    if diagnostics & 2: continue
                    measured=bool(diagnostics & 1)
                    if not measured and data['wheel_speed']<0: continue
                    payload.update(direction_measured=measured,
                        motion_monitor_enabled=bool(diagnostics & 4))
                else:
                    payload.update(direction_measured=None, motion_monitor_enabled=None)
            if data['flags'] & (1 << bit) and stamp > self.last.get(name, -1):
                self.last[name] = stamp
                events.append(dict(sensor=name, boot=self.boot, sample_us=stamp,
                    report_us=report, received_monotonic_us=received_monotonic_us, **payload))
        return sorted(events, key=lambda e: e['sample_us'])
