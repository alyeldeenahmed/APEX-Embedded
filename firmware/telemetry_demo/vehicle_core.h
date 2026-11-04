#pragma once
#include <stdint.h>
#include <math.h>

// Proposed vehicle logic. No hardware output is performed by this header.
namespace vehicle {
inline float clamp(float x,float lo,float hi) { return x<lo?lo:(x>hi?hi:x); }
struct Wheel {
  uint32_t previous=0;
  uint64_t stamp=0;
  float speed=0;
  bool initialized=false, valid=false;
  // metres_per_count must be measured. Single-channel counts are UNSIGNED speed.
  void update(uint32_t count,uint64_t now,float metres_per_count,bool healthy) {
    valid=false;
    if(initialized && now>stamp && isfinite(metres_per_count) && metres_per_count>0 && healthy) {
      speed=float(uint32_t(count-previous))*metres_per_count*1000000.0f/float(now-stamp);
      valid=isfinite(speed);
    }
    previous=count; stamp=now; initialized=true;
  }
};
struct Calibration {
  // Zero defaults deliberately prevent arming. Populate from measured results.
  float max_angle=0, max_accel=0, max_decel=0, steer_rate=0;
  float drive_per_accel=0, brake_per_decel=0;
  bool confirmed=false;
  bool ready() const {
    return confirmed && isfinite(max_angle) && max_angle>0 &&
      isfinite(max_accel) && max_accel>0 && isfinite(max_decel) && max_decel>0 &&
      isfinite(steer_rate) && steer_rate>0 && isfinite(drive_per_accel) &&
      drive_per_accel>0 && isfinite(brake_per_decel) && brake_per_decel>0;
  }
};
enum class Result:uint8_t { Accepted, Disarmed, WrongBoot, OldSequence, Invalid, Uncalibrated, Expired };
struct Command {
  uint32_t boot, sequence;
  uint64_t deadline; // ESP32 boot-relative microseconds, exclusive expiration.
  float steering, acceleration;
  uint8_t action; // 0 disarm/cancel, 1 arm at neutral, 2 setpoint
};
struct Output { float steering=0, effort=0; bool enabled=false; };
class Controller {
  uint32_t boot_,last_sequence_=0;
  uint64_t deadline_=0,last_tick_=0;
  bool seen_=false,armed_=false;
  float requested_steer_=0,requested_accel_=0;
  Output output_;
public:
  Calibration calibration;
  explicit Controller(uint32_t boot):boot_(boot) {}
  void stop() { armed_=false; deadline_=0; requested_steer_=requested_accel_=0; output_=Output{}; }
  Result accept(const Command& c,uint64_t now) {
    if(c.boot!=boot_) return Result::WrongBoot;
    if(seen_ && (uint32_t(c.sequence-last_sequence_)==0 || uint32_t(c.sequence-last_sequence_)>=0x80000000u)) return Result::OldSequence;
    if(c.action>2 || !isfinite(c.steering) || !isfinite(c.acceleration)) return Result::Invalid;
    if(c.action==0) { last_sequence_=c.sequence; seen_=true; stop(); return Result::Accepted; }
    if(c.deadline<=now || c.deadline-now>100000) return Result::Expired;
    if(!calibration.ready()) return Result::Uncalibrated;
    if(fabsf(c.steering)>calibration.max_angle || c.acceleration>calibration.max_accel || c.acceleration < -calibration.max_decel) return Result::Invalid;
    if(c.action==1 && (c.steering!=0 || c.acceleration!=0)) return Result::Invalid;
    if(c.action==2 && (!armed_ || now>=deadline_)) { stop(); return Result::Disarmed; }
    seen_=true; last_sequence_=c.sequence; deadline_=c.deadline;
    if(c.action==1) { stop(); armed_=true; deadline_=c.deadline; last_tick_=now; }
    requested_steer_=c.steering; requested_accel_=c.acceleration;
    return Result::Accepted;
  }
  Output tick(uint64_t now,bool feedback_healthy) {
    if(!armed_ || now>=deadline_ || now<last_tick_ || !feedback_healthy || !calibration.ready()) { stop(); return output_; }
    const float dt=float(now-last_tick_)*1e-6f;
    last_tick_=now;
    output_.steering+=clamp(requested_steer_-output_.steering,-calibration.steer_rate*dt,calibration.steer_rate*dt);
    // Initial feedforward proposal: effort is normalized drive/brake demand.
    // It is NOT measured acceleration or an ESC pulse. Negative effort is brake,
    // never reverse. Hardware adapter must implement the verified ESC semantics.
    output_.effort=clamp(requested_accel_*(requested_accel_>=0?calibration.drive_per_accel:calibration.brake_per_decel),-1,1);
    output_.enabled=true;
    return output_;
  }
};
}
