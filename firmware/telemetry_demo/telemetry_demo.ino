// Simulation by default; optional IMU bench and integrated vehicle modes.
// Vehicle outputs require explicit confirmed calibration in vehicle_config.h.
#include <Arduino.h>
#include <esp_timer.h>
#include "packet.h"
#ifndef APEX_TELEMETRY_HZ
#define APEX_TELEMETRY_HZ 100
#endif
static_assert(APEX_TELEMETRY_HZ==100 || APEX_TELEMETRY_HZ==200,"Supported telemetry rates: 100 or 200 Hz");
constexpr uint64_t data_period=1000000/APEX_TELEMETRY_HZ;
#ifndef APEX_VEHICLE
#define APEX_VEHICLE 0
#endif
#if APEX_VEHICLE
#include "vehicle_board.h"
#endif
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
#if APEX_VEHICLE
  vehicle_board::begin();
#endif
#if APEX_USE_MPU6050
  imu_board::begin();
#endif
  next_data = next_sync = uint64_t(esp_timer_get_time());
}

void loop() {
#if APEX_VEHICLE
  vehicle_board::service();
#endif
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
    // Missing IMU data must not suppress the independent transport heartbeat.
    // Before first acquisition the timestamp is a report time and IMU-valid is 0.
    sample.timestamp=uint32_t(now);
    imu_board::fill(sample,now);
#else
    sample.timestamp = uint32_t(now);
    sample.accel[2] = 9.80665f;
#endif
#if APEX_VEHICLE
    vehicle_board::fill(sample,now);
#endif
    if(available) {
      if(first_packet) sample.flags|=0x40;
#if APEX_VEHICLE
      uint8_t packet[96]; // Queue data + timing together, with the same sequence.
      telemetry::encode(packet,sequence,sample);
      const auto& reading=imu_board::device.reading();
      const uint8_t present=(reading.present?1:0)|(vehicle_board::wheel.initialized?2:0)|
        (vehicle_board::driver.last_write?4:0);
      telemetry::encode_timing(packet+54,sequence,present,now,
        reading.present?reading.timestamp:0,vehicle_board::wheel.stamp,vehicle_board::driver.last_write);
      ++sequence;
#else
      uint8_t packet[54];telemetry::encode(packet,sequence++,sample);
#endif
      if(Serial.availableForWrite()>=int(sizeof(packet))) {
        Serial.write(packet, sizeof(packet));first_packet=false;
      }
#if APEX_VEHICLE
      else ++vehicle_board::tx_drops;
#endif
    }
#endif
    next_data += ((now - next_data) / data_period + 1) * data_period;
  }
  now = uint64_t(esp_timer_get_time());
  if (!APEX_IMU_BENCH && now >= next_sync) {
    uint8_t packet[14];
    telemetry::encode_sync(packet, now);
    if(Serial.availableForWrite()>=int(sizeof(packet))) Serial.write(packet, sizeof(packet));
#if APEX_VEHICLE
    else ++vehicle_board::tx_drops;
#endif
    next_sync += ((now - next_sync) / 1000000 + 1) * 1000000;
  }
#if APEX_USE_MPU6050
  yield(); // Poll data-ready work promptly; no I2C inside the interrupt.
#else
  delay(1);
#endif
}
