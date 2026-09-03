idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES freertos log m5pm_board m5pm_imu_wakeup m5pm_power m5pm_rtc_wakeup m5pm_test_support
)
