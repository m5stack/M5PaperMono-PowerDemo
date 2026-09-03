# 前光档位

[English](README.md) | [简体中文](README_CN.md)

## 目的

本固件在 SSD1677 保持 quality 模式、前光设置为一个选定 PWM 档位时，测量稳定的整板输入电流。

## 启用模块

- `m5pm_test_support`：测试标记和安全保持状态
- `m5pm_phase_test`：共享 awake L2 基线和测量计时
- `m5pm_power`：显示电源和 PM1 前光 PWM 控制
- `m5pm_display`：SSD1677 初始化和静态帧

## 模块配置

| 模块 | 配置 |
|---|---|
| ESP32-S3 | 240 MHz CPU、16 MB Flash、80 MHz OPI PSRAM |
| I2C | PM1/IOE1 共享 100 kHz 总线 |
| 电源控制 | PM1 地址 `0x6E`、IOE1 地址 `0x4F`，使用上述共享总线 |
| 显示 | SSD1677 四灰度面板；portrait 方向 480x800；SPI2 mode 0，SCLK GPIO15、MOSI GPIO14、DC GPIO17、CS GPIO16、BUSY GPIO18，写入时钟 20 MHz；通过 IOE1 `PYB_EPD_EN` 使能 `EPD_3V3_L3B` 电源轨；测量前执行一次 quality 模式白色刷新 |
| 前光 | PM1 GPIO3（`PYG3_BL_PWM`）的 PWM 通道 0，连接显示背光驱动器，频率 5 kHz；初始刷新后应用所选占空比 |
| 固件日志 | `BOARD_OK BASELINE=l2-awake DISPLAY=ssd1677 ORIENTATION=portrait REFRESH=quality AREA=480x800 PERIODIC_REFRESH=0 FRONTLIGHT_PERCENT=%u` |

## 运行模式

测试准备 awake L2 基线，恢复显示电源，初始化 SSD1677，执行一次 quality 模式白色刷新，应用所选前光占空比并输出 `TEST_READY`。随后保持静态帧，在测量窗口内不进行周期刷新。

## 工作模式

构建前选择一个 defaults 文件。每个文件设置一个 Kconfig 符号及对应的 `config_id`：

| `config_id` | Kconfig 选择 | 前光 |
|---|---|---:|
| `frontlight-0` | `CONFIG_M5PM_FRONTLIGHT_0=y` | 0% |
| `frontlight-50` | `CONFIG_M5PM_FRONTLIGHT_50=y` | 50% |
| `frontlight-100` | `CONFIG_M5PM_FRONTLIGHT_100=y` | 100% |

例如：

```powershell
Remove-Item -Force sdkconfig -ErrorAction SilentlyContinue
$env:SDKCONFIG_DEFAULTS = "sdkconfig.defaults;sdkconfig.frontlight-50.defaults"
idf.py reconfigure
```

在 Bash 中，使用 `rm -f sdkconfig`，然后执行 `export SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.frontlight-50.defaults"` 和 `idf.py reconfigure`。

## 测量电流

测量结果中的电流为测量记录中的 `WINDOW Avg`。

| 前光 | 3.7 V | 4.2 V |
|---:|---:|---:|
| 0% | 31.55 mA | 29.13 mA |
| 50% | 61.28 mA | 58.23 mA |
| 100% | 89.98 mA | 85.35 mA |

参见[功耗测量曲线索引](../../docs/power-graphs/README_CN.md)。

## 配置

| 设置 | 数值 |
|---|---|
| 目标 | ESP32-S3 |
| Test ID | `frontlight_levels` |
| Config ID | `frontlight-0`、`frontlight-50` 或 `frontlight-100` |
| 刷新行为 | 初始 quality 刷新一次；采集期间 `PERIODIC_REFRESH=0` |

## 构建与烧录

先在仓库根目录运行 `python fetch_repos.py check`；如果组件缺失，再运行 `python fetch_repos.py fetch --skip-existing`。然后在本目录选择 defaults 文件并构建：

```powershell
Remove-Item -Force sdkconfig -ErrorAction SilentlyContinue
$env:SDKCONFIG_DEFAULTS = "sdkconfig.defaults;sdkconfig.frontlight-50.defaults"
idf.py set-target esp32s3
idf.py build
idf.py -p <PORT> flash
idf.py -p <PORT> monitor
```

使用监视器确认 `TEST_START`、`BOARD_OK` 和 `TEST_READY`，测量电流前断开 USB Type-C。

## 预期验证

`BOARD_OK` 必须在 `TEST_READY` 前报告所选 `FRONTLIGHT_PERCENT`。采集测量窗口期间显示保持静态。
