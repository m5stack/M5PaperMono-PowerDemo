idf_component_register(
    SRCS "m5pm_board.cpp"
    INCLUDE_DIRS "include"
    REQUIRES driver freertos espressif__i2c_bus
)
