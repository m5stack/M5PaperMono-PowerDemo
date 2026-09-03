# M5PaperMono 功耗演示

[English](README.md) | [简体中文](README_CN.md)

## 项目概述

本仓库包含多个相互独立的 ESP-IDF 固件项目，用于测量量产版 M5PaperMono 硬件的功耗。每项测试独立构建和烧录，上电后自动启动，并复用共享的板级支持模块。

项目基于 ESP-IDF 5.5.5，构建、烧录、串口验证和电流测量均为手动操作。

## 功耗测试结果

功耗以整板平均输入电流表示。除详情报告另有说明外，每项数据采用约 60 秒测量窗口中选定的 `WINDOW Avg`。显示刷新和单次发送项目使用详情报告中说明的测量字段。

本表仅列出器件和工作状态摘要，完整配置请参见详情链接。

多模式项目使用斜杠分隔数值，顺序与详情报告中的模式顺序一致：IMU 唤醒为 `low_power` / `reference`，显示刷新为 `fastest` / `fast` / `quality`，前光为 0% / 50% / 100%。

| 测试场景 | 器件 / 工作状态 | 3.7 V 平均电流 | 4.2 V 平均电流 | 详情 |
|---|---|---:|---:|---|
| L0 关机 | PM1：L0 关机<br>ESP32-S3：未初始化 | 17.81 uA | 20.75 uA | [详情](tests/l0_shutdown/README_CN.md) |
| L1 待机 | PM1：L1 standby<br>BMI270：低功耗状态；NFC 电源轨关闭 | 26.07 uA | 29.09 uA | [详情](tests/l1_standby/README_CN.md) |
| L2 深度睡眠 | ESP32-S3：Deep Sleep<br>PM1/IOE1：L2 状态 | 129.08 uA | 81.06 uA | [详情](tests/l2_deep_sleep/README_CN.md) |
| IMU 深度睡眠唤醒 | ESP32-S3：Deep Sleep<br>BMI270：任意运动唤醒 | 726.19 uA（low_power）<br>1.38 mA（reference） | 717.64 uA（low_power）<br>1.36 mA（reference） | [详情](tests/imu_wake_deep_sleep/README_CN.md) |
| IMU 关机唤醒 | PM1：shutdown<br>BMI270：任意运动唤醒 | 42.72 uA（low_power）<br>692.80 uA（reference） | 45.82 uA（low_power）<br>696.20 uA（reference） | [详情](tests/imu_wake_shutdown/README_CN.md) |
| 按键深度睡眠唤醒 | ESP32-S3：Deep Sleep<br>Key1/Key2：EXT1 唤醒 | 101.95 uA | 86.89 uA | [详情](tests/button_wake_deep_sleep/README_CN.md) |
| RTC Alarm 深度睡眠唤醒 | ESP32-S3：Deep Sleep<br>RX8130CE：Alarm 已配置 | 76.05 uA | 63.15 uA | [详情](tests/rtc_alarm_wake_deep_sleep/README_CN.md) |
| RTC Alarm 关机唤醒 | PM1：shutdown<br>RX8130CE：Alarm 已配置 | 18.26 uA | 21.17 uA | [详情](tests/rtc_alarm_wake_shutdown/README_CN.md) |
| RTC Timer 深度睡眠唤醒 | ESP32-S3：Deep Sleep<br>RX8130CE：120 s Timer 已配置 | 661.29 uA | 671.19 uA | [详情](tests/rtc_timer_wake_deep_sleep/README_CN.md) |
| RTC Timer 关机唤醒 | PM1：shutdown<br>RX8130CE：120 s Timer 已配置 | 18.26 uA | 21.27 uA | [详情](tests/rtc_timer_wake_shutdown/README_CN.md) |
| 正常工作 | ESP32-S3：运行状态<br>SSD1677：quality 每 5 s 刷新；前光 50% | 76.80 mA | 72.34 mA | [详情](tests/normal_active/README_CN.md) |
| 显示负载 | ESP32-S3：运行状态<br>SSD1677：静态帧，前光 100% | 89.90 mA | 85.30 mA | [详情](tests/display_only/README_CN.md) |
| 显示刷新模式 | SSD1677：完整刷新<br>模式：fastest、fast、quality | 110.39 / 110.13 / 98.60 mA | 105.21 / 104.92 / 93.78 mA | [详情](tests/display_refresh_modes/README_CN.md) |
| 前光档位 | SSD1677：静态帧<br>前光：0%、50% 或 100% | 31.55 / 61.28 / 89.98 mA | 29.13 / 58.23 / 85.35 mA | [详情](tests/frontlight_levels/README_CN.md) |
| IMU 采样 | ESP32-S3：运行状态<br>BMI270：加速度计 + 陀螺仪 | 31.63 mA | 28.99 mA | [详情](tests/imu_sampling/README_CN.md) |
| 满载 | ESP32-S3：运行状态<br>SSD1677、SX1262、NFC、Wi-Fi 和前光 | 187.32 mA | 176.60 mA | [详情](tests/full_load/README_CN.md) |
| NFC Power Down | ST25R3916：Power Down 状态 | 34.35 mA | 32.39 mA | [详情](tests/nfc_working_current/README_CN.md) |
| NFC-A 轮询 | ST25R3916：NFC-A 轮询 | 109.31 mA | 103.21 mA | [详情](tests/nfc_working_current/README_CN.md) |
| LoRa 周期发送 | SX1262：周期 TX | 61.25 mA | 59.50 mA | [详情](tests/lora_periodic_transmit/README_CN.md) |
| LoRa 单次发送 | SX1262：单次 TX | 96.55 mA | 93.52 mA | [详情](tests/lora_periodic_transmit/README_CN.md) |
| LoRa 接收空闲 | SX1262：连续 RX | 42.02 mA | 39.89 mA | [详情](tests/lora_rx_idle/README_CN.md) |
| LoRa 睡眠 | SX1262：Sleep 状态 | 31.71 mA | 29.68 mA | [详情](tests/lora_sleep/README_CN.md) |
| Wi-Fi 已连接 | ESP32-S3：station 已连接<br>省电：禁用 | 102.18 mA | 93.97 mA | [详情](tests/wifi_connected/README_CN.md) |
| Wi-Fi Light Sleep | ESP32-S3：自动 Light Sleep<br>Wi-Fi：最大 modem power save | 4.28 mA | 5.31 mA | [详情](tests/wifi_light_sleep/README_CN.md) |
| Wi-Fi 周期流量 | ESP32-S3：station 已连接<br>ICMP 周期流量 | 102.08 mA | 94.21 mA | [详情](tests/wifi_periodic_traffic/README_CN.md) |

