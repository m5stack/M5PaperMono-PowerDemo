# 功耗测试项目

[English](README.md) | [简体中文](README_CN.md)

每个目录都是一个独立的 ESP-IDF 5.5.5 项目，对应一个 M5PaperMono 工作状态或负载。目录名是固件和测量证据使用的测试标识。

| 测试分组 | 目录 |
|---|---|
| 低功耗状态 | `l0_shutdown`、`l1_standby`、`l2_deep_sleep` |
| 唤醒源 | `imu_wake_deep_sleep`、`imu_wake_shutdown`、`button_wake_deep_sleep`、`rtc_alarm_wake_deep_sleep`、`rtc_alarm_wake_shutdown`、`rtc_timer_wake_deep_sleep`、`rtc_timer_wake_shutdown` |
| 工作负载 | `normal_active`、`display_only`、`display_refresh_modes`、`frontlight_levels`、`imu_sampling`、`full_load`、`nfc_working_current`、`lora_periodic_transmit`、`lora_rx_idle`、`lora_sleep` |
| Wi-Fi 工作负载 | `wifi_connected`、`wifi_light_sleep`、`wifi_periodic_traffic` |

进入选定目录构建。正式功耗测量前断开 USB Type-C，测量期间不要运行串口监视器。每个测试报告都链接到对应的功耗曲线目录。
