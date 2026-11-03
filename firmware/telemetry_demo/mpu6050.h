#pragma once
#include <stdint.h>
#include <stddef.h>

// Sensor-only logic. Bus must implement read(reg, bytes, length), write(reg, value),
// and recover(). Arduino wiring is isolated in mpu6050_esp32.h.
namespace imu {
constexpr float kGravity = 9.80665f;
constexpr float kRadiansPerDegree = 0.017453292519943295f;
constexpr uint64_t kFreshnessUs = 5000;  // Bench proposal, not an agreed timeout.
struct Reading {
  float accel[3] = {}, gyro[3] = {};
  uint64_t timestamp = 0;
  bool present = false, valid = false, clipped = false;
};
inline int32_t signed_word(const uint8_t* p) {
  const uint32_t v = (uint32_t(p[0]) << 8) | p[1];
  return v >= 32768 ? int32_t(v) - 65536 : int32_t(v);
}
inline Reading convert(const uint8_t* p, uint64_t stamp) {
  Reading r;
  r.timestamp = stamp; r.present = true; r.valid = true;
  for (unsigned i = 0; i < 3; ++i) {
    const int32_t a = signed_word(p + 2*i), g = signed_word(p + 8 + 2*i);
    r.accel[i] = float(a) * (kGravity / 8192.0f);
    r.gyro[i] = float(g) * (kRadiansPerDegree / 32.8f);
    if (a == -32768 || a == 32767 || g == -32768 || g == 32767) r.clipped = true;
  }
  r.valid = !r.clipped;
  return r;
}
inline bool fresh(const Reading& r, uint64_t now) {
  return r.present && r.valid && now >= r.timestamp && now-r.timestamp <= kFreshnessUs;
}
// Signed permutation of sensor axes into vehicle axes. Must be a proper rotation.
struct Axes { int8_t source[3]; int8_t sign[3]; };
inline bool proper_rotation(const Axes& m) {
  int inversions = 0, product = 1;
  for (int i=0;i<3;++i) {
    if (m.source[i]<0 || m.source[i]>2 || (m.sign[i]!=1 && m.sign[i]!=-1)) return false;
    product *= m.sign[i];
    for (int j=0;j<i;++j) {
      if (m.source[i]==m.source[j]) return false;
      if (m.source[j]>m.source[i]) ++inversions;
    }
  }
  return product * ((inversions%2) ? -1 : 1) == 1;
}
inline Reading rotate(const Reading& r, const Axes& m) {
  Reading out=r;
  if (!proper_rotation(m)) { out.valid=false; return out; }
  for (int i=0;i<3;++i) {
    out.accel[i]=m.sign[i]*r.accel[m.source[i]];
    out.gyro[i]=m.sign[i]*r.gyro[m.source[i]];
  }
  return out;
}
enum class State : uint8_t { Retry=0, ResetWait=1, WakeWait=2, Configure=3, Ready=4 };
enum class Fault : uint8_t { None=0, Bus=1, Identity=2, Configuration=3, NoData=4 };
struct Diagnostics {
  uint32_t attempts=0, recoveries=0, read_errors=0, config_errors=0, no_data=0;
  uint32_t samples=0, clipped=0;
};
inline uint8_t config_reg(unsigned i) {
  const uint8_t regs[]={0x6C,0x6A,0x23,0x19,0x1A,0x1B,0x1C,0x37,0x38,0x6B};
  return regs[i];
}
inline uint8_t config_value(unsigned i) {
  const uint8_t values[]={0,0,0,0,3,0x10,0x08,0,1,1};
  return values[i];
}
template<class Bus> class Mpu6050 {
 public:
  explicit Mpu6050(Bus& bus): bus_(bus) {}
  void start(uint64_t now) { state_=State::Retry; due_=now; invalidate(); }
  // No delay calls: one bounded initialization step or audit per invocation.
  void tick(uint64_t now) {
    if(state_==State::Ready) {
      if(now-last_read_>100000) { ++diag.no_data; fail(now,Fault::NoData); return; }
      if(now>=audit_due_) {
        uint8_t value=0;
        if(!bus_.read(config_reg(audit_),&value,1)) { fail(now,Fault::Bus); return; }
        if(value!=config_value(audit_)) { ++diag.config_errors; fail(now,Fault::Configuration); return; }
        audit_=(audit_+1)%10; audit_due_=now+100000;
      }
      return;
    }
    if(now<due_) return;
    if(state_==State::Retry) {
      ++diag.attempts;
      if(!bus_.recover() || !bus_.write(0x6B,0x80)) { fail(now,Fault::Bus); return; }
      state_=State::ResetWait; due_=now+100000; return;
    }
    if(state_==State::ResetWait) {
      uint8_t id=0;
      if(!bus_.read(0x75,&id,1)) { fail(now,Fault::Bus); return; }
      if(id!=0x68) { fail(now,Fault::Identity); return; }
      if(!bus_.write(0x6B,1)) { fail(now,Fault::Bus); return; }
      state_=State::WakeWait; due_=now+100000; return;
    }
    if(state_==State::WakeWait) { state_=State::Configure; config_=0; }
    uint8_t value=0;
    if(!bus_.write(config_reg(config_),config_value(config_)) ||
       !bus_.read(config_reg(config_),&value,1)) { fail(now,Fault::Bus); return; }
    if(value!=config_value(config_)) { ++diag.config_errors; fail(now,Fault::Configuration); return; }
    if(++config_==10) {
      state_=State::Ready; fault_=Fault::None; last_read_=now; audit_due_=now+100000;
      audit_=0; failures_=0; if(generation_++) ++diag.recoveries;
    }
  }
  bool capture(uint64_t stamp) {
    if (!ready()) return false;
    uint8_t bytes[14];
    if (!bus_.read(0x3B,bytes,sizeof(bytes))) {
      ++diag.read_errors; invalidate();
      if(++failures_>=3) fail(stamp,Fault::Bus);
      return false;
    }
    failures_=0; last_read_=stamp; ++diag.samples;
    last_=convert(bytes,stamp); if(last_.clipped) ++diag.clipped;
    return true;
  }
  void invalidate() { last_.valid=false; }
  bool ready() const { return state_==State::Ready; }
  State state() const { return state_; }
  Fault fault() const { return fault_; }
  uint32_t generation() const { return generation_; }
  const Reading& reading() const { return last_; }
  Diagnostics diag;
 private:
  void fail(uint64_t now,Fault why) {
    invalidate(); state_=State::Retry; fault_=why; due_=now+1000000;
  }
  Bus& bus_; Reading last_; State state_=State::Retry; Fault fault_=Fault::None;
  uint64_t due_=0,last_read_=0,audit_due_=0;
  unsigned config_=0,audit_=0,failures_=0; uint32_t generation_=0;
};
}
