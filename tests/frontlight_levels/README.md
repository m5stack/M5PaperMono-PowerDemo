# Frontlight Levels

[English](README.md) | [简体中文](README_CN.md)

## Purpose

This firmware measures steady whole-board input current with the SSD1677 display held in quality mode and the frontlight set to one selected PWM level.

## Enabled Modules

- `m5pm_test_support`: test markers and safe hold state
- `m5pm_phase_test`: shared awake L2 baseline and measurement timing
- `m5pm_power`: display rail and PM1 frontlight PWM control
- `m5pm_display`: SSD1677 display initialization and static frame

## Module Configuration

| Module | Configuration |
|---|---|
| ESP32-S3 | 240 MHz CPU, 16 MB flash, OPI PSRAM at 80 MHz |
| I2C | Shared PM1/IOE1 bus at 100 kHz |
| Power control | PM1 address `0x6E` and IOE1 address `0x4F` on the shared bus |
| Display | SSD1677 4-gray panel; portrait 480x800; SPI2 mode 0, SCLK GPIO15, MOSI GPIO14, DC GPIO17, CS GPIO16, BUSY GPIO18, write clock 20 MHz; EPD rail `EPD_3V3_L3B` enabled through IOE1 `PYB_EPD_EN`; one quality-mode white refresh before measurement |
| Frontlight | PM1 GPIO3 (`PYG3_BL_PWM`), PWM channel 0 on the display backlight driver; 5 kHz frequency; selected duty is applied after the initial refresh |
| Firmware log | `BOARD_OK BASELINE=l2-awake DISPLAY=ssd1677 ORIENTATION=portrait REFRESH=quality AREA=480x800 PERIODIC_REFRESH=0 FRONTLIGHT_PERCENT=%u` |

## Runtime Mode

The test prepares the awake L2 baseline, restores the display rail, initializes the SSD1677, performs one quality-mode white refresh, applies the selected frontlight duty, and emits `TEST_READY`. It then holds the static frame for the measurement window without periodic refreshes.

## Operating Modes

Select one defaults file before building. Each file sets one Kconfig symbol and the matching `config_id`:

| `config_id` | Kconfig selection | Frontlight |
|---|---|---:|
| `frontlight-0` | `CONFIG_M5PM_FRONTLIGHT_0=y` | 0% |
| `frontlight-50` | `CONFIG_M5PM_FRONTLIGHT_50=y` | 50% |
| `frontlight-100` | `CONFIG_M5PM_FRONTLIGHT_100=y` | 100% |

For example:

```powershell
Remove-Item -Force sdkconfig -ErrorAction SilentlyContinue
$env:SDKCONFIG_DEFAULTS = "sdkconfig.defaults;sdkconfig.frontlight-50.defaults"
idf.py reconfigure
```

In Bash, use `rm -f sdkconfig`, then `export SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.frontlight-50.defaults"` and run `idf.py reconfigure`.

## Measured Current

The current shown in the measurement results is the `WINDOW Avg` from the measurement record.

| Frontlight | 3.7 V | 4.2 V |
|---:|---:|---:|
| 0% | 31.55 mA | 29.13 mA |
| 50% | 61.28 mA | 58.23 mA |
| 100% | 89.98 mA | 85.35 mA |

See the [power measurement graph index](../../docs/power-graphs/README.md).

## Configuration

| Setting | Value |
|---|---|
| Target | ESP32-S3 |
| Test ID | `frontlight_levels` |
| Config ID | `frontlight-0`, `frontlight-50` or `frontlight-100` |
| Refresh behavior | One initial quality refresh; `PERIODIC_REFRESH=0` during capture |

## Build and Flash

From the repository root, run `python fetch_repos.py check`; if components are missing, run `python fetch_repos.py fetch --skip-existing`. Then select a defaults file and build here:

```powershell
Remove-Item -Force sdkconfig -ErrorAction SilentlyContinue
$env:SDKCONFIG_DEFAULTS = "sdkconfig.defaults;sdkconfig.frontlight-50.defaults"
idf.py set-target esp32s3
idf.py build
idf.py -p <PORT> flash
idf.py -p <PORT> monitor
```

Use the monitor to confirm `TEST_START`, `BOARD_OK`, and `TEST_READY`; disconnect USB Type-C before current measurement.

## Expected Validation

The `BOARD_OK` line must report the selected `FRONTLIGHT_PERCENT` before `TEST_READY`. The display remains static while the measurement window is collected.
