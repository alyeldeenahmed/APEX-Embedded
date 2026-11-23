#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <driver/gpio.h>
#include <esp_timer.h>
#include "vehicle_config.h"

#include "mt6701_core.h"

class MotorEncoder {
  uint64_t previous_us=0, next_us=0;
  int previous_angle=0;
public:
  uint32_t count=0, gaps=0;
  bool tracking=false, bus_ready=false;
  void begin() {
    using namespace vehicle_config;
    pinMode(encoder_cs,OUTPUT);
    digitalWrite(encoder_cs,HIGH);
    bus_ready=SPI.begin(encoder_clk,encoder_do,-1,encoder_cs);
    gpio_pullup_en(gpio_num_t(encoder_do)); // disconnected data line reads invalid frames
  }
  void poll(uint64_t now) {
    using namespace vehicle_config;
    if (!bus_ready || now<next_us) return;
    SPI.beginTransaction(SPISettings(1000000,MSBFIRST,SPI_MODE2));
    digitalWrite(encoder_cs,LOW);
    delayMicroseconds(1);
    uint32_t frame=0;
    for (int i=0;i<3;++i) frame=(frame<<8) | SPI.transfer(0);
    delayMicroseconds(1);
    digitalWrite(encoder_cs,HIGH);
    SPI.endTransaction();
    const uint64_t end=uint64_t(esp_timer_get_time());
    next_us=now+400;
    int angle;
    if (!mt6701::unpack(frame,angle) || end-now>=max_sample_gap_us) {
      previous_us=0; tracking=false; ++gaps; return;
    }
    // Bound capture-to-capture interval conservatively using prior start, current end.
    tracking=previous_us && end-previous_us<max_sample_gap_us;
    if (tracking) {
      int delta=(angle-previous_angle+24576)%16384-8192;
      if (delta==-8192) { tracking=false; ++gaps; }
      else count+=uint32_t(delta*encoder_sign);
    } else ++gaps;
    previous_angle=angle;
    previous_us=now;
  }
};

