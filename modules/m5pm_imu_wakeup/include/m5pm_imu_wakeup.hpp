#pragma once

#include <cstdint>

#include "bmi2_defs.h"
#include "esp_err.h"
#include "i2c_bus.h"

namespace m5pm::wakeup {

enum class ImuWakeProfile : std::uint8_t {
    low_power,
    reference,
};

struct ImuStatus {
    std::uint16_t raw = 0;
    bool any_motion   = false;
};

struct ImuResult {
    esp_err_t error  = ESP_OK;
    int raw_error    = 0;
    const char* step = "none";

    [[nodiscard]] bool ok() const
    {
        return error == ESP_OK;
    }
};

class ImuWakeup final {
public:
    ImuWakeup()                            = default;
    ImuWakeup(const ImuWakeup&)            = delete;
    ImuWakeup& operator=(const ImuWakeup&) = delete;

    // Reads retained feature status before begin() resets the BMI270.
    [[nodiscard]] static ImuResult read_retained_status(i2c_bus_handle_t bus, ImuStatus* status);

    // Initializes the BMI270 Base API on the board-owned I2C bus.
    [[nodiscard]] ImuResult begin(i2c_bus_handle_t bus);

    // Configures the selected motion-wake sensor load and maps Any-motion to INT1.
    // The low-power profile keeps only the accelerometer running for wake detection.
    [[nodiscard]] ImuResult configure_any_motion(ImuWakeProfile profile = ImuWakeProfile::low_power,
                                                 std::uint8_t duration = 10, std::uint8_t threshold = 30);

    // Releases the software I2C device handle without changing BMI270 state.
    [[nodiscard]] ImuResult release_bus();

private:
    static std::int8_t read(std::uint8_t address, std::uint8_t* data, std::uint32_t length, void* context);
    static std::int8_t write(std::uint8_t address, const std::uint8_t* data, std::uint32_t length, void* context);
    static void delay_us(std::uint32_t period, void* context);

    [[nodiscard]] static ImuResult api_result(std::int8_t raw, const char* step);

    i2c_bus_device_handle_t device_ = nullptr;
    bmi2_dev bmi_                   = {};
    bool initialized_               = false;
};

}  // namespace m5pm::wakeup
