#pragma once
#include <stdint.h>
#include <stddef.h>

// Sensor-only logic. Bus must implement read(reg, bytes, length), write(reg, value),
// and wait_ms(ms). Arduino wiring is isolated in mpu6050_esp32.h.
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
template<class Bus> class Mpu6050 {
 public:
  explicit Mpu6050(Bus& bus): bus_(bus) {}
  bool begin() {
    ready_=false; last_=Reading{};
    if (!bus_.write(0x6B,0x80)) return false;
    bus_.wait_ms(100);
    uint8_t id=0;
    if (!bus_.read(0x75,&id,1) || id!=0x68) return false;
    if (!bus_.write(0x6B,0x01)) return false; // PLL X gyro, awake
    bus_.wait_ms(100);
    // Disable FIFO/DMP; all axes on; 1 kHz, gyro 42 Hz / accel 44 Hz DLPF;
    // gyro +/-1000 deg/s, accel +/-4 g; active-high pulsed data-ready.
    const uint8_t settings[][2]={{0x6C,0},{0x6A,0},{0x23,0},{0x19,0},
      {0x1A,3},{0x1B,0x10},{0x1C,0x08},{0x37,0},{0x38,1}};
    for (const auto& s:settings) {
      uint8_t actual=0;
      if (!bus_.write(s[0],s[1]) || !bus_.read(s[0],&actual,1) || actual!=s[1]) return false;
    }
    uint8_t power=0;
    if (!bus_.read(0x6B,&power,1) || power!=1) return false;
    ready_=true; return true;
  }
  bool capture(uint64_t stamp) {
    if (!ready_) return false;
    uint8_t bytes[14];
    if (!bus_.read(0x3B,bytes,sizeof(bytes))) { invalidate(); return false; }
    last_=convert(bytes,stamp); return true;
  }
  void invalidate() { last_.valid=false; }
  bool ready() const { return ready_; }
  const Reading& reading() const { return last_; }
 private:
  Bus& bus_; bool ready_=false; Reading last_;
};
}
