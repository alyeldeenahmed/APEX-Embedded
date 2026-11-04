#pragma once
#include "vehicle_core.h"
namespace vehicle_config {
// Proposed GPIOs, separate from IMU 8/9/4. Electrical fitting is still pending.
constexpr int encoder_a=5,encoder_b=6,servo=15,esc=16;
constexpr bool quadrature=false, wheel_confirmed=false, outputs_enabled=false;
constexpr float metres_per_count=0; // Quadrature mode counts all four transitions.
constexpr int encoder_sign=1;
constexpr uint32_t min_edge_us=0; // Set only after measuring the maximum pulse rate.
constexpr unsigned pwm_hz=50;
constexpr float servo_left_us=0,servo_center_us=0,servo_right_us=0;
constexpr float esc_neutral_us=0,esc_drive_us=0,esc_brake_us=0;
constexpr bool pulse_mapping_confirmed=false, brake_only_mode_confirmed=false;
inline vehicle::Calibration calibration() { return vehicle::Calibration{}; }
// Edit calibration() with measured limits and force-to-effort coefficients, then
// set its confirmed field. Defaults intentionally prevent ARM.
}
