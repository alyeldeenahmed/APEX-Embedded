#pragma once
#include <stdint.h>
namespace mt6701 {
inline bool unpack(uint32_t frame, int& angle) {
  unsigned crc=0;
  for (int bit=23; bit>=6; --bit) {
    unsigned feedback=((frame >> bit) ^ (crc >> 5)) & 1;
    crc=((crc << 1) & 63) ^ (feedback ? 3 : 0);
  }
  if (crc!=(frame & 63) || ((frame >> 6) & 11)!=0) return false;
  angle=int(frame >> 10);
  return true;
}
}
