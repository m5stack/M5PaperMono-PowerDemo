# 功耗测量曲线

[English](README.md) | [简体中文](README_CN.md)

每张图片展示整板输入电流测量窗口。稳态项目采用对应测量记录中的 `WINDOW Avg`；显示刷新和 LoRa 单次发射项目采用完整事件对应的 `CURSOR Avg`。

## 低功耗状态

### L0 关机
3.7 V

<img src="l0_shutdown/3.7v.png" alt="L0 关机，3.7 V" width="60%">

4.2 V

<img src="l0_shutdown/4.2v.png" alt="L0 关机，4.2 V" width="60%">

### L1 待机
3.7 V

<img src="l1_standby/3.7v.png" alt="L1 待机，3.7 V" width="60%">

4.2 V

<img src="l1_standby/4.2v.png" alt="L1 待机，4.2 V" width="60%">

### L2 深度睡眠
3.7 V

<img src="l2_deep_sleep/3.7v.png" alt="L2 深度睡眠，3.7 V" width="60%">

4.2 V

<img src="l2_deep_sleep/4.2v.png" alt="L2 深度睡眠，4.2 V" width="60%">

## IMU 唤醒

### 从深度睡眠唤醒
3.7 V - `low_power`

<img src="imu_wake_deep_sleep/3.7v-low-power.png" alt="IMU 从深度睡眠唤醒，low_power，3.7 V" width="60%">

3.7 V - `reference`

<img src="imu_wake_deep_sleep/3.7v.png" alt="IMU 从深度睡眠唤醒，reference，3.7 V" width="60%">

4.2 V - `low_power`

<img src="imu_wake_deep_sleep/4.2v-low-power.png" alt="IMU 从深度睡眠唤醒，low_power，4.2 V" width="60%">

4.2 V - `reference`

<img src="imu_wake_deep_sleep/4.2v.png" alt="IMU 从深度睡眠唤醒，reference，4.2 V" width="60%">

### 从关机唤醒
3.7 V - `low_power`

<img src="imu_wake_shutdown/3.7v-low-power.png" alt="IMU 从关机唤醒，low_power，3.7 V" width="60%">

3.7 V - `reference`

<img src="imu_wake_shutdown/3.7v.png" alt="IMU 从关机唤醒，reference，3.7 V" width="60%">

4.2 V - `low_power`

<img src="imu_wake_shutdown/4.2v-low-power.png" alt="IMU 从关机唤醒，low_power，4.2 V" width="60%">

4.2 V - `reference`

<img src="imu_wake_shutdown/4.2v.png" alt="IMU 从关机唤醒，reference，4.2 V" width="60%">

## 按键唤醒

### 从深度睡眠唤醒
3.7 V

<img src="button_wake_deep_sleep/3.7v.png" alt="按键从深度睡眠唤醒，3.7 V" width="60%">

4.2 V

<img src="button_wake_deep_sleep/4.2v.png" alt="按键从深度睡眠唤醒，4.2 V" width="60%">

## RTC 唤醒

### RTC Alarm 从深度睡眠唤醒
3.7 V

<img src="rtc_alarm_wake_deep_sleep/3.7v.png" alt="RTC Alarm 从深度睡眠唤醒，3.7 V" width="60%">

4.2 V

<img src="rtc_alarm_wake_deep_sleep/4.2v.png" alt="RTC Alarm 从深度睡眠唤醒，4.2 V" width="60%">

### RTC Alarm 从关机唤醒
3.7 V

<img src="rtc_alarm_wake_shutdown/3.7v.png" alt="RTC Alarm 从关机唤醒，3.7 V" width="60%">

4.2 V

<img src="rtc_alarm_wake_shutdown/4.2v.png" alt="RTC Alarm 从关机唤醒，4.2 V" width="60%">

### RTC Timer 从深度睡眠唤醒
3.7 V

<img src="rtc_timer_wake_deep_sleep/3.7v.png" alt="RTC Timer 从深度睡眠唤醒，3.7 V" width="60%">

4.2 V

<img src="rtc_timer_wake_deep_sleep/4.2v.png" alt="RTC Timer 从深度睡眠唤醒，4.2 V" width="60%">

### RTC Timer 从关机唤醒
3.7 V

<img src="rtc_timer_wake_shutdown/3.7v.png" alt="RTC Timer 从关机唤醒，3.7 V" width="60%">

4.2 V

<img src="rtc_timer_wake_shutdown/4.2v.png" alt="RTC Timer 从关机唤醒，4.2 V" width="60%">

## 工作负载

### 正常工作
3.7 V

<img src="normal_active/3.7v.png" alt="正常工作，3.7 V" width="60%">

4.2 V

<img src="normal_active/4.2v.png" alt="正常工作，4.2 V" width="60%">

### 显示负载
3.7 V

<img src="display_only/3.7v.png" alt="显示负载，3.7 V" width="60%">

4.2 V

<img src="display_only/4.2v.png" alt="显示负载，4.2 V" width="60%">

### 显示刷新模式
3.7 V - `epd_fast`

<img src="display_refresh_modes/3.7v-fast.png" alt="显示 epd_fast，3.7 V" width="60%">

3.7 V - `epd_fastest`

