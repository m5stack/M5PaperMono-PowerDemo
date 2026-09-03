# RTC Alarm 深度睡眠唤醒

[English](README.md) | [简体中文](README_CN.md)

## 目的

本固件用于测量 RX8130CE RTC Alarm 唤醒 ESP32-S3 Deep Sleep 状态时的整板输入电流。

## 启用模块

- `m5pm_test_support`：测试标记和保持的唤醒验证
- `m5pm_board` 与 `m5pm_rtc_wakeup`：共享 I2C 和 RX8130CE 控制
- `m5pm_rtc_test` 与 `m5pm_power`：通用 RTC 唤醒流程和 ESP32-S3 Deep Sleep 进入

## 模块配置

| 模块 | 配置 |
|---|---|
| RX8130CE | 地址 `0x32`、100 kHz I2C；基线阶段清除标志，并在配置前禁用 Timer / update 中断 |
| 日历 Alarm | 寄存器 `0x17` 的分钟匹配值为 `0x02`；小时和星期字段写入 `0x80`（禁用）；启用 Alarm 中断 |
| PM1/IOE1 | PM1 GPIO0 -> IOE1 聚合 GPIO1 -> ESP32-S3 EXT0；IOE1 为 Deep Sleep 保持供电 |
| 唤醒验证 | Alarm 标志置位、冲突 Timer 标志清零、PM1 GPIO0 IRQ 存在、ESP32 唤醒原因为 EXT0 |
| 固件日志 | `BOARD_OK PM1=1 IOE1=1 RTC=1 WAKE=rtc_alarm` 和 `WAKE_ARMED source=rtc_alarm interval=120s path=pm1_gpio0+aggregate_gpio1+esp_ext0` |

## 运行模式

通用 RTC 测试读取保持的 RTC 和 PM1 标志，准备 L2 RTC Alarm 唤醒链路，配置 120 s Alarm，释放总线，输出 `BOARD_OK`、`WAKE_ARMED` 和 `TEST_READY`，等待 3000 ms 后进入 Deep Sleep。唤醒后验证 RTC Alarm 链路，并拒绝冲突的 Timer 标志。

## 测量电流

测量结果中的电流为测量记录中的 `WINDOW Avg`。

- 3.7 V：76.05 uA
- 4.2 V：63.15 uA

参见[功耗测量曲线索引](../../docs/power-graphs/README_CN.md)。

## 配置

| 设置 | 数值 |
|---|---|
| 目标 | ESP32-S3 |
| Flash | 16 MB |
| PSRAM | OPI，80 MHz |
| CPU | 240 MHz |
| I2C | 使用时为 100 kHz |
| Test ID | `rtc_alarm_wake_deep_sleep` |
| Config ID | `rtc_alarm_wake_deep_sleep` |

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
