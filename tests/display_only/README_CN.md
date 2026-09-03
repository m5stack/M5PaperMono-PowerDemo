# 显示负载

[English](README.md) | [简体中文](README_CN.md)

## 目的

本固件在 SSD1677 保持静态画面、前光为 100% 的状态下，测量 M5PaperMono 的整板输入电流。

## 启用模块

- `m5pm_test_support`：测试标记和安全保持状态
- `m5pm_phase_test`：awake L2 基线和测量计时
- `m5pm_power` 与 `m5pm_display`：SSD1677 电源、quality 刷新和 PM1 前光

## 模块配置

| 模块 | 配置 |
|---|---|
| 显示 | SSD1677 四灰度面板；portrait 480x800；SPI2 mode 0，SCLK GPIO15、MOSI GPIO14、DC GPIO17、CS GPIO16、BUSY GPIO18；写入时钟 20 MHz；通过 IOE1 `PYB_EPD_EN` 使能 EPD 电源轨 |
| 电源控制 | PM1 地址 `0x6E`、IOE1 地址 `0x4F`，均使用共享 100 kHz I2C 总线 |
| 画面 | 使用 `epd_quality` 渲染一次内置 logo PNG；禁用自动显示 |
| 前光 | PM1 GPIO3（`PYG3_BL_PWM`）的 PWM 通道 0，频率 5 kHz，占空比 100%；由 PM1 控制的驱动器使能 `BL_15V_L3B` 背光电源轨 |
| 刷新循环 | 初始 logo 刷新一次；禁用周期刷新 |
| 测试状态 | ESP32-S3 唤醒并保持静态显示帧 |
| 固件日志 | `BOARD_OK BASELINE=l2-awake DISPLAY=ssd1677 ORIENTATION=portrait REFRESH=quality AREA=480x800 FRONTLIGHT_PERCENT=100` |

## 运行模式

测试恢复显示电源，初始化 SSD1677，执行一次 quality 模式 logo 刷新，将前光设置为 100%，输出 `BOARD_OK` 和 `TEST_READY`，并在测量窗口内保持静态帧。

## 测量电流

测量结果中的电流为测量记录中的 `WINDOW Avg`。

- 3.7 V：89.90 mA
- 4.2 V：85.30 mA

参见[功耗测量曲线索引](../../docs/power-graphs/README_CN.md)。

## 配置

| 设置 | 数值 |
|---|---|
| 目标 | ESP32-S3 |
| Flash | 16 MB |
| PSRAM | OPI，80 MHz |
| CPU | 240 MHz |
| I2C | 使用时为 100 kHz |
| Test ID | `display_only` |
| Config ID | `display_only` |

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
