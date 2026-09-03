# LoRa 周期发送

[English](README.md) | [简体中文](README_CN.md)

## 目的

本固件同时测量 SX1262 的稳定周期发送负载和一次完整发送事件。

## 启用模块

- `m5pm_lora`：SX1262 初始化、发送、停止和电源控制接口
- `m5pm_phase_test`：awake L2 基线、停止按键处理和安全保持状态
- `m5pm_power`：通过 PM1/IOE1 恢复和关闭 LoRa 电源
- `m5pm_test_support`：测试标记和验证上下文

## 模块配置

| 模块 | 配置 |
|---|---|
| SX1262 | 868 MHz、125 kHz 带宽、SF12、CR 4/5、同步字 `0x34`、发射功率 22 dBm；SPI3 SCLK GPIO39、MOSI GPIO38、MISO GPIO40、NSS GPIO41、IRQ GPIO5、BUSY GPIO21；通过 PM1 GPIO2 使能 `3V3_L2_LoRa` 电源轨 |
| 射频支持 | 前导码 8 个符号、TCXO 3.0 V、LDO 稳压器 |
| 板级控制 | PM1 地址 `0x6E`、IOE1 地址 `0x4F`，使用共享 100 kHz I2C 总线；IOE1 控制 LoRa 复位和天线开关引脚 |
| 负载 | ASCII `PM_n`，序号从 0 开始 |
| 调度 | 每次发送完成后等待 1,000 ms；单次发送超时 10,000 ms；运行上限 90,000 ms |
| 停止输入 | Key1 或 Key2，由 `configure_stop_buttons()` 配置 |
| 固件日志 | `BOARD_OK BASELINE=l2-awake RADIO=sx1262 FREQ_MHZ=868 BW_KHZ=125 SF=12 CR=4/5 SYNC=0x34 TX_DBM=22 PREAMBLE=8 TCXO_V=3.0 REGULATOR=ldo PAYLOAD=PM_n PERIOD_AFTER_TX_MS=1000 ACK=0 RETRY=0 LOCAL_TX_DONE=required RUNTIME_LIMIT_MS=90000 STOP=key1_or_key2` |

## 运行模式

测试恢复 LoRa 电源，配置停止按键，初始化无线电并输出 `TEST_READY`。随后发送连续的 `PM_n` 负载，每次发送完成后等待 1 s，直到达到 90 s 上限或收到停止按键请求。测试在进入安全保持状态前停止无线电并关闭 LoRa 电源。

## 测量电流

周期发送结果为约 60 秒采集中的 `WINDOW Avg`。单次发送结果使用覆盖一次 TX 事件的 `CURSOR Avg`。

| 测量项目 | 3.7 V | 4.2 V |
|---|---:|---:|
| 周期发送（`WINDOW Avg`） | 61.25 mA | 59.50 mA |
| 单次发送（`CURSOR Avg`） | 96.55 mA，0.840053 s；22.52891 uAh | 93.52 mA，0.840053 s；21.82189 uAh |

参见[功耗测量曲线索引](../../docs/power-graphs/README_CN.md)。

## 配置

| 设置 | 数值 |
|---|---|
| 目标 | ESP32-S3 |
| Test ID | `lora_periodic_transmit` |
| Config ID | `lora_periodic_transmit` |
| 测试结束时的无线电状态 | SX1262 已停止；LoRa 电源已关闭 |

## 构建与烧录

先在仓库根目录运行 `python fetch_repos.py check`；如果组件缺失，再运行 `python fetch_repos.py fetch --skip-existing`。然后在本目录构建：

```bash
idf.py set-target esp32s3
idf.py build
idf.py -p <PORT> flash
idf.py -p <PORT> monitor
```

通过 USB Type-C 确认 `TEST_START`、完整的 `BOARD_OK` 无线参数和 `TEST_READY`，然后在测量前断开 USB Type-C。单次事件采集时，按曲线索引中的方式将游标设置为恰好覆盖一个 TX 脉冲。

## 预期验证

每个发送负载必须先在本地完成，之后才开始 1 s 间隔。手动停止由配置的 Key1/Key2 输入触发；否则由运行时间上限结束周期序列。
