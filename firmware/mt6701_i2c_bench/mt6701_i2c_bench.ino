// Read-only hand-turn test. Never drives ESC/servo. Motor power must be OFF.
#include <Wire.h>
constexpr uint8_t address=0x06;
bool readByte(uint8_t reg,uint8_t& value) {
  Wire1.beginTransmission(address); Wire1.write(reg);
  if(Wire1.endTransmission(false)!=0) return false;
  if(Wire1.requestFrom(address,size_t(1),true)!=1) {
    while(Wire1.available()) Wire1.read(); return false;
  }
  value=uint8_t(Wire1.read()); return true;
}
void setup() {
  Serial.begin(115200);
  pinMode(6,OUTPUT); digitalWrite(6,LOW); // optional PWM buffer disabled
  if(!Wire1.begin(10,11,400000)) Serial.println("ERROR: encoder bus initialization failed");
  Wire1.setTimeOut(2);
  delay(500);
  Serial.println("MT6701 I2C: SDA=10 SCL=11 address=0x06. Motor power OFF.");
  Serial.println("ms,read_ok,angle_counts,angle_degrees");
}
void loop() {
  uint8_t high,low;
  bool ok=readByte(0x03,high) && readByte(0x04,low);
  if(ok) {
    unsigned angle=(unsigned(high)<<6)|(low>>2);
    Serial.printf("%lu,1,%u,%.3f\n",millis(),angle,angle*360.0/16384.0);
  } else Serial.printf("%lu,0,NA,NA\n",millis());
  delay(100); // display only, NOT high-speed RPM or wheel-speed calibration
}
