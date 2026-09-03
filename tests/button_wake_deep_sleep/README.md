# Button Wake from Deep Sleep

[English](README.md) | [简体中文](README_CN.md)

## Purpose

This firmware configures Key1 and Key2 as ESP32-S3 EXT1 wake sources and measures whole-board input current while the device waits for a button wake from Deep Sleep.

## Enabled Modules

- `m5pm_test_support`: test markers and retained wake validation
- `m5pm_board`, `m5pm_power`: shared I2C and L2 button-wake preparation
- `m5pm_rtc_wakeup`, `m5pm_imu_wakeup`: retained RTC and IMU status readback

## Module Configuration

| Module | Configuration |
|---|---|
| ESP32-S3 buttons | Key1 and Key2 configured as GPIO EXT1 wake inputs with pull-ups |
| PM1/IOE1 | PM1 and IOE1 L2 path initialized; no PM1 wake, GPIO or system IRQ is accepted |
| Wake validation | ESP32 wake cause EXT1 and Key1 or Key2 bit present; RTC/IMU retained status is reported |
| Firmware log | `BOARD_OK PM1=1 IOE1=1 RTC=1 WAKE=button` and `WAKE_ARMED source=button path=esp_ext1` |

## Runtime Mode

The test reads retained status, prepares the L2 button-wake path, configures Key1 and Key2, emits `BOARD_OK`, `WAKE_ARMED`, and `TEST_READY`, waits 3000 ms, and enters Deep Sleep. After wake it validates EXT1 and the selected key bit while requiring PM1 wake and IRQ status to be clear.

## Measured Current

The current shown in the measurement results is the `WINDOW Avg` from the measurement record.

- 3.7 V: 101.95 uA
- 4.2 V: 86.89 uA

See the [power measurement graph index](../../docs/power-graphs/README.md).

## Configuration

| Setting | Value |
|---|---|
| Target | ESP32-S3 |
| Flash | 16 MB |
| PSRAM | OPI, 80 MHz |
| CPU | 240 MHz |
| I2C | 100 kHz where used |
| Test ID | `button_wake_deep_sleep` |
| Config ID | `button_wake_deep_sleep` |

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
