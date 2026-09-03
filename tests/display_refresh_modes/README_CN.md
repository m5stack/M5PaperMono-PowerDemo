# 显示刷新模式

[English](README.md) | [简体中文](README_CN.md)

## 目的

本固件测量每种显示刷新模式下整板输入电流，以及一次完整 SSD1677 刷新窗口的持续时间。

## 启用模块

- `m5pm_test_support`：测试标记和安全保持状态
- `m5pm_phase_test`：共享 awake L2 基线和测量窗口计时
- `m5pm_power`：IOE1 显示电源和 PM1 前光控制
- `m5pm_display`：480x800、portrait 方向的 SSD1677 显示驱动

## 模块配置

| 模块 | 配置 |
|---|---|
| ESP32-S3 | `sdkconfig.defaults` 中的 240 MHz CPU、16 MB Flash、80 MHz OPI PSRAM |
| I2C | PM1 和 IOE1 使用共享 100 kHz 板级总线 |
| 电源控制 | PM1 地址 `0x6E`、IOE1 地址 `0x4F`，使用上述共享总线 |
| 显示 | SSD1677 四灰度面板；portrait 方向 480x800；SPI2 mode 0，SCLK GPIO15、MOSI GPIO14、DC GPIO17、CS GPIO16、BUSY GPIO18，写入时钟 20 MHz；通过 IOE1 `PYB_EPD_EN` 使能 `EPD_3V3_L3B` 电源轨；初始帧使用 `render_white()`，之后交替刷新白色帧和 logo 帧 |
| 前光 | 在 `BOARD_OK` 前将 PM1 PWM 设置为 100%；刷新窗口内不改变前光 |
| 固件日志 | `BOARD_OK BASELINE=l2-awake DISPLAY=ssd1677 ORIENTATION=portrait REFRESH=%s AREA=480x800 PERIOD_MS=60000 FRONTLIGHT_PERCENT=100` |

## 运行模式

测试先准备共享 awake L2 基线，恢复显示电源，初始化显示并执行一次白色初始刷新。前光设置为 100% 后输出 `BOARD_OK` 和 `TEST_READY`。随后每个测量窗口交替刷新白色帧和 logo 帧。报告中的刷新结果来自覆盖一次完整刷新事件的游标，而不是 60 秒 `WINDOW Avg`。

## 工作模式

构建前选择且只选择一个 defaults 文件：

| `config_id` | Kconfig 选择 | 刷新模式 |
|---|---|---|
| `display-epd-fastest` | `CONFIG_M5PM_DISPLAY_EPD_FASTEST=y` | `epd_fastest` |
| `display-epd-fast` | `CONFIG_M5PM_DISPLAY_EPD_FAST=y` | `epd_fast` |
| `display-epd-quality` | `CONFIG_M5PM_DISPLAY_EPD_QUALITY=y` | `epd_quality` |

选择示例：

```powershell
Remove-Item -Force sdkconfig -ErrorAction SilentlyContinue
$env:SDKCONFIG_DEFAULTS = "sdkconfig.defaults;sdkconfig.display-epd-fastest.defaults"
idf.py reconfigure
```

其他模式分别将覆盖文件名替换为 `sdkconfig.display-epd-fast.defaults` 或 `sdkconfig.display-epd-quality.defaults`。在 Bash 中，使用 `rm -f sdkconfig` 和 `export SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.display-epd-fast.defaults"`。

## 测量电流

下表中的刷新结果是覆盖一次完整刷新窗口的 `CURSOR Avg`。同时列出持续时间和电荷量，用于区分事件测量和稳态平均值。

| 模式 | 3.7 V 电流 | 3.7 V 持续时间 | 3.7 V 电荷量 | 4.2 V 电流 | 4.2 V 持续时间 | 4.2 V 电荷量 |
|---|---:|---:|---:|---:|---:|---:|
| `display-epd-fastest` | 110.39 mA | 1.192452 s | 36.56481 uAh | 105.21 mA | 1.192452 s | 34.84959 uAh |
| `display-epd-fast` | 110.13 mA | 1.375295 s | 42.07079 uAh | 104.92 mA | 1.375295 s | 40.08100 uAh |
| `display-epd-quality` | 98.60 mA | 4.557625 s | 124.83403 uAh | 93.78 mA | 4.551928 s | 118.57186 uAh |

参见[功耗测量曲线索引](../../docs/power-graphs/README_CN.md)。

## 配置

| 设置 | 数值 |
|---|---|
| 目标 | ESP32-S3 |
| Test ID | `display_refresh_modes` |
| Config ID | `display-epd-fastest`、`display-epd-fast` 或 `display-epd-quality` |
| 测量策略 | 使用 `CURSOR` 选择一次完整刷新事件；模式比较不使用 60 秒 `WINDOW Avg` |

## 构建与烧录

先在仓库根目录运行 `python fetch_repos.py check`；如果组件缺失，再运行 `python fetch_repos.py fetch --skip-existing`。然后在本目录选择 defaults 文件并构建：

```powershell
Remove-Item -Force sdkconfig -ErrorAction SilentlyContinue
$env:SDKCONFIG_DEFAULTS = "sdkconfig.defaults;sdkconfig.display-epd-fastest.defaults"
idf.py set-target esp32s3
idf.py build
idf.py -p <PORT> flash
idf.py -p <PORT> monitor
```

串口监视器仅用于确认 `TEST_START`、`BOARD_OK` 和 `TEST_READY`。正式采集电流前断开 USB Type-C。

## 预期验证

日志必须在 `TEST_READY` 前标明所选 `REFRESH` 值和 `FRONTLIGHT_PERCENT=100`。采集期间，`frame_select` 用于标识每个刷新事件对应的帧。
