# Display Refresh Modes

[English](README.md) | [简体中文](README_CN.md)

## Purpose

This firmware measures the whole-board input current and the duration of one complete SSD1677 refresh window for each configured refresh mode.

## Enabled Modules

- `m5pm_test_support`: test markers and safe hold state
- `m5pm_phase_test`: shared awake L2 baseline and measurement-window timing
- `m5pm_power`: IOE1 display rail and PM1 frontlight control
- `m5pm_display`: SSD1677 portrait display at 480x800

## Module Configuration

| Module | Configuration |
|---|---|
| ESP32-S3 | 240 MHz CPU, 16 MB flash, OPI PSRAM at 80 MHz from `sdkconfig.defaults` |
| I2C | Shared board bus at 100 kHz for PM1 and IOE1 |
| Power control | PM1 address `0x6E` and IOE1 address `0x4F` on the shared bus |
| Display | SSD1677 4-gray panel; portrait 480x800; SPI2 mode 0, SCLK GPIO15, MOSI GPIO14, DC GPIO17, CS GPIO16, BUSY GPIO18, write clock 20 MHz; EPD rail `EPD_3V3_L3B` enabled through IOE1 `PYB_EPD_EN`; `render_white()` for the initial frame and alternating white/logo frames thereafter |
| Frontlight | PM1 PWM set to 100% before `BOARD_OK`; no frontlight change during a refresh window |
| Firmware log | `BOARD_OK BASELINE=l2-awake DISPLAY=ssd1677 ORIENTATION=portrait REFRESH=%s AREA=480x800 PERIOD_MS=60000 FRONTLIGHT_PERCENT=100` |

## Runtime Mode

The test prepares the shared awake L2 baseline, restores the display rail, initializes the display, and performs an initial white refresh. After the frontlight is set to 100%, it emits `BOARD_OK` and `TEST_READY`. It then waits for each measurement window and alternates a white frame with a logo frame. The reported refresh result is taken from a cursor covering one complete refresh event, not from the 60-second `WINDOW Avg`.

## Operating Modes

Select exactly one defaults file before building:

| `config_id` | Kconfig selection | Refresh mode |
|---|---|---|
| `display-epd-fastest` | `CONFIG_M5PM_DISPLAY_EPD_FASTEST=y` | `epd_fastest` |
| `display-epd-fast` | `CONFIG_M5PM_DISPLAY_EPD_FAST=y` | `epd_fast` |
| `display-epd-quality` | `CONFIG_M5PM_DISPLAY_EPD_QUALITY=y` | `epd_quality` |

Example selection commands:

```powershell
Remove-Item -Force sdkconfig -ErrorAction SilentlyContinue
$env:SDKCONFIG_DEFAULTS = "sdkconfig.defaults;sdkconfig.display-epd-fastest.defaults"
idf.py reconfigure
```

Replace the override file name with `sdkconfig.display-epd-fast.defaults` or `sdkconfig.display-epd-quality.defaults` for the other modes. In Bash, use `rm -f sdkconfig` and `export SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.display-epd-fast.defaults"`.

## Measured Current

The refresh result below is the `CURSOR Avg` measured over one complete refresh window. The duration and charge are included to distinguish this event measurement from a steady-state average.

| Mode | 3.7 V current | 3.7 V duration | 3.7 V charge | 4.2 V current | 4.2 V duration | 4.2 V charge |
|---|---:|---:|---:|---:|---:|---:|
| `display-epd-fastest` | 110.39 mA | 1.192452 s | 36.56481 uAh | 105.21 mA | 1.192452 s | 34.84959 uAh |
| `display-epd-fast` | 110.13 mA | 1.375295 s | 42.07079 uAh | 104.92 mA | 1.375295 s | 40.08100 uAh |
| `display-epd-quality` | 98.60 mA | 4.557625 s | 124.83403 uAh | 93.78 mA | 4.551928 s | 118.57186 uAh |

See the [power measurement graph index](../../docs/power-graphs/README.md).

## Configuration

| Setting | Value |
|---|---|
| Target | ESP32-S3 |
| Test ID | `display_refresh_modes` |
| Config ID | `display-epd-fastest`, `display-epd-fast` or `display-epd-quality` |
| Measurement strategy | One complete refresh event selected with `CURSOR`; the 60-second `WINDOW Avg` is not used for the mode comparison |

## Build and Flash

From the repository root, run `python fetch_repos.py check`; if components are missing, run `python fetch_repos.py fetch --skip-existing`. Then select a defaults file and build from this directory:

```powershell
Remove-Item -Force sdkconfig -ErrorAction SilentlyContinue
$env:SDKCONFIG_DEFAULTS = "sdkconfig.defaults;sdkconfig.display-epd-fastest.defaults"
idf.py set-target esp32s3
idf.py build
idf.py -p <PORT> flash
idf.py -p <PORT> monitor
```

Use the serial monitor only to confirm `TEST_START`, `BOARD_OK`, and `TEST_READY`. Disconnect USB Type-C before the formal current capture.

## Expected Validation

The log must identify the selected `REFRESH` value and `FRONTLIGHT_PERCENT=100` before `TEST_READY`. During capture, `frame_select` identifies the frame associated with each refresh event.
