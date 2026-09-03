# Full Load

[English](README.md) | [简体中文](README_CN.md)

## Purpose

This firmware measures the M5PaperMono whole-board input current while the display, frontlight, LoRa, NFC, and Wi-Fi workloads run together. The workload is time-bounded and can be stopped by the board controls described below.

## Enabled Modules

| Module | Configuration and runtime behavior |
|---|---|
| `m5pm_test_support` | Initializes the test context, emits common markers, reports the final status, and enters the safe idle state. |
| `m5pm_phase_test` | Establishes the validated awake L2 baseline and configures the Key1/Key2 stop inputs. |
| `m5pm_power` | Restores the display, LoRa, and NFC rails; sets PM1 frontlight PWM; and powers down LoRa during cleanup. |
| `m5pm_display` | Drives the SSD1677 in portrait orientation at 480 x 800 and renders the full-load status frame with `epd_fastest`. |
| `m5pm_lora` / RadioLib | Initializes the SX1262 at 868 MHz, SF12, and 22 dBm, then transmits one `PM_<count>` payload per loop. |
| `M5UnitUnified` / `M5UnitUnifiedNFC` | Initializes the ST25R3916 reader on the native board I2C bus in NFC-A polling mode and counts detected PICCs. |
| ESP-IDF Wi-Fi | Starts an ESP32-S3 station with the configured SSID and password and waits up to 8 seconds for an IP address. |
| Board touch input | Polls the touch controller and stops the workload when a touch is detected in the title area (`y < 80`). |

## Module Configuration

| Setting | Value or source |
|---|---|
| Full-load safety gate | `CONFIG_M5PM_FULL_LOAD_ENABLE`; disabled by default and must be enabled explicitly in `menuconfig`. |
| Wi-Fi SSID | `CONFIG_M5PM_FULL_LOAD_WIFI_SSID`; required and supplied locally. |
| Wi-Fi password | `CONFIG_M5PM_FULL_LOAD_WIFI_PASSWORD`; configured locally; published files contain no credential values. |
| Maximum runtime | `CONFIG_M5PM_FULL_LOAD_RUNTIME_SECONDS`; 1 to 3600 seconds, default 600. |
| Display | SSD1677, portrait, 480 x 800, `epd_fastest`; EPD rail `EPD_3V3_L3B` enabled through IOE1 `PYB_EPD_EN`; frontlight is 100% during the workload and reduced to 50% during cleanup. |
| LoRa | SX1262, 868 MHz, SF12, 22 dBm; rail `3V3_L2_LoRa` enabled through PM1 GPIO2; one payload is sent per workload loop. |
| NFC | ST25R3916 at I2C address `0x50`, supplied by `3V3_L2` through IOE1 `PYB_NFC_EN`; NFC-A polling, IRQ disabled; detected PICCs are counted and deactivated. |
| Stop inputs | Key1, Key2, or a touch in the title area. |
| Test identifiers | `test_id=full_load`, `config_id=full_load`. |

The firmware emits the following board-state marker before `TEST_READY`:

```text
BOARD_OK FRONTLIGHT=100 DISPLAY_MODE=epd_fastest DISPLAY_ORIENTATION=portrait DISPLAY_AREA=480x800 LORA=868MHz/SF12/22dBm NFC=nfc_a_polling TOUCH=active
```

## Runtime Mode

The firmware initializes the test context, checks that a Wi-Fi SSID is configured locally, establishes the awake L2 baseline, and then restores the display, LoRa, and NFC rails. It starts the display, SX1262, ST25R3916, and Wi-Fi station before emitting `BOARD_OK` and `TEST_READY`. The loop transmits a LoRa payload, polls NFC-A, refreshes the status frame, and waits one second until the configured runtime expires or a stop input is received. Cleanup stops LoRa, reduces the frontlight to 50%, stops Wi-Fi, disables the NFC field, and enters safe idle.

## Safety Gate

The safety gate is disabled in `sdkconfig.defaults`. With the gate disabled, the firmware reports `TEST_FAIL detail=safety_gate_disabled` and does not start the high-load workload. Keep it disabled until the measurement setup, current limit, temperature limit, hard timeout, and stop controls have been reviewed for the intended hardware.

During a formal measurement, use the current and thermal limits defined for the measurement setup. Stop the test if any setup limit is reached. The firmware also enforces `CONFIG_M5PM_FULL_LOAD_RUNTIME_SECONDS` and the Key1, Key2, and title-area touch stop inputs.

## Measured Current

The values below are the `WINDOW Avg` values recorded by the measurement setup:

| Supply voltage | Window average |
|---|---:|
| 3.7 V | 187.32 mA |
| 4.2 V | 176.60 mA |

See the [power measurement graph index](../../docs/power-graphs/README.md).

## Hardware and Build Configuration

| Setting | Value |
|---|---|
| Target | ESP32-S3 |
| Flash | 16 MB |
| PSRAM | OPI, 80 MHz |
| CPU | 240 MHz |
| I2C | 100 kHz where used |
| ESP-IDF | 5.5.5 |

## Dependency Setup

Run these commands from the repository root in an ESP-IDF 5.5.5 shell. The ESP-IDF Python environment is required because `repos.json` includes the registry component `i2c_bus`.

```bash
python fetch_repos.py check
```

If the check reports missing components, run `python fetch_repos.py fetch --skip-existing` to install only missing destinations. If it reports a version mismatch, use `python fetch_repos.py fetch --replace` after confirming that the component tree should be regenerated. The script creates `_component_backups/` before replacement. The registry component requires the ESP-IDF Python environment.

## Configure, Build, and Flash

From this test directory:

```bash
idf.py set-target esp32s3
idf.py menuconfig
```

In `menuconfig`, open **M5PaperMono full-load safety** and enable **Enable the full-load test**. Open **M5PaperMono full-load network** and set:

- **Wi-Fi SSID** (`CONFIG_M5PM_FULL_LOAD_WIFI_SSID`)
- **Wi-Fi password** (`CONFIG_M5PM_FULL_LOAD_WIFI_PASSWORD`)
- **Maximum runtime (seconds)** (`CONFIG_M5PM_FULL_LOAD_RUNTIME_SECONDS`, 1-3600)

Save the configuration, then build and flash:

```bash
idf.py build
idf.py -p <PORT> flash
idf.py -p <PORT> monitor
```

The SSID and password are stored only in the local `sdkconfig`; published defaults, source files, issue reports, and documentation contain no credential values. Confirm the initialization markers in the serial monitor, then close the monitor and disconnect USB Type-C before formal current measurement.

## Expected Validation

Confirm `TEST_START`, the full-load `BOARD_OK` fields, `TEST_READY`, and `LOAD_RUNNING`. If initialization or a safety check cannot proceed, the firmware emits `TEST_FAIL` with a specific detail and enters safe idle.
