#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "vehicle_config.h"
#include "mt6701_core.h"

class MotorEncoder {
public:
  using Snapshot=mt6701::Snapshot;

  // Only this task accesses Wire1. The separate IMU bus may run concurrently.
  void begin() {
    esp_timer_create_args_t args={};
    args.callback=notify;
    args.arg=this;
    args.name="encoder_tick";
    args.skip_unhandled_events=true;
    if(esp_timer_create(&args,&timer_)!=ESP_OK) return;
    if(xTaskCreatePinnedToCore(run,"motor_encoder",4096,this,4,&task_,0)!=pdPASS) {
      esp_timer_delete(timer_);
      return;
    }
    if(esp_timer_start_periodic(timer_,vehicle_config::encoder_poll_us)!=ESP_OK) {
      vTaskDelete(task_);
      esp_timer_delete(timer_);
    }
  }

  Snapshot snapshot() {
    portENTER_CRITICAL(&mux_);
    Snapshot value=published_;
    portEXIT_CRITICAL(&mux_);
    const uint64_t now=uint64_t(esp_timer_get_time());
    if(!value.acquired_us || now<value.acquired_us ||
       now-value.acquired_us>=vehicle_config::max_sample_gap_us) value.tracking=false;
    return value;
  }

  bool read(uint8_t reg,uint8_t& value) {
    Wire1.beginTransmission(vehicle_config::encoder_address);
    Wire1.write(reg);
    if(Wire1.endTransmission(false)!=0) return false;
    if(Wire1.requestFrom(uint8_t(vehicle_config::encoder_address),size_t(1),true)!=1) {
      while(Wire1.available()) Wire1.read();
      return false;
    }
    const int byte_read=Wire1.read();
    if(byte_read<0) return false;
    value=uint8_t(byte_read);
    return true;
  }

private:
  portMUX_TYPE mux_=portMUX_INITIALIZER_UNLOCKED;
  Snapshot published_;
  mt6701::AngleTracker tracker_;
  TaskHandle_t task_=nullptr;
  esp_timer_handle_t timer_=nullptr;
  uint64_t retry_at_us_=0;
  unsigned consecutive_errors_=0;
  bool bus_ready_=false;
  static constexpr uint32_t retry_delay_us=100000;

  static void notify(void* self) {
    xTaskNotifyGive(static_cast<MotorEncoder*>(self)->task_);
  }
  static void run(void* self) {
    auto* encoder=static_cast<MotorEncoder*>(self);
    for(;;) {
      ulTaskNotifyTake(pdTRUE,portMAX_DELAY);
      encoder->poll();
    }
  }
  void poll() {
    using namespace vehicle_config;
    uint64_t now=uint64_t(esp_timer_get_time());
    if(now<retry_at_us_) return;
    if(!bus_ready_) {
      bus_ready_=Wire1.begin(encoder_sda,encoder_scl,encoder_bus_hz);
      if(bus_ready_) Wire1.setTimeOut(2);
    }
    if(!bus_ready_) {
      tracker_.read_failed();
      retry_at_us_=now+retry_delay_us;
    } else {
      int angle;
      const uint64_t start=uint64_t(esp_timer_get_time());
      const bool ok=mt6701::read_angle(*this,angle);
      const uint64_t end=uint64_t(esp_timer_get_time());
      if(ok) {
        consecutive_errors_=0;
        tracker_.observe(angle,start,end,max_sample_gap_us,encoder_sign);
      } else {
        tracker_.read_failed();
        // Rebaseline after any lost transaction. A single NACK used to close
        // the controller and add two 100 ms waits, even on a healthy next read.
        if(++consecutive_errors_>=3) {
          Wire1.end();
          bus_ready_=false;
          consecutive_errors_=0;
          retry_at_us_=end+retry_delay_us;
        }
      }
    }
    portENTER_CRITICAL(&mux_);
    published_=tracker_.state;
    portEXIT_CRITICAL(&mux_);
  }
};
