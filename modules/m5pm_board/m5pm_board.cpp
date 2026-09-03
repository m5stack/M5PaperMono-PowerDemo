#include "m5pm_board.hpp"

#include "freertos/FreeRTOS.h"

namespace m5pm::board {
namespace {

constexpr i2c_port_t kI2cPort = I2C_NUM_0;

i2c_bus_handle_t g_i2c_bus = nullptr;
I2cSpeed g_i2c_speed       = I2cSpeed::standard;
std::uint32_t g_i2c_claims = 0;
portMUX_TYPE g_state_lock  = portMUX_INITIALIZER_UNLOCKED;

i2c_bus_handle_t create_bus(I2cSpeed speed)
{
    i2c_config_t config     = {};
    config.mode             = I2C_MODE_MASTER;
    config.sda_io_num       = kI2cSda;
    config.scl_io_num       = kI2cScl;
    config.sda_pullup_en    = true;
    config.scl_pullup_en    = true;
    config.master.clk_speed = static_cast<std::uint32_t>(speed);
    config.clk_flags        = 0;
    return i2c_bus_create(kI2cPort, &config);
}

}  // namespace

esp_err_t initialize_i2c(I2cSpeed speed)
{
    portENTER_CRITICAL(&g_state_lock);
    if (g_i2c_bus != nullptr) {
        const bool speed_matches = g_i2c_speed == speed;
        portEXIT_CRITICAL(&g_state_lock);
        return speed_matches ? ESP_OK : ESP_ERR_INVALID_STATE;
    }
    portEXIT_CRITICAL(&g_state_lock);

    i2c_bus_handle_t bus = create_bus(speed);
    if (bus == nullptr) {
        return ESP_FAIL;
    }

    portENTER_CRITICAL(&g_state_lock);
    if (g_i2c_bus != nullptr) {
        portEXIT_CRITICAL(&g_state_lock);
        i2c_bus_delete(&bus);
        return ESP_ERR_INVALID_STATE;
    }
    g_i2c_bus   = bus;
    g_i2c_speed = speed;
    portEXIT_CRITICAL(&g_state_lock);
    return ESP_OK;
}

esp_err_t acquire_i2c(i2c_bus_handle_t* bus)
{
    if (bus == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    portENTER_CRITICAL(&g_state_lock);
    if (g_i2c_bus == nullptr) {
        portEXIT_CRITICAL(&g_state_lock);
        return ESP_ERR_INVALID_STATE;
    }
    ++g_i2c_claims;
    *bus = g_i2c_bus;
    portEXIT_CRITICAL(&g_state_lock);
    return ESP_OK;
}

esp_err_t release_i2c()
{
    portENTER_CRITICAL(&g_state_lock);
    if (g_i2c_claims == 0) {
        portEXIT_CRITICAL(&g_state_lock);
        return ESP_ERR_INVALID_STATE;
    }
    --g_i2c_claims;
    portEXIT_CRITICAL(&g_state_lock);
    return ESP_OK;
}

I2cSpeed i2c_speed()
{
    portENTER_CRITICAL(&g_state_lock);
    const I2cSpeed speed = g_i2c_speed;
    portEXIT_CRITICAL(&g_state_lock);
    return speed;
}

}  // namespace m5pm::board
