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
  uint64_t acquired_us=0;
  bool tracking=false;
};

class AngleTracker {
public:
  Snapshot state;

  void lose_continuity() {
    previous_start_us_=0;
    state.tracking=false;
    ++state.gaps;
  }
  void read_failed() {
    ++state.read_errors;
    lose_continuity();
  }
  void observe(int angle,uint64_t start,uint64_t end,uint64_t max_gap_us,int sign) {
    // The angle was latched somewhere during the two-register read. Bound its
    // distance from the previous sample by previous read start to current end.
    if(end<start || end-start>=max_gap_us) {
      lose_continuity();
      return;
    }
    const int delta=difference(angle,previous_angle_);
    state.tracking=previous_start_us_ && start>=previous_start_us_ &&
                   end-previous_start_us_<max_gap_us && delta!=-8192;
    if(state.tracking) state.count+=uint32_t(delta*sign);
    else ++state.gaps;
    state.acquired_us=end;
    previous_angle_=angle;
    previous_start_us_=start;
  }
private:
  uint64_t previous_start_us_=0;
  int previous_angle_=0;
};
}
