#pragma once
#include <Arduino.h>
#include <esp_timer.h>
#include <esp_system.h>
#include <esp_random.h>
#include <esp_task_wdt.h>
#include "vehicle_config.h"
#include "control_endpoint.h"
#include "receipt_watchdog.h"
#include "encoder_math.h"
#include "wheel_health.h"
#include "motor_encoder.h"
namespace vehicle_board {
using namespace vehicle_config;
MotorEncoder encoder;
wheel_health::MotionMonitor motion;
struct WheelState {
  uint32_t count=0, previous_gaps=0;
  uint32_t previous_errors=0;
  bool control_continuous=false;
  uint64_t stamp=0;
  float speed=0;
  bool valid=false, initialized=false, continuous_interval=false;
  void capture(uint64_t now,bool driven,float effort) {
    const auto reading=encoder.snapshot();
    const uint32_t current=reading.count;
    const bool continuous=reading.tracking && previous_gaps==reading.gaps;
    control_continuous=continuous && previous_errors==reading.read_errors;
    previous_errors=reading.read_errors;
    previous_gaps=reading.gaps;
    const auto delta=encoder_math::difference(current,count);
    motion.update(now,driven,effort,delta!=0,motion_effort_threshold,motion_timeout_us);
    // The count and this timestamp come from the same completed acquisition.
    const uint64_t latch=reading.acquired_us;
    const bool calculable=wheel_health::magnitude_valid(initialized,latch,stamp,continuous,true,
                                        metres_per_count,motion.suspect);
    continuous_interval=calculable;
    valid=calculable && wheel_confirmed;
    // Report nominal speed for commissioning even before direction confirmation.
    // Consumers must still honor the wheel-valid bit. Invalid intervals are zero/invalid.
    speed=calculable ? float(delta)*metres_per_count*1e6f/float(latch-stamp) : 0;
    count=current; stamp=latch; initialized=reading.tracking;
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
    // A disarmed servo gets a continuously low signal. Centering it here would
    // command a fresh physical movement on link loss or link recovery.
    const bool steer_ok=out.enabled ? pulse(servo,s) : ledcWrite(servo,0);
    const bool drive_ok=pulse(esc,e);
    if(steer_ok && drive_ok) {
      last_write=out.enabled?uint64_t(esp_timer_get_time()):0;
      applied_effort=effort; applied_enabled=out.enabled;
      return true;
    }
    digitalWrite(pwm_enable,LOW);
    ledcWrite(servo,0); pulse(esc,esc_neutral_us);
    applied_enabled=false; applied_effort=0; last_write=0; failed_=true;
    return false;
  }
} driver;
endpoint::Engine* engine=nullptr;
control_wire::Parser parser;
endpoint::ReceiptWatchdog receipts;
uint32_t tx_drops=0;
uint64_t next_tick=0,next_status=0;
uint64_t speed_detail_until=0;
bool watchdog_ok=false;
bool send(const uint8_t* p,unsigned n) {
  if(Serial.availableForWrite()>=int(n) && Serial.write(p,n)==n) return true;
  ++tx_drops;
  return false;
}
void status(uint64_t now) {
  uint8_t p[64];
  engine->status(p,now,parser.errors,tx_drops,driver.physical(),receipts.lost(),receipts.pending());
  send(p,64);
}
void trip_link(uint64_t now) {
  speed_detail_until=0;
  engine->link_lost();
  driver.write(engine->output);
  status(now);
}
void pair_queued(uint16_t seq,uint64_t report_us) {
  const uint64_t now=uint64_t(esp_timer_get_time());
  if(receipts.check_stall(now)) trip_link(now);
  if(receipts.queued(seq,report_us,now)) trip_link(now);
}
bool healthy(uint64_t now) {
  const auto reading=encoder.snapshot();
  const bool direction_safe=wheel_health::MotionMonitor::configured(motion_effort_threshold,motion_timeout_us);
  return receipts.ready() && reading.tracking && wheel.control_continuous &&
      wheel.previous_gaps==reading.gaps && wheel.previous_errors==reading.read_errors &&
      direction_safe && watchdog_ok && driver.ready() && wheel.valid &&
      wheel.speed>=0 && now>=wheel.stamp && now-wheel.stamp<=25000;
}
void begin(uint64_t telemetry_period_us) {
  static endpoint::Engine instance(esp_random());engine=&instance;engine->calibration()=calibration();
  encoder.begin();
  driver.begin();
  esp_task_wdt_config_t cfg={};cfg.timeout_ms=250;cfg.idle_core_mask=0;cfg.trigger_panic=true;
  esp_err_t err=esp_task_wdt_init(&cfg);
  if(err==ESP_ERR_INVALID_STATE) err=esp_task_wdt_reconfigure(&cfg);
  watchdog_ok=err==ESP_OK && (esp_task_wdt_status(nullptr)==ESP_OK || esp_task_wdt_add(nullptr)==ESP_OK);
  next_tick=next_status=uint64_t(esp_timer_get_time());
  receipts.begin(engine->boot(),next_tick,telemetry_period_us);
}
void service() {
  uint64_t now=uint64_t(esp_timer_get_time());
  if(receipts.check_stall(now)) trip_link(now);
  // Encoder acquisition runs independently on core 0.
  // Bounded receive work: a hostile/noisy stream cannot monopolize the loop.
  for(unsigned budget=0;budget<128&&Serial.available();++budget) {
    control_wire::Request r{};
    control_wire::Receipt ack{};
    now=uint64_t(esp_timer_get_time());
    const auto kind=parser.feed(uint8_t(Serial.read()),now,r,ack);
    if(kind==control_wire::Parser::Kind::SpeedSubscribe && receipts.ready())
      speed_detail_until=now+2000000;
    if(kind==control_wire::Parser::Kind::Ack) {
      const bool was_ready=receipts.ready();
      if(receipts.acknowledge(ack,now) && !was_ready) {
        engine->link_restored();
        status(now);
      }
    }
    if(kind==control_wire::Parser::Kind::Command) {
      engine->receive(r,now,healthy(now),receipts.ready());
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
    next_tick+=((now-next_tick)/10000+1)*10000; // Keep phase and skip missed releases.
  }
  // Deadline enforcement each service, independent of the 100 Hz estimator.
  if(engine->armed()) {
    if(engine->tick(now,healthy(now),driver)) status(now);
  }
  if(now>=next_status) { status(now);next_status=now+100000; }
  if(watchdog_ok) esp_task_wdt_reset();
}
bool fill(telemetry::Sample& s,uint64_t now) {
  // Independent ages refer to transmission time (proposed v0.1 clarification).
  s.ticks=int32_t(wheel.count);s.speed=wheel.speed;
  const uint64_t age=wheel.stamp && now>=wheel.stamp?now-wheel.stamp:65535;
  s.wheel_age=uint16_t(age>65535?65535:age);
  // A later encoder error affects the next window. This completed, timestamped
  // measurement stays usable at its original source time.
  const bool wheel_present=wheel.continuous_interval && !motion.suspect && age<=25000;
  if(wheel.valid && wheel_present) s.flags|=2;
  s.steering=engine->output.steering;
  const uint64_t steer_age=driver.last_write&&now>=driver.last_write?now-driver.last_write:65535;
  s.steer_age=uint16_t(steer_age>65535?65535:steer_age);
  if(driver.physical()&&engine->output.enabled) s.flags|=4;
  // Never set measured-steering or standstill from an unobservable condition.
  return wheel_present;
}
}
