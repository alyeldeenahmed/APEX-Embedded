// Host test adapter: calls the actual firmware serializer, not a second encoder.
#include "firmware/telemetry_demo/packet.h"
#include <iostream>
#include <iomanip>

int main() {
  char kind;
  while (std::cin >> kind) {
    uint8_t output[54];
    unsigned size;
    if (kind == 'S') {
      uint64_t timestamp;
      if (!(std::cin >> timestamp)) return 2;
      telemetry::encode_sync(output, timestamp);
      size = 14;
    } else if (kind == 'T') {
      unsigned seq,present;uint64_t report,imu,wheel,steering;
      if(!(std::cin>>seq>>present>>report>>imu>>wheel>>steering)) return 2;
      telemetry::encode_timing(output,uint16_t(seq),uint8_t(present),report,imu,wheel,steering);
      size=42;
    } else if (kind == 'D') {
      telemetry::Sample s = {};
      uint64_t sequence, timestamp;
      unsigned flags;
      if (!(std::cin >> sequence >> timestamp >> flags
            >> s.gyro[0] >> s.gyro[1] >> s.gyro[2]
            >> s.accel[0] >> s.accel[1] >> s.accel[2]
            >> s.ticks >> s.speed >> s.wheel_age >> s.steering >> s.steer_age)) return 2;
      s.timestamp = uint32_t(timestamp);
      s.flags = uint8_t(flags);
      telemetry::encode(output, uint16_t(sequence), s);
      size = 54;
    } else return 2;
    for (unsigned i = 0; i < size; ++i)
      std::cout << std::hex << std::setfill('0') << std::setw(2) << unsigned(output[i]);
    std::cout << '\n' << std::dec;
  }
}
