# M5PaperMono Power Demo

[English](README.md) | [简体中文](README_CN.md)

## Overview

This repository contains independent ESP-IDF firmware projects for measuring the power consumption of production M5PaperMono hardware. Each test is built and flashed separately, starts automatically after boot, and uses shared board-support modules.

The project is based on ESP-IDF 5.5.5. Build, flash, serial validation, and current measurement are manual operations.

## Power Test Results

Power consumption is represented by the average whole-board input current. Unless noted in the linked report, each value is the selected `WINDOW Avg` from an approximately 60-second measurement window. Display refresh and single-transmission entries use the measurement field described in their report.

Device states are abbreviated in this table; use the details links for complete configurations.

For multi-mode rows, slash-separated values follow the mode order documented in the linked report. The order is `low_power` / `reference` for IMU wake, `fastest` / `fast` / `quality` for display refresh, and 0% / 50% / 100% for frontlight.

| Test scenario | Device / operating state | Avg. current at 3.7 V | Avg. current at 4.2 V | Details |
|---|---|---:|---:|---|
| L0 shutdown | PM1: L0 shutdown<br>ESP32-S3: not initialized | 17.81 uA | 20.75 uA | [Details](tests/l0_shutdown/README.md) |
| L1 standby | PM1: L1 standby<br>BMI270: low-power state; NFC rail off | 26.07 uA | 29.09 uA | [Details](tests/l1_standby/README.md) |
| L2 deep sleep | ESP32-S3: Deep Sleep<br>PM1/IOE1: L2 state | 129.08 uA | 81.06 uA | [Details](tests/l2_deep_sleep/README.md) |
| IMU wake from deep sleep | ESP32-S3: Deep Sleep<br>BMI270: any-motion wake | 726.19 uA (low_power)<br>1.38 mA (reference) | 717.64 uA (low_power)<br>1.36 mA (reference) | [Details](tests/imu_wake_deep_sleep/README.md) |
| IMU wake from shutdown | PM1: shutdown<br>BMI270: any-motion wake | 42.72 uA (low_power)<br>692.80 uA (reference) | 45.82 uA (low_power)<br>696.20 uA (reference) | [Details](tests/imu_wake_shutdown/README.md) |
| Button wake from deep sleep | ESP32-S3: Deep Sleep<br>Key1/Key2: EXT1 wake | 101.95 uA | 86.89 uA | [Details](tests/button_wake_deep_sleep/README.md) |
| RTC Alarm wake from deep sleep | ESP32-S3: Deep Sleep<br>RX8130CE: Alarm armed | 76.05 uA | 63.15 uA | [Details](tests/rtc_alarm_wake_deep_sleep/README.md) |
| RTC Alarm wake from shutdown | PM1: shutdown<br>RX8130CE: Alarm armed | 18.26 uA | 21.17 uA | [Details](tests/rtc_alarm_wake_shutdown/README.md) |
| RTC Timer wake from deep sleep | ESP32-S3: Deep Sleep<br>RX8130CE: 120 s Timer armed | 661.29 uA | 671.19 uA | [Details](tests/rtc_timer_wake_deep_sleep/README.md) |
| RTC Timer wake from shutdown | PM1: shutdown<br>RX8130CE: 120 s Timer armed | 18.26 uA | 21.27 uA | [Details](tests/rtc_timer_wake_shutdown/README.md) |
| Normal operation | ESP32-S3: active<br>SSD1677: quality refresh every 5 s; 50% frontlight | 76.80 mA | 72.34 mA | [Details](tests/normal_active/README.md) |
| Display workload | ESP32-S3: active<br>SSD1677: static frame, 100% frontlight | 89.90 mA | 85.30 mA | [Details](tests/display_only/README.md) |
| Display refresh modes | SSD1677: complete refresh<br>Modes: fastest, fast, quality | 110.39 / 110.13 / 98.60 mA | 105.21 / 104.92 / 93.78 mA | [Details](tests/display_refresh_modes/README.md) |
| Frontlight levels | SSD1677: static frame<br>Frontlight: 0%, 50% or 100% | 31.55 / 61.28 / 89.98 mA | 29.13 / 58.23 / 85.35 mA | [Details](tests/frontlight_levels/README.md) |
| IMU sampling | ESP32-S3: active<br>BMI270: accelerometer + gyroscope | 31.63 mA | 28.99 mA | [Details](tests/imu_sampling/README.md) |
| Full load | ESP32-S3: active<br>SSD1677, SX1262, NFC, Wi-Fi and frontlight | 187.32 mA | 176.60 mA | [Details](tests/full_load/README.md) |
| NFC power down | ST25R3916: power-down state | 34.35 mA | 32.39 mA | [Details](tests/nfc_working_current/README.md) |
| NFC-A polling | ST25R3916: NFC-A polling | 109.31 mA | 103.21 mA | [Details](tests/nfc_working_current/README.md) |
| LoRa periodic transmit | SX1262: periodic TX | 61.25 mA | 59.50 mA | [Details](tests/lora_periodic_transmit/README.md) |
| LoRa single transmission | SX1262: single TX | 96.55 mA | 93.52 mA | [Details](tests/lora_periodic_transmit/README.md) |
| LoRa RX idle | SX1262: continuous RX | 42.02 mA | 39.89 mA | [Details](tests/lora_rx_idle/README.md) |
| LoRa sleep | SX1262: sleep state | 31.71 mA | 29.68 mA | [Details](tests/lora_sleep/README.md) |
| Wi-Fi connected | ESP32-S3: station connected<br>Power save: disabled | 102.18 mA | 93.97 mA | [Details](tests/wifi_connected/README.md) |
| Wi-Fi Light Sleep | ESP32-S3: automatic Light Sleep<br>Wi-Fi: maximum modem power save | 4.28 mA | 5.31 mA | [Details](tests/wifi_light_sleep/README.md) |
| Wi-Fi periodic traffic | ESP32-S3: station connected<br>ICMP periodic traffic | 102.08 mA | 94.21 mA | [Details](tests/wifi_periodic_traffic/README.md) |

