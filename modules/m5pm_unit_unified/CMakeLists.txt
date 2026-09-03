# Build the fetched M5UnitUnified sources through a project-owned native-I2C
# adapter. This avoids registering the upstream component, whose CMake file
# requires M5Unified even when only the ESP-IDF native bus is used.
file(GLOB_RECURSE M5PM_UNIT_UNIFIED_SRCS
     "${CMAKE_CURRENT_LIST_DIR}/../../components/M5UnitUnified/src/*.cpp")
list(FILTER M5PM_UNIT_UNIFIED_SRCS EXCLUDE REGEX "/googletest/")

idf_component_register(
    SRCS ${M5PM_UNIT_UNIFIED_SRCS}
    INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/../../components/M5UnitUnified/src"
    REQUIRES M5HAL M5Utility
    PRIV_REQUIRES driver esp_adc esp_timer
)