## 测量曲线

所有已发布曲线统一收录在[功耗测量曲线](docs/power-graphs/README_CN.md)索引中。

## 仓库结构

```text
components/  获取的第三方组件
modules/     M5PaperMono 共享板级与测试支持组件
tests/       相互独立的 ESP-IDF 功耗测试项目
docs/        已发布的测量证据
```

## 构建与烧录

安装 ESP-IDF 5.5.5，并在运行任何 `idf.py` 命令前激活其导出环境。依赖脚本需要 Python 3 和 Git。在仓库根目录先检查锁定的组件目录，再获取缺失组件：

```bash
python fetch_repos.py check
```

如果检查结果包含缺失组件，运行 `python fetch_repos.py fetch --skip-existing`，仅安装缺失目录。如果检查结果包含版本不匹配，确认需要重新生成组件树后再运行 `python fetch_repos.py fetch --replace`。脚本会在替换前创建 `_component_backups/`。`fetch_repos.py` 读取 `repos.json`，获取其中指定的 Git 提交或 registry 哈希，应用文件中列出的补丁，并按声明路径安装组件。获取 registry 组件时，必须在可用 ESP-IDF Python 环境的 ESP-IDF shell 中运行脚本。

然后进入对应测试目录执行构建和烧录。例如：

```bash
cd tests/l0_shutdown
idf.py set-target esp32s3
idf.py build
idf.py -p <PORT> flash
idf.py -p <PORT> monitor
```

每个测试目录都有完整的 `sdkconfig.defaults`。多模式测试还提供稀疏的模式覆盖文件，不要用覆盖文件替换基础 defaults 文件。首次配置或切换模式前，先删除本地生成的 `sdkconfig`，再将 `SDKCONFIG_DEFAULTS` 设置为基础文件和所选覆盖文件，两者之间使用分号分隔。例如：

```powershell
$env:SDKCONFIG_DEFAULTS = "sdkconfig.defaults;sdkconfig.display-epd-fast.defaults"
idf.py reconfigure
```

在 Bash 中使用等效的 `export SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.display-epd-fast.defaults"`。生成的 `build/`、`sdkconfig` 和 `dependencies.lock` 是本地构建产物。USB Type-C 仅用于烧录和串口验证，正式测量电流前必须断开。

## 测量方法

电流是从整板输入端测得的总输入电流，不是单个器件的电流。固件不会检测供电来源，也不会在串口验证和正式测量之间自动切换模式。

1. 连接 USB Type-C 烧录固件，并通过串口确认 `TEST_START`、`BOARD_OK` 和 `TEST_READY`。
2. 保持固件和配置不变，断开 USB Type-C，仅通过测量装置供电。
3. 在标称 3.7 V 和 4.2 V 下，待电流稳定后测量约 60 秒，并记录测量窗口中的 `WINDOW Avg`。显示刷新和 LoRa 单次发送测试按各自报告中的 `CURSOR Avg` 记录完整事件。

## 许可证

项目自有源代码采用 [MIT License](LICENSE)。第三方组件遵循[第三方声明](THIRD_PARTY_NOTICES_CN.md)中列出的许可证。

## 依赖

锁定的组件版本、来源和许可证见[第三方声明](THIRD_PARTY_NOTICES_CN.md)。项目适配补丁记录在 `patches/`。
