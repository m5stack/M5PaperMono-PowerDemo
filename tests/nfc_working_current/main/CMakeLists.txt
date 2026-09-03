idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES driver freertos espressif__i2c_bus log m5pm_board m5pm_power m5pm_phase_test
             m5pm_test_support m5pm_nfc m5pm_unit_unified M5PM1 M5IOE1 M5HAL M5Utility
)
