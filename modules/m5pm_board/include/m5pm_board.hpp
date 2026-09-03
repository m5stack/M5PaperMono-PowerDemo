#pragma once

#include <cstdint>

#include "driver/gpio.h"
#include "esp_err.h"
#include "i2c_bus.h"

namespace m5pm::board {

inline constexpr gpio_num_t kI2cSda = GPIO_NUM_47;
inline constexpr gpio_num_t kI2cScl = GPIO_NUM_48;

inline constexpr gpio_num_t kAggregateIrq = GPIO_NUM_1;
inline constexpr gpio_num_t kKey1         = GPIO_NUM_2;
inline constexpr gpio_num_t kKey2         = GPIO_NUM_3;
inline constexpr gpio_num_t kTouchIrq     = GPIO_NUM_4;
inline constexpr gpio_num_t kLoraIrq      = GPIO_NUM_5;
inline constexpr gpio_num_t kImuIrq       = GPIO_NUM_6;

inline constexpr std::uint8_t kPm1Address   = 0x6E;
inline constexpr std::uint8_t kIoe1Address  = 0x4F;
inline constexpr std::uint8_t kNfcAddress   = 0x50;
inline constexpr std::uint8_t kRtcAddress   = 0x32;
inline constexpr std::uint8_t kImuAddress   = 0x68;
inline constexpr std::uint8_t kTouchAddress = 0x38;

enum class I2cSpeed : std::uint32_t {
    standard = 100000,
    fast     = 400000,
};

// Creates the shared PaperMono I2C bus. A repeated call is valid only when
// the requested speed matches the active bus.
esp_err_t initialize_i2c(I2cSpeed speed = I2cSpeed::standard);

// Borrows the shared bus. Every successful acquire must be paired with a
// release before the bus can be reconfigured or deleted.
esp_err_t acquire_i2c(i2c_bus_handle_t* bus);
esp_err_t release_i2c();

[[nodiscard]] I2cSpeed i2c_speed();

}  // namespace m5pm::board
