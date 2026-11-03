#pragma once
// Separate CSV-only bench protocol. Never mixed with binary Perception packets.
namespace imu_bench {
uint32_t seq=0,tx_drops=0;
const char* header="seq,uptime_us,sample_us,state,fault,present,valid,calibrating,ax,ay,az,gx,gy,gz,attempts,recoveries,read_errors,config_errors,no_data,clipped,late,ambiguous,missed_edges,tx_drops,generation\n";
void emit(uint64_t now) {
  const auto& r=imu_board::device.reading();
  const auto& d=imu_board::device.diag;
  const bool valid=imu_board::device.ready() && imu::fresh(r,now);
  const bool cal=imu_board::first_sample && now>=imu_board::first_sample_time &&
    now-imu_board::first_sample_time<2000000;
  if(seq%100==0 && Serial.availableForWrite()>=int(strlen(header))) Serial.print(header);
  char line[512];
  const int n=snprintf(line,sizeof(line),
    "%lu,%llu,%llu,%u,%u,%u,%u,%u,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu\n",
    (unsigned long)seq++,(unsigned long long)now,(unsigned long long)r.timestamp,
    unsigned(imu_board::device.state()),unsigned(imu_board::device.fault()),unsigned(r.present),
    unsigned(valid),unsigned(cal),r.accel[0],r.accel[1],r.accel[2],r.gyro[0],r.gyro[1],r.gyro[2],
    (unsigned long)d.attempts,(unsigned long)d.recoveries,(unsigned long)d.read_errors,
    (unsigned long)d.config_errors,(unsigned long)d.no_data,(unsigned long)d.clipped,
    (unsigned long)imu_board::late_reads,(unsigned long)imu_board::ambiguous_reads,
    (unsigned long)imu_board::missed_edges,(unsigned long)tx_drops,
    (unsigned long)imu_board::device.generation());
  if(n>0 && size_t(n)<sizeof(line) && Serial.availableForWrite()>=n)
    Serial.write(reinterpret_cast<const uint8_t*>(line),size_t(n));
  else ++tx_drops;
}
}
