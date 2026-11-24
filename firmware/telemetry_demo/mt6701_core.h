#pragma once
#include <stdint.h>
namespace mt6701 {
// Datasheet Rev 1.8 section 7.7.2: separate reads, high register first.
// No SSI CRC or magnetic status is available through these angle registers.
inline int decode(uint8_t high, uint8_t low) {
  return (int(high)<<6) | (low>>2);
}
template<class Bus> bool read_angle(Bus& bus, int& angle) {
  uint8_t high,low;
  if(!bus.read(0x03,high) || !bus.read(0x04,low)) return false;
  angle=decode(high,low); return true;
}
inline int difference(int current,int previous) {
  return (current-previous+24576)%16384-8192;
}
}
