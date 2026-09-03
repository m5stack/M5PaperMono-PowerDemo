# 正常工作

[English](README.md) | [简体中文](README_CN.md)

## 目的

本固件以 50% 前光和每 5 s 一次的 SSD1677 quality 刷新保持正常工作循环，并测量该状态下的整板输入电流。

## 启用模块

- `m5pm_test_support`：测试标记和安全保持状态
- `m5pm_phase_test`：awake L2 基线和 60 秒计时
- `m5pm_power` 与 `m5pm_display`：SSD1677 电源、quality 刷新和 PM1 前光

## 模块配置

| 模块 | 配置 |
|---|---|
| 显示 | SSD1677，portrait，480x800，quality 模式；使用内置 logo 帧；通过 IOE1 `PYB_EPD_EN` 使能 EPD 电源轨 |
| 前光 | PM1 GPIO3（`PYG3_BL_PWM`）的 5 kHz PWM，占空比 50%；由 PM1 控制的驱动器使能 `BL_15V_L3B` 背光电源轨 |
| 电源控制 | PM1 地址 `0x6E`、IOE1 地址 `0x4F`，均使用共享 100 kHz I2C 总线 |
| 刷新循环 | 每 5000 ms 全屏刷新 logo；`TEST_READY` 前先渲染第一帧 |
| 关闭的负载 | Wi-Fi、NFC 和 LoRa 均关闭 |
| 测试状态 | ESP32-S3 运行空闲帧循环 |
| 固件日志 | `BOARD_OK BASELINE=l2-awake DISPLAY=ssd1677 ORIENTATION=portrait REFRESH=quality FULL_SCREEN=1 REFRESH_PERIOD_MS=5000 REFRESH_COUNT_INITIAL=1 AREA=480x800 FRONTLIGHT_PERCENT=50 WIFI=off NFC=off LORA=off WORKLOAD=idle-frame-loop` |

## 运行模式

测试恢复显示电源，渲染第一帧 logo，将前光设置为 50%，输出 `BOARD_OK` 和 `TEST_READY`，随后每 5 s 刷新一次 logo。60 s 后输出测量窗口标记并继续保持该状态。

## 测量电流

测量结果中的电流为测量记录中的 `WINDOW Avg`。

- 3.7 V：76.80 mA
- 4.2 V：72.34 mA

参见[功耗测量曲线索引](../../docs/power-graphs/README_CN.md)。

## 配置

| 设置 | 数值 |
|---|---|
| 目标 | ESP32-S3 |
| Flash | 16 MB |
| PSRAM | OPI，80 MHz |
| CPU | 240 MHz |
| I2C | 使用时为 100 kHz |
| Test ID | `normal_active` |
| Config ID | `normal_active` |

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
