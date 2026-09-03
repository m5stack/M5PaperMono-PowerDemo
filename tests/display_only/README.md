# Display Workload

[English](README.md) | [简体中文](README_CN.md)

## Purpose

This firmware measures whole-board input current while the SSD1677 holds a static frame with the frontlight at 100%.

## Enabled Modules

- `m5pm_test_support`: test markers and safe hold state
- `m5pm_phase_test`: awake L2 baseline and measurement timing
- `m5pm_power` and `m5pm_display`: SSD1677 rail, quality refresh and PM1 frontlight

## Module Configuration

| Module | Configuration |
|---|---|
| Display | SSD1677 4-gray panel; portrait 480x800; SPI2 mode 0, SCLK GPIO15, MOSI GPIO14, DC GPIO17, CS GPIO16, BUSY GPIO18; write clock 20 MHz; EPD rail enabled through IOE1 `PYB_EPD_EN` |
| Power control | PM1 address `0x6E` and IOE1 address `0x4F` on the shared 100 kHz I2C bus |
| Frame | Embedded logo PNG rendered once with `epd_quality`; automatic display is disabled |
| Frontlight | PM1 GPIO3 (`PYG3_BL_PWM`), PWM channel 0 at 5 kHz, 100% duty; backlight rail `BL_15V_L3B` enabled by the PM1-controlled driver |
| Refresh loop | One initial logo refresh; periodic refresh disabled |
| Test state | ESP32-S3 awake with a static display frame |
| Firmware log | `BOARD_OK BASELINE=l2-awake DISPLAY=ssd1677 ORIENTATION=portrait REFRESH=quality AREA=480x800 FRONTLIGHT_PERCENT=100` |

## Runtime Mode

The test restores the display rail, initializes SSD1677, renders one quality-mode logo frame, sets the frontlight to 100%, emits `BOARD_OK` and `TEST_READY`, and holds the static frame through the measurement window.

## Measured Current

The current shown in the measurement results is the `WINDOW Avg` from the measurement record.

- 3.7 V: 89.90 mA
- 4.2 V: 85.30 mA

See the [power measurement graph index](../../docs/power-graphs/README.md).

## Configuration

| Setting | Value |
|---|---|
| Target | ESP32-S3 |
| Flash | 16 MB |
| PSRAM | OPI, 80 MHz |
| CPU | 240 MHz |
| I2C | 100 kHz where used |
| Test ID | `display_only` |
| Config ID | `display_only` |

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
