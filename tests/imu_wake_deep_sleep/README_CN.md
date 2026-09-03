# IMU 深度睡眠唤醒

[English](README.md) | [简体中文](README_CN.md)

## 目的

本固件将 BMI270 任意运动检测配置为唤醒源，并测量 ESP32-S3 保持 Deep Sleep 时的整板电流。

## 启用模块

- `m5pm_test_support`：保持测试标记、唤醒原因报告和安全保持状态
- `m5pm_board`：共享 I2C 初始化和总线所有权
- `m5pm_power`：PM1/IOE1 L2 准备和 ESP32-S3 Deep Sleep 进入
- `m5pm_imu_wakeup`：BMI270 任意运动配置和保持状态读取

## 模块配置

| 模块 | 配置 |
|---|---|
| PM1 | 使用共享 100 kHz I2C 初始化；L2 IMU 唤醒准备将 GPIO4 配置为唤醒输入 |
| IOE1 | 地址 `0x4F`；聚合 GPIO1 将 PM1 GPIO4 中断转发到 ESP32-S3 EXT0 |
| BMI270 API | Bosch BMI270 Base API，通过 I2C 访问；地址 `0x68`、100 kHz、`read_write_len=30`，不加载配置文件；IMU 由保持的 `3V3_L1` 电源轨供电 |
| 低功耗档位 | 加速度计：50 Hz、+/-2 g、功耗优化滤波、`OSR4_AVG1`；陀螺仪寄存器配置为 25 Hz、2000 dps、功耗优化滤波 / 噪声模式、`OSR4`、OIS 量程 2000 dps，但不启用陀螺仪；启用加速度计和任意运动 |
| 参考档位 | 加速度计：100 Hz、+/-2 g、性能优化滤波、`OSR2_AVG2`；陀螺仪：100 Hz、2000 dps、性能优化滤波和功耗优化噪声模式、`OSR2`、OIS 量程 2000 dps；启用加速度计、陀螺仪、腕戴唤醒和任意运动 |
| 任意运动 | 持续时间 `10`、阈值 `30`，启用 X/Y/Z 轴；任意运动映射到 BMI270 INT1 |
| BMI270 中断 | INT1 低电平有效、推挽、输出启用、输入禁用、非锁存 |
| 唤醒链路 | BMI270 中断 -> PM1 GPIO4 -> IOE1 聚合 GPIO1 -> ESP32-S3 `ESP_SLEEP_WAKEUP_EXT0` |
| 配置选择 | `M5PM_IMU_WAKE_REFERENCE_PROFILE=0` 选择 `PROFILE=low_power`；设为 `1` 后重新构建选择 `PROFILE=reference` |
| 固件日志 | `BOARD_OK PM1=1 IOE1=1 IMU=1 PROFILE=%s WAKE=imu` |

## 运行模式

启动时先在重新初始化器件前读取保持的 BMI270 和 PM1 状态。正常启动时，测试准备 L2 IMU 唤醒链路，配置 BMI270 任意运动，释放 I2C 句柄，输出 `BOARD_OK`、`WAKE_ARMED` 和 `TEST_READY`，等待 3 秒后进入 Deep Sleep。唤醒后仅接受 ESP32 EXT0、PM1 GPIO4 和 BMI270 任意运动证据；其他 PM1/GPIO/系统/按键事件会被拒绝。

## 工作模式

配置档位由源码宏选择，不是 Kconfig 选项：

| 档位 | 源码设置 | 3.7 V `WINDOW Avg` | 4.2 V `WINDOW Avg` |
|---|---|---:|---:|
| `low_power` | `#define M5PM_IMU_WAKE_REFERENCE_PROFILE 0` | 726.19 uA | 717.64 uA |
| `reference` | 构建前将宏改为 `1` | 1.38 mA | 1.36 mA |

## 测量电流

上表数值为每种档位约 60 秒采集中的 `WINDOW Avg`。参见[功耗测量曲线索引](../../docs/power-graphs/README_CN.md)。

## 配置

| 设置 | 数值 |
|---|---|
| 目标 | ESP32-S3 |
| Test ID | `imu_wake_deep_sleep` |
| Config ID | `imu_wake_deep_sleep` |
| 准备延时 | 进入 Deep Sleep 前 3000 ms |
| 唤醒验证 | ESP32 EXT0、PM1 GPIO4、BMI270 任意运动；其他事件必须为空 |

## 构建与烧录

先在仓库根目录运行 `python fetch_repos.py check`；如果组件缺失，再运行 `python fetch_repos.py fetch --skip-existing`。然后在本目录构建：

```bash
idf.py set-target esp32s3
idf.py build
idf.py -p <PORT> flash
idf.py -p <PORT> monitor
```

使用监视器确认 `TEST_START`、`BOARD_OK`、`WAKE_ARMED` 和 `TEST_READY`，测量电流前断开 USB Type-C。测量 `reference` 档位时，先按上文修改宏并重新构建、烧录。

## 预期验证

睡眠前日志必须包含 `PROFILE=low_power` 或 `PROFILE=reference` 以及规定的唤醒链路。唤醒记录只有在预期的 EXT0、PM1 GPIO4 和 BMI270 任意运动标志同时出现时才符合该测试的验证条件。
