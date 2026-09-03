# Wi-Fi Periodic Traffic

[English](README.md) | [简体中文](README_CN.md)

## Purpose

This firmware measures whole-board input current while a connected ESP32-S3 station sends fixed-period ICMP Echo traffic.

## Enabled Modules

- `m5pm_wifi`: station connection and network helper
- `m5pm_phase_test`: shared awake L2 baseline and measurement timing
- `m5pm_test_support`: test markers and safe hold state

## Module Configuration

| Module | Configuration |
|---|---|
| Wi-Fi | Station mode, connected, power save disabled; no explicit scan; credentials use `CONFIG_M5PM_WIFI_SSID` and `CONFIG_M5PM_WIFI_PASSWORD` |
| Connection timeout | `CONFIG_M5PM_WIFI_CONNECT_TIMEOUT_MS`, default 15,000 ms |
| ICMP traffic | `CONFIG_M5PM_WIFI_PING_TARGET`; fixed 64-byte payload every 1000 ms using the configured LAN target; no retries in the test workload |
| Baseline | Shared awake L2 preparation before Wi-Fi is started |
| Firmware log | `BOARD_OK BASELINE=l2-awake WIFI=connected MODE=sta PS=none ICMP_PAYLOAD_BYTES=64 PERIOD_MS=1000 TARGET=fixed_lan ACK_POLICY=none RETRY_WORKLOAD=0 EXPLICIT_SCAN=0` |

## Runtime Mode

The test prepares the awake L2 baseline, connects Wi-Fi without power save, starts the fixed ping task, emits `BOARD_OK` and `TEST_READY`, and waits through the measurement window. It verifies the station remains connected before safe hold.

## Network Configuration

| Setting | Value |
|---|---|
| Wi-Fi mode | Station (`MODE=sta`) |
| Power save | Disabled (`PS=none`) |
| ICMP payload | 64 bytes |
| Period | 1000 ms |
| Target | Fixed LAN target (`TARGET=fixed_lan`) |
| ACK and retries | `ACK_POLICY=none`; `RETRY_WORKLOAD=0` |

## Measured Current

The current shown in the measurement results is the `WINDOW Avg` from the measurement record.

| Voltage | Current |
|---|---:|
| 3.7 V | 102.08 mA |
| 4.2 V | 94.21 mA |

See the [power measurement graph index](../../docs/power-graphs/README.md).

## Configuration

| Setting | Value |
|---|---|
| Target | ESP32-S3 |
| Test ID | `wifi_periodic_traffic` |
| Config ID | `wifi_periodic_traffic` |

Before building, run `idf.py menuconfig`, search for `M5PM_WIFI_SSID`, and set `M5PM_WIFI_SSID`, `M5PM_WIFI_PASSWORD`, and `M5PM_WIFI_PING_TARGET` in the **M5PaperMono Wi-Fi test** menu. Set `M5PM_WIFI_CONNECT_TIMEOUT_MS` as needed; its default is 15,000 ms. The ping target must be a fixed IPv4 host on the same LAN that responds to ICMP Echo requests.

## Build and Flash

From the repository root, run `python fetch_repos.py check`; if components are missing, run `python fetch_repos.py fetch --skip-existing`. Configure the Wi-Fi credentials and fixed LAN target required by `m5pm_wifi`, then build here:

```bash
idf.py set-target esp32s3
idf.py build
idf.py -p <PORT> flash
idf.py -p <PORT> monitor
```

Confirm `TEST_START`, `BOARD_OK`, and `TEST_READY` over USB Type-C. Disconnect USB Type-C before current measurement.

## Expected Validation

The log must report the 64-byte payload, 1000 ms period, fixed-LAN target, and disabled power save before `TEST_READY`.
