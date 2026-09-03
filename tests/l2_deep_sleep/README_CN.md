# L2 深度睡眠

[English](README.md) | [简体中文](README_CN.md)

## 目的

本固件配置板级低功耗外设后使 ESP32-S3 进入无唤醒源的 Deep Sleep，并测量该状态下的整板输入电流。

## 启用模块

- `m5pm_test_support`：测试标记和安全保持状态
- `m5pm_board`：共享 I2C 初始化和总线所有权
- `m5pm_power`：PM1/IOE1 L2 准备和 Deep Sleep 进入
- 板级外设：共享 L2 流程配置 M5PM1、M5IOE1、BMI270、ST25R3916 NFC 和 FT6336G 触摸

## 模块配置

| 模块 | 配置 |
|---|---|
| ESP32-S3 | 进入 Deep Sleep，未配置应用唤醒源 |
| PM1 | 地址 `0x6E`；配置 L2 电源和中断掩码 |
| IOE1 | 地址 `0x4F`；配置 L2 状态所需外设输出 |
| BMI270 | 地址 `0x68`；写入 `PWR_CONF (0x7C)=0x01` 和 `PWR_CTRL (0x7D)=0x00`；隔离器件中断 |
| ST25R3916 | 地址 `0x50`；先写寄存器解锁值 `0xC2`，再写操作控制 `0x00`；L2 基线保持 NFC 电源轨关闭 |
| FT6336G | 地址 `0x38`；写入休眠命令 `0xA5=0x03`；隔离触摸中断 |
| I2C | 共享 100 kHz 总线；睡眠前降低驱动日志级别 |
| 唤醒源 | 无（`WAKE=none`） |
| 测试状态 | PM1 和 IOE1 L2 Deep Sleep 配置 |
| 固件日志 | `BOARD_OK PM1=1 IOE1=1 WAKE=none` |

## 运行模式

共享 I2C 总线、PM1 和 IOE1 完成初始化并应用无唤醒 L2 准备后，输出 `BOARD_OK` 和 `TEST_READY`，等待 3000 ms，再进入 ESP32-S3 Deep Sleep。

## 测量电流

测量结果中的电流为测量记录中的 `WINDOW Avg`。

- 3.7 V：129.08 uA
- 4.2 V：81.06 uA

参见[功耗测量曲线索引](../../docs/power-graphs/README_CN.md)。

## 配置

| 设置 | 数值 |
|---|---|
| 目标 | ESP32-S3 |
| Flash | 16 MB |
| PSRAM | OPI，80 MHz |
| CPU | 240 MHz |
| I2C | 使用时为 100 kHz |
| Test ID | `l2_deep_sleep` |
| Config ID | `l2_deep_sleep` |

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
