# IMU Wake from Deep Sleep

[English](README.md) | [简体中文](README_CN.md)

## Purpose

This firmware configures BMI270 any-motion detection as the wake source and measures whole-board current while the ESP32-S3 remains in Deep Sleep.

## Enabled Modules

- `m5pm_test_support`: retained test markers, wake-cause reporting and safe hold state
- `m5pm_board`: shared I2C initialization and bus ownership
- `m5pm_power`: PM1/IOE1 L2 preparation and ESP32-S3 Deep Sleep entry
- `m5pm_imu_wakeup`: BMI270 any-motion configuration and retained status readback

## Module Configuration

| Module | Configuration |
|---|---|
| PM1 | Initialized on the shared 100 kHz I2C bus; L2 IMU-wake preparation enables GPIO4 as the wake input |
| IOE1 | Initialized at address `0x4F`; aggregate GPIO1 forwards the PM1 GPIO4 interrupt to ESP32-S3 EXT0 |
| BMI270 API | Bosch BMI270 Base API over I2C; address `0x68`, 100 kHz, `read_write_len=30`, no configuration file; IMU supply is the retained `3V3_L1` rail |
| Low-power profile | Accelerometer: 50 Hz, +/-2 g, power-optimized filter, `OSR4_AVG1`; gyroscope registers are configured for 25 Hz, 2000 dps, power-optimized filter/noise mode, `OSR4`, OIS 2000 dps, but the gyroscope is not enabled; enabled sensors: accelerometer and any-motion |
| Reference profile | Accelerometer: 100 Hz, +/-2 g, performance-optimized filter, `OSR2_AVG2`; gyroscope: 100 Hz, 2000 dps, performance-optimized filter with power-optimized noise mode, `OSR2`, OIS 2000 dps; enabled sensors: accelerometer, gyroscope, wrist-wear wake-up and any-motion |
| Any-motion | Duration `10`, threshold `30`, X/Y/Z axes enabled; any-motion mapped to BMI270 INT1 |
| BMI270 interrupt | INT1 active-low, push-pull, output enabled, input disabled, non-latched |
| Wake path | BMI270 interrupt -> PM1 GPIO4 -> IOE1 aggregate GPIO1 -> ESP32-S3 `ESP_SLEEP_WAKEUP_EXT0` |
| Profile selector | `M5PM_IMU_WAKE_REFERENCE_PROFILE=0` selects `PROFILE=low_power`; set it to `1` and rebuild for `PROFILE=reference` |
| Firmware log | `BOARD_OK PM1=1 IOE1=1 IMU=1 PROFILE=%s WAKE=imu` |

## Runtime Mode

At boot the test reads retained BMI270 and PM1 status before reinitializing the devices. On a normal boot it prepares the L2 IMU-wake path, configures BMI270 any-motion, releases the I2C handle, emits `BOARD_OK`, `WAKE_ARMED`, and `TEST_READY`, waits 3 seconds, and enters Deep Sleep. After a wake, it accepts only ESP32 EXT0 with PM1 GPIO4 and BMI270 any-motion evidence; unrelated PM1/GPIO/system/button events are rejected.

## Operating Modes

The profile is a compile-time source macro rather than a Kconfig option:

| Profile | Source setting | 3.7 V `WINDOW Avg` | 4.2 V `WINDOW Avg` |
|---|---|---:|---:|
| `low_power` | `#define M5PM_IMU_WAKE_REFERENCE_PROFILE 0` | 726.19 uA | 717.64 uA |
| `reference` | Change the macro to `1` before building | 1.38 mA | 1.36 mA |

## Measured Current

The values above are the `WINDOW Avg` from the approximately 60-second captures for each profile. See the [power measurement graph index](../../docs/power-graphs/README.md).

## Configuration

| Setting | Value |
|---|---|
| Target | ESP32-S3 |
| Test ID | `imu_wake_deep_sleep` |
| Config ID | `imu_wake_deep_sleep` |
| Preparation delay | 3000 ms before entering Deep Sleep |
| Wake validation | ESP32 EXT0, PM1 GPIO4, BMI270 any-motion; unrelated events must be clear |

## Build and Flash

From the repository root, run `python fetch_repos.py check`; if components are missing, run `python fetch_repos.py fetch --skip-existing`. Then build from this directory:

```bash
idf.py set-target esp32s3
idf.py build
idf.py -p <PORT> flash
idf.py -p <PORT> monitor
```

Use the monitor to confirm `TEST_START`, `BOARD_OK`, `WAKE_ARMED`, and `TEST_READY`; disconnect USB Type-C before current measurement. To measure the reference profile, change the macro described above and rebuild before flashing.

## Expected Validation

The pre-sleep log must contain `PROFILE=low_power` or `PROFILE=reference` and the documented wake path. A wake capture is valid only when the expected EXT0, PM1 GPIO4, and BMI270 any-motion indicators are present.
