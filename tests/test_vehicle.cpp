#include "firmware/telemetry_demo/vehicle_core.h"
#include <cassert>
#include <limits>
int main() {
  vehicle::Wheel w;
  w.update(0,10000,0.01f,true); assert(!w.valid);
  w.update(0,20000,0.01f,true); assert(w.valid && w.speed==0);
  w.update(2,30000,0.01f,true); assert(w.valid && fabsf(w.speed-2)<0.0001f);
  w.update(2,40000,0,false); assert(!w.valid);
  vehicle::Controller c(42);
  vehicle::Command cmd{42,1,100000,0,0,1};
  assert(c.accept(cmd,0)==vehicle::Result::Uncalibrated);
  c.calibration.max_angle=0.5f; c.calibration.max_accel=3; c.calibration.max_decel=4;
  c.calibration.steer_rate=1; c.calibration.drive_per_accel=0.2f;
  c.calibration.brake_per_decel=0.1f; c.calibration.confirmed=true;
  assert(c.accept(cmd,0)==vehicle::Result::Accepted);
  assert(c.accept(cmd,0)==vehicle::Result::OldSequence);
  cmd.sequence=2; cmd.action=2; cmd.steering=0.3f; cmd.acceleration=2;
  assert(c.accept(cmd,0)==vehicle::Result::Accepted);
  auto out=c.tick(10000,true);
  assert(out.enabled && fabsf(out.steering-0.01f)<0.0001f && fabsf(out.effort-0.4f)<0.0001f);
  assert(!c.tick(100000,true).enabled);
  cmd.sequence=3; cmd.deadline=150000;
  assert(c.accept(cmd,100000)==vehicle::Result::Disarmed);
  cmd.action=1; cmd.steering=cmd.acceleration=0;
  assert(c.accept(cmd,100000)==vehicle::Result::Accepted);
  assert(!c.tick(110000,false).enabled);
  cmd.sequence=4; cmd.acceleration=std::numeric_limits<float>::quiet_NaN();
  assert(c.accept(cmd,110000)==vehicle::Result::Invalid);
  cmd.boot=41;
  assert(c.accept(cmd,110000)==vehicle::Result::WrongBoot);
}
