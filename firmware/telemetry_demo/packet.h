#pragma once
#include <stdint.h>
#include <string.h>

namespace telemetry {
static_assert(sizeof(float) == 4, "Protocol requires 32-bit floats");
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
  // Provisional CRC coverage; confirm with Perception.
  u16(p + 12, crc(p + 2, 10));
}
}
