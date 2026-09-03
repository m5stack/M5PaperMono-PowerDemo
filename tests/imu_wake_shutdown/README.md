# IMU Wake from Shutdown

[English](README.md) | [简体中文](README_CN.md)

## Purpose

This firmware configures BMI270 any-motion detection as the wake source and measures whole-board current while PM1 holds the board in shutdown.

## Enabled Modules

- `m5pm_test_support`: test markers, retained wake status and safe hold state
- `m5pm_board`: shared I2C initialization and bus ownership
- `m5pm_power`: PM1 shutdown preparation and GPIO4 wake retention
- `m5pm_imu_wakeup`: BMI270 any-motion configuration and status readback

## Module Configuration

| Module | Configuration |
|---|---|
| PM1 | Initialized on the shared 100 kHz I2C bus; shutdown wake preparation retains the `3V3_L1` LDO domain and configures GPIO4 |
| BMI270 API | Bosch BMI270 Base API over I2C; address `0x68`, 100 kHz, `read_write_len=30`, no configuration file |
| Low-power profile | Accelerometer: 50 Hz, +/-2 g, power-optimized filter, `OSR4_AVG1`; gyroscope registers are configured for 25 Hz, 2000 dps, power-optimized filter/noise mode, `OSR4`, OIS 2000 dps, but the gyroscope is not enabled; enabled sensors: accelerometer and any-motion |
| Reference profile | Accelerometer: 100 Hz, +/-2 g, performance-optimized filter, `OSR2_AVG2`; gyroscope: 100 Hz, 2000 dps, performance-optimized filter with power-optimized noise mode, `OSR2`, OIS 2000 dps; enabled sensors: accelerometer, gyroscope, wrist-wear wake-up and any-motion |
| Any-motion | Duration `10`, threshold `30`, X/Y/Z axes enabled; any-motion mapped to BMI270 INT1 |
| BMI270 interrupt | INT1 active-low, push-pull, output enabled, input disabled, non-latched; the interrupt drives PM1 GPIO4 |
| IOE1 | Not initialized by this shutdown path; no IOE1 aggregate GPIO is required |
| Wake path | BMI270 interrupt -> PM1 GPIO4 -> PM1 external wake -> shutdown restart |
| Profile selector | `M5PM_IMU_WAKE_REFERENCE_PROFILE=0` selects `PROFILE=low_power`; set it to `1` and rebuild for `PROFILE=reference` |
| Firmware log | `BOARD_OK PM1=1 IMU=1 PROFILE=%s WAKE=imu` |

## Runtime Mode

The test reads retained BMI270 and PM1 status, prepares the PM1 shutdown wake path, configures BMI270 any-motion, releases the I2C handle, emits `BOARD_OK`, `WAKE_ARMED`, and `TEST_READY`, then waits 3 seconds before requesting shutdown. After restart, PM1 external-wake evidence and BMI270 any-motion status are checked; unrelated GPIO, system, and button events invalidate the wake record.

## Operating Modes

The profile is selected by the source macro:

| Profile | Source setting | 3.7 V `WINDOW Avg` | 4.2 V `WINDOW Avg` |
|---|---|---:|---:|
| `low_power` | `#define M5PM_IMU_WAKE_REFERENCE_PROFILE 0` | 42.72 uA | 45.82 uA |
| `reference` | Change the macro to `1` before building | 692.80 uA | 696.20 uA |

## Measured Current

The values above are the `WINDOW Avg` from the approximately 60-second captures for each profile. See the [power measurement graph index](../../docs/power-graphs/README.md).

## Configuration

| Setting | Value |
|---|---|
| Target | ESP32-S3 |
| Test ID | `imu_wake_shutdown` |
| Config ID | `imu_wake_shutdown` |
| Preparation delay | 3000 ms before the shutdown request |
| Wake validation | PM1 external wake and BMI270 any-motion; unrelated PM1 events must be clear |

## Build and Flash

From the repository root, run `python fetch_repos.py check`; if components are missing, run `python fetch_repos.py fetch --skip-existing`. Then build and flash from this directory:

```bash
idf.py set-target esp32s3
idf.py build
idf.py -p <PORT> flash
idf.py -p <PORT> monitor
```

Confirm `TEST_START`, `BOARD_OK`, `WAKE_ARMED`, and `TEST_READY` over USB Type-C, then disconnect USB Type-C before current measurement. Change the profile macro and rebuild for the alternate profile.

## Expected Validation

The pre-shutdown log must report the selected profile and `path=pm1_gpio4+shutdown`. A subsequent wake record is accepted only when PM1 external wake and BMI270 any-motion evidence are both present without unrelated events.