<img src="display_refresh_modes/3.7v-fastest.png" alt="显示 epd_fastest，3.7 V" width="60%">

3.7 V - `epd_quality`

<img src="display_refresh_modes/3.7v-quality.png" alt="显示 epd_quality，3.7 V" width="60%">

4.2 V - `epd_fast`

<img src="display_refresh_modes/4.2v-fast.png" alt="显示 epd_fast，4.2 V" width="60%">

4.2 V - `epd_fastest`

<img src="display_refresh_modes/4.2v-fastest.png" alt="显示 epd_fastest，4.2 V" width="60%">

4.2 V - `epd_quality`

<img src="display_refresh_modes/4.2v-quality.png" alt="显示 epd_quality，4.2 V" width="60%">

### 前光档位
3.7 V - `frontlight-0`

<img src="frontlight_levels/3.7v-0-percent.png" alt="前光 0%，3.7 V" width="60%">

3.7 V - `frontlight-50`

<img src="frontlight_levels/3.7v-50-percent.png" alt="前光 50%，3.7 V" width="60%">

3.7 V - `frontlight-100`

<img src="frontlight_levels/3.7v-100-percent.png" alt="前光 100%，3.7 V" width="60%">

4.2 V - `frontlight-0`

<img src="frontlight_levels/4.2v-0-percent.png" alt="前光 0%，4.2 V" width="60%">

4.2 V - `frontlight-50`

<img src="frontlight_levels/4.2v-50-percent.png" alt="前光 50%，4.2 V" width="60%">

4.2 V - `frontlight-100`

<img src="frontlight_levels/4.2v-100-percent.png" alt="前光 100%，4.2 V" width="60%">

### IMU 采样
3.7 V

<img src="imu_sampling/3.7v.png" alt="IMU 采样，3.7 V" width="60%">

4.2 V

<img src="imu_sampling/4.2v.png" alt="IMU 采样，4.2 V" width="60%">

### NFC 工作电流
3.7 V - `nfc-a-polling`

<img src="nfc_working_current/3.7v-nfc-a-polling.png" alt="NFC-A polling，3.7 V" width="60%">

3.7 V - `nfc-power-down`

<img src="nfc_working_current/3.7v-power-down.png" alt="NFC Power Down，3.7 V" width="60%">

4.2 V - `nfc-a-polling`

<img src="nfc_working_current/4.2v-nfc-a-polling.png" alt="NFC-A polling，4.2 V" width="60%">

4.2 V - `nfc-power-down`

<img src="nfc_working_current/4.2v-power-down.png" alt="NFC Power Down，4.2 V" width="60%">

### LoRa 周期发送
3.7 V - 单次发射（`CURSOR`）

<img src="lora_periodic_transmit/3.7v-single-send.png" alt="LoRa 单次发射，3.7 V" width="60%">

3.7 V - 周期发送（`WINDOW`）

<img src="lora_periodic_transmit/3.7v.png" alt="LoRa 周期发送，3.7 V" width="60%">

4.2 V - 单次发射（`CURSOR`）

<img src="lora_periodic_transmit/4.2v-single-send.png" alt="LoRa 单次发射，4.2 V" width="60%">

4.2 V - 周期发送（`WINDOW`）

<img src="lora_periodic_transmit/4.2v.png" alt="LoRa 周期发送，4.2 V" width="60%">

### LoRa RX 空闲
3.7 V

<img src="lora_rx_idle/3.7v.png" alt="LoRa RX 空闲，3.7 V" width="60%">

4.2 V

<img src="lora_rx_idle/4.2v.png" alt="LoRa RX 空闲，4.2 V" width="60%">

### LoRa 睡眠
3.7 V

<img src="lora_sleep/3.7v.png" alt="LoRa 睡眠，3.7 V" width="60%">

4.2 V

<img src="lora_sleep/4.2v.png" alt="LoRa 睡眠，4.2 V" width="60%">

## Wi-Fi 工作负载

### Wi-Fi 已连接
3.7 V

<img src="wifi_connected/3.7v.png" alt="Wi-Fi 已连接，3.7 V" width="60%">

4.2 V

<img src="wifi_connected/4.2v.png" alt="Wi-Fi 已连接，4.2 V" width="60%">

### Wi-Fi Light Sleep
3.7 V

<img src="wifi_light_sleep/3.7v.png" alt="Wi-Fi Light Sleep，3.7 V" width="60%">

4.2 V

<img src="wifi_light_sleep/4.2v.png" alt="Wi-Fi Light Sleep，4.2 V" width="60%">

### Wi-Fi 周期流量
3.7 V

<img src="wifi_periodic_traffic/3.7v.png" alt="Wi-Fi 周期流量，3.7 V" width="60%">

4.2 V

<img src="wifi_periodic_traffic/4.2v.png" alt="Wi-Fi 周期流量，4.2 V" width="60%">

## 满载
3.7 V

<img src="full_load/3.7v.png" alt="满载，3.7 V" width="60%">

4.2 V

<img src="full_load/4.2v.png" alt="满载，4.2 V" width="60%">

## 测量说明

- 测量时断开 USB Type-C，仅通过测量装置为设备供电。
- 数据发布使用标称 3.7 V 和 4.2 V。
