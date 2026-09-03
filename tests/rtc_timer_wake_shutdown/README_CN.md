# RTC Timer 关机唤醒

[English](README.md) | [简体中文](README_CN.md)

## 目的

本固件用于测量 RX8130CE RTC Timer 唤醒 PM1 shutdown 状态时的整板输入电流。

## 启用模块

- `m5pm_test_support`：测试标记和保持的唤醒验证
- `m5pm_board` 与 `m5pm_rtc_wakeup`：共享 I2C 和 RX8130CE 控制
- `m5pm_rtc_test` 与 `m5pm_power`：通用 RTC 唤醒流程和 PM1 shutdown 进入

## 模块配置

| 模块 | 配置 |
|---|---|
| RX8130CE | 地址 `0x32`、100 kHz I2C；基线阶段清除标志，并在配置前禁用 Alarm / Timer 中断 |
| 倒计时 Timer | 计数寄存器 `0x1A-0x1B` 写入 120 s（`0x0078`）；扩展寄存器启用 Timer 并选择 `0x02`（1 Hz）；启用 Timer 中断 |
| PM1 | PM1 进入 shutdown 时保持 GPIO0 外部唤醒；shutdown 链路不使用 IOE1 |
| 唤醒验证 | Timer 标志置位并存在 PM1 外部唤醒证据 |
| 固件日志 | `BOARD_OK PM1=1 IOE1=0 RTC=1 WAKE=rtc_timer` 和 `WAKE_ARMED source=rtc_timer interval=120s path=pm1_gpio0+shutdown` |

## 运行模式

通用 RTC 测试读取保持的标志，准备 PM1 GPIO0 shutdown 唤醒链路，配置 120 s Timer，释放总线，输出 `BOARD_OK`、`WAKE_ARMED` 和 `TEST_READY`，等待 3000 ms 后请求 PM1 shutdown。重启后验证 PM1 外部唤醒证据和 Timer 标志。

## 测量电流

测量结果中的电流为测量记录中的 `WINDOW Avg`。

- 3.7 V：18.26 uA
- 4.2 V：21.27 uA

参见[功耗测量曲线索引](../../docs/power-graphs/README_CN.md)。

## 配置

| 设置 | 数值 |
|---|---|
| 目标 | ESP32-S3 |
| Flash | 16 MB |
| PSRAM | OPI，80 MHz |
| CPU | 240 MHz |
| I2C | 使用时为 100 kHz |
| Test ID | `rtc_timer_wake_shutdown` |
| Config ID | `rtc_timer_wake_shutdown` |

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
