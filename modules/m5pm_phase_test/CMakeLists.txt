idf_component_register(
    SRCS "m5pm_phase_test.cpp"
    INCLUDE_DIRS "include"
    REQUIRES esp_driver_gpio freertos m5pm_board m5pm_power m5pm_test_support
)
