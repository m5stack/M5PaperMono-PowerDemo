idf_component_register(
    SRCS "m5pm_rtc_test.cpp"
    INCLUDE_DIRS "include"
    REQUIRES freertos log m5pm_board m5pm_power m5pm_rtc_wakeup m5pm_test_support
)
