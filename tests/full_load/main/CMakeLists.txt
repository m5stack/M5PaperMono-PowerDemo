idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES driver esp_wifi esp_event esp_netif nvs_flash freertos espressif__i2c_bus log
             m5pm_board m5pm_test_support m5pm_display m5pm_lora m5pm_power m5pm_phase_test M5PM1 RadioLib M5GFX
             m5pm_nfc m5pm_unit_unified M5Utility M5HAL
)
