#include <cassert>
#include <initializer_list>
#include "firmware/telemetry_demo/mt6701_core.h"

// Independent polynomial long division, x^6 + x + 1, initial remainder zero.
static uint32_t frame(unsigned angle, unsigned status) {
  uint32_t bits=(angle<<10)|(status<<6), remainder=bits;
  for(int i=23;i>=6;--i)
    if(remainder & (uint32_t(1)<<i)) remainder ^= uint32_t(0x43)<<(i-6);
  return bits|remainder;
}
int main() {
  for(unsigned a=0;a<16384;++a) {
    int decoded=-1;
    assert(mt6701::unpack(frame(a,0),decoded) && decoded==int(a));
    assert(mt6701::unpack(frame(a,4),decoded)); // push flag alone is permitted
    for(unsigned fault: {1u,2u,3u,8u})
      assert(!mt6701::unpack(frame(a,fault),decoded));
    for(unsigned bit=0;bit<24;++bit)
      assert(!mt6701::unpack(frame(a,0)^(uint32_t(1)<<bit),decoded));
  }
}
