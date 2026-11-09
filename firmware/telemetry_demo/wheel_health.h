#pragma once
#include <stdint.h>
#include <math.h>
namespace wheel_health {
// Latched suspect condition, not proof of a broken encoder. Reset only on reboot.
struct MotionMonitor {
  bool suspect=false, tracking=false;
  uint64_t since=0;
  static bool configured(float threshold,uint64_t timeout) {
    return isfinite(threshold) && threshold>0 && threshold<1 && timeout>0;
  }
  void update(uint64_t now,bool physical_drive,float effort,bool edge,
              float threshold,uint64_t timeout) {
    if(suspect) return;
    if(!configured(threshold,timeout) || !physical_drive ||
       !isfinite(effort) || effort<=threshold) { tracking=false;return; }
    if(!tracking || edge) { since=now;tracking=true;return; }
    if(now<since || now-since>=timeout) suspect=true;
  }
};
inline bool control_direction_allowed(bool quadrature,bool v2,bool forward_confirmed,
 float threshold,uint64_t timeout) {
  return MotionMonitor::configured(threshold,timeout) && (quadrature || (v2 && forward_confirmed));
}
inline bool magnitude_valid(bool initialized,uint64_t now,uint64_t previous,
 bool no_errors,bool confirmed,float scale,bool suspect) {
  return initialized && now>previous && now-previous<=25000 && no_errors &&
    confirmed && isfinite(scale) && scale>0 && !suspect;
}
}