## Measurement Graphs

All published captures are indexed in [Power Measurement Graphs](docs/power-graphs/README.md). The index contains the published images for each voltage and test scenario.

## Repository Layout

```text
components/  Fetched third-party components
modules/     Shared M5PaperMono board and test-support components
tests/       Independent ESP-IDF power-test projects
docs/        Published measurement evidence
```

## Build and Flash

Install ESP-IDF 5.5.5 and activate its export environment before running any `idf.py` command. Python 3 and Git are required by the dependency script. From the repository root, verify the locked component destinations and then fetch missing components:

```bash
python fetch_repos.py check
```

If the check reports missing components, run `python fetch_repos.py fetch --skip-existing` to install only missing destinations. If it reports a version mismatch, use `python fetch_repos.py fetch --replace` after confirming that the component tree should be regenerated. The script creates `_component_backups/` before replacement. `fetch_repos.py` reads `repos.json`, fetches the exact Git commits or registry hashes, applies the patches listed in that file, and installs the components under the paths declared there. A registry component must be fetched from an ESP-IDF shell with the ESP-IDF Python environment available.

Build and flash one test from its own directory:

```bash
cd tests/l0_shutdown
idf.py set-target esp32s3
idf.py build
idf.py -p <PORT> flash
idf.py -p <PORT> monitor
```

Each test directory contains a complete `sdkconfig.defaults`. Multi-mode tests also provide sparse mode overrides. Do not replace the base defaults file with an override. Before the first configuration, or when changing modes, remove the generated `sdkconfig` and set `SDKCONFIG_DEFAULTS` to the base file followed by the selected override, separated by a semicolon. For example:

```powershell
$env:SDKCONFIG_DEFAULTS = "sdkconfig.defaults;sdkconfig.display-epd-fast.defaults"
idf.py reconfigure
```

Use the equivalent `export SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.display-epd-fast.defaults"` form in Bash. Generated `build/`, `sdkconfig`, and `dependencies.lock` files are local artifacts. USB Type-C is used for flashing and serial validation only; disconnect it before formal current measurement.

## Measurement Method

1. Connect USB Type-C to flash the firmware and confirm `TEST_START`, `BOARD_OK`, and `TEST_READY` through the serial monitor.
2. Keep the same firmware and configuration, disconnect USB Type-C, and power the device only through the measurement setup.
3. Measure at nominal 3.7 V and 4.2 V after the current stabilizes. Publish the selected capture's approximately 60-second `WINDOW Avg`.

The firmware does not detect the power source or switch between validation and measurement modes. L0 initializes only the shared I2C bus and M5PM1; other peripherals are intentionally not initialized.

## License

Project-owned source code is released under the [MIT License](LICENSE). Third-party components retain the licenses listed in [Third-Party Notices](THIRD_PARTY_NOTICES.md).

## Dependencies

Locked component revisions, sources and licenses are listed in [Third-Party Notices](THIRD_PARTY_NOTICES.md). Project-specific adaptations are recorded in `patches/`.
