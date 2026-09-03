# Power Test Projects

[English](README.md) | [简体中文](README_CN.md)

Each directory is an independent ESP-IDF 5.5.5 project for one M5PaperMono operating state or workload. The directory name is the test identifier used by the firmware and its measurement evidence.

| Test group | Directories |
|---|---|
| Low-power states | `l0_shutdown`, `l1_standby`, `l2_deep_sleep` |
| Wake sources | `imu_wake_deep_sleep`, `imu_wake_shutdown`, `button_wake_deep_sleep`, `rtc_alarm_wake_deep_sleep`, `rtc_alarm_wake_shutdown`, `rtc_timer_wake_deep_sleep`, `rtc_timer_wake_shutdown` |
| Operating workloads | `normal_active`, `display_only`, `display_refresh_modes`, `frontlight_levels`, `imu_sampling`, `full_load`, `nfc_working_current`, `lora_periodic_transmit`, `lora_rx_idle`, `lora_sleep` |
| Wi-Fi workloads | `wifi_connected`, `wifi_light_sleep`, `wifi_periodic_traffic` |

Build from the selected directory. Before formal power measurement, disconnect USB Type-C and do not run the serial monitor. Each test report links to its corresponding measurement graph directory.
