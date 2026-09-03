# NFC 工作电流

[English](README.md) | [简体中文](README_CN.md)

## 目的

本固件测量 ST25R3916 NFC 读写器处于 Power Down 或 NFC-A 轮询模式时的整板输入电流。

## 启用模块

- `M5UnitUnified` 和 `M5UnitUnifiedNFC`：ST25R3916 读写器控制
- `m5pm_phase_test`：共享 awake L2 基线
- `m5pm_power`：通过 IOE1 和 PM1 恢复 NFC 电源
- `m5pm_test_support`：配置 ID、测试标记和安全保持状态

## 模块配置

| 模块 | 配置 |
|---|---|
| ST25R3916 | I2C 地址 `0x50`，NFC-A 模式，禁用模拟模式，禁用 IRQ 使用；由 IOE1 `PYB_NFC_EN` 使能 `3V3_L2` 电源，并通过共享 I2C 总线控制 |
| NFC Power Down | 关闭场，发送 `CMD_STOP_ALL_ACTIVITIES`，写入并读回操作控制寄存器 `0x00` |
| NFC-A 轮询 | 100 ms 检测超时，随后间隔 20 ms；采集前和采集期间都必须无卡 |
| I2C | 共享 100 kHz 板级总线；NFC 读写器通过 `M5UnitUnified` 连接 |
| 固件日志 | Power Down：`BOARD_OK NFC=power_down operation_control=0x%02X`；轮询：`BOARD_OK NFC=nfc_a_polling poll_period_ms=120 card=absent` |

## 运行模式

测试支持接收所选模式的 `config_id`。测试准备 awake L2 基线，恢复 NFC 电源，等待 120 ms 稳定时间，初始化读写器并输出 `TEST_READY`。Power Down 随后保持读写器空闲。轮询模式重复执行 100 ms 超时的 NFC-A 检测和 20 ms 间隔；检测到卡片时进入安全保持状态。

## 工作模式

构建前选择一个 Kconfig defaults 文件：

| `config_id` | Kconfig 选择 | 工作状态 |
|---|---|---|
| `nfc-power-down` | `CONFIG_M5PM_NFC_MODE_POWER_DOWN=y` | 关闭场、停止所有活动，操作控制 `0x00` |
| `nfc-a-polling` | `CONFIG_M5PM_NFC_MODE_NFC_A_POLLING=y` | NFC-A 轮询，无卡片 |

例如：

```powershell
Remove-Item -Force sdkconfig -ErrorAction SilentlyContinue
$env:SDKCONFIG_DEFAULTS = "sdkconfig.defaults;sdkconfig.nfc-a-polling.defaults"
idf.py reconfigure
```

在 Bash 中，使用 `rm -f sdkconfig`，然后执行 `export SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.nfc-a-polling.defaults"` 和 `idf.py reconfigure`。

## 测量电流

测量结果中的电流为测量记录中的 `WINDOW Avg`。

| 模式 | 3.7 V | 4.2 V |
|---|---:|---:|
| `nfc-power-down` | 34.35 mA | 32.39 mA |
| `nfc-a-polling` | 109.31 mA | 103.21 mA |

参见[功耗测量曲线索引](../../docs/power-graphs/README_CN.md)。

## 配置

| 设置 | 数值 |
|---|---|
| 目标 | ESP32-S3 |
| Test ID | `nfc_working_current` |
| Config ID | `nfc-power-down` 或 `nfc-a-polling` |
| 读写器地址 | 由 M5UnitUnifiedNFC 组件选择 ST25R3916 地址 |

## 构建与烧录

先在仓库根目录运行 `python fetch_repos.py check`；如果组件缺失，再运行 `python fetch_repos.py fetch --skip-existing`。然后在本目录选择所需 defaults 文件并构建：

```powershell
Remove-Item -Force sdkconfig -ErrorAction SilentlyContinue
$env:SDKCONFIG_DEFAULTS = "sdkconfig.defaults;sdkconfig.nfc-a-polling.defaults"
idf.py set-target esp32s3
idf.py build
idf.py -p <PORT> flash
idf.py -p <PORT> monitor
```

通过 USB Type-C 确认 `TEST_START`、对应模式的 `BOARD_OK` 和 `TEST_READY`。正式测量前断开 USB Type-C；进行轮询采集时，NFC 天线区域应保持无卡。

## 预期验证

Power Down 必须读回操作控制 `0x00`。轮询模式必须输出 `card=absent`，并持续执行 100 ms 检测加 20 ms 间隔的循环。
