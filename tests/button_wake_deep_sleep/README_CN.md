# 按键深度睡眠唤醒

[English](README.md) | [简体中文](README_CN.md)

## 目的

本固件将 Key1 和 Key2 配置为 ESP32-S3 EXT1 唤醒源，并测量设备进入 Deep Sleep 后等待按键唤醒期间的整板输入电流。

## 启用模块

- `m5pm_test_support`：测试标记和保持的唤醒验证
- `m5pm_board`、`m5pm_power`：共享 I2C 和 L2 按键唤醒准备
- `m5pm_rtc_wakeup`、`m5pm_imu_wakeup`：保持的 RTC 和 IMU 状态读取

## 模块配置

| 模块 | 配置 |
|---|---|
| ESP32-S3 按键 | Key1 和 Key2 配置为带上拉的 GPIO EXT1 唤醒输入 |
| PM1/IOE1 | 初始化 PM1 和 IOE1 L2 链路；不接受 PM1 唤醒、GPIO 或系统中断 |
| 唤醒验证 | ESP32 唤醒原因为 EXT1 且存在 Key1 或 Key2 位；同时报告保持的 RTC/IMU 状态 |
| 固件日志 | `BOARD_OK PM1=1 IOE1=1 RTC=1 WAKE=button` 和 `WAKE_ARMED source=button path=esp_ext1` |

## 运行模式

测试读取保持的状态，准备 L2 按键唤醒链路，配置 Key1 和 Key2，输出 `BOARD_OK`、`WAKE_ARMED` 和 `TEST_READY`，等待 3000 ms 后进入 Deep Sleep。唤醒后验证 EXT1 和对应按键位，并要求 PM1 唤醒及中断状态为空。

## 测量电流

测量结果中的电流为测量记录中的 `WINDOW Avg`。

- 3.7 V：101.95 uA
- 4.2 V：86.89 uA

参见[功耗测量曲线索引](../../docs/power-graphs/README_CN.md)。

## 配置

| 设置 | 数值 |
|---|---|
| 目标 | ESP32-S3 |
| Flash | 16 MB |
| PSRAM | OPI，80 MHz |
| CPU | 240 MHz |
| I2C | 使用时为 100 kHz |
| Test ID | `button_wake_deep_sleep` |
| Config ID | `button_wake_deep_sleep` |

## 构建与烧录

在本目录使用 ESP-IDF 5.5.5：

```bash
idf.py set-target esp32s3
idf.py build
idf.py -p <PORT> flash
idf.py -p <PORT> monitor
```

正式功耗测量前断开 USB Type-C，测量期间不要运行串口监视器。

## 预期验证

确认 `TEST_START`、测试对应的 `BOARD_OK` 字段和 `TEST_READY`。初始化或验证无法继续时，固件会输出失败标记。
