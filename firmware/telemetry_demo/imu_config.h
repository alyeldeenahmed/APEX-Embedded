#pragma once
// ESP32-S3 DevKitC-1 N16R8, selected 3 November 2026.
// Numbers are GPIO labels, NOT header pin positions.
#ifndef APEX_IMU_SDA
#define APEX_IMU_SDA 8
#endif
#ifndef APEX_IMU_SCL
#define APEX_IMU_SCL 9
#endif
#ifndef APEX_IMU_INT
#define APEX_IMU_INT 4
#endif
#ifndef APEX_IMU_ADDRESS
#define APEX_IMU_ADDRESS 0x68
#endif
// Keep 0 until physical orientation is checked for vehicle-coordinate telemetry.
#ifndef APEX_IMU_AXES_CONFIRMED
#define APEX_IMU_AXES_CONFIRMED 0
#endif
