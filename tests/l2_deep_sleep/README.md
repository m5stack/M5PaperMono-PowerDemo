# L2 Deep Sleep

[English](README.md) | [简体中文](README_CN.md)

## Purpose

This firmware configures the board low-power peripherals, enters ESP32-S3 Deep Sleep without a wake source, and measures whole-board input current in that state.

## Enabled Modules

- `m5pm_test_support`: test markers and safe hold state
- `m5pm_board`: shared I2C initialization and bus ownership
- `m5pm_power`: PM1/IOE1 L2 preparation and Deep Sleep entry
- Board peripherals: M5PM1, M5IOE1, BMI270, ST25R3916 NFC and FT6336G touch are configured by the shared L2 sequence

## Module Configuration

| Module | Configuration |
|---|---|
| ESP32-S3 | Enters Deep Sleep with no application wake source |
| PM1 | Address `0x6E`; L2 power and interrupt masks configured |
| IOE1 | Address `0x4F`; peripheral outputs configured for the L2 state |
| BMI270 | Address `0x68`; `PWR_CONF (0x7C)=0x01` and `PWR_CTRL (0x7D)=0x00`; device interrupt isolated |
| ST25R3916 | Address `0x50`; register-unlock value `0xC2` followed by operation-control `0x00`; NFC rail remains disabled in the L2 baseline |
| FT6336G | Address `0x38`; hibernate command `0xA5=0x03`; touch interrupt isolated |
| I2C | Shared bus at 100 kHz; driver logs reduced before sleep |
| Wake source | None (`WAKE=none`) |
| Test state | PM1 and IOE1 L2 Deep Sleep configuration |
| Firmware log | `BOARD_OK PM1=1 IOE1=1 WAKE=none` |

## Runtime Mode

The shared I2C bus, PM1 and IOE1 are initialized, the no-wake L2 preparation is applied, `BOARD_OK` and `TEST_READY` are emitted, the firmware waits 3000 ms, and ESP32-S3 Deep Sleep is entered.

## Measured Current

The current shown in the measurement results is the `WINDOW Avg` from the measurement record.

- 3.7 V: 129.08 uA
- 4.2 V: 81.06 uA

See the [power measurement graph index](../../docs/power-graphs/README.md).

## Configuration

| Setting | Value |
|---|---|
| Target | ESP32-S3 |
| Flash | 16 MB |
| PSRAM | OPI, 80 MHz |
| CPU | 240 MHz |
| I2C | 100 kHz where used |
| Test ID | `l2_deep_sleep` |
| Config ID | `l2_deep_sleep` |

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
