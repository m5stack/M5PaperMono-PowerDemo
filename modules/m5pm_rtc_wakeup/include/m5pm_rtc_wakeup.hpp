#pragma once

#include <cstddef>
#include <cstdint>

#include "esp_err.h"
#include "i2c_bus.h"

namespace m5pm::wakeup {

struct RtcFlags {
    std::uint8_t raw = 0;

    [[nodiscard]] bool alarm() const
    {
        return (raw & (1U << 3)) != 0U;
    }

    [[nodiscard]] bool timer() const
    {
        return (raw & (1U << 4)) != 0U;
    }

    [[nodiscard]] bool update() const
    {
        return (raw & (1U << 5)) != 0U;
    }

    [[nodiscard]] bool wake_event() const
    {
        return alarm() || timer() || update();
    }
};

struct RtcResult {
    esp_err_t error  = ESP_OK;
    int raw_error    = 0;
    const char* step = "none";

    [[nodiscard]] bool ok() const
    {
        return error == ESP_OK;
    }
};

class RtcWakeup final {
public:
    RtcWakeup() = default;
    ~RtcWakeup();

    RtcWakeup(const RtcWakeup&)            = delete;
    RtcWakeup& operator=(const RtcWakeup&) = delete;

    [[nodiscard]] RtcResult begin(i2c_bus_handle_t bus, std::uint32_t speed_hz = 100000);
    [[nodiscard]] RtcResult read_flags(RtcFlags* flags) const;

    // Sets 2025-09-27 12:00:00, clears latched events, disables all RTC
    // interrupt generation, and verifies that alarm and timer remain clear.
    // The periodic update flag may latch again after the clock restarts.
    [[nodiscard]] RtcResult configure_button_baseline();
    [[nodiscard]] RtcResult configure_alarm_120s();
    [[nodiscard]] RtcResult configure_timer_120s();

    [[nodiscard]] RtcResult release_bus();

private:
    [[nodiscard]] RtcResult read(std::uint8_t reg, std::uint8_t* data, std::size_t length, const char* step) const;
    [[nodiscard]] RtcResult write(std::uint8_t reg, const std::uint8_t* data, std::size_t length, const char* step);

    i2c_bus_device_handle_t device_ = nullptr;
};

}  // namespace m5pm::wakeup
