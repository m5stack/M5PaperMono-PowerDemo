cmake_minimum_required(VERSION 3.16)
set(ENV{IDF_COMPONENT_MANAGER} 0)
set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/../../components/espressif__cmake_utilities"
    "${CMAKE_CURRENT_LIST_DIR}/../../components/espressif__i2c_bus"
    "${CMAKE_CURRENT_LIST_DIR}/../../components/M5IOE1"
    "${CMAKE_CURRENT_LIST_DIR}/../../components/M5PM1"
    "${CMAKE_CURRENT_LIST_DIR}/../../modules/m5pm_board"
    "${CMAKE_CURRENT_LIST_DIR}/../../modules/m5pm_phase_test"
    "${CMAKE_CURRENT_LIST_DIR}/../../modules/m5pm_power"
    "${CMAKE_CURRENT_LIST_DIR}/../../modules/m5pm_test_support"
    "${CMAKE_CURRENT_LIST_DIR}/../../modules/m5pm_wifi"
)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(m5pm_wifi_periodic_traffic)
