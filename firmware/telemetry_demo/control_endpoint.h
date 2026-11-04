#pragma once
#include "vehicle_core.h"
#include "command_protocol.h"
namespace endpoint {
enum Code:uint8_t { Idle,Installed,Accepted,Applied,Simulated,Cancelled,Revoked,
  BadBoot,Replay,BadContext,BadTime,BadValue,NotReady,NotArmed,UnknownTarget,Fault,Expired };
// No future queue: latest accepted setpoint takes effect on the next service tick.
class Engine {
  vehicle::Controller core_;
  uint32_t boot_,session_=0,epoch_=0,sequence_=0,pending_=0,held_=0;
  bool seen_=false,installed_=false,armed_=false,pending_valid_=false;
  uint64_t deadline_=0;
public:
  Code code=Idle;
  uint32_t response_id=0;
  uint64_t applied_at=0;
  vehicle::Output output;
  explicit Engine(uint32_t boot):core_(boot),boot_(boot) {}
  vehicle::Calibration& calibration() { return core_.calibration; }
  uint32_t boot() const { return boot_; }
  uint32_t session() const { return session_; }
  uint32_t epoch() const { return epoch_; }
  uint32_t sequence() const { return sequence_; }
  uint32_t held() const { return held_; }
  bool armed() const { return armed_; }
  void stop() { core_.stop(); armed_=false; pending_valid_=false; output=vehicle::Output{}; }
  Code receive(const control_wire::Request& r,uint64_t now,bool ready) {
    response_id=r.id;
    if(r.boot!=boot_) return code=BadBoot;
    if(seen_ && (uint32_t(r.sequence-sequence_)==0 || uint32_t(r.sequence-sequence_)>=0x80000000u)) return code=Replay;
    // CRC is corruption detection, not authentication. Consume matching-boot
    // request sequences even on rejection; the transport assumes a trusted Pi.
    sequence_=r.sequence; seen_=true;
    if(r.action<1||r.action>5||!r.id||!isfinite(r.steering)||!isfinite(r.acceleration)) return code=BadValue;
    if(r.created>now||now-r.created>100000||r.deadline<=now||r.deadline-now>100000||r.deadline<=r.created) return code=BadTime;
    if(r.action!=control_wire::Set && (r.steering!=0||r.acceleration!=0)) return code=BadValue;
    if(r.action==control_wire::Install) {
      if(!r.session||!r.epoch) return code=BadContext;
      if(installed_ && r.session==session_ && r.epoch<=epoch_) return code=BadContext;
      stop(); held_=0; session_=r.session; epoch_=r.epoch; installed_=true;
      return code=Installed;
    }
    if(!installed_||r.session!=session_||r.epoch!=epoch_) return code=BadContext;
    if(r.action==control_wire::Revoke) { stop(); installed_=false; return code=Revoked; }
    if(r.action==control_wire::Cancel) {
      if((pending_valid_&&r.target==pending_) || (armed_&&r.target==held_)) {
        const bool was_issued=r.target==held_ && held_!=0;
        stop(); return code=was_issued?Revoked:Cancelled;
      }
      return code=UnknownTarget;
    }
    if(!ready||!core_.calibration.ready()) { stop(); return code=NotReady; }
    if(r.action==control_wire::Arm && armed_) return code=BadValue;
    vehicle::Command c{boot_,r.sequence,r.deadline,r.steering,r.acceleration,
      uint8_t(r.action==control_wire::Arm?1:2)};
    auto result=core_.accept(c,now);
    if(result!=vehicle::Result::Accepted) return code=result==vehicle::Result::Disarmed?NotArmed:BadValue;
    armed_=true; deadline_=r.deadline; pending_=r.id; pending_valid_=true;
    return code=Accepted;
  }
  template<class Driver> bool tick(uint64_t now,bool healthy,Driver& driver) {
    if(armed_ && (now>=deadline_ || !healthy)) {
      stop(); driver.write(output); code=now>=deadline_?Expired:Fault; return true;
    }
    if(!armed_) { if(!driver.write(output)) { code=Fault;return true; } return false; }
    output=core_.tick(now,healthy);
    if(!output.enabled || !driver.write(output)) { stop(); driver.write(output); code=Fault; return true; }
    if(pending_valid_) { held_=pending_; pending_valid_=false; applied_at=now;
      response_id=held_; code=driver.physical()?Applied:Simulated; return true; }
    return false;
  }
  void status(uint8_t* p,uint64_t now,uint32_t parser_errors,uint32_t tx_drops,bool physical) const {
    memset(p,0,64); p[0]=0xa5;p[1]=0x5a;p[2]=0x11;p[3]=1;p[4]=code;
    p[5]=(installed_?1:0)|(armed_?2:0)|(physical?4:0)|(output.enabled?8:0);
    telemetry::u32(p+8,boot_);telemetry::u32(p+12,sequence_);telemetry::u32(p+16,session_);
    telemetry::u32(p+20,epoch_);telemetry::u32(p+24,response_id);telemetry::u32(p+28,held_);
    control_wire::w64(p+32,now);control_wire::w64(p+40,applied_at);
    telemetry::f32(p+48,output.steering);telemetry::f32(p+52,output.effort);
    telemetry::u16(p+56,uint16_t(parser_errors>65535?65535:parser_errors));
    telemetry::u32(p+58,tx_drops);telemetry::u16(p+62,telemetry::crc(p+2,60));
  }
};
}
