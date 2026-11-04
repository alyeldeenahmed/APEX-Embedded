#pragma once
#include "packet.h"
namespace control_wire {
constexpr unsigned SIZE=64;
inline uint32_t r32(const uint8_t* p) { return uint32_t(p[0])|uint32_t(p[1])<<8|uint32_t(p[2])<<16|uint32_t(p[3])<<24; }
inline uint64_t r64(const uint8_t* p) { return uint64_t(r32(p))|uint64_t(r32(p+4))<<32; }
inline void w64(uint8_t* p,uint64_t x) { telemetry::u32(p,uint32_t(x)); telemetry::u32(p+4,uint32_t(x>>32)); }
inline float rf(const uint8_t* p) { uint32_t u=r32(p); float x; memcpy(&x,&u,4); return x; }
enum Action:uint8_t { Install=1,Arm=2,Set=3,Cancel=4,Revoke=5 };
struct Request {
  uint8_t action;
  uint32_t boot,sequence,session,epoch,id,target;
  uint64_t created,deadline;
  float steering,acceleration;
};
inline bool decode(const uint8_t* p,Request& r) {
  if(p[0]!=0xa5||p[1]!=0x5a||p[2]!=0x10||p[3]!=1||p[5]||p[6]||p[7]) return false;
  if(telemetry::crc(p+2,60)!=(uint16_t(p[62])|uint16_t(p[63])<<8)) return false;
  for(unsigned i=56;i<62;++i) if(p[i]) return false;
  r.action=p[4]; r.boot=r32(p+8); r.sequence=r32(p+12); r.session=r32(p+16);
  r.epoch=r32(p+20); r.id=r32(p+24); r.target=r32(p+28);
  r.created=r64(p+32); r.deadline=r64(p+40); r.steering=rf(p+48); r.acceleration=rf(p+52);
  return true;
}
class Parser {
  uint8_t bytes_[SIZE]; unsigned size_=0;
  uint64_t last_=0;
public:
  uint32_t errors=0;
  bool feed(uint8_t byte,uint64_t now,Request& r) {
    if(size_ && now-last_>20000) { size_=0; ++errors; }
    last_=now; bytes_[size_++]=byte;
    while(size_ && (bytes_[0]!=0xa5 || (size_>1 && bytes_[1]!=0x5a))) {
      memmove(bytes_,bytes_+1,--size_);
    }
    if(size_<SIZE) return false;
    if(decode(bytes_,r)) { size_=0; return true; }
    ++errors; memmove(bytes_,bytes_+1,SIZE-1); size_=SIZE-1;
    return false;
  }
};
}
