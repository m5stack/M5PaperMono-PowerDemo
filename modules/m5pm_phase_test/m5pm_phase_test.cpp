#include "m5pm_phase_test.hpp"

#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "m5pm_board.hpp"
#include "m5pm_test_support.hpp"

namespace m5pm::phase_test {
namespace {
constexpr char kTag[] = "m5pm_phase";
}

Result AwakeL2::begin()
{
    gpio_deep_sleep_hold_dis();
    // M5IOE1 is powered independently and can retain enabled peripheral
    // outputs across an ESP32 brownout reset. Start the L2 cleanup immediately
    // so a retained display, NFC, or radio load cannot cause a reset loop.
    esp_err_t error = m5pm::board::initialize_i2c(m5pm::board::I2cSpeed::standard);
    if (error != ESP_OK) return {error, static_cast<int>(error), "i2c_initialize"};
    error = m5pm::board::acquire_i2c(&bus_);
    if (error != ESP_OK) return {error, static_cast<int>(error), "i2c_acquire"};
    m5pm::power::PowerResult power_result = power_.begin_l2(bus_);
    if (!power_result.ok()) return {power_result.error, power_result.raw_error, power_result.step};
    power_result = power_.prepare_awake_l2();
    if (!power_result.ok()) return {power_result.error, power_result.raw_error, power_result.step};
    return {};
}

void fail(const char* stage, esp_err_t error, int raw_error)
{
    ESP_LOGE(kTag, "BOARD_FAIL stage=%s error=%s raw=%d", stage, esp_err_to_name(error), raw_error);
    m5pm::test_support::report_final(m5pm::test_support::FinalStatus::fail, stage);
    m5pm::test_support::hold_safe_idle();
}

void wait_measurement_window(std::uint32_t duration_ms)
{
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
}

void pass_and_hold(const char* detail)
{
    m5pm::test_support::report_final(m5pm::test_support::FinalStatus::pass, detail);
    m5pm::test_support::hold_safe_idle();
}

esp_err_t configure_stop_buttons()
{
    constexpr gpio_num_t kButtons[] = {m5pm::board::kKey1, m5pm::board::kKey2};
    for (const gpio_num_t button : kButtons) {
        // prepare_awake_l2() uses the no-wake L2 baseline, which isolates and
        // holds both RTC-capable button GPIOs. Release that retained RTC state
        // before returning the pins to the digital GPIO peripheral.
        esp_err_t error = rtc_gpio_hold_dis(button);
        if (error != ESP_OK) return error;
        error = rtc_gpio_deinit(button);
        if (error != ESP_OK) return error;
        error = gpio_reset_pin(button);
        if (error != ESP_OK) return error;
    }
    gpio_config_t config = {};
    config.pin_bit_mask  = (1ULL << m5pm::board::kKey1) | (1ULL << m5pm::board::kKey2);
    config.mode          = GPIO_MODE_INPUT;
    config.pull_up_en    = GPIO_PULLUP_ENABLE;
    config.pull_down_en  = GPIO_PULLDOWN_DISABLE;
    config.intr_type     = GPIO_INTR_DISABLE;
    return gpio_config(&config);
}

bool stop_requested()
{
    return gpio_get_level(m5pm::board::kKey1) == 0 || gpio_get_level(m5pm::board::kKey2) == 0;
}

}  // namespace m5pm::phase_test
