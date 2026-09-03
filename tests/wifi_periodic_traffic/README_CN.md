# Wi-Fi 周期流量

[English](README.md) | [简体中文](README_CN.md)

## 目的

本固件测量已连接 ESP32-S3 station 发送固定周期 ICMP Echo 流量时的整板输入电流。

## 启用模块

- `m5pm_wifi`：station 连接和网络辅助模块
- `m5pm_phase_test`：共享 awake L2 基线和测量计时
- `m5pm_test_support`：测试标记和安全保持状态

## 模块配置

| 模块 | 配置 |
|---|---|
| Wi-Fi | station 模式、已连接、禁用省电；无显式扫描；凭据使用 `CONFIG_M5PM_WIFI_SSID` 和 `CONFIG_M5PM_WIFI_PASSWORD` |
| 连接超时 | `CONFIG_M5PM_WIFI_CONNECT_TIMEOUT_MS`，默认 15,000 ms |
| ICMP 流量 | 目标由 `CONFIG_M5PM_WIFI_PING_TARGET` 配置；每 1000 ms 发送 64 字节固定负载；测试负载不重试 |
| 基线 | 启动 Wi-Fi 前准备共享 awake L2 |
| 固件日志 | `BOARD_OK BASELINE=l2-awake WIFI=connected MODE=sta PS=none ICMP_PAYLOAD_BYTES=64 PERIOD_MS=1000 TARGET=fixed_lan ACK_POLICY=none RETRY_WORKLOAD=0 EXPLICIT_SCAN=0` |

## 运行模式

测试准备 awake L2 基线，关闭省电连接 Wi-Fi，启动固定 ping 任务，输出 `BOARD_OK` 和 `TEST_READY`，并等待测量窗口结束。进入安全保持状态前检查 station 仍保持连接。

## 网络配置

| 设置 | 数值 |
|---|---|
| Wi-Fi 模式 | Station（`MODE=sta`） |
| 省电 | 禁用（`PS=none`） |
| ICMP 负载 | 64 字节 |
| 周期 | 1000 ms |
| 目标 | 固定局域网目标（`TARGET=fixed_lan`） |
| 确认和重试 | `ACK_POLICY=none`；`RETRY_WORKLOAD=0` |

## 测量电流

测量结果中的电流为测量记录中的 `WINDOW Avg`。

| 电压 | 电流 |
|---|---:|
| 3.7 V | 102.08 mA |
| 4.2 V | 94.21 mA |

参见[功耗测量曲线索引](../../docs/power-graphs/README_CN.md)。

## 配置

| 设置 | 数值 |
|---|---|
| 目标 | ESP32-S3 |
| Test ID | `wifi_periodic_traffic` |
| Config ID | `wifi_periodic_traffic` |

构建前运行 `idf.py menuconfig`，搜索 `M5PM_WIFI_SSID`，在 **M5PaperMono Wi-Fi test** 菜单中设置 `M5PM_WIFI_SSID`、`M5PM_WIFI_PASSWORD` 和 `M5PM_WIFI_PING_TARGET`。按需设置 `M5PM_WIFI_CONNECT_TIMEOUT_MS`，其默认值为 15,000 ms。目标必须是同一局域网中能够响应 ICMP Echo 请求的固定 IPv4 主机。

## 构建与烧录

先在仓库根目录运行 `python fetch_repos.py check`；如果组件缺失，再运行 `python fetch_repos.py fetch --skip-existing`。配置 `m5pm_wifi` 所需的 Wi-Fi 凭据和固定局域网目标，然后在本目录构建：

```bash
idf.py set-target esp32s3
idf.py build
idf.py -p <PORT> flash
idf.py -p <PORT> monitor
```

通过 USB Type-C 确认 `TEST_START`、`BOARD_OK` 和 `TEST_READY`。测量电流前断开 USB Type-C。

## 预期验证

`TEST_READY` 前日志必须报告 64 字节负载、1000 ms 周期、固定局域网目标和禁用省电。
