#pragma once
#include <Arduino.h>
#include <esp_timer.h>
#include <esp_system.h>
#include <esp_random.h>
#include <esp_task_wdt.h>
#include "vehicle_config.h"
#include "control_endpoint.h"
#include "encoder_math.h"
#include "wheel_health.h"
#include "motor_encoder.h"
namespace vehicle_board {
using namespace vehicle_config;
MotorEncoder encoder;
wheel_health::MotionMonitor motion;
struct WheelState {
  uint32_t count=0, previous_gaps=0;
  uint64_t stamp=0;
  float speed=0;
  bool valid=false, initialized=false, continuous_interval=false;
  void capture(uint64_t now,bool driven,float effort) {
    const uint32_t current=encoder.count;
    const bool continuous=encoder.tracking && previous_gaps==encoder.gaps;
    previous_gaps=encoder.gaps;
    now=uint64_t(esp_timer_get_time());
    const auto delta=encoder_math::difference(current,count);
    motion.update(now,driven,effort,delta!=0,motion_effort_threshold,motion_timeout_us);
    continuous_interval=initialized && continuous;
    valid=wheel_health::magnitude_valid(initialized,now,stamp,continuous,wheel_confirmed,
                                        metres_per_count,motion.suspect);
    speed=valid ? float(delta)*metres_per_count*1e6f/float(now-stamp) : 0;
    count=current; stamp=now; initialized=encoder.tracking;
  }
} wheel;
class Driver {
  bool attached_=false,failed_=false;
  static bool pulse_ok(float p) { return isfinite(p)&&p>=500&&p<=2500; }
  static bool pulse(int pin,float us) { return ledcWrite(pin,uint32_t(us*pwm_hz*16383.0f/1e6f+0.5f)); }
public:
  uint64_t last_write=0;
  float applied_effort=0; bool applied_enabled=false;
  bool physical() const { return outputs_enabled&&attached_&&!failed_; }
  bool ready() const { return !outputs_enabled || physical(); }
  void begin() {
    pinMode(pwm_enable,OUTPUT); digitalWrite(pwm_enable,LOW);
    if(!outputs_enabled) return;
    const auto c=calibration();
    if(!pulse_mapping_confirmed||!brake_only_mode_confirmed||!c.ready()||
       !pulse_ok(servo_left_us)||!pulse_ok(servo_center_us)||!pulse_ok(servo_right_us)||
       !pulse_ok(esc_neutral_us)||!pulse_ok(esc_drive_us)||!pulse_ok(esc_brake_us)||
       (servo_left_us-servo_center_us)*(servo_right_us-servo_center_us)>=0||
       (esc_drive_us-esc_neutral_us)*(esc_brake_us-esc_neutral_us)>=0) { failed_=true;return; }
    const bool s=ledcAttach(servo,pwm_hz,14),e=ledcAttach(esc,pwm_hz,14);
    attached_=s&&e;
    if(!attached_) { if(s)ledcDetach(servo);if(e)ledcDetach(esc);failed_=true;return; }
    vehicle::Output neutral; if(write(neutral)) digitalWrite(pwm_enable,HIGH);
  }
  bool write(const vehicle::Output& out) {
    if(!outputs_enabled) return true; // Simulation only; no GPIO writes.
    if(!attached_||failed_) return false;
    const float angle=out.enabled?out.steering:0, effort=out.enabled?out.effort:0;
    const auto c=calibration();
    const float s=servo_center_us+fabsf(angle)/c.max_angle*((angle>=0?servo_left_us:servo_right_us)-servo_center_us);
    const float e=esc_neutral_us+fabsf(effort)*((effort>=0?esc_drive_us:esc_brake_us)-esc_neutral_us);
    const bool ok=pulse(servo,s)&pulse(esc,e);
    if(ok) { last_write=uint64_t(esp_timer_get_time()); applied_effort=effort; applied_enabled=out.enabled; }
    if(!ok) { digitalWrite(pwm_enable,LOW); pulse(servo,servo_center_us);pulse(esc,esc_neutral_us);failed_=true; }
    return ok;
  }
} driver;
endpoint::Engine* engine=nullptr;
control_wire::Parser parser;
uint32_t tx_drops=0;
uint64_t next_tick=0,next_status=0;
bool watchdog_ok=false;
bool send(const uint8_t* p,unsigned n) {
  if(Serial.availableForWrite()<int(n)) { ++tx_drops;return false; }
  return Serial.write(p,n)==n;
}
void status(uint64_t now) {
  uint8_t p[64];engine->status(p,now,parser.errors,tx_drops,driver.physical());send(p,64);
}
bool healthy(uint64_t now) {
  const bool direction_safe=wheel_health::MotionMonitor::configured(motion_effort_threshold,motion_timeout_us);
  return encoder.tracking && wheel.previous_gaps==encoder.gaps && direction_safe && watchdog_ok && driver.ready() && wheel.valid && wheel.speed>=0 && now>=wheel.stamp && now-wheel.stamp<=25000;
}
void begin() {
  static endpoint::Engine instance(esp_random());engine=&instance;engine->calibration()=calibration();
  encoder.begin();
  driver.begin();
  esp_task_wdt_config_t cfg={};cfg.timeout_ms=250;cfg.idle_core_mask=0;cfg.trigger_panic=true;
  esp_err_t err=esp_task_wdt_init(&cfg);
  if(err==ESP_ERR_INVALID_STATE) err=esp_task_wdt_reconfigure(&cfg);
  watchdog_ok=err==ESP_OK && (esp_task_wdt_status(nullptr)==ESP_OK || esp_task_wdt_add(nullptr)==ESP_OK);
  next_tick=next_status=uint64_t(esp_timer_get_time());
}
void service() {
  uint64_t now=uint64_t(esp_timer_get_time());
  encoder.poll(now);
  // Bounded receive work: a hostile/noisy stream cannot monopolize the loop.
  for(unsigned budget=0;budget<128&&Serial.available();++budget) {
    control_wire::Request r;
    now=uint64_t(esp_timer_get_time());
    if(parser.feed(uint8_t(Serial.read()),now,r)) {
      engine->receive(r,now,healthy(now));
      // Revoke/install/cancel outputs take effect before acknowledgement.
      engine->tick(uint64_t(esp_timer_get_time()),healthy(now),driver);
      status(uint64_t(esp_timer_get_time()));
    }
  }
  now=uint64_t(esp_timer_get_time());
  if(now>=next_tick) {
    wheel.capture(now,driver.physical() && driver.applied_enabled,driver.applied_effort);
    now=uint64_t(esp_timer_get_time());
    if(engine->tick(now,healthy(now),driver)) status(now);
    next_tick=now+10000; // Skip late releases; never replay catch-up commands.
  }
  // Deadline enforcement each service, independent of the 100 Hz estimator.
  if(engine->armed()) {
    if(engine->tick(now,healthy(now),driver)) status(now);
  }
  if(now>=next_status) { status(now);next_status=now+100000; }
  if(watchdog_ok) esp_task_wdt_reset();
}
void fill(telemetry::Sample& s,uint64_t now) {
  // Independent ages refer to transmission time (proposed v0.1 clarification).
  s.ticks=int32_t(wheel.count);s.speed=wheel.speed;
  const uint64_t age=now>=wheel.stamp?now-wheel.stamp:65535;
  s.wheel_age=uint16_t(age>65535?65535:age);
  if(wheel.valid && encoder.tracking && wheel.previous_gaps==encoder.gaps && age<=25000) s.flags|=2;
  s.steering=engine->output.steering;
  const uint64_t steer_age=driver.last_write&&now>=driver.last_write?now-driver.last_write:65535;
  s.steer_age=uint16_t(steer_age>65535?65535:steer_age);
  if(driver.physical()&&engine->output.enabled) s.flags|=4;
  // Never set measured-steering or standstill from an unobservable condition.
}
}
