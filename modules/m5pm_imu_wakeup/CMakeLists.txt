idf_component_register(
    SRCS "m5pm_imu_wakeup.cpp"
    INCLUDE_DIRS "include"
    REQUIRES BMI270 esp_rom log m5pm_board
)
