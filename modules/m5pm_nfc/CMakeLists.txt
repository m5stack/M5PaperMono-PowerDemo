# Build the fetched M5Unit-NFC sources through a project-owned adapter.
# The upstream component advertises M5Unified as a public dependency, but this
# ESP-IDF test uses UnitUnified's native i2c_master bus and must keep the
# M5Unified I2C_Class backend out of the PM1/IOE1 dependency graph.
file(GLOB_RECURSE M5PM_NFC_SRCS
     "${CMAKE_CURRENT_LIST_DIR}/../../components/M5Unit-NFC/src/*.cpp")

idf_component_register(
    SRCS ${M5PM_NFC_SRCS}
    INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/../../components/M5Unit-NFC/src"
    REQUIRES m5pm_unit_unified M5Utility
    PRIV_REQUIRES driver mbedtls
)
