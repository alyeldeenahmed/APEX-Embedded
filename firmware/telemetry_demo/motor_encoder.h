#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <esp_timer.h>
#include "vehicle_config.h"
#include "mt6701_core.h"

class MotorEncoder {
  uint64_t previous_us=0, next_us=0;
  int previous_angle=0;
public:
  uint32_t count=0, gaps=0, read_errors=0;
  bool tracking=false, bus_ready=false;
  void begin() {
    using namespace vehicle_config;
    // Independent controller: IMU recovery on Wire must not reset this bus.
    bus_ready=Wire1.begin(encoder_sda,encoder_scl,400000);
    Wire1.setTimeOut(2);
  }
  bool read(uint8_t reg,uint8_t& value) {
    Wire1.beginTransmission(vehicle_config::encoder_address);
    Wire1.write(reg);
    if(Wire1.endTransmission(false)!=0) return false;
    if(Wire1.requestFrom(uint8_t(vehicle_config::encoder_address),size_t(1),true)!=1) {
      while(Wire1.available()) Wire1.read();
      return false;
    }
    value=uint8_t(Wire1.read()); return true;
  }
  void poll(uint64_t now) {
    using namespace vehicle_config;
    if(now<next_us) return;
    if(!bus_ready) { begin(); next_us=now+100000; return; }
    next_us=now+400;
    int angle;
    const bool ok=mt6701::read_angle(*this,angle);
    const uint64_t end=uint64_t(esp_timer_get_time());
    if(!ok || end-now>=max_sample_gap_us) {
      previous_us=0; tracking=false; ++gaps;
      if(!ok) {
        ++read_errors;
        Wire1.end(); bus_ready=false; next_us=end+100000;
      }
      return;
    }
    // Conservative transaction-start to next transaction-end bound.
    tracking=previous_us && end-previous_us<max_sample_gap_us;
    if(tracking) {
      const int delta=mt6701::difference(angle,previous_angle);
      if(delta==-8192) { tracking=false; ++gaps; }
      else count+=uint32_t(delta*encoder_sign);
    } else ++gaps;
    previous_angle=angle;
    previous_us=now;
  }
};
