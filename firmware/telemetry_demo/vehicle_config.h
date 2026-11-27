#pragma once
#include "vehicle_core.h"
namespace vehicle_config {
// November test schematic, with MT6701 mounted at the motor.
constexpr int encoder_scl=11,encoder_sda=10,servo=5,esc=4,pwm_enable=6;
constexpr uint8_t encoder_address=0x06; // 7-bit default; 0x46 only if verified programmed
// CLI: --vehicle --enable-outputs. Arduino IDE: explicitly change this to 1.
// This requests physical PWM; it does NOT bypass calibration or ARM checks.
#ifndef APEX_ENABLE_OUTPUTS
#define APEX_ENABLE_OUTPUTS 0
#endif
static_assert(APEX_ENABLE_OUTPUTS==0 || APEX_ENABLE_OUTPUTS==1,"Outputs must be 0 or 1");
// The forward sign has been checked; reliable continuous tracking is pending.
constexpr bool wheel_confirmed=false, outputs_enabled=APEX_ENABLE_OUTPUTS;
// Assumed upper bound of 30,000 motor RPM; shrink gap for a faster motor.
constexpr uint64_t max_sample_gap_us=1000;
constexpr uint32_t encoder_bus_hz=400000;
constexpr uint32_t encoder_poll_us=400;
// Standard TT-02 original gears confirmed by Aly: Tamiya 22T/70T, 8.27:1.
// 65 mm diameter supplied by Aly; nominal circumference, not loaded rollout.
constexpr float motor_revs_per_wheel_rev=8.27f;
constexpr float wheel_diameter_m=0.065f;
constexpr float loaded_circumference_m=3.14159265358979323846f*wheel_diameter_m;
constexpr float metres_per_count=motor_revs_per_wheel_rev>0
  ? loaded_circumference_m/(16384.0f*motor_revs_per_wheel_rev) : 0;
constexpr int encoder_sign=-1; // Confirmed forward motor rotation on the installed encoder.
// Negotiated extension is implemented, NOT enabled until peer decoder acceptance.
#ifndef APEX_WHEEL_DIAGNOSTICS_V2
#define APEX_WHEEL_DIAGNOSTICS_V2 0
#endif
constexpr bool wheel_diagnostics_v2=APEX_WHEEL_DIAGNOSTICS_V2;
static_assert(encoder_sign==1 || encoder_sign==-1,"Encoder sign must be +1 or -1");
// Hardware calibration required; zero disables the motion diagnostic.
constexpr float motion_effort_threshold=0;
constexpr uint64_t motion_timeout_us=0;
constexpr unsigned pwm_hz=50;
// Actual static measurements: left 20/18 degrees, centre 0/0, right 18/20.
// Equivalent bicycle angle uses their mean; interpolation remains approximate.
constexpr float servo_left_us=1250,servo_center_us=1500,servo_right_us=1750;
constexpr float esc_neutral_us=0,esc_drive_us=0,esc_brake_us=0;
constexpr bool pulse_mapping_confirmed=false, brake_only_mode_confirmed=false;
inline vehicle::Calibration calibration() {
  vehicle::Calibration c;
  c.max_angle=0.33161256f; // +/-19 degrees equivalent road-wheel angle
  c.max_accel=0;        // m/s^2
  c.max_decel=0;        // positive magnitude in m/s^2
  c.steer_rate=0;       // rad/s
  c.drive_per_accel=0;  // normalized effort per m/s^2
  c.brake_per_decel=0;
  c.confirmed=false;
  return c;
}
// Edit calibration() with measured limits and force-to-effort coefficients, then
// set its confirmed field. Defaults intentionally prevent ARM.
}
