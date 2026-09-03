#pragma once

#include <cstdint>

#include "bmi2_defs.h"
#include "esp_err.h"
#include "i2c_bus.h"

namespace m5pm::imu {

struct Sample {
    bmi2_sens_axes_data acceleration = {};
    bmi2_sens_axes_data gyroscope    = {};
};

class Sampler final {
public:
    esp_err_t begin(i2c_bus_handle_t bus);
    esp_err_t read(Sample* sample);

private:
    static std::int8_t read_bus(std::uint8_t address, std::uint8_t* data, std::uint32_t length, void* context);
    static std::int8_t write_bus(std::uint8_t address, const std::uint8_t* data, std::uint32_t length, void* context);
    static void delay_us(std::uint32_t period, void* context);

    i2c_bus_device_handle_t device_ = nullptr;
    bmi2_dev bmi_                   = {};
    bool ready_                     = false;
};

}  // namespace m5pm::imu
