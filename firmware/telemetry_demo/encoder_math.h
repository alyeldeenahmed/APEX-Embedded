#pragma once
#include <stdint.h>
namespace encoder_math {
// 2 denotes an impossible two-bit transition, not two legitimate counts.
inline int transition(uint8_t before,uint8_t after) {
  const uint8_t changed=(before^after)&3;
  if(changed==3) return 2;
  if(!changed) return 0;
  return ((before>>1)^(after&1))?1:-1;
}
inline int64_t difference(uint32_t after,uint32_t before) {
  const uint32_t delta=after-before;
  return delta<=0x7fffffffu?int64_t(delta):int64_t(delta)-0x100000000LL;
}
}
