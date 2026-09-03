idf_component_register(
    SRCS "m5pm_wifi.cpp"
    INCLUDE_DIRS "include"
    REQUIRES esp_event esp_netif esp_wifi freertos lwip nvs_flash
)
