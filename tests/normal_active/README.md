# Normal Operation

[English](README.md) | [简体中文](README_CN.md)

## Purpose

This firmware runs the normal display loop with 50% frontlight and a quality refresh every 5 s, and measures whole-board input current in that state.

## Enabled Modules

- `m5pm_test_support`: test markers and safe hold state
- `m5pm_phase_test`: awake L2 baseline and 60-second timing
- `m5pm_power` and `m5pm_display`: SSD1677 rail, quality refresh and PM1 frontlight

## Module Configuration

| Module | Configuration |
|---|---|
| Display | SSD1677, portrait, 480x800, quality mode; embedded logo frame; EPD rail enabled through IOE1 `PYB_EPD_EN` |
| Frontlight | PM1 GPIO3 (`PYG3_BL_PWM`), 5 kHz PWM at 50%; backlight rail `BL_15V_L3B` enabled by the PM1-controlled driver |
| Power control | PM1 address `0x6E` and IOE1 address `0x4F` on the shared 100 kHz I2C bus |
| Refresh loop | Full-screen logo refresh every 5000 ms; first frame rendered before `TEST_READY` |
| Disabled workloads | Wi-Fi, NFC and LoRa are off |
| Test state | ESP32-S3 active idle-frame loop |
| Firmware log | `BOARD_OK BASELINE=l2-awake DISPLAY=ssd1677 ORIENTATION=portrait REFRESH=quality FULL_SCREEN=1 REFRESH_PERIOD_MS=5000 REFRESH_COUNT_INITIAL=1 AREA=480x800 FRONTLIGHT_PERCENT=50 WIFI=off NFC=off LORA=off WORKLOAD=idle-frame-loop` |

## Runtime Mode

The test restores the display rail, renders the first logo frame, sets the frontlight to 50%, emits `BOARD_OK` and `TEST_READY`, then refreshes the logo every 5 s. After 60 s it reports the measurement-window marker and continues holding the state.

## Measured Current

The current shown in the measurement results is the `WINDOW Avg` from the measurement record.

- 3.7 V: 76.80 mA
- 4.2 V: 72.34 mA

See the [power measurement graph index](../../docs/power-graphs/README.md).

## Configuration

| Setting | Value |
|---|---|
| Target | ESP32-S3 |
| Flash | 16 MB |
| PSRAM | OPI, 80 MHz |
| CPU | 240 MHz |
| I2C | 100 kHz where used |
| Test ID | `normal_active` |
| Config ID | `normal_active` |

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
