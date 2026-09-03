cmake_minimum_required(VERSION 3.16)

set(ENV{IDF_COMPONENT_MANAGER} 0)

set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/../../components/espressif__cmake_utilities"
    "${CMAKE_CURRENT_LIST_DIR}/../../components/espressif__i2c_bus"
    "${CMAKE_CURRENT_LIST_DIR}/../../components/M5PM1"
    "${CMAKE_CURRENT_LIST_DIR}/../../components/M5IOE1"
    "${CMAKE_CURRENT_LIST_DIR}/../../components/RadioLib"
    "${CMAKE_CURRENT_LIST_DIR}/../../components/M5GFX"
    "${CMAKE_CURRENT_LIST_DIR}/../../components/M5HAL"
    "${CMAKE_CURRENT_LIST_DIR}/../../components/M5Utility"
    "${CMAKE_CURRENT_LIST_DIR}/../../modules/m5pm_display"
    "${CMAKE_CURRENT_LIST_DIR}/../../modules/m5pm_lora"
    "${CMAKE_CURRENT_LIST_DIR}/../../modules/m5pm_power"
    "${CMAKE_CURRENT_LIST_DIR}/../../modules/m5pm_phase_test"
    "${CMAKE_CURRENT_LIST_DIR}/../../modules/m5pm_test_support"
    "${CMAKE_CURRENT_LIST_DIR}/../../modules/m5pm_board"
    "${CMAKE_CURRENT_LIST_DIR}/../../modules/m5pm_nfc"
    "${CMAKE_CURRENT_LIST_DIR}/../../modules/m5pm_unit_unified"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(m5pm_full_load)
