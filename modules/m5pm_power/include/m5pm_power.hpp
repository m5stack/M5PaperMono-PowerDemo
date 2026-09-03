#pragma once

#include <cstdint>

#include "M5PM1.h"
#include "esp_err.h"
#include "i2c_bus.h"

#ifndef M5PM_POWER_HAS_IOE1
#if __has_include("M5IOE1.h")
#define M5PM_POWER_HAS_IOE1 1
#else
#define M5PM_POWER_HAS_IOE1 0
#endif
#endif

#if M5PM_POWER_HAS_IOE1
#include "M5IOE1.h"
#endif

namespace m5pm::power {

struct PowerResult {
    esp_err_t error  = ESP_OK;
    int raw_error    = M5PM1_OK;
    const char* step = "none";

    [[nodiscard]] bool ok() const
    {
        return error == ESP_OK;
    }
};

struct WakeStatus {
    std::uint8_t pm1_wake       = 0;
    std::uint8_t pm1_gpio_irq   = 0;
    std::uint8_t pm1_system_irq = 0;
    std::uint8_t pm1_button_irq = 0;
};

class PowerController final {
public:
    PowerController()                                  = default;
    PowerController(const PowerController&)            = delete;
    PowerController& operator=(const PowerController&) = delete;

    // Borrows the board-owned I2C bus and initializes only M5PM1.
    [[nodiscard]] PowerResult begin(i2c_bus_handle_t bus, std::uint32_t speed_hz = 100000);

    // Initializes the PM1/IOE1 devices and establishes the PaperMono startup
    // state required by the L2 sequence.
    [[nodiscard]] PowerResult begin_l2(i2c_bus_handle_t bus, std::uint32_t speed_hz = 100000);

    // Applies the PaperMono L0 sequence: disable charging,
    // disable LDO hold, then request shutdown. No transaction is issued after
    // the shutdown command.
    [[nodiscard]] PowerResult enter_l0();

    // Applies the PaperMono L1 sequence: disable charging, retain the LDO,
    // suspend BMI270, then request shutdown. NFC is on the 3V3_L2 rail and is
    // not accessed in this L1 path. No transaction is issued after shutdown.
    [[nodiscard]] PowerResult enter_l1();

    // Applies the PaperMono no-wake L2 sequence and enters ESP32-S3 Deep
    // Sleep. Returns only when a preparation step fails.
    [[nodiscard]] PowerResult enter_l2();

    // Applies the hardware-validated no-wake L2 peripheral shutdown sequence
    // without putting IOE1, PM1, or the ESP32-S3 to sleep. Phase 2/3 tests
    // restore only their declared workload after this call.
    [[nodiscard]] PowerResult prepare_awake_l2();

    [[nodiscard]] PowerResult restore_display();
    [[nodiscard]] PowerResult restore_nfc();
    [[nodiscard]] PowerResult set_frontlight(std::uint8_t percent);
    [[nodiscard]] PowerResult power_off_display();
    [[nodiscard]] PowerResult restore_imu();
    [[nodiscard]] PowerResult restore_lora();
    [[nodiscard]] PowerResult power_off_lora();

    // Captures retained PM1 wake evidence without clearing it.
    [[nodiscard]] PowerResult capture_wake_status(WakeStatus* status);

    // Prepares the retained LDO domain and PM1 GPIO4 for BMI270 wake from
    // shutdown. BMI270 must be configured after this call and before entry.
    [[nodiscard]] PowerResult prepare_imu_wake_shutdown();

    // Requests PM1 shutdown as the final bus transaction. This must follow
    // prepare_imu_wake_shutdown() and BMI270 configuration.
    [[nodiscard]] PowerResult enter_imu_wake_shutdown();
    [[nodiscard]] PowerResult prepare_rtc_wake_shutdown();
    [[nodiscard]] PowerResult enter_rtc_wake_shutdown();

    // Applies the shared L2 preparation while preserving the BMI270 wake load.
    [[nodiscard]] PowerResult prepare_imu_wake_l2();

    // Arms the aggregate IRQ ext0 path and enters Deep Sleep. This must follow
    // prepare_imu_wake_l2() and BMI270 configuration.
    [[nodiscard]] PowerResult enter_imu_wake_l2();

    // Applies the shared L2 preparation, arms both active-low user buttons as
    // ESP32-S3 EXT1 wake sources, and enters Deep Sleep.
    [[nodiscard]] PowerResult enter_button_wake_l2();
    [[nodiscard]] PowerResult prepare_rtc_alarm_wake_l2();
    [[nodiscard]] PowerResult enter_rtc_alarm_wake_l2();
    [[nodiscard]] PowerResult prepare_rtc_wake_l2();
    [[nodiscard]] PowerResult enter_rtc_wake_l2();

private:
    enum class L2Mode : std::uint8_t {
        no_wake,
        imu_wake,
        button_wake,
        rtc_alarm_wake,
        rtc_wake,
    };

    [[nodiscard]] PowerResult result(m5pm1_err_t raw) const;
#if M5PM_POWER_HAS_IOE1
    [[nodiscard]] PowerResult result(m5ioe1_err_t raw) const;
#endif
    [[nodiscard]] PowerResult prepare_l2(L2Mode mode);
    [[nodiscard]] PowerResult finish_l2(L2Mode mode);
    [[nodiscard]] PowerResult prepare_shutdown_wake(m5pm1_gpio_num_t wake_pin);

    i2c_bus_handle_t bus_ = nullptr;
    M5PM1 pm1_;
#if M5PM_POWER_HAS_IOE1
    M5IOE1 ioe1_;
#endif
    bool initialized_           = false;
    bool ioe1_initialized_      = false;
    bool imu_shutdown_prepared_ = false;
    bool rtc_shutdown_prepared_ = false;
    bool l2_prepared_           = false;
    L2Mode l2_mode_             = L2Mode::no_wake;
};

}  // namespace m5pm::power
