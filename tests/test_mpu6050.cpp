#include "firmware/telemetry_demo/mpu6050.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>

struct FakeBus {
  uint8_t regs[128]={}, raw[14]={};
  bool fail_write=false, fail_read=false, bad_readback=false;
  unsigned restarts=0;
  FakeBus() { regs[0x75]=0x68; }
  bool write(uint8_t reg,uint8_t value) {
    if(fail_write) return false;
    regs[reg]=value; return true;
  }
  bool read(uint8_t reg,uint8_t* out,size_t n) {
    if(fail_read) return false;
    if(reg==0x3B) { assert(n==14); memcpy(out,raw,n); return true; }
    assert(n==1); *out=regs[reg];
    if(bad_readback && reg==0x1B) *out=0;
    return true;
  }
  bool recover() { ++restarts; return !fail_write; }
};
void word(uint8_t* p,int value) { uint16_t v=uint16_t(value); p[0]=v>>8; p[1]=v; }
bool near(float a,float b) { return std::fabs(a-b)<0.00001f; }
void initialize(imu::Mpu6050<FakeBus>& d,uint64_t base=0) {
  d.start(base); d.tick(base); d.tick(base+100000);
  for(unsigned i=0;i<10;++i) d.tick(base+200000+i);
}

int main() {
  FakeBus bus; imu::Mpu6050<FakeBus> dev(bus);
  assert(!dev.capture(1));
  initialize(dev); assert(dev.ready());
  assert(bus.restarts==1);
  assert(bus.regs[0x19]==0 && bus.regs[0x1A]==3);
  assert(bus.regs[0x1B]==0x10 && bus.regs[0x1C]==8);
  assert(bus.regs[0x38]==1 && bus.regs[0x37]==0);
  word(bus.raw,8192); word(bus.raw+2,-8192); word(bus.raw+4,4096);
  word(bus.raw+6,32767); // Temperature must not affect motion validity.
  word(bus.raw+8,3280); word(bus.raw+10,-3280); word(bus.raw+12,0);
  assert(dev.capture(300000));
  auto r=dev.reading();
  assert(r.present && r.valid && !r.clipped && r.timestamp==300000);
  assert(near(r.accel[0],9.80665f) && near(r.accel[1],-9.80665f));
  assert(near(r.accel[2],4.903325f));
  assert(near(r.gyro[0],1.74532925f) && near(r.gyro[1],-1.74532925f));
  assert(imu::fresh(r,305000) && !imu::fresh(r,305001));
  assert(!imu::fresh(r,299999));
  const imu::Axes rotation={{1,0,2},{1,-1,1}};
  assert(imu::proper_rotation(rotation));
  auto rotated=imu::rotate(r,rotation);
  assert(near(rotated.accel[0],r.accel[1]) && near(rotated.gyro[1],-r.gyro[0]));
  assert(!imu::proper_rotation(imu::Axes{{0,0,2},{1,1,1}}));
  assert(!imu::proper_rotation(imu::Axes{{0,1,2},{-1,1,1}})); // Reflection.
  assert(!imu::rotate(r,imu::Axes{{0,1,3},{1,1,1}}).valid);
  bus.fail_read=true;
  assert(!dev.capture(310000));
  assert(!dev.reading().valid && dev.reading().timestamp==300000);
  assert(near(dev.reading().accel[0],9.80665f)); // No fabricated zeros.
  bus.fail_read=false;
  assert(dev.capture(320000) && dev.reading().valid);
  // Each accelerometer and gyro rail must invalidate the entire IMU sample.
  for(unsigned offset: {0u,2u,4u,8u,10u,12u}) {
    for(int rail: {-32768,32767}) {
      memset(bus.raw,0,sizeof(bus.raw)); word(bus.raw+offset,rail);
      assert(dev.capture(330000));
      assert(dev.reading().clipped && !dev.reading().valid);
    }
  }
  memset(bus.raw,0,sizeof(bus.raw)); word(bus.raw,-32767); word(bus.raw+8,32766);
  assert(dev.capture(340000) && dev.reading().valid);
  FakeBus bad_id; bad_id.regs[0x75]=0x69;
  imu::Mpu6050<FakeBus> id_dev(bad_id); initialize(id_dev); assert(!id_dev.ready());
  assert(id_dev.fault()==imu::Fault::Identity);
  FakeBus bad_config; bad_config.bad_readback=true;
  imu::Mpu6050<FakeBus> config_dev(bad_config); initialize(config_dev); assert(!config_dev.ready());
  FakeBus write_error; write_error.fail_write=true;
  imu::Mpu6050<FakeBus> write_dev(write_error); initialize(write_dev); assert(!write_dev.ready());
  FakeBus read_error; read_error.fail_read=true;
  imu::Mpu6050<FakeBus> read_dev(read_error); initialize(read_dev); assert(!read_dev.ready());
  // Three read failures enter retry, preserving the old sample but invalidating it.
  bus.fail_read=true;
  for(unsigned i=0;i<3;++i) assert(!dev.capture(350000+i));
  assert(!dev.ready() && dev.reading().present && !dev.reading().valid);
  bus.fail_read=false;
  dev.tick(1350001); assert(bus.restarts==1); // Too early.
  dev.tick(1350002); dev.tick(1450002);
  for(unsigned i=0;i<10;++i) dev.tick(1550002+i);
  assert(dev.ready() && dev.generation()==2 && dev.diag.recoveries==1);
  // No data-ready/capture for 100 ms must trigger recovery.
  dev.tick(1650012); assert(!dev.ready() && dev.fault()==imu::Fault::NoData);
  // Silent register corruption is discovered by periodic audit, not accepted forever.
  FakeBus audit_bus; imu::Mpu6050<FakeBus> audit_dev(audit_bus); initialize(audit_dev);
  audit_bus.regs[0x6C]=1;
  assert(audit_dev.capture(300000)); audit_dev.tick(300010);
  assert(audit_dev.fault()==imu::Fault::Configuration && !audit_dev.reading().valid);
  // Initially absent sensor can return, without ESP32 restart.
  write_error.fail_write=false;
  write_dev.tick(1000000); write_dev.tick(1100000);
  for(unsigned i=0;i<10;++i) write_dev.tick(1200000+i);
  assert(write_dev.ready());
  std::cout << "PASS: IMU setup/conversion/clipping plus nonblocking startup, retry delay, recovery, missing-data watchdog and configuration audit\n";
}
