# Wi-Fi Light Sleep

[English](README.md) | [简体中文](README_CN.md)

## 目的

本固件测量 ESP32-S3 station 已连接、使用最大 modem power save 和自动 Light Sleep 时的整板输入电流。

## 启用模块

- `m5pm_wifi`：station 连接和最大 modem power save 配置
- ESP-IDF 电源管理：动态调频和 Light Sleep
- `m5pm_phase_test` 与 `m5pm_test_support`：基线、标记和测量计时

## 模块配置

| 模块 | 配置 |
|---|---|
| Wi-Fi | station 模式、已连接、`PS=max_modem`；无应用流量和显式扫描；凭据使用 `CONFIG_M5PM_WIFI_SSID` 和 `CONFIG_M5PM_WIFI_PASSWORD` |
| 连接超时 | `CONFIG_M5PM_WIFI_CONNECT_TIMEOUT_MS`，默认 15,000 ms |
| 电源管理 | `CONFIG_PM_ENABLE=1`、`CONFIG_FREERTOS_USE_TICKLESS_IDLE=1`，启用 Light Sleep |
| CPU 频率 | 最大 240 MHz、最小 40 MHz，由 `esp_pm_get_configuration()` 读回验证 |
| 固件日志 | `BOARD_OK BASELINE=l2-awake WIFI=connected MODE=sta PS=max_modem APP_TRAFFIC=0 EXPLICIT_SCAN=0 AUTO_LIGHT_SLEEP=1 TICKLESS=1 CPU_MIN_MHZ=40 CPU_MAX_MHZ=240` |

## 运行模式

测试准备 awake L2 基线，以最大 modem power save 连接 Wi-Fi，应用并读回 ESP-IDF 电源管理配置，输出 `BOARD_OK` 和 `TEST_READY`，并等待测量窗口结束。进入安全保持状态前检查 station 仍保持连接。

## 网络配置

| 设置 | 数值 |
|---|---|
| Wi-Fi 模式 | Station（`MODE=sta`） |
| 省电 | 最大 modem power save（`PS=max_modem`） |
| 自动 Light Sleep | 启用（`AUTO_LIGHT_SLEEP=1`） |
| Tickless idle | 启用（`TICKLESS=1`） |
| CPU 频率 | 40-240 MHz |
| 应用流量 | 无（`APP_TRAFFIC=0`） |

## 测量电流

测量结果中的电流为测量记录中的 `WINDOW Avg`。

| 电压 | 电流 |
|---|---:|
| 3.7 V | 4.28 mA |
| 4.2 V | 5.31 mA |

参见[功耗测量曲线索引](../../docs/power-graphs/README_CN.md)。

## 配置

| 设置 | 数值 |
|---|---|
| 目标 | ESP32-S3 |
| Test ID | `wifi_light_sleep` |
| Config ID | `wifi_light_sleep` |
| 构建断言 | `CONFIG_PM_ENABLE=1`；`CONFIG_FREERTOS_USE_TICKLESS_IDLE=1` |

构建前运行 `idf.py menuconfig`，搜索 `M5PM_WIFI_SSID`，在 **M5PaperMono Wi-Fi test** 菜单中设置 `M5PM_WIFI_SSID` 和 `M5PM_WIFI_PASSWORD`。`M5PM_WIFI_CONNECT_TIMEOUT_MS` 的默认值为 15,000 ms。保持上述电源管理选项启用。

## 构建与烧录

先在仓库根目录运行 `python fetch_repos.py check`；如果组件缺失，再运行 `python fetch_repos.py fetch --skip-existing`。然后在本目录构建：

```bash
idf.py set-target esp32s3
idf.py build
idf.py -p <PORT> flash
idf.py -p <PORT> monitor
```

通过 USB Type-C 确认 `TEST_START`、电源管理读回值、`BOARD_OK` 和 `TEST_READY`。测量电流前断开 USB Type-C。

## 预期验证

`TEST_READY` 前日志必须报告 `PS=max_modem`、`AUTO_LIGHT_SLEEP=1`、`TICKLESS=1` 以及已验证的 40-240 MHz 范围。
