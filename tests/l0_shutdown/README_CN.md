# L0 关机

[English](README.md) | [简体中文](README_CN.md)

## 目的

本固件请求 PM1 进入 L0 关机，并测量关机状态下 M5PaperMono 的整板输入电流。

## 启用模块

- `m5pm_test_support`：测试标记和安全保持状态
- `m5pm_board`：共享 I2C 初始化和总线所有权
- `m5pm_power`：PM1 初始化和 L0 关机请求

## 模块配置

| 模块 | 配置 |
|---|---|
| ESP32-S3 | 未初始化应用外设 |
| I2C | 用于获取 PM1 的共享板级总线 |
| PM1 | 地址 `0x6E`；通过 `PowerController::begin()` 初始化 |
| 电源转换 | 准备延时 3000 ms 后由 `enter_l0()` 请求 PM1 L0 关机 |
| 测试状态 | PM1 L0 关机；未配置唤醒源 |
| 固件日志 | `BOARD_OK PM1=1` |

## 运行模式

共享 I2C 总线和 PM1 完成初始化后输出 `BOARD_OK` 和 `TEST_READY`，等待 3000 ms，再由 `enter_l0()` 请求 PM1 L0 关机。关机请求成功后不再进行日志输出或 I2C 访问。

## 测量电流

测量结果中的电流为测量记录中的 `WINDOW Avg`。

- 3.7 V：17.81 uA
- 4.2 V：20.75 uA

参见[功耗测量曲线索引](../../docs/power-graphs/README_CN.md)。

## 配置

| 设置 | 数值 |
|---|---|
| 目标 | ESP32-S3 |
| Flash | 16 MB |
| PSRAM | OPI，80 MHz |
| CPU | 240 MHz |
| I2C | 使用时为 100 kHz |
| Test ID | `l0_shutdown` |
| Config ID | `l0_shutdown` |

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
