# L1 Standby

[English](README.md) | [简体中文](README_CN.md)

## Purpose

This firmware measures whole-board input current while the device is in PM1 L1 standby state. BMI270 is placed in its low-power register state before shutdown; NFC is outside the retained L1 power domain.

## Enabled Modules

- `m5pm_test_support`: test markers and safe hold state
- `m5pm_board`: shared I2C initialization and bus ownership
- `m5pm_power`: PM1 initialization, BMI270 low-power preparation and L1 standby transition

## Module Configuration

| Module | Configuration |
|---|---|
| ESP32-S3 | No application peripherals are initialized; the processor is shut down by PM1 |
| I2C | Shared board bus; 100 kHz transactions are used for PM1 and BMI270 preparation |
| PM1 | Address `0x6E`; initialized through `PowerController::begin()`; charging is disabled and the LDO hold is enabled before shutdown |
| BMI270 | Address `0x68`; `PWR_CONF (0x7C)=0x01` and `PWR_CTRL (0x7D)=0x00`, with a 100 ms delay between writes |
| NFC power domain | ST25R3916 is supplied from `3V3_L2` through the `PYB_NFC_EN` load switch; this rail is not retained in L1 and the NFC device is not accessed in this test |
| Test state | PM1 L1 standby with the `3V3_L1` domain retained for the board's L1 loads |
| Firmware log | `BOARD_OK PM1=1` |

## Runtime Mode

The shared I2C bus and PM1 are initialized, the BMI270 low-power registers are written, `BOARD_OK` and `TEST_READY` are emitted, and the firmware waits 3000 ms. `enter_l1()` then requests PM1 L1 standby. The console and I2C path are not used after the transition; the NFC `3V3_L2` rail is outside the retained L1 power domain.

## Measured Current

The current shown in the measurement results is the `WINDOW Avg` from the measurement record.

- 3.7 V: 26.07 uA
- 4.2 V: 29.09 uA

See the [power measurement graph index](../../docs/power-graphs/README.md).

## Configuration

| Setting | Value |
|---|---|
| Target | ESP32-S3 |
| Flash | 16 MB |
| PSRAM | OPI, 80 MHz |
| CPU | 240 MHz |
| I2C | 100 kHz where used |
| Test ID | `l1_standby` |
| Config ID | `l1_standby` |

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
