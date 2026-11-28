// Deterministic host peer for the Pi client. Uses the ESP wire code and watchdog.
#include "firmware/telemetry_demo/receipt_watchdog.h"
#include "firmware/telemetry_demo/control_endpoint.h"
#include "firmware/telemetry_demo/mt6701_core.h"
#include "firmware/telemetry_demo/vehicle_config.h"
#include <iomanip>
#include <iostream>
#include <string>

static void emit(const uint8_t* data, size_t size) {
  for (size_t i=0; i<size; ++i)
    std::cout << std::hex << std::setfill('0') << std::setw(2) << unsigned(data[i]);
  std::cout << std::dec << std::endl;
}

int main() {
  const uint32_t boot = 0x789abcde;
  uint64_t now = 1000;
  uint16_t sequence = 65534;
  endpoint::Engine engine(boot);
  control_wire::Parser parser;
  endpoint::ReceiptWatchdog guard;
  mt6701::AngleTracker angles;
  bool details=false;
  guard.begin(boot, now, 10000);
  std::string op;
  while (std::cin >> op) {
    if (op == "status") {
      uint8_t bytes[64];
      engine.status(bytes, now, parser.errors, 0, false, guard.lost(), guard.pending());
      emit(bytes, sizeof(bytes));
    } else if (op == "pair" || op=="gapped") {
      uint8_t bundle[150];
      uint8_t* bytes=bundle+54;
      telemetry::Sample sample{};
      if(op=="gapped") {
        // Lose 1.6 ms of angle coverage in EVERY reporting interval.
        for(int i=0;i<25;++i) {
          if(i>=9 && i<12) { angles.read_failed(); continue; }
          uint64_t start=now+i*400;
          angles.observe(int((start/400*40)%16384),start,start+100,1000,1);
        }
      }
      now += 10000;
      telemetry::encode(bytes, sequence, sample);
      telemetry::encode_timing(bytes+54, sequence, 0, now, 0, 0, 0);
      if(details && op=="gapped") {
        const auto& s=angles.state;
        const float speed=s.segment_delta*vehicle_config::metres_per_count*1e6f/
            float(s.segment_end_us-s.segment_start_us);
        telemetry::encode_wheel_detail(bundle,boot,sequence,now,false,s.segment_start_us,
            s.segment_end_us,speed,s.read_errors,s.timing_gaps,s.ambiguous_gaps);
        emit(bundle,sizeof(bundle));
      } else emit(bytes,96);
      if (guard.queued(sequence++, now, now)) { engine.link_lost(); details=false; }
    } else if (op == "rx") {
      std::string hex;
      if (!(std::cin >> hex) || hex.size()%2) return 2;
      for (size_t offset=0; offset<hex.size(); offset+=2) {
        const uint8_t byte = uint8_t(std::stoul(hex.substr(offset, 2), nullptr, 16));
        control_wire::Request request{};
        control_wire::Receipt receipt{};
        const auto kind=parser.feed(byte,now,request,receipt);
        if(kind==control_wire::Parser::Kind::SpeedSubscribe && guard.ready()) details=true;
        if (kind == control_wire::Parser::Kind::Ack &&
            guard.acknowledge(receipt, now)) engine.link_restored();
      }
      std::cout << "ok" << std::endl;
    } else if (op == "state") {
      std::cout << unsigned(guard.pending()) << ' ' << guard.lost() << std::endl;
    } else {
      return 2;
    }
  }
}
