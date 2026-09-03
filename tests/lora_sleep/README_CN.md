# LoRa 睡眠

[English](README.md) | [简体中文](README_CN.md)

## 目的

本固件初始化 SX1262 后使其进入 Sleep 状态，并测量无线电睡眠期间的整板输入电流。

## 启用模块

- `m5pm_test_support`：测试标记和安全保持状态
- `m5pm_phase_test`：awake L2 基线和 90 秒测量计时
- `m5pm_lora` 与 `m5pm_power`：SX1262 睡眠模式和 LoRa 电源控制

## 模块配置

| 模块 | 配置 |
|---|---|
| SX1262 | 868 MHz、125 kHz、SF12、CR 4/5、同步字 `0x34`、发射功率 22 dBm；SPI3 SCLK GPIO39、MOSI GPIO38、MISO GPIO40、NSS GPIO41、IRQ GPIO5、BUSY GPIO21；通过 PM1 GPIO2 使能 `3V3_L2_LoRa` 电源轨 |
| 射频支持 | 前导码 8 个符号、TCXO 3.0 V、LDO 稳压器 |
| 板级控制 | PM1 地址 `0x6E`、IOE1 地址 `0x4F`，使用共享 100 kHz I2C 总线；IOE1 控制 LoRa 复位和天线开关引脚 |
| 无线电状态 | Sleep；禁用 TX 和 RX |
| 测量 | `radio.sleep()` 后保持 90,000 ms |
| 固件日志 | `BOARD_OK BASELINE=l2-awake RADIO=sx1262 FREQ_MHZ=868 BW_KHZ=125 SF=12 CR=4/5 SYNC=0x34 TX_DBM=22 PREAMBLE=8 TCXO_V=3.0 REGULATOR=ldo STATE=sleep TX=0 RX=0` |

## 运行模式

测试恢复 LoRa 电源，初始化 SX1262，请求 sleep，输出 `BOARD_OK` 和 `TEST_READY`，保持 sleep 状态 90 s 后进入安全保持状态。

## 测量电流

测量结果中的电流为测量记录中的 `WINDOW Avg`。

- 3.7 V：31.71 mA
- 4.2 V：29.68 mA

参见[功耗测量曲线索引](../../docs/power-graphs/README_CN.md)。

## 配置

| 设置 | 数值 |
|---|---|
| 目标 | ESP32-S3 |
| Flash | 16 MB |
| PSRAM | OPI，80 MHz |
| CPU | 240 MHz |
| I2C | 使用时为 100 kHz |
| Test ID | `lora_sleep` |
| Config ID | `lora_sleep` |

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
