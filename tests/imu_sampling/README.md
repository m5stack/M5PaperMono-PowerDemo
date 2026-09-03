# IMU Sampling

[English](README.md) | [简体中文](README_CN.md)

## Purpose

This firmware continuously samples the BMI270 accelerometer and gyroscope with a fixed configuration and measures whole-board input current during sampling.

## Enabled Modules

- `m5pm_test_support`: test markers and safe hold state
- `m5pm_phase_test`: awake L2 baseline and measurement timing
- `m5pm_power` and `m5pm_imu`: BMI270 rail restore, initialization and sampling

## Module Configuration

| Module | Configuration |
|---|---|
| BMI270 API | Bosch BMI270 Base API over I2C; address `0x68`, 100 kHz, `read_write_len=30`, no configuration file; IMU supply is the `3V3_L1` rail |
| Accelerometer | Enabled; performance-optimized filter, `OSR2_AVG2` bandwidth, 100 Hz ODR, +/-2 g range |
| Gyroscope | Enabled; performance-optimized filter, power-optimized noise mode, `OSR2` bandwidth, 100 Hz ODR, 2000 dps range, OIS range 2000 dps |
| Sensor set | Accelerometer and gyroscope only; AUX, temperature and wake features are not enabled by this sampler |
| Sampling | `bmi2_get_sensor_data()` through `Sampler::read()`; one read every 10 ms, 9000 samples |
| Wake/display | No wake interrupt configuration; display refresh disabled |
| Test state | ESP32-S3 awake while the fixed sampling loop runs |
| Firmware log | `BOARD_OK BASELINE=l2-awake IMU=bmi270 ACCEL=on GYRO=on ODR_HZ=100 READ_PERIOD_MS=10 WAKE_INTERRUPT=0 DISPLAY_REFRESH=0` |

## Runtime Mode

The test restores the IMU rail, initializes the BMI270 Base API with the fixed accelerometer and gyroscope configuration, reads one sample, emits `BOARD_OK` and `TEST_READY`, then reads 9000 samples at 10 ms intervals before entering the safe hold state.

## Measured Current

The current shown in the measurement results is the `WINDOW Avg` from the measurement record.

- 3.7 V: 31.63 mA
- 4.2 V: 28.99 mA

See the [power measurement graph index](../../docs/power-graphs/README.md).

## Configuration

| Setting | Value |
|---|---|
| Target | ESP32-S3 |
| Flash | 16 MB |
| PSRAM | OPI, 80 MHz |
| CPU | 240 MHz |
| I2C | 100 kHz where used |
| Test ID | `imu_sampling` |
| Config ID | `imu_sampling` |

## Build and Flash

Use ESP-IDF 5.5.5 from this directory:

```bash
idf.py set-target esp32s3
idf.py build
idf.py -p <PORT> flash
idf.py -p <PORT> monitor
```

Disconnect USB Type-C before formal power measurement and do not run the serial monitor during measurement.

## Expected Validation

Confirm `TEST_START`, the test-specific `BOARD_OK` fields, and `TEST_READY`. The firmware emits a failure marker if initialization or validation cannot proceed.
