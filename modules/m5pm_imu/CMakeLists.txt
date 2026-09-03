idf_component_register(
    SRCS "m5pm_imu.cpp"
    INCLUDE_DIRS "include"
    REQUIRES BMI270 espressif__i2c_bus esp_hw_support m5pm_board
)
