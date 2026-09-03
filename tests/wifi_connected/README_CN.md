# Wi-Fi 已连接

[English](README.md) | [简体中文](README_CN.md)

## 目的

本固件测量 ESP32-S3 station 连接到配置的接入点、关闭 Wi-Fi 省电且无应用流量时的整板输入电流。

## 启用模块

- `m5pm_wifi`：station 连接和省电配置
- `m5pm_phase_test`：共享 awake L2 基线和测量计时
- `m5pm_test_support`：测试标记和安全保持状态

## 模块配置

| 模块 | 配置 |
|---|---|
| ESP32-S3 Wi-Fi | station 模式；`PowerSave::none`；无应用流量和显式扫描 |
| 凭据 | 项目 Kconfig 菜单中的 `CONFIG_M5PM_WIFI_SSID` 和 `CONFIG_M5PM_WIFI_PASSWORD` |
| 连接超时 | `CONFIG_M5PM_WIFI_CONNECT_TIMEOUT_MS`，默认 15,000 ms |
| 板级基线 | PM1/IOE1 awake L2 配置；关闭显示、触摸、IMU、音频、电机和外部 5 V 负载 |
| 基线 | 启动 Wi-Fi 前准备共享 awake L2 |
| 固件日志 | `BOARD_OK BASELINE=l2-awake WIFI=connected MODE=sta PS=none APP_TRAFFIC=0 EXPLICIT_SCAN=0` |

## 运行模式

测试准备 awake L2 基线，连接 station，输出 `BOARD_OK` 和 `TEST_READY`，并等待测量窗口结束。进入安全保持状态前检查 station 仍保持连接。

## 网络配置

| 设置 | 数值 |
|---|---|
| Wi-Fi 模式 | Station（`MODE=sta`） |
| 省电 | 禁用（`PS=none`） |
| 应用流量 | 无（`APP_TRAFFIC=0`） |
| 扫描 | 无显式扫描（`EXPLICIT_SCAN=0`） |

## 测量电流

测量结果中的电流为测量记录中的 `WINDOW Avg`。

| 电压 | 电流 |
|---|---:|
| 3.7 V | 102.18 mA |
| 4.2 V | 93.97 mA |

参见[功耗测量曲线索引](../../docs/power-graphs/README_CN.md)。

## 配置

| 设置 | 数值 |
|---|---|
| 目标 | ESP32-S3 |
| Test ID | `wifi_connected` |
| Config ID | `wifi_connected` |

构建前运行 `idf.py menuconfig`，搜索 `M5PM_WIFI_SSID`，在 **M5PaperMono Wi-Fi test** 菜单中设置 `M5PM_WIFI_SSID` 和 `M5PM_WIFI_PASSWORD`。`M5PM_WIFI_CONNECT_TIMEOUT_MS` 的默认值为 15,000 ms，用于连接检查。

## 构建与烧录

先在仓库根目录运行 `python fetch_repos.py check`；如果组件缺失，再运行 `python fetch_repos.py fetch --skip-existing`。配置 `m5pm_wifi` 所需的 Wi-Fi 凭据，然后在本目录构建：

```bash
idf.py set-target esp32s3
idf.py build
idf.py -p <PORT> flash
idf.py -p <PORT> monitor
```

通过 USB Type-C 确认 `TEST_START`、连接结果、`BOARD_OK` 和 `TEST_READY`。测量电流前断开 USB Type-C。

## 预期验证

`BOARD_OK` 必须报告 `PS=none`、`APP_TRAFFIC=0` 和 `EXPLICIT_SCAN=0`。测量窗口结束时，连接状态检查必须仍为真。
