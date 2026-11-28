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

struct Snapshot {
  uint32_t count=0, gaps=0, read_errors=0;
  uint32_t timing_gaps=0, ambiguous_gaps=0;
  int32_t segment_delta=0;
  uint64_t segment_start_us=0, segment_end_us=0;
  uint64_t acquired_us=0;
  bool tracking=false;
};

class AngleTracker {
public:
  Snapshot state;

  void lose_continuity() {
    previous_start_us_=0;
    segment_anchor_us_=0;
    state.tracking=false;
    ++state.gaps;
  }
  void read_failed() {
    ++state.read_errors;
    // No new angle. The next success can still prove the entire displacement
    // if both successful reads together satisfy the original half-turn bound.
    state.tracking=false;
  }
  void observe(int angle,uint64_t start,uint64_t end,uint64_t max_gap_us,int sign) {
    // The angle was latched somewhere during the two-register read. Bound its
    // distance from the previous sample by previous read start to current end.
    if(end<start || end-start>=max_gap_us) {
      ++state.timing_gaps;
      lose_continuity();
      return;
    }
    const int delta=difference(angle,previous_angle_);
    const bool bounded=previous_start_us_ && start>=previous_start_us_ &&
                       end-previous_start_us_<max_gap_us;
    state.tracking=bounded && delta!=-8192;
    if(state.tracking) {
      const int signed_delta=delta*sign;
      state.count+=uint32_t(signed_delta);
      segment_sum_+=signed_delta;
      if(end-segment_anchor_us_>=2000) {
        state.segment_start_us=segment_anchor_us_;
        state.segment_end_us=end;
        state.segment_delta=segment_sum_;
        segment_anchor_us_=end;
        segment_sum_=0;
      }
    } else {
      if(previous_start_us_ && !bounded) ++state.timing_gaps;
      if(bounded && delta==-8192) ++state.ambiguous_gaps;
      ++state.gaps;
      segment_anchor_us_=end;
      segment_sum_=0;
    }
    state.acquired_us=end;
    previous_angle_=angle;
    previous_start_us_=start;
  }
private:
  uint64_t previous_start_us_=0;
  uint64_t segment_anchor_us_=0;
  int32_t segment_sum_=0;
  int previous_angle_=0;
};
}
