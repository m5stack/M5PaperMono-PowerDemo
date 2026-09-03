# L1 待机

[English](README.md) | [简体中文](README_CN.md)

## 目的

本固件用于测量设备处于 PM1 L1 standby 时的整板输入电流。关机前将 BMI270 写入低功耗寄存器状态；NFC 不属于 L1 保持的电源域。

## 启用模块

- `m5pm_test_support`：测试标记和安全保持状态
- `m5pm_board`：共享 I2C 初始化和总线所有权
- `m5pm_power`：PM1 初始化、BMI270 低功耗准备和 L1 待机转换

## 模块配置

| 模块 | 配置 |
|---|---|
| ESP32-S3 | 不初始化应用外设；由 PM1 关闭处理器 |
| I2C | 共享板级总线；PM1 和 BMI270 准备阶段使用 100 kHz 事务 |
| PM1 | 地址 `0x6E`；通过 `PowerController::begin()` 初始化；进入 shutdown 前关闭充电并保持 LDO |
| BMI270 | 地址 `0x68`；写入 `PWR_CONF (0x7C)=0x01` 和 `PWR_CTRL (0x7D)=0x00`，两次写入之间延时 100 ms |
| NFC 电源域 | ST25R3916 由 `3V3_L2` 经 `PYB_NFC_EN` 负载开关供电；该电源轨不会在 L1 中保持，本测试不访问 NFC 器件 |
| 测试状态 | PM1 L1 standby；保持板级 L1 负载使用的 `3V3_L1` 电源域 |
| 固件日志 | `BOARD_OK PM1=1` |

## 运行模式

共享 I2C 总线和 PM1 完成初始化，写入 BMI270 低功耗寄存器后输出 `BOARD_OK` 和 `TEST_READY`，等待 3000 ms。随后由 `enter_l1()` 请求 PM1 L1 standby。转换后不再使用串口和 I2C；NFC 使用的 `3V3_L2` 电源轨不属于 L1 保持的电源域。

## 测量电流

测量结果中的电流为测量记录中的 `WINDOW Avg`。

- 3.7 V：26.07 uA
- 4.2 V：29.09 uA

参见[功耗测量曲线索引](../../docs/power-graphs/README_CN.md)。

## 配置

| 设置 | 数值 |
|---|---|
| 目标 | ESP32-S3 |
| Flash | 16 MB |
| PSRAM | OPI，80 MHz |
| CPU | 240 MHz |
| I2C | 使用时为 100 kHz |
| Test ID | `l1_standby` |
| Config ID | `l1_standby` |

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
