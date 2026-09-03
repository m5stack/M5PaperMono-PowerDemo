# RTC Alarm Wake from Shutdown

[English](README.md) | [简体中文](README_CN.md)

## Purpose

This firmware measures whole-board input current while the device is in the RTC Alarm Wake from Shutdown operating state.

## Enabled Modules

- `m5pm_test_support`: test markers and retained wake validation
- `m5pm_board` and `m5pm_rtc_wakeup`: shared I2C and RX8130CE control
- `m5pm_rtc_test` and `m5pm_power`: common RTC wake sequence and PM1 shutdown entry

## Module Configuration

| Module | Configuration |
|---|---|
| RX8130CE | Address `0x32`, 100 kHz I2C; baseline clears flags and disables Timer/update interrupts before configuration |
| Calendar Alarm | Minute match `0x02` at register `0x17`; hour and weekday fields set to `0x80` (disabled); Alarm interrupt enabled |
| PM1 | GPIO0 external wake retained while PM1 enters shutdown; IOE1 is not used in the shutdown path |
| Wake validation | Alarm flag set, competing Timer flag clear, PM1 external-wake evidence present |
| Firmware log | `BOARD_OK PM1=1 IOE1=0 RTC=1 WAKE=rtc_alarm` and `WAKE_ARMED source=rtc_alarm interval=120s path=pm1_gpio0+shutdown` |

## Runtime Mode

The common RTC test reads retained flags, prepares the PM1 GPIO0 shutdown wake path, configures a 120 s Alarm, releases the bus, emits `BOARD_OK`, `WAKE_ARMED`, and `TEST_READY`, waits 3000 ms, and requests PM1 shutdown. After restart it validates PM1 external-wake evidence and the Alarm flag.

## Measured Current

The current shown in the measurement results is the `WINDOW Avg` from the measurement record.

- 3.7 V: 18.26 uA
- 4.2 V: 21.17 uA

See the [power measurement graph index](../../docs/power-graphs/README.md).

## Configuration

| Setting | Value |
|---|---|
| Target | ESP32-S3 |
| Flash | 16 MB |
| PSRAM | OPI, 80 MHz |
| CPU | 240 MHz |
| I2C | 100 kHz where used |
| Test ID | `rtc_alarm_wake_shutdown` |
| Config ID | `rtc_alarm_wake_shutdown` |

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
