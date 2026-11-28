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
struct Receipt {
  uint32_t boot;
  uint16_t sequence;
  uint64_t report_us;
};
constexpr unsigned RECEIPT_SIZE=20;
inline bool decode_receipt(const uint8_t* p,Receipt& r) {
  if(p[0]!=0xa5 || p[1]!=0x5a || p[2]!=0x12 || p[3]!=1 ||
     telemetry::crc(p+2,16)!=(uint16_t(p[18]) | uint16_t(p[19])<<8)) return false;
  r.boot=r32(p+4);
  r.sequence=uint16_t(p[8]) | uint16_t(p[9])<<8;
  r.report_us=r64(p+10);
  return true;
}
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
  void consume(unsigned count) {
    size_-=count;
    memmove(bytes_,bytes_+count,size_);
  }
public:
  enum class Kind { None, Command, Ack, SpeedSubscribe };
  uint32_t errors=0;
  Kind feed(uint8_t byte,uint64_t now,Request& r,Receipt& ack) {
    if(size_ && now-last_>20000) { size_=0; ++errors; }
    last_=now; bytes_[size_++]=byte;
    while(size_) {
      if(bytes_[0]!=0xa5 || (size_>1 && bytes_[1]!=0x5a)) { consume(1); continue; }
      if(size_<3) break;
      const bool command=bytes_[2]==0x10;
      if(bytes_[2]==0x13) {
        if(size_<6) break;
        if(bytes_[3]==1 && telemetry::crc(bytes_+2,2)==
           (uint16_t(bytes_[4]) | uint16_t(bytes_[5])<<8)) {
          consume(6); return Kind::SpeedSubscribe;
        }
        ++errors; consume(1); continue;
      }
      if(!command && bytes_[2]!=0x12) { ++errors; consume(1); continue; }
      const unsigned length=command?SIZE:RECEIPT_SIZE;
      if(size_<length) break;
      if(command ? decode(bytes_,r) : decode_receipt(bytes_,ack)) {
        consume(length);
        return command?Kind::Command:Kind::Ack;
      }
      ++errors;
      consume(1);
    }
    return Kind::None;
  }
  bool feed(uint8_t byte,uint64_t now,Request& r) {
    Receipt ack{};
    return feed(byte,now,r,ack)==Kind::Command;
  }
};
}
