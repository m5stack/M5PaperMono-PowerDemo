idf_component_register(
    SRCS "m5pm_power.cpp"
    INCLUDE_DIRS "include"
    REQUIRES esp_driver_gpio esp_hw_support freertos M5PM1 m5pm_board
)

idf_component_optional_requires(PUBLIC M5IOE1)
