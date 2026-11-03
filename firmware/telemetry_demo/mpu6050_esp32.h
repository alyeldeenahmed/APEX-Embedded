#pragma once
#include <Wire.h>
#include <esp_timer.h>
#include "mpu6050.h"

// Pins are deliberately not assigned until the actual board wiring is agreed.
#ifndef APEX_IMU_SDA
#define APEX_IMU_SDA -1
#endif
#ifndef APEX_IMU_SCL
#define APEX_IMU_SCL -1
#endif
#ifndef APEX_IMU_INT
#define APEX_IMU_INT -1
#endif
#ifndef APEX_IMU_ADDRESS
#define APEX_IMU_ADDRESS 0x68
#endif
// Only enable after verifying the mounting-to-vehicle rotation below on the bench.
#ifndef APEX_IMU_AXES_CONFIRMED
#define APEX_IMU_AXES_CONFIRMED 0
#endif
namespace imu_board {
constexpr imu::Axes axes={{0,1,2},{1,1,1}}; // Placeholder identity, NOT measured mounting.
class WireBus {
 public:
  bool write(uint8_t reg,uint8_t value) {
    Wire.beginTransmission(APEX_IMU_ADDRESS); Wire.write(reg); Wire.write(value);
    return Wire.endTransmission()==0;
  }
  bool read(uint8_t reg,uint8_t* out,size_t n) {
    Wire.beginTransmission(APEX_IMU_ADDRESS); Wire.write(reg);
    if (Wire.endTransmission(false)!=0) return false;
    const size_t got=Wire.requestFrom(uint8_t(APEX_IMU_ADDRESS),n,true);
    if (got!=n) { while(Wire.available()) Wire.read(); return false; }
    for(size_t i=0;i<n;++i) out[i]=uint8_t(Wire.read());
    return true;
  }
  void wait_ms(unsigned ms) { delay(ms); }
};
WireBus bus;
imu::Mpu6050<WireBus> device(bus);
portMUX_TYPE mux=portMUX_INITIALIZER_UNLOCKED;
volatile uint64_t edge_time=0;
volatile uint32_t edge_count=0;
uint32_t processed_edge=0;
uint64_t first_sample_time=0;
bool started=false, first_sample=false;
void ARDUINO_ISR_ATTR data_ready() {
  const uint64_t stamp=uint64_t(esp_timer_get_time());
  portENTER_CRITICAL_ISR(&mux);
  edge_time=stamp; ++edge_count;
  portEXIT_CRITICAL_ISR(&mux);
}
bool begin() {
  if (APEX_IMU_SDA<0 || APEX_IMU_SCL<0 || APEX_IMU_INT<0 ||
      APEX_IMU_SDA==APEX_IMU_SCL || APEX_IMU_SDA==APEX_IMU_INT ||
      APEX_IMU_SCL==APEX_IMU_INT) return false;
  if (!Wire.begin(APEX_IMU_SDA,APEX_IMU_SCL,400000)) return false;
  Wire.setTimeOut(2); // Milliseconds; timing still needs measurement.
  pinMode(APEX_IMU_INT,INPUT);
  attachInterrupt(digitalPinToInterrupt(APEX_IMU_INT),data_ready,RISING);
  started=device.begin();
  return started;
}
void service() {
  if (!started) return;
  uint64_t stamp; uint32_t count;
  portENTER_CRITICAL(&mux); stamp=edge_time; count=edge_count; portEXIT_CRITICAL(&mux);
  if (count==processed_edge) return;
  processed_edge=count;
  const uint64_t now=uint64_t(esp_timer_get_time());
  // Avoid labeling a much newer register sample with an old interrupt timestamp.
  if (now<stamp || now-stamp>=1000) { device.invalidate(); return; }
  const bool captured=device.capture(stamp);
  uint32_t after;
  portENTER_CRITICAL(&mux); after=edge_count; portEXIT_CRITICAL(&mux);
  // If data-ready changed during the transaction, association is ambiguous.
  if (after!=count) { device.invalidate(); return; }
  if (captured && !first_sample) { first_sample=true; first_sample_time=stamp; }
}
bool fill(telemetry::Sample& packet,uint64_t now) {
  const auto r=imu::rotate(device.reading(),axes);
  if (!r.present) return false; // No fabricated sample before first acquisition.
  packet.timestamp=uint32_t(r.timestamp);
  for(unsigned i=0;i<3;++i) { packet.accel[i]=r.accel[i]; packet.gyro[i]=r.gyro[i]; }
  if (APEX_IMU_AXES_CONFIRMED && imu::fresh(r,now)) packet.flags|=1;
  if(first_sample && now>=first_sample_time && now-first_sample_time<2000000) packet.flags|=8;
  return true;
}
}
