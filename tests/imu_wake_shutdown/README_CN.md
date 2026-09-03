# IMU 关机唤醒

[English](README.md) | [简体中文](README_CN.md)

## 目的

本固件将 BMI270 任意运动检测配置为唤醒源，并测量 PM1 保持整板 shutdown 时的整板电流。

## 启用模块

- `m5pm_test_support`：测试标记、保持的唤醒状态和安全保持状态
- `m5pm_board`：共享 I2C 初始化和总线所有权
- `m5pm_power`：PM1 shutdown 准备和 GPIO4 唤醒保持
- `m5pm_imu_wakeup`：BMI270 任意运动配置和状态读取

## 模块配置

| 模块 | 配置 |
|---|---|
| PM1 | 使用共享 100 kHz I2C 初始化；shutdown 唤醒准备保持 `3V3_L1` LDO 电源域并配置 GPIO4 |
| BMI270 API | Bosch BMI270 Base API，通过 I2C 访问；地址 `0x68`、100 kHz、`read_write_len=30`，不加载配置文件 |
| 低功耗档位 | 加速度计：50 Hz、+/-2 g、功耗优化滤波、`OSR4_AVG1`；陀螺仪寄存器配置为 25 Hz、2000 dps、功耗优化滤波 / 噪声模式、`OSR4`、OIS 量程 2000 dps，但不启用陀螺仪；启用加速度计和任意运动 |
| 参考档位 | 加速度计：100 Hz、+/-2 g、性能优化滤波、`OSR2_AVG2`；陀螺仪：100 Hz、2000 dps、性能优化滤波和功耗优化噪声模式、`OSR2`、OIS 量程 2000 dps；启用加速度计、陀螺仪、腕戴唤醒和任意运动 |
| 任意运动 | 持续时间 `10`、阈值 `30`，启用 X/Y/Z 轴；任意运动映射到 BMI270 INT1 |
| BMI270 中断 | INT1 低电平有效、推挽、输出启用、输入禁用、非锁存；中断驱动 PM1 GPIO4 |
| IOE1 | 本 shutdown 链路不初始化；不需要 IOE1 聚合 GPIO |
| 唤醒链路 | BMI270 中断 -> PM1 GPIO4 -> PM1 外部唤醒 -> shutdown 重启 |
| 配置选择 | `M5PM_IMU_WAKE_REFERENCE_PROFILE=0` 选择 `PROFILE=low_power`；设为 `1` 后重新构建选择 `PROFILE=reference` |
| 固件日志 | `BOARD_OK PM1=1 IMU=1 PROFILE=%s WAKE=imu` |

## 运行模式

测试读取保持的 BMI270 和 PM1 状态，准备 PM1 shutdown 唤醒链路，配置 BMI270 任意运动，释放 I2C 句柄，输出 `BOARD_OK`、`WAKE_ARMED` 和 `TEST_READY`，等待 3 秒后请求 shutdown。重启后检查 PM1 外部唤醒证据和 BMI270 任意运动状态；其他 GPIO、系统或按键事件会使唤醒记录无效。

## 工作模式

配置档位由源码宏选择：

| 档位 | 源码设置 | 3.7 V `WINDOW Avg` | 4.2 V `WINDOW Avg` |
|---|---|---:|---:|
| `low_power` | `#define M5PM_IMU_WAKE_REFERENCE_PROFILE 0` | 42.72 uA | 45.82 uA |
| `reference` | 构建前将宏改为 `1` | 692.80 uA | 696.20 uA |

## 测量电流

上表数值为每种档位约 60 秒采集中的 `WINDOW Avg`。参见[功耗测量曲线索引](../../docs/power-graphs/README_CN.md)。

## 配置

| 设置 | 数值 |
|---|---|
| 目标 | ESP32-S3 |
| Test ID | `imu_wake_shutdown` |
| Config ID | `imu_wake_shutdown` |
| 准备延时 | 请求 shutdown 前 3000 ms |
| 唤醒验证 | PM1 外部唤醒和 BMI270 任意运动；其他 PM1 事件必须为空 |

## 构建与烧录

先在仓库根目录运行 `python fetch_repos.py check`；如果组件缺失，再运行 `python fetch_repos.py fetch --skip-existing`。然后在本目录构建并烧录：

```bash
idf.py set-target esp32s3
idf.py build
idf.py -p <PORT> flash
idf.py -p <PORT> monitor
```

通过 USB Type-C 确认 `TEST_START`、`BOARD_OK`、`WAKE_ARMED` 和 `TEST_READY`，然后在测量电流前断开 USB Type-C。切换档位时修改宏并重新构建。

## 预期验证

关机前日志必须报告所选档位和 `path=pm1_gpio4+shutdown`。后续唤醒记录只有在 PM1 外部唤醒和 BMI270 任意运动证据同时存在且无其他事件时才符合验证条件。
