#pragma once

#include <cstdint>

#include "esp_err.h"
#include "i2c_bus.h"
#include "m5pm_power.hpp"

namespace m5pm::phase_test {

struct Result {
    esp_err_t error  = ESP_OK;
    int raw_error    = 0;
    const char* step = "none";

    [[nodiscard]] bool ok() const
    {
        return error == ESP_OK;
    }
};

class AwakeL2 final {
public:
    [[nodiscard]] Result begin();
    [[nodiscard]] i2c_bus_handle_t bus() const
    {
        return bus_;
    }
    [[nodiscard]] m5pm::power::PowerController& power()
    {
        return power_;
    }

private:
    i2c_bus_handle_t bus_ = nullptr;
    m5pm::power::PowerController power_;
};

[[noreturn]] void fail(const char* stage, esp_err_t error, int raw_error = 0);
[[noreturn]] void pass_and_hold(const char* detail = "measurement_window_complete");
void wait_measurement_window(std::uint32_t duration_ms = 60000);
esp_err_t configure_stop_buttons();
bool stop_requested();

}  // namespace m5pm::phase_test
