# Wi-Fi Light Sleep

[English](README.md) | [简体中文](README_CN.md)

## Purpose

This firmware measures whole-board input current with a connected ESP32-S3 station using maximum modem power save and automatic Light Sleep.

## Enabled Modules

- `m5pm_wifi`: station connection and maximum modem power-save configuration
- ESP-IDF power-management: dynamic frequency scaling and Light Sleep
- `m5pm_phase_test` and `m5pm_test_support`: baseline, markers and measurement timing

## Module Configuration

| Module | Configuration |
|---|---|
| Wi-Fi | Station mode, connected, `PS=max_modem`; no application traffic or explicit scan; credentials use `CONFIG_M5PM_WIFI_SSID` and `CONFIG_M5PM_WIFI_PASSWORD` |
| Connection timeout | `CONFIG_M5PM_WIFI_CONNECT_TIMEOUT_MS`, default 15,000 ms |
| Power management | `CONFIG_PM_ENABLE=1`, `CONFIG_FREERTOS_USE_TICKLESS_IDLE=1`, Light Sleep enabled |
| CPU frequency | Maximum 240 MHz; minimum 40 MHz, verified with `esp_pm_get_configuration()` |
| Firmware log | `BOARD_OK BASELINE=l2-awake WIFI=connected MODE=sta PS=max_modem APP_TRAFFIC=0 EXPLICIT_SCAN=0 AUTO_LIGHT_SLEEP=1 TICKLESS=1 CPU_MIN_MHZ=40 CPU_MAX_MHZ=240` |

## Runtime Mode

The test prepares the awake L2 baseline, connects Wi-Fi with maximum modem power save, applies and reads back the ESP-IDF power-management configuration, emits `BOARD_OK` and `TEST_READY`, and waits through the measurement window. It verifies that the station remains connected before safe hold.

## Network Configuration

| Setting | Value |
|---|---|
| Wi-Fi mode | Station (`MODE=sta`) |
| Power save | Maximum modem power save (`PS=max_modem`) |
| Automatic Light Sleep | Enabled (`AUTO_LIGHT_SLEEP=1`) |
| Tickless idle | Enabled (`TICKLESS=1`) |
| CPU frequency | 40-240 MHz |
| Application traffic | None (`APP_TRAFFIC=0`) |

## Measured Current

The current shown in the measurement results is the `WINDOW Avg` from the measurement record.

| Voltage | Current |
|---|---:|
| 3.7 V | 4.28 mA |
| 4.2 V | 5.31 mA |

See the [power measurement graph index](../../docs/power-graphs/README.md).

## Configuration

| Setting | Value |
|---|---|
| Target | ESP32-S3 |
| Test ID | `wifi_light_sleep` |
| Config ID | `wifi_light_sleep` |
| Required build assertions | `CONFIG_PM_ENABLE=1`; `CONFIG_FREERTOS_USE_TICKLESS_IDLE=1` |

Before building, run `idf.py menuconfig`, search for `M5PM_WIFI_SSID`, and set `M5PM_WIFI_SSID` and `M5PM_WIFI_PASSWORD` in the **M5PaperMono Wi-Fi test** menu. The default `M5PM_WIFI_CONNECT_TIMEOUT_MS` is 15,000 ms. Keep the power-management options listed above enabled.

## Build and Flash

From the repository root, run `python fetch_repos.py check`; if components are missing, run `python fetch_repos.py fetch --skip-existing`. Then build from this directory:

```bash
idf.py set-target esp32s3
idf.py build
idf.py -p <PORT> flash
idf.py -p <PORT> monitor
```

Confirm `TEST_START`, the power-management readback, `BOARD_OK`, and `TEST_READY` over USB Type-C. Disconnect USB Type-C before current measurement.

## Expected Validation

The log must report `PS=max_modem`, `AUTO_LIGHT_SLEEP=1`, `TICKLESS=1`, and the verified 40-240 MHz range before `TEST_READY`.
