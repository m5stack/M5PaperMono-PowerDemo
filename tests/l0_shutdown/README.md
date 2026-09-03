# L0 Shutdown

[English](README.md) | [简体中文](README_CN.md)

## Purpose

This firmware requests PM1 L0 shutdown and measures whole-board input current after the shutdown transition.

## Enabled Modules

- `m5pm_test_support`: test markers and safe hold state
- `m5pm_board`: shared I2C initialization and bus ownership
- `m5pm_power`: PM1 initialization and L0 shutdown request

## Module Configuration

| Module | Configuration |
|---|---|
| ESP32-S3 | No application peripherals are initialized |
| I2C | Shared board bus used to claim PM1 |
| PM1 | Address `0x6E`; initialized through `PowerController::begin()` |
| Power transition | `enter_l0()` requests PM1 L0 shutdown after the 3000 ms preparation delay |
| Test state | PM1 L0 shutdown; no wake source configured |
| Firmware log | `BOARD_OK PM1=1` |

## Runtime Mode

The shared I2C bus and PM1 are initialized, `BOARD_OK` and `TEST_READY` are emitted, the firmware waits 3000 ms, and `enter_l0()` requests PM1 L0 shutdown. No logging or I2C access is performed after a successful shutdown request.

## Measured Current

The current shown in the measurement results is the `WINDOW Avg` from the measurement record.

- 3.7 V: 17.81 uA
- 4.2 V: 20.75 uA

See the [power measurement graph index](../../docs/power-graphs/README.md).

## Configuration

| Setting | Value |
|---|---|
| Target | ESP32-S3 |
| Flash | 16 MB |
| PSRAM | OPI, 80 MHz |
| CPU | 240 MHz |
| I2C | 100 kHz where used |
| Test ID | `l0_shutdown` |
| Config ID | `l0_shutdown` |

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
