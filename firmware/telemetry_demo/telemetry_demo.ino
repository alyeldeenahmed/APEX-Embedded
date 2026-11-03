// BENCH ONLY: simulation by default; optional MPU-6050 mode. No actuator outputs.
#include <Arduino.h>
#include <esp_timer.h>
#include "packet.h"
#ifndef APEX_USE_MPU6050
#define APEX_USE_MPU6050 0
#endif
#ifndef APEX_IMU_BENCH
#define APEX_IMU_BENCH 0
#endif
#if APEX_IMU_BENCH && !APEX_USE_MPU6050
#error Bench mode requires MPU-6050 mode
#endif
#if APEX_USE_MPU6050
#include "mpu6050_esp32.h"
#endif
#if APEX_IMU_BENCH
#include "imu_bench.h"
#endif

uint16_t sequence = 0;
bool first_packet = true;
uint64_t next_data, next_sync;

void setup() {
  // Current build selects UART0 through the USB-to-UART connector.
  Serial.setTxBufferSize(2048);
  Serial.begin(921600);
#if APEX_USE_MPU6050
  imu_board::begin();
#endif
  next_data = next_sync = uint64_t(esp_timer_get_time());
}

void loop() {
#if APEX_USE_MPU6050
  imu_board::service();
#endif
  uint64_t now = uint64_t(esp_timer_get_time());
  if (now >= next_data) {
#if APEX_IMU_BENCH
    imu_bench::emit(now);
#else
    telemetry::Sample sample = {};
    bool available=true;
#if APEX_USE_MPU6050
    available=imu_board::fill(sample,now);
#else
    sample.timestamp = uint32_t(now);
    sample.accel[2] = 9.80665f;
#endif
    // Wheel and steering remain invalid in both bench modes.
    if(available) {
      if(first_packet) sample.flags|=0x40;
      uint8_t packet[54];
      telemetry::encode(packet, sequence++, sample);
      Serial.write(packet, sizeof(packet));
      first_packet = false;
    }
#endif
    next_data += ((now - next_data) / 10000 + 1) * 10000;
  }
  now = uint64_t(esp_timer_get_time());
  if (!APEX_IMU_BENCH && now >= next_sync) {
    uint8_t packet[14];
    telemetry::encode_sync(packet, now);
    Serial.write(packet, sizeof(packet));
    next_sync += ((now - next_sync) / 1000000 + 1) * 1000000;
  }
#if APEX_USE_MPU6050
  yield(); // Poll data-ready work promptly; no I2C inside the interrupt.
#else
  delay(1);
#endif
}
