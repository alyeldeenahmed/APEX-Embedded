#include "firmware/telemetry_demo/vehicle_core.h"
#include "firmware/telemetry_demo/encoder_math.h"
#include "firmware/telemetry_demo/packet.h"
#include "firmware/telemetry_demo/wheel_health.h"
#include <cassert>
#include <limits>
int main() {
  using wheel_health::magnitude_valid;
  assert(!wheel_health::control_direction_allowed(false,true,false,.2f,100000));
  assert(!wheel_health::control_direction_allowed(false,true,true,0,0));
  assert(!wheel_health::control_direction_allowed(false,false,true,.2f,100000));
  assert(wheel_health::control_direction_allowed(false,true,true,.2f,100000));
  assert(!wheel_health::control_direction_allowed(true,false,false,0,0));
  assert(wheel_health::control_direction_allowed(true,false,false,.2f,100000));
  assert(magnitude_valid(true,20000,10000,true,true,.001f,false));
  assert(!magnitude_valid(false,20000,10000,true,true,.001f,false));
  assert(!magnitude_valid(true,35001,10000,true,true,.001f,false));
  assert(!magnitude_valid(true,20000,10000,false,true,.001f,false));
  assert(!magnitude_valid(true,20000,10000,true,false,.001f,false));
  assert(!magnitude_valid(true,20000,10000,true,true,0,false));
  assert(!magnitude_valid(true,20000,10000,true,true,.001f,true));
  wheel_health::MotionMonitor m;
  m.update(0,true,.5f,false,.2f,100000);
  m.update(99999,true,.5f,false,.2f,100000); assert(!m.suspect);
  m.update(100000,true,.5f,false,.2f,100000); assert(m.suspect);
  m.update(110000,false,0,true,.2f,100000); assert(m.suspect);
  wheel_health::MotionMonitor moving;
  moving.update(0,true,.5f,false,.2f,100000);
  moving.update(90000,true,.5f,true,.2f,100000);
  moving.update(180000,true,.5f,false,.2f,100000); assert(!moving.suspect);
  moving.update(190000,true,.5f,false,.2f,100000); assert(moving.suspect);
  wheel_health::MotionMonitor idle;
  idle.update(0,false,.5f,false,.2f,100000);
  idle.update(200000,false,.5f,false,.2f,100000); assert(!idle.suspect);
  idle.update(300000,true,.1f,false,.2f,100000); assert(!idle.tracking);
  idle.update(400000,true,-.5f,false,.2f,100000); assert(!idle.tracking);
  idle.update(500000,true,.5f,false,0,0); assert(!idle.tracking);
  uint8_t timing[42];
  telemetry::encode_timing(timing,1,2,100,0,90,0,6,true);
  assert(timing[3]==2 && timing[7]==6);
  assert(telemetry::crc(timing+2,38)==(uint16_t(timing[40])|uint16_t(timing[41])<<8));
  telemetry::encode_timing(timing,65535,3,0x100000020ULL,0xfffffff0ULL,0x100000010ULL,0);
  assert(timing[2]==3 && timing[4]==255 && timing[5]==255 && timing[6]==3);
  assert(timing[8]==32 && timing[12]==1 && timing[16]==240 && timing[20]==0);
  assert(timing[24]==16 && timing[28]==1);
  assert(telemetry::crc(timing+2,38)==(uint16_t(timing[40])|uint16_t(timing[41])<<8));
  const uint8_t cycle[]={0,1,3,2,0};
  for(unsigned i=0;i<4;++i) {
    assert(encoder_math::transition(cycle[i],cycle[i+1])==1);
    assert(encoder_math::transition(cycle[i+1],cycle[i])==-1);
    assert(encoder_math::transition(cycle[i],cycle[i])==0);
  }
  assert(encoder_math::transition(0,3)==2);
  assert(encoder_math::transition(1,2)==2);
  assert(encoder_math::difference(1,0xffffffffu)==2);
  assert(encoder_math::difference(0xffffffffu,1)==-2);
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
