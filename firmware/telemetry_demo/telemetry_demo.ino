// BENCH SIMULATION ONLY: no real sensors and no actuator outputs.
#include <Arduino.h>
#include <esp_timer.h>
#include "packet.h"

uint16_t sequence = 0;
bool first_packet = true;
uint64_t next_data, next_sync;

void setup() {
  Serial.begin(921600);
  next_data = next_sync = uint64_t(esp_timer_get_time());
}

void loop() {
  uint64_t now = uint64_t(esp_timer_get_time());
  if (now >= next_data) {
    telemetry::Sample sample = {};
    sample.timestamp = uint32_t(now);
    sample.accel[2] = 9.80665f;
    // Synthetic readings are NEVER marked valid.
    sample.flags = first_packet ? 0x40 : 0;
    uint8_t packet[54];
    telemetry::encode(packet, sequence++, sample);
    Serial.write(packet, sizeof(packet));
    first_packet = false;
    next_data += ((now - next_data) / 10000 + 1) * 10000;
  }
  now = uint64_t(esp_timer_get_time());
  if (now >= next_sync) {
    uint8_t packet[14];
    telemetry::encode_sync(packet, now);
    Serial.write(packet, sizeof(packet));
    next_sync += ((now - next_sync) / 1000000 + 1) * 1000000;
  }
  delay(1);
}
