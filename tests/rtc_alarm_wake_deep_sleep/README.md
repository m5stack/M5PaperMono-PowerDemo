# RTC Alarm Wake from Deep Sleep

[English](README.md) | [简体中文](README_CN.md)

## Purpose

This firmware measures whole-board input current while the device is in the RTC Alarm Wake from Deep Sleep operating state.

## Enabled Modules

- `m5pm_test_support`: test markers and retained wake validation
- `m5pm_board` and `m5pm_rtc_wakeup`: shared I2C and RX8130CE control
- `m5pm_rtc_test` and `m5pm_power`: common RTC wake sequence and ESP32-S3 Deep Sleep entry

## Module Configuration

| Module | Configuration |
|---|---|
| RX8130CE | Address `0x32`, 100 kHz I2C; baseline clears flags and disables Timer/update interrupts before configuration |
| Calendar Alarm | Minute match `0x02` at register `0x17`; hour and weekday fields set to `0x80` (disabled); Alarm interrupt enabled |
| PM1/IOE1 | PM1 GPIO0 -> IOE1 aggregate GPIO1 -> ESP32-S3 EXT0; IOE1 retained for Deep Sleep |
| Wake validation | Alarm flag set, competing Timer flag clear, PM1 GPIO0 IRQ present, ESP32 wake cause EXT0 |
| Firmware log | `BOARD_OK PM1=1 IOE1=1 RTC=1 WAKE=rtc_alarm` and `WAKE_ARMED source=rtc_alarm interval=120s path=pm1_gpio0+aggregate_gpio1+esp_ext0` |

## Runtime Mode

The common RTC test reads retained RTC and PM1 flags, prepares the L2 RTC Alarm wake path, configures a 120 s Alarm, releases the bus, emits `BOARD_OK`, `WAKE_ARMED`, and `TEST_READY`, waits 3000 ms, and enters Deep Sleep. On wake it validates the RTC Alarm path and rejects a competing Timer flag.

## Measured Current

The current shown in the measurement results is the `WINDOW Avg` from the measurement record.

- 3.7 V: 76.05 uA
- 4.2 V: 63.15 uA

See the [power measurement graph index](../../docs/power-graphs/README.md).

## Configuration

| Setting | Value |
|---|---|
| Target | ESP32-S3 |
| Flash | 16 MB |
| PSRAM | OPI, 80 MHz |
| CPU | 240 MHz |
| I2C | 100 kHz where used |
| Test ID | `rtc_alarm_wake_deep_sleep` |
| Config ID | `rtc_alarm_wake_deep_sleep` |

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
