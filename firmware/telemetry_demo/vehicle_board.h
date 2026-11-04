#pragma once
#include <Arduino.h>
#include <esp_timer.h>
#include <esp_system.h>
#include <esp_random.h>
#include <esp_task_wdt.h>
#include "vehicle_config.h"
#include "control_endpoint.h"
#include "encoder_math.h"
namespace vehicle_board {
using namespace vehicle_config;
portMUX_TYPE mux=portMUX_INITIALIZER_UNLOCKED;
volatile uint32_t ticks=0,edge_errors=0;
volatile uint64_t last_edge=0;
volatile uint8_t previous_ab=0;
void ARDUINO_ISR_ATTR edge() {
  const uint64_t now=uint64_t(esp_timer_get_time());
  const uint8_t ab=uint8_t((digitalRead(encoder_a)<<1)|digitalRead(encoder_b));
  portENTER_CRITICAL_ISR(&mux);
  if(last_edge && now-last_edge<min_edge_us) { ++edge_errors; portEXIT_CRITICAL_ISR(&mux); return; }
  if(quadrature) {
    // Full transition decoder; illegal two-bit changes invalidate this interval.
    const int direction=encoder_math::transition(previous_ab,ab);
    if(direction==2) ++edge_errors;
    else ticks+=uint32_t(direction*encoder_sign);
    previous_ab=ab;
  } else ++ticks;
  last_edge=now;
  portEXIT_CRITICAL_ISR(&mux);
}
struct WheelState {
  uint32_t count=0,errors=0; uint64_t stamp=0;
  float speed=0; bool valid=false,initialized=false;
  void capture(uint64_t now) {
    uint32_t n,e;
    portENTER_CRITICAL(&mux); n=ticks;e=edge_errors;portEXIT_CRITICAL(&mux);
    const uint32_t delta=n-count;
    const int64_t signed_delta=encoder_math::difference(n,count);
    valid=initialized && now>stamp && now-stamp<=25000 && e==errors && wheel_confirmed &&
      quadrature && isfinite(metres_per_count) && metres_per_count>0;
    if(initialized && now>stamp) speed=float(quadrature?signed_delta:int64_t(delta))*metres_per_count*1e6f/float(now-stamp);
    if(!isfinite(speed)) { valid=false;speed=0; }
    count=n;errors=e;stamp=now;initialized=true;
  }
} wheel;
class Driver {
  bool attached_=false,failed_=false;
  static bool pulse_ok(float p) { return isfinite(p)&&p>=500&&p<=2500; }
  static bool pulse(int pin,float us) { return ledcWrite(pin,uint32_t(us*pwm_hz*16383.0f/1e6f+0.5f)); }
public:
  bool physical() const { return outputs_enabled&&attached_&&!failed_; }
  bool ready() const { return !outputs_enabled || physical(); }
  void begin() {
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
    vehicle::Output neutral; write(neutral);
  }
  bool write(const vehicle::Output& out) {
    if(!outputs_enabled) return true; // Simulation only; no GPIO writes.
    if(!attached_||failed_) return false;
    const float angle=out.enabled?out.steering:0, effort=out.enabled?out.effort:0;
    const auto c=calibration();
    const float s=servo_center_us+fabsf(angle)/c.max_angle*((angle>=0?servo_left_us:servo_right_us)-servo_center_us);
    const float e=esc_neutral_us+fabsf(effort)*((effort>=0?esc_drive_us:esc_brake_us)-esc_neutral_us);
    const bool ok=pulse(servo,s)&pulse(esc,e);
    if(!ok) { pulse(servo,servo_center_us);pulse(esc,esc_neutral_us);failed_=true; }
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
  return watchdog_ok && driver.ready() && wheel.valid && wheel.speed>=0 && now>=wheel.stamp && now-wheel.stamp<=25000;
}
void begin() {
  static endpoint::Engine instance(esp_random());engine=&instance;engine->calibration()=calibration();
  pinMode(encoder_a,INPUT_PULLUP);pinMode(encoder_b,INPUT_PULLUP);
  previous_ab=uint8_t((digitalRead(encoder_a)<<1)|digitalRead(encoder_b));
  attachInterrupt(digitalPinToInterrupt(encoder_a),edge,quadrature?CHANGE:RISING);
  if(quadrature) attachInterrupt(digitalPinToInterrupt(encoder_b),edge,CHANGE);
  driver.begin();
  esp_task_wdt_config_t cfg={};cfg.timeout_ms=250;cfg.idle_core_mask=0;cfg.trigger_panic=true;
  esp_err_t err=esp_task_wdt_init(&cfg);
  if(err==ESP_ERR_INVALID_STATE) err=esp_task_wdt_reconfigure(&cfg);
  watchdog_ok=err==ESP_OK && (esp_task_wdt_status(nullptr)==ESP_OK || esp_task_wdt_add(nullptr)==ESP_OK);
  next_tick=next_status=uint64_t(esp_timer_get_time());
}
void service() {
  uint64_t now=uint64_t(esp_timer_get_time());
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
    wheel.capture(now);
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
  if(wheel.valid&&age<=25000) s.flags|=2;
  s.steering=engine->output.steering;s.steer_age=0;
  if(driver.physical()&&engine->output.enabled) s.flags|=4;
  // Never set measured-steering or standstill from an unobservable condition.
}
}
