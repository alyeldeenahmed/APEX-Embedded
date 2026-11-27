#pragma once
#include "command_protocol.h"

namespace endpoint {
// MCU loop owns this window. Only complete 01 + 03 pairs accepted by the UART
// belong here. A receipt cumulatively acknowledges entries through its match.
class ReceiptWatchdog {
public:
  static constexpr uint8_t limit=4;
private:
  struct Pair { uint16_t sequence; uint64_t report_us; };
  Pair pending_[limit]{};
  uint8_t count_=0;
  uint32_t boot_=0;
  uint64_t last_queue_us_=0, max_age_us_=0, recovery_floor_=0;
  bool ready_=false, lost_=false;

  bool trip(uint64_t floor) {
    if(lost_) return false;
    ready_=false; lost_=true; recovery_floor_=floor;
    return true;
  }
public:
  void begin(uint32_t boot,uint64_t now,uint64_t period_us) {
    boot_=boot; last_queue_us_=now; max_age_us_=limit*period_us;
    count_=0; ready_=false; lost_=false; recovery_floor_=0;
  }
  bool ready() const { return ready_; }
  bool lost() const { return lost_; }
  uint8_t pending() const { return count_; }
  // Returns true exactly once per loss transition.
  bool queued(uint16_t sequence,uint64_t report_us,uint64_t now) {
    last_queue_us_=now;
    if(count_==limit) {
      for(unsigned i=1;i<limit;++i) pending_[i-1]=pending_[i];
      --count_;
    }
    pending_[count_++]={sequence,report_us};
    return count_==limit && trip(report_us);
  }
  bool check_stall(uint64_t now) {
    return now-last_queue_us_>=max_age_us_ && trip(now);
  }
  bool acknowledge(const control_wire::Receipt& ack,uint64_t now) {
    if(ack.boot!=boot_ || ack.report_us>now || now-ack.report_us>=max_age_us_ ||
       (lost_ && ack.report_us<recovery_floor_)) return false;
    for(unsigned i=0;i<count_;++i) {
      if(pending_[i].sequence!=ack.sequence || pending_[i].report_us!=ack.report_us) continue;
      const unsigned consumed=i+1;
      for(unsigned j=consumed;j<count_;++j) pending_[j-consumed]=pending_[j];
      count_=uint8_t(count_-consumed);
      ready_=true; lost_=false;
      return true;
    }
    return false;
  }
};
}
