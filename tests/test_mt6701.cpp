#include <cassert>
#include "firmware/telemetry_demo/mt6701_core.h"
struct Bus {
  int calls=0,fail=0; uint8_t high=0xab,low=0xcc;
  bool read(uint8_t reg,uint8_t& value) {
    ++calls; assert(reg==(calls==1?0x03:0x04));
    if(calls==fail) return false;
    value=calls==1?high:low; return true;
  }
};
int main() {
  for(int angle=0;angle<16384;++angle)
    for(int reserved=0;reserved<4;++reserved)
      assert(mt6701::decode(uint8_t(angle>>6),uint8_t((angle<<2)|reserved))==angle);
  Bus b; int angle=-1;
  assert(mt6701::read_angle(b,angle) && b.calls==2 && angle==10995);
  for(int failure=1;failure<=2;++failure) {
    Bus bad; bad.fail=failure; angle=-1;
    assert(!mt6701::read_angle(bad,angle) && angle==-1 && bad.calls==failure);
  }
  assert(mt6701::difference(0,16383)==1);
  assert(mt6701::difference(16383,0)==-1);
  assert(mt6701::difference(8192,0)==-8192); // ambiguous half-turn, driver rejects
  assert(mt6701::difference(5,5)==0);
  mt6701::AngleTracker tracker;
  tracker.observe(2, 100, 250, 1000, -1);
  assert(!tracker.state.tracking);
  tracker.observe(16383, 500, 650, 1000, -1);
  assert(tracker.state.tracking && tracker.state.count==3 && tracker.state.acquired_us==650);
  tracker.read_failed();
  assert(!tracker.state.tracking && tracker.state.read_errors==1);
  tracker.observe(16381, 950, 1050, 1000, -1);
  assert(tracker.state.tracking && tracker.state.count==5 && !tracker.state.timing_gaps);
  tracker.observe(16380, 1400, 1550, 1000, -1);
  assert(tracker.state.tracking && tracker.state.count==6);
  tracker.observe(4900, 2800, 2960, 1000, -1);
  assert(!tracker.state.tracking && tracker.state.count==6);
  assert(tracker.state.timing_gaps==1);
  tracker.observe((4900+8192)%16384, 3200, 3350, 1000, -1);
  assert(!tracker.state.tracking && tracker.state.count==6);
  assert(tracker.state.ambiguous_gaps==1);
  mt6701::AngleTracker segments;
  for (int i=0;i<=5;++i) segments.observe(16383-i*10,100+i*400,200+i*400,1000,-1);
  assert(segments.state.segment_start_us==200 && segments.state.segment_end_us==2200);
  assert(segments.state.segment_delta==50);
  segments.read_failed();
  segments.observe(120,4100,4200,1000,-1);
  assert(!segments.state.tracking && segments.state.count==50 && segments.state.timing_gaps==1);
  for(int i=1;i<=5;++i) segments.observe(120,4100+i*400,4200+i*400,1000,-1);
  assert(segments.state.segment_delta==0 && segments.state.segment_start_us==4200);
  for(int i=1;i<=5;++i) segments.observe(120+2*i,6100+i*400,6200+i*400,1000,-1);
  assert(segments.state.segment_delta==-10 && segments.state.segment_end_us==8200);
}
