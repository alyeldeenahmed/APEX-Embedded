#include "firmware/telemetry_demo/control_endpoint.h"
#include <cassert>
struct Driver {
  bool hardware=false,ok=true; vehicle::Output last;
  bool write(vehicle::Output o) { last=o;return ok; }
  bool physical() const { return hardware; }
};
void calibrated(endpoint::Engine& e) {
  auto& c=e.calibration();c.confirmed=true;c.max_angle=.5f;c.max_accel=3;
  c.max_decel=4;c.steer_rate=1;c.drive_per_accel=.2f;c.brake_per_decel=.1f;
}
int main() {
  using namespace control_wire;using namespace endpoint;
  Engine e(42);calibrated(e);Driver d;
  Request r{};r.boot=42;r.sequence=1;r.session=10;r.epoch=1;r.id=1;
  r.deadline=100000;r.action=Install;
  assert(e.receive(r,0,true)==Installed);
  assert(e.receive(r,0,true)==Replay);
  r.sequence++;r.id++;r.action=Arm;
  assert(e.receive(r,0,true)==Accepted);assert(e.tick(0,true,d));assert(e.code==Simulated);
  r.sequence++;r.id++;r.action=Set;r.steering=.2f;r.acceleration=-2;
  assert(e.receive(r,10000,true)==Accepted);assert(e.tick(10000,true,d));
  assert(d.last.enabled&&fabsf(d.last.effort+.2f)<.0001f);
  r.sequence++;r.id++;r.action=Cancel;r.target=3;r.steering=r.acceleration=0;
  assert(e.receive(r,11000,true)==Revoked);e.tick(11000,true,d);assert(!d.last.enabled);
  r.sequence++;r.id++;r.action=Set;
  assert(e.receive(r,12000,true)==NotArmed);
  r.sequence++;r.id++;r.action=Arm;
  assert(e.receive(r,13000,true)==Accepted);d.hardware=true;
  assert(e.tick(13000,true,d));assert(e.code==Applied);
  assert(e.tick(100000,true,d));assert(e.code==Expired&&!d.last.enabled);
  r.sequence++;r.created=100001;r.deadline=200000;
  assert(e.receive(r,100000,true)==BadTime);
  r.sequence++;r.created=100000;r.action=Install;r.epoch=2;
  assert(e.receive(r,100000,true)==Installed);
  r.sequence++;r.action=Arm;r.epoch=1;
  assert(e.receive(r,100000,true)==BadContext);
  r.sequence++;r.epoch=2;
  assert(e.receive(r,100000,true)==Accepted);
  assert(e.tick(100001,false,d));assert(e.code==Fault&&!d.last.enabled);
  r.sequence++;r.action=Arm;assert(e.receive(r,100002,false)==NotReady);
  r.sequence++;r.boot=43;assert(e.receive(r,100002,true)==BadBoot);
  r.boot=42;r.sequence++;r.action=Revoke;
  assert(e.receive(r,100003,true)==Revoked);
  r.sequence++;r.action=Install;
  assert(e.receive(r,100004,true)==BadContext); // revoked epoch cannot be revived
  r.sequence++;r.epoch++;
  assert(e.receive(r,100005,true)==Installed);
  r.sequence++;r.action=Arm;
  assert(e.receive(r,100006,true)==Accepted);
  r.sequence++;r.action=Cancel;r.target=r.id;
  assert(e.receive(r,100007,true)==Cancelled); // pending, before driver tick
  assert(!e.armed());
  r.sequence++;r.action=Arm;assert(e.receive(r,100008,true)==Accepted);
  d.ok=false;assert(e.tick(100009,true,d)&&e.code==Fault&&!e.armed());
  // Parser supports fragmentation, CRC rejection, arbitrary garbage and recovery.
  uint8_t p[64]={0xa5,0x5a,0x10,1,1};telemetry::u32(p+8,42);
  telemetry::u16(p+62,telemetry::crc(p+2,60));Parser parser;Request parsed{};
  for(unsigned i=0;i<63;++i) assert(!parser.feed(p[i],i,parsed));
  assert(parser.feed(p[63],63,parsed)&&parsed.boot==42);
  p[10]^=1;for(unsigned i=0;i<64;++i) assert(!parser.feed(p[i],100+i,parsed));
  p[10]^=1;bool got=false;for(unsigned i=0;i<64;++i) got=parser.feed(p[i],200+i,parsed)||got;
  assert(got&&parser.errors>0);
  uint8_t status[64];e.status(status,200000,2,3,false);
  assert(status[2]==0x11&&r32(status+8)==42&&r32(status+58)==3);
  assert(telemetry::crc(status+2,60)==(uint16_t(status[62])|uint16_t(status[63])<<8));
}
