#pragma once
#include <stdint.h>
#include <string.h>
#include <limits>

namespace telemetry {
static_assert(sizeof(float) == 4, "Protocol requires 32-bit floats");
static_assert(std::numeric_limits<float>::is_iec559, "Protocol requires IEEE-754 floats");
inline void u16(uint8_t* p, uint16_t v) { p[0] = v; p[1] = v >> 8; }
inline void u32(uint8_t* p, uint32_t v) {
  for (unsigned i = 0; i < 4; ++i) p[i] = v >> (8 * i);
}
inline void f32(uint8_t* p, float v) {
  uint32_t bits; memcpy(&bits, &v, 4); u32(p, bits);
}
inline uint16_t crc(const uint8_t* p, unsigned n) {
  uint16_t c = 0xffff;
  while (n--) {
    c ^= uint16_t(*p++) << 8;
    for (unsigned i = 0; i < 8; ++i)
      c = (c & 0x8000) ? (c << 1) ^ 0x1021 : c << 1;
  }
  return c;
}
struct Sample {
  uint32_t timestamp;
  float gyro[3], accel[3];
  int32_t ticks;
  float speed, steering;
  uint16_t wheel_age, steer_age;
  uint8_t flags;
};
// Companion to type 01. Independent acquisition times are essential when the
// IMU is held/invalid but wheel samples continue. This is an interface extension.
inline void encode_timing(uint8_t* p,uint16_t seq,uint8_t present,uint64_t report,
                          uint64_t imu,uint64_t wheel,uint64_t steering,uint8_t diagnostics=0,bool v2=false) {
  memset(p,0,42);p[0]=0xa5;p[1]=0x5a;p[2]=3;p[3]=1;
  u16(p+4,seq);p[6]=present&7;
  if(v2) { p[3]=2;p[7]=diagnostics&7; }
  const uint64_t stamps[]={report,imu,wheel,steering};
  for(unsigned j=0;j<4;++j) for(unsigned i=0;i<8;++i) p[8+8*j+i]=uint8_t(stamps[j]>>(8*i));
  u16(p+40,crc(p+2,38));
}
inline void encode(uint8_t* p, uint16_t seq, const Sample& s) {
  memset(p, 0, 54);
  p[0] = 0xa5; p[1] = 0x5a; p[2] = 1; p[3] = 1;
  p[4] = s.flags & 0x7f;
  u16(p + 6, seq); u32(p + 8, s.timestamp);
  for (unsigned i = 0; i < 3; ++i) {
    f32(p + 12 + 4*i, s.gyro[i]); f32(p + 24 + 4*i, s.accel[i]);
  }
  u32(p + 36, uint32_t(s.ticks)); f32(p + 40, s.speed);
  u16(p + 44, s.wheel_age); f32(p + 46, s.steering);
  u16(p + 50, s.steer_age); u16(p + 52, crc(p + 2, 50));
}
inline void encode_sync(uint8_t* p, uint64_t timestamp) {
  p[0] = 0xa5; p[1] = 0x5a; p[2] = 2; p[3] = 1;
  for (unsigned i = 0; i < 8; ++i) p[4+i] = timestamp >> (8*i);
  // Perception confirmed CRC coverage: bytes 2-11.
  u16(p + 12, crc(p + 2, 10));
}
}
