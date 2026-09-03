# 满载

[English](README.md) | [简体中文](README_CN.md)

## 目的

本固件用于测量 M5PaperMono 在显示、前光、LoRa、NFC 和 Wi-Fi 负载同时运行时的整板输入电流。负载受运行时长限制，也可以通过下述设备控制输入停止。

## 启用模块

| 模块 | 配置与运行行为 |
|---|---|
| `m5pm_test_support` | 初始化测试上下文，输出通用标记，报告最终状态并进入安全空闲状态。 |
| `m5pm_phase_test` | 建立已验证的 awake L2 基线，并配置 Key1/Key2 停止输入。 |
| `m5pm_power` | 恢复显示、LoRa 和 NFC 电源轨，设置 PM1 前光 PWM，并在清理阶段关闭 LoRa 电源。 |
| `m5pm_display` | 驱动 SSD1677，使用 portrait 方向和 480 x 800 分辨率，并以 `epd_fastest` 渲染满载状态帧。 |
| `m5pm_lora` / RadioLib | 以 868 MHz、SF12 和 22 dBm 初始化 SX1262，每个循环发送一个 `PM_<count>` 负载。 |
| `M5UnitUnified` / `M5UnitUnifiedNFC` | 通过板载原生 I2C 总线初始化 ST25R3916，以 NFC-A 轮询模式运行并统计检测到的 PICC。 |
| ESP-IDF Wi-Fi | 启动 ESP32-S3 station，使用配置的 SSID 和密码，并等待最长 8 秒获取 IP 地址。 |
| 板载触摸输入 | 轮询触摸控制器；在标题区域检测到触摸（`y < 80`）时停止负载。 |

## 模块配置

| 设置 | 数值或来源 |
|---|---|
| 满载安全门槛 | `CONFIG_M5PM_FULL_LOAD_ENABLE`；默认关闭，必须在 `menuconfig` 中显式启用。 |
| Wi-Fi SSID | `CONFIG_M5PM_FULL_LOAD_WIFI_SSID`；必填，仅在本地提供。 |
| Wi-Fi 密码 | `CONFIG_M5PM_FULL_LOAD_WIFI_PASSWORD`；仅在本地配置，发布文件不包含实际凭据。 |
| 最大运行时长 | `CONFIG_M5PM_FULL_LOAD_RUNTIME_SECONDS`；1 至 3600 秒，默认 600 秒。 |
| 显示 | SSD1677、portrait、480 x 800、`epd_fastest`；通过 IOE1 `PYB_EPD_EN` 使能 `EPD_3V3_L3B` 电源轨；负载期间前光为 100%，清理阶段降至 50%。 |
| LoRa | SX1262、868 MHz、SF12、22 dBm；通过 PM1 GPIO2 使能 `3V3_L2_LoRa` 电源轨；每个负载循环发送一个数据负载。 |
| NFC | ST25R3916，I2C 地址 `0x50`，由 IOE1 `PYB_NFC_EN` 使能 `3V3_L2` 电源；NFC-A 轮询，禁用 IRQ；统计检测到的 PICC 并执行停用。 |
| 停止输入 | Key1、Key2 或标题区域触摸。 |
| 测试标识 | `test_id=full_load`、`config_id=full_load`。 |

固件会在 `TEST_READY` 之前输出以下板状态标记：

```text
BOARD_OK FRONTLIGHT=100 DISPLAY_MODE=epd_fastest DISPLAY_ORIENTATION=portrait DISPLAY_AREA=480x800 LORA=868MHz/SF12/22dBm NFC=nfc_a_polling TOUCH=active
```

## 运行模式

固件初始化测试上下文，检查本地是否配置了 Wi-Fi SSID，建立 awake L2 基线，然后恢复显示、LoRa 和 NFC 电源轨。显示、SX1262、ST25R3916 和 Wi-Fi station 启动后，固件输出 `BOARD_OK` 和 `TEST_READY`。循环依次发送 LoRa 数据负载、轮询 NFC-A、刷新状态帧并等待 1 秒，直到达到配置的运行时长或收到停止输入。清理阶段停止 LoRa，将前光降至 50%，停止 Wi-Fi，关闭 NFC 场并进入安全空闲状态。

## 安全门槛

`sdkconfig.defaults` 中的安全门槛默认关闭。门槛关闭时，固件输出 `TEST_FAIL detail=safety_gate_disabled`，不会启动高负载工作状态。在针对目标硬件完成测量装置、限流值、温度限制、硬超时和停止控制的审查前，请保持该门槛关闭。

正式测量期间，应使用测量装置规定的限流值和温度限制。达到任一装置限制时停止测试。固件还会执行 `CONFIG_M5PM_FULL_LOAD_RUNTIME_SECONDS` 运行时限，并响应 Key1、Key2 和标题区域触摸停止输入。

## 测量电流

以下数值为测量装置记录的 `WINDOW Avg`：

| 供电电压 | 窗口平均电流 |
|---|---:|
| 3.7 V | 187.32 mA |
| 4.2 V | 176.60 mA |

参见[功耗测量曲线索引](../../docs/power-graphs/README_CN.md)。

## 硬件与构建配置

| 设置 | 数值 |
|---|---|
| 目标 | ESP32-S3 |
| Flash | 16 MB |
| PSRAM | OPI，80 MHz |
| CPU | 240 MHz |
| I2C | 使用时为 100 kHz |
| ESP-IDF | 5.5.5 |

## 依赖配置

在 ESP-IDF 5.5.5 shell 中，从仓库根目录运行以下命令。由于 `repos.json` 包含 registry 组件 `i2c_bus`，必须使用 ESP-IDF Python 环境。

```bash
python fetch_repos.py check
```

如果检查结果包含缺失组件，运行 `python fetch_repos.py fetch --skip-existing`，仅安装缺失目录。如果检查结果包含版本不匹配，确认需要重新生成组件树后再运行 `python fetch_repos.py fetch --replace`。脚本会在替换前创建 `_component_backups/`。registry 组件需要 ESP-IDF Python 环境。

## 配置、构建与烧录

在本测试目录运行：

```bash
idf.py set-target esp32s3
idf.py menuconfig
```

在 `menuconfig` 中打开 **M5PaperMono full-load safety**，启用 **Enable the full-load test**。然后打开 **M5PaperMono full-load network**，设置：

- **Wi-Fi SSID**（`CONFIG_M5PM_FULL_LOAD_WIFI_SSID`）
- **Wi-Fi password**（`CONFIG_M5PM_FULL_LOAD_WIFI_PASSWORD`）
- **Maximum runtime (seconds)**（`CONFIG_M5PM_FULL_LOAD_RUNTIME_SECONDS`，1-3600）

保存配置后构建并烧录：

```bash
idf.py build
idf.py -p <PORT> flash
idf.py -p <PORT> monitor
```

SSID 和密码仅保存在本地 `sdkconfig` 中；发布的 defaults、源文件、Issue、报告和文档均不包含实际凭据值。确认串口监视器中的初始化标记后，关闭监视器，并在正式测量电流前断开 USB Type-C。

## 预期验证

确认 `TEST_START`、满载测试的 `BOARD_OK` 字段、`TEST_READY` 和 `LOAD_RUNNING`。如果初始化或安全检查无法继续，固件会输出带有具体原因的 `TEST_FAIL`，然后进入安全空闲状态。
