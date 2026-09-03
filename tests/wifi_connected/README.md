# Wi-Fi Connected

[English](README.md) | [简体中文](README_CN.md)

## Purpose

This firmware measures whole-board input current while the ESP32-S3 station remains connected to the configured access point with Wi-Fi power save disabled and no application traffic.

## Enabled Modules

- `m5pm_wifi`: station connection and power-save configuration
- `m5pm_phase_test`: shared awake L2 baseline and measurement timing
- `m5pm_test_support`: test markers and safe hold state

## Module Configuration

| Module | Configuration |
|---|---|
| ESP32-S3 Wi-Fi | Station mode; `PowerSave::none`; no application traffic and no explicit scan |
| Credentials | `CONFIG_M5PM_WIFI_SSID` and `CONFIG_M5PM_WIFI_PASSWORD` from the project Kconfig menu |
| Connection timeout | `CONFIG_M5PM_WIFI_CONNECT_TIMEOUT_MS`, default 15,000 ms |
| Board baseline | PM1/IOE1 awake L2 configuration; display, touch, IMU, audio, motor and external 5 V loads disabled |
| Baseline | Shared awake L2 preparation before Wi-Fi is started |
| Firmware log | `BOARD_OK BASELINE=l2-awake WIFI=connected MODE=sta PS=none APP_TRAFFIC=0 EXPLICIT_SCAN=0` |

## Runtime Mode

The test prepares the awake L2 baseline, connects the station, emits `BOARD_OK` and `TEST_READY`, and waits through the measurement window. It verifies that the station is still connected before entering the safe hold state.

## Network Configuration

| Setting | Value |
|---|---|
| Wi-Fi mode | Station (`MODE=sta`) |
| Power save | Disabled (`PS=none`) |
| Application traffic | None (`APP_TRAFFIC=0`) |
| Scanning | No explicit scan (`EXPLICIT_SCAN=0`) |

## Measured Current

The current shown in the measurement results is the `WINDOW Avg` from the measurement record.

| Voltage | Current |
|---|---:|
| 3.7 V | 102.18 mA |
| 4.2 V | 93.97 mA |

See the [power measurement graph index](../../docs/power-graphs/README.md).

## Configuration

| Setting | Value |
|---|---|
| Target | ESP32-S3 |
| Test ID | `wifi_connected` |
| Config ID | `wifi_connected` |

Before building, run `idf.py menuconfig`, search for `M5PM_WIFI_SSID`, and set the **M5PaperMono Wi-Fi test** values `M5PM_WIFI_SSID` and `M5PM_WIFI_PASSWORD`. The default `M5PM_WIFI_CONNECT_TIMEOUT_MS` is 15,000 ms and is used for the connection check.

## Build and Flash

From the repository root, run `python fetch_repos.py check`; if components are missing, run `python fetch_repos.py fetch --skip-existing`. Configure the Wi-Fi credentials required by `m5pm_wifi`, then build from this directory:

```bash
idf.py set-target esp32s3
idf.py build
idf.py -p <PORT> flash
idf.py -p <PORT> monitor
```

Confirm `TEST_START`, the connection result, `BOARD_OK`, and `TEST_READY` over USB Type-C. Disconnect USB Type-C before current measurement.

## Expected Validation

The `BOARD_OK` line must report `PS=none`, `APP_TRAFFIC=0`, and `EXPLICIT_SCAN=0`. The connection check must remain true through the measurement window.
