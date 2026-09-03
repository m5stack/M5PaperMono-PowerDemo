# IMU 采样

[English](README.md) | [简体中文](README_CN.md)

## 目的

本固件以固定配置连续采样 BMI270 的加速度计和陀螺仪，并测量采样期间的整板输入电流。

## 启用模块

- `m5pm_test_support`：测试标记和安全保持状态
- `m5pm_phase_test`：awake L2 基线和测量计时
- `m5pm_power` 与 `m5pm_imu`：BMI270 电源恢复、初始化和采样

## 模块配置

| 模块 | 配置 |
|---|---|
| BMI270 API | Bosch BMI270 Base API，通过 I2C 访问；地址 `0x68`、100 kHz、`read_write_len=30`，不加载配置文件；IMU 由 `3V3_L1` 电源轨供电 |
| 加速度计 | 已启用；性能优化滤波、`OSR2_AVG2` 带宽、100 Hz ODR、+/-2 g 量程 |
| 陀螺仪 | 已启用；性能优化滤波、功耗优化噪声模式、`OSR2` 带宽、100 Hz ODR、2000 dps 量程，OIS 量程 2000 dps |
| 传感器集合 | 仅启用加速度计和陀螺仪；本采样器不启用 AUX、温度和唤醒功能 |
| 采样 | 通过 `Sampler::read()` 调用 `bmi2_get_sensor_data()`；每 10 ms 读取一次，共 9000 次 |
| 唤醒 / 显示 | 不配置唤醒中断；关闭显示刷新 |
| 测试状态 | ESP32-S3 在固定采样循环中保持运行 |
| 固件日志 | `BOARD_OK BASELINE=l2-awake IMU=bmi270 ACCEL=on GYRO=on ODR_HZ=100 READ_PERIOD_MS=10 WAKE_INTERRUPT=0 DISPLAY_REFRESH=0` |

## 运行模式

测试恢复 IMU 电源，使用固定的 BMI270 Base API 加速度计和陀螺仪配置进行初始化，读取一个样本，输出 `BOARD_OK` 和 `TEST_READY`，随后每 10 ms 读取一次，共 9000 个样本，然后进入安全保持状态。

## 测量电流

测量结果中的电流为测量记录中的 `WINDOW Avg`。

- 3.7 V：31.63 mA
- 4.2 V：28.99 mA

参见[功耗测量曲线索引](../../docs/power-graphs/README_CN.md)。

## 配置

| 设置 | 数值 |
|---|---|
| 目标 | ESP32-S3 |
| Flash | 16 MB |
| PSRAM | OPI，80 MHz |
| CPU | 240 MHz |
| I2C | 使用时为 100 kHz |
| Test ID | `imu_sampling` |
| Config ID | `imu_sampling` |

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
